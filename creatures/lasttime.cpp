/*
 * lasttime.cpp
 *   Lasttime related
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

#include "mud.hpp"                     // for LT, LT_PLAYER_SEND, LT_AGE
#include "mudObjects/creatures.hpp"    // for Creature, PetList
#include "utils.hpp"                   // for MIN, MAX

//********************************************************************
//              fixLts
//********************************************************************

void Creature::fixLts() {
    long tdiff=0, t = time(nullptr);
    int i=0;
    if(isPet())  {
        tdiff = t - getMaster()->lasttime[LT_AGE].ltime;
    }
    else
        tdiff = t - lasttime[LT_AGE].ltime;
    for(i=0; i<MAX_LT; i++) {
        if(i == LT_JAILED)
            continue;
        // Fix pet fade on login here
        if(lasttime[i].ltime == 0 && lasttime[i].interval == 0)
            continue;
        lasttime[i].ltime += tdiff;
        lasttime[i].ltime = MIN(t, lasttime[i].ltime);
    }
}