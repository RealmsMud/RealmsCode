/*
 * effects-xml.cpp
 *   Effects xml
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

#include "mudObject.hpp"
#include "effects.hpp"
#include "xml.hpp"

//*********************************************************************
//                      load
//*********************************************************************

void Effects::load(xmlNodePtr rootNode, MudObject* pParent) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Effect")) {
            try {
                auto* newEffect = new EffectInfo(curNode);
                newEffect->setParent(pParent);
                effectList.push_back(newEffect);
            } catch(std::runtime_error &e) {
                std::clog << "Error adding effect: " << e.what() << std::endl;
            }
        }
        curNode = curNode->next;
    }
}


//*********************************************************************
//                      save
//*********************************************************************

void Effects::save(xmlNodePtr rootNode, const char* name) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, name);
    EffectList::const_iterator eIt;
    for(eIt = effectList.begin() ; eIt != effectList.end() ; eIt++) {
        (*eIt)->save(curNode);
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void EffectInfo::save(xmlNodePtr rootNode) const {
    xmlNodePtr effectNode = xml::newStringChild(rootNode, "Effect");
    xml::newStringChild(effectNode, "Name", name);
    xml::newNumChild(effectNode, "Duration", duration);
    xml::newNumChild(effectNode, "Strength", strength);
    xml::newNumChild(effectNode, "Extra", extra);
    xml::newNumChild(effectNode, "PulseModifier", pulseModifier);
}
