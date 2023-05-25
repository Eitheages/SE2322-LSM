#ifndef SST_UTILS
#define SST_UTILS

#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <vector>

#include "BloomFilter.hpp"
#include "types.hpp"
#include "utils.h"

namespace sst {

inline static std::string generate_hash() {
    static std::random_device random_device{};
    static std::mt19937 engine{random_device()};
    static std::uniform_int_distribution<> dist(0, 0xFFFFFF);

    uint32_t random_number = dist(engine);
    std::stringstream ss{};
    ss << std::setw(6) << std::setfill('0') << std::hex << random_number;
    return ss.str();
}

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
    std::string sst_path;  // The associated sst file (full path)

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
#ifdef TEST2
        if (!(this->header.lower <= key && key <= this->header.upper)) {
            return {0, false};
        }
#else
        if (!(this->header.lower <= key && key <= this->header.upper) || !this->bft.contains(key)) {
            return {0, false};
        }
#endif
        using pair_type = decltype(indices)::value_type;
        auto it = std::lower_bound(indices.begin(), indices.end(), pair_type{key, 0});
        if (it == indices.cend() || it->first != key) {
            return {0, false};
        }
        return {it->second, true};
    }

    bool operator<(const sst_cache &rhs) const {
        return std::tie(rhs.level, this->header.time_stamp, this->header.count) <
               std::tie(this->level, rhs.header.time_stamp, rhs.header.count);
    }

    bool operator>(const sst_cache &rhs) const {
        return rhs < *this;
    }

    std::vector<kv_type> get_kv() const {
        std::vector<kv_type> kv_list{};
        kv_list.reserve(this->header.count);
        std::ifstream in{sst_path};
        if (!in) {
            throw std::runtime_error{"Cannot open sst file " + sst_path};
        }
        in.seekg(this->indices[0].second);
        for (const auto &index : indices) {
            std::string buffer;
            std::getline(in, buffer, '\0');
            kv_list.emplace_back(index.first, std::move(buffer));
        }
        return kv_list;
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
            in.read(reinterpret_cast<char *>(&index), sizeof(key_type) + sizeof(offset_type));
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

    return {level,
            {sr.time_stamp, sr.count, sr.lower, sr.upper},
            std::move(sr.bft),
            std::move(sr.indices),
            std::move(sst_path)};
}

struct sst_buffer {
    using key_type = lsm::key_type;
    using value_type = lsm::value_type;
    using kv_type = std::pair<key_type, value_type>;

    std::vector<kv_type> kv_list;
    lsm::size_type byte_size;
    uint64_t timestamp;
    std::string target_dir;
    int level;

    sst_buffer(uint64_t _timestamp, const std::string &_dir)
        : byte_size(32 + lsm::BLF_SIZE),
          timestamp(_timestamp),
          target_dir(_dir),
          level(std::stoi(_dir.substr(target_dir.find('-') + 1))) {
        if (utils::mkdir(target_dir.c_str()) != 0) {
            throw std::runtime_error{"Cannot create directory " + target_dir};
        }
    }

    // I'd like to use unique_ptr. However, copy elision isn't mandatory in C++14.
    sst_cache *append(key_type key, value_type value) {
        auto tmp_size = this->byte_size + sizeof(key_type) + sizeof(lsm::offset_type) +
                        (value.length() + 1) * sizeof(char);
        if (tmp_size <= lsm::MTB_MAXSIZE) {
            this->byte_size = tmp_size;
            kv_list.emplace_back(key, value);
            return nullptr;
        }

        auto *cache_ptr = to_binary();

        this->byte_size +=
            sizeof(key_type) + sizeof(lsm::offset_type) + (value.length() + 1) * sizeof(char);
        this->kv_list = {{key, value}};

        return cache_ptr;
    }

