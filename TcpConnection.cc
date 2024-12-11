#include "TcpConnection.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <netinet/tcp.h>

static EventLoop* checkLoopIsNull(EventLoop* loop) {
    if (loop == nullptr) {
        // logger
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, 
    const std::string& name,
    int sockfd,
    const InetAddress& localAddr,
    const InetAddress& peerAddr)
     : loop_(checkLoopIsNull(loop)),
     name_(name),
     state_(kConnecting),
     reading_(true),
     socket_(new Socket(sockfd)),
     channel_(new Channel(loop, sockfd)),
     localAddr_(localAddr),
     peerAddr_(peerAddr),
     highWaterMark_(1024 * 1024 * 64)
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", 
        name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string& buf) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf.c_str(), buf.size());
        } else {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        // TODO when to use this or shared_from_this
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::connectEstablished() {
    setstate(kConnected);

    channel_->tie(shared_from_this());
    // TODO why enable reading
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

// TODO kDisconnecting & kDisconnected
void TcpConnection::connectDestroyed() {
    if (state_ == kConnected) {
        setstate(kDisconnected);

        // disableAll and remove TODO
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

// TODO the relation of callback and handleRead
void TcpConnection::handleRead(Timestamp receiveTime) {
    int saveErrno = 0;

    ssize_t nread = inputBuffer_.readFd(channel_->fd(), &saveErrno);

    if (nread > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (nread == 0) {
        handleClose(); // why close TODO
    } else {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if(channel_->isWriting() == false) {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
        return;
    }

    int saveErrno = 0;
    ssize_t nwrote = ouputBuffer_.writeFd(channel_->fd(), &saveErrno);

    if(nwrote >= 0) {
        ouputBuffer_.retrieve(nwrote);
        if (ouputBuffer_.readableBytes() == 0) {
            if (writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(&TcpConnection::writeCompleteCallback_, shared_from_this()));
            }
            channel_->disableWriting();
            if (state_ == kDisconnecting)
            {
                shutdownInLoop();
            }
        }
    } else {
        LOG_ERROR("TcpConnection::handleWrite");
    }
}

void TcpConnection::handleClose() {
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setstate(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr conn(shared_from_this());
    // TODO who call the callback? loop or connection
    connectionCallback_(conn);
    closeCallback_(conn);
}

void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;

    // the usage of getsockopt
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }

    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}

void TcpConnection::sendInLoop(const void* message, size_t len) {
    ssize_t nwrote = 0;
    size_t remaining = 0;
    bool faultError = false;

    if (state_ == kDisconnected) {
        // error       
        LOG_ERROR("disconnected, give up writing!");
        return;
    }

    // channel第一次写事件 -> TODO 写事件什么时候会被置位
    if(channel_->isWriting() == false && ouputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), message, len);

        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
               loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE  RESET
                {
                    faultError = true;
                }
            }
        }
    }

    if (faultError == false && remaining > 0) {
        size_t oldLen = ouputBuffer_.readableBytes();

        if (oldLen < highWaterMark_
        && oldLen + remaining >= highWaterMark_
        && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }

        ouputBuffer_.append((char *)message + nwrote, len - nwrote);
        if(channel_->isWriting() == false) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdownInLoop() {
    if(channel_->isWriting() == false) {
        socket_->shutdownWrite();
    }
}
