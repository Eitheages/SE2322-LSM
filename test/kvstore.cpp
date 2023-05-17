#include "../include/kvstore.h"

int main() {
    KVStore kv{"./data"};
    kv.reset();
}