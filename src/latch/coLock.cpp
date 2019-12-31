#include "coLock.hpp"

namespace zhuyh
{
  //TODO:暂时不确定
  bool CoSemaphore::tryWait()
  {
    ASSERT(Scheduler::getThis() != nullptr);
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
    auto scheduler = Scheduler::getThis();
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
    Fiber::YieldToSwitch();
  }

  void CoSemaphore::notify()
  {
    auto scheduler = Scheduler::getThis();
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
