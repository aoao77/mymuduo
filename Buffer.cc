#include "Buffer.h"

#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>

ssize_t Buffer::readFd(int fd, int* saveErrno) {
    char extrabuffer[65536] = {0};

    struct iovec vec[2];

    const size_t writable = writableBytes();

    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuffer;
    vec[1].iov_len = sizeof extrabuffer;

    int vec_len = writable > sizeof extrabuffer ? 1 : 2;

    ssize_t n = ::readv(fd, vec, vec_len);

    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)
    {
        writerIndex_ += n;
    }
    else
    {
        writerIndex_ = buffer_.size();
        append(extrabuffer, n - writable);
    }

    return n;    
}

ssize_t Buffer::writeFd(int fd, int* saveErrno) {
    ssize_t n = ::write(fd, peek(), readableBytes());

    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}