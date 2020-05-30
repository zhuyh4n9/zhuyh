#include"mysql.hpp"
#include<time.h>
#include"../logs.hpp"
#include"../util.hpp"
#include"../macro.hpp"
#include"../config.hpp"

namespace zhuyh
{
namespace db
{
  ConfigVar<std::map<std::string,std::string>>::ptr s_mysql_define =
    Config::lookUp("mysql.define",std::map<std::string,std::string>({
             {"charset","UTF8"},
	     {"port","3306"},
	     {"host","127.0.0.1"},
	     {"user","root"},
	     {"passwd","169074291"},
	     {"dbname","prac"}
	}),"mysql define");

  ConfigVar<uint32_t>::ptr s_max_conns =
    Config::lookUp<uint32_t>("mysql.max_conns",30,"mysql max connections");
  
  static Logger::ptr s_logger = GET_LOGGER("system");
  class MySQLUtils
  {
  public:
    template<class F,class T>
    static std::shared_ptr<T> ptrTypeCast(const std::shared_ptr<F> from)
    {
      std::shared_ptr<T> to = std::dynamic_pointer_cast<T>(from);
      if(to == nullptr)
	{
	  throw(std::logic_error("ptrTypeCast failed"));
	}
      return to;
    }
    /*
     *@breif : MYSQL_TIME -> time_t
     *@return : 0 for failed
     */
    static time_t fromMySQLTime(const MYSQL_TIME& mt)
    {
      struct tm tm;
      time_t t;
      localtime_r(&t,&tm);
      tm.tm_year = mt.year - 1900;
      tm.tm_mon = mt.month - 1;
      tm.tm_mday = mt.day;
      tm.tm_hour = mt.hour;
      tm.tm_min = mt.minute;
      tm.tm_sec = mt.second;
      t = mktime(&tm);
      return ( t == (time_t) -1) ? 0 : t;
    }
    
    /*
     *@brief : time_t -> MySQL_TIME
     *@return : 0 for failedx
     */
    static MYSQL_TIME toMySQLTime(time_t t)
    {
      struct tm tm;
      localtime_r(&t,&tm);
      MYSQL_TIME mt;
      mt.year = tm.tm_year + 1900;
      mt.month = tm.tm_mon + 1;
      mt.day = tm.tm_mday;
      mt.hour = tm.tm_hour;
      mt.minute = tm.tm_min;
      mt.second = tm.tm_sec;
      return mt;
    }
  };

  struct MySQLThreadIniter
  {
    MySQLThreadIniter()
    {
      mysql_thread_init();
    }
    ~MySQLThreadIniter()
    {
      mysql_thread_end();
    }    
  };


  //初始化并且连接
  static MYSQL* mysql_init(const std::map<std::string,std::string>& params,
			   int timeout)
  {
    static MySQLThreadIniter s_thread_initer;
    MYSQL* mysql = ::mysql_init(nullptr);
    if(mysql == nullptr)
      {
	LOG_ERROR(s_logger) << "mysql_init error";
	return nullptr;
      }
    //设置连接超时
    if(timeout > 0)
      {
	mysql_options(mysql,MYSQL_OPT_CONNECT_TIMEOUT,&timeout);
      }
    bool close = false;
    //连接丢失是否自动重连
    mysql_options(mysql,MYSQL_OPT_RECONNECT,&close);
    //设置字符集
    std::string charset = getParamValue<std::string>(params,"charset","UTF8");
    mysql_options(mysql,MYSQL_SET_CHARSET_NAME,charset.c_str());

    int port = getParamValue<int>(params,"port");
    std::string host = getParamValue<std::string>(params,"host");
    std::string user = getParamValue<std::string>(params,"user");
    std::string passwd = getParamValue<std::string>(params,"passwd");
    std::string dbname = getParamValue<std::string>(params,"dbname");

    auto rt = mysql_real_connect(mysql,host.c_str(),user.c_str(),
				passwd.c_str(),dbname.c_str(),
				port,nullptr,0);
    if(rt == nullptr)
      {
	LOG_ERROR(s_logger) << "mysql_real_connect("<<host<<" , "
			    <<user<<" , "<<passwd<<") error : "
			    <<mysql_error(mysql);
	mysql_close(mysql);
	return nullptr;
      }
    return mysql;
  }

