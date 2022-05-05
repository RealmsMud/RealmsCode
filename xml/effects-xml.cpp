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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <libxml/parser.h>  // for xmlNodePtr, xmlNode
#include <list>             // for operator==, list<>::const_iterator, _List...
#include <memory>           // for allocator
#include <ostream>          // for basic_ostream::operator<<, operator<<
#include <stdexcept>        // for runtime_error

#include "effects.hpp"      // for EffectInfo, Effects, EffectList
#include "xml.hpp"          // for newNumChild, newStringChild, NODE_NAME
#include "config.hpp"

class MudObject;


//*********************************************************************
//                      EffectInfo
//*********************************************************************

EffectInfo::EffectInfo(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "Name")) xml::copyToString(name, curNode);
        else if(NODE_NAME(curNode, "Duration")) xml::copyToNum(duration, curNode);
        else if(NODE_NAME(curNode, "Strength")) xml::copyToNum(strength, curNode);
        else if(NODE_NAME(curNode, "Extra")) xml::copyToNum(extra, curNode);
        else if(NODE_NAME(curNode, "PulseModifier")) xml::copyToNum(pulseModifier, curNode);

        curNode = curNode->next;
    }

    lastPulse = lastMod = time(nullptr);
    myEffect = gConfig->getEffect(name);

    if(!myEffect) {
        throw std::runtime_error("Can't find effect listing " + name);
    }
}

//*********************************************************************
//                      load
//*********************************************************************

void Effects::load(xmlNodePtr rootNode, const std::shared_ptr<MudObject>& pParent) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Effect")) {
            try {
                auto* newEffect = new EffectInfo(curNode);
                newEffect->setParent(pParent.get());
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
