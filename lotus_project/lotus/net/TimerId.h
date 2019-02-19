#ifndef LOTUS_NET_TIMERID_H
#define LOTUS_NET_TIMERID_H

#include "copyable.h"

namespace lotus
{
namespace net
{

class Timer;

class TimerId : public copyable
{
 public:
  TimerId()
    : timer_(NULL),
      sequence_(0)
  {
  }

  TimerId(Timer* timer, int64_t seq)
    : timer_(timer),
      sequence_(seq)
  {
  }

  // default copy-ctor, dtor and assignment are okay

 friend class TimerQueue;

 private:
  Timer* timer_;
  int64_t sequence_;
};

}
}

#endif  // LOTUS_NET_TIMERID_H
