#include "util.hpp"
#include "logUtil.hpp"
#include "concurrent/fiber.hpp"
#include "macro.hpp"
#include "netio/Hook.hpp"

namespace zhuyh
{
  static auto sys_log = GET_LOGGER("system");
  pid_t getThreadId()
  {
    return syscall(SYS_gettid);
  }
  
  uint32_t getCoroutineId()
  {
    return Fiber::getFid();
  }
  
  pid_t getProcessId()
  {
    return getpid();
  }
  
  const time_t& start()
  {
    struct timeval tm;
    gettimeofday(&tm,NULL);
    static time_t s = tm.tv_sec;
    return s;
  }
  
  time_t getCurrentTime()
  {
    struct timeval tm;
    gettimeofday(&tm,NULL);
    return tm.tv_sec;
  }

  time_t getElapseTime()
  {
    struct timeval tm;
    gettimeofday(&tm,NULL);
    return tm.tv_sec - start();
  }

  static std::string Demangle(const char* str)
  {
    size_t size = 0;
    int status = 0;
    std::string rt;
    rt.resize(256);
    // %[..]表示读取一个字符集合,%[^...]表示读取不包含该字符集的字符
    if(sscanf(str,"%*[^(]%*[(]%255[^)+]",&rt[0]) == 1)
      {
	char* s = abi::__cxa_demangle(&rt[0],nullptr,&size,&status);
	if(status == 0)
	  {
	    std::string res(s);
	    free(s);
	    return res;
	  }
      }
    if(sscanf(str,"%255s",&rt[0]) == 1)
      {
	return rt;
      }
    return str;
  }

  void BackTrace(std::vector<std::string>& bt,int size,int skip)
  {
    void** addr = (void**) malloc(sizeof(void*)*size);
    size_t s = backtrace(addr,size);
    char** strings = backtrace_symbols(addr,s);
    if(strings == NULL )
      {
	LOG_ERROR(sys_log) << "backtrace_symbols error";
	return;
      }
    for(size_t i=skip;i<s;i++)
      {
	bt.push_back(Demangle(strings[i]));
      }
    free(strings);
    free(addr);
  }

  std::string Bt2Str(int size,int skip,const std::string& prefix)
  {
    std::vector<std::string> bt;
    BackTrace(bt,size,skip);
    std::stringstream ss;
    for(auto& v:bt)
      {
	ss<<prefix<<v<<std::endl;
      }
    return ss.str();
  }
  
}