  MySQLConn::MySQLConn(const std::map<std::string,std::string>& params)
    :m_mysql(nullptr),
     m_params(params),
     m_lastUsedTime(0),
     m_name("root")
  { 
  }

  MySQLConn::~MySQLConn()
  {
  }
  
  MySQLConn::ptr MySQLConn::Create(const std::string& name)
  {
    auto mgr = Singleton<MySQLManager>::getInstance();
    return mgr->getConn(name);
  }
  
  bool MySQLConn::connect()
  {
    if(m_mysql != nullptr && m_hasError == false) return true;
    m_mysql = mysql_init(m_params,10000);
    if(!m_mysql)
      {
	m_hasError = true;
	return false;
      }
    m_hasError = false;
    m_connect = true;
    return true;
  }

  void MySQLConn::close()
  {
    if(!m_connect)
      {
	mysql_close(m_mysql);
	m_connect = true;
      }
  }

  bool MySQLConn::ping()
  {
    if(m_connect == false || m_mysql == nullptr) return false;
    int rt = mysql_ping(m_mysql);
    if(rt)
      {
	m_hasError = true;
	return false;
      }
    m_hasError = false;
    return true;
  }

  MySQLCommand::~MySQLCommand()
  {
    //如果不是事务则归还给
    if(m_close)
      {
	auto mgr = MySQLManager::Mgr::getInstance();
	auto conn = std::dynamic_pointer_cast<MySQLConn>(m_conn);
	ASSERT( conn != nullptr);
	mgr->addConn(conn->getName(),conn);
      }
  }

  IDBRes::ptr MySQLCommand::command(const char* fmt,...)
  {
    va_list ap;
    va_start(ap,fmt);
    auto rt = command(fmt,ap);
    va_end(ap);
    return rt;
  }

  IDBRes::ptr MySQLCommand::command(const char* fmt,va_list ap)
  {
    char* buf = nullptr;
    int len = vasprintf(&buf,fmt,ap);
    std::string sql = std::string(buf,len);
    free(buf);
    return command(sql);
  }
  IDBRes::ptr MySQLCommand::command(const std::string& sql)
  {
    if(m_conn == nullptr) return nullptr;
    MySQLConn::ptr conn = MySQLUtils::ptrTypeCast<IDBConn,MySQLConn>(m_conn);
    int rt = mysql_real_query(conn->get(),sql.c_str(),sql.size());
    if(rt)
      {
	m_hasError = true;
	LOG_ERROR(s_logger) << "sql : " <<sql << " failed,error :" << getError();
	throw std::logic_error(sql + std::string(",execute failed"));
      }
    m_hasError = false;
    m_cmdStr = sql;
    MYSQL_RES* tmp = mysql_store_result(conn->get());
    MySQLRes::ptr res(new MySQLRes(tmp,getError(),getErrno()));
    if(m_conn == nullptr) m_row = 0;
    else
      {
	auto conn = MySQLUtils::ptrTypeCast<IDBConn,MySQLConn>(m_conn);
	m_row  = mysql_affected_rows(conn->get());
      }
    return res;
  }

  int MySQLCommand::getAffectedRow()
  {
    return m_row;
  }

  MySQLRes::MySQLRes(MYSQL_RES* res,
		     const std::string& err,
		     int eno)
    :IDBRes(err,eno)
  {
    if(res == nullptr)
      {
	m_data = nullptr;
	m_rowCnt = m_colCnt = 0;
	m_fields = nullptr;
      }
    else
      {
	m_data.reset(res,mysql_free_result);
	m_rowCnt = mysql_num_rows(m_data.get());
	m_colCnt = mysql_num_fields(m_data.get());
	
	m_fields = mysql_fetch_fields(m_data.get());
	
	if(m_fields == nullptr) throw std::logic_error(err);
      }
  }
  
