#pragma once

#include "config.hpp"
#include <list>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <atomic>
#include "latch/lock.hpp"
#include "concurrent/Thread.hpp"

namespace zhuyh
{
  class Logger;
  class Appender;
  //日志级别
  class LogLevel
  {
  public:
    enum Level
      {
	UNKNOWN = 0,
	DEBUG = 1,
	INFO = 2,
	WARN = 3,
	ERROR = 4,
	FATAL = 5,
	LevelMaxinum = 6
      };
    static std::string toString(Level level);
    static Level fromString(const std::string& level);
  };

  //日志事件
  struct LogEvent
  {
    LogLevel::Level l_level = LogLevel::DEBUG;
    time_t u_eplapse;
    time_t u_times;
    pid_t p_pid;
    pid_t p_tid;
    uint32_t u_cid;
    //行号
    uint32_t u_lines;
    //文件
    std::string s_file ;
    //函数
    std::string s_func ;
    std::string s_logname;
    //最终所有消息写入到stringstream中
    std::stringstream ss;
    std::shared_ptr<Logger> logger;
    typedef std::shared_ptr<LogEvent> ptr;
    std::string getMsg()
    {
      return ss.str();
    }
    LogEvent(std::shared_ptr<Logger> _logger,LogLevel::Level level,
	     time_t elapse,time_t times,pid_t pid,pid_t tid,
	     uint32_t lines,const std::string& files,uint32_t cid,
	     const std::string& func,const std::string& logname = "");
    LogEvent() {}
  };

