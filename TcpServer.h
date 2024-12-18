#pragma once

#include "noncopyable.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "EventLoopThreadPool.h"

#include <functional>
#include <atomic>
#include <unordered_map>
#include <string>
#include <memory>

class TcpServer : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,
            const InetAddress& addr,
            const std::string& nameArg,
            Option option = kReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb;}
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb;}
    
    void setThreadNum(int numThread);

    void start();
private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_;

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;

    std::shared_ptr<EventLoopTheadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    ThreadInitCallback threadInitCallback_;

    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_;
};