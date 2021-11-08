/*
 * diety-xml.cpp
 *   Diety-xml
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
#include <libxml/parser.h>        // for xmlNode, xmlFreeDoc, xmlNodePtr
#include <cstdio>                 // for snprintf
#include <map>                    // for operator==, map, allocator

#include "config.hpp"             // for Config
#include "deityData.hpp"          // for DeityData
#include "paths.hpp"              // for Game
#include "playerTitle.hpp"        // for PlayerTitle
#include "xml.hpp"                // for getIntProp, NODE_NAME, copyPropToBS...

//**********************************************************************
//                      loadDeities
//**********************************************************************

bool Config::loadDeities() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;
    int     i=0;

    char filename[80];
    snprintf(filename, 80, "%s/deities.xml", Path::Game);
    xmlDoc = xml::loadFile(filename, "Deities");

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

    clearDeities();
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Deity")) {
            i = xml::getIntProp(curNode, "id");

            if(deities.find(i) == deities.end())
                deities[i] = new DeityData(curNode);
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}


//*********************************************************************
//                      DeityData
//*********************************************************************

DeityData::DeityData(xmlNodePtr rootNode) {
    int lvl=0;

    id = 0;
    name = "";
    xmlNodePtr curNode, childNode;

    id = xml::getIntProp(rootNode, "id");
    xml::copyPropToString(name, rootNode, "name");

    curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Titles")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Title")) {
                    lvl = xml::getIntProp(childNode, "level");
                    titles[lvl] = new PlayerTitle;
                    titles[lvl]->load(childNode);
                }
                childNode = childNode->next;
            }
        }
        curNode = curNode->next;
    }
}

