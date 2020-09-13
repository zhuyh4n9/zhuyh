#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <functional>
#include <vector>
#include "concurrent/fiber.hpp"
#include "TSQueue.hpp"
#include "latch/lock.hpp"
#include "concurrent/Thread.hpp"
#include "Task.hpp"
#include "TimerManager.hpp"
#include "Singleton.hpp"
#include "LogThread.hpp"

namespace zhuyh {
  //协程调度器本质是一个线程池
  // Scheduler -1---N-> Processer(Thread) -1----M-> Fiber
  // 最后再次对Fiber进行封装,变为Coroutine,创建后即加入调度器
  // Fiber::YieldToReady() --宏定义--> co_yield
  // Fiber::YieldToHold() --宏定义---> co_yield_to_hold
class Reactor;
class Processer;
class Timer;
class Scheduler final : public std::enable_shared_from_this<Scheduler>{
public:
    friend class Singleton<Scheduler>;
    friend class Reactor;
    friend class Processer;
    friend class CoSemaphore;
    typedef std::shared_ptr<Scheduler> ptr;
    typedef std::function<void()> CbType;
    typedef Singleton<Scheduler> Schd;
    //用户创建调度器
    Scheduler(const std::string& name,int threads);
    ~Scheduler();
    void stop();
    //设置回调则复用主线程
    void start(CbType cb = nullptr);
    static Scheduler* getThis();
    static void setThis(Scheduler* schd);
    //添加计时器
    //void addTimer();
    /*
     *负载均衡时机:某一个线程无任务时,去向另外一个线程偷取任务
     */
    //供外界添加新任务
    void addNewTask(CbType cb);
    void addNewTask(std::shared_ptr<Task> task);
    void addNewTask(Fiber::ptr fiber);
    int addReadEvent(int fd,std::function<void()> cb = nullptr);
    int addWriteEvent(int fd,std::function<void()> cb = nullptr);
    //static Scheduler* getThis();
    int getHold();
    int addTimer(std::shared_ptr<Timer> timer,std::function<void()> cb = nullptr,
		        Timer::TimerType type = Timer::TimerType::SINGLE);
    int cancelReadEvent(int fd);
    int cancelWriteEvent(int fd);
    int cancelAllEvent(int fd);
    
private:
    int balance(std::shared_ptr<Processer> prc);
    //需要线程安全,供Reactor使用
    void addTask(std::shared_ptr<Task> task);
    void addTask(std::shared_ptr<Task>* task);
    void addTask(CbType cb);
    void addHold();
    void delHold();
    void dispatcher();
public:
    void addTask(Fiber::ptr fiber);
private:
    //按照配置文件创建调度器
    Scheduler();
    //偷任务时使用
    std::shared_ptr<Processer> getMaxPayLoad();
    //放任务时使用
    std::shared_ptr<Processer> getMinPayLoad();
    //当前线程数
    std::atomic<int> m_currentThread{0};
    //线程数上下限制
    int m_minThread = 4;
    int m_maxThread = 20;
    std::atomic<int> m_totalTask{0};
    std::shared_ptr<Reactor> m_reactor = nullptr;
    std::vector<std::shared_ptr<Processer>> m_pcsQue;
    std::atomic<bool> m_stopping {false};
    std::atomic<bool> m_stop {true};
    std::string m_name = "Scheduler";

    LogThread::ptr m_lgThread;
    Thread::ptr m_dispatcher;
};

}
