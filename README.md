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
    创建调度器 
        目前还不允许用户创建调度器,只可以使用根调度器
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
    IO事件管理器 
        - 为了避免jump_fcontext被同时执行两次,IO事件在加入时会先变为SWITCHING状态,切换到主协程后变为HOLD
        - 目前只支持通过获取根调度器添加IO事件
            zhuyh::Scheduler::getThis()  -> addWriteEvent(fd,Task::ptr);
            zhuyh::Scheduler::getThis()  -> addReadEvent(fd,Task::ptr);
    基于timerfd的异步定时器
       - 支持纳秒级定时器
       - 目前只支持单次计时,不支持循环计时
         - 回调函数
            zhuyh::Scheduler::getThis() -> addTimer(Timer::ptr(new Timer(sec,msec,usec,nsec)),call_back);
         - 协程
            zhuyh::Scheduler::getThis() -> addTimer(Timer::ptr(new Timer(sec,msec,usec,nsec));
       - 缺陷 : 定时器占用资源过多，计划改为基于epoll_wait的timeout的定时器,减少系统资源的消耗
          - 利用clock_gettime获取纳秒级的时间
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
```
