#include "ByteArray.hpp"
#include "../logs.hpp"
#include <string>
#include <exception>
#include "../socket/endian.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include "../macro.hpp"
#include <iostream>
#include <iomanip>

namespace zhuyh
{
  static Logger::ptr s_logger = GET_LOGGER("system");
  ByteArray::Node::Node(size_t size)
    :m_buff(nullptr),
     m_size(size),
     m_next(nullptr)
  {
    m_buff = new char[size];
  }

  ByteArray::Node::~Node()
  {
    if(m_buff)
      delete [] m_buff;
    m_buff = nullptr;
    m_next = nullptr;
  }

  ByteArray::ByteArray(size_t blockSize)
    :m_endian(BYTE_ORDER),
     m_totalSize(blockSize),
     m_blockSize(blockSize),
     m_position(0),
     m_capacity(blockSize),
     m_head(nullptr),
     m_cur(nullptr)
  {
    if(blockSize == 0)
      {
	m_blockSize = 4096;
      }
    m_head = new Node(m_blockSize);
    m_cur = m_head;
  }

  ByteArray::~ByteArray()
  {
    Node* tmp = nullptr;
    for(auto cur = m_head;cur != nullptr;)
      {
	tmp = cur;
	cur = tmp->m_next;
	delete tmp;
      }
    m_head = nullptr;
    m_cur = nullptr;
  }

  void ByteArray::writeInt8(int8_t val)
  {
    write((void*)&val,sizeof(int8_t));
  }

  void ByteArray::writeInt16(int16_t val)
  {
    if(m_endian != BYTE_ORDER)
      val = byteSwap(val);
    write((void*)&val,sizeof(int16_t));

  }
  void ByteArray::writeInt32(int32_t val)
  {
    if(m_endian != BYTE_ORDER)
      val = byteSwap(val);
    write((void*)&val,sizeof(int32_t));
  }
  void ByteArray::writeInt64(int64_t val)
  {
    if(m_endian != BYTE_ORDER)
      val = byteSwap(val);
    write((void*)&val,sizeof(int64_t));
  }
  
  void ByteArray::writeUint8(uint8_t val)
  {
    write((void*)&val,sizeof(uint8_t));
  }
  
  void ByteArray::writeUint16(uint16_t val)
  {
    if(m_endian != BYTE_ORDER)
      val = byteSwap(val);
    write((void*)&val,sizeof(uint16_t));
  }
  void ByteArray::writeUint32(uint32_t val)
  {
    if(m_endian != BYTE_ORDER)
      val = byteSwap(val);
    write((void*)&val,sizeof(uint32_t));
  }
  void ByteArray::writeUint64(uint64_t val)
  {
    if(m_endian != BYTE_ORDER)
      val = byteSwap(val);
    write((void*)&val,sizeof(uint64_t));
  }


  //每个字节存7位,最高位为1表示还有数据
  void ByteArray::writeVint32(int32_t val)
  {
    uint8_t tmp[5];
    size_t pos = 0;
    uint32_t v = Encode(val);
    //LOG_INFO(s_logger) << "val after encode : "<<v;
    while(v >= 0x80)
      {
	tmp[pos++] = (v & 0x7f) | 0x80;
	v >>= 7;
	//LOG_INFO(s_logger)<<"Writing Byte Value : "<< (uint)tmp[pos-1];
      }
    tmp[pos++] = v;
    //LOG_INFO(s_logger)<<"Writing Byte Value : "<< (uint)tmp[pos-1];
    write((void*)tmp,pos);
  }
  void ByteArray::writeVint64(int64_t val)
  {
    uint8_t tmp[10];
    int pos = 0;
    uint64_t v = Encode(val);
    //LOG_INFO(s_logger) << "val after encode : "<<v;
    while(v >= 0x80)
      {
	tmp[pos++] = (v & 0x7f) | 0x80;
	//LOG_INFO(s_logger)<<"Writing Byte Value : "<< (uint64_t)tmp[pos-1];
	v >>= 7;
      }
    tmp[pos++] = v;
    //LOG_INFO(s_logger)<<"Writing Byte Value : "<< (uint64_t)tmp[pos-1];
    write((void*)tmp,pos);
  }
  
