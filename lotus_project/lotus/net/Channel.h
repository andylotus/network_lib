#ifndef LOTUS_NET_CHANNEL_H
#define LOTUS_NET_CHANNEL_H

#include <sys/epoll.h>
#include "Callbacks.h"
#include "noncopyable.h"

#include <memory>
#include <functional>
#include <string>


namespace lotus
{
namespace net
{

class EventLoop;

class Channel : noncopyable
{
 public:
  typedef std::function<void()> EventCallback;
  typedef std::function<void(Timestamp)> ReadEventCallback;

  Channel(EventLoop* loop, int fd);
  ~Channel();

  static const int kNoneEvent = 0;
  static const int kReadEvent = EPOLLIN | EPOLLPRI;
  static const int kWriteEvent = EPOLLOUT;

  void handleEvent(Timestamp receiveTime);
  
  void setReadCallback(const ReadEventCallback& cb)
  { readCallback_ = cb; }
  void setWriteCallback(const EventCallback& cb)
  { writeCallback_ = cb; }
  void setCloseCallback(const EventCallback& cb)
  { closeCallback_ = cb; }
  
  void setErrorCallback(const EventCallback& cb)
  { errorCallback_ = cb; }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const std::shared_ptr<void>&);

  int fd() const { return fd_; } 
  int events() const { return events_; }
  void set_events(int revt) { events_ = revt; } // used by Epoll
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  void enableReading() { events_ |= kReadEvent; update(); }
  void enableWriting() { events_ |= kWriteEvent; update(); }
  void disableWriting() { events_ &= ~kWriteEvent; update(); }
  void disableAll() { events_ = kNoneEvent; update(); }
  bool isWriting() const { return events_ & kWriteEvent; }

  // for Epoller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  // for debug
  std::string eventsToString() const;

  void doNotLogHup() { logHup_ = false; }

  EventLoop* ownerLoop() { return loop_; }
  
  void remove();

 private:
  void update();
  void handleEventWithGuard(Timestamp receiveTime);

  EventLoop* loop_;			// EventLoop which the Channel belongs to
  const int  fd_;			
  int events_;		// events watched and returned, like 'EPULLIN'
  int index_;		// used by EPoller --- status of the channel, kNew = -1 (default)
  bool logHup_;		// for POLLHUP

  std::weak_ptr<void> tie_;
  bool tied_;
  bool eventHandling_;		
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}
}
#endif  // LOTUS_NET_CHANNEL_H
