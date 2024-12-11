#include "TcpServer.h"
#include "TcpConnection.h"
#include "Logger.h"

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop,
    const InetAddress& addr,
    const std::string& nameArg,
    Option option) 
    : loop_(CheckLoopNotNull(loop)),
    ipPort_(addr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, addr, option == kReusePort)),
    threadPool_(new EventLoopTheadPool(loop, nameArg)),
    connectionCallback_(),
    messageCallback_(),
    started_(0),
    nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(std::bind(TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {
    for(auto& item : connections_) {
        TcpConnectionPtr conn(item.second);
        item.second.reset();

        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed(), conn));
    }
}

void TcpServer::setThreadNum(int numThread) {
    threadPool_->setThreadNum(numThread);
}

void TcpServer::start() {
    if (started_++ == 0) {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
    EventLoop* ioloop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    nextConnId_++;

    std::string connName = name_ + buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    sockaddr_in local;
    socklen_t localAddrLen = sizeof local;
    ::bzero(&local, sizeof local);
    if (::getsockname(sockfd, (sockaddr*)&localAddr, &localAddrLen) < 0) {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(localAddr);

    TcpConnectionPtr conn(new TcpConnection(
        ioloop,
        connName,
        sockfd,
        localAddr,
        peerAddr
    ));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_); 
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    conn->getLoop()->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
             name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());

    conn->getLoop()->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