  void ByteArray::writeVuint32(uint32_t val)
  {
    uint8_t tmp[5];
    int pos = 0;
    while(val >= 0x80)
      {
	tmp[pos++] = (val & 0x7f) | 0x80;
	LOG_INFO(s_logger)<<"Writing Byte Value : "<< tmp[pos-1];
	val >>= 7;
      }
    tmp[pos++] = val;
    LOG_INFO(s_logger)<<"Writing Byte Value : "<< tmp[pos-1];
    write((void*)tmp,pos);
  }
  void ByteArray::writeVuint64(uint64_t val)
  {
    uint8_t tmp[5];
    int pos = 0;
    while(val >= 0x80)
      {
	tmp[pos++] = (val & 0x7f) | 0x80;
	val >>= 7;
      }
    tmp[pos++] = val;
    write((void*)tmp,pos);
  }
  
  void ByteArray::writeFloat(float val)
  {
    write((void*)&val,sizeof(float));
  }
  void ByteArray::writeDouble(double val)
  {
    write((void*)&val,sizeof(double));
  }

  //写入字符串
  //length : uint16_t buff : string
  void ByteArray::writeStringU16(const std::string& val)
  {
    uint64_t v = val.size();
    if( v > 65535) throw std::out_of_range("string length is out of 65535");
    uint16_t length = val.size();
    writeUint16(length);
    write((void*)val.c_str(),val.size());
  }
  //length : uint32_t buff : string
  void ByteArray::writeStringU32(const std::string& val)
  {
    uint32_t length = val.size();
    writeUint32(length);
    write((void*)val.c_str(),val.size());
  }
  //length : uint64_t buff : string
  void ByteArray::writeStringU64(const std::string& val)
  {
    uint64_t length = (uint64_t)val.size();
    writeUint64(length);
    write((void*)val.c_str(),val.size());
  }
  //length : 无符号varint64_t buff : string
  void ByteArray::writeStringV64(const std::string& val)
  {
    uint64_t length = (uint64_t)val.size();
    writeVuint64(length);
    write((void*)val.c_str(),val.size());
  }
  //不带长度信息
  void ByteArray::writeString(const std::string& val)
  {
    write((void*)val.c_str(),val.size());
  }
  //读取固定长度数据
  int8_t    ByteArray::readInt8()
  {
    uint8_t v;
    read((void*)&v,sizeof(int8_t));
    return v;
  }
  int16_t   ByteArray::readInt16()
  {
    int16_t v;
    read((void*)&v,sizeof(int16_t));
    if(m_endian == BYTE_ORDER)
      return v;
    else
      return byteSwap(v);
  }
  int32_t   ByteArray::readInt32()
  {
    int32_t v;
    read((void*)&v,sizeof(int32_t));
    if(m_endian == BYTE_ORDER)
      return v;
    else
      return byteSwap(v);
  }
  int64_t   ByteArray::readInt64()
  {
    int64_t v;
    read((void*)&v,sizeof(int64_t));
    if(m_endian == BYTE_ORDER)
      return v;
    else
      return byteSwap(v);
  }
  
  uint8_t   ByteArray::readUint8()
  {
    uint8_t v;
    read((void*)&v,sizeof(uint8_t));
    return v;
  }
  uint16_t  ByteArray::readUint16()
  {
    uint16_t v;
    read((void*)&v,sizeof(uint16_t));
    if(m_endian == BYTE_ORDER)
      return v;
    else
      return byteSwap(v);
  }
  uint32_t  ByteArray::readUint32()
  {
    uint32_t v;
    read((void*)&v,sizeof(uint32_t));
    if(m_endian == BYTE_ORDER)
      return v;
    else
      return byteSwap(v);
  }
  uint64_t  ByteArray::readUint64()
  {
    uint64_t v;
    read((void*)&v,sizeof(uint64_t));
    if(m_endian == BYTE_ORDER)
      return v;
    else
      return byteSwap(v);
  }
  
  //读取被压缩的各种类型数据
  int32_t   ByteArray::readVint32()
  {
    uint8_t tmp;
    uint32_t v = 0;
    for(size_t i=0;i<32;i+=7)
      {
	read((void*)&tmp,sizeof(uint8_t));
	//LOG_INFO(s_logger)<<"Reading Byte Value : "<< (uint)tmp;
	if(tmp >= 0x80)
	  {
	    //说明数据不对
	    if(i+7 > 32 ) throw std::logic_error("broken data");
	    v |= ((uint32_t)(tmp & 0x7f)) << i;
	  }
	else
	  {
	    v |= ((uint32_t)tmp) << i;
	    break;
	  }
      }
    return Decode(v);
  }
  