    sst_cache *clear() {
        if (this->kv_list.empty()) {
            return nullptr;
        }
        return to_binary();
    }

private:
    // Will clear the kv_list and reset byte_size.
    sst_cache *to_binary() {
#ifndef NDEBUG
        bool flag = std::is_sorted(
            kv_list.begin(), kv_list.end(),
            [](const kv_type &kv1, const kv_type &kv2) -> bool { return kv1.first < kv2.first; });
        assert(flag);
#endif

        basic_ds::BloomFilter<lsm::BLF_SIZE> bft;
        for (const auto &kv : kv_list) {
            bft.insert(kv.first);
        }

        std::string bin_name = target_dir + '/' + generate_hash() + ".sst";

        std::ofstream bin_out{bin_name, std::ios::binary};  // Trunc
        if (!bin_out) {
            throw std::runtime_error{"Cannot write sst " + bin_name +
                                     ". Please check if the directory exists."};
        }

        std::pair<uint64_t, uint64_t> range{kv_list.front().first, kv_list.back().first};
        uint64_t count = kv_list.size();

        // Write the header
        bin_out.write(reinterpret_cast<const char *>(&timestamp), sizeof timestamp)
            .write(reinterpret_cast<const char *>(&count), sizeof count)
            .write(reinterpret_cast<const char *>(&range), sizeof range);

        // Write the bloom filter
        bin_out << bft;

        // The below implements are value_type-dependent

        // Write the index table
        decltype(sst::sst_cache{}.indices) indices;
        indices.reserve(count);

        lsm::offset_type offset =
            32 + lsm::BLF_SIZE + count * (sizeof(key_type) + sizeof(lsm::offset_type));
        for (kv_type &kv : kv_list) {
            bin_out.write(reinterpret_cast<const char *>(&kv.first), sizeof(key_type))
                .write(reinterpret_cast<const char *>(&offset), sizeof(lsm::offset_type));
            indices.emplace_back(kv.first, offset);
            offset += kv.second.length() + 1;  // null-terminated
        }

        // Write the value data
        for (kv_type &kv : kv_list) {
            bin_out.write(kv.second.c_str(), kv.second.length() + 1);
        }

        this->byte_size = 32 + lsm::BLF_SIZE;

        return new sst_cache{level,
                             {timestamp, count, range.first, range.second},
                             std::move(bft),
                             std::move(indices),
                             {bin_name}};
    }
};

/**
 * @brief Merge sort multiple sst files. This function will delete all the referred ssts,
 *        and write at least several ssts into the target level.
 * @param cache_list
 * @param level the target level where the compacted ssts are put into.
 * @return std::vector<sst::sst_cache> the caches associated with newly-created ssts.
 */
inline std::vector<sst_cache> sort_and_merge(const std::vector<sst_cache> &cache_list,
                                             std::string target_dir, bool is_last = false) {
    using kv_type = std::pair<lsm::key_type, lsm::value_type>;

    uint64_t timestamp = cache_list.front().header.time_stamp;
    sst_buffer buffer{timestamp, target_dir};

    const std::size_t N = cache_list.size();
    std::vector<std::vector<kv_type>> kv_list;
    kv_list.reserve(N);

    for (std::size_t i = 0; i < N; ++i) {
        kv_list.push_back(cache_list[i].get_kv());
        utils::rmfile(cache_list.at(i).sst_path.c_str());
    }

    std::vector<std::size_t> p(N, 0);

    auto key_to_select = [&](std::size_t i) -> lsm::key_type {
        return kv_list.at(i).at(p.at(i)).first;
    };

    auto remove_cache = [&](std::size_t i) -> void {
        if (++p[i] == kv_list.at(i).size()) {
            kv_list.erase(kv_list.begin() + i);
            p.erase(p.begin() + i);
        }
    };

    std::vector<sst_cache> res{};

    while (!p.empty()) {
        std::size_t selected = 0;
        auto selected_key = key_to_select(selected);
        for (std::size_t i = 1; i < p.size(); ++i) {
            auto tmp = key_to_select(i);
            if (tmp == selected_key) {
                remove_cache(i--);
            } else if (tmp < selected_key) {
                selected_key = tmp;
                selected = i;
            }
        }
        auto &selected_kv_list = kv_list.at(selected);
        lsm::value_type to_append_value = selected_kv_list.at(p.at(selected)).second;
        if (!is_last || to_append_value != lsm::DeleteNote) {
            auto cache_ptr = buffer.append(selected_key, to_append_value);
            if (cache_ptr) {
                res.push_back(std::move(*cache_ptr));
                delete cache_ptr;
            }
        }
        remove_cache(selected);
    }
    // Clear the resident kv
    auto cache_ptr = buffer.clear();
    if (cache_ptr) {
        res.push_back(std::move(*cache_ptr));
        delete cache_ptr;
    }
    return res;
}

}  // namespace sst
#endif