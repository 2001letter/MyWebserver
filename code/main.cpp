#include "./server/server.h"
#include <iostream>
#include <signal.h>

int main() {
    const std::string HOST = "tcp://127.0.0.1:3306";
    const std::string USER = "letter";
    const std::string PASS = "521117";
    const std::string DATABASE = "webserverdb";
    Server server(1316, 3, true, LOG_INFO, 60000, /*端口，ET/LT模式，日志开关，日志等级，超时时间(ms)*/
                  HOST, USER, PASS, DATABASE,      /*MYSQL配置*/
                  12, 8);                         /*线程池数量，连接池数量*/
    server.start();
    return 0;
}
