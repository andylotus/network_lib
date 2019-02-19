#include"Timer.h"

namespace lotus
{
namespace net
{
	std::atomic<int64_t> Timer::id{0};

	void Timer::restart(Timestamp now)
	{
		if (repeat_) {
			expiration_ = now + interval_;
		}
		else {
			expiration_ = Timestamp{};
		}
	}

	Timer::Timer(const TimerCallback &cb, Timestamp when, std::chrono::milliseconds interval_ms)
		: callback_(cb)
		  , expiration_(when)
		  , interval_(interval_ms)
		  , repeat_(interval_ > std::chrono::milliseconds(0))
		  , sequence_(++id)
		  , heapIndex_(-1)
	{}


	void Timer::run() const
	{
		callback_();
	}

	Timestamp Timer::expiration() const 
	{
		return expiration_;
	}

	bool Timer::repeat() const 
	{
		return repeat_;
	}

	int64_t Timer::sequence() const 
	{
		return sequence_;
	}

}
}
