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
    std::shared_ptr<char> data(new char[256],[](char* ptr){ delete [] ptr;});
    auto buff = data.get();
    //std::string rt;
    //rt.resize(256);
    // %[..]表示读取一个字符集合,%[^...]表示读取不包含该字符集的字符
    if(sscanf(str,"%*[^(]%*[(]%255[^)+]",buff) == 1)
      {
	char* s = abi::__cxa_demangle(buff,nullptr,&size,&status);
	if(status == 0)
	  {
	    std::string res(s);
	    free(s);
	    return res;
	  }
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
  std::string getEnv(const std::string& envName)
  {
    char* str = getenv(envName.c_str());
    if(str == nullptr) return "";
    std::string env(str);
    return env;
  }

  time_t str2Time(const char* str,const char* fmt)
  {
    if(str == nullptr || fmt == nullptr) return (time_t)-1;
    struct tm tm;
    auto rt = strptime(str,fmt,&tm);
    if(rt == nullptr) return (time_t)-1;
    return mktime(&tm);
  }

  std::string time2Str(time_t t,const char* fmt)
  {
    if(str == nullptr || fmt == nullptr) return (time_t)-1;
    struct tm tm;
    char buf[128];
    strftime(buf,128,fmt,&tm);
    return std::string(buf);
  }
}
