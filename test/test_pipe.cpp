#include"../zhuyh.hpp"
#include<bits/stdc++.h>
using namespace zhuyh;
int main()
{
  co [](){
    int pipefd[2];
    LOG_ROOT_INFO() << "BEGIN";
    int rt = pipe(pipefd);
    if(rt < 0)
      {
	LOG_ROOT_ERROR() << "pipe create error = "<<strerror(errno);
	exit(1);
      }
    co [pipefd](){
      int fd = dup(pipefd[1]);
      for(int i=0;i<10;i++)
	{
	  std::stringstream ss;
	  ss<<"this is test_"<<i+1;
	  LOG_ROOT_INFO()<< "Sending : " << ss.str();
	  sleep(1);
	  int rt = write(fd,ss.str().c_str(),ss.str().size());
	  if(rt < 0)
	    {
	      LOG_ROOT_ERROR() << "write to pipe error = "<<strerror(errno);
	    }
	}
      close(pipefd[1]);
      close(fd);
    };  
    co [pipefd]()
      {
	while(1)
	  {
	    LOG_ROOT_INFO() << "start to read";
	    char* buf =(char*) malloc(1025);
	    int rt = read(pipefd[0],buf,1024);
	    if(rt == 0)
	      break;
	    if(rt < 0)
	      {
		LOG_ROOT_ERROR() << "write to pipe error = "<<strerror(errno);
		break;
	      }
	    buf[rt] = 0;
	    LOG_ROOT_INFO() << buf;
	    free(buf);
	  }
	close(pipefd[0]);
      };
  };
  return 0;
}
