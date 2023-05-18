#ifndef MEMTABLE_CLASS
#define MEMTABLE_CLASS

#include <fstream>
#include "BloomFilter.hpp"
#include "SkipList.hpp"
#include "sst.hpp"
#include "types.hpp"

namespace mtb {

using namespace std::string_literals;
// Only for value_type = std::string
class MemTable {
public:
    using key_type = lsm::key_type;
    using val_type = lsm::value_type;
    using size_type = lsm::size_type;
    using kv_type = std::pair<key_type, val_type>;
    using offset_type = lsm::offset_type;

    explicit MemTable()
        : _time_stamp(1), _count(0), _range{1, 0}, _byte(HEADER_SIZE + lsm::BLF_SIZE) {}

    explicit MemTable(uint64_t ts)
        : _time_stamp(ts), _count(0), _range{1, 0}, _byte(HEADER_SIZE + lsm::BLF_SIZE) {}

    ~MemTable() = default;  // nothing todo

    // This method is a little dangerous, since it throw an exception
    sst::sst_cache to_binary(const std::string &bin_name, int level) const {
        std::ofstream bin_out{bin_name, std::ios::binary};  // Trunc
        if (!bin_out) {
            throw std::runtime_error {
                "Cannot write sst " + bin_name + ". Please check if the directory exists."
            };
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
        decltype(sst::sst_cache{}.indices) indices;
        indices.reserve(this->_count);

        offset_type offset = HEADER_SIZE + lsm::BLF_SIZE +
                             this->_count * (sizeof(key_type) + sizeof(offset_type));
        for (kv_type &kv : kv_list) {
            bin_out.write(reinterpret_cast<const char *>(&kv.first), sizeof(key_type))
                .write(reinterpret_cast<const char *>(&offset), sizeof(offset_type));
            indices.emplace_back(kv.first, offset);
            offset += kv.second.length() + 1;  // null-terminated
        }

        // Write the value data
        for (kv_type &kv : kv_list) {
            bin_out.write(kv.second.c_str(), kv.second.length() + 1);
        }

        return {
            level,
            {this->_time_stamp, this->_count, this->_range.first, this->_range.second},
            this->bft,
            std::move(indices),
            {bin_name}};
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
            return this->_byte + MemTable::predict_insert_size(key, val);
        }
        return this->_byte + MemTable::predict_update_size(key, val, get_result.first);
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
    static constexpr size_type HEADER_SIZE = 32;

    /** 32 bytes in the header */
    uint64_t _time_stamp;
    uint64_t _count;
    std::pair<key_type, key_type> _range;

    /** Size in byte (when stored as sst) */
    size_type _byte;

    /** Basic data structure */
    basic_ds::SkipList<key_type, val_type> dst;  // dynamic search table
    basic_ds::BloomFilter<lsm::BLF_SIZE> bft;    // bloom filter

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
// const lsm::value_type MemTable::DeleteNote = "~DELETED~"s;
// constexpr typename MemTable::size_type MemTable::HEADER_SIZE;

}  // namespace mtb
#endif