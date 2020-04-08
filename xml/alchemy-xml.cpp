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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "alchemy.hpp"
#include "config.hpp"
#include "objects.hpp"
#include "xml.hpp"
#include "paths.hpp"

bool Config::loadAlchemy() {
    xmlDocPtr   xmlDoc;

    xmlNodePtr  curNode;
    char        filename[80];

    // build an XML tree from a the file
    sprintf(filename, "%s/alchemy.xml", Path::Code);

    xmlDoc = xml::loadFile(filename, "Alchemy");
    if(xmlDoc == nullptr)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == nullptr) {
        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();
        return(false);
    }

    clearAlchemy();
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "AlchemyInfo")) {
            auto* alcInfo = new AlchemyInfo(curNode);
            alchemy.insert(std::make_pair(alcInfo->getName(), alcInfo));
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

AlchemyInfo::AlchemyInfo(xmlNodePtr rootNode) {

    rootNode = rootNode->children;
    while(rootNode != nullptr)
    {
        if(NODE_NAME(rootNode, "Name")) xml::copyToBString(name, rootNode);
        else if(NODE_NAME(rootNode, "Action")) xml::copyToBString(action, rootNode);
        else if(NODE_NAME(rootNode, "PythonScript")) xml::copyToBString(pythonScript, rootNode);
        else if(NODE_NAME(rootNode, "Positive")) xml::copyToBool(positive, rootNode);
        else if(NODE_NAME(rootNode, "Throwable")) xml::copyToBool(throwable, rootNode);
        else if(NODE_NAME(rootNode, "PotionDisplayName")) xml::copyToBString(potionDisplayName, rootNode);
        else if(NODE_NAME(rootNode, "PotionPrefix")) xml::copyToBString(potionPrefix, rootNode);
        else if(NODE_NAME(rootNode, "BaseDuration")) xml::copyToNum(baseDuration, rootNode);
        else if(NODE_NAME(rootNode, "BaseStrength")) xml::copyToNum(baseStrength, rootNode);

        rootNode = rootNode->next;
    }
}

//*********************************************************************
//                      loadAlchemyEffects
//*********************************************************************
AlchemyEffect::AlchemyEffect(xmlNodePtr curNode) {
    quality=duration = strength = 1;
    xmlNodePtr childNode = curNode->children;
    while(childNode) {
        if(NODE_NAME(childNode, "Effect")) {
            effect = xml::getBString(childNode);
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