  int64_t   ByteArray::readVint64()
  {
    uint8_t tmp;
    uint64_t v = 0;
    for(uint64_t i=0;i<64;i+=7)
      {
	read((void*)&tmp,sizeof(uint8_t));
	if(tmp >= 0x80)
	  {
	    if(i+7 > 64 ) throw std::logic_error("broken data");
	    v |= ((uint64_t)(tmp & 0x7f)) << i;
	  }
	else
	  {
	    v |= ((uint64_t)tmp) << i;
	    break;
	  }
      }
    return Decode(v);
  }
  
  uint32_t  ByteArray::readVuint32()
  {
    uint8_t tmp;
    uint32_t v = 0;
    for(int i=0;i<64;i+=7)
      {
	read((void*)&tmp,sizeof(uint8_t));
	if(tmp >= 0x80)
	  {
	    if(i+7 > 32 ) throw std::logic_error("broken data");
	    v |= ((uint32_t)(tmp & 0x7f)) << i;
	  }
	else
	  {
	    v |= ((uint32_t)tmp) << i;
	    break;
	  }
      }
    return v;
  }
  
  uint64_t  ByteArray::readVuint64()
  {
    uint8_t tmp;
    uint64_t v = 0;
    for(uint64_t i=0;i<64;i+=7)
      {
	read((void*)&tmp,sizeof(uint8_t));
	if(tmp >= 0x80)
	  {
	    if(i+7 > 64 ) throw std::logic_error("broken data");
	    v |= ((uint64_t)(tmp & 0x7f)) << i;
	  }
	else
	  {
	    v |= ((uint64_t)tmp) << i;
	    break;
	  }
      }
    return v;
  }

  //读取浮点数
  float  ByteArray::readFloat()
  {
    float v;
    read((void*)&v,sizeof(float));
    return v;
  }
  double ByteArray::readDouble()
  {
    double v;
    read((void*)&v,sizeof(double));
    return v;
  }
  //length : uint16_t buff : string
  std::string ByteArray::readStringU16()
  {
    uint16_t length = readUint16();
    std::string str;
    str.resize(length);
    read(&str[0],length);
    return str;
  }
  
  //length : uint32_t buff : string
  std::string ByteArray::readStringU32()
  {
    uint32_t length = readUint32();
    std::string str;
    str.resize(length);
    read(&str[0],length);
    return str;
  }
  //length : uint64_t buff : string
  std::string ByteArray::readStringU64()
  {
    uint64_t length = readUint64();
    std::string str;
    str.resize(length);
    read(&str[0],length);
    return str;
  }
  //length : 无符号varint64_t buff : string
  std::string ByteArray::readStringV64()
  {
    uint64_t length = readVuint64();
    std::string str;
    str.resize(length);
    read(&str[0],length);
    return str;
  }

  //清空缓存区
  void ByteArray::clear()
  {
    m_totalSize = 0;
    m_position = 0;
    m_capacity = m_blockSize;
    Node* cur = m_head->m_next;
    Node* tmp = nullptr;
    while(cur)
      {
	tmp = cur;
	cur = cur->m_next;
	delete tmp;
      }
    m_cur = m_head;
    m_cur -> m_next = nullptr;
  }

  void ByteArray::write(const void* buff,size_t size)
  {
    if(size == 0) return ;
    allocate(size);
    //当前块剩余大小
    size_t rm = 0;
    size_t npos = 0;
    size_t nblockPos = 0;
    size_t nwrite = 0;
    //最后一次是否填满
    int newtag = 1;
    while(size > 0)
      {
	//本块剩余大小
	rm = m_blockSize - m_position % m_blockSize;
	nblockPos = m_position % m_blockSize;
	//本次要写的数据
	nwrite = std::min(size,rm);
	memcpy(m_cur->m_buff + nblockPos,(char*)buff + npos,nwrite);
	ASSERT(size >= nwrite);
	size -= nwrite;
	npos += nwrite;
	m_position += nwrite;
	//写满一块则指向下一块
	if(m_blockSize <= nwrite + nblockPos )
	  {
	    //最后一次填满了
	    if(size == 0)
	      newtag = 1;
	    else
	      m_cur = m_cur->m_next;
	  }
	//just in case
	if(m_cur == nullptr) throw std::logic_error("broken bytearray");
	//LOG_INFO(s_logger)<<"Writing "<<nwrite<<" Bytes";
      }
    //写满一块则再分配一块空间
    if(m_position >= m_totalSize)
      m_totalSize = m_position;
    if(newtag)
      {
	allocate(m_blockSize);
	m_cur=m_cur->m_next;
      }

  }

