#pragma once

#include "Scheduler.hpp"
#include "../concurrent/fiber.hpp"
#include "../concurrent/Thread.hpp"
#include "../util.hpp"
#include "../macro.hpp"
#include "../logUtil.hpp"
#include "../latch/lock.hpp"
#include "../config.hpp"
#include "TSQueue.hpp"
namespace zhuyh
{
  struct Task
  {
    typedef std::shared_ptr<Task> ptr;
    Fiber::ptr fiber = nullptr;
    Fiber::CbType cb = nullptr;
    bool stealable = true;
    bool finTag = false;
    Task(Fiber::ptr f)
      :fiber(f)
    {}
    Task(Fiber::ptr* f)
    {
      fiber.swap(*f);
    }
    Task(Fiber::CbType c)
      :cb(c)
    {}
    Task(Fiber::CbType* c)
    {
      cb.swap(*c);
    }
    Task() {}
  };
  class Scheduler;
  //必须在堆上分配
  class Processer : public std::enable_shared_from_this<Processer>
  {
  public:
    friend class Scheduler;
    typedef std::shared_ptr<Processer> ptr;
    //构造函数提供最大空闲协程数和负载因子,maxIdle = 0 采用配置文件中个数
    Processer(const std::string name = "",Scheduler* scheduler = nullptr);
    ~Processer();
    //TODO:需要线程安全
    using Deque = TSQueue<Fiber::ptr>;
    //向该processer添加一个任务
    bool addTask(Task::ptr task);
    bool addTask(Task::ptr* task);
    void start();
    void stop();
    void join()
    {
      _thread->join();
    }
    //是否正在退出
    inline bool isStopping() const;
    //获取_stopping值
    inline bool getStopping() const;
    void run();
    //获取/设置主协程
    static Fiber::ptr getMainFiber();
    static void setMainFiber(Fiber::ptr);
  private:
    //偷取k个协程
    std::list<Fiber::ptr> steal(int k);
    //放入k个协程
    bool store(std::list<Fiber::ptr>& tasks);
    //将线程从epoll_wait中唤醒
    int notify();
  protected:
    void null() {} 
    virtual void idle();
  private:
    //Fiber::ptr _idleFiber = nullptr;
    //Fiber::ptr _nxtTask = nullptr;
    std::string _name;
    Thread::ptr _thread;
    //线程阻塞在epoll,而不是消息队列
    int _epfd;
    int _notifyFd[2];
    std::atomic<int> _payLoad{0};
    Deque _readyTask;
    //bool _forceStop = false;
    //准备就绪,可以停止了
    std::atomic<bool> _stop { false};
    //准备停止了
    std::atomic<bool> _stopping {false};
    mutable Mutex mx;
    Scheduler* _scheduler;
  };
  
}
