/*
 * objIncrease.cpp
 *   Object increase
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include <libxml/parser.h>                          // for xmlNodePtr, xmlNode

#include "objIncrease.hpp"                          // for ObjIncrease, Unkn...
#include "xml.hpp"                                  // for saveNonZeroNum

//*********************************************************************
//                          ObjIncrease
//*********************************************************************

ObjIncrease::ObjIncrease() {
    reset();
}

//*********************************************************************
//                          reset
//*********************************************************************

void ObjIncrease::reset() {
    type = UnknownIncrease;
    increase = "";
    amount = 0;
    onlyOnce = canAddIfNotKnown = false;
}

//*********************************************************************
//                          operator=
//*********************************************************************

ObjIncrease& ObjIncrease::operator=(const ObjIncrease& o) {
    if(this == &o)
        return(*this);

    type = o.type;
    increase = o.increase;
    amount = o.amount;
    onlyOnce = o.onlyOnce;
    canAddIfNotKnown = o.canAddIfNotKnown;
    return(*this);
}

//*********************************************************************
//                      load
//*********************************************************************

void ObjIncrease::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    reset();

    while(childNode) {
        if(NODE_NAME(childNode, "Amount")) xml::copyToNum(amount, childNode);
        else if(NODE_NAME(childNode, "Type")) type = (IncreaseType)xml::toNum<int>(childNode);
        else if(NODE_NAME(childNode, "Increase")) xml::copyToString(increase, childNode);
        else if(NODE_NAME(childNode, "OnlyOnce")) xml::copyToBool(onlyOnce, childNode);
        else if(NODE_NAME(childNode, "CanAddIfNotKnown")) xml::copyToBool(canAddIfNotKnown, childNode);

        childNode = childNode->next;
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void ObjIncrease::save(xmlNodePtr curNode) const {
    if(!isValid())
        return;

    xml::saveNonZeroNum(curNode, "Type", (int)type);
    xml::saveNonZeroNum(curNode, "Amount", amount);
    xml::saveNonNullString(curNode, "Increase", increase.c_str());
    xml::saveNonZeroNum(curNode, "OnlyOnce", onlyOnce);
    xml::saveNonZeroNum(curNode, "CanAddIfNotKnown", canAddIfNotKnown);
}

//*********************************************************************
//                      isValid
//*********************************************************************

bool ObjIncrease::isValid() const {
    return(type != UnknownIncrease && !increase.empty());
}

