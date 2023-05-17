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
    // Check the directory and create when necessary
    // TODO don't throw an exception
    if (!utils::dirExists(dir)) {
        throw std::runtime_error{"No such data directory!"};
    }
    // Check if there is resident data
    std::vector<std::string> dir_levels{};
    utils::scanDir(data_dir, dir_levels);
    if (dir_levels.empty()) {
        mtb_ptr = std::make_unique<mtb_type>(1);
        return;
    }
    for (const auto &dir : dir_levels) {
        int level = std::stoi(dir.substr(dir.find('-') + 1));
        std::string dir_path = data_dir + '/' + dir + '/';
        std::vector<std::string> sst_list;
        utils::scanDir(dir_path, sst_list);
        if (sst_list.empty()) {
            continue;
        }
        for (const auto &sst_name : sst_list) {
            auto cache = sst::read_sst(dir_path + sst_name, level);
            this->caches.emplace_back(std::move(cache));
        }
    }
    std::sort(this->caches.begin(), this->caches.end(),
              [](const sst::sst_cache &a, const sst::sst_cache &b) -> bool {
                  return a.header.time_stamp > b.header.time_stamp;
              });
    if (!this->caches.empty()) {
        this->cur_ts = this->caches[0].header.time_stamp + 1;
        mtb_ptr = std::make_unique<mtb_type>(this->cur_ts);
    } else {
        mtb_ptr = std::make_unique<mtb_type>(1);
    }
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
    for (auto &cache : caches) {
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
        this->mtb_ptr->put(key, KVStore::DeleteNote);
        return true;
    }
    for (auto &cache : caches) {
        lsm::offset_type offset;
        bool flag;
        std::tie(offset, flag) = cache.search(key);
        if (flag) {
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
        if (sst_list.empty()) {
            continue;
        }
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
    // TODO all ssts are saved in level-0
    int level = 0;

    std::string sst_name = KVStore::generate_hash() + ".sst";

    const std::string target_dir = this->data_dir + "/level-" + std::to_string(level);
    if (utils::mkdir(target_dir.c_str()) != 0) {
        throw std::runtime_error{"Cannot create directory " + target_dir};
    }

    auto cache = mtb_ptr->to_binary(target_dir + "/" + sst_name, level);
    this->caches.emplace(this->caches.begin(), std::move(cache));

    // Reset the memory table.
    mtb_ptr = std::make_unique<mtb_type>(++this->cur_ts);
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