  void ByteArray::read(void* buff,size_t size)
  {
    //数据不够读
    //    LOG_INFO(s_logger) << "Readable : " << getReadableSize();
    if(size > getReadableSize() ) throw std::out_of_range("data is not available now");
    size_t rm = 0;
    size_t npos = 0;
    size_t nblockPos = 0;
    size_t nread = 0;
    while(size > 0)
      {
	rm = m_blockSize - m_position % m_blockSize;
	nblockPos = m_position % m_blockSize;
	nread = std::min(size,rm);
	memcpy((char*)buff + npos,m_cur->m_buff + nblockPos,nread);
	m_position += nread;
	npos += nread;
	//just in case
	ASSERT(size >= nread);
	size -= nread;
	if(m_blockSize <= nread + nblockPos)
	  m_cur = m_cur->m_next;
	if(m_cur == nullptr) throw  std::logic_error("broken data");
      }
  }
  
  void ByteArray::read(void* buff,size_t size,size_t pos) const
  {
    if(size > (m_totalSize - pos) ) throw std::out_of_range("data is not available now");
    Node* cur = m_head;
    //找到pos所在的块
    int step = pos / m_blockSize;
    while(step--)
      {
	cur = cur->m_next;
	if(cur == nullptr) throw std::logic_error("broken data");
      }
    size_t rm = 0;
    size_t npos = 0;
    size_t nblockPos = 0;
    size_t nread =0;
    while(size > 0)
      {
	rm = m_blockSize - pos % m_blockSize;
	nblockPos = pos % m_blockSize;
	nread = std::min(size,rm);
	memcpy((char*)buff + npos,cur->m_buff + nblockPos,nread);
	npos += nread;
	pos += nread;
	ASSERT(size >= nread);
	size -= nread;
	if(m_blockSize <= nread + nblockPos)
	  {
	    cur = cur->m_next;
	  }
	if(cur == nullptr) throw  std::logic_error("broken data");
      }
  }

  void ByteArray::setPosition(size_t v)
  {
    if( v > m_capacity) throw std::out_of_range("position "+std::to_string(v)+
						" is bigger than capacity : "+
						std::to_string(m_capacity));
    if( v >= m_totalSize)
      {
	m_totalSize = v;
      }
    Node* cur = m_head;
    int step = v / m_blockSize;
    while(step--)
      {
	cur = cur->m_next;
	if(cur == nullptr) throw std::logic_error("broken data");
      }
    m_cur = cur;
    m_position = v;
  }

  bool ByteArray::dumpToFile(const std::string& path) const
  {
    if(m_position == m_totalSize)
      {
	LOG_WARN(s_logger) << "no data available";
	return true;
      }
    std::ofstream  ofs;
    ofs.open(path,std::ios::trunc | std::ios::binary);
    if(!ofs)
      {
	LOG_ERROR(s_logger) << "file open failed , error : "<<strerror(errno)
			    << " errno : "<<errno;
	return false;
      }
    size_t pos = m_position;
    Node* cur = m_cur;
    size_t size = getReadableSize();
    size_t nwrite = 0;
    size_t rm = 0;
    size_t nblockPos = 0;
    while(size > 0)
      {
	rm = m_blockSize - pos % m_blockSize;
	nblockPos = pos % m_blockSize;
	nwrite = std::min(rm,size);
	try
	  {
	    ofs.write(cur->m_buff + nblockPos,nwrite);
	  }
	catch(std::exception& e)
	  {
	    LOG_ERROR(s_logger) << "dump to file failed , error : "<<e.what();
	    return false;
	  }
	pos += nwrite;
	ASSERT(size >= nwrite);
	size -= nwrite;
	cur = cur -> m_next;
	if(cur == nullptr) throw  std::logic_error("broken data");
      }
    return true;
  }

