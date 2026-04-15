#include "heaptimer.h"

void HeapTimer::AdjustTimer(int id, unsigned long newExpired) {
    assert(ref_.count(id));
    size_t i = ref_[id];
    timer_[i].expired = Clock::now() + MS(newExpired);
    if(!ShiftDown_(i)){
        ShiftUp_(i);
    }
}

// 清除超时节点
void HeapTimer::Tick() {
    while(!timer_.empty()){
        TimerNode node = timer_.front();
        if(std::chrono::duration_cast<MS>(node.expired - Clock::now()).count() > 0){
            break;
        }
        node.cb();
        // DelTimer(node.id);
    }
}

long HeapTimer::GetNextTick() { 
    Tick();
    long expired = -1;
    if(!timer_.empty()){
        expired = std::chrono::duration_cast<MS>(timer_.front().expired - Clock::now()).count();
    }
    return expired; 
}

// 上滑，与父节点比较
void HeapTimer::ShiftUp_(size_t i) {
    while(i > 0){
        size_t parent = (i - 1) / 2;
        if(timer_[parent].expired > timer_[i].expired){
            SwapNode_(i, parent);
            i = parent;
        } else {
            break;
        }
    }
}

// 下滑，与孩子节点比较
bool HeapTimer::ShiftDown_(size_t i) { 
    size_t index = i;
    size_t left = i * 2 + 1;
    while(left < timer_.size()){
        size_t min = left;
        if(left + 1 < timer_.size() && timer_[left + 1].expired < timer_[left].expired){
            min = left + 1;
        }
        if(timer_[min].expired < timer_[i].expired){
            SwapNode_(i, min);
            i = min;
            left = i * 2 + 1;
        } else {
            break;
        }
    }
    return index < i; 
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
    std::swap(timer_[i], timer_[j]);
    ref_[timer_[i].id] = i;
    ref_[timer_[j].id] = j;
}

void HeapTimer::DelTimer(int id) {
    if(!ref_.count(id)){
        return;
    }
    size_t i = ref_[id];
    size_t n = timer_.size() - 1;
    SwapNode_(i, n);
    ref_.erase(timer_[n].id);
    timer_.pop_back();
    if((i != n) && !ShiftDown_(i)){
        ShiftUp_(i);
    }
}
