/*
 * track.cpp
 *   Track files
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

#include "bstring.hpp"                              // for bstring
#include "proto.hpp"                                // for whatSize
#include "size.hpp"                                 // for Size, NO_SIZE
#include "track.hpp"                                // for Track
#include "utils.hpp"                                // for MAX
#include "xml.hpp"                                  // for saveNonZeroNum

//*********************************************************************
//                      Track
//*********************************************************************

Track::Track() {
    num = 0;
    size = NO_SIZE;
    direction = "";
}

short Track::getNum() const { return(num); }
Size Track::getSize() const { return(size); }
bstring Track::getDirection() const { return(direction); }

void Track::setNum(short n) { num = MAX<short>(0, n); }
void Track::setSize(Size s) { size = s; }
void Track::setDirection(std::string_view dir) { direction = dir; }

//*********************************************************************
//                      load
//*********************************************************************

void Track::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Direction")) xml::copyToBString(direction, childNode);
        else if(NODE_NAME(childNode, "Size")) size = whatSize(xml::toNum<int>(childNode));
        else if(NODE_NAME(childNode, "Num")) xml::copyToNum(num, childNode);

        childNode = childNode->next;
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void Track::save(xmlNodePtr curNode) const {
    xml::saveNonNullString(curNode, "Direction", direction.c_str());
    xml::saveNonZeroNum(curNode, "Size", size);
    xml::saveNonZeroNum(curNode, "Num", num);
}
