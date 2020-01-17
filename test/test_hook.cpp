#include "../zhuyh.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

using namespace zhuyh;

int main()
{
  int fd = socket(AF_INET,SOCK_STREAM,0);
  if(fd < 0 )
    {
      LOG_ROOT_ERROR() << "failed to create socket";
      exit(1);
    }
  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(50000);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  int rt = bind(fd,(struct sockaddr*)&addr,sizeof(addr));
  if(rt < 0 )
    {
      LOG_ROOT_ERROR() << "Failed to bind address";
      exit(1);
    }
  rt = listen(fd,5);
  if(rt < 0)
    {
      LOG_ROOT_ERROR() << "Failed to listen socket";
      exit(1);
    }
  while(1)
    {
      int cfd = accept(fd,nullptr,nullptr);
      ASSERT2(cfd >= 0,"failed to accept socket");
      co [cfd](){
	char buf[1024];
	int n = read(cfd,buf,1024);
	if(n < 0)
	  {
	    LOG_ROOT_ERROR() << "failed to read";
	    return;
	  }
	buf[n] = 0;
	LOG_ROOT_INFO() << "Received : " << buf;
	int rt = write(cfd,buf,n);
	if(rt < 0)
	  {
	    LOG_ROOT_ERROR() << "failed to send msg";
	  }
	close(cfd);
      };
    }
  return 0;
}

