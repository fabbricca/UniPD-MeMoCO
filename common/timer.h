// timer.h - wall-clock stopwatch (seconds).
#ifndef TIMER_H
#define TIMER_H
#include <chrono>

class Timer {
    std::chrono::steady_clock::time_point start_;
public:
    Timer() : start_(std::chrono::steady_clock::now()) {}
    void reset() { start_ = std::chrono::steady_clock::now(); }
    double elapsed() const {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
    }
};
#endif
