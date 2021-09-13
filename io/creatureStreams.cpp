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
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <string>               // for basic_string<>::npos

#include "bstring.hpp"          // for bstring
#include "creatureStreams.hpp"  // for Streamable, setf, setn, operator<<
#include "creatures.hpp"        // for Player, Monster, Creature
#include "mudObject.hpp"        // for MudObject
#include "objects.hpp"          // for Object
#include "socket.hpp"           // for Socket
#include "stats.hpp"            // for Stat

Streamable& Streamable::operator << ( Streamable& (*op)(Streamable&)) {
    // call the function passed as parameter with this stream as the argument
    return (*op)(*this);
}

Streamable& operator<<(Streamable& out, setf flags) {
    return flags(out);
}

Streamable& setf::operator()(Streamable& out) const {
    out.setManipFlags(value);
    return(out);
}

Streamable& operator<<(Streamable& out, setn num) {
    return num(out);
}

Streamable& setn::operator()(Streamable& out) const {
    out.setManipNum(value);
    return(out);
}


void Streamable::initStreamable() {
    manipFlags = 0;
    manipNum = 0;
    streamColor = false;
    petPrinted = false;
}


Streamable& Streamable::operator<< ( const MudObject& mo) {
    auto* thisPlayer = dynamic_cast<Player*>(this);
    auto* thisCreature = dynamic_cast<Creature*>(this);
    if (!thisCreature)
        throw std::runtime_error("WTF");

    unsigned int mFlags = thisCreature->displayFlags();

    if(thisPlayer && thisPlayer->getSock()) {
        mFlags |= thisPlayer->getManipFlags();
    }
    int mNum = this->getManipNum();
    const Creature* creature = mo.getAsConstCreature();
    if(creature) {
        doPrint(thisCreature->getCrtStr(thisPlayer, mFlags, mNum));
        return(*this);
    }

    const Object* object = mo.getAsConstObject();
    if(object) {
        doPrint(object->getObjStr(thisPlayer, mFlags, mNum));
        return(*this);
    }

    return(*this);
}

Streamable& Streamable::operator<< ( const MudObject* mo) {
    return(*this << *mo);
}

Streamable& Streamable::operator<< (const bstring& str) {
    doPrint(str);
    return(*this);
}
Streamable& Streamable::operator<< (const int num) {
    doPrint(num);
    return(*this);
}
Streamable& Streamable::operator<< (Stat& stat) {
    doPrint(stat.toString());
    return(*this);
}
void Streamable::setColorOn() {
    streamColor = true;
}
void Streamable::setColorOff() {
    streamColor = false;
}

void Streamable::setManipFlags(unsigned int flags) {
    manipFlags |= flags;
}

// Returns the manipFlags and resets them
unsigned int Streamable::getManipFlags() {
    unsigned int toReturn = manipFlags;
    manipFlags = 0;

    return (toReturn);
}

void Streamable::setManipNum(int num) {
    manipNum = num;

}

// Returns the manipNum and resets them
int Streamable::getManipNum() {
    int toReturn = manipNum;
    manipNum = 0;
    return(toReturn);
}

void Streamable::doPrint(const bstring& toPrint) {
    const Player* player = dynamic_cast<Player*>(this);
    const Monster* monster = dynamic_cast<Monster*>(this);
    const Player* master = nullptr;
    Socket* sock = nullptr;

    if(player)
        sock = player->getSock();

    if(monster) {
        master = monster->getConstPlayerMaster();
        if(master)
            sock = master->getSock();
    }
    if(sock) {
        if(master) {
            if(!petPrinted) {
                sock->bprint("Pet> ");
                if(toPrint.find("\n") == bstring::npos)
                    petPrinted = true;
            }
            else {
                if(toPrint.find("\n") != bstring::npos)
                    petPrinted = false;
            }
        }
        if(streamColor)
            sock->bprintColor(toPrint);
        else
            sock->bprintNoColor(toPrint);
    }

}
