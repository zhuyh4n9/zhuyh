#include "HttpSession.hpp"
#include "HttpParser.hpp"

namespace zhuyh
{
namespace http
{
  HttpSession::HttpSession(Socket::ptr sock,bool owner)
    :SocketStream(sock,owner)
  {
  }

  HttpRequest::ptr HttpSession::recvRequest()
  {
    HttpRequestParser::ptr parser = std::make_shared<HttpRequestParser>();
    int64_t header_size = HttpRequestParser::getMaxHeaderSize();
    std::shared_ptr<char> buff(new char[header_size],[](char* data) { delete [] data; });
    char* data = buff.get();
    int64_t remain = 0;
    int64_t nparser = 0;
    /*
      | --- waiting for parser --- | --- free --- |
      0           nparser         len         header_size
                      |            |
		      --------------
		          remain
     */
    while(1)
      {
	int len = read(data,header_size-remain);
	if(len <= 0)
	  {
	    close();
	    return nullptr;
	  }
	//当前剩余未paser的数据总长度
	len += remain;
	nparser = parser->execute(data,len);
	if(parser->hasError())
	  {
	    close();
	    return nullptr;
	  }
	//当前parser结束后剩余数据
	remain = len - nparser;
	if(remain == header_size)
	  {
	    close();
	    return nullptr;
	  }
	if(parser->isFinished())
	  {
	    break;
	  }
      }
    int64_t length = parser->getContentLength();
    if(length > 0)
      {
	std::string body;
	body.resize(length);
	length -= remain;
	int len = 0;
	if(length >= remain)
	  {
	    body.append(data,remain);
	    len = remain;
	  }
	else
	  {
	    body.append(data,length);
	    len = length;
	  }
	length -= remain;
	if(length > 0)
	  {
	    if(readFixSize(&body[len],length) <=0)
	      {
		close();
		return nullptr;
	      }
	  }
	parser->getData()->setBody(body);
      }
    return parser->getData();
  }
  
  int HttpSession::sendResponse(HttpResponse::ptr resp)
  {
    std::stringstream ss;
    ss<<*resp;
    //LOG_ROOT_INFO() << *resp;
    return writeFixSize(ss.str().c_str(),ss.str().size());
  }
  
}
}
