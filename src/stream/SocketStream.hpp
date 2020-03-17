#pragma once

#include"Stream.hpp"
#include"../socket/Socket.hpp"

namespace zhuyh
{
  class SocketStream : public Stream
  {
  public:
    typedef std::shared_ptr<SocketStream> ptr;
    SocketStream(Socket::ptr sock,bool owner = true);

    virtual ~SocketStream();
    virtual int read(void* buff,size_t length) override;
    virtual int read(ByteArray::ptr ba,size_t length) override;

    virtual int write(const void* buff,size_t length) override;
    virtual int write(ByteArray::ptr ba,size_t length) override;

    virtual void close() override;

    Socket::ptr getSocket() const
    {
      return m_sock;
    }

    bool isConnected() const
    {
      return m_sock->isConnected();
    }
  protected:
    //为true则析构时关闭socket
    bool m_owner;
    Socket::ptr m_sock;
  };
}
