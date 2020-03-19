#include "LogThread.hpp"
#include "netio/Hook.hpp"
#include <assert.h>
#include <fcntl.h>
#include <sys/epoll.h>
namespace zhuyh
{
  static int setNonb(int fd)
  {
    int flags = fcntl_f(fd,F_GETFL);
    if(flags < 0 )
      return -1;
    return fcntl_f(fd,F_SETFL,flags | O_NONBLOCK);
  }
  
  static int clearNonb(int fd)
  {
    int flags = fcntl_f(fd,F_GETFL);
    if(flags < 0 )
      return -1;
    return fcntl_f(fd,F_SETFL,flags & ~O_NONBLOCK);
  }
  LogThread::LogThread()
    :m_close(true)
  {
    m_notifyFd[0] = m_notifyFd[1] = -1;
    int rt = pipe(m_notifyFd);
    if(rt <0 )
      {
	std::cout<<"Failed to create pipe for LogThread rt : "<<rt
		 << " error : "<<strerror(errno)
		 << " errno : "<<errno<<std::endl;
	assert(0);
      }
    rt = setNonb(m_notifyFd[0]);
    if(rt <0 )
      {
	std::cout<<"Failed to setNonb m_notifyFd[0]"<<m_notifyFd[0]<<" for LogThread rt : "
		 <<rt << " error : "<<strerror(errno)
		 << " errno : "<<errno<<std::endl;
	assert(0);
      }
    
    if(rt < 0)
      {
	std::cout<<"Failed to setNonb m_notifyFd[1]"<<m_notifyFd[1]<<" for LogThread rt : "
		 <<rt << " error : "<<strerror(errno)
		 << " errno : "<<errno<<std::endl;
	assert(0);
      }
    m_epfd = epoll_create(1);
    if(m_epfd < 0)
      {
	std::cout<<"Failed to create epoll_fd m_epfd : "<<m_epfd<<" for LogThread rt : "
		 <<rt << " error : "<<strerror(errno)
		 << " errno : "<<errno<<std::endl;
	assert(0);
      }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = m_notifyFd[0];
    //读端添加到epoll
    rt = epoll_ctl(m_epfd,EPOLL_CTL_ADD,m_notifyFd[0],&ev);
    if(rt < 0)
      {
	std::cout<<"Failed to add m_notifyFd[0]"<<m_notifyFd[0]
		 <<" to epoll for LogThread rt : "
		 <<rt << " error : "<<strerror(errno)
		 << " errno : "<<errno<<std::endl;
	assert(0);
      }
    start();
  }

  LogThread::~LogThread()
  {
    if(m_close == false)
      close();
  }
  
  void LogThread::start()
  {
    if(m_close)
      {
	m_thread = std::move(std::thread(std::bind(&LogThread::run,this)));
	m_close = false;
      }
  }


  void LogThread::work()
  {
    while(!m_que.empty())
      {
	std::list<LogEvent::ptr> events;
	m_que.try_popk_front(10,events);
	for(auto& event : events)
	  {
	    event->logger->log(event->l_level,event);
	    //TODO:如果是致命错误则触发断言,可能不妥，待修改
	    if(event->l_level == LogLevel::FATAL)
	      assert(0);
	  }
      }
  }
  void LogThread::run()
  {
    std::shared_ptr<char> data(new char[1024],[](char * ptr) { delete [] ptr;});
    char* buff = data.get();
    struct epoll_event evs[10];
    while(1)
      {
	work();
	if(m_close)
	  {
	    //保证日志被打印
	    work();
	    break;
	  }
	int rt = epoll_wait(m_epfd,evs,10,10);
	if(rt < 0)
	  {
	    std::cout<<"epoll_wait for LogThread error : "<<strerror(errno)
		     <<" errno : "<<errno<<" rt : "<<rt;
	  }
	else
	  {
	    while(1)
	      {
		int rt = read(m_notifyFd[0],buff,1023);
		if(rt == -1)
		  {
		    if(errno == EAGAIN || errno == EWOULDBLOCK)
		      break;
		    std::cout<<"read notifyFd[0] : "<<m_notifyFd[0]<<" failed"
			     <<" errno : " <<errno << " error : "<<strerror(errno)
			     <<" rt : "<<rt<<std::endl;
		    break;
		  }
	      }
	  }
      }
  }
  void LogThread::close()
  {
    if(m_close == true && m_thread.joinable())
      {
	//写一个空消息，唤醒epoll_wait
	int rt = write(m_notifyFd[1],"",1);
	if(rt < 0)
	  {
	    std::cout<<"write notifyFd[1] : "<<m_notifyFd[1]<<" failed"
		     <<" errno : " <<errno << " error : "<<strerror(errno)
		     <<" rt : "<<rt<<std::endl;
	  }
	m_thread.join();
	if(m_notifyFd[0] >= 0)
	  ::close(m_notifyFd[0]);
	if(m_notifyFd[1] >= 0)
	  ::close(m_notifyFd[1]);
      }
  }

  void LogThread::sendLog(LogEvent::ptr event)
  {
    //压人队列并且写空消息
    if(m_close)
      {
	std::cout<<"LogThread Closed"<<std::endl;
	return;
      }
    m_que.push_back(event);
    if(m_notifyFd[1] > 0)
      {
	int rt = write(m_notifyFd[1],"",1);
	if(rt < 0)
	  {
	    std::cout<<"read notifyFd[1] : "<<m_notifyFd[0]<<" failed"
		     <<" errno : " <<errno << " error : "<<strerror(errno)
		     <<" rt : "<<rt<<std::endl;
	  }
      }
  }

}
