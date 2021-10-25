/*
 * range.cpp
 *   CatRef range object
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "range.hpp"

#include <ostream>      // for basic_ostream::operator<<, operator<<, basic_...
#include <sstream>
#include <string>       // for char_traits, operator<<

//*********************************************************************
//                      Range
//*********************************************************************

Range::Range() {
    high = 0;
}

Range& Range::operator=(const Range& r) = default;

bool Range::operator==(const Range& r) const {
    return !(low != r.low ||
             high != r.high);
}
bool Range::operator!=(const Range& r) const {
    return(!(*this==r));
}


//*********************************************************************
//                      belongs
//*********************************************************************

bool Range::belongs(const CatRef& cr) const {
    return( low.isArea(cr.area) && (
            (cr.id >= low.id && cr.id <= high) ||
            (low.id == -1 && high == -1)
    ));
}

//*********************************************************************
//                      str
//*********************************************************************

std::string Range::str() const {
    std::ostringstream oStr;
    oStr << low.area << ":";
    if(high == -1 && low.id == -1)
        oStr << "entire range";
    else if(high == low.id)
        oStr << low.id;
    else
        oStr << low.id << "-" << high;
    return(oStr.str());
}

//*********************************************************************
//                      isArea
//*********************************************************************

bool Range::isArea(std::string_view c) const {
    return(low.isArea(c));
}
