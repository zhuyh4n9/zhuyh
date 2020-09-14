#include <list>
#include <algorithm>
#include "config.hpp"
#include "Reactor.hpp"
#include "Scheduler.hpp"
#include "netio/Hook.hpp"

namespace zhuyh {
static Logger::ptr s_syslog = GET_LOGGER("system");

static int setNonb(int fd) {
    int flags = fcntl_f(fd,F_GETFL);
    if(flags < 0 )
        return -1;
    return fcntl_f(fd,F_SETFL,flags | O_NONBLOCK);
}
  
static int clearNonb(int fd) {
    int flags = fcntl_f(fd,F_GETFL);
    if(flags < 0 )
        return -1;
    return fcntl_f(fd,F_SETFL,flags & ~O_NONBLOCK);
}

Reactor::Reactor(const std::string& name,Scheduler* schd) {
    int rt = 0;

    m_epfd = epoll_create(1);
    if (m_epfd < 0 ) {
	    throw std::logic_error("epoll_create error");
    }
    if (schd == nullptr) {
        m_sched = Scheduler::Schd::getInstance();
    } else {
        m_sched = schd;
    } if (name == "") {
        m_name = "Reactor";
    } else {
        m_name = name;
    }

    rt = pipe(m_notifyFd);
    ASSERT2(rt >= 0,strerror(errno));
    setNonb(m_notifyFd[0]);
    setNonb(m_notifyFd[1]);
    struct epoll_event ev;
    m_notifyEvent = new FdEvent();
    m_notifyEvent->fd = m_notifyFd[0];
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = m_notifyEvent;
    rt = epoll_ctl(m_epfd,EPOLL_CTL_ADD, m_notifyFd[0],&ev);
    resizeMap((uint32_t)4096);
    ASSERT(rt >= 0);
    m_thread.reset(new Thread(std::bind(&Reactor::run,this), m_name));
    //LOG_INFO(s_syslog) << "Reactor : "<< m_name <<" Created!";
}

Reactor::~Reactor() {
    if (!m_stopping)
        stop();
    //LOG_INFO(s_syslog) << "Reactor Destroyed";
    if (m_epfd >= 0) {
        close(m_epfd);
        m_epfd = -1;
    }
    if (m_notifyFd[0] != -1) {
	    close(m_notifyFd[0]);
	    m_notifyFd[0]=-1;
    }
    if (m_notifyFd[1] != -1) {
        close(m_notifyFd[1]);
        m_notifyFd[1] = -1;
    }
    m_thread->join();
    for(auto  p : m_eventMap) {
        if(p){
            delete p;
            p = nullptr;
        }
    }
    delete m_notifyEvent;
    m_eventMap.clear();
}

void Reactor::notify() {
    int rt = write_f(m_notifyFd[1],"",1);
    if(rt < 0) {
	    LOG_ROOT_ERROR() << "rt : "<< rt << " error : "<<strerror(errno) << " errno : "<< errno;
    }
}

void Reactor::resizeMap(uint32_t size) {
    m_eventMap.resize(size);
    for(size_t i=0;i<m_eventMap.size();i++) {
	    if(m_eventMap[i] == nullptr) {
	        m_eventMap[i] = new FdEvent(i,NONE);
	    }
    }
}

int Reactor::addEvent(int fd, Fiber::ptr fiber, EventType type) {
    ASSERT( type == READ  || type == WRITE);
    RDLockGuard lg(m_lk);
    struct epoll_event ev;
    //TODO : 改为vector
    FdEvent* epEv = nullptr;
    ASSERT2(fd >= 0,"fd cannot be smaller than 0");
    //LOG_INFO(s_syslog) << "fd = "<<fd;
    if((size_t)fd < m_eventMap.size()) {
        epEv = m_eventMap[fd];
        lg.unlock();
    } else {
	    lg.unlock();
	    WRLockGuard lg3(m_lk);
	    resizeMap(m_eventMap.size()*2);
	    epEv = m_eventMap[fd];
    }
    //LOG_ROOT_ERROR() << "adding Event,  fd : "<<fd<< " type : "
    //                 <<(type == READ ? "READ" : "WRITE");
    LockGuard lg2(epEv->lk);
    int rt = setNonb(fd);
    if(rt < 0) {
	    //ASSERT(0);
	    LOG_ERROR(s_syslog) << "Failed to setNonb";
	    return -1;
    }
    auto op = epEv->event == NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    if(type & epEv->event) {
	    LOG_ERROR(s_syslog) << "type : " << type << " Exist"
		    	            << "current : " << epEv->event
			                <<" fd = " <<fd;
	
	    return -1;
    }
    ev.events = epEv->event | EPOLLET | type;
    ev.data.ptr = epEv;
    rt = 0;
    rt = epoll_ctl(m_epfd,op,fd,&ev);
    if(rt) {
        LOG_ERROR(s_syslog) << "epoll_ctl errro : " << strerror(errno);
        //ASSERT2(false,strerror(errno));
        return -1;
    }
    if(type & EventType::READ) {
        ASSERT(epEv->rdtask == nullptr);
        epEv->rdtask = fiber;
        epEv->event = (EventType)(epEv->event | EventType::READ);
    } else if(type & EventType::WRITE) {
        ASSERT(epEv->wrtask == nullptr);
        epEv->wrtask = fiber;
        epEv->event = (EventType)(epEv->event | EventType::WRITE);
    }
    ++m_holdCount;
    return 0;
}

int Reactor::delEvent(int fd,EventType type) {
    ASSERT( !((type & READ) && (type & WRITE)));
    ASSERT( type != NONE);
    ASSERT(type == READ || type == WRITE);
    struct epoll_event ev;
    RDLockGuard lg(m_lk);
    if((size_t)fd >= m_eventMap.size()) {
        LOG_ERROR(s_syslog) << "fd<"<<fd<<"> is bigger than m_eventMap.size()<"<<m_eventMap.size()<<">";
        return -1;
    }
    FdEvent* epEv = m_eventMap[fd];
    lg.unlock();
    
    LockGuard lg2(epEv->lk);
    //LOG_ROOT_ERROR() << "deleting Event,  fd : "<<fd<< " type : "
    //		     <<(type == READ ? "READ" : "WRITE");
    if( (EventType)(type & epEv->event) == NONE ) {
	    LOG_ERROR(s_syslog) << "type : " << type << " Not Exist";
	    return -1;
    }
    auto tevent = ~type & epEv->event;
    auto op =  tevent ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    ev.events = tevent | EPOLLET;
    ev.data.ptr = epEv;
    int rt = epoll_ctl(m_epfd,op,fd,&ev);
    if(rt) {
        LOG_ERROR(s_syslog) << "epoll_ctl error";
        return -1;
    }
    epEv->event = (EventType)tevent;
    if(type & READ) {
	    epEv->rdtask.reset();
    } else if(type & WRITE) {
	    epEv->wrtask.reset();
    }
    --m_holdCount;
    return 0;
}

int Reactor::cancelEvent(int fd,EventType type) {
    ASSERT( !((type & READ) && (type & WRITE))  );
    ASSERT( type != NONE);
    ASSERT(type == READ || type == WRITE);
    struct epoll_event ev;
    RDLockGuard lg(m_lk);
    if((size_t)fd >= m_eventMap.size()) {
        LOG_WARN(s_syslog) << "fd<"<<fd<<"> is bigger than m_eventMap.size()<"<<m_eventMap.size()<<">";
        return -1;
    }
    FdEvent* epEv = m_eventMap[fd];
    lg.unlock();
    //LOG_ROOT_ERROR() << "cancel Event,  fd : "<<fd<< " type : "
    //               <<(type == READ ? "READ" : "WRITE");
    LockGuard lg2(epEv->lk);
    if ((EventType)(epEv->event & type) == NONE ) {
	    //LOG_WARN(s_syslog) << "type : " << type << " Not Exist";
	    return -1;    	
    }
    auto tevent = epEv->event & ~type;
    auto op = tevent ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    ev.events = EPOLLET | tevent;
    ev.data.ptr = epEv;
    int rt = epoll_ctl(m_epfd,op,fd,&ev);
    if (rt) {
        LOG_ERROR(s_syslog) << "epoll_ctl error";
        return -2;
    }
    triggerEvent(epEv,type);
    --m_holdCount;
    return 0;
}

int Reactor::cancelAll(int fd) {
    RDLockGuard lg(m_lk);
    if((size_t)fd >= m_eventMap.size()) {
        LOG_WARN(s_syslog) << "fd<"<<fd<<"> is bigger than m_eventMap.size()<"<<m_eventMap.size()<<">";
        return -1;
    }
    FdEvent* epEv = m_eventMap[fd];
    lg.unlock();
    //LOG_ROOT_ERROR() << "deleting Event,  fd : "<<fd<< " type : ALL";
    LockGuard lg2(epEv->lk);
    if(epEv->event == NONE) return false;
    int rt = epoll_ctl(m_epfd,EPOLL_CTL_DEL,fd,nullptr);
    if(rt) {
	    LOG_ERROR(s_syslog) << "epoll_ctl error";
	    return -1;
    }
    
    if (epEv->event & READ) {
        triggerEvent(epEv,READ);
        --m_holdCount;
    }
    if(epEv->event & WRITE) {
        triggerEvent(epEv,WRITE);
	    --m_holdCount;
    }
    return 0;
}

bool Reactor::isStopping() const {
    return m_holdCount == 0 && m_stopping && m_sched->m_totalFibers == 0;
}
  
void Reactor::stop() {
    m_stopping = true;
    notify();
}

void Reactor::run() {
    Scheduler::setThis(m_sched);
    Hook::setHookState(true);
    Fiber::getThis();
    std::shared_ptr<char> idleBuff(new char[1024],[](char* ptr) { delete [] ptr;});
    const int MaxEvent = 1000;
    struct epoll_event* events = new epoll_event[1001];
    //毫秒
    const int MaxTimeOut = 1000;
    while(1) {
	//LOG_WARN(s_syslog) << "Holding : " << m_holdCount << " Total  : " << m_sched->m_totalFibers;
        if(isStopping()) {
            LOG_INFO(s_syslog) << "Reactor : " << m_name << " stopped!";
            delete [] events;
            break;
        }
        int rt = 0;
        int nxtTimeOut = (int)std::min(getNextExpireInterval(),(uint64_t)MaxTimeOut);
	    do{
            rt = epoll_wait(m_epfd,events,MaxEvent,nxtTimeOut);
            if(rt<0 && errno == EINTR){
                LOG_INFO(s_syslog) <<"Reactor Receive EINTR";
            } else {
                break;
            }
	    } while(1);
        //dealing with timers
        std::list<Fiber::ptr> fibers = getExpiredTasks();
        for(Fiber::ptr& fiber : fibers) {
            m_sched->addFiber(fiber);
        }
        // dealing with event
        for(int i=0;i<rt;i++) {
            struct epoll_event& ev = events[i];
            FdEvent* epEv = (FdEvent*)ev.data.ptr;
            if(epEv->fd == m_notifyFd[0]) {
                int rt = 0;
                do {
                    rt = read_f(m_notifyFd[0],idleBuff.get(),1023);
                    if(rt < 0) {
                        if(errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        LOG_ERROR(s_syslog) << "read notifyFd failed,rt : "<<rt
                                << " error : "<<strerror(errno)
                                << " errno : "<<errno;
                        break;
                    }
                } while(1);
            }
            int real_event = NONE;
            LockGuard lg(epEv->lk);
            if(ev.events & (EPOLLERR | EPOLLHUP)) {
                ev.events |= (EPOLLIN | EPOLLOUT) & (epEv->event);
            }
            if(ev.events & READ ) {
                real_event |= READ;
            }
            if(ev.events & WRITE) {
                real_event |= WRITE;
            }
            if((real_event & epEv->event) == NONE) 
                continue;

            EventType tevent = (EventType)(~real_event & epEv->event);
            
            int op = (tevent == NONE) ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
            ev.events = tevent | EPOLLET;
            int rt2 = epoll_ctl(m_epfd,op,epEv->fd,&ev);
            if(rt2) {
                LOG_ERROR(s_syslog) << "epoll_ctl error";
                continue;
            }
            if(real_event & READ) {
                triggerEvent(epEv,READ);
                --m_holdCount;
            }
            if(real_event & WRITE) {
                triggerEvent(epEv,WRITE);
                --m_holdCount;
            }
	    }
    }
}

int Reactor::triggerEvent(FdEvent* epEv,EventType type) {
    //ASSERT2(0, "Trigger event");
    ASSERT( type == READ  || type == WRITE);
    ASSERT(type & epEv->event);
    epEv->event =(EventType)(epEv-> event & ~type);
    if(type & READ) {
        //LOG_ROOT_INFO() << "Triggle READ Event : "<<epEv->fd;
        ASSERT(epEv->rdtask != nullptr);
        Fiber::ptr fiber = nullptr;
        fiber.swap(epEv->rdtask);
        m_sched->addFiber(fiber);
    }
    else if(type & WRITE) {
        //LOG_ROOT_INFO() << "Triggle WRITE Event : " << epEv->fd;
        Fiber::ptr fiber = nullptr;
        fiber.swap(epEv->wrtask);
        //LOG_ROOT_INFO() << "added task"<<(unsigned long long)task.get();
        m_sched->addFiber(fiber);
    }
    return 0;
}

} // end of namespace zhuyh
