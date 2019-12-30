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
    Scheduler* scheduler = Scheduler::getThis();
    scheduler->addTimer(Timer::ptr(new Timer(0,800,20,1)));
    LOG_ROOT_INFO() <<"Ding!";
    close(fd[0]);
    close(fd[1]);
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
  Scheduler* scheduler = Scheduler::getThis();
  LOG_ROOT_INFO() <<"Time up";
  scheduler->addTimer(Timer::ptr(new Timer(0,500,20,1)));
  LOG_ROOT_INFO() <<"Time up";
}
int a = 0;
zhuyh::CoSemaphore sem(1);
void test_co()
{
  //LOG_ROOT_INFO() << "Enter test";
  //LOG_ROOT_INFO() << "test mid";
  
  for(int i=0;i<10;++i)
    {
      sem.wait();
      a++;
      LOG_ROOT_INFO() << "a = "<< a;
      sem.notify();
      co_yield;
    }
  Scheduler* scheduler = Scheduler::getThis();
  scheduler->addTimer(Timer::ptr(new Timer(0,500,20,1)));
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
  TEST* test = new TEST[2000];
  LOG_ROOT_INFO() << "Entering";
  //Scheduler* scheduler = Scheduler::getThis();
  //scheduler->start();
  for(int i =0 ;i<200;++i)
    {
      co test_co;
      int rt=pipe(test[i].fd);
      
      ASSERT2(rt >= 0,strerror(errno));
      scheduler->addReadEvent(test[i].fd[0],
       			      Task::ptr(new Task(std::bind(&TEST::funcRead,&test[i]))) );
      scheduler->addWriteEvent(test[i].fd[1],
       			       Task::ptr(new Task(std::bind(&TEST::funcWrite,&test[i]))) );
      rt = 0;
      if((rt=scheduler->addTimer(Timer::ptr(new Timer(1)),Alarm) < 0))
	{
       	  LOG_ROOT_ERROR() << "Failed : rt = "<<rt;
      	}
      
      co test_co;
      //co test_co;
      co [](){
       
	for(int i=0;i<1000;i++) co_yield;
	Scheduler* scheduler = Scheduler::getThis();
      	scheduler->addTimer(Timer::ptr(new Timer(0,0,0,1)));
      	LOG_ROOT_INFO() << "back to coroutine 1";
      	scheduler->addTimer(Timer::ptr(new Timer(0,999,999,999)));
      	LOG_ROOT_INFO() << "back to coroutine 2";
      	scheduler->addTimer(Timer::ptr(new Timer(0,500,20,1)));
      	LOG_ROOT_INFO() << "back to coroutine 3";
      	scheduler->addTimer(Timer::ptr(new Timer(1,700,0,1)));
      	for(int i=0;i<1000;i++) co_yield;
      	LOG_ROOT_INFO() << "back to coroutine 4";
      };
      //LOG_ROOT_ERROR() <<" ADD TIMER : "<<i;
    }
  LOG_ROOT_INFO() << "EXIT";
  Scheduler::getThis()->stop();
  return 0;
}
