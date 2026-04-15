#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},          {".xml", "text/xml"},          {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},          {".rtf", "application/rtf"},   {".pdf", "application/pdf"},
    {".word", "application/nsword"}, {".png", "image/png"},         {".gif", "image/gif"},
    {".jpg", "image/jpeg"},          {".jpeg", "image/jpeg"},       {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},         {".mpg", "video/mpeg"},        {".avi", "video/x-msvideo"},
    {".mp4", "video/mpeg4"},         {".gz", "application/x-gzip"}, {".tar", "application/x-tar"},
    {".css", "text/css "},           {".js", "text/javascript "},
};

const std::unordered_map<std::string, std::string> HttpResponse::CODE_STATUS = {
    {"200", "OK"},        {"206", "Partial Content"}, {"400", "Bad Request"},
    {"403", "Forbidden"}, {"404", "Not Found"},       {"500", "Internal Server Error"},
};

const std::unordered_map<std::string, std::string> HttpResponse::CODE_PATH = {
    {"400", "/400.html"},
    {"403", "/403.html"},
    {"404", "/404.html"},
};

HttpResponse::HttpResponse()
    : isKeepAlive_(false), timeout_(-1), headMes_(), stateCode_(), contentFile_(nullptr), fileFd_(-1), fileSize_(0),
      fileStart_(0), fileEnd_(0) {}

HttpResponse::~HttpResponse() { Close(); }

void HttpResponse::Close() {
    close(fileFd_);
    FreeMmap_();
}

void HttpResponse::ResponseInit(const std::string &dir, const std::string &path, bool keepAlive, long timeout,
                                const std::string &mes, const std::string &code) {
    FreeMmap_();
    version_ = "HTTP/1.1";
    srcDir_ = dir;
    path_ = path;
    isKeepAlive_ = keepAlive;
    timeout_ = timeout;
    headMes_ = mes;
    stateCode_ = code;
    stateMes_ = "";
    body_ = "\r\n";
    header_.clear();
    contentFile_ = nullptr;
    fileFd_ = -1;
    fileSize_ = 0;
    fileStart_ = 0;
    fileEnd_ = 0;
    fileStat_ = {};
}

void HttpResponse::MakeResponse(Buffer &buffer) {
    buffer.RetrieveAll();
    // 判断client请求的文件是否合法
    std::string reqFilePath = srcDir_ + path_;
    bool fileExist = fs::exists(reqFilePath);
    if (fileExist) {
        fileSize_ = fs::file_size(srcDir_ + path_);
        fileStat_ = fs::status(srcDir_ + path_);
        if (fs::is_directory(reqFilePath)) {
            stateCode_ = "404";
        } else if ((fileStat_.permissions() & fs::perms::others_read) == fs::perms::none) {
            stateCode_ = "403";
        } else if (ParseHeadMes_(headMes_)) {
            stateCode_ = "206";
        } else {
            stateCode_ = "200";
        }
    } else {
        stateCode_ = "404";
    }
    // 1.构造状态行  HTTP/1.1 200 OK
    AddStateLine_(buffer);
    // 2.构造响应头 Content-Type: text/html\r\nContent-Length: 114514\r\nConnection: keep-alive(close)
    // 错误状态码需要重定向path路径
    auto it = CODE_PATH.find(stateCode_);
    if (it != CODE_PATH.end()) {
        path_ = it->second;
        fileSize_ = fs::file_size(srcDir_ + path_);
        fileStat_ = fs::status(srcDir_ + path_);
    }
    AddResHeader_(buffer);
    // 3.空行
    // 4.构造响应体
    AddResBody_(buffer);
    buffer.Append(body_);
}

void HttpResponse::MakeResponse(Buffer &buffer, const std::unordered_map<std::string, std::string> &body) {
    buffer.RetrieveAll();
    // 需要连接数据库验证
    std::string mes;
    if (UserVerify_(body)) {
        mes = "{\"code\":200, \"data\":{\"username\":\"" + body.at("username") + "\", \"token\":\"1\"}}";
    } else {
        mes = "{\"code\":400}";
    }
    body_ += mes;
    AddStateLine_(buffer);
    buffer.Append("Connection: ");
    if (isKeepAlive_) {
        buffer.Append("keep-alive\r\n");
        buffer.Append("keep-alive: timeout=" + std::to_string(timeout_ / 1000) + "\r\n");
    } else {
        buffer.Append("close\r\n");
    }
    buffer.Append("Content-type: application/json\r\n");
    buffer.Append("Content-length: " + std::to_string(mes.length()) + "\r\n");
    buffer.Append(body_);
}

char *HttpResponse::GetFilePtr() { return contentFile_; }

off_t *HttpResponse::GetFileStart() { return &fileStart_; }

uintmax_t HttpResponse::GetFileEnd() const { return fileEnd_; }

uintmax_t HttpResponse::GetFileSize() const { return fileSize_; }

