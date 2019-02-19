#include "EventLoop.h"
#include "Channel.h"
#include "TimerQueue.h"
#include "Log.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <cassert>
#include <stdio.h>
#include <signal.h>


namespace lotus
{
  namespace CurrentThread
  {
    //cached thread id for current thread
    thread_local int t_cachedTid = 0;
  }
}

using namespace lotus;
using namespace lotus::net;

namespace
{
	// the EventLoop pointer of current thread
	thread_local EventLoop* t_loopInThisThread = 0;
	  
	int createWakeupEventfd()
	{
	  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	  if (evtfd < 0)
	  {
		LOG_FATAL<< "Failed in creating eventfd";
	  }
	  return evtfd;
	}
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()), 
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createWakeupEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_))
{

  LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
  //printf("In Printf, EventLoop created in thread %d\n", threadId_);
  // if current thread already created EventLoop object, then LOG_FATAL
  if (t_loopInThisThread)
  {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;
  }
  else
  {
    t_loopInThisThread = this;
  }
  wakeupChannel_->setReadCallback(
      std::bind(&EventLoop::handleRead, this));
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  ::close(wakeupFd_);
  t_loopInThisThread = NULL;
}

// EventLoop cannot be called by another thread (can only called by its own thread)
void EventLoop::loop()
{
  assertInLoopThread();
  assert(!looping_);
  looping_ = true;
  LOG_TRACE << "EventLoop " << this << " start looping";

  while(!quit_)
  {
    
    Timestamp pollReturnTime_ = poller_.poll(&activeChannels_);

    printActiveChannels();
 
    eventHandling_ = true;
    for (auto & currentActiveChannel : activeChannels_)
      currentActiveChannel->handleEvent(pollReturnTime_);
    
    activeChannels_.clear();//Must clear activeChannels_ each while loop
    eventHandling_ = false;
    
    doPendingFunctors();    
  }
  LOG_TRACE << "EventLoop " << this << " stop looping";

  looping_ = false;
}


void EventLoop::quit()
{
  quit_ = true;
  
  if (!isInLoopThread())
    wakeup();
}


void EventLoop::printActiveChannels() const
{
  
  for (auto &ch : activeChannels_)
    LOG_TRACE << "{" << ch->eventsToString() << "} ";

}

void EventLoop::runInLoop(const Functor& cb)
{
  if(isInLoopThread())
    cb();
  else
    queueInLoop(cb);
}

void EventLoop::queueInLoop(const Functor& cb)
{
  {
  std::lock_guard<std::mutex> lock(mutex_);
  pendingFunctors_.push_back(cb);
  }
  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_.update(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_.remove(channel);
}

void EventLoop::handleRead()
{
  uint64_t one = 1;

  ssize_t n = ::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::doPendingFunctors()
{
      std::vector<std::function<void()>> fns;
      callingPendingFunctors_ = true;
      {
          std::lock_guard<std::mutex> lock(mutex_);
          fns.swap(pendingFunctors_);
      }

      for (auto &f:fns)
          f();
  
      callingPendingFunctors_ = false;
}


TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
  return timerQueue_->addTimer(cb, time, std::chrono::milliseconds(0));
}

TimerId EventLoop::runAfter(double delay_sec, const TimerCallback& cb)
{
  Timestamp time(TimerQueue::now()+ std::chrono::milliseconds(static_cast<int64_t>(delay_sec*1000)));
  return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval_sec, const TimerCallback& cb)
{
  Timestamp time(TimerQueue::now()+ std::chrono::milliseconds(static_cast<int64_t>(interval_sec*1000)));
  return timerQueue_->addTimer(cb, time, std::chrono::milliseconds(static_cast<int64_t>(interval_sec*1000)));
}

void EventLoop::cancel(TimerId timerId)
{
  return timerQueue_->cancel(timerId);
}


bool EventLoop::isInLoopThread() const
{ 
  return threadId_ == CurrentThread::tid(); 
}

void EventLoop::assertInLoopThread()
{
    if (!isInLoopThread())    
      abortNotInLoopThread();
}

void EventLoop::abortNotInLoopThread()
{
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}