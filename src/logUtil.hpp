#pragma once

#include <map>
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include "config/config.hpp"
#include "log.hpp"
#include "util.hpp"
#include "logUtil.hpp"
#include "macro.hpp"
#include "concurrent/Thread.hpp"

namespace zhuyh
{
  //根日志默认级别为INFO
  Logger::ptr& getRootLogger(LogLevel::Level loglevel = LogLevel::INFO,
			     const std::string& fmt = "%D{%Y-%m-%d %H:%M:%S}%t"
			     "%N%t[%L]%t%P%t%T%t%F:%l(%f)"
			     "%t%s%B%n",
			     const std::string& logname = "root");
    // 日志管理器，单例
  class LogMgr
  {
  public:
    //
    typedef std::shared_ptr<LogMgr> ptr;
    bool addStdoutAppender(const std::string& name);
    bool addFileAppender(const std::string& name,const std::string& filename);

    void delFileAppender(const std::string& name,const std::string& filename);
    void delStdoutAppender(const std::string& name);
    void addAppender(const std::string& name,Appender::ptr app);
    inline Appender::ptr lookUpFileAppender(const std::string& filename);
    inline Appender::ptr lookUpStdoutAppender();

    //查找
    Logger::ptr lookUp(const std::string& name);
    
    void delLogger(const std::string& name);
    Logger::ptr addLogger(const std::string& name,LogLevel::Level loglevel = LogLevel::INFO,
			  const std::string& fmt = "%D{%Y-%m-%d %H:%M:%S}%t%N%t[%L]%t%C%t%K(%T)"
			  "%t%F:%l(%f)%t%s%B%n");
    //获取实例
    static LogMgr::ptr getInstance();
  private:
    LogMgr();
    static std::atomic_flag once;
    bool nameValidate(const std::string& name);
    std::map<std::string,Logger::ptr> log_map;
    std::map<std::string,Appender::ptr> app_map;
    Appender::ptr stdoutAppender;
    //禁止拷贝复制
    LogMgr& operator=(const LogMgr&) = delete;
  };
  LogMgr::ptr& logMgr();
  Logger::ptr ADD_LOGGER(const std::string& name,
			 LogLevel::Level level,
			 const std::string& fmt);
  
}

#define GET_LOGGER(name) zhuyh::logMgr()->lookUp(name)

#define LOGGER(level,logger)   \
  if(level>=logger->getLogLevel())					\
    zhuyh::LogWrapper(zhuyh::LogEvent::ptr(new zhuyh::LogEvent(logger,level, \
							       zhuyh::getElapseTime(), \
							       zhuyh::getCurrentTime(), \
							       zhuyh::getProcessId(), \
							       zhuyh::getThreadId(), \
							       __LINE__,__FILE__, \
							       zhuyh::getCoroutineId(), \
							       __func__,logger->getLogName(), \
							       zhuyh::Thread::thisName()))).getSS()

#define LOG_ROOT(level) LOGGER(level,GET_LOGGER("root"))
#define LOG_ROOT_DEBUG() LOG_ROOT(zhuyh::LogLevel::Level::DEBUG)
#define LOG_ROOT_INFO() LOG_ROOT(zhuyh::LogLevel::Level::INFO)
#define LOG_ROOT_WARN() LOG_ROOT(zhuyh::LogLevel::Level::WARN)
#define LOG_ROOT_ERROR() LOG_ROOT(zhuyh::LogLevel::Level::ERROR)
#define LOG_ROOT_FATAL() LOG_ROOT(zhuyh::LogLevel::Level::FATAL)

#define LOG_DEBUG(logger) LOGGER(zhuyh::LogLevel::Level::DEBUG,logger)
#define LOG_INFO(logger) LOGGER(zhuyh::LogLevel::Level::INFO,logger)
#define LOG_WARN(logger) LOGGER(zhuyh::LogLevel::Level::WARN,logger)
#define LOG_ERROR(logger) LOGGER(zhuyh::LogLevel::Level::ERROR,logger)
#define LOG_FATAL(logger) LOGGER(zhuyh::LogLevel::Level::FATAL,logger)


#define ADD_FILE_APPENDER(name,filename) zhuyh::logMgr()->addFileAppender(name,filename)
#define ADD_STDOUT_APPENDER(name) zhuyh::logMgr()->addStdoutAppender(name)

#define DEL_FILE_APPENDER(name,filename) zhuyh::logMgr()->delFileAppender(name,filename)
#define DEL_STDOUT_APPENDER(name) zhuyh::logMgr()->delStdoutAppender(name)

#define GET_LOG_MGR() zhuyh::logMgr()

#define ADD_DFT_LOGGER(name) GET_LOG_MGR()->addLogger(name)
      
#define DEL_LOGGER(name) GET_LOG_MGR()->delLogger(name)
