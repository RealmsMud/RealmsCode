/*
 * Songs-xml.cpp
 *   Additional song-casting routines - XML:
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

#include "songs.hpp"                                // for Song
#include "xml.hpp"                                  // for copyToBString


//*********************************************************************
//                      Song
//*********************************************************************

Song::Song(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    priority = 100;
    delay = 5;
    duration = 15;

    while(curNode) {
        if(NODE_NAME(curNode, "Name")) {
            xml::copyToBString(name, curNode);
            parseName();
        }
        else if(NODE_NAME(curNode, "Script")) xml::copyToBString(script, curNode);
        else if(NODE_NAME(curNode, "Effect")) xml::copyToBString(effect, curNode);
        else if(NODE_NAME(curNode, "Type")) xml::copyToBString(type, curNode);
        else if(NODE_NAME(curNode, "TargetType")) xml::copyToBString(targetType, curNode);
        else if(NODE_NAME(curNode, "Priority")) xml::copyToNum(priority, curNode);
        else if(NODE_NAME(curNode, "Description")) xml::copyToBString(description, curNode);
        else if(NODE_NAME(curNode, "Delay")) xml::copyToNum(delay, curNode);
        else if(NODE_NAME(curNode, "Duration")) xml::copyToNum(duration, curNode);

        curNode = curNode->next;
    }
}


//*********************************************************************
//                      save
//*********************************************************************

void Song::save(xmlNodePtr rootNode) const {
    xml::saveNonNullString(rootNode, "Name", name);
    xml::saveNonNullString(rootNode, "Script", script);
    xml::saveNonNullString(rootNode, "Effect", effect);
    xml::saveNonNullString(rootNode, "Type", type);
    xml::saveNonNullString(rootNode, "TargetType", targetType);
    xml::saveNonZeroNum<int>(rootNode, "Priority", priority);
    xml::saveNonNullString(rootNode,"Description", description);
    xml::saveNonZeroNum<int>(rootNode, "Delay", priority);
    xml::saveNonZeroNum<int>(rootNode, "Duration", priority);

}


