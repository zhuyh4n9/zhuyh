#include <sys/epoll.h>
#include "Processer.hpp"
#include "Scheduler.hpp"
#include "logUtil.hpp"
#include "macro.hpp"
#include "netio/Hook.hpp"

namespace zhuyh {

static Logger::ptr g_syslog  = GET_LOGGER("system");
static thread_local Fiber::ptr g_main_fiber = nullptr;

Processer::Processer(const std::string name, Scheduler* sched)
    :m_name(name), 
     m_que(new WorkQueue<>()),
     m_lastIdleMS(0){
    //if not sched is setted, then we use default root sched
    if (sched == nullptr) {
        m_sched = Scheduler::Schd::getInstance();
    } else {
        m_sched = sched;
    }
}

Processer::~Processer() {
    if (!m_stopping) {
        stop();
    }
#ifdef ZHUYH_PROCESSOR_PROFILING
    LOG_INFO(g_syslog) << "Processer : "<<m_name<<"  Destroyed, worked = :"<<m_worked;
#endif
}

void Processer::start(CbType cb) {
    if (cb == nullptr) { 
        try{
            m_thread.reset(new Thread(std::bind(&Processer::run,shared_from_this()), m_name));
        } catch (std::exception& e) {
            LOG_ERROR(g_syslog) << e.what();
        }
    } else {
        // use main thread
	    Thread::setName(m_name);
	    addFiber(Fiber::ptr(new Fiber(cb)));
	    run();
    }
    m_stop = false;
	//LOG_INFO(g_syslog) << "Processer Started!";
}

bool Processer::addFiber(Fiber::ptr fiber) {
    //LOG_ROOT_ERROR() << "ADD NEW TASK1";
    ASSERT2(m_sched != nullptr, "no scheduler is setted");
    if (fiber == nullptr) {
        return false;
    }
    m_que->addFiber(fiber);
    ++m_payLoad;
    notify();
    //LOG_ROOT_ERROR() << "ADD NEW TASK2";
    return true;
}

//swap操作将会使得*t变为空
bool Processer::addFiber(Fiber::ptr* fiber) {
    if (fiber == nullptr) {
        return false;
    }
    Fiber::ptr f;
    f.swap(*fiber);
    return addFiber(f);
}

bool Processer::addFiber(CbType cb) {
    ASSERT2(m_sched != nullptr, "no scheduler is setted");
    if (nullptr == cb) {
        return false;
    }
    Fiber::ptr fiber(new Fiber(cb));
    //must be a new fiber
    ++(m_sched->m_totalFibers);
    m_que->addFiber(fiber);
    ++m_payLoad;
    notify();
    //LOG_ROOT_ERROR() << "ADD NEW TASK2";
    return true;
}

bool Processer::addFiber(CbType *cb) {
    if (nullptr == cb) {
        return false;
    }
    CbType c;
    c.swap(*cb);
    return addFiber(c);
}

std::list<Fiber::ptr> Processer::steal(int k) {
    std::list<Fiber::ptr> fibers;
    if (m_que->fetchFibers(k, fibers) != 0) {
        m_payLoad -= fibers.size();
    }
    return fibers;
}

//put fiber or callback to 
bool Processer::store(std::list<Fiber::ptr>& fibers) {
    /**
     * TODO : This is not used yet. But in following update, 
     *        the size of the queue would be a fixed size
     */
    std::list<Fiber::ptr> left;
    size_t nr = 0;

    nr = m_que->addFibers(fibers, left);
    m_payLoad += nr;
    return true;
}

void Processer::stop() {
    m_stopping = 1;
    notify();
}

int Processer::notify() {
    try {
        m_sem.notify();
    } catch (std::exception& e) {
        LOG_WARN(g_syslog) << "notify failed : "<< e.what();
        return -1;
    }
    return 0;
}

int Processer::waitForNotify() {
    try{
        m_sem.wait();
    } catch(std::exception& e) {
        LOG_WARN(g_syslog)<<"waitForNotify failed : "<<e.what();
        return -1;
    }
    return 0;
}

//TODO : looks not pretty, need modification
bool Processer::isStopping() const {
    ASSERT(getMainFiber() != nullptr);
    return (m_que->emptyStrong()  && m_stopping );
}

bool Processer::getStopping() const {
    return m_stopping;
}

void Processer::run() {
    Scheduler::setThis(m_sched);
    Hook::setHookState(true);
    setMainFiber(Fiber::getThis());
    //ASSERT(m_readyTasks.empty());
    while(1) {
        Fiber::ptr fiber;
        m_idle = true;
        //LOG_INFO(g_syslog) << " totalTask : "<<m_readyTasks.size();
	    while(m_que->fetchFiber(fiber) != WorkQueue<>::Priority::NONE) {
            m_idle = false;
            m_lastIdleMS = getCurrentTimeMS();
	        //LOG_ROOT_ERROR() << "Get New Task";
            ASSERT(fiber != nullptr);

#ifdef ZHUYH_PROCESSOR_PROFILING
            m_worked++;
#endif

            if (fiber) {
                if (fiber->getState() != Fiber::HOLD &&
                    fiber->getState() != Fiber::READY) {
                        // ASSERT2(task -> fiber->getState() == Fiber::EXEC,
                        // Fiber::getState(task->fiber->getState()));
                        // We 说明未完全切换HOLD态
                        if(fiber->getState() == Fiber::EXEC) {
                            m_que->addFiber(fiber);
                            continue;
                        }
                }
                fiber -> setState(Fiber::READY);
            } else {
                ASSERT2(false, "unexpect get a null fiber");
            }
            //task.reset();
            ASSERT(fiber != nullptr);
            ASSERT(fiber->_stack != nullptr);
            ASSERT2(fiber->getState() == Fiber::READY,Fiber::getState(fiber->getState()));
            fiber->swapIn();
            //LOG_ROOT_ERROR() << "Swapped Out";
            ASSERT(fiber->getState() != Fiber::INIT);
            if (fiber->getState() == Fiber::READY) {  // swap out by co_yield
                m_que->addFiber(fiber);
            } else if(fiber->getState() == Fiber::EXEC) { //swap out by using block method
                fiber->_state = Fiber::HOLD;
                --m_payLoad;
            } else if(fiber->getState() == Fiber::HOLD) {
                --m_payLoad;
            } else if(fiber->getState() == Fiber::TERM
                     || fiber->getState() == Fiber::EXCEPT) {
                --m_payLoad;
                --(m_sched->m_totalFibers);
                fiber.reset();
            } else {
                ASSERT2(false,Fiber::getState(fiber->getState()));
            }
	    }
        //NO Task Left, We're trying to steal some fiber
        if (m_sched->balance(shared_from_this()) > 0) {
            //LOG_INFO(g_syslog) << "STOLED";
            continue;
        }
        //Failed to steal fiber, check whether we're stopping
        if (m_stopping) {
            if (m_sched->m_totalFibers <= 0
                && m_que->emptyRelax()
                && m_sched->getHold() <= 0) {

                m_stop = true;
                //LOG_INFO(g_syslog) << "EXIT PROCESSOR1";
                return;
            }
        }
        //nothing to do, so we wait for event
        waitForNotify();
        if (m_stopping) {
            //TODO:改为调度器holdCount
            if (m_sched->m_totalFibers <= 0
                && m_que->emptyRelax()
                && m_sched->getHold() <= 0) {
                    m_stop = true;
                    //LOG_INFO(g_syslog) << "EXIT PROCESSOR";
                    --m_sched->m_currentThread;
                    return;
            }
        }
    }
}

Fiber::ptr Processer::getMainFiber() {
    return g_main_fiber;
}

void Processer::setMainFiber(Fiber::ptr fiber) {
    g_main_fiber = fiber;
}  

} // end of namespace zhuyh