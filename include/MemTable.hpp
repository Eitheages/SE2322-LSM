#ifndef MEMTABLE_CLASS
#define MEMTABLE_CLASS

#include <fstream>
#include "BloomFilter.hpp"
#include "SkipList.hpp"

namespace mtb {

// This template is only for value type std::string.
template <typename _Key = LSM_KeyType>
class MemTable {
public:
    using key_type = _Key;
    using val_type = LSM_ValType;
    using size_type = basic_ds::size_type;
    using kv_type = std::pair<key_type, val_type>;

    explicit MemTable()
        : _time_stamp(1),
          _count(0),
          _range{1, 0},
          _byte(HEADER_SIZE + basic_ds::BLF_SIZE) {}

    ~MemTable() = default;  // TODO

    // Returns: -1 on failure (the directory does not exist), 0 on success.
    int to_binary(const char *bin_name) const {
        std::ofstream bin_out{bin_name, std::ios::binary};  // Trunc
        if (!bin_out) {
            return -1;
        }

        // Write the header
        bin_out
            .write(reinterpret_cast<const char *>(&this->_time_stamp),
                   sizeof this->_time_stamp)
            .write(reinterpret_cast<const char *>(&this->_count), sizeof this->_count)
            .write(reinterpret_cast<const char *>(&this->_range), sizeof this->_range);

        // Write the bloom filter
        bin_out << this->bft;

        auto kv_list = this->dst.get_kv();

        // The below implements are value_type-dependent

        // Write the index table
        offset_type offset = HEADER_SIZE + basic_ds::BLF_SIZE +
                             this->_count * (sizeof(key_type) + sizeof(offset_type));
        for (kv_type &kv : kv_list) {
            bin_out.write(reinterpret_cast<const char *>(&kv.first), sizeof(key_type))
                .write(reinterpret_cast<const char *>(&offset), sizeof(offset_type));
            offset += kv.second.length() + 1;  // null-terminated
        }

        // Write the value data
        for (kv_type &kv : kv_list) {
            bin_out.write(kv.second.c_str(), kv.second.length() + 1);
        }

        return 0;
    }

    int to_binary(const std::string &bin_name) const noexcept {
        return this->to_binary(bin_name.c_str());
    }

    void put(const key_type &key, const val_type &val) noexcept {

        size_type future_size = this->predict_byte_size(key, val);

        bool is_inserted = dst.insert_or_assign(key, val).second;
        bft.insert(key);

        // Update range
        if (_range.first > _range.second) {
            // The first key to insert, or not initialized yet.
            _range = std::make_pair(key, key);
        } else {
            _range.first = std::min(_range.first, key);
            _range.second = std::max(_range.second, key);
        }

        // Update byte
        this->_byte = future_size;

        // Update count
        this->_count += static_cast<int>(is_inserted);
    }

    std::pair<key_type, key_type> range() const noexcept {
        return this->_range;
    }

    size_type byte_size() const noexcept {
        return this->_byte;
    }

    // Predict the byte size after insert/merge the given (key, value).
    size_type predict_byte_size(const key_type &key, const val_type &val) const noexcept {
        auto get_result = this->get(key);
        if (!get_result.second) {
            return this->_byte + MemTable<key_type>::predict_insert_size(key, val);
        }
        return this->_byte +
               MemTable<key_type>::predict_update_size(key, val, get_result.first);
    }

    bool in_range(const key_type &key) const noexcept {
        return key <= _range.second && key >= _range.first;
    }

    /**
     * @brief Get the value from the memory table.
     *
     * @param key
     * @return std::pair<val_type, bool> consisting of the value found in the memory table and
     *         a bool flag denotes whether the found result is valid (whether the key exists).
     */
    std::pair<val_type, bool> get(const key_type &key) const noexcept {
        val_type found = val_type{};  // Use the default initialization.
        if (!this->in_range(key) || !bft.contains(key)) {
            return {found, false};
        }
        auto p = this->dst.find(key);
        if (!p) {
            return {found, false};
        }
        found = p->val;
        return {found, true};
    }

    size_type size() const noexcept {
        return this->_count;
    }

private:
    using offset_type = uint32_t;
    static const val_type DeleteNote; /* ~DELETE~ */
    static constexpr size_type HEADER_SIZE = 32;
    static constexpr size_type MAXSIZE = 2 * 1024 * 1024; /* 2 MB */

    /** 32 bytes in the header */
    uint64_t _time_stamp;
    uint64_t _count;
    std::pair<key_type, key_type> _range;

    /** Size in byte (when stored as sst) */
    size_type _byte;

    /** Basic data structure */
    basic_ds::SkipList<key_type, val_type> dst;               // dynamic search table
    basic_ds::BloomFilter<basic_ds::BLF_SIZE, key_type> bft;  // bloom filter

    inline static size_type predict_insert_size(const key_type &key,
                                                const val_type &val) noexcept {
        return (sizeof(key_type) + sizeof(offset_type) +
                (val.length() + 1 /* null-terminated */) * sizeof(char));
    }

    inline static size_type predict_update_size(const key_type &key, const val_type &val,
                                                const val_type &pre_val) noexcept {
        return (val.length() - pre_val.length()) * sizeof(char);
    }
};

template <typename _Key>
const LSM_ValType MemTable<_Key>::DeleteNote = "~DELETED~"s;

template <typename _Key>
constexpr typename MemTable<_Key>::size_type MemTable<_Key>::HEADER_SIZE;

template <typename _Key>
constexpr typename MemTable<_Key>::size_type MemTable<_Key>::MAXSIZE;

// A wrapper structure to read from sst files.
template <typename _Key = LSM_KeyType>
struct sst_reader {
    using key_type = _Key;
    using value_type = std::string;
    using kv_type = std::pair<key_type, value_type>;
    using offset_type = uint32_t;

    uint64_t time_stamp, count, lower, upper;  // The header
    std::vector<std::pair<key_type, offset_type>> indices;
    basic_ds::BloomFilter<basic_ds::BLF_SIZE> bft;
    bool is_success;

    sst_reader() = delete;
    explicit sst_reader(const char *sst_name) : is_success(false), in{sst_name} {
        if (!in) {
            return;
        }
        std::tuple<uint64_t, uint64_t, uint64_t, uint64_t> header;
        in.read(reinterpret_cast<char *>(&header), 32);
        if (!in.good()) {
            return;
        }
        std::tie(time_stamp, count, lower, upper) = header;
        in >> bft;
        if (!in.good()) {
            return;
        }

        indices = decltype(indices)(count);
        for (auto& index : indices) {
            in.read(reinterpret_cast<char *>(&index),
                    sizeof(key_type) + sizeof(offset_type));
            if (!in.good()) {
                return;
            }
        }
        is_success = true;
    }
    value_type read_from_offset(offset_type offset) {
        in.seekg(offset, std::ios::beg);
        std::string str;
        for (char c; (in >> c) && c;) {
            str.append(1, c);
        }
        is_success &= static_cast<bool>(in);
        return str;
    }

private:
    std::ifstream in;  // The associated binary file (sst)
};

}  // namespace mtb
#endif