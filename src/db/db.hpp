#pragma once

#include<memory>

namespace zhuyh
{
namespace db
{
  class IDBRes;
  class IDBStmt;
  class IDBConn
  {
  public:
    typedef std::shared_ptr<IDBConn> ptr;
    //通过连接池获取一个连接
    //static IDBConn::ptr newConnection(IDBConn::ptr factory);
    virtual ~IDBConn()
    {
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
    virtual int getErrno() const = 0;
    //C++11有move优化
    virtual std::string getError() const = 0;
    virtual bool connect() = 0;
    virtual bool ping() = 0;
    virtual void close() = 0;
    virtual bool hasError() const { return m_hasError;  }
  protected:
    bool m_connect = false;
    bool m_hasError = false;
    bool m_transcation = false;
  };

  //执行SQL
  class IDBCommand
  {
  public:    
    typedef std::shared_ptr<IDBCommand> ptr;
    IDBCommand(IDBConn::ptr conn)
      :m_conn(conn),
       m_hasError(false)
    {}
    virtual ~IDBCommand() {} 
    //负责执行sql
    virtual std::shared_ptr<IDBRes> command(const std::string& sql) = 0;
    virtual std::shared_ptr<IDBRes> command(const char* fmt,...)  = 0;
    virtual std::shared_ptr<IDBRes> command(const char* fmt,va_list ap) = 0;
    /*
    virtual bool commandStmt(const std::string& sql) const = 0;
    virtual bool commandStmt(const char* fmt,va_list ap) const = 0;
    virtual bool commandStmt(const char* fmt,...) const = 0;
    */
    
    virtual int getAffectedRow()  { return -1;}
    int getErrno() const { return m_conn->getErrno();}
    std::string getError() const { return m_conn->getError();}

    virtual std::shared_ptr<IDBRes> getRes() = 0;
    
    const std::string& getCmdStr() const { return m_cmdStr; }
  protected:
    //command的结果
    std::string m_cmdStr;
    IDBConn::ptr m_conn;
    bool m_hasError = false;
  };
    //结果集
  class IDBRes
  {
  public:
    typedef std::shared_ptr<IDBRes> ptr;

    /*
     *brief: 获取错误信息
     */
    int getErrno() const { return m_errno; }
    const std::string& getError() const { return m_error; }
  public:
    virtual ~IDBRes() {}
    IDBRes(const std::string& err,int eno)
      :m_error(err),
       m_errno(eno)
    {}
    
    virtual int getRowCount() const = 0;
    virtual int getColumnCount() const = 0;
    //当前第k列字节数
    virtual int getColumnBytes(int idx) const = 0;
    //当前第k列类型;
    virtual int getColumnType(int idx) const = 0;
    //获取列名
    virtual std::string getColumnName(int idx) const = 0;

    //第k列是否为null
    virtual bool isNull(int idx) const = 0;
    //获取各种类型
    virtual int8_t  getInt8 (int idx) const = 0;
    virtual int16_t getInt16(int idx) const = 0;
    virtual int32_t getInt32(int idx) const = 0;
    virtual int64_t getInt64(int idx) const = 0;

    virtual uint8_t  getUint8 (int idx) const = 0;
    virtual uint16_t getUint16(int idx) const = 0;
    virtual uint32_t getUint32(int idx) const = 0;
    virtual uint64_t getUint64(int idx) const = 0;

    virtual float  getFloat(int idx) const = 0;
    virtual double getDoube(int idx) const = 0;

    virtual std::string getString(int idx) const = 0;
    virtual std::string getBlob(int idx) const = 0;

    virtual time_t getTime(int idx) const = 0;
    //获取下一个
    virtual bool nextRow()  = 0;
    
  protected:
    std::string m_error;
    int m_errno;
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
    virtual int  bindString(int idx,const char* v) = 0;
    virtual int  bindBlob(int idx,const std::string& v) = 0;
    virtual int  bindBlob(int idx,const char* v,uint64_t size) = 0;
    virtual int  bindTime(int idx,time_t v) = 0;
    virtual int  bindNull(int idx) = 0;

    virtual int execute() = 0;
    virtual int64_t getLastInsertId() = 0;
    virtual std::shared_ptr<IDBRes> command() = 0;

    virtual int getErrno() const = 0;
    virtual std::string  getError() const = 0;
  protected:
    int m_errno;
    std::string m_error;
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
