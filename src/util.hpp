#pragma once

#include <vector>
#include <string>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <execinfo.h>
#include <cxxabi.h>
#include "log.hpp"
#include <fcntl.h>
#include <boost/lexical_cast.hpp>
#include <type_traits>

namespace zhuyh {

    pid_t getThreadId();
    uint32_t getCoroutineId();
    pid_t getProcessId();
    const time_t& start();
    time_t getCurrentTime();
    time_t getElapseTime();
    time_t getCurrentTimeMS();
    time_t getElapseTimeMS();
    //打印函数调用栈
    void BackTrace(std::vector<std::string>& bt,int size,int skip);
    std::string Bt2Str(int size,int skip,const std::string& prefix);
    std::string getEnv(const std::string& envName);
    time_t str2Time(const char* str,const char* fmt);
    std::string time2Str(time_t t,const char* fmt);
    template<class T,class V = std::string>
    T getParamValue(const std::map<std::string,V>& mp,const std::string& key,const T& dft=T()) {
        auto it = mp.find(key);
        if(it == mp.end())
            return dft;
        try {
            return boost::lexical_cast<T>(it->second);
        } catch(...) {
            return dft;
        }
    }
}
