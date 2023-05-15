#ifndef MEMTABLE_CLASS
#define MEMTABLE_CLASS

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

    void to_binary() {
        // TODO to binary
        // return;
    }

    void put(const key_type &key, const val_type &val) noexcept {

        size_type future_size = this->predict_byte_size(key, val);

        dst.insert_or_assign(key, val);
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
    }

    std::pair<key_type, key_type> range() const noexcept {
        return this->_range;
    }

    size_type byte_size() const noexcept {
        return this->_byte;
    }

    size_type predict_byte_size(const key_type &key, const val_type &val) const noexcept {
        auto get_result = this->get(key);
        if (!get_result.second) {
            return this->_byte + MemTable<key_type>::predict_insert_size(key, val);
        }
        return this->_byte +
               MemTable<key_type>::predict_update_size(key, val, get_result.first);
    }

    inline static size_type predict_insert_size(const key_type &key,
                                                const val_type &val) noexcept {
        return (sizeof(key_type) + sizeof(offset_type) +
                (val.length() + 1 /* null-terminated */) * sizeof(char));
    }

    inline static size_type predict_update_size(const key_type &key, const val_type &val,
                                                const val_type &pre_val) noexcept {
        return (val.length() - pre_val.length()) * sizeof(char);
    }

    bool in_range(const key_type &key) const noexcept {
        return key <= _range.second && key >= _range.first;
    }

    std::pair<val_type, bool> get(const key_type &key) const noexcept {
        val_type found = ""s;
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
};

template <typename _Key>
const LSM_ValType MemTable<_Key>::DeleteNote = "~DELETED~"s;

template <typename _Key>
constexpr typename MemTable<_Key>::size_type MemTable<_Key>::HEADER_SIZE;

template <typename _Key>
constexpr typename MemTable<_Key>::size_type MemTable<_Key>::MAXSIZE;

}  // namespace mtb
#endif