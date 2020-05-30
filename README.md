# C++多线程多协程HTTP代理服务器 -- 开发中
## 开发工具
  - 编辑器   : emacs
  - 编译工具 : g++ 8.3.0 , cmake 3.10.2 , make 4.1
## 使用
  - 需要安装Boost库,yaml-cpp,tinyxml2,openssl库
  - 需要配置环境变量PATH_CFG,必须是一个目录
## 日志模块
```C++
    使用根日志
    LOG_ROOT_XXX() << "Message"; 
    创建日志
    auto logger = GET_LOGGER("日志名");
    使用日志
    LOG_XXX(logger) << "Message"
```
## 协程模块
```C++
    创建协程
    void func()
    {
        LOG_ROOT_INFO() << "Running on a coroutine";
        co_yield;
        LOG_ROOT_INFO() << "Coroutine is about to exit";
    }
    co func;
    co [](){
        LOG_ROOT_INFO() << "Running on a coroutine";
        co_yield;
        LOG_ROOT_INFO() << "Coroutine is about to exit";
    };
    协程
      基于boost.context的协程
    协程信号量
         创建一个协程信号量
           CoSemaphore a(val) ;
         支持的方法有:
           a.wait();
           a.notify();
           a.tryWait();
    Channel 
        计划利用协程信号量实现一套channel用于协程间的通信

```

## HOOK
```C++
目前hook的系统调用
sleep
usleep
nanosleep
socket
connect
accept
read
readv
recv
recvfrom
recvmsg
write
writev
send
sendto
sendmsg
close
fcntl
ioctl
setsockopt
pipe
pipe2
dup
dup2
dup3

```