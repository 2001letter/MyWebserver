#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
  public:
    ThreadPool(int maxCount = 8) : isClose_(false) {
        for (int i = 0; i < maxCount; i++) {
            workers_.emplace_back([this] { WorkLoop_(); });
        }
    }
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lck(mutex_);
            isClose_ = true;
        }
        cond_.notify_all();
        for (auto &thread : workers_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    template <typename T> void AddTask(T &&task) {
        {
            std::lock_guard<std::mutex> lck(mutex_);
            tasks_.emplace(std::forward<T>(task));
        }
        cond_.notify_one();
    }

  private:
    void WorkLoop_() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lck(mutex_);
                cond_.wait(lck, [this] { return !tasks_.empty() || isClose_; });
                if ( isClose_ && tasks_.empty()) {
                    return;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }
    // 工作队列
    std::vector<std::thread> workers_;
    // 任务队列
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool isClose_;
};

#endif