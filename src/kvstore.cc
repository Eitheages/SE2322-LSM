/**
 * @file kvstore.cc
 * @author BojunRen (albert.cauchy725@gmail.com)
 * @brief Implements for the class `KVStore`.
 * @date 2023-05-13
 *
 * @copyright Copyright (c) 2023, All Rights Reserved.
 *
 */
#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>

#include "../include/kvstore.h"
#include "../include/utils.h"

KVStore::KVStore(const std::string &dir)
    : KVStoreAPI(dir), data_dir{_format_dir(dir)}, cur_ts{1}, mtb_ptr{nullptr} {
    static_assert(KVStore::MEMORY_MAXSIZE > lsm::BLF_SIZE, "No enough space!");
    // Hard-coded configuration
    strategy = {{0, 2, level_type::TIERING}, {1, 4}, {2, 8}, {3, 16}, {4, 32},
                {5, /* uint32_max */}};

    // Check the directory and create when necessary
    if (!utils::dirExists(dir)) {
        throw std::runtime_error{"No such data directory!"};
    }

    std::vector<std::string> dir_list{};
    utils::scanDir(data_dir, dir_list);

    for (const std::string& level_dir : dir_list) {
        int level = std::stoi(level_dir.substr(level_dir.find('-') + 1));
        std::string dir_path = data_dir + '/' + level_dir + '/';
        std::vector<std::string> sst_list;
        utils::scanDir(dir_path, sst_list);
        for (const auto &sst_name : sst_list) {
            auto cache = sst::read_sst(dir_path + sst_name, level);
            if (cache.level == -1) {
                throw std::runtime_error{"Cannot read sst " + dir_path + sst_name};
            }
            caches.push_back(std::move(cache));
        }
    }
    std::sort(caches.begin(), caches.end(), std::less<decltype(caches)::value_type>{});
    if (!caches.empty()) {
        cur_ts = caches.back().header.time_stamp + 1;
    }
    mtb_ptr = std::make_unique<mtb_type>(cur_ts);
}

