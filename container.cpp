/*
 * Container.cpp
 *   Classes to handle classes that can be containers or contained by a container
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

//################################################################################
//# Containable
//################################################################################

Container::Container() {
    // TODO Auto-generated constructor stub

}

bool Container::remove(Containable* toRemove) {
    if(!toRemove)
        return(false);

    Object* remObject = dynamic_cast<Object*>(toRemove);
    Player* remPlayer = dynamic_cast<Player*>(toRemove);
    Monster* remMonster = dynamic_cast<Monster*>(toRemove);

    if(remObject) {
        objects.erase(remObject);
        return(true);
    } else if(remPlayer) {
        players.erase(remPlayer);
        return(true);
    } else if(remMonster) {
        monsters.erase(remMonster);
        return(true);
    } else {
        std::cout << "Don't know how to add " << toRemove << std::endl;
        return(false);
    }
}

bool Container::add(Containable* toAdd) {
    if(!toAdd)
        return(false);

    Object* addObject = dynamic_cast<Object*>(toAdd);
    Player* addPlayer = dynamic_cast<Player*>(toAdd);
    Monster* addMonster = dynamic_cast<Monster*>(toAdd);

    if(addObject) {
        objects.insert(addObject);
        return(true);
    } else if(addPlayer) {
        players.insert(addPlayer);
        return(true);
    } else if(addMonster) {
        monsters.insert(addMonster);
        return(true);
    } else {
        std::cout << "Don't know how to add " << toAdd << std::endl;
        return(false);
    }
}

bool Container::checkAntiMagic(Monster* ignore) {
    for(Monster* mons : monsters) {
        if(mons == ignore)
            continue;
        if(mons->flagIsSet(M_ANTI_MAGIC_AURA)) {
            broadcast(NULL,  this, "^B%M glows bright blue.", mons);
            return(true);
        }
    }
}
//################################################################################
//# Containable
//################################################################################
Containable::Containable() {
    parent = 0;
}

bool Containable::addTo(Container* container) {
    if(this->parent != NULL)
        return(0);

    if(container == NULL)
        return(remove());

    return(container->add(this));
}

bool Containable::remove() {
    if(!parent)
        return(false);

    return(parent->remove(this));
}

void Containable::setParent(Container* container) {
    parent = container;
}
Container* Containable::getParent() {
    return(parent);
}
