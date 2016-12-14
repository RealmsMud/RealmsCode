/*
 * deityData.cpp
 *   Deity data storage file
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"


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

    if(curNode == 0) {
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

//**********************************************************************
//                      getDeity
//**********************************************************************

const DeityData* Config::getDeity(int id) const {
    std::map<int, DeityData*>::const_iterator it = deities.find(id);

    if(it == deities.end())
        it = deities.begin();

    return((*it).second);
}

//*********************************************************************
//                      dmShowDeities
//*********************************************************************

int dmShowDeities(Player* player, cmd* cmnd) {
    std::map<int, DeityData*>::iterator it;
    std::map<int, PlayerTitle*>::iterator tt;
    DeityData* data=0;
    PlayerTitle* title=0;
    bool    all = player->isDm() && cmnd->num > 1 && !strcmp(cmnd->str[1], "all");


    player->printColor("Displaying Deities:%s\n",
        player->isDm() && !all ? "  Type ^y*deitylist all^x to view all information." : "");
    for(it = gConfig->deities.begin() ; it != gConfig->deities.end() ; it++) {
        data = (*it).second;
        player->printColor("Id: ^c%-2d^x   Name: ^c%s\n", data->getId(), data->getName().c_str());
        if(all) {
            for(tt = data->titles.begin() ; tt != data->titles.end(); tt++) {
                title = (*tt).second;
                player->printColor("   Level: ^c%-2d^x   Male: ^c%s^x   Female: ^c%s\n",
                    (*tt).first, title->getTitle(true).c_str(), title->getTitle(false).c_str());
            }
            player->print("\n");
        }
    }
    player->print("\n");
    return(0);
}

//**********************************************************************
//                      clearDeities
//**********************************************************************

void Config::clearDeities() {
    std::map<int, DeityData*>::iterator it;
    std::map<int, PlayerTitle*>::iterator tt;

    for(it = deities.begin() ; it != deities.end() ; it++) {
        DeityData* d = (*it).second;
        for(tt = d->titles.begin() ; tt != d->titles.end() ; tt++) {
            PlayerTitle* p = (*tt).second;
            delete p;
        }
        d->titles.clear();
        delete d;
    }
    deities.clear();
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
    xml::copyPropToBString(name, rootNode, "name");

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

//*********************************************************************
//                      getId
//*********************************************************************

int DeityData::getId() const {
    return(id);
}

//*********************************************************************
//                      getName
//*********************************************************************

bstring DeityData::getName() const {
    return(name);
}

//**********************************************************************
//                      cmdReligion
//**********************************************************************

int cmdReligion(Player* player, cmd* cmnd) {
    if(!player->getDeity()) {
        player->print("You do not belong to a religion.\n");
    } else {
        const DeityData* deity = gConfig->getDeity(player->getDeity());
        if(player->alignInOrder())
            player->print("You are in good standing with %s.\n", deity->getName().c_str());
        else
            player->print("You are not in good standing with %s.\n", deity->getName().c_str());
    }
    return(0);
}
