#ifndef LOTUS_NET_EVENTLOOP_H
#define LOTUS_NET_EVENTLOOP_H

#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <functional>
#include <assert.h>

#include "noncopyable.h"
#include "Epoll.h"
#include "TimerId.h"
#include "CurrentThread.h"

namespace lotus
{
  namespace net
  {
  class Channel;
  class TimerQueue;
    
  class EventLoop : public noncopyable
  {
   public:
    EventLoop();
    ~EventLoop();  

    // Must be called in the same thread as creation of the object.
    void loop();
    
	//thread-safe(can be called from another thread)
    void quit();

    void assertInLoopThread();
    bool isInLoopThread() const; 

    static EventLoop* getEventLoopOfCurrentThread();
    
    void wakeup();
    void updateChannel(Channel* channel); //update channel(fd) in epoll
    void removeChannel(Channel* channel); //remove channel(fd) from epoll, but not destroy channel object

    typedef std::function<void()> Functor;
    typedef std::function<void()> TimerCallback;
    
    //Thread-safe
    void runInLoop(const Functor& cb);
    void queueInLoop(const Functor& cb);
    
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    TimerId runAfter(double delay, const TimerCallback& cb);
    TimerId runEvery(double interval, const TimerCallback& cb);
    void cancel(TimerId timerId);

   private:
    void abortNotInLoopThread();
    void printActiveChannels() const; // for debug
    void handleRead();  // for wakeupfd Channel
    void doPendingFunctors();
    
    Epoll poller_;
    bool looping_; /* atomic */
    bool quit_;
    bool eventHandling_; /* atomic */
    bool callingPendingFunctors_;
    const pid_t threadId_;
    std::vector<Channel*> activeChannels_; //active channels returned by Epoll
    std::unique_ptr<TimerQueue> timerQueue_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    std::vector<Functor> pendingFunctors_;
    std::mutex mutex_;
    
  };

 }
}
#endif  // LOTUS_NET_EVENTLOOP_H
