#pragma once
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
	ThreadPool(size_t numThreads) : m_stop(false)
	{
		for (size_t i = 0; i < numThreads; ++i)
		{
			workers.emplace_back([this] {
				while (true) {
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(queueMutex);
						condition.wait(lock, [this] { return m_stop || !tasks.empty(); });
						if (m_stop)
						{
							return;
						}
						task = std::move(tasks.front());
						tasks.pop();
					}
					task();
				}
				});
		}
	}

	//入队列
	template <class F, class... Args>
	void enqueue(F&& f, Args&&... args)
	{
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			tasks.emplace([=] { f(args...); });
		}
		condition.notify_one();
	}

	~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			m_stop = true;
		}
		//通知所有的condition继续执行
		condition.notify_all();
		for (std::thread& worker : workers)
		{
			worker.join();
		}
	}

private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;
	std::mutex queueMutex;
	std::condition_variable condition;
	std::atomic<bool> m_stop;
};
