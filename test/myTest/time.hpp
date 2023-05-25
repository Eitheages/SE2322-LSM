/**
 * @file time.hpp
 * @author BojunRen (albert.cauchy725@gmail.com)
 * @brief APIs for calculating program's run time
 * @date 2023-04-01
 *
 * @copyright Copyright (c) 2023, All Rights Reserved.
 *
 */
#include <chrono>
#include <optional>
#include <stdexcept>

namespace test {
class TimeRecorder {
    using Clock = std::chrono::high_resolution_clock;
    using Time = decltype(Clock::now());
    using TimePoint = std::optional<Time>;

public:
    TimeRecorder() {
        record_start();
    }

    unsigned long long duration() {
        record_end();
        if (!(start.has_value() && end.has_value())) {
            throw std::runtime_error{
                "You should invoke record_start/end before calculating duration!"};
        }
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end.value() - start.value());

        return duration.count();
    }

private:
    TimePoint start, end;
    void record_start() noexcept {
        start = Clock::now();
    }

    void record_end() noexcept {
        end = Clock::now();
    }

};
}  // namespace test