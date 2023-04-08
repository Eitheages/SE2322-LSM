#include <cstdint>
#include <string>
#include <limits>

using Key = uint64_t;
using Val = std::string;

constexpr auto KEY_MAX =  UINT64_MAX;
constexpr auto KEY_MIN =  0ull;

#define VAL_INVALID std::string("~INVALID~")
#define VAL_DELETE std::string("~DELETED~")

constexpr int32_t MAX_MEMORY = (2 * 1024 * 1024 * 8); // 2MB == 2 * 1024 * 1024 * 8 bit
constexpr int32_t BLOOM_FILTER_SIZE = 10240 * 8; // 10240 bytes