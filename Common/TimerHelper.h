#pragma once
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <functional>
using namespace std;

using TaskFunc = std::function<void()>;

struct Task
{
    uint64_t id;        // ɾ������ʱ����Ҫ���ݴ�id�ҵ���ɾ������
    uint64_t period;    // ִ�е����ڣ��������ʱ��������������ִ��ʱ��
    bool repeated;      // �Ƿ�Ϊ�ظ�������
    TaskFunc func;      // �������ʵ��
    bool removed;       // �����Ƿ��ѱ�ɾ��
    Task(uint64_t id, uint64_t period, bool repeated, TaskFunc func)
        : id(id), period(period), repeated(repeated), func(func), removed(false)
    {

    }
};
class Timer
{
private:
    // var
    std::thread m_worker;
    std::atomic<bool> m_stop;
    std::multimap<uint64_t, Task> m_tasks;
    std::mutex m_tasks_mutex;
    std::condition_variable m_condition;
    uint64_t m_cur_id;
    // func
    void run();
    uint64_t now();

public:
    // func
    Timer();
    ~Timer();
    uint64_t add(uint64_t period_ms, bool repeated, TaskFunc func);
    bool remove(uint64_t);
    void stop();
};