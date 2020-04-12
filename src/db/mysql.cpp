#include"mysql.hpp"
#include<time.h>
#include"../logs.hpp"
#include"../util.hpp"
#include"../macro.hpp"

namespace zhuyh
{
namespace db
{
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
    mysql_options(mysql,MYSQL_SET_CHARSET_NAME,"UTF8");

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
     m_lastUsedTime(0)
  { 
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

  bool MySQLCommand::command(const char* fmt,...)
  {
    va_list ap;
    va_start(ap,fmt);
    int rt = command(fmt,ap);
    va_end(ap);
    return rt;
  }

  bool MySQLCommand::command(const char* fmt,va_list ap)
  {
    char* buf = nullptr;
    int len = vasprintf(&buf,fmt,ap);
    std::string sql = std::string(buf,len);
    free(buf);
    return command(sql);
  }
  bool MySQLCommand::command(const std::string& sql)
  {
    if(m_conn == nullptr) return false;
    MySQLConn::ptr conn = MySQLUtils::ptrTypeCast<IDBConn,MySQLConn>(m_conn);
    int rt = mysql_query(conn->get(),sql.c_str());
    if(rt)
      {
	m_hasError = true;
	LOG_ERROR(s_logger) << "sql : " <<sql << " failed,error :" << getError();
	return false;
      }
    m_hasError = false;
    m_cmdStr = sql;
    return true;
  }

  int MySQLCommand::getAffectedRow()
  {
    if(m_conn == nullptr) return 0;
    auto conn = MySQLUtils::ptrTypeCast<IDBConn,MySQLConn>(m_conn);
    return mysql_affected_rows(conn->get());
  }

  IDBRes::ptr MySQLCommand::getRes()
  {
    if(m_cmdStr.empty() || m_hasError) return nullptr;
    auto conn = MySQLUtils::ptrTypeCast<IDBConn,MySQLConn>(m_conn);
    MYSQL_RES* tmp = mysql_store_result(conn->get());
    MySQLRes::ptr res(new MySQLRes(tmp,getError(),getErrno()));
    return res;
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
	if(cb(row,cols,id++) == false) return false;
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

  
}
}
