/*
 * carry.cpp
 *   Carried objects file
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

#include "carry.hpp"

#include <ostream>      // for basic_ostream::operator<<, operator<<, ostrin...

#include "xml.hpp"      // for getIntProp, newNumProp

//*********************************************************************
//                      Carry
//*********************************************************************

Carry::Carry() {
    numTrade = 0;
}

Carry& Carry::operator=(const Carry& cry) {
    info = cry.info;
    numTrade = cry.numTrade;
    return(*this);
}

bool Carry::operator==(const Carry& cry) const {
    return(info == cry.info && numTrade == cry.numTrade);
}
bool Carry::operator!=(const Carry& cry) const {
    return(!(*this == cry));
}
//*********************************************************************
//                      load
//*********************************************************************

void Carry::load(xmlNodePtr curNode) {
    info.load(curNode);
    numTrade = xml::getIntProp(curNode, "NumTrade");
}

//*********************************************************************
//                      save
//*********************************************************************

xmlNodePtr Carry::save(xmlNodePtr curNode, const char* childName, bool saveNonZero, int pos) const {
    curNode = info.save(curNode, childName, saveNonZero, pos);
    if(curNode && numTrade)
        xml::newNumProp(curNode, "NumTrade", numTrade);
    return(curNode);
}

//*********************************************************************
//                      str
//*********************************************************************

std::string Carry::str(std::string current, char color) const {
    std::ostringstream oStr;
    oStr << info.displayStr(current, color);
    if(numTrade)
        oStr << " (" << numTrade << ")";
    return(oStr.str());
}

