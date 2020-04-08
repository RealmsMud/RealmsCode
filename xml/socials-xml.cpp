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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "socials.hpp"
#include <config.hpp>
#include <xml.hpp>
#include <paths.hpp>

//*********************************************************************
//                      loadSocials
//*********************************************************************

bool Config::loadSocials() {
    xmlDocPtr   xmlDoc;

    xmlNodePtr  curNode;
    char        filename[80];

    // build an XML tree from a the file
    sprintf(filename, "%s/socials.xml", Path::Code);

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

    clearAlchemy();
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Social")) {
            auto* social = new SocialCommand(curNode);
            socials.insert(SocialMap::value_type(social->getName(),social));
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

int cmdSocial(Creature* creature, cmd* cmnd);
SocialCommand::SocialCommand(xmlNodePtr rootNode) {
    rootNode = rootNode->children;
    priority = 100;
    auth = nullptr;
    description = "";
    fn = cmdSocial;

    wakeTarget = false;
    rudeWakeTarget = false;
    wakeRoom = false;

    while(rootNode != nullptr)
    {
        if(NODE_NAME(rootNode, "Name")) xml::copyToBString(name, rootNode);
        else if(NODE_NAME(rootNode, "Description")) xml::copyToBString(description, rootNode);
        else if(NODE_NAME(rootNode, "Priority")) xml::copyToNum(priority, rootNode);
        else if(NODE_NAME(rootNode, "Script")) xml::copyToBString(script, rootNode);
        else if(NODE_NAME(rootNode, "SelfNoTarget")) xml::copyToBString(selfNoTarget, rootNode);
        else if(NODE_NAME(rootNode, "RoomNoTarget")) xml::copyToBString(roomNoTarget, rootNode);
        else if(NODE_NAME(rootNode, "SelfOnTarget")) xml::copyToBString(selfOnTarget, rootNode);
        else if(NODE_NAME(rootNode, "RoomOnTarget")) xml::copyToBString(roomOnTarget, rootNode);
        else if(NODE_NAME(rootNode, "VictimOnTarget")) xml::copyToBString(victimOnTarget, rootNode);
        else if(NODE_NAME(rootNode, "SelfOnSelf")) xml::copyToBString(selfOnSelf, rootNode);
        else if(NODE_NAME(rootNode, "RoomOnSelf")) xml::copyToBString(roomOnSelf, rootNode);
        else if(NODE_NAME(rootNode, "WakeTarget")) xml::copyToBool(wakeTarget, rootNode);
        else if(NODE_NAME(rootNode, "RudeWakeTarget")) xml::copyToBool(rudeWakeTarget, rootNode);
        else if(NODE_NAME(rootNode, "WakeRoom")) xml::copyToBool(wakeRoom, rootNode);

        rootNode = rootNode->next;
    }
}