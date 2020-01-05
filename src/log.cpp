#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>						
#include"logUtil.hpp"
#include"log.hpp"
#include<set>
#include<functional>
#include<map>
#include<sstream>
#include"util.hpp"
#include"macro.hpp"

namespace zhuyh
{
  /*
   *LogLevel 的静态方法定义
   */
  std::string LogLevel::toString(Level level)
  {
#define XX(name)	       \
    if( level == name )	       \
      return #name
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    return "UNKNOWN";
  }
  
  LogLevel::Level LogLevel::fromString(const std::string& level)
  {
#define XX(name) if(level == #name ) return name
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    return DEBUG;
  }


  
  /*
   *使用标准输出的Appender
   */
  void StdoutAppender::log(std::shared_ptr<Logger> logger,
			   LogEvent::ptr event)
  {
    //事件需要确定已经达到了日志要求的级别才输出
    if( event->l_level >= logger->getLogLevel() )
      {
	LockGuard lg(lk);
	std::cout<< f_formatter->format(logger,event);
      }
  }

  /*
    输出到文件的Appender
   */
  FileAppender::FileAppender(const std::string& filename)
    :s_filename(filename)
  {
    //尝试打开
    //std::cout<<s_filename<<std::endl;
    reopen();
  }

  bool FileAppender::reopen()
  {
    out.open(s_filename,std::ios::app);
    //对于打开失败的处理
    if(!out)
      {
	std::cout<<"oops! bad i/o in function: "
		 <<__func__<<" lines : "<<__LINE__
	         <<" file : "<<__FILE__<<std::endl;
	return false;
      }
    isopen = 1;
    return true;
  }

  void FileAppender::log(std::shared_ptr<Logger> logger,
			 LogEvent::ptr event)
  {
    if(isopen == 0)
      {
	reopen();
      }
    //因为reopen不是一定可以打开
    if( isopen && event->l_level >= logger->getLogLevel() )
      {
	LockGuard lg(lk);
	out<<f_formatter->format(logger,event);
      }
  }

  /*
   * Logger的代码实现
   */

  //日志等级默认为DEBUG
  Logger::Logger(LogLevel::Level level,
		 const std::string& fmt,
		 const std::string& name)
    :s_name(name),
     l_loglevel(level),
     f_formatter(new Formatter(fmt) )
  {
  }

  void Logger::addAppender(Appender::ptr appender)
  {
    LockGuard lg(lk);
    //check existanse
    for(auto& val:l_appenders)
      {
	//已经存在则不必添加appender
	if(val == appender )
	  {
	    return ;
	  }
      }
    //appender 如果之前没有设置formatter，则需要设置为logger的formatter
    if(appender -> getFormatter() == nullptr )
      {
	appender -> setFormatter(f_formatter);
      }
    l_appenders.push_back(appender);
  }

  void Logger::delAppender(Appender::ptr appender)
  {
    LockGuard lg(lk);
    for(auto it = l_appenders.begin();
	it != l_appenders.end() ; it++ )
      {
	if(*it == appender )
	  {
	    l_appenders.erase(it);
	    break;
	  }
      }
  }

  //TODO:level为冗余,需要修复
  void Logger::log(LogLevel::Level level,LogEvent::ptr event)
  {
    if(level >= l_loglevel )
      {
	auto self = shared_from_this();
	lk.lock();
	//如果附加器列表为空，则添加一个标准输出附加器
	if(l_appenders.empty() )
	  {
	    auto lg = getRootLogger();
	    lg->log(level,event);
	  }
	else
	  { 
	    for(auto& it:l_appenders)
	      {
		//std::cout<<LogLevel::toString(getBtLevel());
		it->log(self,event);
	      }
	  }
	lk.unlock();
      }
  }

