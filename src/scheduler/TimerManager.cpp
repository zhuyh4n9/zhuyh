#include "TimerManager.hpp"

namespace zhuyh
{
  Timer Timmer::getCurrentTimer()
  {
    struct timespec _timer;
    clock_gettime(CLOCK
  }
  void TimerManager::addTimer(Timer&& timer)
  {
    LockGuard lg(_mx);
    _timerSet.insert(timer);
  }

  void TimerManager::addTimer(Timer timer)
  {
    LockGuard lg(_mx);
    _timerSet.insert(timer);
  }
  
  void TimerManager::removeTimer(const Timer& timer)
  {
    LockGuard lg(_mx);
    _timerSet.erase(timer);
  }

  std::list<Timer> listExpiredTimer()
  {
    
    return std::list<Timer>();
  }
  
}
