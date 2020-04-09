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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
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



//*********************************************************************
//                      saveBans
//*********************************************************************

bool Config::saveBans() const {
    int found=0;
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr  curNode;
    char        filename[80];

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr,BAD_CAST "Bans", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    std::list<Ban*>::const_iterator it;
    Ban* ban;

    for(it = bans.begin() ; it != bans.end() ; it++) {
        found++;
        ban = (*it);

        curNode = xmlNewChild(rootNode, nullptr, BAD_CAST "Ban", nullptr);
        // Site
        xmlNewChild(curNode, nullptr, BAD_CAST "Site", BAD_CAST ban->site.c_str());
        // Duration
        //sprintf(buf, "%d", ban->duration);
        xml::newNumChild(curNode, "Duration", ban->duration);
        // Unban Time
        //sprintf(buf, "%ld", ban->unbanTime);
        xml::newNumChild(curNode, "UnbanTime", ban->unbanTime);
        // Banned By
        xmlNewChild(curNode, nullptr, BAD_CAST "BannedBy", BAD_CAST ban->by.c_str());
        // Ban Time
        xmlNewChild(curNode, nullptr, BAD_CAST "BanTime", BAD_CAST ban->time.c_str());
        // Reason
        xmlNewChild(curNode, nullptr, BAD_CAST "Reason", BAD_CAST ban->reason.c_str());
        // Password
        xmlNewChild(curNode, nullptr, BAD_CAST "Password", BAD_CAST ban->password.c_str());
        // Suffix
        xmlNewChild(curNode, nullptr, BAD_CAST "Suffix", BAD_CAST iToYesNo(ban->isSuffix));
        // Prefix
        xmlNewChild(curNode, nullptr, BAD_CAST "Prefix", BAD_CAST iToYesNo(ban->isPrefix));
    }
    sprintf(filename, "%s/bans.xml", Path::Config);
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
    return(true);
}


