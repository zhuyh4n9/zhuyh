/*
版本1
 */

#pragma once

#include<memory>

namespace zhuyh
{

  class IDBConnectionFactory
  {
  public:
    typedef std::shared_ptr<IDBConnectionFactory> ptr;
    virtual bool getMaxConnection() const = 0;
    virtual 
  };
  class IDBConnection
  {
  public:
    typedef std::shared_ptr<IDBConnection> ptr;
    //通过连接池获取一个连接
    static IDBConnection::ptr newConnection(IDBConnection::ptr factory);
    void init()
    {
      if(!m_connect) connect();
    }
    bool isConnect() const
    {
      return m_connect;
    }
    void
  protected:
    virtual connect() = 0;
  public:
  private:
    bool m_connect;
    bool m_isTranscation;
  };

  class IDBCommand
  {
  public:
    typedef std::shared_ptr<IDBCommand> ptr;
    command
  };
  class IDBResult
  {
  };
  class ITranscation
  {
  };
    
};
