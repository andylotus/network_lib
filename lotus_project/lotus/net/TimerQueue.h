#pragma once

#include <set>
#include <map>
#include <vector>
#include <chrono>
#include <memory>
#include "noncopyable.h"
#include "TimerId.h"
#include "Channel.h"
#include "Callbacks.h"
#include "Timer.h"

namespace lotus
{
namespace net
{	
	class EventLoop;

	class TimerQueue : public noncopyable
	{
	public:
		using Timestamp =std::chrono::time_point<std::chrono::system_clock>;
		using duration = std::chrono::system_clock::duration;
		using Ms = std::chrono::milliseconds;

		explicit TimerQueue(EventLoop *loop);

		~TimerQueue();

		TimerId addTimer(const EventCallback &cb, Timestamp when, std::chrono::milliseconds interval_ms);

		void cancel(TimerId timerId);

		void cancel_all();

		static Timestamp now();

	private:
		timespec fromNow(Timestamp when);

		void readTimerfd(int timerfd, Timestamp now);

		void resetTimerfd(int timerfd, Timestamp expiration);

		void addTimerInLoop(Timer *timer);

		void cancelInLoop(TimerId timerId);

		void handleRead();

		std::vector<Timer*> getExpired(Timestamp now);

		void reset(const std::vector<Timer*> &expired, Timestamp now);

		bool insert(Timer *timer);
		void removeTimer(Timer* timer);
		
	private:
		void UpHeap(int64_t index);
		void DownHeap(int64_t index);
		void SwapHeap(int64_t index1, int64_t index2);

	private:
		EventLoop *loop_;
		const int timerfd_;
		Channel timerfdChannel_;
		bool callingExpiredTimers_;
		std::vector<Timer*> cancelingTimers_;

		struct HeapEntry
		{
			Timestamp expirationTime_;
			Timer* timer_;
		};			

		std::vector<HeapEntry> heap_;
		
	};
}
}

/*
///Reference 


class TimerQueue
{
public:
    static unsigned long long GetCurrentMillisecs();
    void DetectTimers();
private:
    friend class Timer;
    void AddTimer(Timer* timer);
    void RemoveTimer(Timer* timer);
 
    void UpHeap(size_t index);
    void DownHeap(size_t index);
    void SwapHeap(size_t, size_t index2);
 
private:
    struct HeapEntry
    {
        unsigned long long time;
        Timer* timer;
    };
    std::vector<HeapEntry> heap_;
};

*/