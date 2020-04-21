#include"../scheduler/Scheduler.hpp"
#include"LogInServlet.hpp"
#include"WorkServlet.hpp"
#include"../config.hpp"
#include<string>
#include"../http/HttpServer.hpp"
#include"../co.hpp"

zhuyh::ConfigVar<std::string>::ptr s_pathPrefix =
  zhuyh::Config::lookUp<std::string>("proxy.path_prefix","/home/zhuyh/Code/GraduationDesign/zhuyh/materias/","proxy path prefix");

zhuyh::Scheduler::ptr accept_schd = nullptr;
zhuyh::Scheduler::ptr web_schd = nullptr;
void run()
{
  zhuyh::http::HttpServer::ptr server = std::make_shared<zhuyh::http::HttpServer>
    (false,web_schd.get(),accept_schd.get(),"ProxyServer");
  zhuyh::IAddress::ptr webAddr = zhuyh::IAddress::newAddressByHostAnyIp("0.0.0.0:8080");

  zhuyh::IAddress::ptr proxyAddr = zhuyh::IAddress::newAddressByHostAnyIp("0.0.0.0:8081");
  zhuyh::IAddress::ptr secureProxyAddr = zhuyh::IAddress::newAddressByHostAnyIp("0.0.0.0:8082");

  std::vector<zhuyh::IAddress::ptr> addrs,failed;
  addrs.push_back(webAddr);
  addrs.push_back(proxyAddr);
  addrs.push_back(secureProxyAddr);
  while(!server->bind(addrs,failed))
    {
      sleep(2);
    }
  auto dispatch = server->getServletDispatch();


  auto indexServlet = std::make_shared<zhuyh::http::MethodServlet>("IndexServlet");
  auto index = std::make_shared<zhuyh::proxy::IndexServlet>
    (s_pathPrefix->getVar()+"index.html");
  indexServlet->addServlet(zhuyh::http::HttpMethod::GET,index);
  indexServlet->addServlet(zhuyh::http::HttpMethod::POST,index);
  dispatch->addServlet("/",indexServlet);


  auto materiaServlet = std::make_shared<zhuyh::http::MethodServlet>("MateriaServlet");
  auto materias = std::make_shared<zhuyh::proxy::MateriaServlet>
    (s_pathPrefix->getVar());
  materiaServlet->addServlet(zhuyh::http::HttpMethod::GET,materias);
  materiaServlet->addServlet(zhuyh::http::HttpMethod::POST,materias);
  dispatch->addGlobServlet("/*",materiaServlet);

  auto loginServlet = std::make_shared<zhuyh::http::MethodServlet>
    ("LogInServlet");
  auto login = std::make_shared<zhuyh::proxy::LogInServlet>(s_pathPrefix->getVar());
  loginServlet->addServlet(zhuyh::http::HttpMethod::POST,login);
  dispatch->addServlet("/login",loginServlet);

  auto registerServlet = std::make_shared<zhuyh::http::MethodServlet>
    ("RegisterServlet");
  auto reg = std::make_shared<zhuyh::proxy::RegisterServlet>(s_pathPrefix->getVar());
  registerServlet->addServlet(zhuyh::http::HttpMethod::POST,reg);
  dispatch->addServlet("/register",registerServlet);


  auto proxyServlet = std::make_shared<zhuyh::http::MethodServlet>
    ("ProxyServlet");
  auto proxy = std::make_shared<zhuyh::proxy::ProxyServlet>();
  auto httpsProxy = std::make_shared<zhuyh::proxy::ConnectServlet>();
  proxyServlet->setDefault(proxy);
  proxyServlet->addServlet(zhuyh::http::HttpMethod::CONNECT,httpsProxy);
  dispatch->addGlobServlet("*",proxyServlet);
  
  server->start();
}

int main()
{
  accept_schd.reset(new zhuyh::Scheduler("accept",1));
  accept_schd->start();
  
  web_schd.reset(new zhuyh::Scheduler("web",8));
  web_schd->start();
  co run;
  while(1) sleep(1);
  return 0;
}
