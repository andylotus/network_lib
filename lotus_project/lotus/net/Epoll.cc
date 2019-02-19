#include "Epoll.h"
#include "Log.h"

#include <unistd.h>

namespace lotus
{
namespace net
{
	namespace
	{
	const int kNew = -1;
	const int kAdded = 1;
	}

Epoll::Epoll()
  : epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventVectSize) 
    {   if (epollfd_ < 0)
        LOG_FATAL<< "epoll_create error";
    }

Epoll::~Epoll()
{
   ::close(epollfd_);
}

Timestamp Epoll::poll(std::vector<Channel*>* pChannels)
{
    int numEvents = ::epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), -1);
    if(numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happended";
   
        for(int i = 0; i < numEvents; i++)
        {
            Channel* pChannel = static_cast<Channel*>(events_[i].data.ptr); //ptr set up in Epoll::update()
            pChannel->set_events(events_[i].events);
            pChannels->push_back(pChannel);
        }
         if (static_cast<size_t>(numEvents) == events_.size())
        {
          events_.resize(events_.size()*2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_TRACE << " nothing happended";
    }
    else
    {
        LOG_ERROR << "Epoll::poll()";
    }
    //return Timestamp::now();
    return std::chrono::system_clock::now();
}

void Epoll::update(Channel* pChannel)
{
    LOG_TRACE << "fd = " << pChannel->fd() << " events = " << pChannel->events();
    int index = pChannel->index();
    if(index == kNew) //add a new one
    {
        struct epoll_event ev;
        ev.data.ptr = pChannel;
        ev.events = pChannel->events();
        int fd = pChannel->fd();
        pChannel->set_index(kAdded);
        ::epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev);
    }
    else // kAdded --- update an existing one
    {
        struct epoll_event ev;
        ev.data.ptr = pChannel;
        ev.events = pChannel->events();
        int fd = pChannel->fd();
        ::epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ev);
    }
}
    
void Epoll::remove(Channel* pChannel)
{
        LOG_TRACE << "remove fd = " << pChannel->fd() << " events = " << pChannel->events();
        struct epoll_event ev;
        //ev.data.ptr = pChannel;
        //ev.events = pChannel->events();
        int fd = pChannel->fd();
        ::epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &ev);

}
    
}
}
