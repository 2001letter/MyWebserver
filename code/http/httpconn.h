#ifndef HTTPCONN_H
#define HTTPCONN_H
#include "../buffer/buffer.h"
#include "../log/log.h"
#include "httprequest.h"
#include "httpresponse.h"
#include <arpa/inet.h>
#include <atomic>
#include <iostream>
#include <netinet/in.h> // sockaddr_in
#include <sys/sendfile.h>

// 从socket缓冲区读取数据到buffer->解析(http request)->构造响应(http response)->发送buffer数据到socket缓冲区
class HttpConn {
  public:
    HttpConn();
    ~HttpConn();
    void HttpInit(int fd, const sockaddr_in &addr);
    ssize_t ReadRequest(int *Errno);
    ssize_t WriteResponse(int *Errno);
    void Process();
    bool TryClose();
    void Close();

    const sockaddr_in *Getsockaddr() const;
    const char *GetIP() const;
    int GetPort() const;
    int GetFd() const;
    int GetClientCount() const;
    bool IsKeepAlive() const;

    static void LoadCacheFile();
    static std::unordered_map<std::string, std::string> cacheFile;

    static bool isET;
    static std::atomic<int> clientCount;
    static std::string srcDir;
    static long timeout;

  private:
    off_t offset_;
    size_t remaining_;
    Buffer buffer_;
    int fd_;
    sockaddr_in addr_;
    std::atomic<bool> isClose_;
    bool isRange_;
    HttpRequest httpReq_;
    HttpResponse httpRes_;
};

#endif