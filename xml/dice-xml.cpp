/*
 * dice-xml.cpp
 *   Dice XML
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

#include <libxml/parser.h>                          // for xmlNodePtr, xmlNode
#include <ostream>                                  // for basic_ostream::op...

#include "dice.hpp"                                 // for Dice
#include "xml.hpp"                                  // for saveNonZeroNum

//*********************************************************************
//                      load
//*********************************************************************

void Dice::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    clear();

    while(childNode) {
        if(NODE_NAME(childNode, "Number")) setNumber(xml::toNum<unsigned short>(childNode));
        else if(NODE_NAME(childNode, "Sides")) setSides(xml::toNum<unsigned short>(childNode));
        else if(NODE_NAME(childNode, "Plus")) setPlus(xml::toNum<short>(childNode));
        else if(NODE_NAME(childNode, "Mean")) setMean(xml::toNum<double>(childNode));

        childNode = childNode->next;
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void Dice::save(xmlNodePtr curNode, const char* name) const {
    if(!number && !sides && !plus && !mean)
        return;
    xmlNodePtr childNode = xml::newStringChild(curNode, name);

    xml::saveNonZeroNum(childNode, "Number", number);
    xml::saveNonZeroNum(childNode, "Sides", sides);
    xml::saveNonZeroNum(childNode, "Plus", plus);
    xml::saveNonZeroNum(childNode, "Mean", mean);
}