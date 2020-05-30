#pragma once

#include<mysql/mysql.h>
#include<functional>
#include"db.hpp"
#include"../latch/lock.hpp"
#include<map>
#include"../Singleton.hpp"
#include<vector>
#include<unordered_map>
#include<deque>
#include"../macro.hpp"
#include"../logs.hpp"
namespace zhuyh
{
namespace db
{
  class MySQLConnGuard;
  class MySQLManager;
  class MySQLStmt;
  class MySQLStmtRes;
  class MySQLRes;
  class MySQLCommand;
  class MySQLTranscation;
  
  class MySQLConn : public IDBConn,
		    public std::enable_shared_from_this<MySQLConn>
  {
  public:
    typedef std::shared_ptr<MySQLConn> ptr;
    friend class MySQLConnGuard;
    friend class MySQLManager;
  private:
    MySQLConn(const std::map<std::string,std::string>& params);
  public:
    //从连接池获取一个连接
    static MySQLConn::ptr Create(const std::string& name);
    
    ~MySQLConn();
    bool connect() override;
    void close() override;
    bool ping() override;
    MYSQL* get() { return m_mysql;}
    const std::string getName() const override { return m_name;}
    int getErrno() const override
    {
      if(m_mysql == nullptr)
	{
	  return -1;
	}
      return mysql_errno(m_mysql);
    }
    std::string getError() const override
    {
      if(m_mysql == nullptr)
	{
	  return std::string("connection failure");
	}
      return std::string(mysql_error(m_mysql));
    }
  private:
    MYSQL* m_mysql = nullptr;
    std::map<std::string,std::string> m_params;
    uint64_t m_lastUsedTime = 0;
    std::string m_name;
  };

  //只可以通过MySQLCommand创获取
  class MySQLRes : public IDBRes
  {
  public:
    friend MySQLCommand;
    typedef std::shared_ptr<MySQLRes> ptr;
    typedef std::function<bool(MYSQL_ROW row,
			       int cols,int id,
			       unsigned long* len)> CbType;
  private:
    MySQLRes(MYSQL_RES* res,const std::string& err,int eno);
  public:
    std::shared_ptr<MYSQL_RES> getData() { return m_data; }

    //对每一行执行cb
    bool foreach(CbType cb);
    
    int getRowCount() const override
    {
      return m_rowCnt;
    }
    int getColumnCount() const override
    {
      return m_colCnt;
    }
    //当前第k列字节数
    int getColumnBytes(int idx) const override;
    
    //当前第k列类型
    int getColumnType(int idx) const override;
    //获取列名
    std::string getColumnName(int idx) const override;

    //第k列是否为null
    bool isNull(int idx) const override;
    //获取各种类型
    int8_t  getInt8 (int idx) const override;
    int16_t getInt16(int idx) const override;
    int32_t getInt32(int idx) const override;
    int64_t getInt64(int idx) const override;
    
    uint8_t  getUint8 (int idx) const override;
    uint16_t getUint16(int idx) const override;
    uint32_t getUint32(int idx) const override;
    uint64_t getUint64(int idx) const override;
    
    float  getFloat(int idx) const override;
    double getDoube(int idx) const override;

    std::string getString(int idx) const override;
    std::string getBlob(int idx) const override;
    
    time_t getTime(int idx) const override;
    //获取下一个
    bool nextRow() override;
  private:
    int64_t  toInt64(const char* str) const
    {
      if(str == nullptr) return 0;
      return atoll(str);
    }
    uint64_t toUint64(const char* str) const
    {
      if(str == nullptr) return 0;
      return strtoull(str,nullptr,10);
    }
    double toDouble(const char* str) const
    {
      if(str == nullptr) return 0.0;
      return atof(str);
    }
  private:
    int m_rowCnt;
    int m_colCnt;
    //当前行每一列长度
    unsigned long* m_curLength = 0;
    //当前行
    MYSQL_ROW m_row;
    //结果集
    MYSQL_FIELD* m_fields;
    std::shared_ptr<MYSQL_RES> m_data;
  };

  
  class MySQLCommand : public IDBCommand
  {
  public:
    friend class MySQLTranscation;
    typedef std::shared_ptr<MySQLCommand> ptr;
    
    MySQLCommand(MySQLConn::ptr conn,bool close = false)
      :IDBCommand(conn),
       m_close(close){}
    ~MySQLCommand();
    IDBRes::ptr command(const std::string& sql) override final;
    IDBRes::ptr command(const char* fmt,va_list ap) override final;
    IDBRes::ptr command(const char* fmt,...) override final;

    int getAffectedRow() override;
    
    bool getClose() { return m_close;}
    void setClose(bool close) { m_close = close; }

    template<class... Args>
    int executeStmtStr(const std::string& str,Args&&... args);

    template<class... Args>
    std::shared_ptr<MySQLStmtRes> commandStmtStr(const std::string& str,Args&&... args);
  private:
    //是否是事务
    bool m_close = false;
    int m_row = 0;
  };