  bool MySQLRes::foreach(CbType cb)
  {
    if(m_data == nullptr) return false;
    int cols = getColumnCount();
    MYSQL_ROW row = nullptr;
    uint32_t id = 0;
    while(( row = mysql_fetch_row(m_data.get()) ))
      {
	m_curLength = mysql_fetch_lengths(m_data.get());
	if(cb(row,cols,id++,m_curLength) == false) return false;
      }
    return true;
  }

  int MySQLRes::getColumnBytes(int idx) const
  {
    if(m_curLength == nullptr || m_colCnt <= idx)
      return -1;
    return m_curLength[idx];
  }

  int MySQLRes::getColumnType(int idx) const
  {
    if( m_fields == nullptr
       ||m_colCnt <= idx)
      return -1;
    return m_fields[idx].type;
  }
  std::string MySQLRes::getColumnName(int idx) const
  {
    if(m_fields == nullptr
       ||m_colCnt <= idx)
      return "ERROR";
    return std::string(m_fields[idx].name);
  }

  bool MySQLRes::isNull(int idx) const
  {
    if(m_curLength == nullptr || m_colCnt <= idx)
      return true;
    return m_row[idx] == nullptr;
  }

  int8_t MySQLRes::getInt8(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return (int8_t)toInt64(m_row[idx]);
  }

  int16_t MySQLRes::getInt16(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return (int16_t)toInt64(m_row[idx]);
  }
  int32_t MySQLRes::getInt32(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return (int32_t)toInt64(m_row[idx]);
  }
  int64_t MySQLRes::getInt64(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return (int64_t)toInt64(m_row[idx]);
  }
  
  uint8_t  MySQLRes::getUint8 (int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return (uint8_t)toUint64(m_row[idx]);
  }
  uint16_t MySQLRes::getUint16(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return (uint16_t)toUint64(m_row[idx]);
  }
  uint32_t MySQLRes::getUint32(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return (uint32_t)toUint64(m_row[idx]);
  }
  uint64_t MySQLRes::getUint64(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return (uint64_t)toUint64(m_row[idx]);
  }
  
  float  MySQLRes::getFloat(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return (float)toDouble(m_row[idx]);
  }
  double MySQLRes::getDoube(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return (double)toDouble(m_row[idx]);
  }
  
  std::string MySQLRes::getString(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return std::string(m_row[idx]);
  }
  std::string MySQLRes::getBlob(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return std::string(m_row[idx]);
  }
  
  time_t MySQLRes::getTime(int idx) const
  {
    if( m_colCnt <= idx)
      throw std::out_of_range(std::to_string(idx) + " out of bound "
			      + std::to_string(m_colCnt));
    return str2Time(m_row[idx],"%Y-%m-%d %H:%M:%S");
  }

  bool MySQLRes::nextRow()
  {
    m_row = mysql_fetch_row(m_data.get());
    m_curLength = mysql_fetch_lengths(m_data.get());
    return m_row != nullptr;
  }


  MySQLStmt::ptr MySQLStmt::Create(MySQLConn::ptr conn,
				   const std::string& stmt)
  {
    MYSQL_STMT* st = mysql_stmt_init(conn->get());
    if(st == nullptr) return nullptr;
    int rt = mysql_stmt_prepare(st,stmt.c_str(),stmt.size());
    if(rt != 0)
      {
	LOG_ERROR(s_logger)<<"stmt prepare failed : " << stmt
			   <<" errno : "<<mysql_stmt_errno(st)
			   <<" error : "<<mysql_stmt_error(st);
	mysql_stmt_close(st);
	return nullptr;
      }
    //获取参数个数
    int count = mysql_stmt_param_count(st);
    MySQLStmt::ptr res(new MySQLStmt(conn,st));
    res->m_binds.resize(count);
    memset(&res->m_binds[0],0,sizeof(res->m_binds[0])*count);
    return res;
  }
  MySQLStmt::MySQLStmt(MySQLConn::ptr conn,
		       MYSQL_STMT* stmt)
    :m_stmt(stmt),
     m_conn(conn)
  {
  }

  MySQLStmt::~MySQLStmt()
  {
    if(m_stmt) mysql_stmt_close(m_stmt);

    for(auto& item : m_binds)
      {
	if(item.buffer != nullptr)
	  free(item.buffer);
      }
  }
  
