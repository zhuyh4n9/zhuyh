#pragma once
#include <memory>
#include <stdint.h>
#include <type_traits>

namespace zhuyh {

template<class Type, typename std::enable_if<std::is_integral<Type>::value && !std::is_same<bool, Type>::value, bool>::type v = true>
class BitMap {
public:
    BitMap(uint32_t size)
        : m_size(size),
          m_data((size - 1 + sizeof(Type)*8) / (8*sizeof(Type))) {
    }
    bool haveIndex(uint32_t index) {
        return (((m_data[index/(sizeof(Type) * 8)]) >> (index%sizeof(Type)*8)) & 0x01);
    }
    bool freeIndex(uint32_t index) {
        return 
    }
    uint32_t allocIndex();
    bool resize(uint32_t size);
    uint32_t getSize();
private:
    std::vector<Type> m_data;
    uint32_t size;
};

}