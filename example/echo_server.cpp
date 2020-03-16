#include "../zhuyh.hpp"
#include <bits/stdc++.h>

using namespace zhuyh;

class EchoServer : public TcpServer
{
public:
  EchoServer(int type);
  void handleClient(Socket::ptr client) override;
private:
  int m_type = 0;
};

EchoServer::EchoServer(int type)
  :m_type(type)
{
}

void EchoServer::handleClient(Socket::ptr client)
{
  LOG_ROOT_INFO() << "new connection : "<<client;
  ByteArray::ptr ba = std::make_shared<ByteArray>();
  while(1)
    {
      ba->clear();
      std::vector<iovec> iovs;
      ba->getWriteBuffers(iovs,1024);
      
      int rt = client->recv(&iovs[0],iovs.size());
      if(rt == 0)
	{
	  LOG_ROOT_INFO() << "client close : "<<client;
 	  break;
	}
      else if(rt < 0)
	{
	  LOG_ROOT_ERROR() << "client error :"<<strerror(errno)
			   << " errno : "<<errno << " rt : "<<rt;
	  break;
	}
      //修改totalsize;
      ba->setPosition(ba->getPosition() + rt);
      ba->setPosition(0);
      if(m_type == 1)
	{
            std::cout << ba->dump()<<std::endl;
	}
      else
	{
	    std::cout << ba->dumpToHex()<<std::endl;
	}
       rt =  client->send(&iovs[0],iovs.size());
       if(rt == 0)
	{
	  LOG_ROOT_INFO() << "client close : "<<client;
	  break;
	}
      else if(rt < 0)
	{
	  LOG_ROOT_ERROR() << "client error :"<<strerror(errno)
			   << " errno : "<<errno << " rt : "<<rt;
	  break;
	}
    }
}


void run(int type)
{
  EchoServer::ptr server = std::make_shared<EchoServer>(type);
  
  IAddress::ptr addr = IAddress::newAddressByHostAny("0.0.0.0:8080");
  std::vector<IAddress::ptr> addrs;
  std::vector<IAddress::ptr> fails;
  addrs.push_back(addr);

  while(!server->bind(addrs,fails))
    sleep(2);
  server->start();
}
int main(int argc,char* argv[])
{
  if(argc < 2)
    {
      std::cerr<<"Usage : echo_server <-b|-t>"<<std::endl;
      exit(1);
    }
  int type = 0;
  if(strcmp(argv[1],"-b"))
      type = 1;
  else
    type = 0;
  co std::bind(run,type);
  return 0;
}
