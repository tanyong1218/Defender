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
    uint64_t id;        // 删除任务时，需要根据此id找到被删的任务
    uint64_t period;    // 执行的周期，添加任务时用来计算该任务的执行时刻
    bool repeated;      // 是否为重复的任务
    TaskFunc func;      // 任务具体实现
    bool removed;       // 任务是否已被删除
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