#include <vector>
#include "../zhuyh.hpp"
#include <fstream>
using namespace zhuyh;

std::ofstream fs;

Mutex lk;
void func()
{
  for(int i=0;i<100000;i++)
  {
    LockGuard lg(lk);
    fs << "system	[INFO]	4366	NONE<4366>	/home/zhuyh/mnt/Code/GraduationDesign/zhuyh/example/logtest.cpp:11(main)	2019-12-08 13:31:15	Performance Test123" << std::endl;
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
