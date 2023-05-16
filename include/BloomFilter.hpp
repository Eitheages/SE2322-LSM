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

#include <vector>
#include <array>
#include <istream>
#include <ostream>
#include <type_traits>

#include "MurmurHash3.h"
namespace basic_ds {
using size_type = std::size_t;
// Fixed size bloom filter. By default, it can be written into a binary file and occupies 10 KB.
// Only for key_type = uint64_t.
template <size_type _Size>
class BloomFilter {
public:
    using key_type = uint64_t;
    using CharT = char;  // The inner type to store boolean.
    BloomFilter() : table(_Size, 0) {}
    ~BloomFilter() = default;

    void insert(key_type k) noexcept {
        auto hash_buf = getHash(k);
        for (auto x : hash_buf) {
            size_type idx = x / (sizeof(CharT) * 8);
            int offset = x % (sizeof(CharT) * 8);
            table[idx] |= (1u << offset);
        }
    }

    bool contains(key_type k) const noexcept {
        auto hash_buf = getHash(k);
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

    template <typename Traits>
    friend std::basic_istream<char, Traits> &operator>>(
        std::basic_istream<char, Traits> &is, BloomFilter &bft) {
        is.read(bft.table.data(), _Size);
        return is;
    }

    // Improve: use std::vector to make the bloom filter moveable.
    // `std::vector::data()` ensures:
    // The pointer is such that range [data(); data()+size()) is always a valid range.
    std::vector<CharT> table;  // Use char type to store the boolean.

    auto getHash(key_type k) const noexcept {
        // #ifndef NDEBUG
        //         static_assert(
        //             std::is_same<KeyType, uint64_t>::value,
        //             "Current implements of class BloomFilter only support key type uint64_t!");
        // #endif
        std::array<uint32_t, 4> hash_buf{};
        MurmurHash3_x64_128(&k, sizeof(k), 1, hash_buf.data());
        for (auto &x : hash_buf) {
            x %= _Size * sizeof(CharT) * 8;  // 1 byte = 8 bits
        }
        return hash_buf;
    }
};

}  // namespace basic_ds

#endif