/*
 *协程锁以及协程信号量
 */
#pragma once

#include "lock.hpp"
#include "../scheduler/Scheduler.hpp"
#include "../concurrent/fiber.hpp"
#include "../macro.hpp"
#include "../logUtil.hpp"

namespace zhuyh
{
  class CoSemaphore
  {
  public:
    CoSemaphore(int value)
      :_value{value}
    {
    }
    ~CoSemaphore()
    {
      ASSERT2(_holdQue.empty(),std::to_string(_holdQue.size()));
    }
    bool tryWait();
    void wait();
    void notify();
  private:
    Mutex _mx;
    std::deque<Fiber::ptr> _holdQue;
    int _value;
  };

  class CoMutex : public ILock
  {
  public:
    CoMutex()
      :m_sm(1){}

    void lock() override
    {
      m_sm.wait();
    }
    void unlock()
    {
      m_sm.notify();
    }
  private:
    CoSemaphore m_sm;
  };
  
}
