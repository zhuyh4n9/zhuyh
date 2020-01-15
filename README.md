# C++多线程多协程HTTP代理服务器 -- 开发中
## 开发工具
  - 编辑器   : emacs
  - 编译工具 : g++ 7.4.0 , cmake 3.10.2 , make 4.1
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
- 目前的问题:
    1.创建协程性能较差(1s只能创建20万个协程,未来尝试使用内存池优化)
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
    协程调度器
      本质是一个线程池,默认为包含32个执行器和1个IO管理器
      获取协程调度器
        auto scheduler = zhuyh::Scheduler::getThis();
      添加任务(建议以co 函数名的方式添加任务)
        scheduler->addTask(Task::ptr(new Task(cb)));
      每次把新任务加给当前负载最小的执行器
    执行器
        - 本质是一个线程
        - 无可执行任务时则尝试向任务最多的一个线程偷取任务,偷不到则阻塞在epoll_wait,等待超时/执行队列有新任务
    IO事件管理器 
        - 目前只支持通过获取根调度器添加IO事件
            zhuyh::Scheduler::getThis()  -> addWriteEvent(fd,Task::ptr);
            zhuyh::Scheduler::getThis()  -> addReadEvent(fd,Task::ptr);
    基于epoll_wait超时的定时器
         - 使用clock_gettime调用获取开机时间(TFD_MONOTONIC),无需担心系统时间被更改
         - 回调函数
            zhuyh::Scheduler::getThis() -> addTimer(Timer::ptr(new Timer(sec,msec,usec,nsec)),call_back);
         - 协程
            zhuyh::Scheduler::getThis() -> addTimer(Timer::ptr(new Timer(sec,msec,usec,nsec));
    协程信号量
         创建一个协程信号量
           CoSemaphore a(val) ;
         支持的方法有:
		       a.wait();
           a.notify();
           a.tryWait();
         如果使用协程信号量,需要显式关闭调度器 : Scheduler::getThis() -> stop();
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
iocntl
setsockopt

未来计划支持
pipe
pipe2
dup
dup2

```
