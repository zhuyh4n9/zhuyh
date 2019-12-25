#include"../zhuyh.hpp"
#include<iostream>
using namespace std;

static auto sys_log = GET_LOGGER("system");
void test()
{
  LOG_ERROR(sys_log)<<"HELLO";
  LOG_ROOT_ERROR()<<"Hello World";
}
int main()
{
  zhuyh::Fiber::getThis();
//  DEL_FILE_APPENDER("root","root.log");
  // DEL_STDOUT_APPENDER("root");
  test();
  return 0;
}
