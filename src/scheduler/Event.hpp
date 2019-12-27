/*
        IEvent
          |
	_____
 TimerEvent  FdEvent
 */
#pragma once

namespace zhuyh
{
  class Scheduler;
  class IEvent
  {
  public:
    virtual void TriggleEvent(EventType Type) = 0;
    void setScheduler(Scheduler* scheduler) const final
    {
      _scheduler = scheduler;
    }
    const Scheduler* getScheduler() const final
    {
      return _scheduler;
    }
    bool isTimer() const final
    {
      return _isTimerFlag;
    }
    virtual ~IEvent() {}
    IEvent(bool isTimerFlag = false,Scheduler* scheduler = nullptr)
      :_isTimerFlag(isTimerFlag)
    {
      if(scheduler == nullptr)
	_scheduler =  Scheduler::getThis();
      else
	_scheduler = scheduler;
    }
    Mutex lk;
  protected:
    Scheduler* _scheduler;
    bool _isTimerFlag = false;
  };
}
