#pragma once

#include <set>
#include <vector>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <functional>
#include <list>

#include "latch/lock.hpp"
#include "concurrent/fiber.hpp"
/*
 *时间均采用开机时间,不需要担心时间被推迟
 */
namespace zhuyh{

class TimerManager;
class Timer : public std::enable_shared_from_this<Timer> {
public:
    typedef std::function<void()> CbType;
    typedef std::shared_ptr<Timer> ptr;
    friend class TimerManager;
    enum TimerType {
        SINGLE, //单次计时
        LOOP //循环计时
    };
    Timer(time_t sec = 0, time_t msec = 0, time_t usec = 0, time_t nsec = 0);

    Timer(uint64_t* msec) {
        m_cancelled = false;
        m_nxtExpireTime = *msec;
    }

    void setManager(TimerManager* manager);
    ~Timer() {
      // LOG_ROOT_INFO() << "timer destroyed _tfd = "<<_tfd;
    }
    static uint64_t getCurrentTime();
    TimerType getTimerType() const {
        return m_type;
    }
    void start();
    void setLoop() {
        if(m_started) return;
            m_type = LOOP;
    }
    bool cancel();
    bool isCancelled() const {
        return m_cancelled;
    }
    //nullptr表示使用当前协程,否则表示回调函数
    void setTask(CbType cb = nullptr);
    Fiber::ptr getTask() {
        return m_fiber;
    }
private:
    void setNextExpireTime() {
        if(m_type == SINGLE)
            return;
        m_nxtExpireTime += m_interval;
    }
private:
    Fiber::ptr m_fiber = nullptr;
    bool m_started = false;
    bool m_cancelled = false;
    TimerType m_type = SINGLE;
    uint64_t m_interval = 0;
    uint64_t m_nxtExpireTime = 0;
    TimerManager* m_manager = nullptr;
};

class TimerManager  {
public:
    typedef std::shared_ptr<TimerManager> ptr;
    friend class Timer;
    virtual ~TimerManager() {}
    int addTimer(Timer::ptr* timer,
                std::function<void()> cb = nullptr,
                Timer::TimerType type = Timer::SINGLE);
    int addTimer(Timer::ptr timer,
                std::function<void()> cb = nullptr,
                Timer::TimerType type = Timer::SINGLE);
    //获取下一次超时时间
    uint64_t getNextExpireTime();
    //获取距离下一次超时的间隔时间
    uint64_t getNextExpireInterval();
    virtual void notify() = 0;
    //列出所有超时的Timer
    std::list<Fiber::ptr> getExpiredTasks();
public:
    class Comparator{
    public:
        bool operator() (const Timer::ptr& o1,const Timer::ptr& o2) const;
    };
private:
    std::set<Timer::ptr,Comparator> m_timers;
    mutable Mutex m_mx;
};
  
}
