#pragma once

#include "kvstore_api.h"
#include "MemTable.hpp"
#include "sst.hpp"

class KVStore : public KVStoreAPI {
    // You can add your implementation here
public:
    using key_type = uint64_t;
    using value_type = std::string;
    KVStore(const std::string &dir);
    KVStore() = delete;

    ~KVStore();

    void put(uint64_t key, const std::string &s) override;

    value_type get(uint64_t key) override;

    bool del(uint64_t key) override;

    void reset() override;

    void scan(uint64_t key1, uint64_t key2,
              std::list<std::pair<uint64_t, std::string>> &list) override;
private:
    using mtb_type = mtb::MemTable;
    const std::string data_dir;
    uint64_t cur_ts;  // Current time stamp.
    std::unique_ptr<mtb_type> mtb_ptr;
    std::vector<sst::sst_cache> caches;

    enum class level_type {
        TIERING, LEVELING
    };

    static constexpr std::size_t MEMORY_MAXSIZE = 2 * 1024 * 1024; /* 2 MB */

    /**
     * @brief The flow to be implemented when `put` or `delete` (aka put `"~DELETED~"`)
     *        operation will overflow the memory table size.
     */
    void handle_sst();

    inline static std::string generate_hash();

    std::string _format_dir(const std::string &dir) {
        // TODO improve robustness
        return dir;
    }

    std::pair<value_type, bool> search_sst(key_type key) const;

};