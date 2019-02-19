#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>

#include "noncopyable.h"
#include "Channel.h"
#include "Callbacks.h"

#include <vector>

/*
   typedef union epoll_data {
       void        *ptr;
       int          fd;
       uint32_t     u32;
       uint64_t     u64;
   } epoll_data_t;

   struct epoll_event {
       uint32_t     events;      // Epoll events 
       epoll_data_t data;        // User data variable 
   }
*/

namespace lotus
{
  namespace net
  {

class Epoll : noncopyable
{
public:
//using Timestamp =std::chrono::time_point<std::chrono::system_clock>;
    Epoll();
    ~Epoll();
    Timestamp poll(std::vector<Channel*>* pChannels);
    void update(Channel* pChannel);
    void remove(Channel* pChannel);
private:
    int epollfd_;
    std::vector<struct epoll_event> events_;
    static const int kInitEventVectSize = 16;
};

  }
}
#endif
