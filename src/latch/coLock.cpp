#include "coLock.hpp"

namespace zhuyh {

bool CoSemaphore::tryWait() {
    ASSERT(Scheduler::Schd::getInstance() != nullptr);
    {
        LockGuard lg(m_mx);
        if(_value > 0) {
            --_value;
            return true;
        }
        return false;
    }
}

void CoSemaphore::wait() {
    auto scheduler = Scheduler::Schd::getInstance();
    ASSERT(scheduler != nullptr);
    {
      LockGuard lg(m_mx);
        if(_value > 0) {
	        --_value;
	        return;
	    }
        _holdQue.push_back(Fiber::getThis());
        //告诉调度器不能结束
        scheduler->addHold();
        //--(scheduler->totalTask);
    }
 
    Fiber::YieldToHold();
}

void CoSemaphore::notify() {
    auto scheduler = Scheduler::Schd::getInstance();
    ASSERT(scheduler);
    LockGuard lg(m_mx);
    if(!_holdQue.empty()) {
        Fiber::ptr fiber = _holdQue.front();
        _holdQue.pop_front();
        scheduler->activateFiber(fiber);
    } else {
	    ++_value;
    }
}
  
} // end of namespace zhuyh
