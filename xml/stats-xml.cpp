/*
 * stats-xml.cpp
 *   Stats xml
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
#include <stats.hpp>                                // for StatModifier, Stat
#include <xml.hpp>                                  // for copyToNum, newStr...
#include <ostream>                                  // for basic_ostream::op...



//*********************************************************************
//                      load
//*********************************************************************
// Loads a single stat into the given stat pointer

bool Stat::load(xmlNodePtr curNode, std::string_view statName) {
    xmlNodePtr childNode = curNode->children;
    name = statName;
    while(childNode) {
        if(NODE_NAME(childNode, "Current")) xml::copyToNum(cur, childNode);
        else if(NODE_NAME(childNode, "Max")) xml::copyToNum(max, childNode);
        else if(NODE_NAME(childNode, "Initial")) xml::copyToNum(initial, childNode);
        else if(NODE_NAME(childNode, "Modifiers")) loadModifiers(childNode);
        childNode = childNode->next;
    }
    return(true);
}

bool Stat::loadModifiers(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    while(childNode) {
        if(NODE_NAME(childNode, "StatModifier")) {
            auto* mod = new StatModifier(childNode);
            if(mod->getName().empty()) {
                delete mod;
            } else {
                modifiers.insert(ModifierMap::value_type(mod->getName(), mod));
            }
            childNode = childNode->next;
        }
    }
    return(true);
}
StatModifier::StatModifier(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Name")) xml::copyToString(name, childNode);
        else if(NODE_NAME(childNode, "ModAmt")) xml::copyToNum(modAmt, childNode);
        else if(NODE_NAME(childNode, "ModType")) modType = (ModifierType)xml::toNum<unsigned short>(childNode);

        childNode = childNode->next;
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void Stat::save(xmlNodePtr parentNode, const char* statName) const {
    xmlNodePtr curNode = xml::newStringChild(parentNode, "Stat");
    xml::newProp(curNode, "Name", statName);

    xml::newNumChild(curNode, "Initial", initial);
    xmlNodePtr modNode = xml::newStringChild(curNode, "Modifiers");
    for(ModifierMap::value_type p: modifiers) {
        p.second->save(modNode);
    }
}

void StatModifier::save(xmlNodePtr parentNode) {
    xmlNodePtr curNode = xml::newStringChild(parentNode, "StatModifier");
    xml::newStringChild(curNode, "Name", name);
    xml::newNumChild(curNode, "ModAmt", modAmt);
    xml::newNumChild(curNode, "ModType", modType);
}

