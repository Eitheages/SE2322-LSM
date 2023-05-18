#include <iostream>
#include "../include/kvstore.h"

int main() {
    KVStore kv{"./data"};
    kv.reset();
    for (uint64_t i = 1000; i < 1'000'000; ++i) {
        kv.put(i, std::to_string(1ll * i * i));
    }
    std::cout << "initialized!" << std::endl;
}