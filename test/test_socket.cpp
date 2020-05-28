#include "../zhuyh.hpp"
#include <bits/stdc++.h>

using namespace zhuyh;

bool checkPortValid(const std::string& str,uint16_t& port)
{
  if(str[0] == '-') return false;
  if(str.size() > 5) return false;
  int res = 0;
  for(size_t i=0;i<str.size();i++)
    {
      if(!std::isdigit(str[i])) return false;
      res = res*10 + str[i]-'0';
    }
  if(res > 65535 || res < 0) return false;
  port = (uint16_t)res;
  return true;
}
void doClient(Socket::ptr sock)
{
  sock->setRecvTimeout(3000);
  char* buff = new char[1025];
  int rt =  sock->recv((void*)buff,1024,0);
  if(rt < 0)
    {
      LOG_ROOT_ERROR() << " failed to recv message,error : "
		       <<strerror(errno) << " errno : "<<errno<<std::endl
		       <<" sock : "<<sock;
      sock->close();
      delete [] buff;
      return;
    }
  std::stringstream ss;
  ss<<"HTTP/1.1 200 OK\r\n\r\n";
  rt = sock->send(ss.str().c_str(),ss.str().size(),0);
  if(rt < 0)
    {
      LOG_ROOT_ERROR() << " failed to send message,error : "
		       <<strerror(errno) << " errno : "<<errno<<std::endl
		       <<" sock : " <<sock;
      sock->close();
      delete [] buff;
      return;
    }
  //std::cout<<"Send : "<<buff<<std::endl;
  delete [] buff;
  sock->close();
}
void doServerLoop(uint16_t port)
{
  Socket::ptr sock = Socket::newTCPSocket();
  IAddress::ptr addr(new IPv4Address(INADDR_ANY,port));
  int rt = sock->bind(addr);
  if(rt == false) return;
  rt = sock->listen();
  if(rt == false) return;
  //accept超时
  rt = sock->setRecvTimeout(3000);
  LOG_ROOT_INFO()<<"rt : "<<rt;
  LOG_ROOT_INFO()<<"Recv Timeout : "<<sock->getRecvTimeout();
  while(1)
    {
      //LOG_ROOT_INFO() << "HERE";
      auto client = sock->accept();
      if(client == nullptr )
	{
	  if(errno != ETIMEDOUT)
	    {
	      LOG_ROOT_ERROR() << "Failed to accept a socket";
	      sock->close();
	      return ;
	    }
	  continue;
	}
      co std::bind(doClient,client);
    }
}
int main(int argc,char* argv[])
{
  if(argc == 1)
    {
      std::cerr << "Usage : test_socket <port>" <<std::endl;
      exit(1);
    }
  uint16_t port;
  std::string str(argv[1]);
  if(checkPortValid(str,port) == false)
    {
      LOG_ROOT_ERROR() << "Port : "<< str << " is not Valid";
      exit(1);
    }
  co std::bind(doServerLoop,port));
  
  while(1) sleep(1);
  return 0;
}
