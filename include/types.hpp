#ifndef LSM_TYPES
#define LSM_TYPES

#include <cstdint>
#include <cstdlib>
#include <string>

namespace lsm {
using size_type = std::size_t;
using key_type = uint64_t;
using value_type = std::string;
using offset_type = uint32_t;

constexpr size_type BLF_SIZE = 10240;
constexpr size_type MTB_MAXSIZE = 2 * 1024 * 1024; /* 2 MB */
};                                                 // namespace lsm

#endif