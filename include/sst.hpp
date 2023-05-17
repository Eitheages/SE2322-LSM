#ifndef SST_UTILS
#define SST_UTILS

#include <fstream>
#include <memory>
#include <vector>

#include "BloomFilter.hpp"
#include "types.hpp"

namespace sst {

// Cache for sst files, stored in the memory.
// It's an aggregate, moveable type.
struct sst_cache {
    using key_type = lsm::key_type;
    using value_type = lsm::value_type;
    using kv_type = std::pair<key_type, value_type>;
    using offset_type = lsm::offset_type;

    struct sst_header {
        // 32-byte head
        uint64_t time_stamp, count, lower, upper;  // The header
    };

    // Variables
    int level;
    struct sst_header header;
    basic_ds::BloomFilter<lsm::BLF_SIZE> bft;  // bloom filter is designed to be moveable.
    std::vector<std::pair<lsm::key_type, lsm::offset_type>> indices;
    std::string sst_path;

    // Read the associated sst file and return the value from offset.
    value_type from_offset(offset_type offset) const {
        std::ifstream in{sst_path};
        in.seekg(offset, std::ios::beg);
        std::string str;
        for (char c; (in >> c) && c;) {
            str.append(1, c);
        }
        return str;
    }

    // Search the key in indices. If found, return the offset and bool flag `true`.
    std::pair<offset_type, bool> search(key_type key) const {
        if (!(this->header.lower <= key && key <= this->header.upper) ||
            !this->bft.contains(key)) {
            return {0, false};
        }
        using pair_type = decltype(indices)::value_type;
        auto it = std::lower_bound(indices.begin(), indices.end(), pair_type{key, 0});
        if (it == indices.cend()) {
            return {0, false};
        }
        return {it->second, true};
    }

    bool operator<(const sst_cache &other) const {
        return this->header.time_stamp < other.header.time_stamp;
    }

    bool operator>(const sst_cache &other) const {
        return other < *this;
    }
};

// A wrapper structure to read from sst files.
// Used when initialize the memory table from current ssts.
struct sst_reader {
    using key_type = lsm::key_type;
    using value_type = lsm::value_type;
    using kv_type = std::pair<key_type, value_type>;
    using offset_type = lsm::offset_type;

    uint64_t time_stamp, count, lower, upper;  // The header
    std::vector<std::pair<key_type, offset_type>> indices;
    basic_ds::BloomFilter<lsm::BLF_SIZE> bft;
    bool is_success;

    sst_reader() = delete;
    sst_reader(sst_reader &&) = delete;
    sst_reader(const sst_reader &) = delete;
    explicit sst_reader(const char *sst_name) : is_success(false) {
        std::ifstream in{sst_name};
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
        for (auto &index : indices) {
            in.read(reinterpret_cast<char *>(&index),
                    sizeof(key_type) + sizeof(offset_type));
            if (!in.good()) {
                return;
            }
        }
        is_success = true;
    }
};

/**
 * @brief Read sst file, return the cache.
 *
 * @param sst_path
 * @param level
 * @return sst_cache, level -1 indicates the read is failed or the given argument is invalid.
 */
inline sst_cache read_sst(const std::string &sst_path, int level) {
    if (level < 0) {
        return {-1};
    }
    sst_reader sr{sst_path.c_str()};
    if (!sr.is_success) {
        return {-1};
    }

    sst_cache res{level,
                  {sr.time_stamp, sr.count, sr.lower, sr.upper},
                  std::move(sr.bft),
                  std::move(sr.indices),
                  std::move(sst_path)};
    // sst_cache res;
    // res.indices = std::move(sr.indices);
    // res.header = {sr.time_stamp, sr.count, sr.lower, sr.upper};
    // res.level = level;
    // res.sst_path = sst_path;
    // std::swap(res.bft, sr.bft);

    return res;
}

}  // namespace sst
#endif