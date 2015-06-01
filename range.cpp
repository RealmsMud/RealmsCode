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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"

//*********************************************************************
//                      Range
//*********************************************************************

Range::Range() {
    high = 0;
}

Range& Range::operator=(const Range& r) {
    low = r.low;
    high = r.high;
    return(*this);
}

bool Range::operator==(const Range& r) const {
    if( low != r.low ||
        high != r.high
    )
        return(false);
    return(true);
}
bool Range::operator!=(const Range& r) const {
    return(!(*this==r));
}

//*********************************************************************
//                      load
//*********************************************************************

void Range::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
             if(NODE_NAME(childNode, "Low")) low.load(childNode);
        else if(NODE_NAME(childNode, "High")) xml::copyToNum(high, childNode);

        childNode = childNode->next;
    }
    if(!high && low.id)
        high = low.id;
}

//*********************************************************************
//                      save
//*********************************************************************

xmlNodePtr Range::save(xmlNodePtr rootNode, const char* childName, int pos) const {
    if(!low.id && !high)
        return(NULL);
    xmlNodePtr curNode = xml::newStringChild(rootNode, childName);
    if(pos)
        xml::newNumProp(curNode, "Num", pos);

    low.save(curNode, "Low", false);
    xml::newNumChild(curNode, "High", high);

    return(curNode);
}

//*********************************************************************
//                      belongs
//*********************************************************************

bool Range::belongs(CatRef cr) const {
    return( low.isArea(cr.area) && (
            (cr.id >= low.id && cr.id <= high) ||
            (low.id == -1 && high == -1)
    ));
}

//*********************************************************************
//                      str
//*********************************************************************

bstring Range::str() const {
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

bool Range::isArea(bstring c) const {
    return(low.isArea(c));
}
