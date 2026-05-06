#ifndef LOG_H
#define LOG_H

#include "blockqueue.h"
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <regex>
#include <thread>

namespace fs = std::filesystem;

enum LogLevel { LOG_INFO, LOG_WARN, LOG_ERROR, LOG_DEBUG };

#define DEBUG_LOG(level, formats, ...)                             \
    do {                                                           \
        Log *log = Log::Instance();                                \
        if (log->isOpen() && (log->getLevel() >= level)) {         \
            std::string str = std::format(formats, ##__VA_ARGS__); \
            log->logMessage(level, str);                           \
        }                                                          \
    } while (0);

class Log {
  public:
    static Log *Instance();
    void init(LogLevel level, const std::string &path, bool isAsyn);
    void logMessage(LogLevel level, const std::string &formats); // 将日志加入阻塞队列
    static void asynWriteThread();
    bool isOpen() const;
    LogLevel getLevel() const;

  private:
    Log();
    ~Log();

    std::string findFileOpen(const std::string &path);
    void syncWrite();

    std::string path_; // 日志路径
    LogLevel level_;   // 日志等级
    bool isOpen_;      // 是否开启日志
    int today_;        // 按当天日期分类
    int index_;
    int countLine_;                   // 当前写入行数
    static const int MAX_LINE = 10000; // 一个文件中最大行数
    std::ofstream file_;              // 打开的文件对象
    bool isAsyn_;                     // 是否开启异步
    std::mutex mutex_;
    std::unique_ptr<BlockQueue<std::string>> blockDeq_;
    std::unique_ptr<std::thread> writeThread_;
};

#endif