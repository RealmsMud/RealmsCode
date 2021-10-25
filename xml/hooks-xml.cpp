/*
 * hooks-xml.cpp
 *   Dynamic hooks xml
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

#include <libxml/parser.h>  // for xmlNodePtr, xmlNode
#include <utility>          // for pair

#include "hooks.hpp"        // for Hooks
#include "xml.hpp"          // for newStringChild, copyPropToString, copyTo...

//*********************************************************************
//                      load
//*********************************************************************

void Hooks::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    std::string event, code;
    while(childNode) {
        event = "", code = "";
        if(NODE_NAME(childNode, "Hook")) {
            xml::copyPropToString(event, childNode, "event");
            xml::copyToString(code, childNode);
            add(event, code);
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void Hooks::save(xmlNodePtr curNode, const char* name) const {
    if(hooks.empty())
        return;

    xmlNodePtr childNode, subNode;

    childNode = xml::newStringChild(curNode, name);

    for( const auto& p: hooks ) {
        subNode = xml::newStringChild(childNode, "Hook", p.second);
        xml::newProp(subNode, "event", p.first);
    }
}