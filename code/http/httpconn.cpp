#include "httpconn.h"

bool HttpConn::isET;
std::atomic<int> HttpConn::clientCount;
std::string HttpConn::srcDir;
long HttpConn::timeout;
std::unordered_map<std::string, std::string> HttpConn::cacheFile{};

HttpConn::HttpConn()
    : iovCount_(0), offset_(nullptr), remaining_(0), fd_(0), addr_({0}), isClose_(false), isRange_(false), httpRes_(new HttpResponse) {}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::HttpInit(int fd, const sockaddr_in &addr) {
    if(offset_) {
        offset_ = nullptr;
    }
    if(!httpRes_){
        httpRes_ = new HttpResponse;
    }
    fd_ = fd;
    addr_ = addr;
    isClose_ = false;
    isRange_ = false;
    clientCount++;
    buffer_.RetrieveAll();
    DEBUG_LOG(LOG_INFO, "New Client[{}] {}:{} connect, UserCount: {}", fd, GetIP(), GetPort(), GetClientCount());
}

ssize_t HttpConn::ReadRequest(int *Errno) {
    ssize_t len;
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
        len = writev(fd_, iov_, iovCount_);
        if (len < 0) {
            *Errno = errno;
            break;
        }
        if (iov_[0].iov_len == 0) { // buffer已写完，写文件
            if (httpRes_->GetFileFd() != -1) {
                remaining_ = httpRes_->GetFileSize() - *offset_;
                len = sendfile(fd_, httpRes_->GetFileFd(), offset_, remaining_);
                if (len <= 0) {
                    *Errno = errno;
                    break;
                }
            } else {
                break;
            }
        }
        if (iov_[0].iov_len >= static_cast<size_t>(len)) { // buffer里的数据没有写完
            iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            buffer_.Retrieve(len);
        } else {
            buffer_.RetrieveAll();
            iov_[0].iov_len = 0;
        }
        // else if (iovCount_ == 2) { // 映射的文件没写完
        //     iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (len - iov_[0].iov_len);
        //     iov_[1].iov_len -= (len - iov_[0].iov_len);
        //     if (iov_[0].iov_len) {
        //         buffer_.RetrieveAll();
        //         iov_[0].iov_len = 0;
        //     }
        // }
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
    httpRes_->ResponseInit(srcDir, httpReq_.GetPath(), IsKeepAlive(), HttpConn::timeout, mes, stateCode);
    if (httpReq_.GetMethod() == "GET" || httpReq_.GetMethod() == "HEAD") {
        httpRes_->MakeResponse(buffer_);
    } else if (httpReq_.GetMethod() == "POST") {
        httpRes_->MakeResponse(buffer_, httpReq_.GetBody());
    }

    std::string resq(buffer_.Peek(), buffer_.ReadableBytes());
    iov_[0].iov_base = const_cast<char *>(buffer_.Peek());
    iov_[0].iov_len = buffer_.ReadableBytes();
    iovCount_ = 1;
    if (httpRes_->GetFileFd() != -1) {
        offset_ = httpRes_->GetFileStart();
    }
    // if (httpRes_->GetFileSize() > 0 && httpRes_->GetFilePtr()) {
    //     iov_[1].iov_base = httpRes_->GetFilePtr();
    //     iov_[1].iov_len = httpRes_->GetFileSize();
    //     iovCount_ = 2;
    // }
}

void HttpConn::Close() {
    httpRes_->Close();
    delete httpRes_;
    httpRes_ = nullptr;
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
