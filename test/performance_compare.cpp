#include <vector>
#include "../zhuyh.hpp"
#include <fstream>
using namespace zhuyh;

std::ofstream fs;

SpinLock lk;
void func()
{
  for(int i=0;i<100000;i++)
  {
    LOG_ROOT_INFO() << "Performance Test";
  }
}

int main()
{
  std::vector<Thread::ptr> vec;
  fs.open("LogRoot.txt");
  for(int i=0;i<5;i++)
  {
    std::stringstream ss;
    ss<<"thread_"<<i;
    Thread::ptr thread(new Thread(func,ss.str()));
    vec.push_back(thread);
  }
  for(int i=0;i<5;i++)
  {
    vec[i]->join();
  }
  return 0;
}
