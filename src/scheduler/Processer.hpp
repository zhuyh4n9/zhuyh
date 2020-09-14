#pragma once

#include <atomic>
#include "concurrent/fiber.hpp"
#include "concurrent/Thread.hpp"
#include "latch/lock.hpp"
#include "TSQueue.hpp"
#include "Task.hpp"
#include "TimerManager.hpp"
#include "util.hpp"

namespace zhuyh {
class Scheduler;

//must be a shared_ptr
class Processer : public std::enable_shared_from_this<Processer> {
public:
    friend class Scheduler;
    typedef std::function<void()> CbType;
    typedef std::shared_ptr<Processer> ptr;
    using Deque = NonbTSQueue<Fiber::ptr>;
    //构造函数提供最大空闲协程数和负载因子,maxIdle = 0 采用配置文件中个数
    Processer(const std::string name = "",Scheduler* schd = nullptr);
    ~Processer();
    uint64_t getBlockTime() const;
    //向该processer添加一个协程
    bool addFiber(Fiber::ptr fiber);
    bool addFiber(Fiber::ptr* fiber);
    //add a cb and make it a fiber
    bool addFiber(CbType cb);
    bool addFiber(CbType* cb);
    //size_t addFibers(std::vector<Fiber::ptr>&& fibers);

    void start(CbType cb = nullptr);
    void stop();
    //get the time(in ms) of running a fiber
    inline time_t getActiveMS() {
        if (m_idle) {
            return 0;
        }
        return getCurrentTimeMS() - m_lastIdleMS;
    }
    bool isIdle() const{ return m_idle;}
    inline void join() {
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
    std::list<Fiber::ptr> steal(int k);
    bool store(std::list<Fiber::ptr>& fibers);

    int notify();
    int waitForNotify();
private:
    //Fiber::ptr _idleFiber = nullptr;
    //Fiber::ptr _nxtTask = nullptr;
    std::string m_name;
    Thread::ptr m_thread;
    std::atomic<int> m_payLoad{0};
    Deque m_readyFibers;
    //bool _forceStop = false;
    //准备就绪,可以停止了
    std::atomic<bool> m_stop {true};
    //准备停止了
    std::atomic<bool> m_stopping {false};
    std::atomic<bool> m_idle {true};
    //record last moment we this processor had nothing to do
    std::atomic<time_t> m_lastIdleMS {0};
    //mutable Mutex m_mx;
    Scheduler* m_sched;
    Semaphore m_sem;
#ifdef ZHUYH_PROCESSOR_PROFILING
    int m_worked{0};
#endif
}; // end of class Processor
  
} // end of namespace zhuyh
