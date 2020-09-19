#pragma once

#include <sys/epoll.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include <unordered_map>
#include <memory>
#include <set>
#include <list>
#include <string.h>
#include "concurrent/Thread.hpp"
#include "concurrent/fiber.hpp"
#include "latch/lock.hpp"
#include <sys/epoll.h>

#include "logUtil.hpp"
#include "macro.hpp"
#include "TimerManager.hpp"

namespace zhuyh {

class Scheduler;
class Reactor final : public TimerManager {
public:
    friend class Scheduler;
    enum EventType {
        NONE = 0x0,
        READ = EPOLLIN,
        WRITE = EPOLLOUT
    };
    struct FdEvent final {
        typedef std::shared_ptr<FdEvent> ptr;
        FdEvent(int _fd,EventType _event){
            fd = _fd;
            event = _event;
        }
        FdEvent() {}
    public:
        Mutex lk;
        int fd;
        EventType event = NONE;
        Fiber::ptr rdtask = nullptr;
        Fiber::ptr wrtask = nullptr;
    };
    typedef std::shared_ptr<Reactor> ptr;
    Reactor(const std::string& name = "", Scheduler* schd = nullptr);
    ~Reactor();
    int addEvent(int fd,Fiber::ptr fiber, EventType type);
    int delEvent(int fd, EventType type);
    //取消应该事件,事件存在则触发事件
    int cancelEvent(int fd,EventType type);
    //取消fd的所有事件并且触发
    int cancelAll(int fd);
    void stop();
    void join() {
      m_thread->join();
    }
    void notify() override;
    bool isStopping() const;
    void run();
    void clearAllEvent();
    bool getStopping() const {
      return m_stopping;
    }
    //设置/获取调度器
    //Scheduler* getScheduler();
    //void setScheduler(Scheduler* scheduler);
    //触发一个事件
    int triggerEvent(FdEvent* epEv, EventType type);
private:
    void resizeMap(uint32_t size);
private:
    //epoll句柄
    int m_epfd = -1;
    Scheduler* m_sched;
    // fd --> event
    std::vector<FdEvent*> m_eventMap;
    mutable RWLock m_lk;
    int m_notifyFd[2];
    Thread::ptr m_thread = nullptr;
    std::string m_name;
    std::atomic<bool> m_stopping{false};
    std::atomic<int> m_holdCount{0};
    FdEvent* m_notifyEvent;
};

}
