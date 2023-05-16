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
    explicit BloomFilter() : table{} {}
    ~BloomFilter() = default;

    void insert(key_type k) noexcept {
        getHash(k);
        for (auto x : hash_buf) {
            size_type idx = x / (sizeof(CharT) * 8);
            int offset = x % (sizeof(CharT) * 8);
            table[idx] |= (1u << offset);
        }
    }

    bool contains(key_type k) const noexcept {
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

    template <typename Traits>
    friend std::basic_istream<char, Traits> &operator>>(
        std::basic_istream<char, Traits> &is, BloomFilter &bft) {
        is.read(bft.table.data(), _Size);
        return is;
    }

    // `std::array::data()` ensures:
    // The pointer is such that range [data(); data()+size()) is always a valid range.
    std::array<CharT, _Size> table;  // Use char type to store the boolean.

    // Designed by the given hash function.
    static std::array<uint32_t, 4> hash_buf;

    void getHash(key_type k) const noexcept {
        // #ifndef NDEBUG
        //         static_assert(
        //             std::is_same<KeyType, uint64_t>::value,
        //             "Current implements of class BloomFilter only support key type uint64_t!");
        // #endif
        MurmurHash3_x64_128(&k, sizeof(k), 1, hash_buf.data());
        for (auto &x : hash_buf) {
            x %= _Size * sizeof(CharT) * 8;  // 1 byte = 8 bits
        }
    }
};

template <size_type _Size>
std::array<uint32_t, 4> BloomFilter<_Size>::hash_buf{};

}  // namespace basic_ds

#endif