#include "TcpServer.h"
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

TcpServer::~TcpServer();

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

}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {

}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {

}
