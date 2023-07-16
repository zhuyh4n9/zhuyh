#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <concepts>
#include <type_traits>
#include <cstdlib>
#include <sstream>
namespace zhuyh {

template<class F, class T>
class Lexical_Cast {
public:
    T operator()(const F& from) {
        return T(from);
    }
};

template<class T>
requires std::is_integral<T>::value
class Lexical_Cast<std::string, T> {
public:
    T operator()(const std::string& from) {
        return T(strtoll(from.c_str(), NULL, 10));
    }
};

template<>
class Lexical_Cast<std::string, double> {
public:
    double operator()(const std::string &from) {
        return strtod(from.c_str(), NULL);
    }
};

template<>
class Lexical_Cast<std::string, float> {
public:
    double operator()(const std::string &from) {
        return strtof(from.c_str(), NULL);
    }
};


template<>
class Lexical_Cast<std::string, long double> {
public:
    double operator()(const std::string &from) {
        return strtold(from.c_str(), NULL);
    }
};


template<class F>
class Lexical_Cast<F, std::string> {
public:
    std::string operator()(const F& from) {
        // std::format is not supported yet
        std::stringstream ss;
        ss << from;
        return ss.str();
    }
};

}