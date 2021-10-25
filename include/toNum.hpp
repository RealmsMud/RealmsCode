//
// Created by jason on 10/18/21.
//

#ifndef REALMSPYTHON_TONUM_HPP
#define REALMSPYTHON_TONUM_HPP

#include <string>
#include <boost/lexical_cast.hpp>

template <class Type>
Type toNum(const std::string &fromStr) {
    Type toReturn = static_cast<Type>(0);

    try {
        toReturn = boost::lexical_cast<Type>(fromStr);
    } catch (boost::bad_lexical_cast &) {
        // And do nothing
    }

    return (toReturn);
}


#endif //REALMSPYTHON_TONUM_HPP
