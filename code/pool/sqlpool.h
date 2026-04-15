#ifndef SQL_POOL_H
#define SQL_POOL_H

#include <condition_variable>
#include <cppconn/driver.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>

class SqlPool {
  public:
    using ConnectionPtr = std::unique_ptr<sql::Connection, std::function<void(sql::Connection *)>>;
    static SqlPool &Instance() {
        static SqlPool sqlPool;
        return sqlPool;
    }
    void Init(const std::string &host, const std::string &name, const std::string &pwd, const std::string &db,
              int maxConn = 8) {
        driver_ = get_driver_instance();
        for (int i = 0; i < maxConn; i++) {
            auto con = driver_->connect(host, name, pwd);
            con->setSchema(db);
            conn_.emplace(con);
        }
    }
    ConnectionPtr GetSqlConnection() {
        std::unique_lock<std::mutex> lck(mutex_);
        cond_.wait(lck, [this] { return !conn_.empty() || isClose_; });
        if (isClose_) {
            return ConnectionPtr(nullptr, [](sql::Connection *) {});
        }
        sql::Connection *sqlConn = conn_.front();
        conn_.pop();
        return ConnectionPtr(sqlConn, [this](sql::Connection *c) {
            std::unique_lock<std::mutex> lck(mutex_);
            conn_.push(c);
            cond_.notify_one();
        });
    }

  private:
    SqlPool() : isClose_(false), driver_(nullptr) {}
    ~SqlPool() {
        {
            std::lock_guard<std::mutex> lck(mutex_);
            isClose_ = true;
        }
        cond_.notify_all();
        std::lock_guard<std::mutex> lck(mutex_);
        while (!conn_.empty()) {
            delete conn_.front();
            conn_.pop();
        }
    }

    SqlPool(const SqlPool &) = delete;
    SqlPool &operator=(const SqlPool &) = delete;
    SqlPool(const SqlPool &&) = delete;
    SqlPool &operator=(SqlPool &&) = delete;

    bool isClose_;
    sql::Driver *driver_;
    std::queue<sql::Connection *> conn_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif