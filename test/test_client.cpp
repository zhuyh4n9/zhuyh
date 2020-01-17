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
  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(50000);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  int rt = connect(fd,(struct sockaddr*)&addr,sizeof(addr));
  if(rt < 0)
    {
      LOG_ROOT_ERROR() << "failed to connect to server";
    }
  char buf[1025];
  if(fgets(buf,1024,stdin) == nullptr)
    {
      LOG_ROOT_INFO() << "read empty";
      return 0;
    }
  LOG_ROOT_INFO() << "Sending : " << buf;
  int n = write(fd,buf,strlen(buf));
  if(n < 0)
    {
      LOG_ROOT_ERROR() << "Failed to sending msg";
    }
  n = read(fd,buf,1024);
  buf[n] = 0;
  LOG_ROOT_INFO() << "Received : " << buf;
  Scheduler::getThis()->stop();
  return 0;
}
