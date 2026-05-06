#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlpool.h"
#include "httpconn.h"
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

namespace fs = std::filesystem;

class HttpResponse {
  public:
    HttpResponse();
    ~HttpResponse();
    void Close();
    void ResponseInit(const std::string &dir, const std::string &path, bool keepAlive, long timeout,
                      const std::string &mes, const std::string &code);

    void MakeResponse(Buffer &buffer);
    void MakeResponse(Buffer &buffer, const std::unordered_map<std::string, std::string> &body);

    char *GetFilePtr();
    uintmax_t GetFileSize() const;
    int GetFileFd() const;
    off_t* GetFileStart();
    uintmax_t GetFileEnd() const;

  private:
    void FreeMmap_();
    void AddStateLine_(Buffer &buffer);
    void AddResHeader_(Buffer &buffer);
    void AddResBody_(Buffer &buffer);
    void ErrorHtml_();
    bool UserVerify_(const std::unordered_map<std::string, std::string> &body);
    bool ParseHeadMes_(const std::string &mes);
    std::string GetSuffixType_() const;
    bool isKeepAlive_;
    long timeout_;
    std::string headMes_;
    std::string version_, stateCode_, stateMes_, srcDir_, path_, body_;
    std::unordered_map<std::string, std::string> header_;
    // 响应体
    char *contentFile_;
    int fileFd_;
    fs::file_status fileStat_;
    uintmax_t fileSize_;
    off_t fileStart_;
    uintmax_t fileEnd_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<std::string, std::string> CODE_STATUS;
    static const std::unordered_map<std::string, std::string> CODE_PATH;
};

#endif