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

bstring escapeColor(bstring color);

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
        doPrint(creature->getCrtStr(thisPlayer, mFlags, mNum));
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

Streamable& Streamable::operator<< (std::string_view str) {
    doPrint(str);
    return(*this);
}
Streamable& Streamable::operator<< (const int num) {
    doPrint(bstring(num));
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
void Streamable::setPagerOn() {
    pager = true;
}
void Streamable::setPagerOff() {
    pager = false;
    Socket* sock = getMySock();
    if(sock) sock->donePaging();
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

void Streamable::doPrint(std::string_view toPrint) {
    const Monster* monster = dynamic_cast<Monster*>(this);
    const Player* master = nullptr;
    Socket* sock = getMySock();

    if(monster) {
        master = monster->getConstPlayerMaster();
    }
    if(sock) {
        if(master) {
            if(!petPrinted) {
                sock->bprint("Pet> ");
                if(toPrint.find('\n') == bstring::npos)
                    petPrinted = true;
            }
            else {
                if(toPrint.find('\n') != bstring::npos)
                    petPrinted = false;
            }
        }
        // TODO: This doesn't work yet, we need to buffer it up until we get a full line and then send it
        if(pager) { // Paged
            if(streamColor)
                sock->appendPaged(toPrint);
            else
                sock->appendPaged(escapeColor(toPrint));
        } else { // Unpaged
            if(streamColor)
                sock->bprint(toPrint);
            else
                sock->bprint(escapeColor(toPrint));
        }
    }

}

Socket *Streamable::getMySock() {
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
    return sock;

}
