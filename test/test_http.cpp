#include "../zhuyh.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

int main()
{
  
  co [](){
    int fd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = inet_addr("104.193.88.77");
    int rt = connect(fd,(struct sockaddr*)&addr,sizeof(addr));
    if(rt < 0)
      {
	LOG_ROOT_INFO() << "connection failed" << " error = "<< strerror(errno);
	return;
      }
    LOG_ROOT_INFO() << "connect success !";
    const char  buf[] = "GET / HTTP/1.0\r\n\r\n";
    rt = write(fd,buf,strlen(buf));
    if(rt < 0)
      {
	LOG_ROOT_INFO() << "request failed"<<" error = "<< strerror(errno);
	return;
      }
    LOG_ROOT_INFO() << "request success";
    char* buff = (char*)malloc(4097);
    while(1)
      {
	rt = read(fd,buff,4096);
	if(rt == 0) break;
	if(rt < 0)
	  {
	    LOG_ROOT_INFO() << "read failed"<<" error = "<< strerror(errno);
	    break;
	  }
	buff[rt] = 0;
	std::cout<<buff;
      }
    std::cout<<std::endl;
    free(buff);
    close(fd);
  };
  zhuyh::Scheduler::Schd::getInstance()->stop();
}
	
