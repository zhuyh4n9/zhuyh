#pragma once

#include "TSQueue.hpp"
#include "Processer.hpp"
#include "IOManager.hpp"
#include "../latch/lock.hpp"
#include "../concurrent/Thread.hpp"
#include <memory>
#include <functional>
#include "../logUtil.hpp"
#include "../log.hpp"

namespace zhuyh
{
  //协程调度器本质是一个线程池
  // Scheduler -1---N-> Processer(Thread) -1----M-> Fiber
  // 最后再次对Fiber进行封装,变为Coroutine,创建后即加入调度器
  // Fiber::YieldToReady() --宏定义--> co_yield
  // Fiber::YieldToHold() --宏定义---> co_yield_to_hold
  class IOManager;
  class Processer;
  struct Task;
  class Scheduler : public std::enable_shared_from_this<Scheduler>
  {
  public:
    friend class IOManager;
    typedef std::shared_ptr<Scheduler> ptr;
    typedef std::function<void()> CbType;
    //用户创建调度器
    Scheduler(const std::string& name,int minThread,int maxThread,int limitPayLoad = 20);
    ~Scheduler();
    void stop();
    void start();
    //添加计时器
    //void addTimer();
    /*
     *负载均衡时机:某一个线程无任务时,去向另外一个线程偷取任务
     */
    //供外界添加新任务
    void addNewTask(std::shared_ptr<Task> task);
    //添加一个读事件
    int addReadEvent(int fd,std::shared_ptr<Task> task);
    //添加一个写事件
    int addWriteEvent(int fd,std::shared_ptr<Task> task);
    //负载均衡,返回正在偷取任务数
    int balance(std::shared_ptr<Processer> prc);
    //获取根管理器,需要在进入main函数前进行一次初始化
    static Scheduler* getThis();
    int getHold();
  private:
    //需要线程安全,供IOManager使用
    void addTask(std::shared_ptr<Task> task);
    void addTask(CbType cb);
    void addTask(Fiber::ptr fiber);
    //按照配置文件创建调度器
    Scheduler();
    //偷任务时使用
    std::shared_ptr<Processer> getMaxPayLoad();
    //放任务时使用
    std::shared_ptr<Processer> getMinPayLoad();
    int _holdFiberCount = 0;
    //当前线程数
    int _currentThread = 0;
    int _activeThread = 0;
    //线程数上下限制
    int _minThread = 4;
    int _maxThread = 20;
    //执行器平均任务大于次值则开启新线程
    int _limitPayLoad = 20;
    //static Mutex _addTaskmx;
    //static SpinLock _getThismx;
    //用于协程信号量
    TSQueue<Fiber::ptr> _holdQueue;
    std::shared_ptr<IOManager> _ioMgr = nullptr;
    std::vector<std::shared_ptr<Processer>> _pcsQue;
    std::atomic<bool> _stopping {false};
    std::atomic<bool> _stop {true};
    std::string _name = "Scheduler";
  };
  
}
