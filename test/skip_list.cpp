#include <map>
#include "../include/SkipList.hpp"

int main() {
    basic_ds::SkipList<uint64_t, uint64_t> sl;
    std::map<uint64_t, uint64_t> mp;

    for (int i = 0; i < 100; i += 2) {
        mp.insert(std::make_pair(i, i * i));
        sl.insert(i, i * i);
    }

    for (int i = 0; i < 100; ++i) {
        const auto it = mp.find(i);
        auto p = sl.find(i);
        if (!p && it == mp.cend()) {
            continue;
        }
        if (p && it != mp.cend() && p->val == it->second) {
            continue;
        }
        return 1;
    }

    for (int i = 0; i < 100; ++i) {
        if (!sl.insert_or_assign(i, i % 2 == 0 ? i : i * i).second && i % 2 != 0) {
            return 1;
        }
    }
    // odd: i * i, even: i
    for (int i = 0; i < 100; ++i) {
        if (sl.find(i)->val != (i % 2 == 0 ? i : i * i)) {
            return 1;
        }
    }

    // Test edge cases.
    auto val1 = sl.find(1)->val;
    for (int k = 0; k < 100; ++k) {
        auto val = sl.insert_or_assign(0, k).first->val;
        if (val != sl.find(0)->val || val != k) {
            return 1;
        }
        if (sl.find(1)->val != val1) {
            return 1;
        }
    }
}