  class MySQLStmtCommand
  {
  public:
    typedef std::shared_ptr<MySQLStmtCommand> ptr;
    MySQLStmtCommand(std::shared_ptr<MySQLStmt> stmt)
      :m_stmt(stmt) {
    }
    template<class... Args>
    std::shared_ptr<MySQLStmtRes> command(Args&&... args);
    
    template<class... Args>
    int execute(Args&&... args);

    inline int getErrno() const;
    inline std::string getError() const;

    int getAffectedRow() const  { return m_row;}
  private:
    int m_row;
    std::shared_ptr<MySQLStmt> m_stmt;
  };
  
  class MySQLStmt : public IDBStmt,
		    public std::enable_shared_from_this<MySQLStmt>
  {
  public:
    typedef std::shared_ptr<MySQLStmt> ptr;
    static MySQLStmt::ptr Create(MySQLConn::ptr conn,
				 const std::string& stmt);
    ~MySQLStmt();
  private:
    MySQLStmt(MySQLConn::ptr conn,
	      MYSQL_STMT* stmt);
  public:
    
    int bind(int idx,const int8_t& v);
    int bind(int idx,const int16_t& v);
    int bind(int idx,const int32_t& v);
    int bind(int idx,const int64_t& v);

    int bind(int idx,const uint8_t& v);
    int bind(int idx,const uint16_t& v);
    int bind(int idx,const uint32_t& v);
    int bind(int idx,const uint64_t& v);

    int bind(int idx,const float& v);
    int bind(int idx,const double& v);

    int bind(int idx,const std::string& v);
    
    int bind(int idx,const char* v);
    int bind(int idx,const char* v,int len);

    int bind(int idx);
    
    int  bindInt8(int idx,int8_t v) override;
    int  bindInt16(int idx,int16_t v) override;
    int  bindInt32(int idx,int32_t v) override;
    int  bindInt64(int idx,int64_t v) override;
    int  bindUint8(int idx,uint8_t v) override;
    int  bindUint16(int idx,uint16_t v) override;
    int  bindUint32(int idx,uint32_t v) override;
    int  bindUint64(int idx,uint64_t v) override;
    int  bindFloat(int idx,float v) override;
    int  bindDouble(int idx,double v) override;
    int  bindString(int idx,const std::string& v) override;
    int  bindString(int idx,const char* v) override;
    int  bindBlob(int idx,const std::string& v) override;
    int  bindBlob(int idx,const char* v,uint64_t size) override;
    int  bindTime(int idx,time_t v) override;
    int  bindNull(int idx) override;
    
    int execute() override;
    int64_t getLastInsertId() override;
    
    std::shared_ptr<IDBRes> command();
    
    MYSQL_STMT* get() const
    {
      return m_stmt;
    }

    int getErrno() const override
    {
      if(m_stmt == nullptr)
	{
	  return -1;
	}
      return mysql_stmt_errno(m_stmt);
    }
    std::string getError() const override
    {
      if(m_stmt == nullptr)
	{
	  return std::string("UNKNOWN ERROR");
	}
      return std::string(mysql_stmt_error(m_stmt));
    }

    MySQLConn::ptr getConn()
    {
      return m_conn;
    }
  private:
    MYSQL_STMT* m_stmt;
    MySQLConn::ptr m_conn;
    std::vector<MYSQL_BIND> m_binds;
    int m_rowCnt;
  };


  class MySQLStmtRes : public IDBRes
  {
  public:
    typedef std::shared_ptr<MySQLStmtRes> ptr;
    
    static MySQLStmtRes::ptr Create(std::shared_ptr<MySQLStmt> stmt);
    ~MySQLStmtRes() {
      if(!m_errno)
	{
	  mysql_stmt_free_result(m_stmt->get());
	}
    }
  private:
    MySQLStmtRes(std::shared_ptr<MySQLStmt> stmt,
		 const std::string& err,
		 int eno);
    struct Data
    {
      ~Data()
      {
	if(buffer != nullptr) free(buffer);
      }
      void alloc(size_t size)
      {
	if(buffer)
	  free(buffer);
	buffer = (char*)malloc(size);
	if(buffer == nullptr)
	  {
	    throw std::logic_error("out of memory");
	  }
	memset(buffer,0,sizeof(char)*size);
	length = buffer_length = size;
      }
      
      my_bool is_null = false;
      my_bool error = false;
      enum_field_types type{(enum_field_types)0};
      unsigned long length = 0;
      //缓冲区长度
      int32_t buffer_length = 0;
      char* buffer = nullptr;
    };
  public:
    int getRowCount() const override;
    int getColumnCount() const override;
    //当前第k列字节数
    int getColumnBytes(int idx) const override;
    //当前第k列类型
    int getColumnType(int idx) const override;
    //获取列名
    std::string getColumnName(int idx) const override;
    //第k列是否为null
    bool isNull(int idx) const override;
    //获取各种类型
    int8_t  getInt8 (int idx) const override;
    int16_t getInt16(int idx) const override;
    int32_t getInt32(int idx) const override;
    int64_t getInt64(int idx) const override;
    
