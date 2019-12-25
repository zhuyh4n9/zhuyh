#include <sstream>
#include "../zhuyh.hpp"
#include <vector>
#include <unistd.h>

void func()
{
  LOG_ROOT_INFO() << "Thread ID : "<<zhuyh::getThreadId()
		  << " Thread Name : "<<zhuyh::Thread::thisName();
					sleep(100);
}
int main()
{
  std::vector<zhuyh::Thread::ptr> vec;
  for(int i=0;i<5;i++)
    {
      std::stringstream ss;
      ss<<"thread_"<<i;
      zhuyh::Thread::ptr thread(new zhuyh::Thread(func,ss.str()));
      vec.push_back(thread);
    }
  for(int i=0;i<5;i++)
    {
      vec[i]->join();
    }
  return 0;
}
