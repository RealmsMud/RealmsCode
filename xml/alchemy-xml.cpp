/*
 * alchemy-xml.cpp
 *   Alchemy - xml files
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
#include <map>                                      // for map<>::mapped_type
#include <ostream>                                  // for basic_ostream::op...
#include <string>                                   // for allocator, string

#include "alchemy.hpp"                              // for AlchemyEffect
#include "mudObjects/objects.hpp"                   // for Object, AlchemyEf...
#include "xml.hpp"                                  // for copyToNum, newNum...


//*********************************************************************
//                      loadAlchemyEffects
//*********************************************************************
AlchemyEffect::AlchemyEffect(xmlNodePtr curNode) {
    quality= duration = strength = 1;
    xmlNodePtr childNode = curNode->children;
    while(childNode) {
        if(NODE_NAME(childNode, "Effect")) {
            effect = xml::getString(childNode);
        } else if(NODE_NAME(childNode, "Duration")) {
            xml::copyToNum(duration, childNode);
        } else if(NODE_NAME(childNode, "Strength")) {
            xml::copyToNum(strength, childNode);
        } else if(NODE_NAME(childNode, "Quality")) {
            xml::copyToNum(quality, childNode);
        }
        childNode = childNode->next;
    }
}
void Object::loadAlchemyEffects(xmlNodePtr curNode) {
    int i=0;
    xmlNodePtr childNode = curNode->children;
    while(childNode) {
        if(NODE_NAME(childNode, "AlchemyEffect")) {
            i = xml::getIntProp(childNode, "Num");
            if(i>0) {
                alchemyEffects[i] = AlchemyEffect(childNode);
            }
        }
        childNode = childNode->next;
    }
}

int AlchemyEffect::saveToXml(xmlNodePtr rootNode) {
    if(rootNode == nullptr)
        return(-1);
    xml::newStringChild(rootNode, "Effect", effect);
    xml::newNumChild(rootNode, "Duration", duration);
    xml::newNumChild(rootNode, "Strength", strength);
    xml::newNumChild(rootNode, "Quality", quality);

    return(0);
}
