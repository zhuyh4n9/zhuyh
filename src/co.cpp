#include "co.hpp"

namespace zhuyh
{
  struct _SchedulerInit
  {
    _SchedulerInit()
    {
      Fiber::getThis();
      Thread::setName("Main");
      Scheduler* _scheduler = Scheduler::getThis();
      _scheduler->start();
    }
  };
  static struct _SchedulerInit __scheduler_initer;
  
  void Co(Fiber::CbType cb)
  {
    Scheduler* _scheduler = Scheduler::getThis();
    _scheduler -> addNewTask(Task::ptr(new Task(cb)));
  }

}