KVStore::~KVStore() {
    if (this->mtb_ptr->size() != 0) {
        handle_sst();
    }
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s) {
    if (mtb_ptr->predict_byte_size(key, s) >= KVStore::MEMORY_MAXSIZE) {
        handle_sst();
    }
    // Automatically destruct the previous memory table.
    mtb_ptr->put(key, s);
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key) {
    auto mtb_get_res = mtb_ptr->get(key);
    if (mtb_get_res.second) {
        if (mtb_get_res.first == KVStore::DeleteNote) {
            return {};
        }
        return mtb_get_res.first;
    }
    // The cache list is ordered in ascending order (respect to the timestamp)
    for (auto it = caches.rbegin(); it != caches.rend(); ++it) {
        const auto &cache = *it;
        lsm::offset_type offset;
        bool flag;
        std::tie(offset, flag) = cache.search(key);
        if (flag) {
            std::string res = cache.from_offset(offset);
            if (res == KVStore::DeleteNote) {
                return {};
            }
            return res;
        }
    }
    return {};
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key) {
    auto mtb_get_res = mtb_ptr->get(key);
    if (mtb_get_res.second) {
        if (mtb_get_res.first == KVStore::DeleteNote) {
            return false;
        }
        this->mtb_ptr->put(key, KVStore::DeleteNote);
        return true;
    }
    for (auto it = caches.rbegin(); it != caches.rend(); ++it) {
        const auto &cache = *it;
        lsm::offset_type offset;
        bool flag;
        std::tie(offset, flag) = cache.search(key);
        if (flag) {
            std::string found = cache.from_offset(offset);
            if (found == KVStore::DeleteNote) {
                return false;
            }
            this->mtb_ptr->put(key, KVStore::DeleteNote);
            return true;
        }
    }
    return false;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset() {
    std::vector<std::string> dir_levels{};
    utils::scanDir(data_dir, dir_levels);
    if (dir_levels.empty()) {
        return;
    }
    for (const auto &dir : dir_levels) {
        std::string dir_path = data_dir + '/' + dir + '/';
        std::vector<std::string> sst_list;
        utils::scanDir(dir_path, sst_list);
        for (const auto &sst_name : sst_list) {
            utils::rmfile((dir_path + sst_name).c_str());
        }
        utils::rmdir(dir_path.c_str());
    }
    this->caches = decltype(this->caches){};
    this->cur_ts = 1;
    this->mtb_ptr = std::make_unique<mtb_type>(1);
}

/**
 * Return a list including all the key-value pair between key1 and key2.
 * keys in the list should be in an ascending order.
 * An empty string indicates not found.
 */
void KVStore::scan(uint64_t key1, uint64_t key2,
                   std::list<std::pair<uint64_t, std::string>> &list) {}

void KVStore::handle_sst() {
    // Write the memory table to level-0
    std::string sst_name = KVStore::generate_hash() + ".sst";

    const std::string target_dir = this->data_dir + "/level-0";
    utils::mkdir(target_dir.c_str());

    auto cache = mtb_ptr->to_binary(target_dir + "/" + sst_name, 0);
    this->caches.push_back(std::move(cache));
    /**
     * All caches maintained by kvstore has smaller time stamp.
     * Therefore, no need to sort.
     */

    // Reset the memory table.
    mtb_ptr = std::make_unique<mtb_type>(++this->cur_ts);

    // Recursively check each level
    // check_level(0); // TODO enable this
}

void KVStore::check_level(int level) {
    assert(level < strategy.size());  // The strategy generated method ensures this.
    if (level >= strategy.size()) {
        return;
    }
    auto file_count = std::count_if(
        caches.begin(), caches.end(),
        [=](const sst::sst_cache &cache) -> bool { return cache.level == level; });
    if (file_count <= strategy[level].max_file) {
        return;
    }
    compact(level, level + 1);
    check_level(level + 1);
}

void KVStore::compact(int l1, int l2) {
    // TODO compaction
    // Step 1: SSTable select

    // 1.1 select from level l1
    key_type l1_min_key = std::numeric_limits<key_type>::max();
    key_type l1_max_key = std::numeric_limits<key_type>::min();
    std::vector<sst::sst_cache> lhs_selected_cache{};
    // Tiering: select all
    uint32_t lhs_selected_cnt = std::count_if(
        caches.begin(), caches.end(),
        [=](const sst::sst_cache &cache) -> bool { return cache.level == l1; });
    // Leveling: truncate
    if (strategy[l1].type == level_type::LEVELING) {
        lhs_selected_cnt -= strategy[l1].max_file;
    }

    lhs_selected_cache.reserve(lhs_selected_cnt);
    for (auto it = caches.begin();
         it != caches.end() && lhs_selected_cache.size() < lhs_selected_cnt;) {
        if (it->level == l1) {
            l1_max_key = std::max(it->header.upper, l1_max_key);
            l1_min_key = std::min(it->header.lower, l1_min_key);
            lhs_selected_cache.push_back(std::move(*it));
            it = caches.erase(it);
        } else {
            ++it;
        }
    }
    assert(lhs_selected_cache.size() == lhs_selected_cnt);

    // 1.2 select from level l2
    std::vector<sst::sst_cache> rhs_selected_cache{};
}

std::string KVStore::generate_hash() {
    // The generator is managed by the same template class.
    static std::random_device random_device{};
    static std::mt19937 engine{random_device()};
    static std::uniform_int_distribution<> dist(0, 0xFFFFFF);

    uint32_t random_number = dist(engine);
    std::stringstream ss{};
    ss << std::setw(6) << std::setfill('0') << std::hex << random_number;
    return ss.str();
}

const typename KVStore::value_type KVStore::DeleteNote{"~DELETED~"};