  int MySQLStmt::bind(int idx,const int8_t& v)
  {
    return bindInt8(idx,v);
  }
  int MySQLStmt::bind(int idx,const int16_t& v)
  {
    return bindInt16(idx,v);
  }
  int MySQLStmt::bind(int idx,const int32_t& v)
  {
    return bindInt32(idx,v);
  }
  int MySQLStmt::bind(int idx,const int64_t& v)
  {
    return bindInt64(idx,v);
  }

  int MySQLStmt::bind(int idx,const uint8_t& v)
  {
    return bindUint8(idx,v);
  }
  int MySQLStmt::bind(int idx,const uint16_t& v)
  {
    return bindUint16(idx,v);
  }
  int MySQLStmt::bind(int idx,const uint32_t& v)
  {
    return bindUint32(idx,v);
  }
  int MySQLStmt::bind(int idx,const uint64_t& v)
  {
    return bindUint64(idx,v);
  }

  int MySQLStmt::bind(int idx,const std::string& v)
  {
    return bindString(idx,v);
  }
  int MySQLStmt::bind(int idx,const char* v)
  {
    return bindString(idx,v);
  }
  int MySQLStmt::bind(int idx,const char* v,int len)
  {
    return bindBlob(idx,v,len);
  }
  
  int MySQLStmt::bind(int idx)
  {
    return bindNull(idx);
  }
  
#define ALLOC_AND_COPY(ptr,size)				\
  if(m_binds[idx].buffer == nullptr)				\
    {								\
      m_binds[idx].buffer = malloc(size);			\
      if(m_binds[idx].buffer == nullptr)			\
	throw std::logic_error("out of memory");		\
    }								\
  else if(m_binds[idx].buffer_length < size)			\
    {								\
      free(m_binds[idx].buffer);				\
      m_binds[idx].buffer = malloc(size);			\
      if(m_binds[idx].buffer == nullptr)			\
	throw std::logic_error("out of memory");		\
    }								\
  memcpy(m_binds[idx].buffer,ptr,size);

#define BIND_VALUE(type,ptr,size,unsign)	\
  m_binds[idx].buffer_type = type;		\
  ALLOC_AND_COPY(ptr,size);			\
  m_binds[idx].is_unsigned = unsign;		\
  m_binds[idx].buffer_length = size;		
  
  int  MySQLStmt::bindInt8(int idx,int8_t v)
  {
    BIND_VALUE(MYSQL_TYPE_TINY,&v,sizeof(v),false);
    return 0;
  }
  int  MySQLStmt::bindInt16(int idx,int16_t v)
  {
    BIND_VALUE(MYSQL_TYPE_SHORT,&v,sizeof(v),false);
    return 0;
  }
  int  MySQLStmt::bindInt32(int idx,int32_t v)
  {
    BIND_VALUE(MYSQL_TYPE_LONG,&v,sizeof(v),false);
    return 0;
  }
  int  MySQLStmt::bindInt64(int idx,int64_t v)
  {
    BIND_VALUE(MYSQL_TYPE_LONGLONG,&v,sizeof(v),false);
    return 0;
  }
  
  int  MySQLStmt::bindUint8(int idx,uint8_t v)
  {
    BIND_VALUE(MYSQL_TYPE_TINY,&v,sizeof(v),true);
    return 0;
  }
  int  MySQLStmt::bindUint16(int idx,uint16_t v)
  {
    BIND_VALUE(MYSQL_TYPE_SHORT,&v,sizeof(v),true);
    return 0;
  }
  int  MySQLStmt::bindUint32(int idx,uint32_t v)
  {
    BIND_VALUE(MYSQL_TYPE_LONG,&v,sizeof(v),true);
    return 0;
  }
  int  MySQLStmt::bindUint64(int idx,uint64_t v)
  {
    BIND_VALUE(MYSQL_TYPE_LONGLONG,&v,sizeof(v),true);
    return 0;
  }
  int  MySQLStmt::bindFloat(int idx,float v)
  {
    BIND_VALUE(MYSQL_TYPE_FLOAT,&v,sizeof(v),false);
    return 0;
  }
  int  MySQLStmt::bindDouble(int idx,double v)
  {
    BIND_VALUE(MYSQL_TYPE_DOUBLE,&v,sizeof(v),false);
    return 0;
  }
  
