/*
 * socials-xml.cpp
 *   Socials xml
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

#include <libxml/parser.h>                          // for xmlCleanupParser
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <config.hpp>                               // for Config, SocialSet
#include <cstdio>                                   // for sprintf
#include <functional>                               // for function
#include <paths.hpp>                                // for Code
#include <string>                                   // for string

#include "socials.hpp"                              // for SocialCommand
#include "xml.hpp"                                  // for copyToString, NOD...

//*********************************************************************
//                      loadSocials
//*********************************************************************

bool Config::loadSocials() {
    xmlDocPtr   xmlDoc;

    xmlNodePtr  curNode;
    char        filename[80];

    // build an XML tree from the file
    sprintf(filename, "%s/socials.xml", Path::Code.c_str());

    xmlDoc = xml::loadFile(filename, "Socials");
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

    clearSocials();
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Social")) {
            socials.emplace(curNode);
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

int cmdSocial(const std::shared_ptr<Creature>& creature, cmd* cmnd);
SocialCommand::SocialCommand(xmlNodePtr rootNode) {
    rootNode = rootNode->children;
    priority = 100;
    auth = nullptr;
    description = "";
    fn = cmdSocial;

    wakeTarget = false;
    rudeWakeTarget = false;
    wakeRoom = false;

    while(rootNode != nullptr) {
        if(NODE_NAME(rootNode, "Name")) xml::copyToString(name, rootNode);
        else if(NODE_NAME(rootNode, "Description")) xml::copyToString(description, rootNode);
        else if(NODE_NAME(rootNode, "Priority")) xml::copyToNum(priority, rootNode);
        else if(NODE_NAME(rootNode, "Script")) xml::copyToString(script, rootNode);
        else if(NODE_NAME(rootNode, "SelfNoTarget")) xml::copyToString(selfNoTarget, rootNode);
        else if(NODE_NAME(rootNode, "RoomNoTarget")) xml::copyToString(roomNoTarget, rootNode);
        else if(NODE_NAME(rootNode, "SelfOnTarget")) xml::copyToString(selfOnTarget, rootNode);
        else if(NODE_NAME(rootNode, "RoomOnTarget")) xml::copyToString(roomOnTarget, rootNode);
        else if(NODE_NAME(rootNode, "VictimOnTarget")) xml::copyToString(victimOnTarget, rootNode);
        else if(NODE_NAME(rootNode, "SelfOnSelf")) xml::copyToString(selfOnSelf, rootNode);
        else if(NODE_NAME(rootNode, "RoomOnSelf")) xml::copyToString(roomOnSelf, rootNode);
        else if(NODE_NAME(rootNode, "WakeTarget")) xml::copyToBool(wakeTarget, rootNode);
        else if(NODE_NAME(rootNode, "RudeWakeTarget")) xml::copyToBool(rudeWakeTarget, rootNode);
        else if(NODE_NAME(rootNode, "WakeRoom")) xml::copyToBool(wakeRoom, rootNode);

        rootNode = rootNode->next;
    }
}



