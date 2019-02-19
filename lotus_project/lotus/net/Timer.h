#pragma once

#include <atomic>
#include <chrono>
#include "Callbacks.h"
#include "TimerQueue.h"

namespace lotus
{
namespace net
{
	class Timer
	{
	public:
		//using Timestamp =std::chrono::system_clock::time_out;
		Timer(const TimerCallback &cb, Timestamp when,  std::chrono::milliseconds interval_ms);

		void run() const;

		Timestamp expiration() const;

		bool repeat() const;

		int64_t sequence() const;

		void restart(Timestamp now);

		static const int kMicroSecondsPerSecond = 1000 * 1000;
		static const int kMilliSecondsPerSecond = 1000;
		
	private:
		friend class TimerQueue;
		
		TimerCallback callback_;
		Timestamp expiration_;
		std::chrono::milliseconds interval_;
		bool repeat_;
		const int64_t sequence_;
		static std::atomic<int64_t> id;
		int64_t heapIndex_;
		
	};
}
}
