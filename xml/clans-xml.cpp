/*
 * clans-xml.cpp
 *   xml parsing for clans - XML
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
#include <libxml/parser.h>                          // for xmlFreeDoc, xmlCl...
#include <stdio.h>                                  // for snprintf
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <map>                                      // for operator==, map
#include <string>                                   // for string, allocator

#include "clans.hpp"                                // for Clan
#include "config.hpp"                               // for Config, ClanMap
#include "paths.hpp"                                // for Game
#include "xml.hpp"                                  // for toNum, NODE_NAME

//*********************************************************************
//                      loadClans
//*********************************************************************

bool Config::loadClans() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;
    int     i=0;

    char filename[80];
    snprintf(filename, 80, "%s/clans.xml", Path::Game.c_str());
    xmlDoc = xml::loadFile(filename, "Clans");

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

    clearClans();
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Clan")) {
            i = xml::getIntProp(curNode, "Id");

            if(clans.find(i) == clans.end()) {
                clans[i] = new Clan;
                clans[i]->load(curNode);
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

void Clan::load(xmlNodePtr rootNode) {
    xmlNodePtr childNode, curNode = rootNode->children;
    std::string temp;

    id = xml::getIntProp(rootNode, "Id");
    while(curNode) {
        if(NODE_NAME(curNode, "Name")) xml::copyToString(name, curNode);
        else if(NODE_NAME(curNode, "Join")) join = xml::toNum<int>(curNode);
        else if(NODE_NAME(curNode, "Rescind")) rescind = xml::toNum<int>(curNode);
        else if(NODE_NAME(curNode, "Deity")) {
            xml::copyToString(temp, curNode);
            deity = gConfig->deitytoNum(temp);
        }
        else if(NODE_NAME(curNode, "SkillBonus")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Skill")) {
                    xml::copyPropToString(temp, childNode, "Name");
                    skillBonus[temp] = xml::toNum<short>(childNode);
                }
                childNode = childNode->next;
            }
        }
        curNode = curNode->next;
    }
}



