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
    close(fd[0]);
    close(fd[1]);
  }
  
  void funcWrite()
  {
    LOG_ROOT_INFO() << "WRITE EVENT TRIGGERED!";
    int rt = write(fd[1],"a",1);
    ASSERT(rt >= 0);
  }
};


void test_co()
{
  //LOG_ROOT_INFO() << "Enter test";
  
  //LOG_ROOT_INFO() << "test mid";
  for(int i=0;i<1000;i++);
      co_yield;
  //LOG_ROOT_INFO() << "END";
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
  //  test_co();
  /*
    int fd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = inet_addr("183.232.231.174");
  */
  //auto scheduler = Scheduler::getThis();
  //TEST test[400];
  for(int i =0 ;i<1000;i++)
    {
      /*
      int rt=pipe(test[i].fd);
      ASSERT2(rt >= 0,strerror(errno));
      scheduler->addReadEvent(test[i].fd[0],Task::ptr(new Task(std::bind(&TEST::funcRead,&test[i]))) );
      scheduler->addWriteEvent(test[i].fd[1],Task::ptr(new Task(std::bind(&TEST::funcWrite,&test[i]))) );
      */
      co(test_co);
      //
    }

  //connect(fd,(struct sockaddr*)&addr,sizeof(addr));
  //Scheduler::getThis()->stop();
  //LOG_ROOT_DEBUG() <<"MAIN EXIT";
  return 0;
}

