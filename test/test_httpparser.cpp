#include "all.hpp"
#include<bits/stdc++.h>

using namespace zhuyh;

// char request_test[] = "GET / HTTP/1.1\r\n"
//     "Content-Length: 11\r\n"
//   "Host: www.baidu.com\r\n\r\n"
//   "hello world";

char request_test2[] = "GET /5bU_dTmfKgQFm2e88IuM_a/w.gif?q=ragel%BD%E2%CE%F6http&fm=se&T=1584331494&y=F7C9BF7E&rsv_cache=0&rsv_pre=0&rsv_reh=85_85_89_85_85_85_85_85_85|_85&rsv_scr=1000_1421_27_0_1080_1920&rsv_psid=73EBD0D7D9B774D47A1D510A32AF9AC5&rsv_pstm=1583065479&rsv_idc=3&rsv_sid=30970_1430_21095_30825_30824_26350_30717&cid=0&qid=e01ca7270000e74e&t=1584331494790&rsv_iorr=1&rsv_tn=baidu&rsv_ssl=1&path=https%3A%2F%2Fwww.baidu.com%2Fs%3Fie%3Dutf-8%26f%3D8%26rsv_bp%3D1%26rsv_idx%3D1%26tn%3Dbaidu%26wd%3Dragel%25E8%25A7%25A3%25E6%259E%2590http%26oq%3Duri%26rsv_pq%3Ddfc4c6470001c18f%26rsv_t%3Dd7f2TxlIneCu1kydaPonZrfI4Vz6589mGqJGjdXix9wlt8sy%252FW%252B4KGYkh9E%26rqlang%3Dcn%26rsv_enter%3D1%26rsv_dl%3Dtb%26inputT%3D3517%26rsv_sug3%3D42%26rsv_sug1%3D29%26rsv_sug7%3D101%26rsv_sug2%3D0%26rsv_sug4%3D4501&rsv_did=97cc01e0c4c5511db38347349dfb5ad7 HTTP/1.1\r\n"
  "Host: sp0.baidu.com\r\n"
  "Connection: keep-alive\r\n"
  "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.122 Safari/537.36\r\n"
  "Sec-Fetch-Dest: image\r\n"
  "Accept: image/webp,image/apng,image/*,*/*;q=0.8\r\n"
  "Sec-Fetch-Site: same-site\r\n"
  "Sec-Fetch-Mode: no-cors\r\n"
  "Referer: https://www.baidu.com/s?ie=utf-8&f=8&rsv_bp=1&rsv_idx=1&tn=baidu&wd=ragel%E8%A7%A3%E6%9E%90http&oq=uri&rsv_pq=dfc4c6470001c18f&rsv_t=d7f2TxlIneCu1kydaPonZrfI4Vz6589mGqJGjdXix9wlt8sy%2FW%2B4KGYkh9E&rqlang=cn&rsv_enter=1&rsv_dl=tb&inputT=3517&rsv_sug3=42&rsv_sug1=29&rsv_sug7=101&rsv_sug2=0&rsv_sug4=4501\r\n"
  "Accept-Encoding: gzip, deflate, br\r\n"
  "Accept-Language: zh-CN,zh;q=0.9\r\n"
  "Cookie: BIDUPSID=73EBD0D7D9B774D47A1D510A32AF9AC5; PSTM=1583065479; BAIDUID=CBAEDA3694434F41A8BA2F6026CA803C:FG=1; BDORZ=B490B5EBF6F3CD402E515D22BCDA1598; H_PS_PSSID=30970_1430_21095_30825_30824_26350_30717; delPer=0; PSINO=3; ZD_ENTRY=bai\r\n";

char request_test3[]  = "GET http://www.baidu.com/hm.gif?cc=1&ck=1&cl=24-bit&ds=1920x1080&vl=1250&et=0&ja=0&ln=zh-cn&lo=0&lt=1584330098&rnd=1961655666&si=3eec0b7da6548cf07db3bc477ea905ee&v=1.2.68&lv=3&sn=43421&ct=!!&tt=C%20%E5%BA%93%E5%87%BD%E6%95%B0%20%E2%80%93%20atoi()%20%7C%20%E8%8F%9C%E9%B8%9F%E6%95%99%E7%A8%8B HTTP/1.1\r\n"
"Host: hm.baidu.com\r\n"
"Connection: keep-alive\r\n"
"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.122 Safari/537.36\r\n"
"Sec-Fetch-Dest: image\r\n"
"Accept: image/webp,image/apng,image/*,*/*;q=0.8\r\n"
"Sec-Fetch-Site: cross-site\r\n"
"Sec-Fetch-Mode: no-cors\r\n"
"Referer: https://www.runoob.com/cprogramming/c-function-atoi.html\r\n"
"Accept-Encoding: gzip, deflate, br\r\n"
"Accept-Language: zh-CN,zh;q=0.9\r\n"
"Cookie: BIDUPSID=73EBD0D7D9B774D47A1D510A32AF9AC5; PSTM=1583065479; HMACCOUNT=462D07BF46A59020; BAIDUID=CBAEDA3694434F41A8BA2F6026CA803C:FG=1; BDORZ=B490B5EBF6F3CD402E515D22BCDA1598; HMVT=6bcd52f51e9b3dce32bec4a3997715ac|1584331417|; H_PS_PSSID=30970_1430_21095_30824_26350_30717; delPer=0; PSINO=3\r\n\r\n";

