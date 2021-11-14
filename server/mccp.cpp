/*
 * mccp.cpp
 *   Handle Mud Client Compression Protocol, Modified for use with mordor by Bane
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

#include "mudObjects/players.hpp"  // for Player
#include "socket.hpp"              // for Socket, will_comp1, will_comp2

class cmd;

int mccp(Player* player, cmd* cmnd) {
    if(!player)
        return(0);

    if(player->getSock()->mccpEnabled() == 0) {
        player->print("Attempting to enable MCCP.\n");
        player->print("%s", telnet::will_comp2);
        player->print("%s", telnet::will_comp1);
    } else {
        player->print("Ending compression.\n");
        player->getSock()->endCompress();
    }
    return(0);
}
