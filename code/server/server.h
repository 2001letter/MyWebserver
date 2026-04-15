#ifndef SERVER_H
#define SERVER_H
#include "../buffer/buffer.h"
#include "../http/httpconn.h"
#include "../log/log.h"
#include "../pool/sqlpool.h"
#include "../pool/threadpool.h"
#include "../timer/heaptimer.h"
#include "epoller.h"
#include <arpa/inet.h> // inet_ntoa
#include <cstring>
#include <fcntl.h> // fcntl
#include <iostream>
#include <memory>
#include <netinet/in.h> // sockaddr_in
#include <sys/socket.h> // socket
#include <unordered_map>

class Server {
  public:
    Server(int port, int trigeMode, bool openLog, LogLevel logLevel, long timeout, const std::string &host,
           const std::string &name, const std::string &pwd, const std::string &db);
    ~Server();
    void start();

  private:
    void InitTrigeMode_(int trigeMode);
    bool InitSocket_();
    // 设置非阻塞的函数 ET MODE
    int SetNonBlocking_(int fd);
    void DealListen_();
    void DealRead_(HttpConn &httpClient);
    void OnRead_(HttpConn &httpClient);
    void DealWrite_(HttpConn &httpClient);
    void OnWrite_(HttpConn &httpClient);
    void AdjustTimer_(HttpConn &httpClient);
    void CloseClient_(HttpConn &httpClient);
    int port_;
    int serverFd_;
    bool isClose_;
    long timeout_;
    std::string resDir_;
    uint32_t serverEvent_;
    uint32_t clientEvent_;
    Buffer buffer_;
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<Epoller> epoller_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unordered_map<int, HttpConn> client_; // [fd]httpconn
};

#endif