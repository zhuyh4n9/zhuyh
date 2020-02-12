#include "coLock.hpp"

namespace zhuyh
{

  bool CoSemaphore::tryWait()
  {
    ASSERT(Scheduler::Schd::getInstance() != nullptr);
    {
      LockGuard lg(_mx);
      if(_value > 0)
	{
	  --_value;
	  return true;
	}
      return false;
    }
  }

  void CoSemaphore::wait()
  {
    auto scheduler = Scheduler::Schd::getInstance();
    ASSERT(scheduler != nullptr);
    {
      LockGuard lg(_mx);
      if(_value > 0)
	{
	  --_value;
	  return;
	}
      _holdQue.push_back(Fiber::getThis());
      scheduler->addHold();
      //--(scheduler->totalTask);
    }
    Fiber::YieldToHold();
  }

  void CoSemaphore::notify()
  {
    auto scheduler = Scheduler::Schd::getInstance();
    ASSERT(scheduler);
    LockGuard lg(_mx);
    if(!_holdQue.empty())
      {
	Fiber::ptr fiber = _holdQue.front();
	_holdQue.pop_front();
	scheduler->addTask(fiber);
	scheduler->delHold();
      }
    else
      {
	++_value;
      }
  }
  
}
