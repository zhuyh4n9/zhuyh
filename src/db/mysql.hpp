#pragma once

#include<mysql/mysql.h>
#include<functional>
#include"db.hpp"
#include"../latch/lock.hpp"

namespace zhuyh
{
namespace db
{
  class MySQLManager;
  class MySQLStmtRes;
  
  class MySQLUtils
  {
    //MYSQL_TIME -> time_t
    static time_t fromMySQLTime(const MYSQL_TIME& mt);
    //time_t -> MySQL_TIME
    static MYSQL_TIME toMySQLTime(time_t t);
  };

  class MySQLConnection : public IDBConnection
  {
  public:
    typedef std::shared_ptr<MySQLConnection> ptr;
  private:
    void mysql_init();
  public:
    bool connect();
    void close();
    bool ping();
    MYSQL* get() { return m_mysql;}
  private:
    MYSQL* m_mysql;
  };

  class MySQLStmt : public IDBStmt
  {
  public:
    typedef std::shared_ptr<MySQLStmt> ptr;
    static MySQLStmt::ptr Create(MySQLConnection::ptr conn,
				 const std::string& stmt);
    ~MySQLStmt();
  private:
    MySQLStmt(MySQLConnection::ptr conn,
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
    MySQLConnection::ptr m_conn;
    std::vector<MYSQL_BIND> m_binds;
  };
  
  class MySQLRes : public IDBRes
  {
  public:
    typedef std::shared_ptr<MySQLRes> ptr;

    typedef std::function<bool(MYSQ_ROW row,
				 int field_count,
				 int row_id)> CbType;
    MySQLRes(MySQLRes* res,int err,const char* errstr);

    std::shared_ptr<MYSQL_RES>
    getData() { return m_res; }

    //对每一行执行cb
    bool foreach(CbType cb);
    
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
    //当前行每一列长度
    uint32_t* m_curLengthe;
    //当前行
    MYSQL_ROW m_row;
    //结果集
    std::shared_ptr<MYSQL_RES> m_data;
  };

  //绑定格式的结果集
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
    using Map = std::map<std::string,std::list<MySQLConnection::ptr> >;
    MySQLConnection::ptr getConn(const std::string& conn);
    bool addConn(const std::string& name,MySQLConnection::ptr conn);
    
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
}
}
