#include <bits/stdc++.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>        /* Definition of uint64_t */
class Test final
{
public:
  const char* getA() const
  {
    return a;
  }
private:
  char* a = nullptr;
};
int main()
{
  int fd = timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK);
  if(fd < 0)
    {
      std::cout<< "time_create : "<< strerror(errno);
      exit(1);
    }
  struct timespec timer;
  struct itimerspec new_timer;
  clock_gettime(CLOCK_MONOTONIC,&timer);
  std::cout<<timer.tv_sec<< " " <<timer.tv_nsec<<std::endl;
  bzero(&new_timer,sizeof(new_timer));
  new_timer.it_value.tv_sec = timer.tv_sec - 11 ;
  new_timer.it_value.tv_nsec = 0;
  if(timerfd_settime(fd,TFD_TIMER_ABSTIME,&new_timer,nullptr) < 0)
    {
      std::cout << "timerfd_create : " << strerror(errno) << std::endl;
    }
  fd_set rset,nset;
  FD_ZERO(&rset);
  FD_SET(fd,&rset);
  nset = rset;
  char c;
  while(1)
    {
      select(fd+1,&rset,nullptr,nullptr,nullptr);
      std::cout<<"TRIGGERED"<<std::endl;
      std::cout<<read(fd,&c,0)<<std::endl;
      rset = nset;
      break;
    }
  return 0;
}
