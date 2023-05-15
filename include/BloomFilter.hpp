/**
 * @file BloomFilter.hpp
 * @author BojunRen (albert.cauchy725@gmail.com)
 * @brief Define a serializable DS bloom filter.
 * @date 2023-05-14
 *
 * @copyright Copyright (c) 2023, All Rights Reserved.
 *
 */
#ifndef BLF_CLASS
#define BLF_CLASS

#include <array>
#include <istream>
#include <ostream>
#include "MurmurHash3.h"
#include "basic.hpp"

#ifndef NDEBUG
#include <type_traits>
#endif

namespace basic_ds {

// Fixed size bloom filter. By default, it can be written into a binary file and occupy 10 KB.
template <size_type _Size = BLF_SIZE, typename Key = uint64_t, typename CharT = char>
class BloomFilter {
public:
    explicit BloomFilter() : table{} {}
    ~BloomFilter() = default;

    void insert(Key k) noexcept {
        getHash(k);
        for (auto x : hash_buf) {
            size_type idx = x / (sizeof(CharT) * 8);
            int offset = x % (sizeof(CharT) * 8);
            table[idx] |= (1u << offset);
        }
    }

    bool contains(Key k) const noexcept {
        getHash(k);
        bool f = true;
        for (auto x : hash_buf) {
            size_type idx = x / (sizeof(CharT) * 8);
            int offset = x % (sizeof(CharT) * 8);
            f &= table[idx] >> offset;
        }
        return f;
    }

    constexpr size_type byte_size() const noexcept {
        return _Size;
    }

private:
    // The std-like stream operator overloads.
    template <typename Traits>
    friend std::basic_ostream<char, Traits> &operator<<(
        std::basic_ostream<char, Traits> &os, const BloomFilter &bft) {
        os.write(bft.table.data(), _Size);
        return os;
    }

    template <class Traits>
    friend std::basic_istream<char, Traits> &operator>>(
        std::basic_istream<char, Traits> &is, BloomFilter &bft) {
        is.read(bft.table.data(), _Size);
        return is;
    }

    // `std::array::data()` ensures:
    // The pointer is such that range [data(); data()+size()) is always a valid range.
    std::array<CharT, _Size> table;

    // Designed by the given hash function.
    static std::array<uint32_t, 4> hash_buf;

    void getHash(Key k) const noexcept {
#ifndef NDEBUG
        static_assert(
            std::is_same<Key, uint64_t>::value,
            "Current implements of class BloomFilter only support key type uint64_t!");
#endif
        MurmurHash3_x64_128(&k, sizeof(k), 1, hash_buf.data());
        for (auto &x : hash_buf) {
            x %= _Size * sizeof(CharT) * 8;  // 1 byte = 8 bits
        }
    }
};

template <size_type _Size, typename Key, typename CharT>
std::array<uint32_t, 4> BloomFilter<_Size, Key, CharT>::hash_buf{};

}  // namespace basic_ds

#endif