  int  MySQLStmt::bindString(int idx,const std::string& v)
  {
    BIND_VALUE(MYSQL_TYPE_STRING,v.c_str(),v.size(),false);
    return 0;
  }

  int  MySQLStmt::bindString(int idx,const char* v)
  {
    BIND_VALUE(MYSQL_TYPE_STRING,v,strlen(v),false);
    return 0;
  }
  int  MySQLStmt::bindBlob(int idx,const char* v,uint64_t size)
  {
    BIND_VALUE(MYSQL_TYPE_BLOB,v,size,false);
    return 0;
  }

  int  MySQLStmt::bindBlob(int idx,const std::string& v)
  {
    BIND_VALUE(MYSQL_TYPE_BLOB,v.c_str(),v.size(),false);
    return 0;
  }
  int  MySQLStmt::bindTime(int idx,time_t v)
  {
    std::string timeStr = time2Str(v,"%Y-%m-%d %H:%M:%S");
    return bindString(idx,timeStr);
  }
  int  MySQLStmt::bindNull(int idx)
  {
    m_binds[idx].buffer_type = MYSQL_TYPE_NULL;
    return 0;
  }

  int64_t MySQLStmt::getLastInsertId()
  {
    return mysql_stmt_insert_id(m_stmt);
  }
  int MySQLStmt::execute()
  {
    mysql_stmt_bind_param(m_stmt,&m_binds[0]);
    return mysql_stmt_execute(m_stmt);
  }

  IDBRes::ptr MySQLStmt::command()
  {
    mysql_stmt_bind_param(m_stmt,&m_binds[0]);
    return MySQLStmtRes::Create(shared_from_this());
  }

  MySQLStmtRes::ptr MySQLStmtRes::Create(std::shared_ptr<MySQLStmt> stmt)
  {
    int eno = mysql_stmt_errno(stmt->get());
    std::string err = mysql_stmt_error(stmt->get());
    MySQLStmtRes::ptr res(new MySQLStmtRes(stmt,err,eno));
    if(eno != 0)
      {
	return MySQLStmtRes::ptr(new MySQLStmtRes(stmt,mysql_stmt_error(stmt->get())
						  ,mysql_stmt_errno(stmt->get())));
      }
    //获取元数据
    MYSQL_RES* rs = mysql_stmt_result_metadata(stmt->get());
    if(rs == nullptr)
      {
	return MySQLStmtRes::ptr(new MySQLStmtRes(stmt,mysql_stmt_error(stmt->get())
						  ,mysql_stmt_errno(stmt->get())));
      }

    auto&  binds = res -> m_binds;
    auto&  datas = res -> m_datas;
    auto*& field = res->m_fields;

    int cnt = mysql_num_fields(rs);
    field = mysql_fetch_fields(rs);
    
    binds.resize(cnt);
    memset(&binds[0],0,sizeof(binds[0])*cnt);
    datas.resize(cnt);
    
    for(int i = 0;i<cnt;i++)
      {
	datas[i].type = field[i].type;
#define XX(enum_type,type)			\
	case enum_type :			\
	  datas[i].alloc(sizeof(type));		\
	  break

	switch(field[i].type)
	  {
	    XX(MYSQL_TYPE_TINY,int8_t);
	    XX(MYSQL_TYPE_SHORT,int16_t);
	    XX(MYSQL_TYPE_LONG,int32_t);
	    XX(MYSQL_TYPE_LONGLONG,int64_t);
	    XX(MYSQL_TYPE_FLOAT,float);
	    XX(MYSQL_TYPE_DOUBLE,double);
	    XX(MYSQL_TYPE_TIMESTAMP,MYSQL_TIME);
	    XX(MYSQL_TYPE_DATETIME,MYSQL_TIME);
	    XX(MYSQL_TYPE_DATE,MYSQL_TIME);
	    XX(MYSQL_TYPE_TIME,MYSQL_TIME);
	  default:
	    datas[i].alloc(field[i].length);
	  }
#undef XX

	binds[i].buffer_type = datas[i].type;
	binds[i].buffer = datas[i].buffer;
	binds[i].buffer_length = datas[i].buffer_length;
	binds[i].length = &datas[i].length;
	binds[i].is_null = &datas[i].is_null;
	binds[i].error = &datas[i].error;
      }
    //绑定结果
    int rt = mysql_stmt_bind_result(stmt->get(),&binds[0]);
    if(rt != 0)
      {
	return MySQLStmtRes::ptr(new MySQLStmtRes(stmt,mysql_stmt_error(stmt->get())
						  ,mysql_stmt_errno(stmt->get())));
      }
    //执行预处理
    rt = stmt->execute();
    if(rt != 0)
      {
	return MySQLStmtRes::ptr(new MySQLStmtRes(stmt,mysql_stmt_error(stmt->get())
						  ,mysql_stmt_errno(stmt->get())));
      }

    rt = mysql_stmt_store_result(stmt->get());
    if(rt != 0)
      {
	return MySQLStmtRes::ptr(new MySQLStmtRes(stmt,mysql_stmt_error(stmt->get())
						  ,mysql_stmt_errno(stmt->get())));
      }
    res->m_rowCnt = mysql_stmt_num_rows(stmt->get());
    
    return res;
  }
  
