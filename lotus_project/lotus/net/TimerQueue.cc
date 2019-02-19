#include <sys/time.h>
#include <chrono>
#include <cassert>
#include <sys/timerfd.h>
#include <unistd.h>
#include <cstring>
#include <strings.h>
#include <functional>
#include <algorithm>
#include <iostream>

#include "Log.h"
#include"TimerQueue.h"
#include"EventLoop.h"
 
namespace lotus
{
namespace net
{
	namespace detail
	{

	// 创建定时器
	int createTimerfd()
	{
		int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
									 TFD_NONBLOCK | TFD_CLOEXEC);
		if (timerfd < 0)
		{
		LOG_FATAL << "Failed in timerfd_create";
		}
		return timerfd;
	}

	// 计算超时时刻与当前时间的时间差
	struct timespec fromNow(Timestamp when)
	{
		using namespace std::chrono;
		TimerQueue::duration du = when - system_clock::now();

		struct timespec ts;
		ts.tv_sec = static_cast<time_t>(duration_cast<seconds>(du).count());

		ts.tv_nsec = static_cast<long>(duration_cast<nanoseconds>(du % seconds{1}).count());
		return ts;
	}

	// 清除定时器，避免一直触发
	void readTimerfd(int timerfd, Timestamp now)
	{
		uint64_t howmany;
		ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
		LOG_TRACE << "TimerQueue::handleRead() " << howmany ;
		if (n != sizeof howmany)
		{
		LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
		}
	}

	// reset the expiration time of the Timer重置定时器的超时时间
	void resetTimerfd(int timerfd, Timestamp expiration)
	{
		// wake up loop by timerfd_settime()
		struct itimerspec newValue;
		struct itimerspec oldValue;
		bzero(&newValue, sizeof newValue);
		bzero(&oldValue, sizeof oldValue);
		newValue.it_value = fromNow(expiration);
		int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
		if (ret)
		{
		LOG_ERROR << "timerfd_settime()";
		}
	}

	}
}
}


using namespace lotus;
using namespace lotus::net;
using namespace lotus::net::detail;

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop)
    , timerfd_(createTimerfd())
    , timerfdChannel_(loop, timerfd_)
    , callingExpiredTimers_(false)
{
  timerfdChannel_.setReadCallback(
      std::bind(&TimerQueue::handleRead, this));
  // we are always reading the timerfd, we disarm it with timerfd_settime.
  timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
  ::close(timerfd_);
  // do not remove channel, since we're in EventLoop::dtor();
  for (auto &it : heap_ )
  {
    delete it.timer_; //timer pointer in HeapEntry
  }
}
TimerId TimerQueue::addTimer(const TimerCallback& cb, Timestamp when, std::chrono::milliseconds interval_ms)
{
    auto *timer = new Timer(cb, when, interval_ms);
	  loop_->runInLoop(
      std::bind(&TimerQueue::addTimerInLoop, this, timer));
	  
  //addTimerInLoop(timer);
  return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
  loop_->assertInLoopThread();
  // 插入一个定时器，有可能会使得最早到期的定时器发生改变
  bool earliestChanged = insert(timer);

  if (earliestChanged)
  {
    // 重置定时器的超时时刻(timerfd_settime)
    detail::resetTimerfd(timerfd_, timer->expiration());
  }
}

bool TimerQueue::insert(Timer* timer)
{
  loop_->assertInLoopThread();

  bool earliestChanged = false;
  Timestamp when = timer->expiration();
 auto it = heap_.begin();
  // 如果timers_为空或者when小于timers_中的最早到期时间
  if (it == heap_.end() || when < it->expirationTime_)
  {
    earliestChanged = true;
  }

	timer->heapIndex_ = heap_.size();
    HeapEntry entry = { timer->expiration_, timer };
    heap_.push_back(entry);
    UpHeap(heap_.size() - 1);
	
	return earliestChanged;
}

void TimerQueue::cancel(TimerId timerId)
{
  loop_->runInLoop(
      std::bind(&TimerQueue::cancelInLoop, this, timerId));
  //cancelInLoop(timerId);
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
  loop_->assertInLoopThread();

	Timer* timer = timerId.timer_;
    int64_t index = timer->heapIndex_;
  //if index == -1, means it's already been cancelled, so nothing will happen
	if(index >=0) 
	{
		removeTimer(timer);
		delete timer;
	}
	else if(callingExpiredTimers_)
	{
    // 已经到期，并且正在调用回调函数的定时器
    cancelingTimers_.push_back(timer); //放入cancelingTimers_ vector 中的Timer×，会在reset中delete。留在heap_中的Timer*,由TimerQueue dtor销毁
	}
}

