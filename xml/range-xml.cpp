/*
 * range-xml.cpp
 *   CatRef range object xml
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
#include "xml.hpp"


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
        return(nullptr);
    xmlNodePtr curNode = xml::newStringChild(rootNode, childName);
    if(pos)
        xml::newNumProp(curNode, "Num", pos);

    low.save(curNode, "Low", false);
    xml::newNumChild(curNode, "High", high);

    return(curNode);
}