#include"../zhuyh.hpp"
#include<bits/stdc++.h>

using namespace zhuyh;

void test_request()
{
  http::HttpRequest::ptr req(new http::HttpRequest());
  req->setHeader("Host","www.baidu.com");
  req->setBody("Hello World");

  std::cout<<*req<<std::endl;
}
void test_response()
{
  http::HttpResponse::ptr resp(new http::HttpResponse());
  resp->setHeader("XXX","XXX");
  resp->setBody("Hello World");
  resp->setStatus(http::HttpStatus::INTERNAL_SERVER_ERROR);
  std::cout<<*resp<<std::endl;
}

int main()
{
  test_request();
  test_response();
  return 0;
}