    uint8_t  getUint8 (int idx) const override;
    uint16_t getUint16(int idx) const override;
    uint32_t getUint32(int idx) const override;
    uint64_t getUint64(int idx) const override;
    
    float  getFloat(int idx) const override;
    double getDoube(int idx) const override;

    std::string getString(int idx) const override;
    std::string getBlob(int idx) const override;
    
    time_t getTime(int idx) const override;

    bool nextRow() override;
  private:
    std::shared_ptr<MySQLStmt> m_stmt;
    std::vector<MYSQL_BIND> m_binds;
    std::vector<Data> m_datas;
    MYSQL_FIELD* m_fields;
    int m_rowCnt;
  };
  
  //Mysql管理器
  class MySQLManager
  {
  public:
    friend class Singleton<MySQLManager>;
    friend class MySQLConn;
    typedef Singleton<MySQLManager> Mgr;
    typedef std::shared_ptr<MySQLManager> ptr;
    using MutexType = Mutex;
    using Map = std::unordered_map<std::string,std::deque<MySQLConn::ptr> >;

    MySQLConn::ptr getConn(const std::string& name,
			   bool newName = true);
    bool addConn(const std::string& name,MySQLConn::ptr conn,bool newName = true);
    
    IDBRes::ptr command(const std::string& name,const char* fmt,...);
    IDBRes::ptr command(const std::string& name,const char* fmt,va_list ap);
    IDBRes::ptr command(const std::string& name,const char* sql);

    void setDefine(const std::map<std::string,std::string>& def){
      m_dbDefine = def;
    }
  private:
    MySQLManager(const MySQLManager&) = delete;
    MySQLManager& operator=(const MySQLManager&) = delete;
    //单例
    MySQLManager();
  private:
    uint32_t m_maxConns;
    MutexType m_mx;
    Map m_conns;
    std::map<std::string,std::string> m_dbDefine;
  };

  class MySQLTranscation : public IDBTranscation,
			   public MySQLCommand
  {
  public:
    typedef std::shared_ptr<MySQLTranscation> ptr;
    friend class MySQLCommand;
    static MySQLTranscation::ptr Create(const std::string& name);
    bool begin() override;
    bool commit() override;
    bool rollback() override;
  private:
    MySQLTranscation(MySQLConn::ptr conn)
      :MySQLCommand(conn,false),
       m_isFinished(true)
    {
    }
  private:
    bool m_isFinished = false;
  };
  
  class MySQLConnGuard 
  {
  public:
    MySQLConnGuard(const MySQLConn::ptr& conn)
      :m_conn(conn){}
    ~MySQLConnGuard()
    {
      auto mgr = MySQLManager::Mgr::getInstance();
      mgr->addConn(m_conn->m_name,m_conn);
    }
  private:
    MySQLConn::ptr m_conn;
  };


  
  template<class... Args>
  MySQLStmtRes::ptr MySQLCommand::commandStmtStr(const std::string& str,Args&&... args)
  {
    int idx = 0;
    auto conn = std::dynamic_pointer_cast<MySQLConn>(m_conn);
    ASSERT(conn != nullptr);
    auto stmt = MySQLStmt::Create(conn,str);
    if(stmt == nullptr) return nullptr;
    (stmt->bind(idx++,args),...);
    auto res = stmt->command();

    m_row  = mysql_stmt_affected_rows(stmt->get());
    return std::dynamic_pointer_cast<MySQLStmtRes>(res);
  }
  
  template<class... Args>
  int MySQLCommand::executeStmtStr(const std::string& str,
				Args&&... args)
  {
    int idx = 0;
    if(m_conn == nullptr) return -1;
    auto conn = std::dynamic_pointer_cast<MySQLConn>(m_conn);
    ASSERT(conn != nullptr);
    auto stmt = MySQLStmt::Create(conn,str);
    if(stmt == nullptr) return nullptr;
    
    (stmt->bind(idx++,args),...);
    int rt = stmt->execute();
    m_row  = mysql_stmt_affected_rows(stmt->get());
    return rt;
  }
  
  template<class... Args>
  std::shared_ptr<MySQLStmtRes> MySQLStmtCommand::command(Args&&... args)
  {
    int idx = 0;
    (m_stmt->bind(idx++,args),...);
    auto res = m_stmt->command();
    m_row  = mysql_stmt_affected_rows(m_stmt->get());
    return std::dynamic_pointer_cast<MySQLStmtRes>(res);
  }
  
  template<class... Args>
  int MySQLStmtCommand::execute(Args&&... args)
  {
    int idx = 0;
    (m_stmt->bind(idx++,args),...);
    int rt = m_stmt->execute();
    m_row  = mysql_stmt_affected_rows(m_stmt->get());
    return rt;
  }

  int MySQLStmtCommand::getErrno() const
  {
    return mysql_stmt_errno(m_stmt->get());
  }
  std::string MySQLStmtCommand::getError() const
  {
    return mysql_stmt_error(m_stmt->get());
  }
}
}
