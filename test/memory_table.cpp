#include <map>
#include "../include/MemTable.hpp"

using namespace std::string_literals;

#define TestEqual(expect, real) \
    if ((expect) != (real))     \
    return 1

int main() {
    mtb::MemTable mtb;
    std::map<decltype(mtb)::key_type, decltype(mtb)::val_type> mp;

    TestEqual(10240 + 32, mtb.byte_size());
    for (int i = 0; i < 100; i += 2) {
        mp.insert(std::make_pair(i, std::to_string(i % 10)));
        mtb.put(i, std::to_string(i % 10));
    }
    std::size_t expect_size = 32 + 10240 + 12 * 50 + 2 * 50;
    TestEqual(expect_size, mtb.byte_size());
    for (int i = 0; i < 100; ++i) {
        const auto it = mp.find(i);
        auto p = mtb.get(i);
        if (!p.second && it == mp.cend()) {
            continue;
        }
        if (p.second && it != mp.cend() && p.first == it->second) {
            continue;
        }
        return 1;
    }
    TestEqual(50, mtb.size());
    mtb.put(2, "8"s);
    TestEqual(expect_size, mtb.byte_size());
    mtb.put(2, "11"s);
    TestEqual(50, mtb.size());
    expect_size += 1;
    TestEqual(expect_size, mtb.byte_size());
    mtb.put(2, "~DELETED~"s);
    expect_size += 7;
    TestEqual(expect_size, mtb.byte_size());
    mtb.put(3, "~DELETED~"s);
    TestEqual(51, mtb.size());

    auto cache = mtb.to_binary("test_read.sst", 0);
    TestEqual(51, cache.header.count);
}