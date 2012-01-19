/*
 * creatureStreams.cpp
 *   Handles streaming to creatures
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "mud.h"

Creature& Creature::operator<< ( MudObject& mo) {
    Player* player = getPlayer();
    if(player && player->getSock()) {
        Socket* sock = player->getSock();

        const Creature* creature = mo.getCreature();
        const Object* object = mo.getObject();

        int mFlags = player->displayFlags() & player->getManipFlags();
        int mNum = player->getManipNum();
        if(creature) {
            sock->bprint(creature->getCrtStr(this, mFlags, mNum));
        } else if(object) {
            sock->bprint(object->getObjStr(this, mFlags, mNum));
        }

    }
    return(*this);
}

Creature& Creature::operator<< ( MudObject* mo) {
    return(*this << *mo);
}

Creature& Creature::operator<< (const bstring& str) {
    const Player* player = getConstPlayer();
    if(player && player->getSock()) {
        Socket* sock = player->getSock();
        sock->bprint(str);
    }
    return(*this);
}


void Creature::setManipFlags(int flags) {
    manipFlags &= flags;
}

// Returns the manipFlags and resets them
int Creature::getManipFlags() {
    int toReturn = manipFlags;
    manipFlags = 0;
    return (toReturn);
}

void Creature::setManipNum(int num) {
    manipNum = num;
}

// Returns the manipNum and resets them
int Creature::getManipNum() {
    int toReturn = manipNum;
    manipNum = 0;
    return(toReturn);
}