  bool ByteArray::loadFromFile(const std::string& path)
  {
    std::ifstream  ifs;
    ifs.open(path, std::ios::binary);
    if(!ifs)
      {
	LOG_ERROR(s_logger) << "file open failed , error : "<<strerror(errno)
			    << " errno : "<<errno;
	return false;
      }
    std::shared_ptr<char> buff(new char[m_blockSize],[] (char* ptr) { delete [] ptr;});
    while(!ifs.eof())
      {
	try
	  {
	    ifs.read(buff.get(),m_blockSize);
	  }
	catch(std::exception& e)
	  {
	    LOG_ERROR(s_logger) << "load from file failed , error : "<<e.what();
	    return false;
	  }
	write(buff.get(),ifs.gcount());
      }
    return true;
  }

  
  std::string ByteArray::dump() const
  {
    std::string str;
    size_t size = getReadableSize();
    if(size == 0 )
      {
	return "";
      }
    str.resize(size);
    //不希望修改m_position,该函数肯抛出异常
    read(&str[0],str.size(),m_position);
    return str;
  }

  std::string ByteArray::dumpToHex() const
  {
    std::string str = dump();
    std::stringstream ss;

    for(size_t i = 0;i<str.size();i++)
      {
	if( i > 0 && i%32 == 0) ss << std::endl;
	ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i] <<" ";
      }
    return ss.str();
  }
  
  uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers,uint64_t size) const
  {
    size = size > getReadableSize() ? getReadableSize() : size;
    if(size == 0) return 0;
    size_t pos = m_position;
    size_t len = size;
    size_t rm = 0;
    size_t nblockPos = 0;
    struct iovec iov;
    Node* cur = m_cur;
    while(size)
      {
	nblockPos = pos % m_blockSize;
	rm = m_blockSize - pos % m_blockSize;
	int nread = std::min(size,(uint64_t)rm);
	iov.iov_base = cur->m_buff + nblockPos;
	iov.iov_len = nread;
	size -= nread;
	pos += nread;;
	cur = cur->m_next;
	buffers.push_back(iov);
      }
    return len;
  }
  //从pos开始获取
  uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers,uint64_t size
				     ,uint64_t pos) const
  {
    if(pos >= m_totalSize) return 0;
    size = size > (m_totalSize - pos) ? getReadableSize() : size;
    if(size == 0) return 0;
    size_t len = size;
    size_t rm = 0;
    size_t nblockPos = 0;
    struct iovec iov;
    Node* cur = m_cur;
    while(size)
      {
	nblockPos = pos % m_blockSize;
	rm = m_blockSize - pos % m_blockSize;
	int nread = std::min(size,(uint64_t)rm);
	iov.iov_base = cur->m_buff + nblockPos;
	iov.iov_len = nread;
	size -= nread;
	pos += nread;;
	cur = cur->m_next;
	buffers.push_back(iov);
      }
    return len;
  }

  //获取可写的缓存，返回实际长度
  uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers,uint64_t size)
  {
    allocate(size);
    size_t len = size;
    size_t rm = 0;
    size_t pos = m_position;
    size_t nblockPos = 0;
    struct iovec iov;
    Node* cur = m_cur;
    while(size)
      {
	nblockPos = pos % m_blockSize;
	rm = m_blockSize - pos % m_blockSize;
	uint64_t nread = std::min(size,(uint64_t)rm);
	iov.iov_base = cur->m_buff + nblockPos;
	iov.iov_len = nread;
	size -= nread;
	pos += nread;
	cur = cur->m_next;
	buffers.push_back(iov);
	//LOG_ROOT_INFO() << "pos : "<<pos<<" nblockSize : "<<nblockPos
	//		<<" rm : " <<rm<<" nread : "<<nread;
      }
    return len;
  }

  void ByteArray::allocate(size_t size)
  {
    //剩余空间足够
    if( getRemain() > size)
      {
	return;
      }
    Node* cur = m_cur;
    //需要额外增加的大小
    size -= getRemain();
    while(size)
      {
	Node* tmp = new Node(m_blockSize);
	m_capacity+=m_blockSize;
	size -= std::min(size,m_blockSize);
	//LOG_INFO(s_logger) << "Allocating Block...size : "<<size
	//		   <<" blockSize : "<<m_blockSize;
	cur->m_next = tmp;
	cur = cur->m_next;
      }
    // LOG_INFO(s_logger) << "Capacity : "<<m_capacity
    // 		       << " Remain : "<<getRemain()
    // 		       <<" ReadableSize : "<<getReadableSize();
  }
  
}
