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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <ostream>                                  // for basic_ostream::op...
#include <string>                                   // for allocator, string
#include <string_view>                              // for string_view

#include "size.hpp"                                 // for whatSize, Size
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
std::string Track::getDirection() const { return(direction); }

void Track::setNum(short n) { num = MAX<short>(0, n); }
void Track::setSize(Size s) { size = s; }
void Track::setDirection(std::string_view dir) { direction = dir; }

//*********************************************************************
//                      load
//*********************************************************************

void Track::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Direction")) xml::copyToString(direction, childNode);
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
