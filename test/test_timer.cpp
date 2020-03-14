#include "../zhuyh.hpp"
using namespace zhuyh;

void Alarm()
{
  LOG_ROOT_INFO() << "Time up";
}

int main()
{
  auto scheduler = Scheduler::Schd::getInstance();
  scheduler->addTimer(Timer::ptr(new Timer(2)),Alarm,Timer::LOOP);
  
}
