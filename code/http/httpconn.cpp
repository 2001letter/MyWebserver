#include "httpconn.h"

bool HttpConn::isET;
std::atomic<int> HttpConn::clientCount;
std::string HttpConn::srcDir;
long HttpConn::timeout;
std::unordered_map<std::string, std::string> HttpConn::cacheFile{};

HttpConn::HttpConn() : offset_(0), remaining_(0), fd_(0), addr_({0}), isClose_(false), isRange_(false) {}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::HttpInit(int fd, const sockaddr_in &addr) {
    offset_ = 0;
    fd_ = fd;
    addr_ = addr;
    isClose_ = false;
    isRange_ = false;
    clientCount++;
    buffer_.RetrieveAll();
    DEBUG_LOG(LOG_INFO, "New Client[{}] {}:{} connect, UserCount: {}", fd, GetIP(), GetPort(), GetClientCount());
}

ssize_t HttpConn::ReadRequest(int *Errno) {
    ssize_t len = -1;
    do {
        len = buffer_.ReadFd(fd_, Errno);
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HttpConn::WriteResponse(int *Errno) {
    ssize_t len = -1;
    do {
        len = send(fd_, buffer_.Peek(), buffer_.ReadableBytes(), 0);
        if (len < 0) {
            *Errno = errno;
            break;
        }
        if(buffer_.ReadableBytes() == 0){   // buffer已写完，写文件
            if (httpRes_.GetFileFd() != -1) {
                remaining_ = httpRes_.GetFileSize() - offset_;
                len = sendfile(fd_, httpRes_.GetFileFd(), &offset_, remaining_);
                if (len <= 0) {
                    *Errno = errno;
                    break;
                }
            } else {
                break;
            }
        }
        if(buffer_.ReadableBytes() >= static_cast<size_t>(len)){ // buffer里的数据没有写完
            buffer_.Retrieve(len);
        } else {
            buffer_.RetrieveAll();
        }
    } while (isET);
    return len;
}

void HttpConn::Process() {
    // init request
    httpReq_.RequestInit();
    // 解析
    std::string stateCode;
    if (httpReq_.Parse(buffer_)) {
        stateCode = "200";
    } else {
        stateCode = "400";
    }

    // 这里解析完成后，根据GET POST方法调用各自的处理函数
    std::string mes;
    httpReq_.GetHeadMes(mes);
    httpRes_.ResponseInit(srcDir, httpReq_.GetPath(), IsKeepAlive(), HttpConn::timeout, mes, stateCode);
    if (httpReq_.GetMethod() == "GET" || httpReq_.GetMethod() == "HEAD") {
        httpRes_.MakeResponse(buffer_);
    } else if (httpReq_.GetMethod() == "POST") {
        httpRes_.MakeResponse(buffer_, httpReq_.GetBody());
    }

    if (httpRes_.GetFileFd() != -1) {
        offset_ = httpRes_.GetFileStart();
    }
}

bool HttpConn::TryClose()
{
    bool expected = false;
    return isClose_.compare_exchange_strong(expected, true);
}

void HttpConn::Close() {
    isClose_ = true;
    close(fd_);
    clientCount--;
    DEBUG_LOG(LOG_INFO, "Client[{}] quit!", fd_);
}

const sockaddr_in *HttpConn::Getsockaddr() const { return &addr_; }

const char *HttpConn::GetIP() const { return inet_ntoa(addr_.sin_addr); }

int HttpConn::GetPort() const { return addr_.sin_port; }

int HttpConn::GetFd() const { return fd_; }

int HttpConn::GetClientCount() const { return clientCount; }

bool HttpConn::IsKeepAlive() const { return httpReq_.IsKeepAlive(); }

void HttpConn::LoadCacheFile()
{
    for(const auto& file : fs::directory_iterator(srcDir)){
        if(file.is_regular_file()){
            std::ifstream infile(file.path(), std::ios::binary);
            if(infile){
                std::string fileName = "/" + file.path().filename().string();
                std::string content((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
                cacheFile[fileName] = std::move(content);
            }
        }
    }
}
