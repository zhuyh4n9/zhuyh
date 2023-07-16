#include "stackTrait.hpp"
#include <unistd.h>
#include <sys/mman.h>
#include "log.hpp"
#include "logUtil.hpp"
#include "config/config.hpp"
#include <cstring>

namespace zhuyh
{
  //默认保护的页面数量
  static ConfigVar<size_t>::ptr __n_protect_page =
    Config::lookUp<size_t>("fiber.protect_page",1,"fiber stack protect page count");
  static Logger::ptr s_syslog = GET_LOGGER("system");

  //获取要保护的大小
  size_t& StackTrait::getProtectStackPageSize()
  {
    static size_t _pages = __n_protect_page->getVar();
    return _pages;
  }

  /*
   * 对协程栈的栈顶进行保护,将访问权限设置为PROT_NONE(不可访问)
   * 栈是从高地址向低地址增长
   *     |   stack   | 传入的栈顶部最大地址
   *     |  pagesize | 保护1页(默认1页)的数据
   */
  bool StackTrait::protectStack(void* stack,size_t size,
				size_t pages_protect)
  {
    if(!stack) return false;
    if(!pages_protect) return false;
    if(size <= (pages_protect + 1)*getpagesize() )
      {
	LOG_WARN(s_syslog) << "Fiber Stack is Smaller than (pages_protect + 1)*getpagesize()"
	  " Stack Protect Failed";
	return false;
      }
    //保证开始保护的地址为4k字节对齐(mprotect要求)
    void* start_addr = ((size_t)stack & 0xfff) ? (void*)(((size_t)stack & ~0xfff ) + 0x1000) : stack;
    //将高4k设置为不允许访问
    if( mprotect(start_addr,pages_protect*getpagesize(),PROT_NONE ) )
      {
	if(errno == EACCES)
	  {
	    LOG_WARN(s_syslog) << "EACCES : " << strerror(errno);
	  }
	else if(errno == EINVAL )
	  {
	    LOG_WARN(s_syslog) << "EINVAL : " << strerror(errno);
	  }
        else if(errno == ENOMEM)
	  {
	    LOG_WARN(s_syslog) << "ENOMEM : " << strerror(errno);
	  }
	else
	  {
	    LOG_WARN(s_syslog) << "UNKNOWN";
	  }
        return false;
      }
    //    LOG_DEBUG(s_syslog) << "Successfully Protect the Fiber Stack, addr : "
    //		       <<start_addr <<" origin addr : "<<stack;
    return true;
  }

  //取消栈保护
  void StackTrait::unprotectStack(void* stack,size_t pages_protect)
  {
    if(!stack) return;
    if(!pages_protect) return;
    //保证开始保护的地址为4k字节对齐(mprotect的要求)
    void* start_addr = ((size_t)stack & 0xfff) ? (void*)(((size_t)stack & ~0xfff ) + 0x1000) : stack;
    //将高4k设置为不允许访问
    if( mprotect(start_addr,pages_protect*getpagesize(),
		 PROT_READ | PROT_WRITE) )
      {
	//如果出问题,可能问题比较严重因此在这里使用断言
	ASSERT2(false,strerror(errno));
      }
    //    LOG_DEBUG(s_syslog) << "Successfully Unprotect the Fiber Stack, addr : "
    //		       <<start_addr <<" origin addr : "<<stack;
  }
  
}
