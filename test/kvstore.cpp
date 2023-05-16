#include "../include/kvstore.h"

int main() {
    KVStore kv{"./data"};
    for (int i = 0; i < 100000; ++i) {
        kv.put(i, std::to_string(1ll*i*i));
    }
}