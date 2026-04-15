#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <assert.h>
#include <chrono>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

using Clock = std::chrono::steady_clock;
using MS = std::chrono::milliseconds;
using TimePoint = Clock::time_point;
struct TimerNode {
    int id;
    TimePoint expired;
    std::function<void()> cb;
    bool operator<(const TimerNode &timer) { return this->expired < timer.expired; }
    bool operator>(const TimerNode &timer) { return this->expired > timer.expired; }
};

class HeapTimer {
  public:
    HeapTimer() = default;
    ~HeapTimer() = default;
    template <typename T> void AddTimer(int id, unsigned long expired, T &&cb) {
        // 如果id存在，则调整
        if (ref_.count(id)) {
            AdjustTimer(id, expired);
            timer_[ref_[id]].cb = std::forward<T>(cb);
        } else {
            size_t n = timer_.size();
            ref_[id] = n;
            timer_.emplace_back(id, Clock::now() + MS(expired), std::forward<T>(cb));
            ShiftUp_(n);
        }
    }
    void AdjustTimer(int id, unsigned long newExpired);
    void DelTimer(int id);
    void Tick();
    long GetNextTick();

  private:
    void ShiftUp_(size_t i);
    bool ShiftDown_(size_t i);
    void SwapNode_(size_t i, size_t j);
    
    std::vector<TimerNode> timer_;
    std::unordered_map<int, size_t> ref_; // id对应数组中的下标
};

#endif
