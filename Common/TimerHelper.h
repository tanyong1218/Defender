#pragma once
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <functional>
using namespace std;

//TODO:需要把参数也作为可变模板包传进来
//实际处理的时候，不需要可变参数包，直接通过Lambada表达式捕获即可，所以这里的Args...没有实际作用
template<typename... Args>
struct Task
{
	uint64_t id;						// 删除任务时，需要根据此id找到被删的任务
	uint64_t period;					// 执行的周期，添加任务时用来计算该任务的执行时刻
	bool repeated;						// 是否为重复的任务
	std::function<void(Args...)> func;  // 任务具体实现
	bool removed;						// 任务是否已被删除
	//Args && ...args;					// 任务的参数
	Task(uint64_t id, uint64_t period, bool repeated, std::function<void(Args...)> func)
		: id(id), period(period), repeated(repeated), func(func), removed(false)
	{
	}
};

//定时器，可用于定时检测某一进程是否存在，定时执行某一函数等
template<typename... Args>
class Timer
{
private:
	std::thread m_worker;
	std::atomic<bool> m_stop;

	std::multimap<uint64_t, Task<Args ...>> m_tasks;
	std::mutex m_tasks_mutex;
	std::condition_variable m_condition;
	uint64_t m_cur_id;
	void run()
	{
		while (true) {
			std::unique_lock<std::mutex> lock(m_tasks_mutex);
			m_condition.wait(lock, [this]() -> bool { return !m_tasks.empty() || m_stop; });
			if (m_stop)
			{
				break;
			}
			else
			{
				uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				auto it = m_tasks.begin();
				if (it->first <= now)
				{
					Task<Args...> task = std::move(it->second);
					m_tasks.erase(it);
					lock.unlock();
					task.func(Args()...);
					if (task.repeated && !task.removed) {
						task.id = m_cur_id++;
						task.removed = false;
						m_tasks.emplace(now + task.period, std::move(task));
					}
				}
				else
				{
					m_condition.wait_for(lock, std::chrono::milliseconds(it->first - now));
				}
			}
		}
	}
public:
	Timer(const Timer&) = delete;			  // 禁用拷贝构造函数
	Timer& operator=(const Timer&) = delete;  // 禁用拷贝赋值运算符
	Timer() : m_stop(false), m_cur_id(0)
	{
		m_worker = std::thread(&Timer<Args ...>::run, this);
	}

	~Timer()
	{
		m_stop.store(true);
		m_condition.notify_all();
		m_worker.join();
	}

	uint64_t add(uint64_t period_ms, bool repeated, std::function<void(Args...)> func)
	{
		uint64_t when = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()
		).count() + period_ms;
		Task<Args...> task(m_cur_id, period_ms, repeated, func);
		{
			std::lock_guard<std::mutex> lock(m_tasks_mutex);
			m_tasks.emplace(when, std::move(task));
		}
		m_condition.notify_all();
		return m_cur_id++;
	}

	bool remove(uint64_t id)
	{
		bool flag = false;
		std::lock_guard<std::mutex> lock(m_tasks_mutex);
		auto it = std::find_if(m_tasks.begin(), m_tasks.end(), [id](const std::pair<uint64_t, Task<Args ...>>& item) -> bool { return item.second.id == id; });
		if (it != m_tasks.end())
		{
			it->second.removed = true;
			flag = true;
		}
		return flag;
	}

	void stop()
	{
		m_stop.store(true);
		m_condition.notify_all();
	}
};
