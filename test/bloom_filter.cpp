#include <iostream>
#include <fstream>
#include "../include/BloomFilter.hpp"

#define SIZE 64

int main() {
    basic_ds::BloomFilter<SIZE> bft;
    for (int i = 0; i < 100; ++i) {
        bft.insert(i);
    }
    for (int i = 70; i < 120; i += 2) {
        if (!bft.contains(i) && i >= 0 && i < 100) {
            return 1;
        }
    }
    if (bft.byte_size() != SIZE) {
        return 1;
    }
    std::ofstream out{"./test.sst", std::ios_base::binary | std::ios_base::trunc};
    out << bft;
    out.close();
    basic_ds::BloomFilter<SIZE> new_bft;
    std::ifstream in{"./test.sst", std::ios_base::binary};
    in >> new_bft;
    in.close();
    for (int i = 50; i < 150; ++i) {
        if (new_bft.contains(i) != bft.contains(i)) {
            return 1;
        }
    }
}