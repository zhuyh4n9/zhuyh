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
  for(int i=0;i<20;++i)
    {
      co [i](){
	int fd = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(50000);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	int rt = connect(fd,(struct sockaddr*)&addr,sizeof(addr));
	if(rt < 0)
	  {
	    LOG_ROOT_ERROR() << "failed to connect to server "<< strerror(errno);
	    return;
	  }
	LOG_ROOT_INFO() << "connect success !";
	char* buf = (char*)malloc(1025);
	snprintf(buf,1024,"This is a test_%d",i);
	LOG_ROOT_INFO() << "Sending : " << buf;
	int n = write(fd,buf,strlen(buf));
	if(n < 0)
	  {
	    LOG_ROOT_ERROR() << "Failed to sending msg";
	  }
	int cnt = 0;
	n = 0;
	while(1)
	  {
	    n = read(fd,buf,1024);
	    if( n <0 )
	      {
		LOG_ROOT_ERROR() << "recv size "<<n << " errno = "<< errno << " error= "<<strerror(errno);
		if(++cnt <= 5)
		  continue;
		break;
	      }
	    else
	      {
		buf[n] = 0;
		LOG_ROOT_INFO() << "Received : " << buf;
		break;
	      }
	  }
	close(fd);
	free(buf);
      };
    }
  Scheduler::getThis()->stop();
  return 0;
}