void TimerQueue::removeTimer(Timer* timer)
{
	int64_t index = timer->heapIndex_;
    if (!heap_.empty() && index < heap_.size())
    {
        if (index == heap_.size() - 1)
        {
            heap_.pop_back();
        }
        else
        {
            SwapHeap(index, heap_.size() - 1);
            heap_.pop_back();
            int64_t parent = (index - 1) / 2;
            if (index > 0 && heap_[index].expirationTime_ < heap_[parent].expirationTime_)
                UpHeap(index);
            else
                DownHeap(index);
        }
    }
	
	timer->heapIndex_ = -1; //set heapIndex_ as -1
}

 
void TimerQueue::handleRead()
{
  loop_->assertInLoopThread();
  Timestamp now = TimerQueue::now();
  detail::readTimerfd(timerfd_, now);		// 清除该事件，避免一直触发

  // 获取该时刻之前所有的定时器列表(即超时定时器列表)
  std::vector<Timer*> expired = getExpired(now);

  callingExpiredTimers_ = true;
  cancelingTimers_.clear();
  // safe to callback outside critical section
  for (auto &it : expired)
  {
    // 这里回调定时器处理函数
    it->run();
  }
  callingExpiredTimers_ = false;

  // 不是一次性定时器，需要重启
  reset(expired, now);
}

std::vector<Timer*> TimerQueue::getExpired(Timestamp now)
{
  std::vector<Timer*> expired;
    while (!heap_.empty() && heap_[0].expirationTime_ <= now)
    {
        Timer* timer = heap_[0].timer_;
		    expired.push_back(timer);
        removeTimer(timer);
        //timer->OnTimer(now);
    }
  return expired;
}

void TimerQueue::reset(const std::vector<Timer*>& expired, Timestamp now)
{
  Timestamp nextExpire;

  for (auto & it : expired)
  {
	 
    // 如果是重复的定时器并且是未取消定时器，则重启该定时器
    if (it->repeat()
        && std::find(cancelingTimers_.begin(), cancelingTimers_.end(), it) == cancelingTimers_.end())
    {
      it->restart(now);
      insert(it);
    }
    else
    {
      // 一次性定时器或者已被取消的定时器是不能重置的，因此删除该定时器
      // FIXME move to a free list
      delete it; // FIXME: no delete please
    }
  }

  if (!heap_.empty())
  {
    // 获取最早到期的定时器超时时间
    nextExpire = heap_.begin()->timer_->expiration();
  }
 //std::cout << (nextExpire.time_since_epoch()).count() << std::endl;
 // auto time = nextExpire.time_since_epoch().count();
  if(nextExpire > Timestamp())
  {
  //if (nextExpire.valid())
  ///{
    // 重置定时器的超时时刻(timerfd_settime)
    detail::resetTimerfd(timerfd_, nextExpire);
    //std::cout << "get in resetTimerfd" << std::endl;
  //}
  }
}

void TimerQueue::UpHeap(int64_t index)
{
    int64_t parent = (index - 1) / 2;
    while (index > 0 && heap_[index].expirationTime_ < heap_[parent].expirationTime_)
    {
        SwapHeap(index, parent);
        index = parent;
        parent = (index - 1) / 2;
    }
}
 
void TimerQueue::DownHeap(int64_t index)
{
    int64_t child = index * 2 + 1;
    while (child < heap_.size())
    {
        int64_t minChild = (child + 1 == heap_.size() || heap_[child].expirationTime_ < heap_[child + 1].expirationTime_)
            ? child : child + 1;
        if (heap_[index].expirationTime_ < heap_[minChild].expirationTime_)
            break;
        SwapHeap(index, minChild);
        index = minChild;
        child = index * 2 + 1;
    }
}
 
void TimerQueue::SwapHeap(int64_t index1, int64_t index2)
{
    HeapEntry tmp = heap_[index1];
    heap_[index1] = heap_[index2];
    heap_[index2] = tmp;
    heap_[index1].timer_->heapIndex_ = index1;
    heap_[index2].timer_->heapIndex_ = index2;
}

 Timestamp TimerQueue::now()
{
	return std::chrono::system_clock::now();
}
