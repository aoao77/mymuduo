#pragma once

#include "Poller.h"
#include <sys/epoll>

class EPollPoller : Poller
{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller();

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
private:
    static const int kInitEventListSize = 16;

    void fillActiveChannels(int numEvents, ChannelList* channels);
    void update(int operation, Channel* channel);

    using EventList = std::vector<epoll_event>;

    Eventlist events_;
    int epollfd_;

};


