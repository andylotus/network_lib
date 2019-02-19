#ifndef LOTUS_NET_EVENTLOOPTHREADPOOL_H
#define LOTUS_NET_EVENTLOOPTHREADPOOL_H

#include <condition_variable>
#include <mutex>
#include <vector>
#include <functional>
#include "noncopyable.h"


namespace lotus
{

namespace net
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : public noncopyable
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThreadPool(EventLoop* baseLoop);
  ~EventLoopThreadPool();
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }
  void start(const ThreadInitCallback& cb = ThreadInitCallback());
  EventLoop* getNextLoop();

 private:

  EventLoop* baseLoop_;	// 与Acceptor所属EventLoop相同
  bool started_;
  int numThreads_;		// 线程数
  int next_;			// 新连接到来，所选择的EventLoop对象下标
  //boost::ptr_vector<EventLoopThread> threads_;		// IO线程列表
  //std::vector<std::unique_ptr<EventLoopThread> > threads_;
  std::vector<std::shared_ptr<EventLoopThread> > threads_;
  std::vector<EventLoop*> loops_;					// EventLoop列表
};

}
}

#endif  // LOTUS_NET_EVENTLOOPTHREADPOOL_H
