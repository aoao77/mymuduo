#pragma once

#include <vector>
#include <algorithm>
#include <string>
//---------prepend---------read----------write----------
class Buffer {
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer (size_t initialSize = kInitialSize) 
        : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend) {}

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }

    size_t writableBytes() const { return buffer_.size() - writerIndex_; }

    size_t prependableBytes() { return readerIndex_; }

    const char* peek() const  { //可读起始地址
        return begin() + readerIndex_;
    }

    void retrieve(size_t len) // string <- Buffer -> 修改index
    {
        if (len < readableBytes()) {
            readerIndex_ += len;
        }
        else {
            retrieveAll();
        }
    }

    void retrieveAll() {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len) {
        std::string str(peek(), len);
        retrieve(len);
        return str;
    }

    void ensureWriteableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
    }

    void append(const char* data, size_t len) {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite() {
        return begin() + writerIndex_;
    }
    const char* beginWrite() const {
        return begin() + writerIndex_;
    }

    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);
private:
    char* begin() {
        return &*buffer_.begin();
    } 

    const char* begin() const {
        return &*buffer_.cbegin();
    }

    void makeSpace(ssize_t len) { // if size not enough -> resize then move(copy), update index;
        if (prependableBytes() + writableBytes() < kCheapPrepend + len) {
            buffer_.resize(len + writerIndex_);
        } else {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ =  readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};