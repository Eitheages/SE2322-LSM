#include "KVTest.hpp"


int main(int argc, char** argv) {
    int N;
    if (argc == 1) {
        N = 10'000;
    } else {
        N = std::stoi(argv[1]);
    }
    KVTest test{};
    for (int i = 0; i < N; ++i) {
        test.put(i, std::string(i+1, 'a'));
    }
    for (int i = 0; i < N; ++i) {
        if (test.get(i) != std::string(i+1, 'a')) {
        }
        test.del(i);
    }
    test.report();
}