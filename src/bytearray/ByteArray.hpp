#pragma once

#include<memory>
#include<string>
#include<stdint.h>
#include<vector>
#include<sys/types.h>
#include<sys/socket.h>

namespace zhuyh
{
  /*
    线程不安全
   */
  class ByteArray
  {
  public:
    typedef std::shared_ptr<ByteArray> ptr;
    struct Node
    {
      Node(size_t size);
      ~Node();
      char* m_buff;
      size_t m_size;
      Node* m_next;
    };
    ByteArray(size_t blockSize = 0);
    ~ByteArray();
    //写入固定长度的各种类型数据
    void writeInt8(int8_t val);
    void writeInt16(int16_t val);
    void writeInt32(int32_t val);
    void writeInt64(int64_t val);
    void writeUint8(uint8_t val);
    void writeUint16(uint16_t val);
    void writeUint32(uint32_t val);
    void writeUint64(uint64_t val);

    //写入被压缩的各种类型数据
    void writeVint32(int32_t val);
    void writeVint64(int64_t val);
    void writeVuint32(uint32_t val);
    void writeVuint64(uint64_t val);

    //写入浮点数
    void writeFloat(float val);
    void writeDouble(double val);

    //写入字符串
    //length : uint16_t buff : string
    void writeStringU16(const std::string& val);
    //length : uint32_t buff : string
    void writeStringU32(const std::string& val);
    //length : uint64_t buff : string
    void writeStringU64(const std::string& val);
    //length : 无符号varint64_t buff : string
    void writeStringV64(const std::string& val);
    //不带长度信息
    void writeString(const std::string& val);

    //读取固定长度的数据
    /*
     *@exception : std::logic_error
     */
    int8_t    readInt8();
    int16_t   readInt16();
    int32_t   readInt32();
    int64_t   readInt64();
    uint8_t   readUint8();
    uint16_t  readUint16();
    uint32_t  readUint32();
    uint64_t  readUint64();

    //读取被压缩的各种类型数据
    int32_t   readVint32();
    int64_t   readVint64();
    uint32_t  readVuint32();
    uint64_t  readVuint64();

    //读取浮点数
    float  readFloat();
    double readDouble();

    //读取字符串
    //length : uint16_t buff : string
    std::string readStringU16();
    //length : uint32_t buff : string
    std::string readStringU32();
    //length : uint64_t buff : string
    std::string readStringU64();
    //length : 无符号varint64_t buff : string
    std::string readStringV64();

    //清空缓存
    void clear();
  private:
    //写入size字节数据
    void write(const void* buff,size_t size);
    //读取m_pos开始读取size字节数据
    void read(void* buff,size_t size);
    //从pos开始读取size大小数据
    void read(void* buff,size_t size,size_t pos) const;
  public:
    size_t getPosition() const
    {
      return m_position;
    }
    void setPosition(size_t v);
  public:
    bool dumpToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

    //获取Node的buff大小
    size_t getBlockSize() const
    {
      return m_blockSize;
    }
    //获取可读大小
    size_t getReadableSize() const
    {
      return m_totalSize - m_position;
    }
    bool isLittleEndian() const
    {
      return m_endian == LITTLE_ENDIAN;
    }
    void setLittleEndian(bool v)
    {
      if(v) m_endian = LITTLE_ENDIAN;
      else m_endian = BIG_ENDIAN;
    }
    /*
     *@brief : 输出为字符串，非二进制安全
     *@exception : logic_error
     *@return : ByteArray中可读字符串
     */
    std::string dump() const;
    //输出16进制字符串，因此安全
    std::string dumpToHex() const;
    /*
     *@function : 获取可读取的缓存，返回实际长度
     */
    
    uint64_t getReadBuffers(std::vector<iovec>& buffers,uint64_t size = -1) const;
    //从pos开始读取
    uint64_t getReadBuffers(std::vector<iovec>& buffers,uint64_t size ,uint64_t pos) const;

    //获取可写的缓存，返回实际长度
    uint64_t getWriteBuffers(std::vector<iovec>& buffers,uint64_t size = -1);

    //获取总数据大小
    size_t getDataSize()
    {
      return m_totalSize;
    }
    size_t getCapacity()
    {
      return m_capacity;
    }
    size_t getRemain()
    {
      return m_capacity - m_position;
    }
  private:
    //属于容量小与size则添加容量，否则
    void allocate(size_t size);
  private:
    //将int类型压缩
    uint32_t Encode(int32_t v)
    {
      if(v < 0)
	{
	  return ((uint32_t)-v)*2-1;
	}
      return ((uint32_t)v)*2;
    }
    uint64_t Encode(int64_t v)
    {
      if(v < 0)
	{
	  return ((uint64_t)-v)*2-1;
	}
      return ((uint64_t)v)*2;
    }
    
    //将int类型解压缩
    int32_t Decode(uint32_t v)
    {
      return (int32_t)( (v>>1)^-(v & 1));
    }
    int64_t Decode(uint64_t v)
    {
      return (int64_t)( (v>>1)^-(v & 1));
    }
  private:
    uint32_t m_endian = LITTLE_ENDIAN;
    //数据的大小
    size_t m_totalSize;
    //没块大小
    size_t m_blockSize;
    //当前写的位置
    size_t m_position;
    //容量
    size_t m_capacity;
    Node* m_head = nullptr;
    //当前写的节点
    Node* m_cur = nullptr;
  };
  
}
