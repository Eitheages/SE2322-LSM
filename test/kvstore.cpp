#include "../include/kvstore.h"
#include <map>

#define TestEqual(expect, real) \
    if ((expect) != (real))     \
    return 1

void print_cache(const sst::sst_cache &cache) {
    printf("%lld %lld\n", cache.header.time_stamp, cache.header.count);
}

struct TestKVStore {
    KVStore kv;
    TestKVStore(const std::string& str) : kv{str} {}
    ~TestKVStore() {kv.reset();}
    void put(const lsm::key_type& key, const lsm::value_type& value) {
        kv.put(key, value);
    }
    lsm::value_type get(const lsm::key_type& key) {
        return kv.get(key);
    }
};

int main() {
    std::vector<std::string> data{"sssssssssssssssssssssssssssss", "hhhhhhhhhhhhhhhhhhhhhhh",
                                  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
    TestKVStore kv{"./data"};
    std::map<lsm::key_type, lsm::value_type> mp;
    for (int i = 0; i < 10000; ++i) {
        kv.put(i, data[0]);
        mp[i] = data[0];
    }
    for (int i = 0; i < 30000; i += 2) {
        kv.put(i, data[1]);
        mp[i] = data[1];
    }
    for (int i = 10000; i < 60000; i += 3) {
        kv.put(i, data[2]);
        mp[i] = data[2];
    }
    for (int i = 10; i < 444444; i += 2) {
        kv.put(i, data[0] + data[1]);
        mp[i] = data[0] + data[1];
    }

    for (auto value : mp) {
        TestEqual(kv.get(value.first), value.second);
    }

    // auto cache = sst::read_sst("./data/level-0/c80eda.sst", 0);
    // std::cout << std::hex;
    // std::cout << cache.search(59989).first << std::endl;
}