  MySQLStmtRes::MySQLStmtRes(MySQLStmt::ptr stmt,
			     const std::string& err,
			     int eno)
    :IDBRes(err,eno),
     m_stmt(stmt)
  {
  }

  int MySQLStmtRes::getRowCount() const
  {
    return m_rowCnt;
  }

  int MySQLStmtRes::getColumnCount() const
  {
    return m_binds.size();
  }

  int MySQLStmtRes::getColumnBytes(int idx) const
  {
    if(idx >= getColumnCount())
      throw std::logic_error("getColumnBytes("+std::to_string(idx)+
			     ") out of bound("+std::to_string(getColumnCount()));
    return m_datas[idx].length;
  }

  int MySQLStmtRes::getColumnType(int idx) const
  {
    if(idx >= getColumnCount())
      throw std::logic_error("getColumnType("+std::to_string(idx)+
			     ") out of bound("+std::to_string(getColumnCount()));
    return m_datas[idx].type;
  }

  std::string MySQLStmtRes::getColumnName(int idx) const
  {
    if(idx >= getColumnCount() || m_fields == nullptr)
      throw std::logic_error("getColumnName("+std::to_string(idx)+
			     ") out of bound("+std::to_string(getColumnCount()));
    return std::string(m_fields[idx].name);
  }

  bool MySQLStmtRes::isNull(int idx) const
  {
    if(idx >= getColumnCount() )
      throw std::logic_error("isNull("+std::to_string(idx)+
			     ") out of bound("+std::to_string(getColumnCount()));
    return m_datas[idx].is_null;
  }

  int8_t  MySQLStmtRes::getInt8 (int idx) const
  {
    return *(int8_t*)m_datas[idx].buffer;
  }

  int16_t MySQLStmtRes::getInt16(int idx) const
  {
    return *(int16_t*)m_datas[idx].buffer;
  }
  int32_t MySQLStmtRes::getInt32(int idx) const
  {
    return *(int32_t*)m_datas[idx].buffer;
  }
  
  int64_t MySQLStmtRes::getInt64(int idx) const
  {
    return *(int64_t*)m_datas[idx].buffer;
  }
    
  uint8_t  MySQLStmtRes::getUint8 (int idx) const
  {
    return *(uint8_t*)m_datas[idx].buffer;
  }
  uint16_t MySQLStmtRes::getUint16(int idx) const
  {
    return *(uint16_t*)m_datas[idx].buffer;
  }
  uint32_t MySQLStmtRes::getUint32(int idx) const
  {
    return *(uint32_t*)m_datas[idx].buffer;
  }
  uint64_t MySQLStmtRes::getUint64(int idx) const
  {
    return *(uint64_t*)m_datas[idx].buffer;
  }
    
