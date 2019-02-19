/*
* 设计思想
* 让线程从阻塞队列中取出任务
* 当队列为空时，线程阻塞，直到往队列中添加了新任务
* 当队列为满时，线程阻塞，直到从队列中取走了任务
*/

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>  
#include <mutex>  
#include <functional>  
#include <string>  
#include <condition_variable>  
#include <deque>  
#include <vector>  
#include <memory>  
#include "noncopyable.h"  

namespace lotus
{
namespace net
{
	class ThreadPool :public noncopyable
	{
	public:
		typedef std::function<void()> Task;
		
		explicit ThreadPool(const std::string &name = std::string("ThreadPool"));
		~ThreadPool();

		//MUST be called before start()
		void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }//设置任务队列可存放最大任务数  

		void start(int numThreads);//设置线程数，创建numThreads个线程  
		void stop();//线程池结束  
		void run(const Task& f);//任务f在线程池中运行  --- addTask to the queue by main()!! not by the threadpool in reality!!


	private:
		bool isFull();//任务队列是否已满  
		void runInThread();//线程池中每个thread执行的function  
		Task take();//从任务队列中取出一个任务  --- getTask from the queue to be executed by threads vector.

		std::mutex mutex_;
		std::condition_variable notEmpty_; //both the _notEmpty and _notFull condition variables are the queue --- the Buffer!!
		std::condition_variable notFull_;
		std::string name_;
		std::vector<std::shared_ptr<std::thread> > threads_;
		std::deque<Task> queue_;
		size_t maxQueueSize_;
		bool running_;
	};
}
}
#endif // THREADPOOL_H 
