/*
 *协程锁以及协程信号量
 */
#pragma once

#include "lock.hpp"
#include "../scheduler/Scheduler.hpp"
#include "../concurrent/fiber.hpp"
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
  
}
