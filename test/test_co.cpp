#include "../zhuyh.hpp"

void test_co()
{
  LOG_ROOT_INFO() << "co begin";
  co_yield;
  LOG_ROOT_INFO() << "co middle";
  co_yield;
  LOG_ROOT_INFO() << "co end";
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
  };
  return 0;
}
