#include "server.h"

Server::Server(int port, int trigeMode, bool openLog, LogLevel logLevel, long timeout, const std::string &host,
               const std::string &name, const std::string &pwd, const std::string &db)
    : port_(port), isClose_(false), timeout_(timeout), timer_(new HeapTimer), epoller_(new Epoller),
      threadPool_(new ThreadPool(8)) {
    // 日志初始化
    if (openLog) {
        Log::Instance()->init(logLevel, "./log/", true);
        DEBUG_LOG(LOG_INFO, "------server init------");
    }

    resDir_ = fs::current_path();
    resDir_.append("/resource");
    HttpConn::clientCount = 0;
    HttpConn::srcDir = resDir_;
    HttpConn::timeout = timeout_;
    HttpConn::LoadCacheFile();
    SqlPool::Instance().Init(host, name, pwd, db);
    // LT ET模式初始化 socket初始化
    InitTrigeMode_(trigeMode);
    if (!InitSocket_()) {
        isClose_ = true;
    }
}

Server::~Server() {
    close(serverFd_);
    isClose_ = true;
}

void Server::start() {
    if (!isClose_) {
        DEBUG_LOG(LOG_INFO, "------Server start------");
    }
    // long timeout = -1;
    while (!isClose_) {
        // if (timeout_ > 0) {
        //     timeout = timer_->GetNextTick();
        // }
        int nfds = epoller_->Wait();
        for (int i = 0; i < nfds; i++) {
            int fd = epoller_->GetEventFd(i);
            uint32_t event = epoller_->GetEvent(i);
            if (fd == serverFd_) { // 有新连接加入
                DealListen_();
            } else if (event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                CloseClient_(client_[fd]);
            } else if (event & EPOLLIN) {
                DealRead_(client_[fd]);
            } else if (event & EPOLLOUT) {
                DealWrite_(client_[fd]);
            } else {
                DEBUG_LOG(LOG_ERROR, "Unexpect event!");
            }
        }
    }
}

void Server::InitTrigeMode_(int trigeMode) {
    serverEvent_ = EPOLLRDHUP;
    clientEvent_ = EPOLLONESHOT | EPOLLRDHUP; // EPOLLONESHOT保证事件被同一线程处理
    switch (trigeMode) {
    case 0: // server client LT
        break;
    case 1: // server ET client LT
        serverEvent_ |= EPOLLET;
        break;
    case 2: // server LT client ET
        clientEvent_ |= EPOLLET;
        break;
    case 3:
        serverEvent_ |= EPOLLET;
        clientEvent_ |= EPOLLET;
        break;
    default:
        serverEvent_ |= EPOLLET;
        clientEvent_ |= EPOLLET;
    }
    HttpConn::isET = clientEvent_ & EPOLLET;
}

bool Server::InitSocket_() {
    struct sockaddr_in serverAddr;
    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        return false;
    }
    SetNonBlocking_(serverFd_);
    int optval = 1;
    if (setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) == -1) {
        DEBUG_LOG(LOG_ERROR, "set socket setsockopt error !");
        close(serverFd_);
        return false;
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // 监听所有网络接口
    serverAddr.sin_port = htons(port_);
    if (bind(serverFd_, (struct sockaddr *)&serverAddr, sizeof(serverAddr))) {
        DEBUG_LOG(LOG_ERROR, "bind prot: {} error!", port_);
        close(serverFd_);
        return false;
    }
    if (listen(serverFd_, SOMAXCONN) < 0) {
        DEBUG_LOG(LOG_ERROR, "listen port: {} error!", port_);
        close(serverFd_);
        return false;
    }
    if (!epoller_->AddFd(serverFd_, serverEvent_ | EPOLLIN)) {
        DEBUG_LOG(LOG_ERROR, "add server fd to epoller error!");
        return false;
    }
    DEBUG_LOG(LOG_INFO, "Listen Port: {}", port_);
    return true;
}

// 设置非阻塞
int Server::SetNonBlocking_(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::DealListen_() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    do {
        int fd = accept(serverFd_, (struct sockaddr *)&clientAddr, &clientLen);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            DEBUG_LOG(LOG_ERROR, "client connect fail!");
        }
        client_[fd].HttpInit(fd, clientAddr);
        SetNonBlocking_(fd);
        epoller_->AddFd(fd, clientEvent_ | EPOLLIN);
        if (timeout_ > 0) {
            timer_->AddTimer(fd, timeout_, [this, fd] { CloseClient_(client_[fd]); });
        }
    } while (serverEvent_ & EPOLLET);
}

void Server::DealRead_(HttpConn &httpClient) {
    AdjustTimer_(httpClient);
    threadPool_->AddTask([this, &httpClient] { OnRead_(httpClient); });
}

void Server::OnRead_(HttpConn &httpClient) {
    int Errno = 0;
    int ret = httpClient.ReadRequest(&Errno);
    if (ret <= 0 ) {
        if(Errno == EAGAIN || Errno == EWOULDBLOCK){
            httpClient.Process();
            epoller_->ModFd(httpClient.GetFd(), clientEvent_ | EPOLLOUT);
            return;
        }
        DEBUG_LOG(LOG_ERROR, "Client[{}] read error with code {}, ret {}", httpClient.GetFd(), Errno, ret);
        httpClient.Close();
    }
}

void Server::DealWrite_(HttpConn &httpClient) {
    AdjustTimer_(httpClient);
    threadPool_->AddTask([this, &httpClient] { OnWrite_(httpClient); });
}

void Server::OnWrite_(HttpConn &httpClient) {
    int Errno = 0;
    int ret = httpClient.WriteResponse(&Errno);
    if (ret < 0) {
        if (Errno == EAGAIN) {
            epoller_->ModFd(httpClient.GetFd(), clientEvent_ | EPOLLOUT);
            return;
        }
        if (Errno == EPIPE || Errno == ECONNRESET) {
            CloseClient_(httpClient);
            return;
        }
    } else if (httpClient.IsKeepAlive()) {
        epoller_->ModFd(httpClient.GetFd(), clientEvent_ | EPOLLIN);
        return;
    }
    CloseClient_(httpClient);
}

void Server::AdjustTimer_(HttpConn &httpClient) {
    if (timeout_ > 0) {
        timer_->AdjustTimer(httpClient.GetFd(), timeout_);
    }
}

void Server::CloseClient_(HttpConn &httpClient) {
    int fd = httpClient.GetFd();
    epoller_->DelFd(fd);
    httpClient.Close();
    // timer_->DelTimer(fd);
    // client_.erase(fd);
}
