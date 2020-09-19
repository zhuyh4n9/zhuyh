#include "TimerManager.hpp"
#include "macro.hpp"
#include "logUtil.hpp"
#include <list>
#include <vector>
#include <algorithm>
namespace zhuyh
{
  //std::atomic<uint64_t> Timer::__id__{0};
static Logger::ptr s_syslog = GET_LOGGER("system");
Timer::Timer(time_t sec,time_t msec, time_t usec, time_t nsec) {
    //_id = ++__id__;
    m_cancelled = false;
    ASSERT(sec >= 0 && msec >= 0 && usec >= 0 && nsec >= 0);
    usec += nsec/1000;
    msec += usec/1000;
    m_interval = ((uint64_t)msec) + ((uint64_t)sec)*1000ull;
    //开机时间
    m_nxtExpireTime = m_interval + getCurrentTime();
    //LOG_ROOT_INFO() << "nxt : "<<_nxtExpireTime;
    m_type = SINGLE;
    m_manager = nullptr;
}

void Timer::start() {
    ASSERT(m_started == false);
    //必须设置完任务之后定时器才可以开始
    ASSERT(m_fiber != nullptr);
    m_started = true;
}

uint64_t Timer::getCurrentTime() {
    struct timespec timer;
    clock_gettime(CLOCK_MONOTONIC,&timer);
    return (((uint64_t)timer.tv_nsec)/1000000ull) + ((uint64_t)timer.tv_sec)*1000ull;
}
  
void Timer::setManager(TimerManager* manager) {
    m_manager = manager;
}

bool Timer::cancel() {
    //ASSERT(0); 
    LockGuard lg(m_manager->m_mx);
    if (m_cancelled) {
        return false;
    }
    if (m_fiber) {
        m_cancelled = true;
        auto self = shared_from_this();
        auto it = m_manager->m_timers.find(self);
        //LOG_ROOT_INFO() << (unsigned long long)self.get();
        //if(it != m_manager->m_timers.end())
        //coredump here
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

void Timer::setTask(CbType cb) {
    ASSERT(m_cancelled == false);
    ASSERT(m_started == false);
    if (cb == nullptr) {
	    m_fiber = Fiber::getThis();
    } else {
	    m_fiber.reset(new Fiber(cb));
    }
}

bool TimerManager::Comparator::operator() (const Timer::ptr& o1,const Timer::ptr& o2) const {
    if(!o1 && !o2) {
        return false;
    }
    if(!o1) {
        return true;
    }
    if (!o2) {
        return false;
    }
    if (o1->m_nxtExpireTime < o2->m_nxtExpireTime) {
        return true;
    }
    if (o1->m_nxtExpireTime > o2->m_nxtExpireTime) {
      return false;
    }
    return (o1.get() < o2.get());
}

std::list<Fiber::ptr> TimerManager::getExpiredTasks() {
    std::list<Fiber::ptr> res;
    std::list<Timer::ptr> tms;
    LockGuard lg(m_mx);
    //LOG_ROOT_INFO() << "total : "<<m_timers.size();
    if (m_timers.empty()) {
        return res;
    }
    auto t = Timer::ptr(new Timer());
    auto pos = m_timers.begin();
    while (pos != m_timers.end() && (*pos)->m_nxtExpireTime <= t->m_nxtExpireTime) {
	    pos++;
    }
    tms.insert(tms.begin(),m_timers.begin(),pos);
    m_timers.erase(m_timers.begin(),pos);
    for (auto& item : tms) {
        res.push_back(item->m_fiber);
        if (item->m_type == Timer::LOOP) {
            m_timers.insert(item);
        } else {
            item->m_fiber = nullptr;
        }
    }
    return res;
}

uint64_t TimerManager::getNextExpireTime() {
    LockGuard lg(m_mx);
    if (m_timers.empty()) {
        return (uint64_t)-1;
    }
    auto it =  m_timers.begin();
    return (*it)->m_nxtExpireTime;
}

uint64_t TimerManager::getNextExpireInterval() {
    auto cur = Timer::getCurrentTime();
    auto nxt = getNextExpireTime();
    if (nxt == (uint64_t)-1) {
        return (uint64_t)-1;
    }
    return nxt >= cur ? nxt - cur : 0;
}

int TimerManager::addTimer(Timer::ptr timer,
			                std::function<void()> cb,
			                Timer::TimerType type) {
    ASSERT(timer != nullptr);
    bool headTag = false;
    LockGuard lg(m_mx);
    //如果是循环定时器更改定时器类型
    if (type == Timer::TimerType::LOOP) {
        timer->setLoop();
    }
    //设置本Manager为定时器Manager
    timer->m_cancelled = false;
    timer->setManager(this);
    //设置任务
    timer->setTask(cb);
    m_timers.insert(timer);
    if (m_timers.find(timer) == m_timers.begin()) {
        headTag = true;
    }
    timer->start();
    lg.unlock();
    if (headTag) {
	    notify();
    }
    return 0;
}

int TimerManager::addTimer(Timer::ptr* timer,
			     std::function<void()> cb,
			     Timer::TimerType type) {
    ASSERT(timer != nullptr);
    Timer::ptr t = nullptr;
    t.swap(*timer);
    return addTimer(t,cb,type);
}

} // end of namespace zhuyh
