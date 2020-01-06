#pragma once

#include <unistd.h>
#include <sys/time.h>
#include <time.h>

namespace zhuyh
{
  //线程级别的Hook
  struct Hook
  {
    enum State
      {
	OFF = 0,
	ON = 1
      };
    static void setHookState(State state);
    static bool isHookEnable();
  };
  
}

extern "C"
{
  using sleep_func =  unsigned int(*)(unsigned int seconds);
  extern sleep_func sleep_f;

  using usleep_func =  int (*)(useconds_t usec);
  extern usleep_func usleep_f;

  using nanosleep_func = int (*)(const struct timespec *req, struct timespec *rem);
  extern nanosleep_func nanosleep_f;
  
  using socket_func =  int (*)(int domain,int type,int protocol);

  using fcntl_func = int (*)(int fd, int cmd, ... /* arg */ );
  extern fcntl_func fcntl_f;
}
