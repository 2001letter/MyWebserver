#include "httprequest.h"

HttpRequest::HttpRequest() : state_(LINE) {}

void HttpRequest::RequestInit() {
    state_ = LINE;
    method_ = path_ = version_ = "";
    head_.clear();
    body_.clear();
}

bool HttpRequest::Parse(Buffer &buffer) {
    size_t readable = buffer.ReadableBytes();
    while (readable > 0 && state_ != FINISH) {
        auto lineEnd = std::search(buffer.Peek(), buffer.BeginWritePtr(), "\r\n", "\r\n" + 2); // 以\r\n分割消息
        std::string line(buffer.Peek(), lineEnd);
        switch (state_) {
        case LINE:
            if (!ParseLine_(line)) {
                return false;
            }
            ParsePath_();
            state_ = HEAD;
            break;
        case HEAD:
            if (!ParseHead_(line)) {
                state_ = BODY; // 匹配失败说明请求头结束了，匹配请求体
            }
            if (buffer.ReadableBytes() <= 2) { // GET请求
                state_ = FINISH;
            }
            break;
        case BODY:
            if(!ParseBody_(line)){
                return false;
            }
            state_ = FINISH;
            break;
        default:
            return false;
        }
        if (lineEnd == buffer.BeginWritePtr()) {
            buffer.RetrieveAll();
            break;
        }
        buffer.Retrieve(lineEnd + 2 - buffer.Peek());
    }
    DEBUG_LOG(LOG_DEBUG, "Request: {} {} {}", method_, path_, version_);
    return true;
}

bool HttpRequest::IsKeepAlive() const { 
    // Connection: keep-alive
    auto it = head_.find("Connection");
    if(it != head_.end()){
        return it->second == "keep-alive";
    }
    return false;
}

bool HttpRequest::IsRange() const { 
    return head_.count("Range"); 
}

const std::string &HttpRequest::GetMethod() const {
  return method_;
}

const std::string &HttpRequest::GetPath() const { return path_; }

const std::string &HttpRequest::GetVersion() const {
    return version_;
}

void HttpRequest::GetHeadMes(std::string &mes) {
    auto it = head_.find("Range");
    if(it!= head_.end()){
        mes = it->second;
    }
}


const std::unordered_map<std::string, std::string> &
HttpRequest::GetBody() const {
  return body_;
}

bool HttpRequest::ParseLine_(const std::string &line) {
    std::regex patten("^([A-Z]+) (\\S+) (HTTP\\/\\d+\\.\\d+)$");
    std::smatch matches;
    if (std::regex_match(line, matches, patten)) {
        method_ = matches[1];
        path_ = matches[2];
        version_ = matches[3];
        if(version_ != "HTTP/1.1"){
            return false;
        }
        return true;
    }
    DEBUG_LOG(LOG_ERROR, "Parse Request line error");
    return false;
}

bool HttpRequest::ParseHead_(const std::string &line) {
    std::regex patten("^([\\w-]+):\\s*(.*?)\\s*$");
    std::smatch matches;
    if (std::regex_match(line, matches, patten)) {
        head_[matches[1]] = matches[2];
        return true;
    }
    return false;
}

bool HttpRequest::ParseBody_(const std::string &line) { 
    // {"username": "name", "password": "pwd"}
    return ParseJson_(line);
}

void HttpRequest::ParsePath_() { 
    // GET / HTTP/1.1
    // GET /picture.html HTTP/1.1
    // GET /common.js HTTP/1.1
    // /auth.html?tab=login
    if(path_ == "/"){
        path_ = "/index.html";
    }
    if(path_.find("?") != std::string::npos){
        path_.erase(path_.find("?"));
    }
    // todo,在这里判断请求的URL是否合法
}

bool HttpRequest::ParseJson_(const std::string &json) { 
    // {"username":"1","password":"1"}
    // 去掉首尾空格
    size_t start = json.find_first_not_of("\t\n\r");
    size_t end = json.find_last_not_of("\t\n\r");
    if(start == std::string::npos || end == std::string::npos){
        return false;
    }
    std::string trimed = json.substr(start, end - start + 1);
    // 检查并去除首尾花括号
    if(trimed.size() < 2 || trimed.front() != '{' || trimed.back() != '}'){
        DEBUG_LOG(LOG_ERROR, "Invalid JSON");
        return false;
    }
    std::string content = trimed.substr(1, trimed.size() - 2);
    // "username":"1","password":"1"
    // "username":"111","email":"22223@ex.com","password":"14711"
    size_t index = 0;
    while(index < content.size()){
        // 跳过空格
        while(index < content.size() && std::isspace(content[index])){
            ++index;
        }
        if(index >= content.size()){
            return false;
        }
        // 查找key
        if(content[index] != '"'){
            return false;
        }
        size_t keyStart = ++index;
        size_t keyEnd = content.find('"', keyStart);
        if(keyEnd == std::string::npos){
            return false;
        }
        std::string key = content.substr(keyStart, keyEnd - keyStart);
        index = ++keyEnd;
        // 跳过逗号,引号
        if(content[index] == ':'){
            ++index;
        }
        // 查找value
        size_t valueStart = ++index;
        size_t valueEnd = content.find('"', valueStart);
        if(valueEnd == std::string::npos){
            return false;
        }
        std::string value = content.substr(valueStart, valueEnd - valueStart);
        body_[key] = value;
        index = valueEnd + 2;
    }
    return true; 
}
