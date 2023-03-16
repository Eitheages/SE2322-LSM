/**
 * @file BloomFilter.h
 * @author BojunRen (albert.cauchy725@gmail.com)
 * @attention Pre-define __BLOOM_VERSION__ to use the header
 *            Version 1 has not been checked yet.
 *
 * @date 2023-03-16
 *
 * @copyright Copyright (c) 2023, All Rights Reserved.
 *
 */
#if defined(__BLOOM_VERSION__) && !defined(__BLOOM_FILTER__)

#define __BLOOM_FILTER__

#include <fstream>
#include <vector>
#include "MurmurHash3.h"
#include "types.h"
#define MAX_SIZE (10240 * 8) // 10 KB
// #define MAX_SIZE (128)  // test

#if (__BLOOM_VERSION__ == 1)
typedef std::vector<bool> Bitset;
static Bitset __bs(MAX_SIZE, 0);

#elif (__BLOOM_VERSION__ == 2)
#include <bitset>
typedef std::bitset<MAX_SIZE> Bitset;
static Bitset __bs;  // default: all 0
#endif

static uint32_t __hash[4];

#define BLOOM_GET_HASH(key)                            \
    MurmurHash3_x64_128(&key, sizeof(key), 1, __hash); \
    for (int i = 0; i < 4; ++i)                        \
    __hash[i] %= MAX_SIZE

#if (__BLOOM_VERSION__ == 1)

#define BLOOM_INSERT(key)       \
    BLOOM_GET_HASH(key);        \
    for (int i = 0; i < 4; ++i) \
    __bs[__hash[i]] = 1

/**
 * @attention Remember to invoke MACRO BLOOM_GET_HASH first
 * @return boolean
 */
#define BLOOM_EXIST(key) \
    (__bs[__hash[0]] && __bs[__hash[1]] && __bs[__hash[2]] && __bs[__hash[3]])

#define BLOOM_RESET() __bs.assign(MAX_SIZE, 0)

#elif (__BLOOM_VERSION__ == 2)

#define BLOOM_INSERT(key)       \
    BLOOM_GET_HASH(key);        \
    for (int i = 0; i < 4; ++i) \
    __bs.set(__hash[i])

/**
 * @attention Remember to invoke MACRO BLOOM_GET_HASH first
 * @return boolean
 */
#define BLOOM_EXIST(key)                                                     \
    (__bs.test(__hash[0]) && __bs.test(__hash[1]) && __bs.test(__hash[2]) && \
     __bs.test(__hash[3]))

#define BLOOM_RESET() __bs->reset()

#endif  // check __BLOOM_VERSION__

template <std::size_t N>
class BitBuffer {
    static_assert((N & 07) == 0,
                  "Template parameter N of BitBuffer must be a multiple of 8!");

    typedef uint8_t Byte;  // write byte by byte

    template <std::size_t X>
    friend std::ostream &operator<<(std::ostream &os, const BitBuffer<X> &obj);

    template <std::size_t X>
    friend std::istream &operator>>(std::istream &is, BitBuffer<X> &obj);

public:
    BitBuffer() noexcept : _buffer((N + 7) >> 3) {}

    BitBuffer(const std::bitset<N> &bs) noexcept : _buffer((N + 7) >> 3) {
        add(bs);
    }

    constexpr void add(const std::bitset<N> &bs) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            _buffer[i >> 3] |= (bs[i] << (i & 07));
        }
    }

    ~BitBuffer() = default;
    BitBuffer(const BitBuffer &) = delete;
    BitBuffer(BitBuffer &&) = delete;

private:

    std::vector<Byte> _buffer;
    /**
     * @attention This function should be used very carefully
     *                  since it emits most of the checks.
     */
    void read_from(std::istream &is) noexcept {
        // _buffer.assign(_buffer.size(), 0);
        is.read(reinterpret_cast<char*>(&_buffer[0]), N >> 3);
    }
};

template <std::size_t N>
std::ostream &operator<<(std::ostream &os, const BitBuffer<N> &obj) {
    // for (const auto& byte : obj._buffer)
    //     os << byte;
    os.write(reinterpret_cast<const char*>(&obj._buffer[0]), N >> 3);
    return os;
}

template <std::size_t N>
std::istream &operator>>(std::istream &is, BitBuffer<N> &obj) {
    obj.read_from(is);
    return is;
}

#endif