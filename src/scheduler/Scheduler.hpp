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
#include "TimerManager.hpp"
#include "Singleton.hpp"
#include "LogThread.hpp"
#include "WorkQueue.hpp"

namespace zhuyh {

class Reactor;
class Processer;
class Timer;
class Scheduler final {
public:
    friend class Singleton<Scheduler>;
    friend class Reactor;
    friend class Processer;
    friend class CoSemaphore;
    typedef std::shared_ptr<Scheduler> ptr;
    typedef std::function<void()> CbType;
    typedef Singleton<Scheduler> Schd;
    //用户创建调度器
    Scheduler(const std::string& name, int threads);
    ~Scheduler();
    void stop();
    //设置回调则复用主线程
    void start(CbType cb = nullptr);
    static Scheduler* getThis();
    static void setThis(Scheduler* schd);
    //供外界添加新任务
    void addNewFiber(CbType cb);
    void addNewFiber(Fiber::ptr fiber);
    void addNewFiber(Fiber::ptr *fiber);
    int addReadEvent(int fd, std::function<void()> cb = nullptr);
    int addWriteEvent(int fd, std::function<void()> cb = nullptr);
    void activateFiber(Fiber::ptr fiber) {
        addNewFiber(fiber);
        delHold();
    }
    void activateFiber(Fiber::ptr *fiber) {
        addNewFiber(fiber);
        delHold();
    }
    //static Scheduler* getThis();
    int getHold();
    int addTimer(std::shared_ptr<Timer> timer, std::function<void()> cb = nullptr,
                 Timer::TimerType type = Timer::TimerType::SINGLE);
    int cancelReadEvent(int fd);
    int cancelWriteEvent(int fd);
    int cancelAllEvent(int fd);

private:
    int balance(std::shared_ptr<Processer> prc);
    //需要线程安全,供Reactor使用
    void addFiber(Fiber::ptr fiber);
    void addFiber(Fiber::ptr *fiber);
    void addFiber(CbType cb);
    void addHold();
    void delHold();
    void dispatcher();
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
    std::atomic<int> m_totalFibers{0};
    std::shared_ptr<Reactor> m_reactor = nullptr;
    std::vector<std::shared_ptr<Processer>> m_pcsQue;
    std::atomic<bool> m_stopping {false};
    std::atomic<bool> m_stop {true};
    std::string m_name = "Scheduler";

    LogThread::ptr m_lgThread;
    Thread::ptr m_dispatcher;
};

}
