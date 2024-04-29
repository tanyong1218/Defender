#pragma once
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <functional>
using namespace std;

//TODO:��Ҫ�Ѳ���Ҳ��Ϊ�ɱ�ģ���������
//ʵ�ʴ����ʱ�򣬲���Ҫ�ɱ��������ֱ��ͨ��Lambada���ʽ���񼴿ɣ����������Args...û��ʵ������
template<typename... Args>
struct Task
{
	uint64_t id;						// ɾ������ʱ����Ҫ���ݴ�id�ҵ���ɾ������
	uint64_t period;					// ִ�е����ڣ��������ʱ��������������ִ��ʱ��
	bool repeated;						// �Ƿ�Ϊ�ظ�������
	std::function<void(Args...)> func;  // �������ʵ��
	bool removed;						// �����Ƿ��ѱ�ɾ��
	//Args && ...args;					// ����Ĳ���
	Task(uint64_t id, uint64_t period, bool repeated, std::function<void(Args...)> func)
		: id(id), period(period), repeated(repeated), func(func), removed(false)
	{
	}
};

//��ʱ���������ڶ�ʱ���ĳһ�����Ƿ���ڣ���ʱִ��ĳһ������
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
	Timer(const Timer&) = delete;			  // ���ÿ������캯��
	Timer& operator=(const Timer&) = delete;  // ���ÿ�����ֵ�����
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
