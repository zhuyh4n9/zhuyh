#pragma once

#include "scheduler/Scheduler.hpp"
#include "scheduler/Processer.hpp"
#include "scheduler/IOManager.hpp"
#include "concurrent/fiber.hpp"


namespace zhuyh
{
  namespace
  {
    struct _SchedulerInit
    {
      _SchedulerInit()
      {
	Fiber::getThis();
	//Thread::setName("Main");
	Scheduler* _scheduler = Scheduler::Schd::getInstance();
	_scheduler->start();
      }
    };
  }
  struct __co
  {
    typedef std::function<void()> CbType;
    __co() {}
    inline const __co& operator=(CbType cb)
    {
      static struct _SchedulerInit __scheduler_initer;
      Scheduler::Schd::getInstance()->addNewTask(Task::ptr(new Task(cb)));
      return *this;
    }
   
  };
  
}
