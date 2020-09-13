#include "all.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

using namespace zhuyh;

int main()
{
  co [](){
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
  rt = listen(fd,SOMAXCONN);
  if(rt < 0)
    {
      LOG_ROOT_ERROR() << "Failed to listen socket";
      exit(1);
    }
  while(1)
    {
      int cfd = accept(fd,nullptr,nullptr);
      if(cfd < 0)
	{
	  LOG_ROOT_INFO() << "Accept Failed" << " error = "<<strerror(errno);
	  continue;
	}
      //LOG_ROOT_INFO() << "Accept A Connection";
      co [cfd](){
	char buf[1024];
	int n = read(cfd,buf,1024);
	if(n < 0)
	  {
	    LOG_ROOT_ERROR() << "failed to read" << " error = "<<strerror(errno);
	    return;
	  }
	buf[n] = 0;
	//std::cout << buf << std::endl;
	std::stringstream ss;
	ss<<"HTTP/1.1 200 OK\r\n\r\n";
	    int rt = write(cfd,ss.str().c_str(),ss.str().size());
	if(rt < 0)
	  {
	    LOG_ROOT_ERROR() << "failed to send msg" << " error = "<<strerror(errno);
	  }
	//std::cout<< buf <<std::endl;
	close(cfd);
				//LOG_ROOT_INFO() << "connection closed";
      };
    }
  };
  return 0;
}

