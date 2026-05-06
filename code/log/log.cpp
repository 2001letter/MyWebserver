#include "log.h"

Log *Log::Instance() {
    static Log log;
    return &log;
}

void Log::init(LogLevel level, const std::string &path, bool isAsyn) {
    isOpen_ = true;
    level_ = level;
    path_ = path;
    countLine_ = 0;
    isAsyn_ = isAsyn;
    if (isAsyn_) {
        blockDeq_ = std::make_unique<BlockQueue<std::string>>();
        writeThread_ = std::make_unique<std::thread>(asynWriteThread);
    }

    if (!path_.empty() && !fs::exists(path_)) {
        fs::create_directories(path_);
    }
    // 2026-xx-xx-index.log
    {
        std::lock_guard<std::mutex> lck(mutex_);
        std::string fileName = path_ + findFileOpen(path_);
        file_.open(fileName, std::ios::app | std::ios::out);
    }
}

void Log::logMessage(LogLevel level, const std::string &formats) {
    // 1.日志格式 [level] 2026-xx-xx xx:xx:xx: formats
    time_t now;
    struct tm *sysTime;
    time(&now);
    sysTime = localtime(&now);
    if (today_ != sysTime->tm_mday || countLine_ > MAX_LINE) {
        std::string ofile, fileName;
        if (countLine_ > MAX_LINE) {
            ofile = std::format("/{}-{:02}-{:02}-{:02}.log", sysTime->tm_year + 1900, sysTime->tm_mon + 1,
                                sysTime->tm_mday, index_ + 1);
            index_ += 1;
            countLine_ = 0;
        }
        if (today_ != sysTime->tm_mday) {
            // todo 
            // 压缩前一天的日志
            ofile = std::format("/{}-{:02}-{:02}-{:02}.log", sysTime->tm_year + 1900, sysTime->tm_mon + 1,
                                sysTime->tm_mday, 1);
            today_ = sysTime->tm_mday;
            index_ = 1;
            countLine_ = 0;
        }
        fileName = path_ + ofile;
        {
            std::lock_guard<std::mutex> lck(mutex_);
            file_.close();
            file_.open(fileName, std::ios::app | std::ios::out);
        }
    }
    // 2.将日志加入阻塞队列
    std::string lv;
    std::string date;
    switch (level) {
    case LOG_INFO:
        lv = "LOG_INFO";
        break;
    case LOG_WARN:
        lv = "LOG_WARN";
        break;
    case LOG_ERROR:
        lv = "LOG_ERROR";
        break;
    case LOG_DEBUG:
        lv = "LOG_DEBUG";
        break;
    default:
        lv = "LOG_INFO";
    }
    date = std::format("{}-{:02}-{:02} {:02}:{:02}:{:02}:", sysTime->tm_year + 1900, sysTime->tm_mon + 1,
                       sysTime->tm_mday, sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec);
    std::string message = std::format("[{:9}] {} {}", lv, date, formats);

    {
        std::lock_guard<std::mutex> lck(mutex_);
        if (isAsyn_ && blockDeq_ && !blockDeq_->full()) { // 异步写
            blockDeq_->push_back(message);
            ++countLine_;
        } else {
            // 同步写
            file_ << message << std::endl;
            ++countLine_;
        }
    }
}

void Log::asynWriteThread() { Log::Instance()->syncWrite(); }

void Log::syncWrite() {
    std::string message;
    while (blockDeq_->pop_front(message)) {
        std::lock_guard<std::mutex> lck(mutex_);
        file_ << message << std::endl;
    }
}

bool Log::isOpen() const {
    return isOpen_;
}

LogLevel Log::getLevel() const {
    return level_;
}

Log::Log(): today_(0), index_(1), countLine_(0), isAsyn_(false), blockDeq_(nullptr), writeThread_(nullptr){}

Log::~Log() {
    while(!blockDeq_->empty()){
        blockDeq_->WakeCustomer();
    }
    blockDeq_->Close();
    writeThread_->join();
    std::lock_guard<std::mutex> lck(mutex_);
    file_.close();
}

std::string Log::findFileOpen(const std::string &path) {
    time_t now;
    struct tm *sysTime;
    time(&now);
    sysTime = localtime(&now);
    std::string date = std::format("{}-{:02}-{:02}", sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday);
    std::regex pattenIndex("(\\d+)(\\.log$)"); // index.log
    std::regex pattenDate(date);
    std::smatch matches;
    std::string openFile;
    std::string index = "01";
    for (const auto &file : fs::directory_iterator(path)) {
        if (file.is_regular_file()) {
            std::string fileName = file.path().filename().string();
            if (std::regex_search(fileName, matches, pattenDate)) { // 匹配到当天的日志文件名
                std::regex_search(fileName, matches, pattenIndex);
                index = matches[1].str() > index ? matches[1].str() : index;
            }
        }
    }
    today_ = sysTime->tm_mday;
    index_ = stoi(index);
    openFile = std::format("/{}-{}.log", date, index);
    return openFile;
}
