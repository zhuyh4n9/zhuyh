#include"../scheduler/Scheduler.hpp"
#include"LogInServlet.hpp"
#include"../config.hpp"
#include<string>
#include"../http/HttpServer.hpp"
#include"../co.hpp"
#include"HttpProxyServer.hpp"
#include"SecureHttpProxyServer.hpp"
#include<sys/signal.h>
zhuyh::ConfigVar<std::string>::ptr s_pathPrefix =
  zhuyh::Config::lookUp<std::string>("proxy.path_prefix","/home/zhuyh/Code/GraduationDesign/zhuyh/materias/","proxy path prefix");

zhuyh::Scheduler::ptr accept_schd = nullptr;
zhuyh::Scheduler::ptr web_schd = nullptr;

void start_webServer()
{
  zhuyh::http::HttpServer::ptr webServer = std::make_shared<zhuyh::http::HttpServer>
    (false,web_schd.get(),accept_schd.get(),"ProxyServer");

  zhuyh::IAddress::ptr webAddr = zhuyh::IAddress::newAddressByHostAnyIp("0.0.0.0:8080");
  while(!webServer->bind(webAddr))
    {
      sleep(2);
    }
  auto dispatch = webServer->getServletDispatch();
  
  auto indexServlet = std::make_shared<zhuyh::http::MethodServlet>("IndexServlet");
  auto index = std::make_shared<zhuyh::proxy::IndexServlet>
    (s_pathPrefix->getVar()+"index.html");
  indexServlet->addServlet(zhuyh::http::HttpMethod::GET,index);
  indexServlet->addServlet(zhuyh::http::HttpMethod::POST,index);
  dispatch->addServlet("/",indexServlet);
  //素材
  auto materiaServlet = std::make_shared<zhuyh::http::MethodServlet>("MateriaServlet");
  auto materias = std::make_shared<zhuyh::proxy::MateriaServlet>
    (s_pathPrefix->getVar());
  materiaServlet->addServlet(zhuyh::http::HttpMethod::GET,materias);
  materiaServlet->addServlet(zhuyh::http::HttpMethod::POST,materias);
  dispatch->addGlobServlet("/*",materiaServlet);
  //登录
  auto loginServlet = std::make_shared<zhuyh::http::MethodServlet>
    ("LogInServlet");
  auto login = std::make_shared<zhuyh::proxy::LogInServlet>(s_pathPrefix->getVar());
  loginServlet->addServlet(zhuyh::http::HttpMethod::POST,login);
  dispatch->addServlet("/login",loginServlet);
  //注册
  auto registerServlet = std::make_shared<zhuyh::http::MethodServlet>
    ("RegisterServlet");
  auto reg = std::make_shared<zhuyh::proxy::RegisterServlet>(s_pathPrefix->getVar());
  registerServlet->addServlet(zhuyh::http::HttpMethod::POST,reg);
  dispatch->addServlet("/register",registerServlet);
  webServer->start();
}

void start_proxyServer()
{
  zhuyh::TcpServer::ptr httpProxyServer = std::make_shared<zhuyh::proxy::HttpProxyServer>
    (web_schd.get(),accept_schd.get(),"HttpProxyServer");
  zhuyh::IAddress::ptr proxyAddr = zhuyh::IAddress::newAddressByHostAnyIp("0.0.0.0:8081");
  while(!httpProxyServer->bind(proxyAddr))
    {
      sleep(2);
    }
  httpProxyServer->start();
}

void start_secureProxyServer()
{
  zhuyh::TcpServer::ptr secureHttpProxyServer = std::make_shared<zhuyh::proxy::SecureHttpProxyServer>
    (nullptr,accept_schd.get(),"SecureHttpProxyServer");
  zhuyh::IAddress::ptr secureProxyAddr = zhuyh::IAddress::newAddressByHostAnyIp("0.0.0.0:8082");
  while(!secureHttpProxyServer->bind(secureProxyAddr))
    {
      sleep(2);
    }
  secureHttpProxyServer->start();
}
void run()
{
  start_webServer();
  start_proxyServer();
  start_secureProxyServer();
}

int main()
{
  sigset_t set;

  /* Block SIGQUIT and SIGUSR1; other threads created by main()
     will inherit a copy of the signal mask. */
  
  sigemptyset(&set);
  sigaddset(&set, SIGPIPE);
  int rt  = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if(rt < 0)
    {
      LOG_ROOT_ERROR() << "failed to BLOCK SIG_PIPE";
      exit(1);
    }
  accept_schd.reset(new zhuyh::Scheduler("accept",1));  
  web_schd.reset(new zhuyh::Scheduler("web",32));
  
  web_schd->start();
  accept_schd->start();
  co run;
  while(1) sleep(1);
  return 0;
}
