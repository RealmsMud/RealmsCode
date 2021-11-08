/*
 * Spells-xml.cpp
 *   Additional spell-casting routines - XML
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
#include "xml.hpp"                                  // for copyToString
#include "structs.hpp"                              // for Spell, PFNCOMPARE


//*********************************************************************
//                      load
//*********************************************************************

Spell::Spell(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    priority = 100;

    while(curNode) {
        if(NODE_NAME(curNode, "Name")) {
            xml::copyToString(name, curNode);
            parseName();
        }
        else if(NODE_NAME(curNode, "Script")) xml::copyToString(script, curNode);
        else if(NODE_NAME(curNode, "Priority")) xml::copyToNum(priority, curNode);
        else if(NODE_NAME(curNode, "Description")) xml::copyToString(description, curNode);

        curNode = curNode->next;
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void Spell::save(xmlNodePtr rootNode) const {
    xml::saveNonNullString(rootNode, "Name", name);
    xml::saveNonNullString(rootNode, "Script", script);
    xml::saveNonZeroNum<int>(rootNode, "Priority", priority);
    xml::saveNonNullString(rootNode,"Description", description);

}



