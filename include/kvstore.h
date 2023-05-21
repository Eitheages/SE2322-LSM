#pragma once

#include "MemTable.hpp"
#include "kvstore_api.h"
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

// private:
    using mtb_type = mtb::MemTable;
    enum class level_type { TIERING, LEVELING };
    struct lsm_config {
        int level;  // Not used
        uint32_t max_file = UINT32_MAX;
        enum level_type type = level_type::LEVELING;

        template <typename Traits>
        friend std::basic_istream<char, Traits> &operator>>(
            std::basic_istream<char, Traits> &is, lsm_config &config) {
            std::string type_str;
            is >> config.level >> config.max_file >> type_str;
            // case insensitive
            std::transform(type_str.begin(), type_str.end(), type_str.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (type_str == "tiering") {
                config.type = level_type::TIERING;
            }
            return is;
        }
        template <typename Traits>
        friend std::basic_ostream<char, Traits> &operator<<(
            std::basic_ostream<char, Traits> &os, const lsm_config &config) {
            os << config.level << ' ' << config.max_file << ' ' << (int)config.type;
            return os;
        }
    };

    const std::string data_dir;  // No ending '/'
    uint64_t cur_ts;             // Current time stamp.
    std::unique_ptr<mtb_type> mtb_ptr;
    std::vector<sst::sst_cache> caches; // Ordered by timestamp (ascending)
    std::vector<lsm_config> strategy;

    static constexpr std::size_t MEMORY_MAXSIZE = 2 * 1024 * 1024; /* 2 MB */
    static const value_type DeleteNote;                            /* ~DELETE~ */

    /**
     * @brief The flow to be implemented when `put` or `delete` (aka put `"~DELETED~"`)
     *        operation will overflow the memory table size.
     */
    void handle_sst();

    void check_level(int level);

    void compact(int l1, int l2);

};
