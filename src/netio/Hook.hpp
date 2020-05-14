#pragma once

#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include"../util.hpp"

namespace zhuyh
{
  //线程级别的Hook
  struct Hook
  {
    static void setHookState(bool state);
    static bool isHookEnable();
  };
  
}

extern "C"
{
  /*
   * sleep相关
   */
  using sleep_func =  unsigned int(*)(unsigned int seconds);
  extern sleep_func sleep_f;

  using usleep_func =  int (*)(useconds_t usec);
  extern usleep_func usleep_f;

  using nanosleep_func = int (*)(const struct timespec *req, struct timespec *rem);
  extern nanosleep_func nanosleep_f;

  /*
   * 创建套接字
   */
  using socket_func =  int (*)(int domain,int type,int protocol);
  extern socket_func socket_f;

  /*
   * 套接字选项
   */
  using fcntl_func = int (*)(int fd, int cmd, ... /* arg */ );
  extern fcntl_func fcntl_f;

  using ioctl_func = int (*)(int fd, unsigned long request, ...);
  extern ioctl_func ioctl_f;
  
  using setsockopt_func = int (*)(int sockfd, int level, int optname,
				  const void *optval, socklen_t optlen);
  extern setsockopt_func setsockopt_f;
  
  /*
   *管道
   */
  using pipe_func = int (*)(int pipefd[2]);

  using pipe2_func = int (*)(int pipefd[2], int flags);
  /*
   *Socket IO读写相关
   */
  using accept_func = int (*)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
  extern accept_func accept_f;

  using connect_func = int (*)(int sockfd, const struct sockaddr *addr,
			       socklen_t addrlen);
  extern connect_func connect_f;
  
  using read_func = ssize_t (*)(int fd, void *buf, size_t count);
  extern read_func read_f;

  using readv_func = ssize_t (*)(int fd, const struct iovec *iov, int iovcnt);
  extern readv_func readv_f;

  using send_func = ssize_t (*)(int sockfd, const void *buf, size_t len, int flags);
  extern send_func send_f;
  
  using sendto_func = ssize_t (*)(int sockfd, const void *buf, size_t len, int flags,
				     const struct sockaddr *dest_addr, socklen_t addrlen);
  extern sendto_func sendto_f;
  
  using sendmsg_func = ssize_t (*)(int sockfd, const struct msghdr *msg, int flags);
  extern sendmsg_func sendmsg_f;

  using write_func = ssize_t (*)(int fd, const void *buf, size_t count);
  extern write_func write_f;
  
  using writev_func = ssize_t (*)(int fd, const struct iovec *iov, int iovcnt);
  extern writev_func writev_f;
  
  using recv_func = ssize_t (*)(int sockfd, void *buf, size_t len, int flags);
  extern recv_func recv_f;

  using recvfrom_func = ssize_t (*)(int sockfd, void *buf, size_t len, int flags,
				    struct sockaddr *src_addr, socklen_t *addrlen);
  extern recvfrom_func recvfrom_f;
  
  using recvmsg_func = ssize_t (*)(int sockfd, struct msghdr *msg, int flags);
  extern recvmsg_func recvmsg_f;
  
  using close_func = int (*)(int fd);
  extern close_func close_f;

  using pipe_func = int (*)(int pipefd[2]);
  extern pipe_func pipe_f;

  using pipe2_func = int (*)(int pipefd[2], int flags);
  extern pipe2_func pipe2_f;

  using dup_func = int (*)(int oldfd);
  extern dup_func dup_f;

  using dup2_func = int (*)(int oldfd, int newfd);
  extern dup2_func dup2_f;
  
  using dup3_func = int (*)(int oldfd, int newfd, int flags);
  extern dup3_func dup3_f;

  using shutdown_func =  int(*)(int sockfd, int how);
  extern shutdown_func shutdown_f;
}
