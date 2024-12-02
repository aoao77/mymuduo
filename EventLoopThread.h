#pragma once

#include "noncopyable.h"
#include "Thread.h"
#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;

class EventLoopThread : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
        const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();
private:
    void threadFunc();

    EventLoop* loop_;
    bool existing_;
    std::mutex mutex_;
    std::condition_variable cond_;
    Thread thread_;
    ThreadInitCallback callback_;
};