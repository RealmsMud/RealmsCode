/*
 * toNum.hpp
 *   toNum template functions
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later

 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

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

template <class Type>
Type toNumSV(const std::string_view &fromStr) {
    Type toReturn = static_cast<Type>(0);

    try {
        toReturn = boost::lexical_cast<Type>(fromStr);
    } catch (boost::bad_lexical_cast &) {
        // And do nothing
    }

    return (toReturn);
}
