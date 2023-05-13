#ifndef BLF_CLASS
#define BLF_CLASS

#include <array>
#include <cstddef>
#include "MurmurHash3.h"

#ifndef NDEBUG
#include <type_traits>
#endif

namespace basic_ds {

using IndexT = std::size_t;
constexpr IndexT BLF_SIZE = 10240;

template <typename Key = uint64_t, IndexT N = BLF_SIZE, typename CharT = char>
class BloomFilter {
public:
    BloomFilter() : table{} {}
    ~BloomFilter() = default;

    void insert(Key k) noexcept {
        getHash(k);
        for (auto x : hash_buf) {
            IndexT idx = x / sizeof(CharT);
            int offset = x % sizeof(CharT);
            table[idx] |= 1u << offset;
        }
    }

    bool contains(Key k) noexcept {
        getHash(k);
        bool f = true;
        for (auto x : hash_buf) {
            IndexT idx = x / sizeof(CharT);
            int offset = x % sizeof(CharT);
            f &= table[idx] >> offset;
        }
        return f;
    }

private:
    std::array<CharT, BLF_SIZE> table;
    std::array<uint32_t, 4> hash_buf;

    void getHash(Key k) noexcept {
#ifndef NDEBUG
        static_assert(
            std::is_same<Key, uint64_t>::value,
            "Current implements of class BloomFilter only support key type uint64_t!");
#endif
        MurmurHash3_x64_128(&k, sizeof(k), 1, hash_buf.data());
        for (auto &x : hash_buf) {
            x %= BLF_SIZE * sizeof(CharT);
        }
    }
};
}  // namespace basic_ds

#endif