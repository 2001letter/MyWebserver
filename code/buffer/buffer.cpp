#include "buffer.h"

Buffer::Buffer(int initSize) : readIndex_(0), writeIndex_(0), buffer_(initSize) {}

size_t Buffer::ReadableBytes() const { return writeIndex_ - readIndex_; }

size_t Buffer::WriteableBytes() const { return buffer_.size() - writeIndex_; }

size_t Buffer::PrependableBytes() const { return readIndex_; }

const char *Buffer::Peek() const { return &buffer_[readIndex_]; }

void Buffer::Retrieve(size_t len) { readIndex_ += len; }

void Buffer::RetrieveAll() {
    readIndex_ = 0;
    writeIndex_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(&buffer_[readIndex_], ReadableBytes());
    RetrieveAll();
    return str;
}

const char *Buffer::BeginWritePtr() const { return &buffer_[writeIndex_]; }

void Buffer::Append(const char *str, size_t len) {
    EnsureWriteable_(len);
    std::copy(str, str + len, BeginPtr_() + writeIndex_);
    writeIndex_ += len;
}

void Buffer::Append(const std::string &str) {
    Append(str.c_str(), str.length());
}

ssize_t Buffer::ReadFd(int fd, int *Errno) {
    char buff[65535];
    size_t writeable = WriteableBytes();
    struct iovec iov[2];
    iov[0].iov_base = BeginPtr_() + writeIndex_;
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *Errno = errno;
    } else if (static_cast<size_t>(len) <= writeable) {
        writeIndex_ += len;
    } else {
        writeIndex_ = buffer_.size();
        Append(buff, len - writeable);
    }

    return len;
}

ssize_t Buffer::WriteFd(int fd, int *Errno) {
    ssize_t len = write(fd, &buffer_[readIndex_], ReadableBytes());
    if (len < 0) {
        *Errno = errno;
    } else {
        Retrieve(len);
    }
    return len;
}

char *Buffer::BeginPtr_() { return &buffer_[0]; }

void Buffer::EnsureWriteable_(size_t len) {
    if (len > WriteableBytes()) {
        MakeSpace_(len);
    }
}

void Buffer::MakeSpace_(size_t len) {
    if (len <= WriteableBytes() + PrependableBytes()) {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readIndex_, BeginPtr_() + writeIndex_, BeginPtr_());
        readIndex_ = 0;
        writeIndex_ = readable;
    } else {
        buffer_.resize(writeIndex_ + len + 1);
    }
}
