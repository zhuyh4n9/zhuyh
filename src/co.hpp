#pragma once

#include "scheduler/Scheduler.hpp"
#include "scheduler/Processer.hpp"
#include "scheduler/IOManager.hpp"
#include "concurrent/fiber.hpp"


namespace zhuyh
{
  struct __co
  {
    typedef std::function<void()> CbType;
    __co() {}
    inline __co& operator-(CbType cb)
    {
      Scheduler::getThis()->addNewTask(Task::ptr(new Task(cb)));
      return *this;
    }
   
  };
  
}
