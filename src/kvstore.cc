/**
 * @file kvstore.cc
 * @author BojunRen (albert.cauchy725@gmail.com)
 * @brief Implements for the class `KVStore`.
 * @date 2023-05-13
 *
 * @copyright Copyright (c) 2023, All Rights Reserved.
 *
 */
#include "../include/kvstore.h"
#include <random>
#include <sstream>
#include "../include/utils.h"

KVStore::KVStore(const std::string &dir)
    : KVStoreAPI(dir), data_dir{_format_dir(dir)}, mtb_ptr{new mtb_type{1}}, cur_ts{1} {
    static_assert(KVStore::MEMORY_MAXSIZE > basic_ds::BLF_SIZE, "No enough space!");
    // Check the directory and create when necessary
    if (utils::mkdir(data_dir.c_str()) != 0) {
        throw std::runtime_error{"Cannot initialize the directory!"};
    }
}

KVStore::~KVStore() {}

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
        return mtb_get_res.first;
    }
    return "";
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key) {
    return false;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset() {}

/**
 * Return a list including all the key-value pair between key1 and key2.
 * keys in the list should be in an ascending order.
 * An empty string indicates not found.
 */
void KVStore::scan(uint64_t key1, uint64_t key2,
                   std::list<std::pair<uint64_t, std::string>> &list) {}

void KVStore::handle_sst() {
    // TODO all ssts are saved in level-0
    std::string sst_name = KVStore::generate_hash() + ".sst";

    const std::string target_dir = this->data_dir + "/level-0";
    if (utils::mkdir(target_dir.c_str()) != 0) {
        throw std::runtime_error{"Cannot create directory " + target_dir};
    }

    mtb_ptr->to_binary(target_dir + "/" + sst_name);

    // Reset the memory table.
    mtb_ptr = std::make_unique<mtb_type>(++this->cur_ts);
}

std::string KVStore::generate_hash() {
    // The generator is managed by the same template class.
    static std::random_device random_device{};
    static std::mt19937 engine{random_device()};
    static std::uniform_int_distribution<> dist(0, 0xFFFFFF);

    auto random_number = dist(engine);
    std::stringstream ss{};
    ss << std::hex << random_number;
    return ss.str();
}

std::pair<typename KVStore::value_type, bool> KVStore::search_sst(key_type key) const {
    return {};
}