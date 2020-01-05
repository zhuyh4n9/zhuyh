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
  typedef unsigned int(*sleep_func)(unsigned int seconds);
  extern sleep_func sleep_f;

  typedef int (*usleep_func)(useconds_t usec);
  extern usleep_func usleep_f;

  // typedef int (*nanosleep_func)(const struct timespec *req, struct timespec *rem);
  // nanosleep_func nanosleep_f;
}
