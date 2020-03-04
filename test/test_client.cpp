#include "../zhuyh.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
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

bool checkAddressValid(const std::string& addr)
{
  int val = 0;
  int cnt = 0;
  int head = 1;
  std::cout<<addr<<std::endl;
  for(size_t i = 0;i<addr.size();i++)
    {
      if(!isdigit(addr[i]) && addr[i]!='.') return false;
      if(addr[i] == '.')
	{
	  cnt++;
	  if(cnt >3 || val > 255) return false;
	  head = 1;
	  //std::cout<<"val "<<val<<std::endl;
	  val = 0;
	  continue;
	}
      if(head == 1) 
	{
	  if(addr[i] == '0')
	    {
	      if(i+1 < addr.size() && isdigit(addr[i+1]))
		return false;
	    }
	  head = 0;
	}
      val = val*10 + addr[i]-'0';
      //std::cout<<"v : "<<val<<std::endl;
    }
  //std::cout<<"val "<<val<<std::endl;
  if(val > 255) return false;
  return true;
}
void doClient(int i,char * saddr,uint16_t port)
{
  Socket::ptr sock = Socket::newTCPSocket();
  IAddress::ptr addr = IPv4Address::newAddress(saddr,port);
  bool rc = sock->connect(addr);
  if(rc == false)
    {
      LOG_ROOT_ERROR() << "connect to server failed"<<std::endl;
      return ;
    }
  char* buf = new char[1025];
  snprintf(buf,1024,"This is test_%d",i);
  int msgLen = strlen(buf);
  LOG_ROOT_INFO() << "Sending : "<<buf;
  int rt = sock->send(buf,msgLen,0);
  if(rt < 0)
    {
      LOG_ROOT_ERROR() << "send to server failed,error : "
		       <<strerror(errno)<<" errno : "<<errno
		       <<std::endl<<"sock : "<<sock;
      sock->close();
      delete [] buf;
      return;
    }
  rt = sock->recv(buf,1024,0);
  if(rt < 0)
    {
      LOG_ROOT_ERROR() << "send to server failed,error : "
		       <<strerror(errno)<<" errno : "<<errno
		       <<std::endl<<"sock : "<<sock;
      sock->close();
      delete [] buf;
      return;
    }
  LOG_ROOT_INFO() << "Received : "<<buf;
  sock->close();
  delete [] buf;
}
int main(int argc,char* argv[])
{
  if(argc < 3)
    {
      std::cerr << "Usage : test_client <address> <port>"<<std::endl;
      exit(1);
    }
  uint16_t port;
  if(!checkAddressValid(argv[1]))
    {
      LOG_ROOT_ERROR() << "Address : "<<argv[1]<<" is Not Valid";
      exit(1);
    }
  if(!checkPortValid(argv[2],port))
    {
      LOG_ROOT_ERROR() << "Port : "<<argv[2]<<" is Not Valid";
      exit(1);
    }
  for(int i=0;i<30;i++)
    co std::bind(doClient,i,argv[1],port);
  return 0;
}
