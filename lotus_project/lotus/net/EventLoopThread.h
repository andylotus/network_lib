#ifndef LOTUS_NET_EVENTLOOPTHREAD_H
#define LOTUS_NET_EVENTLOOPTHREAD_H

#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

#include "noncopyable.h"

namespace lotus
{
namespace net
{

class EventLoop;

class EventLoopThread : noncopyable
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
  ~EventLoopThread();
  EventLoop* startLoop();	// 启动线程，该线程就成为了IO线程

 private:
  void threadFunc();		// 线程函数

  EventLoop* loop_;			// loop_指针指向一个EventLoop对象
  bool exiting_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;		// when callback_ is not empty(NULL), it will be called as initialization回调函数在EventLoop::loop事件循环之前被调用
};

}
}

#endif  // LOTUS_NET_EVENTLOOPTHREAD_H

