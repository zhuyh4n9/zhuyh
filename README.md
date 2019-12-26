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
    IO事件 - 目前只支持通过获取根调度器添加IO事件
        zhuyh::Scheduler::getThis()  -> addWriteEvent(fd,Task::ptr)
        zhuyh::Scheduler::getThis()  -> addReadEvent(fd,Task::ptr)
    定时器 - 正在开发
    协程信号量 - 待开发
```