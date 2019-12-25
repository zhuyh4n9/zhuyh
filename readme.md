# mylog

## 说明
 - 本日志系统模仿log4cplus的三大组件(logger,appender,formatter)完成
 - 线程锁使用自旋锁

## 实例代码
 - 见目录下example文件夹
## Formatter格式
 - %s : 日志体
 - %L : 日志级别
 - %r : 非转义字符串
 - %D{format} : 日期{日期格式}
    - %Y :年
    - %m : 月
    - %d :日
    - %H :时
    - %M :分
    - %S :秒
 - %T : 线程号
 - %P : 进程号
 - %E : 程序运行时间
 - %C : 协程号
 - %F : 文件名
 - %f : 函数名	
 - %l : 文件行号
 - %t : tab键	
 - %n : 换行	
 - %N : 日志名		
 - %% : 表示一个%,%%%表示一个%和一个转义的'%'
 - 默认formatter : %N%t[%L]%t%P%t%T%t%F:%l(%f)%t%D{%Y-%m-%d %H:%M:%S}%t%s%n
 
## 使用日志
 - LOGGER_ROOT(level) 使用root日志来打印level级别的日志
   -  LOGGER_ROOT_XXX() 使用root日志打印XXX级别日志 XXX属于{DEBUG,INFO,WARN,ERROR,FATAL}
 - LOGGER_XXX(logger) 使用logger来印XXX级别日志 XXX属于{DEBUG,INFO,WARN,ERROR,FATAL}
 - GET_LOGGER(name) 获取日志名为name的日志
 
### 日志管理
 - 建议使用GET_LOG_MGR宏来获取日志管理器,方便统一管理日志器
 - ADD/DEL_FILE_APPENDER(name,file) 向名为name的日志增加/删除文件为file的附加器
 - ADD/DEL_STDOUT_APPENDER(name) 向名为name的日志增加/删除标准输出附加器
 - GET_LOG_MGR()获取日志管理器
 - ADD_DFT_LOGGER(name)
   - 创建名为name使用默认配置的日志器
 - ADD_LOGGER(name,level,fmt)
   - 创建一个名为name手动配置的日志器
 - DEL_LOGGER(name)
   - 删除一个名为name的日志器
   - 不可以删除root日志器
   
### 使用LogMgr函数管理日志
 - Logger::ptr addLogger(const std::string& name,LogLevel::Level level,const std::string& fmt)
    - 默认fmt:%N%t[%L]%t%P%t%T%t%F:%l(%f)%t%D{%Y-%m-%d %H:%M:%S}%t%s%n
    - level : INFO
 - inline Appender::ptr lookUpFileAppender(const std::string& filename)
   - 查看file的appender,没有则返回nullptr
 - inline Appender::ptr lookUpStdoutAppender()
   - 查看stdout的appender
 - bool/void del/addFileAppender(const std::string& name,const std::string& filename)
   - 向名为name的日志增加/删除文件名为filename的附加器
 - void/bool add/delStdoutAppender(const std::string& name)
   - 向名为name的日志增加/删除stdout附加器
 - void delLogger(const std::string& name);
   - 增加/删除名为name的日志器
 - Logger::ptr lookUp(const std::string& name)
   - 查找名为name的日志器,没有则返回nullptr
