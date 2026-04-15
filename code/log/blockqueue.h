#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <iostream>
#include <mutex>

template <typename T> class BlockQueue {
  public:
    BlockQueue(size_t maxSize = 1000);
    ~BlockQueue();
    void push_back(const T &item);
    bool pop_front(T &item);
    void Close();
    void clear();
    bool empty();
    bool full();
    size_t size();
    size_t capacity();

    void WakeCustomer();

  private:
    std::deque<T> deq_; // 阻塞队列，元素是一条日志
    size_t capacity_;   // 队列的最大容量
    size_t size_;       // 队列已有元素
    bool close_;
    std::mutex mutex_;                 // 阻塞队列的锁
    std::condition_variable producer_; // 生产者
    std::condition_variable customer_; // 消费者
};

template <typename T> BlockQueue<T>::BlockQueue(size_t maxSize) : capacity_(maxSize), close_(false) {}

template <typename T> BlockQueue<T>::~BlockQueue() { Close(); }

template <typename T> void BlockQueue<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> ulck(mutex_);
    while (deq_.size() >= capacity_) { // 如果队列已满，等待消费者唤醒
        producer_.wait(ulck);
    }
    deq_.push_back(item);
    customer_.notify_one();
}

template <typename T> bool BlockQueue<T>::pop_front(T &item) {
    std::unique_lock<std::mutex> ulck(mutex_);
    while (deq_.empty()) { // 如果队列为空，等待生产者唤醒
        customer_.wait(ulck, [this] { return !deq_.empty() || close_; });
        if (close_)
            return false;
    }
    item = deq_.front();
    deq_.pop_front();
    producer_.notify_one();
    return true;
}

template <typename T> inline void BlockQueue<T>::Close() {
    clear();
    close_ = true;
    customer_.notify_all();
    producer_.notify_all();
}

template <typename T> void BlockQueue<T>::clear() {
    std::lock_guard<std::mutex> ulck(mutex_);
    deq_.clear();
}

template <typename T> bool BlockQueue<T>::empty() {
    std::lock_guard<std::mutex> ulck(mutex_);
    return deq_.empty();
}

template <typename T> bool BlockQueue<T>::full() {
    std::lock_guard<std::mutex> ulck(mutex_);
    return (deq_.size() >= capacity_);
}

template <typename T> size_t BlockQueue<T>::size() {
    std::lock_guard<std::mutex> ulck(mutex_);
    return deq_.size();
}

template <typename T> size_t BlockQueue<T>::capacity() {
    std::lock_guard<std::mutex> ulck(mutex_);
    return capacity_;
}

template <typename T> inline void BlockQueue<T>::WakeCustomer() { customer_.notify_one(); }

#endif