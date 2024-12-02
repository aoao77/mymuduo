#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

EventLoopTheadPool::EventLoopTheadPool(EventLoop* baseloop, const std::string& nameArg)
    : baseloop_(baseloop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0)
{

}

EventLoopTheadPool::~EventLoopTheadPool() {

}

void EventLoopTheadPool::start(const ThreadInitCallback& cb = ThreadInitCallback()) {
    for (int i = 0; i < numThreads_; i++)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }
    
    if (numThreads_ == 0 && cb) {
        cb(baseloop_);
    }
}

EventLoop* EventLoopTheadPool::getNextLoop() {
    EventLoop* loop = baseloop_;

    if (loops_.size() > 0) {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopTheadPool::getAllLoops() {
    if (loops_.empty()) {
        return std::vector<EventLoop*>(1, baseloop_);
    } else {
        return loops_;
    }
}