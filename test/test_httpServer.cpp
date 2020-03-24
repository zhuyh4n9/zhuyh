#include<bits/stdc++.h>
#include"../zhuyh.hpp"

using namespace zhuyh;

void run()
{
  http::HttpServer::ptr server = std::make_shared<http::HttpServer>();
  IAddress::ptr addr = IAddress::newAddressByHostAnyIp("0.0.0.0:8080");
  while(!server->bind(addr))
    {
      sleep(2);
    }
  auto dispatch = server->getServletDispatch();
  dispatch->addServlet("/",[](http::HttpRequest::ptr req,
			      http::HttpResponse::ptr resp,
			      http::HttpSession::ptr session)
			   {
			     resp->setStatus(http::HttpStatus::MOVED_PERMANENTLY);
			     resp->setHeader("Location","/zhuyh/xx");
			     resp->setBody(req->toString());
			     return 0;
			   });
  dispatch->addServlet("/zhuyh/xx",[](http::HttpRequest::ptr req,
				      http::HttpResponse::ptr resp,
				      http::HttpSession::ptr session)
				   {
				     resp->setStatus(http::HttpStatus::OK);
				     resp->setBody(req->toString());
				     return 0;
				   });
  dispatch->addGlobServlet("/zhuyh/*",[](http::HttpRequest::ptr req,
					 http::HttpResponse::ptr resp,
					 http::HttpSession::ptr session)
				      {
					resp->setStatus(http::HttpStatus::OK);
					resp->setBody("Glob\r\n"+req->toString());
					return 0;
				      });
  server->start();
}

int main()
{
  co run;
  return 0;
}
