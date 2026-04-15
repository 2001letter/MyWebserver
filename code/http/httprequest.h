#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "../buffer/buffer.h"
#include "../log/log.h"
#include <iostream>
#include <regex>
#include <unordered_map>

class HttpRequest {
  public:
    HttpRequest();
    ~HttpRequest() = default;
    void RequestInit();

    bool Parse(Buffer &buffer);

    bool IsKeepAlive() const;
    bool IsRange() const;
    const std::string &GetMethod() const;
    const std::string &GetPath() const;
    const std::string &GetVersion() const;
    void GetHeadMes(std::string &mes);
    const std::unordered_map<std::string, std::string> &GetBody() const;

  private:
    bool ParseLine_(const std::string &line);
    bool ParseHead_(const std::string &line);
    bool ParseBody_(const std::string &line);
    void ParsePath_();
    bool ParseJson_(const std::string &json);
    enum PARSE_STATE { LINE, HEAD, BODY, FINISH };
    PARSE_STATE state_;
    // 请求行
    std::string method_, path_, version_;
    // 请求头
    std::unordered_map<std::string, std::string> head_;
    // 请求体
    std::unordered_map<std::string, std::string> body_;
};

#endif