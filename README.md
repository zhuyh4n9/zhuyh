# C++多线程多协程HTTP代理服务器 -- 开发中
## 开发工具
  - 编辑器   : emacs
  - 编译工具 : g++ 8.3.0 , cmake 3.10.2 , make 4.1
## 使用
  - 需要安装Boost库,yaml-cpp,tinyxml2库
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

## Socket模块
```C++
//创建一个IPv4地址
IAddress::ptr addr = IPv4Address::newAddress(addr_str,port);
IAddress::ptr addr = std::make_shared<IPv4Address>(addr_uint,port);
//创建一个IPv6地址
IAddress::ptr addr = IPv6Address::newAddress(addr_str,port);
IAddress::ptr addr = std::make_shared<IPv6Address>(addr_uint,port);
//创建一个地址(不使用DNS)
IAddress::ptr addr = IPAddress::newAddress(addr_str,port);
//通过struct sockaddr结构体指针创建一个地址
IAddress::ptr addr = IAddress::newAddress(struct sockaddr* saddr);
//通过uri获取地址(全部地址)
bool rc = IAddress::newAddressByHost(std::vector<IAddress::ptr>& res,const std::string& host,
		     bool any ,int family,int sockType int protocol)
//通过uri获取地址(获取任意一个IP)
IAddress::ptr addr = IAddress:newAddressByHostAnyIp(host,family,type,protocol);
//创建套接字
Socket::ptr sock = Socket::newTCPSocket();
Socket::ptr sock = Socket::newTCPSocketv6();
Socket::ptr sock = Socket::newUDPSocket();
Socket::ptr sock = Socket::newUDPSocketv6();
//绑定地址
sock->bind(IAddress::ptr addr);
//设置监听队列大小
sock->listen(int backlog);
//接受一个连接
Socket::ptr client = sock->accept();
//读
int rt = client->recv(buff,length,flags);
//写
int rt = client->send(buff,length,flags);
//连接
bool rc = client->connect(IAddress::ptr);
//设置读超时时间
bool rc = client->setRecvTimeout(uint64_t ms);
//获取读超时时间
uint64_t to = client->getRecvTimeout();
//设置写超时时间
bool rc = client->setSendTimeout(uint64_t ms);
//获取写超时时间
uint64_t to = client->getSendTimeout();
```
## Http
```C++
//解析Http请求
HttpRequestParser::ptr parser(new HttpRequestParser())
while(!parser->isFinished())
{
  //自动移动以读取数据
    parser->execute(buff);
}
HttpRequest::ptr req = parser->getData();

//解析Http响应
HttpResponseParser::ptr parser(new HttpResponse());
while(!parser->isFinished())
{
  //自动移动以读取数据
    parser->execute(buff);
}
HttpResponse::ptr resp = parser->getData();
```
## MySQL
```C++

//连接池中获取一个连接
MySQLConn::ptr conn = MySQLConn::Create(name);
//RAII资源管理,析构时交给MySQLManager管理
MySQLConnGuard::ptr connGuard(conn);
//执行SQL语句
MySQLCommand::ptr cmd(new MySQLCommand(conn));
MySQLRes::ptr res = cmd->command(fmt,...);
MySQLStmtRes::ptr res = cmd->commandStmt(stmt,args);
MySQLStmtRes::ptr res = cmd->commandStmtStr(stmtStr,args);

```