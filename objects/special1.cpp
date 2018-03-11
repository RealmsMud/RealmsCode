/*
 * special1.cpp
 *   Special routines.
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
#include "creatures.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "socket.hpp"

//*********************************************************************
//                      doSpecial
//*********************************************************************

int Object::doSpecial(Player* player) {
    BaseRoom* room = player->getRoomParent();
    Socket* sock = player->getSock();
    char    str[80], str2[160];
    unsigned int i=0;

    switch(special) {
    case SP_MAPSC:
        strcpy(str, getCName());
        for(i=0; i<strlen(str); i++)
            if(str[i] == ' ')
                str[i] = '_';
        sprintf(str2, "%s/%s.txt", Path::Sign, str);
        if(flagIsSet(O_LOGIN_FILE))
            viewLoginFile(player->getSock(), str2);
        else
            viewFile(player->getSock(), str2);
        break;
        
    case SP_COMBO:
        char    str[80];
        int     dmg, i;

        str[0] = damage.getSides()+'0';
        str[1] = 0;

        if(damage.getNumber() == 1 || strlen(sock->tempstr[3]) > 70)
            strcpy(sock->tempstr[3], str);
        else
            strcat(sock->tempstr[3], str);

        player->print("Click.\n");
        if(player->getName() == "Bane")
            player->print("Combo so far: %s\n", sock->tempstr[3]);

        broadcast(sock, room, "%M presses %P^x.", player, this);

        if(strlen(sock->tempstr[3]) >= strlen(use_output)) {
            if(strcmp(sock->tempstr[3], use_output)) {
                dmg = mrand(20,40 + player->getLevel());
                player->hp.decrease(dmg);
                player->printColor("You were zapped for %s%d^x damage!\n", player->customColorize("*CC:DAMAGE*").c_str(), dmg);
                broadcast(sock, room, "%M was zapped by %P^x!", player, this);
                sock->tempstr[3][0] = 0;

                if(player->hp.getCur() < 1) {
                    player->print("You died.\n");
                    player->die(ZAPPED);
                }
            } else {
                Exit* toOpen = 0;
                i = 1;
                for(Exit* ext : room->exits) {
                    if(i++ >= damage.getPlus())
                        toOpen = ext;
                }
//              for(i=1, xp = room->first_ext;
//                      xp && i < damage.getPlus();
//                      i++, xp = xp->next_tag)
//                  ;
                if(!toOpen)
                    return(0);
                player->statistics.combo();
                player->print("You opened the %s!\n", toOpen->getCName());
                broadcast(player->getSock(), player->getParent(),
                    "%M opened the %s!", player, toOpen->getCName());
                toOpen->clearFlag(X_LOCKED);
                toOpen->clearFlag(X_CLOSED);
                toOpen->ltime.ltime = time(0);
            }
        }
        break;
        
    default:
        player->print("Nothing.\n");
        break;
    }

    return(0);
}
