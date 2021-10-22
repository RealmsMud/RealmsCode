/*
 * factions-xml.cpp
 *   Faction code - XML
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

#include "config.hpp"                               // for Config, gConfig
#include "factions.hpp"                             // for Faction, FactionR...
#include "paths.hpp"                                // for Path
#include "xml.hpp"                                  // for copyToNum, NODE_NAME



bool Config::loadFactions() {
    xmlDocPtr   xmlDoc;
    //GuildCreation * curCreation;
    xmlNodePtr  cur;
    char        filename[80];

    // build an XML tree from the file
    sprintf(filename, "%s/factions.xml", Path::Game);

    xmlDoc = xml::loadFile(filename, "Factions");
    if(xmlDoc == nullptr)
        return(false);

    cur = xmlDocGetRootElement(xmlDoc);

    // First level we expect a faction
    cur = cur->children;
    while(cur && xmlIsBlankNode(cur))
        cur = cur->next;

    if(cur == nullptr) {
        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();
        return(false);
    }

    clearFactionList();
    Faction* faction;
    while(cur != nullptr) {
        if(NODE_NAME(cur, "Faction")) {
            faction = new Faction;
            faction->load(cur);
            factions[faction->getName()] = faction;
        }
        cur = cur->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();

    std::map<std::string, Faction*>::const_iterator it, fIt;

    for(it = factions.begin(); it != factions.end(); it++) {
        faction = (*it).second;

        if(!faction->getParent().empty()) {
            fIt = factions.find(faction->getParent());

            if(fIt == factions.end()) {
                faction->setParent("");
            } else {
                (*fIt).second->setIsParent(true);
            }
        }
    }
    return(true);
}

void Faction::load(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "Name")) { xml::copyToString(name, curNode); }
        else if(NODE_NAME(curNode, "DisplayName")) { xml::copyToString(displayName, curNode); }
        else if(NODE_NAME(curNode, "Parent")) { xml::copyToString(parent, curNode); }
        else if(NODE_NAME(curNode, "Group")) { xml::copyToString(group, curNode); }
        else if(NODE_NAME(curNode, "Social")) { xml::copyToString(social, curNode); }
        else if(NODE_NAME(curNode, "BaseRegard")) { xml::copyToNum( baseRegard, curNode); }
        else if(NODE_NAME(curNode, "InitialRegard")) initial.load(curNode);
        else if(NODE_NAME(curNode, "MaxRegard")) max.load(curNode);
        else if(NODE_NAME(curNode, "MinRegard")) min.load(curNode);
        curNode = curNode->next;
    }
}

