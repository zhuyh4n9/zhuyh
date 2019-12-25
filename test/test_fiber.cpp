#include "../zhuyh.hpp"
#include <sstream>
#include <vector>

using namespace zhuyh;
void run()
{
  LOG_ROOT_INFO() << "run begin"<< Fiber::totalFibers()<<" not term:"
		  <<Fiber::getTotalActive();
  Fiber::YieldToHold();
  LOG_ROOT_INFO() << "run mid "<< Fiber::totalFibers()<<" not term:"
		  <<Fiber::getTotalActive();
  Fiber::YieldToHold();
  LOG_ROOT_INFO() << "run end " << Fiber::totalFibers()<<" not term:"
		  <<Fiber::getTotalActive();
}
void func()
{
  Fiber::getThis();
  Fiber::ptr fiber(new Fiber(run));
  Fiber::ptr fiber2(new Fiber(run));
  while(Fiber::getLocalActive() != 1)
    {
      fiber->swapIn();
      fiber2->swapIn();
      LOG_ROOT_INFO() << " Back Main "<<" Total : "<<Fiber::totalFibers()<<" not term:"
		  <<Fiber::getTotalActive();
    }
  LOG_ROOT_INFO() << "Main End "<<" Total : "<<Fiber::totalFibers()<<" not term:"
		  <<Fiber::getTotalActive();
}

int main()
{
  std::vector<Thread::ptr> vec;
  for(int i=0;i<3;i++)
    {
      std::stringstream ss;
      ss<<"thread_"<<i;
      Thread::ptr thread(new Thread(func,ss.str()));
      vec.push_back(thread);
    }
  for(int i=0;i<3;i++)
    {
      vec[i]->join();
    }
  return 0;
}
