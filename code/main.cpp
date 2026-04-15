#include "./server/server.h"
#include <iostream>
#include <signal.h>

int main() {
    signal(SIGPIPE, SIG_IGN);
    const std::string HOST = "tcp://127.0.0.1:3306";
    const std::string USER = "letter";
    const std::string PASS = "521117";
    const std::string DATABASE = "webserverdb";
    Server server(1316, 3, true, LOG_DEBUG, 0, HOST, USER, PASS, DATABASE);
    server.start();
    return 0;
}
