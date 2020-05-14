#include"SocketStream.hpp"

namespace zhuyh
{
  SocketStream::SocketStream(Socket::ptr sock,bool owner)
    :m_owner(owner),
     m_sock(sock)
  {
  }
  SocketStream::~SocketStream()
  {
    if(m_owner && m_sock) close();
  }
  int SocketStream::read(void* buff,size_t length)
  {
    if(m_sock == nullptr || !isConnected()) return -1;
    //LOG_ROOT_ERROR() << "start sockstream read";
    return m_sock->recv(buff,length);
  }
  int SocketStream::read(ByteArray::ptr ba,size_t length)
  {
    if(m_sock == nullptr || !isConnected()) return -1;
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs,length);
    int rt = m_sock->recv(&iovs[0],iovs.size());
    if(rt >0)
      ba->setPosition(ba->getPosition()+rt);
    return rt;
  }
  
  int SocketStream::write(const void* buff,size_t length)
  {
    if(m_sock == nullptr || !isConnected()) return -1;
    return m_sock->send(buff,length);
  }
  int SocketStream::write(ByteArray::ptr ba,size_t length)
  {
    if(m_sock == nullptr || !isConnected()) return -1;
    std::vector<iovec> iovs;
    ba->getReadBuffers(iovs,length);
    int rt = m_sock->send(&iovs[0],iovs.size());
    if(rt >0)
      ba->setPosition(ba->getPosition()+rt);
    return rt;
  }
  
  void SocketStream::close()
  {
    if(m_sock)
      {
	std::call_once(flag,std::bind(&Socket::close,m_sock));
      }
  }
}
