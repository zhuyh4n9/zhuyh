#include "all.hpp"

using namespace zhuyh;

void func()
{
  LOG_ROOT_INFO() << "enter func";
  co_yield;
  LOG_ROOT_INFO() << "func mid";
  co_yield;
  LOG_ROOT_INFO() << "exit func";
}

int main()
{
  //Thread::setName("Main");
  //Fiber::getThis();
  //Processer::ptr p(new Processer("processer"));
  //p->addTask(Task::ptr(new Task(func)));
  //p->addTask(Task::ptr(new Task(func)));
  co(func);
  co(func);
  //sleep(1);
  //p->stop();
  // p->join();
  return 0;
}
