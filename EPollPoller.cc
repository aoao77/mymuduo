#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

const int kNew = -1; // the index of channel is -1
const int kAdded = 1;
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop* loop) 
    : Poller(loop),
    events_(kInitEventListSize),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC))
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
    
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());
    
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size(), timeoutMs));
    int saveErrno = errno;

    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
       LOG_INFO("%d events happened \n", numEvents); 
       fillActiveChannels(numEvents, activeChannels);
       
       if (numEvents == events_.size())
       {
            events_.resize(events_.size() * 2);
       }
    } else if (numEvents == 0) {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    } else {
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }

    return now;
}

// kNew, kDeleted -> kAdded
// kAdded & (noEvent) -> delete
// kAdded -> modify
void EPollPoller::updateChannel(Channel* channel) {
    // channels_, channel->index, update -> epoll_ctl
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted)
    {
        if (index == kNew) {
            channels_[channel->fd()] = channel;
        }
        channel->index = kAdded;
        update(EPOLL_CTL_ADD, channel);
    } else {
        if (channel->isNoneEvent() == true) {
            update(EPOLL_CTL_DEL, channel);
            channel->index = kDeleted;
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// delete from channel map and epoll fd
void EPollPoller::removeChannel(Channel* channel) {
    const int index = channel->index();

    channels_.erase(channel->fd());

    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) {
    for(int n = 0; n < numEvents; n++) {
        Channel* channel = static_cast<Channel*>(events_[n].data.ptr);
        channel->set_revents(events_[n].events);
        activeChannels->push_back(channel);
    }
}

void EPollPoller::update(int operation, Channel* channel) {
    // epoll_event -> fd, data.ptr, data.event
    epoll_event event;
    bzero(&event, sizeof event);

    event->events = channel->events();
    event->data.ptr = channel;
    event->data.fd = channel->fd();

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        } else {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
    
}