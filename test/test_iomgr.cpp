#include "../zhuyh.hpp"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
using namespace zhuyh;
struct TEST
{
  int fd[2];
  void funcRead()
  {
    LOG_ROOT_INFO() << "READ EVENT TRIGGERED!";
    char c;
    int rt = read(fd[0],&c,1);
    ASSERT2(rt >= 0,std::to_string(fd[0]));
    //close(fd[0]);
    //close(fd[1]);
  }
  
  void funcWrite()
  {
    LOG_ROOT_INFO() << "WRITE EVENT TRIGGERED!";
    int rt = write(fd[1],"a",1);
    //std::cout<<"rt="<<rt<<" fd[1]="<<fd[1]<<std::endl;
    ASSERT2(rt >= 0,std::to_string(fd[1]));
  }
  ~TEST()
  {
    std::cout<<"Destroyed\n";
  }
};

void Alarm()
{
  LOG_ROOT_INFO() <<"Time up";
}
void test_co()
{
  //LOG_ROOT_INFO() << "Enter test";
  //LOG_ROOT_INFO() << "test mid";
  for(int i=0;i<200000;i++)
    co_yield;
  LOG_ROOT_INFO() << "DONE";
}

void test_thread()
{
  std::vector<zhuyh::Thread::ptr> vec(1000);
  for(int i =0 ;i<1000;i++)
    {
      vec[i] = zhuyh::Thread::ptr(new zhuyh::Thread(test_co,"test_"+std::to_string(i)));
    }
  for(int i=0;i<1000;i++) vec[i]->join();
}
int main()
{
  auto scheduler = Scheduler::getThis();
  TEST* test = new TEST[200];
  for(int i =0 ;i<200;++i)
    {
      int rt=pipe(test[i].fd);
      
      ASSERT2(rt >= 0,strerror(errno));
      scheduler->addReadEvent(test[i].fd[0],
			      Task::ptr(new Task(std::bind(&TEST::funcRead,&test[i]))) );
      scheduler->addWriteEvent(test[i].fd[1],
			       Task::ptr(new Task(std::bind(&TEST::funcWrite,&test[i]))) );
      if(scheduler->addTimer(Timer::ptr(new Timer(2)),Alarm) < 0)
	{
	  LOG_ROOT_ERROR() << "Failed";
	}
      
      co test_co;
      co [](){
	//for(int i=0;i<1000;i++) co_yield;
	Scheduler* scheduler = Scheduler::getThis();
	scheduler->addTimer(Timer::ptr(new Timer(1)),Fiber::getThis());
	LOG_ROOT_INFO() << "back to coroutine";
      };
    }
  //Scheduler::getThis()->stop();
  return 0;
}

