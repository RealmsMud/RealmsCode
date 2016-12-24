#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#ifndef JOIN_HPP
#define JOIN_HPP

template <typename T>
std::string join(const T& v, const std::string& delim) {
    std::ostringstream s;

    typename T::const_iterator itBegin = v.begin();
    typename T::const_iterator itEnd = v.end();

    if(itBegin != itEnd) {
        s << *itBegin;
        ++itBegin;
    }
    for(;itBegin!=itEnd; ++itBegin) {
        s << delim;
        s << *itBegin;
    }
    return s.str();
}

#endif
