# C++多线程多协程HTTP代理服务器 -- 开发中

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
```
- 目前的问题:
    1.创建协程性能较差(1s只能创建20万个协程,未来尝试使用内存池优化)
    2.协程创建比较频繁时使用自旋锁性能较差,而对于协程切换使用自旋锁性能较高
    3.是否需要使用map(需要加锁)来代替遍历(原子量)来找到负载最小的线程
```
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
        - 目前只支持通过获取根调度器添加IO事件
            zhuyh::Scheduler::getThis()  -> addWriteEvent(fd,Task::ptr)
            zhuyh::Scheduler::getThis()  -> addReadEvent(fd,Task::ptr)
    基于timerfd的异步定时器 
        - 正在开发
    协程信号量
         - 待开发
```