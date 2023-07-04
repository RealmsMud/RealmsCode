#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#pragma once

template <typename T>
std::string join(const T& v, const std::string& delim) {
    std::ostringstream s;

    typename T::const_iterator it = v.begin();
    typename T::const_iterator itEnd = v.end();

    if(it != itEnd) {
        s << *it;
        ++it;
    }
    for(;it!=itEnd; ++it) {
        s << delim;
        s << *it;
    }
    return s.str();
}

template <typename T>
std::string mjoin(const T& v, const std::string& delim) {
    std::ostringstream s;

    typename T::const_iterator it = v.begin();
    typename T::const_iterator itEnd = v.end();

    if(it != itEnd) {
        s << it->second;
        ++it;
    }
    for(;it!=itEnd; ++it) {
        s << delim;
        s << it->second;
    }
    return s.str();
}

