/*
 * bans-xml.cpp
 *  Various functions to ban a person from the mud - xml functions
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
#include <stdexcept>
#include <exception>
#include <config.hpp>
#include <paths.hpp>
#include "bans.hpp"
#include "xml.hpp"

bool Config::loadBans() {
    xmlDocPtr xmlDoc;
    Ban *curBan=nullptr;
    xmlNodePtr cur;

    char filename[80];
    snprintf(filename, 80, "%s/bans.xml", Path::Config);
    xmlDoc = xml::loadFile(filename, "Bans");

    if(xmlDoc == nullptr)
        return(false);

    cur = xmlDocGetRootElement(xmlDoc);


    // First level we expect a Ban
    cur = cur->children;
    while(cur && xmlIsBlankNode(cur)) {
        cur = cur->next;
    }
    if(cur == nullptr) {
        xmlFreeDoc(xmlDoc);
        return(false);
    }

    clearBanList();
    while(cur != nullptr) {
        if((!strcmp((char*) cur->name, "Ban"))) {
            curBan = new Ban(cur);
            bans.push_back(curBan);
        }
        cur = cur->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

Ban::Ban(xmlNodePtr curNode) {

    curNode = curNode->children;
    while(curNode != nullptr)
    {
        if(NODE_NAME(curNode, "Site")) xml::copyToBString(site, curNode);
        else if(NODE_NAME(curNode, "Duration")) xml::copyToNum(duration, curNode);
        else if(NODE_NAME(curNode, "UnbanTime")) xml::copyToNum(unbanTime, curNode);
        else if(NODE_NAME(curNode, "BannedBy")) xml::copyToBString(by, curNode);
        else if(NODE_NAME(curNode, "BanTime")) xml::copyToBString(time, curNode);
        else if(NODE_NAME(curNode, "Reason")) xml::copyToBString(reason, curNode);
        else if(NODE_NAME(curNode, "Password")) xml::copyToBString(password, curNode);
        else if(NODE_NAME(curNode, "Suffix")) xml::copyToBool(isSuffix, curNode);
        else if(NODE_NAME(curNode, "Prefix")) xml::copyToBool(isPrefix, curNode);

        curNode = curNode->next;
    }
}