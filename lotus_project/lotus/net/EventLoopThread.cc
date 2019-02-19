#include "EventLoopThread.h"
#include "EventLoop.h"
#include <functional>

using namespace lotus;
using namespace lotus::net;


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
  : loop_(NULL),
    exiting_(false),
    callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  loop_->quit();		// 退出IO线程，让IO线程的loop循环退出，从而退出了IO线程
  assert(thread_.joinable());
  thread_.join();
}

EventLoop* EventLoopThread::startLoop()
{
  //assert(!thread_.started());
  //thread_.start(); //此处启动新线程
  thread_ = std::thread(&EventLoopThread::threadFunc, this);

  {
    std::unique_lock<std::mutex> lock(mutex_);
    while (loop_ == NULL)
    {
      cond_.wait(lock);
    }
  }

  return loop_;
}

void EventLoopThread::threadFunc() //threadFunc() --线程函数 与startLoop（）--类似于主函数 属于两个不同的线程,可能并发执行
{
  EventLoop loop;

  if (callback_)
  {
    callback_(&loop);
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    // loop_指针指向了一个栈上的对象，threadFunc函数退出之后，这个指针就失效了
    // threadFunc函数退出，就意味着线程退出了，EventLoopThread对象也就没有存在的价值了。
    // 因而不会有什么大的问题
    loop_ = &loop;
    cond_.notify_one();
  }

  loop.loop();
}

