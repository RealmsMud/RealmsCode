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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <cstring>                // for strcmp
#include <map>                    // for operator==, operator!=, allocator

#include "cmd.hpp"                // for cmd
#include "config.hpp"             // for Config, gConfig
#include "creatures.hpp"          // for Player
#include "deityData.hpp"          // for DeityData
#include "playerTitle.hpp"        // for PlayerTitle

//**********************************************************************
//                      getDeity
//**********************************************************************

const DeityData* Config::getDeity(int id) const {
    auto it = deities.find(id);

    if(it == deities.end())
        it = deities.begin();

    return((*it).second);
}

//*********************************************************************
//                      dmShowDeities
//*********************************************************************

int dmShowDeities(Player* player, cmd* cmnd) {
    DeityDataMap::iterator it;
    std::map<int, PlayerTitle*>::iterator tt;
    DeityData* data=nullptr;
    PlayerTitle* title=nullptr;
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
    DeityDataMap::iterator it;
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
//                      getId
//*********************************************************************

int DeityData::getId() const {
    return(id);
}

//*********************************************************************
//                      getName
//*********************************************************************

std::string DeityData::getName() const {
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
