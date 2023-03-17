#include <cstdint>
#include <string>
#include <limits>

typedef uint64_t Key;
typedef std::string Val;

// typedef int32_t int_t;

#define KEY_MAX UINT64_MAX
#define KEY_MIN 0u

#define VAL_INVALID std::string("~INVALID~")
#define VAL_DELETE std::string("~DELETED~")

#define MAX_MEMORY (1 << 24) // 2MB == 2 * 1024 * 1024 * 8 bit