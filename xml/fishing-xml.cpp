/*
 * fishing-xml.cpp
 *   Fishing XML
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
#include <libxml/parser.h>                          // for xmlNode, xmlFreeDoc
#include <cstdio>                                   // for snprintf

#include "fishing.hpp"                              // for FishingItem, Fishing
#include "config.hpp"                               // for Config, gConfig
#include "paths.hpp"                                // for Path
#include "xml.hpp"                                  // for copyToNum, NODE_NAME

//*********************************************************************
//                      loadFishing
//*********************************************************************

bool Config::loadFishing() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;

    char filename[80];
    snprintf(filename, 80, "%s/fishing.xml", Path::Game);
    xmlDoc = xml::loadFile(filename, "Fishing");
    if(xmlDoc == nullptr)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode))
        curNode = curNode->next;

    if(curNode == nullptr) {
        xmlFreeDoc(xmlDoc);
        return(false);
    }

    clearFishing();
    std::string id = "";
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "List")) {
            xml::copyPropToString(id, curNode, "id");
            if(!id.empty()) {
                Fishing list;
                list.load(curNode);
                list.id = id;
                fishing[id] = list;
            }
        }
        curNode = curNode->next;
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

//*********************************************************************
//                      load
//*********************************************************************

void Fishing::load(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    xml::copyPropToString(id, rootNode, "id");

    while(curNode) {
        if(NODE_NAME(curNode, "Item")) {
            FishingItem item;
            item.load(curNode);
            items.push_back(item);
        }
        curNode = curNode->next;
    }
}

void FishingItem::load(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "Fish")) fish.load(curNode);
        else if(NODE_NAME(curNode, "DayOnly")) dayOnly = true;
        else if(NODE_NAME(curNode, "NightOnly")) nightOnly = true;
        else if(NODE_NAME(curNode, "Weight")) xml::copyToNum(weight, curNode);
        else if(NODE_NAME(curNode, "Experience")) xml::copyToNum(exp, curNode);
        else if(NODE_NAME(curNode, "MinQuality")) xml::copyToNum(minQuality, curNode);
        else if(NODE_NAME(curNode, "MinSkill")) xml::copyToNum(minSkill, curNode);
        else if(NODE_NAME(curNode, "Monster")) monster = true;
        else if(NODE_NAME(curNode, "Aggro")) aggro = true;
        curNode = curNode->next;
    }
}
