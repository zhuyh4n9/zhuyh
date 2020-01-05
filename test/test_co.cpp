#include "../zhuyh.hpp"

void test_co()
{
  LOG_ROOT_INFO() << "co begin";
  co_yield;
  LOG_ROOT_INFO() << "co middle";
  co_yield;
  LOG_ROOT_INFO() << "co end";
  co_yield;
}

int main()
{
  co test_co;
  co test_co;
  co test_co;
  co [](){
    LOG_ROOT_INFO() << "A";
    co_yield;
    LOG_ROOT_INFO() << "B";
    co_yield;
    LOG_ROOT_INFO() << "C";
    co_yield;
  };
  zhuyh::Scheduler::getThis() -> stop();
  return 0;
}
