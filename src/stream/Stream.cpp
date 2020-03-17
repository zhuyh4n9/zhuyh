#include"Stream.hpp"


namespace zhuyh
{
  int Stream::readFixSize(void* buff,size_t length)
  {
    size_t offset = 0;
    size_t left = length;
    int len;
    while(length)
      {
	len = read((char*)buff+offset,length);
	if(len <= 0 )
	  return len;
	offset+=len;
	length-=len;
      }
    return left;
  }
  
  int Stream::readFixSize(ByteArray::ptr ba,size_t length)
  {
    size_t left = length;
    int len;
    while(length)
      {
	len = read(ba,length);
	if(len <= 0 )
	  return len;
	length-=len;
      }
    return left;
  }
  
  int Stream::writeFixSize(const void* buff,size_t length)
  {
    size_t offset = 0;
    size_t left = length;
    int len;
    while(length)
      {
	len = write((char*)buff+offset,length);
	if(len <= 0 )
	  return len;
	offset+=len;
	length-=len;
      }
    return left;
  }
  int Stream::writeFixSize(ByteArray::ptr ba,size_t length)
  {
    size_t left = length;
    int len;
    while(length)
      {
	len = read(ba,length);
	if(len <= 0 )
	  return len;
	length-=len;
      }
    return left;
  }
}
