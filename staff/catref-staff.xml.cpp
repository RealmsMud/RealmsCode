/*
 * catRefStaff.h
 *   Category Reference - Staff commands
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

#include <iomanip>
#include <iostream>

#include "catRef.hpp"
#include "catRefInfo.hpp"
#include "config.hpp"
#include "creatures.hpp"

//*********************************************************************
//                      dmCatRefInfo
//*********************************************************************

int dmCatRefInfo(Player* player, cmd* cmnd) {
    std::list<CatRefInfo*>::const_iterator it;
    std::ostringstream oStr;
    const CatRefInfo *cri=nullptr;
    int i=0;

    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr << "^yCatRefInfo List^x\n";

    for(it = gConfig->catRefInfo.begin() ; it != gConfig->catRefInfo.end() ; it++) {
        cri = (*it);
        i = 20;
        if(cri->getName().getAt(0) == '^')
            i += 2;
        oStr << "  Area: ^c" << std::setw(10) << cri->str()
             << "^x Name: ^c" << std::setw(i)
             << cri->getName() << "^x ";
        if(cri->getParent().empty()) {
            oStr << "TeleportZone: ^c" << std::setw(4) << cri->getTeleportZone() << "^x "
                 << "TrackZone: ^c" << std::setw(4) << cri->getTrackZone() << "^x "
                 << "TeleportWeight: ^c" << cri->getTeleportWeight();
            if(!cri->getFishing().empty())
                oStr << " Fishing: ^c" << cri->getFishing();
            oStr << "^x\n"
                 << "                                              "
                 << "Limbo: ^c" << cri->getLimbo() << "^x    Recall: ^c" << cri->getRecall() << "^x\n";
        } else {
            oStr << "Parent: ^c" << cri->getParent() << "^x "
                 << "TeleportWeight: ^c" << cri->getTeleportWeight();
            if(!cri->getFishing().empty())
                oStr << " Fishing: ^c" << cri->getFishing();
            oStr << "^x\n";
        }
    }

    player->printColor("%s\n", oStr.str().c_str());
    return(0);
}