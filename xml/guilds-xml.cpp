/*
 * guilds-xml.cpp
 *   Guilds xml
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

#include "config.hpp"
#include "guilds.hpp"
#include "xml.hpp"
#include "paths.hpp"



//*********************************************************************
//                      loadGuilds
//*********************************************************************
// Causes the xml guild file to be parsed

bool Config::loadGuilds() {
    xmlDocPtr   xmlDoc;
    GuildCreation *curCreation=nullptr;
    xmlNodePtr  cur;
    char        filename[80];

    // build an XML tree from a the file
    sprintf(filename, "%s/guilds.xml", Path::PlayerData);

    xmlDoc = xml::loadFile(filename, "Guilds");
    if(xmlDoc == nullptr)
        return(false);

    cur = xmlDocGetRootElement(xmlDoc);
    gConfig->nextGuildId = xml::getIntProp(cur, "NextGuildId");
//  xml::copyToNum(gConfig->nextGuildId, = toNum<int>((char *)xmlGetProp(cur, BAD_CAST "NextGuildId"));

    //  numGuilds = 0;
    //  memset(guildTable, 0, sizeof(guildTable));

    // First level we expect a Guild
    cur = cur->children;
    while(cur && xmlIsBlankNode(cur))
        cur = cur->next;

    if(cur == nullptr) {
        // DOH! Forgot to clean up stuff here...not that it happened, but would have been leaky
        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();
        return(false);
    }

    clearGuildList();
    Guild* guild;
    while(cur != nullptr) {
        if(!strcmp((char *)cur->name, "Guild")) {
            guild = new Guild(cur);
            addGuild(guild);
        }
        if(!strcmp((char *)cur->name, "GuildCreation")) {
            curCreation = parseGuildCreation(cur);
            if(curCreation != nullptr)
                addGuildCreation(curCreation);
        }
        cur = cur->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

//*********************************************************************
//                      parseGuild
//*********************************************************************
// Parse an individual guild from a valid xml file

Guild::Guild(xmlNodePtr curNode) {
    int guildId = xml::getIntProp(curNode, "ID");
    if(guildId == 0)
        throw(std::runtime_error("Invalid GuildID"));

    num = guildId;
    gConfig->numGuilds = MAX(gConfig->numGuilds, guildId);
    curNode = curNode->children;
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
        else if(NODE_NAME(curNode, "Leader")) xml::copyToBString(leader, curNode);
        else if(NODE_NAME(curNode, "Level")) xml::copyToNum(level, curNode);
        else if(NODE_NAME(curNode, "NumMembers")) xml::copyToNum(numMembers, curNode);
        else if(NODE_NAME(curNode, "Members")) parseGuildMembers(curNode);
        else if(NODE_NAME(curNode, "PkillsWon")) xml::copyToNum(pkillsWon, curNode);
        else if(NODE_NAME(curNode, "PkillsIn")) xml::copyToNum(pkillsIn, curNode);
        else if(NODE_NAME(curNode, "Points")) xml::copyToNum(points, curNode);
        else if(NODE_NAME(curNode, "Bank")) bank.load(curNode);

        curNode = curNode->next;
    }
}

//*********************************************************************
//                      parseGuildCreation
//*********************************************************************
// Parse an individual guild from a valid xml file

GuildCreation * parseGuildCreation(xmlNodePtr cur) {
    auto* gc = new GuildCreation;

    bstring temp = "";
    cur = cur->children;
    while(cur != nullptr) {
        if(NODE_NAME(cur, "Name")) xml::copyToBString(gc->name, cur);
        else if(NODE_NAME(cur, "Leader")) xml::copyToBString(gc->leader, cur);
        else if(NODE_NAME(cur, "LeaderIp")) xml::copyToBString(gc->leaderIp, cur);
        else if(NODE_NAME(cur, "Status")) xml::copyToNum(gc->status, cur);
        else if(NODE_NAME(cur, "Supporter")) {
            xml::copyToBString(temp, cur);
            gc->supporters[temp] = xml::getProp(cur, "Ip");
            gc->numSupporters++;
        }
        cur = cur->next;
    }
    return(gc);
}

//*********************************************************************
//                      parseGuildMembers
//*********************************************************************

void Guild::parseGuildMembers(xmlNodePtr cur) {
    cur = cur->children;
    while(cur != nullptr) {
        if(NODE_NAME(cur, "Member")) {
            char *tmp = xml::getCString(cur);
            addMember(tmp);
            free(tmp);
        }
        cur = cur->next;
    }
}
