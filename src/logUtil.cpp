#include"logUtil.hpp"
#include<map>
#include"macro.hpp"

namespace zhuyh
{
  //根日志默认级别为INFO
  Logger::ptr& getRootLogger(LogLevel::Level loglevel, 
		const std::string& fmt,
		const std::string& logname )
  {
    //Elapse从创建根日志时开始计算
    start();
    static Logger::ptr logger(new Logger(loglevel,fmt,logname) );
    return logger;
  }
  
  Logger::ptr ADD_LOGGER(const std::string& name,
				 LogLevel::Level level,
				 const std::string& fmt)
  {
    if(fmt.empty() || fmt == "")
      {
	return GET_LOG_MGR()->addLogger(name,level);
      }
    else
      {
	return GET_LOG_MGR()->addLogger(name,level,fmt);
      }
  }
  /*
   * 日志管理器
   */
  std::atomic_flag LogMgr::once(ATOMIC_FLAG_INIT);
  LogMgr::LogMgr()
    :stdoutAppender(new StdoutAppender)
  {
    app_map["s4d0ut"] = stdoutAppender;
    getRootLogger()->addAppender(stdoutAppender );
    log_map.insert(make_pair("root",getRootLogger() ) );
    addFileAppender("root","root.log");
  }

  bool LogMgr::addStdoutAppender(const std::string& name = "s4d0ut" )
  {
    auto it = log_map.find(name);
    if(it == log_map.end() )
      return false;
    it->second->addAppender(stdoutAppender);
    return true;
  }

  void LogMgr::addAppender(const std::string& name , Appender::ptr app)
  {
    if(app == nullptr) return;
    app_map[app->getFileName()] = app;
    log_map[name]->addAppender(app);
  }
  bool LogMgr::addFileAppender(const std::string& name,const std::string& filename)
  {
    auto it = log_map.find(name);
    if(it == log_map.end() )
      return false;
    auto appender = lookUpFileAppender(filename);
    if(appender == nullptr)
      {
	appender.reset(new FileAppender(filename) );
      }
    app_map.insert( std::make_pair(filename,appender) );
    it->second->addAppender(appender);
    return true;   
  }

  void LogMgr::delStdoutAppender(const std::string&name )
  {
    auto it = log_map.find(name);
    if(it == log_map.end() )
      return;
    it->second->delAppender(lookUpStdoutAppender());
  }
  
  Appender::ptr LogMgr::lookUpStdoutAppender()
  {
    return stdoutAppender;
  }
  
  void LogMgr::delFileAppender(const std::string& name,const std::string& filename)
  {
    auto it = log_map.find(name);
    if(it == log_map.end() ) return;
    auto appender = lookUpFileAppender(filename);
    if(appender != nullptr)
      it->second->delAppender(appender);
  }

  Appender::ptr LogMgr::lookUpFileAppender(const std::string& filename)
  {
    auto it2 = app_map.find(filename);
    if(it2 != app_map.end() )
      {
	return it2->second;
      }
    return nullptr;
  }

  //查找
  Logger::ptr LogMgr::lookUp(const std::string& name)
  {
    auto it = log_map.find(name);
    if(it == log_map.end() )
      {
	//没有则返回root日志
	it = log_map.find("root");
	//以防万一
	if(it == log_map.end() )
	  return nullptr;
      }
    return it->second;
  }
  
  void LogMgr::delLogger(const std::string& name)
  {
    if(name == "root") return;
    auto it = log_map.find(name);
    if(it == log_map.end() ) return;
    log_map.erase(it);
  }

  Logger::ptr LogMgr::addLogger(const std::string& name,
				LogLevel::Level loglevel,
				const std::string& fmt )
  {
    Logger::ptr logger(new Logger(loglevel,fmt,name));
    log_map[name]=logger;
    return logger;
  }

  LogMgr::ptr LogMgr::getInstance()
  {
    if(!once.test_and_set() )
      {
	return LogMgr::ptr(new LogMgr()); 
      }
    return nullptr;
  }
  
  LogMgr::ptr& logMgr()
  {
   static auto v =  LogMgr::getInstance();
   return v;
  }
}
