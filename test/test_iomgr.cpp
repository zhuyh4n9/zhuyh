#include "../zhuyh.hpp"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "../src/netio/Hook.hpp"

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
    usleep(800*1000);
    LOG_ROOT_INFO() <<"Ding!";
    close(fd[0]);
    close(fd[1]);
  }
  
  void funcWrite()
  {
    LOG_ROOT_INFO() << "WRITE EVENT TRIGGERED!";
    int rt = write(fd[1],"a",1);
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
  sleep(1);
  LOG_ROOT_INFO() <<"Time up";
}
int a = 0;
zhuyh::CoSemaphore sem(1);
void test_co()
{ 
  for(int i=0;i<10;++i)
    {
      sem.wait();
      LOG_ROOT_INFO() << "a = "<<++a;
      sem.notify();
      co_yield;
    }
  sleep(3);
  LOG_ROOT_INFO() << "DONE";
  co Alarm;
}

int main()
{
  LOG_ROOT_INFO() << "Entering";
  Scheduler* scheduler = Scheduler::Schd::getInstance();
  scheduler->start();
  for(int i =0 ;i<400;++i)
    {
      TEST* test = new TEST();
      co test_co;
      int rt=pipe(test->fd);
      
      ASSERT2(rt >= 0,strerror(errno));
      scheduler->addReadEvent(test->fd[0],
       			      std::bind(&TEST::funcRead,test) );
      scheduler->addWriteEvent(test->fd[1],
			       std::bind(&TEST::funcWrite,test) );
      rt = 0;
      if((rt=scheduler->addTimer(Timer::ptr(new Timer(5)),Alarm) < 0))
      	{
       	  LOG_ROOT_ERROR() << "Failed : rt = "<<rt;
      	}
      co [scheduler](){
	
      	for(int i=0;i<1000;i++) co_yield;
      	usleep(700*1000);
      	for(int i=0;i<1000;i++) co_yield;
      	LOG_ROOT_INFO() << "back to coroutine";
      };
      //LOG_ROOT_ERROR() <<" ADD TIMER : "<<i;
    }
  LOG_ROOT_INFO() << "HOOK STATE : "<<zhuyh::Hook::isHookEnable();
  sleep(10);
  Scheduler::Schd::getInstance()->stop();
  return 0;
}
