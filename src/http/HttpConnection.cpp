#include "HttpConnection.hpp"
#include "HttpParser.hpp"

namespace zhuyh
{
namespace http
{
  static Logger::ptr s_logger = GET_LOGGER("system");
  HttpConnection::HttpConnection(Socket::ptr sock,bool owner)
    :SocketStream(sock,owner)
  {
  }

  HttpResponse::ptr HttpConnection::recvResponse()
  {
    HttpResponseParser::ptr parser = std::make_shared<HttpResponseParser>();
    int64_t header_size = HttpResponseParser::getMaxHeaderSize();
    std::shared_ptr<char> buff(new char[header_size+1],[](char* data) { delete [] data; });
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
	int len = read(data+remain,header_size-remain);
	if(len <= 0)
	  {
	    close();
	    return nullptr;
	  }
	//当前剩余未paser的数据总长度
	len += remain;
	data[len] = 0;
	//std::cout<<data<<std::endl;
	//LOG_ROOT_INFO() << data;
	nparser = parser->execute(data,len,false);
	if(parser->hasError())
	  {
	    close();
	    return nullptr;
	  }
	//当前parser结束后剩余数据
	remain = len - nparser;
	data[remain] = 0;

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
    auto& client_parser = parser->getParser();
    std::string body;
    if(client_parser.chunked)
      {
	parser->getData()->delHeader("Transfer-Encoding");
	int len = remain;
	int left = 0;
	do
	  {
	    bool begin = true;
	    do
	      {
		if(!begin || len == 0)
		  {
		    int rt = read(data + len,header_size - len);
		    if(rt <= 0)
		      {
			//LOG_ROOT_INFO() << "closed1";
			close();
			return nullptr;
		      }
		    len += rt;
		  }
		data[len] = 0;
		//LOG_ROOT_INFO() << data;
		size_t nparser = parser->execute(data,len,true);
		// LOG_ROOT_INFO() << " content len "<<parser->getParser().content_len
		// 		<<" nparser :"<<nparser;
		if(parser->hasError())
		  {
		    //LOG_ROOT_INFO() << "closed2";
		    close();
		    return nullptr;
		  }
		len -= nparser;
		if(len >= (int)header_size)
		  {
		    LOG_ROOT_INFO() << "closed3";
		    close();
		    return nullptr;
		  }
		//begin = false;
		//剩余数据包含所有内容
	      }
	    while(!parser->isFinished());
	    //LOG_ROOT_INFO() << " len : "<<len << " client_parser.content_len : " <<client_parser.content_len;
	    if(client_parser.content_len +2<= len)
		  {
		    body.append(data,client_parser.content_len);
		    memmove(data,data+client_parser.content_len+2,
			    len - client_parser.content_len-2);
		    len -= client_parser.content_len+2;
		  }
		else
		  {
		    body.append(data,len);
		    left = client_parser.content_len - len + 2;
		    len = 0;
		    while(left >0)
		      {
			int nread = std::min((int64_t)left,header_size);
			int rt = readFixSize(data,nread);
			if(rt <= 0)
			  {
			    return nullptr;
			  }
			body.append(data,rt);
			left -= rt;
		      }
		    //去除trunk末尾的回车换行
		    body.resize(body.size()-2);
		  }
	  }
	while(!client_parser.chunks_done);
      }
    else
      {
	int64_t length = parser->getContentLength();
	if(length > 0)
	  {
	    body.resize(length);
	    int len = 0;
	    if(length >= remain)
	      {
		memcpy(&body[0],data,remain);
		len = remain;
	      }
	    else
	      {
		memcpy(&body[0],data,length);
		len = length;
	      }
	    length -= len;
	    if(length > 0)
	      {
		if(readFixSize(&body[len],length) <=0)
		  {
		    close();
		    return nullptr;
		  }
	      }
	  }
      }
    parser->getData()->setBody(body);
    return parser->getData();
  }
  
  int HttpConnection::sendRequest(HttpRequest::ptr resp)
  {
    std::stringstream ss;
    ss<<*resp;
    //LOG_ROOT_INFO() << *resp;
    return writeFixSize(ss.str().c_str(),ss.str().size());
  }
  
}
}
