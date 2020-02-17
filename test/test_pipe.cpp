#include"../zhuyh.hpp"
#include"../src/netio/Hook.hpp"

#include<bits/stdc++.h>
using namespace zhuyh;
int main()
{
  co [](){
    LOG_ROOT_INFO() << "BEGIN";
    int *pipefd = new int[2]();
    int rt = pipe(pipefd);
    int rd = pipefd[0];
    if(rt < 0)
      {
	LOG_ROOT_ERROR() << "pipe create error = "<<strerror(errno);
	exit(1);
      }
    //int fd = dup(pipefd[1]);
    //close(pipefd[1]);
    co [rd]()
      {
	char* buf =(char*) malloc(1025);
	while(1)
	  {
	    LOG_ROOT_INFO() << "start to read";    
	    int rt = read(rd,buf,1024);
	    if(rt == 0)
	      break;
	    if(rt < 0)
	      {
		LOG_ROOT_ERROR() << "read from pipe error = "<<strerror(errno);
		break;
	      }
	    buf[rt] = 0;
	    LOG_ROOT_INFO() << buf;
	  }
	free(buf);
	LOG_ROOT_INFO() << "read finished : rd = "<<rd;
	close(rd);
      };
    for(int i=0;i<100;i++)
      {
	std::stringstream ss;
	ss<<"this is test_"<<i+1;
	LOG_ROOT_INFO()<< "Sending : " << ss.str();
	int rt = write(pipefd[1],ss.str().c_str(),ss.str().size());
	if(rt < 0)
	  {
	    LOG_ROOT_ERROR() << "write to pipe error = "<<strerror(errno);
	  }
      }
    sleep(3);
    int ret = close(pipefd[1]);
    LOG_ROOT_INFO() << "send finished : "<< "pipe[1] = "<<pipefd[1] << " rt = "<<ret;
  };
  return 0;
}
