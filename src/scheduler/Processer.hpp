#pragma once

#include <atomic>
#include "../concurrent/fiber.hpp"
#include "../concurrent/Thread.hpp"
#include "../latch/lock.hpp"
#include "TSQueue.hpp"
#include "Task.hpp"
#include "TimerManager.hpp"

namespace zhuyh {
class Scheduler;

//must be a shared_ptr
class Processer : public std::enable_shared_from_this<Processer> {
public:
    friend class Scheduler;
    typedef std::function<void()> CbType;
    typedef std::shared_ptr<Processer> ptr;
    using Deque = NonbTSQueue<Task::ptr>;
    //构造函数提供最大空闲协程数和负载因子,maxIdle = 0 采用配置文件中个数
    Processer(const std::string name = "",Scheduler* schd = nullptr);
    ~Processer();

    //向该processer添加一个任务
    bool addTask(Task::ptr task);
    bool addTask(Task::ptr* task);
    void start(CbType cb = nullptr);
    void stop();
    void join() {
        m_thread->join();
    }
    inline bool isStopped() const {
        return m_stop;
    }
    inline bool isStopping() const;
    //获取_stopping值
    inline bool getStopping() const;
    void run();
    //获取/设置主协程
    static Fiber::ptr getMainFiber();
    static void setMainFiber(Fiber::ptr);
private:
    std::list<Task::ptr> steal(int k);
    bool store(std::list<Task::ptr>& tasks);

    int notify();
    int waitForNotify();
private:
    //Fiber::ptr _idleFiber = nullptr;
    //Fiber::ptr _nxtTask = nullptr;
    std::string m_name;
    Thread::ptr m_thread;
    std::atomic<int> m_payLoad{0};
    Deque m_readyTasks;
    //bool _forceStop = false;
    //准备就绪,可以停止了
    std::atomic<bool> m_stop {true};
    //准备停止了
    std::atomic<bool> m_stopping {false};
    //mutable Mutex m_mx;
    Scheduler* m_sched;
    Semaphore m_sem;
#ifdef ZHUYH_PROCESSOR_PROFILING
    int m_worked{0};
#endif
}; // end of class Processor
  
} // end of namespace zhuyh