// char request_test2[] = "GET /5bU_dTmfKgQFm2e88IuM_a/w.gif?q=1&b=2&t=ragel%BD%E2%CE%F6http&path=https%3A%2F%2Fwww.baidu.com%2Fs%3Fie%3Dutf-8%26f%3D8%26rsv_bp%3D1%26rsv_idx%3D1%26tn%3Dbaidu%26wd%3Dragel%25E8%25A7%25A3%25E6%259E%2590http%26oq%3Duri%26rsv_pq%3Ddfc4c6470001c18f%26rsv_t%3Dd7f2TxlIneCu1kydaPonZrfI4Vz6589mGqJGjdXix9wlt8sy%252FW%252B4KGYkh9E%26rqlang%3Dcn%26rsv_enter%3D1%26rsv_dl%3Dtb%26inputT%3D3517%26rsv_sug3%3D42%26rsv_sug1%3D29%26rsv_sug7%3D101%26rsv_sug2%3D0%26rsv_sug4%3D4501 HTTP/1.1\r\n"
//   "Host: sp0.baidu.com\r\n"
//   "Connection: keep-alive\r\n"
//   "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.122 Safari/537.36\r\n"
//   "Sec-Fetch-Dest: image\r\n"
//   "Accept: image/webp,image/apng,image/*,*/*;q=0.8\r\n"
//   "Sec-Fetch-Site: same-site\r\n"
//   "Sec-Fetch-Mode: no-cors\r\n"
//   "Referer: https://www.baidu.com/s?ie=utf-8&f=8&rsv_bp=1&rsv_idx=1&tn=baidu&wd=ragel%E8%A7%A3%E6%9E%90http&oq=uri&rsv_pq=dfc4c6470001c18f&rsv_t=d7f2TxlIneCu1kydaPonZrfI4Vz6589mGqJGjdXix9wlt8sy%2FW%2B4KGYkh9E&rqlang=cn&rsv_enter=1&rsv_dl=tb&inputT=3517&rsv_sug3=42&rsv_sug1=29&rsv_sug7=101&rsv_sug2=0&rsv_sug4=4501\r\n"
//   "Accept-Encoding: gzip, deflate, br\r\n"
//   "Accept-Language: zh-CN,zh;q=0.9\r\n"
//   "Cookie: BIDUPSID=73EBD0D7D9B774D47A1D510A32AF9AC5; PSTM=1583065479; BAIDUID=CBAEDA3694434F41A8BA2F6026CA803C:FG=1; BDORZ=B490B5EBF6F3CD402E515D22BCDA1598; H_PS_PSSID=30970_1430_21095_30825_30824_26350_30717; delPer=0; PSINO=3; ZD_ENTRY=bai\r\n";

void test_request()
{
 http::HttpRequestParser::ptr parser(new http::HttpRequestParser());
 size_t rt = parser->execute(request_test3,strlen(request_test3));
 http::HttpRequest::ptr req = parser->getData();
 LOG_ROOT_INFO() << "rt : " << rt << " has_error "<<parser->hasError()
		 <<" isFinished : "<<parser->isFinished()
		 <<" total : " <<strlen(request_test3);

 std::cout<<*req<<std::endl;
 req->initParam();
 const auto& params = req->getParams();
 for(const auto& item : params){
   std::cout<<item.first<<":"<<item.second<<std::endl;
 }
 std::cout<<std::endl<<"Cookies\n";
 const auto& cookies = req->getCookies();
 for(const auto& item : cookies){
   std::cout<<item.first<<":"<<item.second<<std::endl;
 }
 //std::cout<<*(parser->getData())<<std::endl;
 std::cout<<"content-length : " << parser->getContentLength()<<std::endl;
 std::cout<<std::string(request_test3,parser->getContentLength())<<std::endl;
}

char response[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
	"<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
	"</html>\r\n";
void test_response()
{
  http::HttpResponseParser::ptr parser(new http::HttpResponseParser());
  size_t rt = parser->execute(response,strlen(response),0);
  LOG_ROOT_INFO() << "rt : " << rt << " has_error "<<parser->hasError()
		  <<" isFinished : "<<parser->isFinished()
		  <<" total : " <<strlen(response);
  std::cout<<*(parser->getData())<<std::endl;
  std::cout<<"content-length : " << parser->getContentLength()<<std::endl;
   std::cout<<std::string(response,parser->getContentLength())<<std::endl;
}
int main()
{
  test_request();
  test_response();
}
