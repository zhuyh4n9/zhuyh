#pragma once

#include<mysql/mysql.h>
#include<functional>
#include"db.hpp"
#include"../latch/lock.hpp"
#include<map>

namespace zhuyh
{
namespace db
{
  class MySQLManager;
  class MySQLStmtRes;
  class MySQLRes;
  class MySQLCommand;
  
  class MySQLConn : public IDBConn
  {
  public:
    typedef std::shared_ptr<MySQLConn> ptr;
  public:
    MySQLConn(const std::map<std::string,std::string>& params);
    bool connect() override;
    void close() override;
    bool ping() override;
    MYSQL* get() { return m_mysql;}
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
    bool nextRow()  override;
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
    MySQLCommand(MySQLConn::ptr conn)
      :IDBCommand(conn){}
    bool command(const std::string& sql) override;
    bool command(const char* fmt,va_list ap) override;
    bool command(const char* fmt,...) override;
    /*
    bool commandStmt(const std::string& sql) const override;
    bool commandStmt(const char* fmt,va_list ap) const override;
    bool commandStmt(const char* fmt,...) const override;
    */
    IDBRes::ptr getRes() override;
    //std::shared_ptr<MySQLStmtRes> getStmtRes() const override;
    
    int getAffectedRow() override;
  };
  
  /*
  class MySQLStmt : public IDBStmt
  {
  public:
    typedef std::shared_ptr<MySQLStmt> ptr;
    static MySQLStmt::ptr Create(MySQLConn::ptr conn,
				 const std::string& stmt);
    ~MySQLStmt();
  private:
    MySQLStmt(MySQLConn::ptr conn,
	      const std::string& stmt);
  public:
    int bind(int idx,const int_& v);
    int bind(int idx,const int_& v);
    int bind(int idx,const int_& v);
    int bind(int idx,const int_& v);

    int bind(int idx,const uint_& v);
    int bind(int idx,const uint_& v);
    int bind(int idx,const uint_& v);
    int bind(int idx,const uint_& v);
    int bind(int idx,const uint_& v);

    int bind(int idx,const float& v);
    int bind(int idx,const double& v);

    int bind(int idx,const std::string& v);
    
    int bind(int idx,const char* v);
    int bind(int idx,const void* v,int len);

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
    int  bindBlob(int idx,const std::string& v) override;
    int  bindTime(int idx,time_t v) override;
    int  bindNull(int idx) override;
    
    int execute() override;
    int getLastInsertId() override;
    std::shared_ptr<IDBRes> query() override;

    MYSQL_STMT* getRaw() const
    {
      return m_stmt;
    }
  private:
    MYSQL_STMT* m_stmt;
    MySQLConn::ptr m_conn;
    std::vector<MYSQL_BIND> m_binds;
  };

  class MySQLStmtRes : public IDBRes
  {
  public:
    typedef std::shared_ptr<MySQLStmtRes> ptr;

    static MySQLStmtRes::ptr Create(std::shared_ptr<MySQLStmt> stmt);
    ~MySQLStmtRes();
  private:
    MySQLStmtRes(std::shared_ptr<MySQLStmt> stmt,
		 int err,const char* strerr);
    struct Data
    {
      Data();
      ~Data();
      void alloc(size_t size);

      my_bool is_null;
      my_bool error;
      enum_field_types type;
      uint32_t len;
      int32_t data_len;
      char* data;
    };
  public:
    int getDataCount() const override;
    int getColumnCount() const override;
    //当前第k列字节数
    int getColumnBytes(int idx) const override;
    //当前第k列类型
    int getColumnType(int idx) const override;
    //获取列名
    std::string getColmnName(int idx) const override;
    //获取所有列名
    std::vector<std::string> getColumnNames() const override;

    //第k列是否为null
    bool isNull(int idx) override;
    //获取各种类型
    int8_t  getInt8 (int idx) const override;
    int16_t getInt16(int idx) const override;
    int32_t getInt32(int idx) const override;
    int64_t getInt64(int idx) const override;
    
    uint8_t  getInt8 (int idx) const override;
    uint16_t getInt16(int idx) const override;
    uint32_t getInt32(int idx) const override;
    uint64_t getInt64(int idx) const override;
    
    float  getFloat(int idx) const override;
    double getDoube(int idx) const override;

    std::string getString(int idx) const override;
    std::string getBlob(int idx) const override;
    
    time_t getTime(int idx) const override;
    //获取下一个
    bool next() const override;
  private:
    std::shared_ptr<MySQLStmt> m_stmt;
    std::vector<MYSQ_BIND> m_binds;
    std::vector<Data> m_datas;
  };
  
  //Mysql管理器
  class MySQLManager
  {
  public:
    friend class Singleton<MySQLManager>;
    typedef Singleton<MySQLManager> Mgr;
    typedef std::shared_ptr<MySQLManager> ptr;
    using MutexType = Mutex;
    
    //由于需要加锁，使用unordered_map来管理连接有可能会导致独占使用时间过长
    using Map = std::map<std::string,std::list<MySQLConn::ptr> >;
    MySQLConn::ptr getConn(const std::string& conn);
    bool addConn(const std::string& name,MySQLConn::ptr conn);

    int execute(const std::string& name,const char* fmt,...);
    int execute(const std::string& name,const char* fmt,va_list ap);
    int execute(const std::string& name,const char* sql);
    
    MySQLRes::ptr query(const std::string& name,const char* fmt,...);
    MySQLRes::ptr query(const std::string& name,const char* fmt,va_list ap);
    MySQLRes::ptr query(const std::string& name,const char* sql);
    bool init();
  private:
    MySQLManager(const MySQLManager&) = delete;
    MySQLManager& operator=(const MySQLManager&) = delete;
    //单例
    MySQLManager();
  private:
    MutexType m_mx;
    Map m_conns;
    uint64_t m_timeout;
    std::map<std::string,std::map<std::string,std::string>> m_dbDefine;
    
  };

  */  
}
}
