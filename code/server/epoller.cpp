#include "epoller.h"

Epoller::Epoller(int maxClient) : epfd_(epoll_create1(0)), events_(maxClient) {}

Epoller::~Epoller() { close(epfd_); }

bool Epoller::AddFd(int fd, uint32_t event) {
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = event;
    return epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool Epoller::ModFd(int fd, uint32_t event) {
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = event;
    return (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == 0);
}

bool Epoller::DelFd(int fd) { return (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL) == 0); }

int Epoller::Wait(int timeOut) { return epoll_wait(epfd_, &events_[0], events_.size(), timeOut); }

int Epoller::GetEventFd(size_t index) const { return events_[index].data.fd; }

uint32_t Epoller::GetEvent(size_t index) const { return events_[index].events; }
