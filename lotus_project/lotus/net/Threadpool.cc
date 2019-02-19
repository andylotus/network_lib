#include "Threadpool.h"  
#include <iostream>
#include <cassert> 
#include <exception> 

using namespace lotus;
using namespace lotus::net;

ThreadPool::ThreadPool(const std::string &name) :
	name_(name),
	maxQueueSize_(0),
	running_(false)
{

}

ThreadPool::~ThreadPool()
{
	if (running_)
	{
		stop();
	}
}

void ThreadPool::start(int numThreads)
{
	assert(threads_.empty());
	running_ = true;
	threads_.reserve(numThreads);

	for (int i = 0; i < numThreads; ++i)
	{
		threads_.push_back(std::make_shared<std::thread>(std::bind(&ThreadPool::runInThread, this))); //during the start, the runInThread has been executed! Because there is a Constructor for std::thread!!
	}
}

void ThreadPool::stop()
{
	{
		std::unique_lock<std::mutex>  lock(mutex_);
		running_ = false;
		notEmpty_.notify_all(); 
		//why not need _notFull.notify_all() here?? notEmpty_ means there are some tasks in the queue to be consumed!
	}

	for(auto &t : threads_)
	{
		t->join();
	}
}

void ThreadPool::run(const Task &f) //same as put() function in the P-C boundedBlockingQueue
{
	if (threads_.empty()) //Never happen, as long as threadsNum >= 1
	{
		f(); //if threads vector is empty, then just execute f() without threads! 
	}
	else
	{
		std::unique_lock<std::mutex> lock(mutex_);

		std::thread::id this_id = std::this_thread::get_id();
		std::cout << "thread " << this_id << " in run()" << std::endl;

		while (isFull())
		{
			notFull_.wait(lock); // this thread waits on the condition variable --- that the queue is _notFull / until the queue is _notFull
		}

		assert(!isFull());
		queue_.push_back(f); //no certain order between f and cout --- no Constructor involved! f may be executed Later!!
		//std::cout << "after push_back, still in run() " << std::endl;
		std::cout << "now queue_size is " << queue_.size() << std::endl;
		notEmpty_.notify_one();
	}
}

ThreadPool::Task ThreadPool::take()
{
	std::unique_lock<std::mutex> lock(mutex_);

	while (queue_.empty() && running_)
	{
		notEmpty_.wait(lock); //this thread waits on the condition variable --- that the queue is _notEmpty
	}

	Task task;
	if (!queue_.empty())
	{
		task = queue_.front();
		queue_.pop_front();

		if (maxQueueSize_ > 0)
		{
			notFull_.notify_one();
		}
	}
	return task;
}

bool ThreadPool::isFull()
{
	return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}


void ThreadPool::runInThread()
{
	//std::cout << "now get into ThreadPool::runInThread " << std::endl;
	try
	{
		while (running_)
		{
			//std::cout << "before task in runInThread " << std::endl;
			Task task = take();
			//std::cout << "after task in runInThread " << std::endl;
			if (task)
			{
				task();
			}
		}
	}
	catch (const std::exception& ex)
	{
		fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
		abort();
	}
	catch (...)
	{
		fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
		throw;
	}
}
