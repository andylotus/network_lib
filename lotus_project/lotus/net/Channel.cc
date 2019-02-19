#include "Log.h"
#include "Channel.h"
#include "EventLoop.h"

#include <sstream>
#include <assert.h>

using namespace lotus;
using namespace lotus::net;

Channel::Channel(EventLoop* loop, int fd)
  : loop_(loop),
    fd_(fd),
    events_(0),
    index_(-1),
    logHup_(true),
    tied_(false),
    eventHandling_(false)
{
}

Channel::~Channel()
{
  assert(!eventHandling_);
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
  tie_ = obj;
  tied_ = true;
}

void Channel::update()
{
  loop_->updateChannel(this);
}

// Before call this function, Make sure first call disableAll, for the very channel(fd)!
void Channel::remove()
{
  assert(isNoneEvent());
  loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
  std::shared_ptr<void> guard;
  if (tied_)
  {
    guard = tie_.lock();
    if (guard)
    {
      LOG_TRACE << "[6] usecount=" << guard.use_count();
      handleEventWithGuard(receiveTime);
      LOG_TRACE << "[12] usecount=" << guard.use_count();
    }
  }
  else
  {
    handleEventWithGuard(receiveTime);
  }
}

//EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
//EPOLLOUT：表示对应的文件描述符可以写；
//EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
//EPOLLERR：表示对应的文件描述符发生错误；
//EPOLLHUP：表示对应的文件描述符被挂断(output only)；
//EPOLLRDHUP: Stream socket peer closed connection, or shut down writing half of connection.
//POLLNVAL: fd does not actually refer to any open file, i.e. it was closed or never open to begin with.

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
  eventHandling_ = true;
  if ((events_ & EPOLLHUP) && !(events_ & EPOLLIN))
  {
    if (logHup_)
    {
      LOG_WARN << "Channel::handle_event() POLLHUP";
    }
    if (closeCallback_) {closeCallback_();}
  }
  if (events_ & EPOLLERR)
  {
    if (errorCallback_) errorCallback_();
  }
  if (events_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
  {
    if (readCallback_) readCallback_(receiveTime);
  }
  if (events_ & EPOLLOUT)
  {
    if (writeCallback_) writeCallback_();
  }
  eventHandling_ = false;
}

std::string Channel::eventsToString() const
{
  std::ostringstream oss;
  oss << fd_ << ": ";
  if (events_ & EPOLLIN)
    oss << "IN ";
  if (events_ & EPOLLPRI)
    oss << "PRI ";
  if (events_ & EPOLLOUT)
    oss << "OUT ";
  if (events_ & EPOLLHUP)
    oss << "HUP ";
  if (events_ & EPOLLRDHUP)
    oss << "RDHUP ";
  if (events_ & EPOLLERR)
    oss << "ERR ";

  return oss.str().c_str();
}
