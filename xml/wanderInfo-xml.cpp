/*
 * wanderInfo-xml.cpp
 *   Random monster wandering info - XML
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
#include <memory>                                   // for allocator
#include <ostream>                                  // for basic_ostream::op...

#include "mudObjects/rooms.hpp"                     // for NUM_RANDOM_SLOTS
#include "wanderInfo.hpp"                           // for WanderInfo
#include "xml.hpp"                                  // for copyToNum, loadCa...

//*********************************************************************
//                      load
//*********************************************************************

void WanderInfo::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Traffic")) xml::copyToNum(traffic, childNode);
        else if(NODE_NAME(childNode, "RandomMonsters")) {
            loadCatRefArray(childNode, random, "Mob", NUM_RANDOM_SLOTS);
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void WanderInfo::save(xmlNodePtr curNode) const {
    xml::saveNonZeroNum(curNode, "Traffic", traffic);
    saveCatRefArray(curNode, "RandomMonsters", "Mob", random, NUM_RANDOM_SLOTS);
}

