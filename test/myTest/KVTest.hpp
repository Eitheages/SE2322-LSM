#define NDEBUG
#include <iostream>
#include "kvstore.h"
#include "time.hpp"
using namespace std::string_literals;

class KVTest {
    using key_type = KVStore::key_type;
    using value_type = KVStore::value_type;

public:
    KVTest() {
        utils::mkdir("./data");
        kv = std::make_unique<KVStore>("./data"s);
        kv->reset();
        std::cout << "Start a test" << std::endl;
    }

    ~KVTest() {
        kv->reset();
    }

    void put(const key_type& key, const value_type& value) {
        test::TimeRecorder recorder;
        kv->put(key, value);
        auto time = recorder.duration();
        latency[0].first++;
        latency[0].second += time;
    }

    value_type get(const key_type& key) {
        test::TimeRecorder recorder;
        auto res = kv->get(key);
        auto time = recorder.duration();
        latency[1].first++;
        latency[1].second += time;
        return res;
    }

    bool del(const key_type& key) {
        test::TimeRecorder recorder;
        auto res = kv->del(key);
        auto time = recorder.duration();
        latency[2].first++;
        latency[2].second += time;
        return res;
    }

    void report() const {
        std::cout << "Here is the test report.\n";
        std::cout << "PUT: \t" << latency[0].first << '\t' << latency[0].second << '\n';
        std::cout << "GET: \t" << latency[1].first << '\t' << latency[1].second << '\n';
        std::cout << "DEL: \t" << latency[2].first << '\t' << latency[2].second << '\n';
        std::cout << std::endl;
    }

private:
    using report_type = std::pair<unsigned, unsigned long long>;
    std::unique_ptr<KVStore> kv;
    std::array<report_type, 3> latency{};
};