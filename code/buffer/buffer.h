#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <vector>
#include <atomic>
#include <sys/uio.h>    // iovec readv

class Buffer {
  public:
    Buffer(int initSize = 4096);
    ~Buffer() = default;
    size_t ReadableBytes() const;
    size_t WriteableBytes() const;
    size_t PrependableBytes() const;
    const char* Peek() const;
    void Retrieve(size_t len);
    void RetrieveAll();
    std::string RetrieveAllToStr(); 
    const char* BeginWritePtr() const;
    void Append(const char* str, size_t len);
    void Append(const std::string &str);

    ssize_t ReadFd(int fd, int* Errno);
    ssize_t WriteFd(int fd, int* Errno);

  private:
    char* BeginPtr_();
    void EnsureWriteable_(size_t len);
    void MakeSpace_(size_t len);

    std::atomic<size_t> readIndex_;
    std::atomic<size_t> writeIndex_;
    std::vector<char> buffer_;
};

#endif