  //日志格式器组件抽象类
  class FormatterItem
  {
  public:
    typedef std::shared_ptr<FormatterItem> ptr;
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) = 0;
    FormatterItem(const std::string& str = "") {}
    virtual ~FormatterItem() {}
    
  };
  class LogBackTraceItem : public FormatterItem
  {
    public:
    typedef std::shared_ptr<LogBackTraceItem> ptr;
    LogBackTraceItem(const std::string& str) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  
  };
  //日志体 %s
  class LogMessageItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogMessageItem> ptr;
    LogMessageItem(const std::string& str) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };
  
  //日志级别%L
  class LogLevelItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogLevelItem> ptr;
    LogLevelItem(const std::string& str) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };

  //非转义字符串
  class LogRegularItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogRegularItem> ptr;
    LogRegularItem(const std::string& str):s_str(str) { }
    void format(std::stringstream& ss,
		std::shared_ptr<Logger> logger,
		LogEvent::ptr event) override;
  private:
    std::string s_str;
  };

  //日期 %D{format} format可选，默认format:%Y-%m-%d %H:%M:%S
  class LogDateItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogDateItem> ptr;
    LogDateItem(const std::string& fmt = "%Y-%m-%d %H:%M:%S"):s_fmt(fmt) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  private:
    std::string s_fmt;
  };

  //线程号 %T
  class LogThreadItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogThreadItem> ptr;
    LogThreadItem(const std::string& fmt) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };
  
  //进程号 %P
  class LogProcessItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogProcessItem> ptr;
    LogProcessItem(const std::string& fmt) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };
  
  //程序运行时间 %E
  class LogElapseItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogElapseItem> ptr;
    LogElapseItem(const std::string& fmt) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };
  
  //程序文件名 %F
  class LogFileItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogFileItem> ptr;
    LogFileItem(const std::string& fmt) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };
  
  //协程号 %C
  class LogCoroutineItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogCoroutineItem> ptr;
    LogCoroutineItem(const std::string& fmt) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };

  //函数名 %f
  class LogFunctionItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogFunctionItem> ptr;
    LogFunctionItem(const std::string& fmt) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };

  //文件行号 %L
  class LogLineItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogLineItem> ptr;
    LogLineItem(const std::string& fmt) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };

  //Tab键 %t
  class LogTabItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogTabItem> ptr;
    LogTabItem(const std::string& fmt) {}
    virtual void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };

  //换行 %n
  class LogNewLineItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogNewLineItem> ptr;
    LogNewLineItem(const std::string& fmt) {}
    void format(std::stringstream& ss,
			std::shared_ptr<Logger> logger,
			LogEvent::ptr event) override;
  };
  
  class LogNameItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<LogNameItem> ptr;
    LogNameItem(const std::string& fmt) {}
    void format(std::stringstream& ss,
		std::shared_ptr<Logger> logger,
		LogEvent::ptr event);
  };
  
  class ThreadNameItem : public FormatterItem
  {
  public:
    typedef std::shared_ptr<ThreadNameItem> ptr;
    ThreadNameItem(const std::string& fmt) {}
    void format(std::stringstream& ss,
		std::shared_ptr<Logger> logger,
		LogEvent::ptr event);
  };
  //各个组件内容类型，用于Formatter::init
  class FormatType
  {
  public:
    enum Type{
      REGULAR = 0,
      NOAPPENDFORMAT = 1,
      APPENDFORMAT = 2,
      FORMATERROR = 3
    };
  };
  //日志格式器
  class Formatter
  {
  public:
    typedef std::shared_ptr<Formatter> ptr;
    void init();
    //event打包成字符串
    std::string format(std::shared_ptr<Logger> logger,
		       LogEvent::ptr event);
    Formatter(const std::string& pattern)
      :s_pattern(pattern) {
      init();
    }
    bool isCorrect() const { return i_error == 0; }
  private:
    int i_error = 0;
    std::string s_pattern;
    std::vector<FormatterItem::ptr> v_items;
  };

  //抽象基类
  class Appender : public std::enable_shared_from_this<Appender>
  {
  public:
    friend class FormatterItem;
    typedef std::shared_ptr<Appender> ptr;
    virtual ~Appender() {}
    virtual std::string getFileName()
    {
      return "s4d0ut";
    }
    virtual void log(std::shared_ptr<Logger> logger,
		     LogEvent::ptr event) = 0;
    Appender():f_formatter(nullptr) {}
    
    Formatter::ptr getFormatter()  {
      return f_formatter;
    }
    
    void setFormatter(Formatter::ptr formatter) {
      f_formatter = formatter;
    }
  protected:
    Mutex lk;
    Formatter::ptr f_formatter;
  };
  
  class Logger : public std::enable_shared_from_this<Logger>
  {
  public:
    typedef std::shared_ptr<Logger> ptr;
    Logger(LogLevel::Level level = LogLevel::DEBUG,
	   const std::string& fmt = "[%L]%D{%Y-%m-%d %H:%M:%S}%t%s%n",
	   const std::string& name = "root");
    //日志级别
    LogLevel::Level getLogLevel() {
      //LockGuard lg(lk);
      return l_loglevel;
    }
    void setLogLevel(LogLevel::Level level)  {
      LockGuard lg(lk);
      l_loglevel = level;
    }
    void delAppender(Appender::ptr appender);
    void addAppender(std::shared_ptr<Appender> appender);
    void clearAppender()
    {
      LockGuard lg(lk);
      l_appenders.clear();
    }
    //日志名
    std::string getLogName()
    {
      return s_name;
    }

    //打印日志
    void setFormatter(Formatter::ptr formatter)
    {
      f_formatter = formatter;
    }
    Formatter::ptr  getFormatter()
    {
      return f_formatter;
    }
    void setBtLevel(LogLevel::Level btLevel) { _btLevel = btLevel;}
    LogLevel::Level getBtLevel() const { return _btLevel; }
    void log(LogLevel::Level level,LogEvent::ptr event);
  private:
    //打印调用栈的最低级别
    LogLevel::Level _btLevel = LogLevel::ERROR;
    Mutex lk;
    std::list<Appender::ptr> l_appenders;
    std::string s_name;
    LogLevel::Level l_loglevel;
    Formatter::ptr f_formatter;
  };
  
  class StdoutAppender : public Appender
  {
  public:
    typedef std::shared_ptr<StdoutAppender> ptr;
    void log(std::shared_ptr<Logger> logger,
	     LogEvent::ptr event) override;
  };
  
  class FileAppender : public Appender
  {
  public:
    typedef std::shared_ptr<FileAppender> ptr;
    void log(std::shared_ptr<Logger> logger,
	     LogEvent::ptr event) override;
    FileAppender(const std::string& filename );
    std::string getFileName() override
    {
      return s_filename;
    }
    ~FileAppender()
    {
      out.close();
    }
  private:
    //打开一个文件
    std::ofstream out;
    bool reopen();
    bool isopen = 0;
    std::string s_filename;
  };

  class LogWrapper
  {
  public:
    //TODO:logger设置为根日志
    LogWrapper(LogEvent::ptr event)
      :l_event(event)
    {
    }
    ~LogWrapper() {
      l_event->logger->log(l_event->l_level,l_event);
      //std::cout<<"HERE\n";
    }
    std::stringstream& getSS()
    {
      return l_event->ss;
    }
  private:    
    LogEvent::ptr l_event;
  };

}