int HttpResponse::GetFileFd() const { return fileFd_; }

void HttpResponse::FreeMmap_() {
    if (contentFile_) {
        munmap((void *)contentFile_, fileSize_);
        contentFile_ = nullptr;
    }
}

void HttpResponse::AddStateLine_(Buffer &buffer) {
    // HTTP/1.1 200 OK
    auto it = CODE_STATUS.find(stateCode_);
    if (it != CODE_STATUS.end()) {
        stateMes_ = it->second;
    }
    std::string line = std::format("{} {} {}\r\n", version_, stateCode_, stateMes_);
    buffer.Append(line);
}

void HttpResponse::AddResHeader_(Buffer &buffer) {
    // Content-type: text/html
    // Content-length: xxxx
    // Connection: keep-alive
    buffer.Append("Connection: ");
    if (isKeepAlive_) {
        buffer.Append("keep-alive\r\n");
        buffer.Append("keep-alive: timeout=" + std::to_string(timeout_ / 1000) + "\r\n");
    } else {
        buffer.Append("close\r\n");
    }
    // Accept-Ranges: bytes
    // Content-Range: bytes 0-73400319/73400320
    if (stateCode_ == "206") {
        buffer.Append("Accept-Ranges: bytes\r\n");
        std::string rangeSize = std::format("Content-Range: {}-{}/{}\r\n", fileStart_, fileEnd_, fileSize_);
        buffer.Append(rangeSize);
        fileSize_ = fileEnd_ + 1;
    }
    buffer.Append("Content-type: " + GetSuffixType_() + "\r\n");
    buffer.Append("Content-length: " + std::to_string(fileSize_ - fileStart_) + "\r\n");
}

void HttpResponse::AddResBody_(Buffer &buffer) {
    int fd = open((srcDir_ + path_).c_str(), O_RDONLY);
    if (fd == -1) {
        DEBUG_LOG(LOG_ERROR, "open file {} error", path_);
        ErrorHtml_();
        return;
    }
    fileFd_ = fd;
    // contentFile_ = (char *)mmap(NULL, fileSize_, PROT_READ, MAP_PRIVATE, fd, 0);
    // if (contentFile_ == MAP_FAILED) {
    //     ErrorHtml_();
    //     DEBUG_LOG(LOG_ERROR, "mmap file {} error", path_);
    // }
}

void HttpResponse::ErrorHtml_() {
    body_ += "<!DOCTYPE html><html lang=\"zh-CN\"><body><p>500 - 服务器内部错误 </p></body></html>";
}

bool HttpResponse::UserVerify_(const std::unordered_map<std::string, std::string> &body) {
    if (body.empty()) {
        DEBUG_LOG(LOG_DEBUG, "Empty post request body");
        return false;
    }
    std::string username = body.at("username");
    std::string password = body.at("password");
    std::string sql;
    try {
        auto conn = SqlPool::Instance().GetSqlConnection();
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        if (path_ == "/api/login") {
            sql = std::format("select password from user where username='{}'", username);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(sql));
            if (res->next()) {
                if (res->getString("password") != password) {
                    DEBUG_LOG(LOG_DEBUG, "PASSWORD incorrect");
                    return false;
                }
            } else {
                DEBUG_LOG(LOG_DEBUG, "user {} is not exist", username);
                return false;
            }
        } else if (path_ == "/api/register") {
            sql = std::format("select 1 from user where username='{}'", username);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(sql));
            if (res->next()) {
                DEBUG_LOG(LOG_DEBUG, "user {} is exist", username);
                return false;
            }
            std::string email = body.at("email");
            std::unique_ptr<sql::PreparedStatement> pstmt(
                conn->prepareStatement("insert into user (username, password, email) values (?, ?, ?)"));
            pstmt->setString(1, username);
            pstmt->setString(2, password);
            pstmt->setString(3, email);
            pstmt->executeUpdate();
        }
    } catch (sql::SQLException &e) {
        DEBUG_LOG(LOG_ERROR, "SQL Error: {}, code: {}, status: {}", e.what(), e.getErrorCode(), e.getSQLState());
        return false;
    }
    return true;
}

bool HttpResponse::ParseHeadMes_(const std::string &mes) {
    // bytes=0-114514
    std::regex patten("(\\d+)-(\\d+)");
    std::smatch matches;
    if (std::regex_search(mes, matches, patten)) {
        fileStart_ = std::stoul(matches[1]);
        fileEnd_ = std::stoul(matches[2]);
        return true;
    }
    return false;
}

std::string HttpResponse::GetSuffixType_() const {
    // /picture.html /common.js
    std::string fpath = path_, suffix;
    suffix = fpath.substr(fpath.find_last_of("."));
    auto it = SUFFIX_TYPE.find(suffix);
    if (it != SUFFIX_TYPE.end()) {
        return it->second;
    }
    return "text/plain";
}