  float  MySQLStmtRes::getFloat(int idx) const
  {
    return *(float*)m_datas[idx].buffer;
  }
  double MySQLStmtRes::getDoube(int idx) const
  {
    return *(double*)m_datas[idx].buffer;
  }

  std::string MySQLStmtRes::getString(int idx) const
  {
    return std::string(m_datas[idx].buffer,m_datas[idx].length);
  }
  std::string MySQLStmtRes::getBlob(int idx) const
  {
    return std::string(m_datas[idx].buffer,m_datas[idx].length);
  }
  
  time_t MySQLStmtRes::getTime(int idx) const
  {
    return MySQLUtils::fromMySQLTime(*(MYSQL_TIME*)m_datas[idx].buffer);
  }

  bool MySQLStmtRes::nextRow()
  {
    return !mysql_stmt_fetch(m_stmt->get());
  }

  MySQLManager::MySQLManager()
    :m_maxConns(s_max_conns->getVar())
    ,m_dbDefine(s_mysql_define->getVar())
  {
    if(m_maxConns == 0) m_maxConns = 30;
  }

  MySQLConn::ptr MySQLManager::getConn(const std::string& name,
				       bool newName)
  {

    MySQLConn::ptr conn;
    
    {
      LockGuard lg(m_mx);
      
      auto it = m_conns.find(name);
      //
      if(it == m_conns.end())
	{
	  if(newName == false)
	    return nullptr;
	  m_conns[name] = std::deque<MySQLConn::ptr>();
	  conn.reset(new MySQLConn(m_dbDefine));
	}
      else if(it->second.empty() == true)
	{
	  conn.reset(new MySQLConn(m_dbDefine));
	  conn->m_name = name;
	}
      else
	{
	  std::deque<MySQLConn::ptr>& deq = it->second;
	  conn = deq.front();
	  deq.pop_front();
	}
    }
    //end of locking

    //make sure the connection is available
    if(conn->connect() == false)
      {
	LOG_ERROR(s_logger) << "connect failed";
	return nullptr;
      }
    if(conn->ping() == false)
      {
	LOG_ERROR(s_logger) << "ping failed";
	return nullptr;
      }
    return conn;
  }

  bool MySQLManager::addConn(const std::string& name,MySQLConn::ptr conn,bool newName)
  {
    if(conn == nullptr) return false;
    LockGuard lg(m_mx);
    
    auto it = m_conns.find(name);
    if(it == m_conns.end())
      {
	if(newName == true)
	  {
	    m_conns[name] = std::deque<MySQLConn::ptr>({conn});
	    return true;
	  }
	return false;
      }
    std::deque<MySQLConn::ptr>& deq = it->second;
    //超过上限
    if(deq.size() > m_maxConns)
      {
	//MySQLConn析构函数不再担任关闭连接的功能
	conn->close();
	return false;
      }
    deq.push_back(conn);
    return true;
  }

  bool MySQLTranscation::begin()
  {
    try
      {
	command("BEGIN");
	m_isFinished = false;
      }
    catch(std::exception& e)
      {
	LOG_ERROR(s_logger) << e.what();
	return false;
      }
    return true;
  }
  
  bool MySQLTranscation::commit()
  {
    if(m_isFinished == true) return false;
    try
      {
	command("COMMIT");
	m_isFinished = true;
      }
    catch(std::exception& e)
      {
	LOG_ERROR(s_logger) << e.what();
	return false;
      }
    return true;
  }
  bool MySQLTranscation::rollback()
  {
    if(m_isFinished == true) return false;
    try
      {
	command("ROLLBACK");
	m_isFinished = true;
      }
    catch(std::exception& e)
      {
	LOG_ERROR(s_logger) << e.what();
	return false;
      }
    return true;
  }

  MySQLTranscation::ptr MySQLTranscation::Create(const std::string& name)
  {
    auto mgr = MySQLManager::Mgr::getInstance();
    auto conn = mgr->getConn(name);

    return MySQLTranscation::ptr(new MySQLTranscation(conn));
  }
}
}
