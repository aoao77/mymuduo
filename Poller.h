#pragma once

#include "noncopyable.h"
#include <unordered_map>
#include "Timestamp.h"
#include <vector>

class EventLoop;
class Channel;

class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    // is Poller has the Channel
    bool hasChannel(Channel* channel) const;

    // create a instance from EventLoop
    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_;
}