/*
 * rooms-xml.cpp
 *   Rooms xml
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

#include <stdexcept>                                // for runtime_error

#include "skills.hpp"                               // for SkillCommand, Ski...
#include "xml.hpp"                                  // for NODE_NAME, copyTo...

class Creature;
class cmd;

//*********************************************************************
//                      load
//*********************************************************************

Skill::Skill(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    reset();

    while(curNode) {
        if(NODE_NAME(curNode, "Name")) {
            setName(xml::getString(curNode));
        } else if(NODE_NAME(curNode, "Gained")) {
            xml::copyToNum(gained, curNode);
        } else if(NODE_NAME(curNode, "GainBonus")) {
            xml::copyToNum(gainBonus, curNode);
        }
        curNode = curNode->next;
    }
    if(name.empty() || gained == 0) {
        throw(std::runtime_error("Invalid Skill Xml"));
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void Skill::save(xmlNodePtr rootNode) const {
    xmlNodePtr skillNode = xml::newStringChild(rootNode, "Skill");
    xml::newStringChild(skillNode, "Name", getName());
    xml::newNumChild(skillNode, "Gained", getGained());
    xml::newNumChild(skillNode, "GainBonus", getGainBonus());
}

