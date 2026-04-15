#ifndef EPOLLER_H
#define EPOLLER_H
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>     // read write close等系统调用
#include <vector>

class Epoller {
  public:
    Epoller(int maxClient = 4096);
    ~Epoller();
    bool AddFd(int fd, uint32_t event);
    bool ModFd(int fd, uint32_t event);
    bool DelFd(int fd);
    int Wait(int timeOut = -1);
    int GetEventFd(size_t index) const;
    uint32_t GetEvent(size_t index) const;

  private:
    int epfd_;
    std::vector<struct epoll_event> events_;
};

#endif