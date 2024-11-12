#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <condition_variable>
#include <iostream>

class ThreadPool
{
public:
	std::vector<std::thread> Threads;
	std::queue<std::function<void()>> Jobs;
	std::mutex QueueMutex;
	std::condition_variable MutexCondition;
	bool ShouldTerminate = false;

	void Start()
	{
		const int ThreadsNum = std::thread::hardware_concurrency();
		for (int i = 0; i < ThreadsNum; i++)
		{
			Threads.emplace_back(std::thread(&ThreadPool::ThreadLoop, this));
		}
	}

	void ThreadLoop()
	{
		while (true)
		{
			std::function<void()> Job;
			{
				std::unique_lock<std::mutex> lock(QueueMutex);
				MutexCondition.wait(lock, [this] {
					return !Jobs.empty() || ShouldTerminate;
				});
				if (ShouldTerminate)
					return;
				Job = Jobs.front();
				Jobs.pop();
			}
			std::cout << "Found job!\n";
			Job();
		}
	}

	std::function<void()> PopJob()
	{
		{
			std::unique_lock<std::mutex> lock(QueueMutex);

			std::function<void()> Job = Jobs.front();
			Jobs.pop();
			return Job;
		}
	}

	void QueueJob(const std::function<void()>& Job)
	{
		{
			std::unique_lock<std::mutex> lock(QueueMutex);
			Jobs.push(Job);
		}
		MutexCondition.notify_one();
	}

	bool Busy()
	{
		bool PoolBusy;
		{
			std::unique_lock<std::mutex> lock(QueueMutex);
			PoolBusy = !Jobs.empty();
		}
		return PoolBusy;
	}

	void Stop()
	{
		{
			std::unique_lock<std::mutex> lock(QueueMutex);
			ShouldTerminate = true;
		}
		MutexCondition.notify_all();
		JoinAllThreads();
	}

	void JoinAllThreads()
	{
		for (auto& Thread : Threads)
		{
			Thread.join();
		}
		Threads.clear();
	}

	~ThreadPool()
	{
		Stop();
	}
};
