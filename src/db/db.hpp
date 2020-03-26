#pragma once

#include<memory>

namespace zhuyh
{
namespace db
{
  class IDBResult;
  class IDBStmt;
  class IDBConnection
  {
  public:
    typedef std::shared_ptr<IDBConnection> ptr;
    //通过连接池获取一个连接
    //static IDBConnection::ptr newConnection(IDBConnection::ptr factory);
    virtual ~IDBConnection()
    {
      if(m_connect) close();
    }
    void init()
    {
      if(!m_connect) connect();
    }
    bool isConnect() const
    {
      return m_connect;
    }
    
    bool isTranscation() { return m_transcation;}
    void setTranscation(bool v)  { m_transcation = v;}
    
    //获取错误信息
    int getErrno() const final { return m_errno;}
    const std::string& getStrerr() const final{ return m_strerr; }
  protected:
    //负责设置errno与strerr
    virtual void setError(int v) = 0;
  protected:
    virtual connect() = 0;
    virtual close() = 0;
  protected:
    int m_errno = 0;
    bool m_connect = 0;
    bool m_transcation = false;
    std::string m_strerr;
  };

  //执行SQL
  class IDBCommand
  {
  public:    
    typedef std::shared_ptr<IDBCommand> ptr;
    IDBQuery(IDBConnection::ptr conn,
	     std::shared_ptr<IDBStmt> stmt)
      :m_conn(conn),
       m_stmt(stmt)
    {}
    virtual ~IDBQuery() {} 
    //负责执行sql
    virtual bool command(const std::string& sql) const = 0;
    virtual bool command(const std::string& fmt,...) const = 0;
    //获取结果集
    std::shared_ptr<IDBResult> getRes() { return m_res;}
    std::shared_ptr<IDBStmt> getStmt() {  return m_stmt;}
  protected:
    //command的结果
    std::shared_ptr<IDBResult> m_res;
    std::shared_ptr<IDBStmt> m_stmt;
    IDBConnection::ptr m_conn;
  };
  
  //设置类型绑定
  class IDBStmt
  {
  public:
    std::shared_ptr<IDBStmt> ptr;
    virtual ~IDBStmt() {}
    virtual int  bindInt8(int idx,int8_t v) = 0;
    virtual int  bindInt16(int idx,int16_t v) = 0;
    virtual int  bindInt32(int idx,int32_t v) = 0;
    virtual int  bindInt64(int idx,int64_t v) = 0;
    virtual int  bindUint8(int idx,uint8_t v) = 0;
    virtual int  bindUint16(int idx,uint16_t v) = 0;
    virtual int  bindUint32(int idx,uint32_t v) = 0;
    virtual int  bindUint64(int idx,uint64_t v) = 0;
    virtual int  bindFloat(int idx,float v) = 0;
    virtual int  bindDouble(int idx,double v) = 0;
    virtual int  bindString(int idx,const std::string& v) = 0;
    virtual int  bindBlob(int idx,const std::string& v) = 0;
    virtual int  bindTime(int idx,time_t v) = 0;
    virtual int  bindNull(int idx) = 0;

    virtual int execute() = 0;
    virtual int getLastInsertId() = 0;
    virtual std::shared_ptr<IDBRes> query() = 0;

    int getErrno() const { return m_errno;}
    const std::string&  getStrerr() const { retrun m_strerr;}
  protected:
    virtual void setError(int v) = 0;
  protected:
    int m_errno;
    std::string m_strerr;
  };
  
  //结果集
  class IDBResult
  {
  public:
    typedef std::shared_ptr<IDBResult> ptr;

    /*
     *brief: 获取错误信息
     */
    int getErrno() const;
    const std::string& getStrerr() const;
  protected:
    virtual void setError(int v) = 0;
  public:
    virtual ~IDBResult() {}
    //获取数据大小
    virtual int getDataCount() const = 0;
    virtual int getColumnCount() const = 0;
    //当前第k列字节数
    virtual int getColumnBytes(int idx) const = 0;
    //当前第k列类型
    virtual int getColumnType(int idx) const = 0;
    //获取列名
    virtual std::string getColmnName(int idx) const = 0;
    //获取所有列名
    virtual std::vector<std::string> getColumnNames() const = 0;

    //第k列是否为null
    virtual bool isNull(int idx) = 0;
    //获取各种类型
    virtual int8_t  getInt8 (int idx) const = 0;
    virtual int16_t getInt16(int idx) const = 0;
    virtual int32_t getInt32(int idx) const = 0;
    virtual int64_t getInt64(int idx) const = 0;

    virtual uint8_t  getInt8 (int idx) const = 0;
    virtual uint16_t getInt16(int idx) const = 0;
    virtual uint32_t getInt32(int idx) const = 0;
    virtual uint64_t getInt64(int idx) const = 0;

    virtual float  getFloat(int idx) const = 0;
    virtual double getDoube(int idx) const = 0;

    virtual std::string getString(int idx) const = 0;
    virtual std::string getBlob(int idx) const = 0;

    virtual time_t getTime(int idx) const = 0;
    //获取下一个
    virtual bool next() const = 0;
    
  protected:
    int m_errno;
    std::string m_strerr;
    
  };

  
  class IDBTranscation : public IDBCommand
  {
  public:
    typedef std::shared_ptr<IDBTranscation> ptr;
    virtual ~IDBTranscation() {}
    virtual bool begin() = 0;
    virtual bool commit() = 0;
    virtual bool rollback() = 0;
  };

}
}
