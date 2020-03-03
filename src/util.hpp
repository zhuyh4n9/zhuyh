#pragma once

#include <vector>
#include <string>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <execinfo.h>
#include <cxxabi.h>
#include "log.hpp"
#include <fcntl.h>

namespace zhuyh
{
  pid_t getThreadId();
  uint32_t getCoroutineId();
  pid_t getProcessId();
  const time_t& start();
  time_t getCurrentTime();
  time_t getElapseTime();
  //打印函数调用栈
  void BackTrace(std::vector<std::string>& bt,int size,int skip);
  std::string Bt2Str(int size,int skip,const std::string& prefix);
  std::string getEnv(const std::string& envName);
}
