#ifndef BASIC_DEF
#define BASIC_DEF

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace mtb {
using namespace std::string_literals;
using LSM_KeyType = uint64_t;
using LSM_ValType = std::string;
}  // namespace mtb
namespace basic_ds {

using size_type = std::size_t;
constexpr size_type BLF_SIZE = 10240;

}  // namespace basic_ds
#endif