  /*
   * %s - 日志体
   * %L - 日志级别
   * %D{format} - 日期{日期格式} {%Y %m %d %H %M %S}
   * %T - 线程号
   * %K - 线程名称
   * %P - 进程号
   * %E - 程序运行时间
   * %F - 文件号
   * %C - 协程号
   * %f - 函数名
   * %L - 文件行号
   * %t - tab键
   * %n - 换行
   * %N - 日志名
   * %% - 表示一个%,%%%表示一个%和一个转义的'%'
   */
  void Formatter::init()
  {
    //regStr为非上述格式化串
    std::string regStr = "";
    /*
     * 表示解析器状态，0表示没有遇到'%',1表示已经遇到'%'但没有遇到'{',2表示已经遇到'}'
     */
    int status = 0 ;
    int errPos;
    //表示类型与附加格式
    /*
     * FormatType::REGULAR        - 非格式化串
     * FormatType::NOAPPENDFORMAT - 无附加格式
     * FormatType::APPENDFORMAT   - 有附加格式
     * FormatType::FORMATERROR    - 出现错误，解析结束
     */
    typedef std::pair<std::string,int> psi;
    std::list< std::pair<std::string,psi> > strType;
    for(int i=0;i<(int)s_pattern.size();i++)
      {
	if(s_pattern[i] != '%' )
	  {
	    regStr+=s_pattern[i];
	  }
	else
	  {
	    int j = i+1;
	    if(j>=(int)s_pattern.size() )
	      {
		strType.push_back(std::make_pair(std::string("ERROR"),
						 std::make_pair(std::string("%"),
								FormatType::FORMATERROR)));
		errPos = i;
		status = 2;
		break;
	      }
	    //接收一个'%'    
	    if(s_pattern[j] == '%')
	      {
		regStr+=s_pattern[j];
		i=j;
		continue;
	      }
	    // '%'后第一个为被转移的字符，记录之前需要先存下regStr并且清除
	    if(regStr != "" || !regStr.empty() )
	      {
		strType.push_back(std::make_pair("r",
						 std::make_pair(regStr,
								FormatType::REGULAR) ) );
		regStr.clear();
		regStr = "";
	      }
	    //类型
	    std::string appFormat = "";
	    std::string type = "";
	    if(!isalpha(s_pattern[j]) )
	      {
		type+=s_pattern[j];
		strType.push_back(std::make_pair("ERROR",
						 std::make_pair(type,
								FormatType::FORMATERROR) ) );
		status = 2;
		errPos = j;
		break;
	      }
	    type+=s_pattern[j];
	    i=j;
	    int k = j+1;
	    //假定存在附加格式
	    if(k<(int)s_pattern.size())
	      status = 2;
	    for(;k<(int)s_pattern.size();k++)
	      {
		if(k == j+1 && s_pattern[k] != '{')
		  {
		    //此时不为'{'则表示没有附加格式
		    status = 0;
		    break;
		  }
		if(k==j+1 ) continue;
		if(s_pattern[k] == '}')
		  {
		    i=k;
		    status = 0;
		    break;
		  }
		appFormat+=s_pattern[k];
	      }
	    if(status)
	      {
		errPos = k;
		strType.push_back(std::make_pair(appFormat,
						 std::make_pair("",FormatType::FORMATERROR) ) );
		break;
	      }
	    auto tmp = appFormat == "" ? FormatType::NOAPPENDFORMAT : FormatType::APPENDFORMAT;
	    strType.push_back(std::make_pair(type,
					     std::make_pair(appFormat,tmp) ) );
	  }
      }
    //解析结束后状态非0表示出现问题
    if(status)
      {
	auto errEle =strType.back();
	std::cout<<"pattern string : \""<<s_pattern<<"\" error : \""<<errEle.second.first
		 <<"\" at "<<errPos <<std::endl;
	i_error = 1;
      }
    else
      {
	if(regStr != "" && !regStr.empty() )
	  {
	    strType.push_back(std::make_pair("r",
			      std::make_pair(regStr,FormatType::REGULAR) ) );
	    regStr.clear();
	    regStr = "";
	  }
	//利用工厂模式与function,映射查找一个构造函数
	static std::map<std::string,std::function<FormatterItem::ptr(const std::string&)> > mp = {
#define XX(type,CLASSNAME)						\
	  {#type,[](const std::string& fmt) { 	return FormatterItem::ptr(new CLASSNAME(fmt) ); } }
	  XX(s,LogMessageItem),    // %s 日志消息
	  XX(r,LogRegularItem),    // %r 普通字符
	  XX(L,LogLevelItem),     // %L 日志级别
	  XX(D,LogDateItem),      // %D 日期
	  XX(T,LogThreadItem),    // %T 线程号
	  XX(K,ThreadNameItem),   // %K 线程名
	  XX(P,LogProcessItem),   // %P 进程号
	  XX(E,LogElapseItem),    // %E 系统运行时间
	  XX(C,LogCoroutineItem), // %C 协程号
	  XX(f,LogFunctionItem),  // %f 函数名
	  XX(F,LogFileItem),      // %F 文件名
	  XX(l,LogLineItem),      // %l 行号
	  XX(t,LogTabItem),      // %t Tab
	  XX(n,LogNewLineItem),  // %n 换行
	  XX(N,LogNameItem),    // %N 日志名
	  XX(B,LogBackTraceItem) // %B 打印函数调用栈
#undef XX
	};
	
	for(auto& it:strType)
	  {
	    //	         std::cout<<"type: "<<it.first<< "--- appStr "
	    //   <<it.second.first<<"--- typeNum "
	    //	     <<it.second.second<<std::endl;
	    // 	    FormatType::Type typeNum = it.second.second;
	    std::string type = it.first;
	    std::string appStr = it.second.first;
	    auto func = mp.find(type);
	    if(func == mp.end() )
	      {
		std::cout<<"error : type " << type <<" doesn't exist"<<std::endl;
		continue;
	      }
	    auto item = func->second(appStr);
	    v_items.push_back(item);
	  }
	//	std::cout<<v_items.size()<<std::endl;
	
      }
  }
  std::string Formatter::format(Logger::ptr logger,LogEvent::ptr event)
  {
    std::stringstream ss;
    ss.str("");
    ss.clear();
    //将各format组件输出ss
    for(auto& val :v_items)
      {
	//	std::cout<<"HERE\n"<<std::endl;
	val->format(ss,logger,event);
      }
    
    return ss.str();
  }
  void LogBackTraceItem::format(std::stringstream& ss,
				std::shared_ptr<Logger> logger,
				LogEvent::ptr event)
  {
    if(event->l_level >= logger->getBtLevel())
      {
	ss<<"\nBackTrace:\n"<<Bt2Str(100,7,"         ");
      }
  }
  //日志体组件
  void LogMessageItem::format(std::stringstream& ss,
			      std::shared_ptr<Logger> logger,
			      LogEvent::ptr event)
  {
    //  std::cout<<"Msg = "<<event->getMsg()<<std::endl;
    ss<<event->getMsg();
  }

  //日志级别
  void LogLevelItem::format(std::stringstream& ss,
			      std::shared_ptr<Logger> logger,
			      LogEvent::ptr event)
  {
    ss << LogLevel::toString(event->l_level);
  }

  //非转义字符
  void LogRegularItem::format(std::stringstream& ss,
			      std::shared_ptr<Logger> logger,
			      LogEvent::ptr event)
  {
    ss << s_str;
  }

  //日期
  void LogDateItem::format(std::stringstream& ss,
			      std::shared_ptr<Logger> logger,
			      LogEvent::ptr event)
  {
    struct tm tm;
    char buf[128];
    time_t t = event->u_times;
    localtime_r(&t,&tm);
    strftime(buf,128,s_fmt.c_str(),&tm);
    ss<<buf;
    
  }

  //线程号
  void LogThreadItem::format(std::stringstream& ss,
			      std::shared_ptr<Logger> logger,
			      LogEvent::ptr event)
  {
    ss << event->p_tid;
  }

  //进程号
  void LogProcessItem::format(std::stringstream& ss,
			      std::shared_ptr<Logger> logger,
			      LogEvent::ptr event)
  {
    ss << event->p_pid;
  }

  //程序运行时间
  void LogElapseItem::format(std::stringstream& ss,
			     std::shared_ptr<Logger> logger,
			     LogEvent::ptr event)
  {
    ss << event->u_eplapse;
  }
  //文件名
  void LogFileItem::format(std::stringstream& ss,
			   std::shared_ptr<Logger> logger,
			   LogEvent::ptr event)
  {
    ss<< event->s_file; 
  }

  //协程号
  void LogCoroutineItem::format(std::stringstream& ss,
				std::shared_ptr<Logger> logger,
				LogEvent::ptr event)
  {
    ss << event->u_cid;
  }
  
  //函数名
  void LogFunctionItem::format(std::stringstream& ss,
			       std::shared_ptr<Logger> logger,
			       LogEvent::ptr event)

  {
    ss << event->s_func;
  }

  //行号
  void LogLineItem::format(std::stringstream& ss,
			   std::shared_ptr<Logger> logger,
			   LogEvent::ptr event)
  {
    //std::cout<<"LINE:"<< event->u_lines <<std::endl;
    ss << event->u_lines;
  }

  //换行
  void LogNewLineItem::format(std::stringstream& ss,
			    std::shared_ptr<Logger> logger,
			    LogEvent::ptr event)
  {
    ss <<"\n";
  }

  //Tab
  void LogTabItem::format(std::stringstream& ss,
			  std::shared_ptr<Logger> logger,
			  LogEvent::ptr event)
  {
    ss<<"\t";
  }

  //日志名
  void LogNameItem::format(std::stringstream& ss,
			   std::shared_ptr<Logger> logger,
			   LogEvent::ptr event)
  {
    ss << event->s_logname;
    //    std::cout<<"name = "<<event->s_logname<<" ss = "<<ss.str()<<std::endl;
  }
  void ThreadNameItem::format(std::stringstream& ss,
	      std::shared_ptr<Logger> logger,
	      LogEvent::ptr event)
  {
    ss << Thread::thisName();
  }
  LogEvent::LogEvent(std::shared_ptr<Logger> _logger,LogLevel::Level level,
		     time_t elapse,time_t times,pid_t pid,pid_t tid,
		     uint32_t lines,const std::string& files,uint32_t cid,
		     const std::string& func,const std::string& logname)
    :l_level(level),u_eplapse(elapse),u_times(times),
     p_pid(pid),p_tid(tid),u_cid(cid),u_lines(lines),s_file(files),
    s_func(func),s_logname(logname),logger(_logger)
  {
   if(s_logname == "" || s_logname.empty())
     {
       s_logname = logger->getLogName();
     }
  }
  
  /*
   *@brief 定义日志
   */
  struct AppenderDefine
  {
    std::string s_format;
    LogLevel::Level l_level = LogLevel::Level::DEBUG;
    //0代表标准输出
    bool type = 0;
    std::string s_filename;
    bool isStdout()
    {
      return type == 0;
    }
    bool operator==(const AppenderDefine& o) const
    {
      return o.s_format == s_format &&
	 l_level == o.l_level &&
	type == o.type &&
	s_filename == o.s_filename;
    }
  };
  
  struct LogDefine
  {
    std::string s_logName;
    std::string s_format;
    std::vector<AppenderDefine> apps;
    LogLevel::Level l_level;
    LogLevel::Level btLevel;
    bool operator==(const LogDefine& o) const 
    {
      return s_logName == o.s_logName &&
	s_format == o.s_format &&
	apps == o.apps &&
	l_level == o.l_level;
    }
    friend std::ostream& operator<<(std::ostream& os,const LogDefine& o)
    {
      os<<"LogDefine Info:"<<std::endl;
      os<<"name:"<<o.s_logName<<std::endl;
      os<<"format:"<<o.s_format<<std::endl;
      os<<"level:"<<LogLevel::toString(o.l_level);
      return os;
    }
    bool operator<(const LogDefine& o) const
    {
      return s_logName < o.s_logName;
    }

    bool isValid() const
    {
      return !s_logName.empty();
    }
  };

  /*
   *@brief std::string --> LogDefine
   *@exception logic_error
   */
  template<>
  class Lexical_Cast<std::string,LogDefine>
  {
  public:
    LogDefine operator() (const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      LogDefine ld;
      if( !node["name"].IsDefined() )
	{
	  std::cout<<"log name is empty"<<std::endl;
	  throw std::logic_error("log name is null");
	}
      //转换为std::string
      ld.s_logName = node["name"].as<std::string>();
      //BackTrace Level(Default ERROR)
      ld.btLevel = LogLevel::fromString(node["btLevel"].IsDefined()?node["btLevel"].as<std::string>():"ERROR");
      //不设置则为DEBUG级别
      ld.l_level = LogLevel::fromString(node["level"].IsDefined()?node["level"].as<std::string>():"");
      //日志格式
      if(node["formatter"].IsDefined() )
	{
	  ld.s_format = node["formatter"].as<std::string>();
	}
      else
	{
	  ld.s_format = "";
	  ld.s_format.clear();
	}
      //解析Appender
      if(node["appenders"].IsDefined() )
	{
	  auto apps = node["appenders"];
	  for(size_t i = 0;i<apps.size();i++)
	    {
	      AppenderDefine ad;
	      auto n = apps[i];
	      ad.l_level = LogLevel::fromString(n["level"].IsDefined()?n["level"].as<std::string>():"");
	      if(node["formatter"].IsDefined() )
		{
		  ad.s_format = n["formatter"].as<std::string>();
		}
	      else
		{
		  ad.s_format = "";
		  ad.s_format.clear();
		}
	      if(!n["type"].IsDefined() || n["type"].as<std::string>() == "StdoutAppender")
		{
		  goto appDone;
		}
	      if(!n["file"].IsDefined() ) goto appDone;
	      ad.type = 1;
	      ad.s_filename = n["file"].as<std::string>();
	    appDone:
	      ld.apps.push_back(ad);
	    }
	}
      return ld;
    }
  };
  template<>
  class Lexical_Cast<LogDefine,std::string>
  {
  public:
    std::string operator() (const LogDefine& v)
    {
      YAML::Node node(YAML::NodeType::Map);
      node["name"] = v.s_logName;
      node["level"] = LogLevel::toString(v.l_level);
      node["btLevel"] = LogLevel::toString(v.btLevel);
      if( !v.s_format.empty() )
	{
	  node["formatter"] =  v.s_format;
	}
      else
	{
	  node["formatter"] = "";
	}
      
      YAML::Node app(YAML::NodeType::Sequence);
      for(auto& val:v.apps)
	{
	  YAML::Node app(YAML::NodeType::Map);
	  if(val.type == false)
	    app["type"] = "StdoutAppender";
	  else
	    {
	      app["type"] = "FileAppender";
	      app["file"] = val.s_filename;
	    }
	  app["level"] = LogLevel::toString(val.l_level);
	  if( !val.s_format.empty() )
	    {
	      app["formatter"] = val.s_format;
	    }
	  else
	    {
	      app["formatter"] = "";
	    }
	}
      node["appenders"] = app;
      std::stringstream ss;
      ss<<node;
      return ss.str();
    }
  };

  ConfigVar<std::set<LogDefine> >::ptr __log_define_ =
    Config::lookUp("logs",std::set<LogDefine>(),"log configs");

  
  void onLogChange(const std::set<LogDefine>& oldVal,
		  const std::set<LogDefine>& newVal)
  {
    for(auto& item:newVal)
      {
	auto it = oldVal.find(item);
	Logger::ptr logger;
	if(it == oldVal.end() || !(item == *it) )
	  {
	    //不存在/不相同则新增
	    logger = ADD_LOGGER(item.s_logName,item.l_level,item.s_format);
	  }
	else
	  {
	    //完全相同什么都不做
	    continue;      
	  }
	logger->setBtLevel(item.btLevel);
	logger->setLogLevel(item.l_level);
	if( !item.s_format.empty() )
	  {
	    Formatter::ptr fmt( new Formatter(item.s_format) );
	    if(fmt->isCorrect() )
	      {
		logger->setFormatter(fmt);
	      }
	    else
	      {
		std::cout<<"Uncorrect Log Format!"<<std::endl;
	      }
	  }
	logger->clearAppender();
	for(auto& i:item.apps)
	  {
	    Appender::ptr ap;
	    if(i.type == 0)
	      {
		ap.reset(new StdoutAppender() );
	      }
	    else
	      {
		ap.reset(new FileAppender(i.s_filename));
		//		std::cout<<i.s_filename<<std::endl;
		//ap->log(GET_LOGGER("root"),std::make_shared<LogEvent>(LogEvent()) );
	      }
	    if(!i.s_format.empty() )
	      {
		Formatter::ptr fmt(new Formatter(i.s_format) );
		if(fmt->isCorrect() )
		  {
		    ap->setFormatter(fmt);
		  }
		else
		  {
		    std::cout<<"formatter error"<<std::endl;
		  }
	      }
	    //	    std::cout<<ap->getFileName()<<std::endl;
	    logMgr()->addAppender(item.s_logName,ap);
	  }
      }
    for(auto& item:oldVal)
      {
	auto it = newVal.find(item);
	if( it == newVal.end() )
	  {
	    //删除logger
	    auto logger = GET_LOGGER(item.s_logName);
	    logger->setLogLevel((LogLevel::Level)100);
	    logger->clearAppender();
	  }
      }
  }
  struct LogIniter
  {
    LogIniter()
    {
      __log_define_->addCb(onLogChange);
      Config::loadFromYamlFile("/home/zhuyh/mnt/Code/GraduationDesign/zhuyh/config/logs.yml");
    }
  };
  static LogIniter __log_initer_;
} 
