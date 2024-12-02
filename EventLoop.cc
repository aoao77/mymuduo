#include "EventLoop.h"
#include "Logger.h"
#include "Channel.h"
#include "Poller.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

__thread EventLoop* t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

int createEventfd() {
    int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return eventfd;
}

// construct -> one thread per loop
// -> set read callback -> enable read call back
EventLoop::EventLoop() : 
    looping_(false),
    quit_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    callingPengindFunctors_(false)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);

    if(t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    } else {
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}


    
// disconstruct -> disable & remove -> other res (fd, ptr)
EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// loop -> status bit -> poller & channel handle -> 
void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    activeChannels_.clear();

    while (quit_ == false) {
        poller_->poll(kPollTimeMs, &activeChannels_);

        for(Channel* channel : activeChannels_) {
            channel->handleEvent(pollReturnTime_);
        }

        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// quit -> status bit -> wakeup
void EventLoop::quit() {
    quit_ = true;
    
    if(!isInLoopThread()) {
        wakeup();
    }
}

// runInLoop & queue in loop -> two conditions in wake up
void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
        wakeup();
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    if (!isInLoopThread() || callingPengindFunctors_) {
        wakeup();
    }
}
// handle read
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);

    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

// wakeup -> write
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);

    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

// doingPendingFunctors -> 
void EventLoop::doPendingFunctors() {
    callingPengindFunctors_ = true;

    std::vector<Functor> funtors;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        funtors.swap(pendingFunctors_);
    }

    for(const auto& functor : funtors) {
        functor();
    }

    callingPengindFunctors_ = false;
}