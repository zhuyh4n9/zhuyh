#pragma once

#include <atomic>
#include "../concurrent/fiber.hpp"
#include "../concurrent/Thread.hpp"
#include "../latch/lock.hpp"
#include "TSQueue.hpp"
#include "Task.hpp"
#include "TimerManager.hpp"

namespace zhuyh
{
  class Scheduler;
  //必须在堆上分配
  class Processer : public std::enable_shared_from_this<Processer>
  {
  public:
    friend class Scheduler;
    typedef std::function<void()> CbType;
    typedef std::shared_ptr<Processer> ptr;
    //构造函数提供最大空闲协程数和负载因子,maxIdle = 0 采用配置文件中个数
    Processer(const std::string name = "",Scheduler* schd = nullptr);
    ~Processer();
    //TODO:需要线程安全
    using Deque = NonbTSQueue<Task::ptr>;
    //向该processer添加一个任务
    bool addTask(Task::ptr task);
    bool addTask(Task::ptr* task);
    void start(CbType cb = nullptr);
    void stop();
    void join()
    {
      _thread->join();
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
    std::string _name;
    Thread::ptr _thread;
    std::atomic<int> _payLoad{0};
    Deque _readyTask;
    //bool _forceStop = false;
    //准备就绪,可以停止了
    std::atomic<bool> _stop { false};
    //准备停止了
    std::atomic<bool> _stopping {false};
    mutable Mutex mx;
    Scheduler* _scheduler;
    Semaphore m_sem;
    //DEBUG
    int worked{0};
  };
  
}
