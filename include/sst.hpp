#include <fstream>
#include <vector>

#include "types.hpp"
#include "BloomFilter.hpp"

namespace sst {

struct sst_header {
    // 32-byte head
    uint64_t time_stamp, count, lower, upper;  // The header
};

// A wrapper structure to read from sst files.
// Only for value_type = std::string
struct sst_reader {
    using key_type = lsm::key_type;
    using value_type = std::string;
    using kv_type = std::pair<key_type, value_type>;
    using offset_type = uint32_t;

    uint64_t time_stamp, count, lower, upper;  // The header
    std::vector<std::pair<key_type, offset_type>> indices;
    basic_ds::BloomFilter<lsm::BLF_SIZE> bft;
    bool is_success;

    sst_reader() = delete;
    sst_reader(sst_reader&&) = delete;
    sst_reader(const sst_reader&) = delete;
    explicit sst_reader(const char *sst_name) : is_success(false), in{sst_name} {
        if (!in) {
            return;
        }
        std::tuple<uint64_t, uint64_t, uint64_t, uint64_t> header;
        in.read(reinterpret_cast<char *>(&header), 32);
        if (!in.good()) {
            return;
        }
        std::tie(time_stamp, count, lower, upper) = header;
        in >> bft;
        if (!in.good()) {
            return;
        }

        indices = decltype(indices)(count);
        for (auto& index : indices) {
            in.read(reinterpret_cast<char *>(&index),
                    sizeof(key_type) + sizeof(offset_type));
            if (!in.good()) {
                return;
            }
        }
        is_success = true;
    }
    value_type read_from_offset(offset_type offset) {
        in.seekg(offset, std::ios::beg);
        std::string str;
        for (char c; (in >> c) && c;) {
            str.append(1, c);
        }
        is_success &= static_cast<bool>(in);
        return str;
    }

private:
    std::ifstream in;  // The associated binary file (sst)
};

// Cache for sst files, stored in the memory.
struct sst_cache {
    uint32_t level;
    std::string sst_name;
    struct sst_header header;
    std::vector<std::pair<lsm::key_type, lsm::offset_type>> indices;
};
}