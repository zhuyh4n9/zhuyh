#include"LogInServlet.hpp"
#include"WorkServlet.hpp"
#include"../config.hpp"
#include<string>
#include"../http/HttpServer.hpp"
#include"../co.hpp"

zhuyh::ConfigVar<std::string>::ptr s_pathPrefix =
  zhuyh::Config::lookUp<std::string>("proxy.path_prefix","/home/zhuyh/Code/GraduationDesign/zhuyh/materias/","proxy path prefix");

void run()
{
  zhuyh::http::HttpServer::ptr server = std::make_shared<zhuyh::http::HttpServer>();
  zhuyh::IAddress::ptr addr = zhuyh::IAddress::newAddressByHostAnyIp("0.0.0.0:8080");
  while(!server->bind(addr))
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
  proxyServlet->setDefault(proxy);
  dispatch->addGlobServlet("*",proxyServlet);
  
  server->start();
}

int main()
{
  co run;
  return 0;
}
