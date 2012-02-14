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

    bool toReturn = false;
    if(remObject) {
        objects.erase(remObject);
        toReturn = true;
    } else if(remPlayer) {
        players.erase(remPlayer);
        toReturn = true;
    } else if(remMonster) {
        monsters.erase(remMonster);
        toReturn = true;
    } else {
        std::cout << "Don't know how to add " << toRemove << std::endl;
        toReturn = false;
    }

    if(toReturn)
        toRemove->setParent(NULL);

    return(toReturn);
}

bool Container::add(Containable* toAdd) {
    if(!toAdd)
        return(false);

    Object* addObject = dynamic_cast<Object*>(toAdd);
    Player* addPlayer = dynamic_cast<Player*>(toAdd);
    Monster* addMonster = dynamic_cast<Monster*>(toAdd);
    bool toReturn = false;
    if(addObject) {
        std::pair<ObjectSet::iterator, bool> p = objects.insert(addObject);
        toReturn = p.second;
    } else if(addPlayer) {
        std::pair<PlayerSet::iterator, bool> p = players.insert(addPlayer);
        toReturn = p.second;
    } else if(addMonster) {
        std::pair<MonsterSet::iterator, bool> p = monsters.insert(addMonster);
        toReturn = p.second;
    } else {
        std::cout << "Don't know how to add " << toAdd << std::endl;
        toReturn = false;
    }
    if(toReturn) {
        toAdd->setParent(this);
    }
    return(toReturn);
}

bool Containable::inRoom() const {
	return(typeid(*this) == typeid(BaseRoom));
}
bool Containable::inUniqueRoom() const {
	return(typeid(*this) == typeid(UniqueRoom));
}
bool Containable::inAreaRoom() const {
	return(typeid(*this) == typeid(AreaRoom));
}
bool Containable::inObject() const {
	return(typeid(*this) == typeid(Object));
}
bool Containable::inPlayer() const {
	return(typeid(*this) == typeid(Player));
}
bool Containable::inMonster() const {
	return(typeid(*this) == typeid(Monster));
}
bool Containable::inCreature() const {
	return(typeid(*this) == typeid(Creature));
}

BaseRoom* Containable::getRoomParent() {
	return(dynamic_cast<BaseRoom*>(this));
}
UniqueRoom* Containable::getUniqueRoomParent() {
	return(dynamic_cast<UniqueRoom*>(this));
}
AreaRoom* Containable::getAreaRoomParent() {
	return(dynamic_cast<AreaRoom*>(this));
}
Object* Containable::getObjectParent() {
	return(dynamic_cast<Object*>(this));
}
Player* Containable::getPlayerParent() {
	return(dynamic_cast<Player*>(this));
}
Monster* Containable::getMonsterParent() {
	return(dynamic_cast<Monster*>(this));
}
Creature* Containable::getCreatureParent() {
	return(dynamic_cast<Creature*>(this));
}

const BaseRoom* Containable::getConstRoomParent() const {
	return(dynamic_cast<const BaseRoom*>(this));
}
const UniqueRoom* Containable::getConstUniqueRoomParent() const {
	return(dynamic_cast<const UniqueRoom*>(this));
}
const AreaRoom* Containable::getConstAreaRoomParent() const {
	return(dynamic_cast<const AreaRoom*>(this));
}
const Object* Containable::getConstObjectParent() const {
	return(dynamic_cast<const Object*>(this));
}
const Player* Containable::getConstPlayerParent() const {
	return(dynamic_cast<const Player*>(this));
}
const Monster* Containable::getConstMonsterParent() const {
	return(dynamic_cast<const Monster*>(this));
}
const Creature* Containable::getConstCreatureParent() const {
	return(dynamic_cast<const Creature*>(this));
}
//*********************************************************************
//						wake
//*********************************************************************

void Container::wake(bstring str, bool noise) const {
    for(Player* ply : players) {
		ply->wake(str, noise);
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
    return(false);
}
//################################################################################
//# Containable
//################################################################################
Containable::Containable() {
    parent = 0;
}

bool Containable::addTo(Container* container) {
    if(this->parent != NULL) {
        std::cout << "Non Null Parent" << std::endl;
        return(0);
    }

    if(container == NULL)
        return(removeFrom());

    return(container->add(this));
}

bool Containable::removeFrom() {
    if(!parent) {
        std::cout << "No Parent!" << std::endl;
        return(false);
    }

    return(parent->remove(this));
}

void Containable::setParent(Container* container) {
    parent = container;
}
Container* Containable::getParent() const {
    return(parent);
}

MudObject* Container::findTarget(Creature* searcher, const cmd* cmnd, int num) {
	return(findTarget(searcher, cmnd->str[num], cmnd->val[num]));
}

MudObject* Container::findTarget(Creature* searcher, const bstring& name, const int num,  bool monFirst, bool firstAggro, bool exactMatch) {
	int match=0;
	return(findTarget(searcher, name, num, monFirst, firstAggro, exactMatch, match));
}

MudObject* Container::findTarget(Creature* searcher, const bstring& name, const int num,  bool monFirst, bool firstAggro, bool exactMatch, int& match) {
	MudObject* toReturn = 0;

	if((toReturn = findCreature(searcher, name, num, monFirst, firstAggro, exactMatch, match))) {
		return(toReturn);
	}
	// TODO: Search the container's objects
	return(toReturn);
}


// Wrapper for the real findCreature to support legacy callers
Creature* Container::findCreature(Creature* searcher, const cmd* cmnd, int num) {
	return(findCreature(searcher, cmnd->str[num], cmnd->val[num]));
}

Creature* Container::findCreaturePython(Creature* searcher, const bstring& name, bool monFirst, bool firstAggro, bool exactMatch ) {
	int ignored=0;
	int num = 1;

	bstring newName = name;
	newName.trim();
	unsigned int sLoc = newName.ReverseFind(" ");
	if(sLoc != bstring::npos) {
		num = newName.right(newName.length() - sLoc).toInt();
		if(num != 0) {
			newName = newName.left(sLoc);
		} else {
			num = 1;
		}
	}

	std::cout << "Looking for '" << newName << "' #" << num << "\n";

	return(findCreature(searcher, newName, num, monFirst, firstAggro, exactMatch, ignored));
}

// Wrapper for the real findCreature for callers that don't care about the value of match
Creature* Container::findCreature(Creature* searcher, const bstring& name, const int num, bool monFirst, bool firstAggro, bool exactMatch ) {
	int ignored=0;
	return(findCreature(searcher, name, num, monFirst, firstAggro, exactMatch, ignored));
}
bool isMatch(Creature* searcher, Creature* target, const bstring& name, bool exactMatch, bool checkVisibility) {
    if(!target)
        return(false);
    if(checkVisibility && !searcher->canSee(target))
        return(false);

    if(exactMatch) {
        if(!strcmp(target->name, name.c_str())) {
            return(true);
        }

    } else {
        if(keyTxtEqual(target, name.c_str())) {
            return(true);
        }
    }
    return(false);
}

// The real findCreature, will take and return the value of match as to allow for findTarget to find the 3rd thing named
// gold with a monster name goldfish, player named goldmine, and some gold in the room!
Creature* Container::findCreature(Creature* searcher, const bstring& name, const int num, bool monFirst, bool firstAggro, bool exactMatch, int& match) {

	if(!searcher || name.empty())
		return(NULL);

	Creature* target=0;
	for(int i = 1 ; i <= 2 ; i++) {
	    if( (monFirst && i == 1) || (!monFirst && i == 2) ) {
	        target = findMonster(searcher, name, num, firstAggro, exactMatch, match);
	    } else {
	        target = findPlayer(searcher, name, num, exactMatch, match);
	    }

	    if(target) {
	        return(target);
	    }
	}
	return(target);

}

Monster* Container::findMonster(Creature* searcher, const cmd* cmnd, int num) {
	return(findMonster(searcher, cmnd->str[num], cmnd->val[num]));
}
Monster* Container::findMonster(Creature* searcher, const bstring& name, const int num, bool firstAggro, bool exactMatch) {
    int match = 0;
    return(findMonster(searcher, name, num, firstAggro, exactMatch, match));
}
Monster* Container::findMonster(Creature* searcher, const bstring& name, const int num, bool firstAggro, bool exactMatch, int& match) {
    Monster* target = 0;
    for(Monster* mons : searcher->getRoom()->monsters) {
        if(isMatch(searcher, mons, name, exactMatch)) {
            match++;
            if(match == num) {
                if(exactMatch)
                    return(mons);
                else {
                    target = mons;
                    break;
                }

            }
        }
    }
    if(firstAggro && target) {
        if(num < 2 && searcher->pFlagIsSet(P_KILL_AGGROS))
            return(getFirstAggro(target, searcher));
        else
            return(target);
    } else  {
        return(target);
    }
}
Player* Container::findPlayer(Creature* searcher, const cmd* cmnd, int num) {
	return(findPlayer(searcher, cmnd->str[num], cmnd->val[num]));
}
Player* Container::findPlayer(Creature* searcher, const bstring& name, const int num, bool exactMatch) {
    int match = 0;
    return(findPlayer(searcher, name, num, exactMatch, match));
}
Player* Container::findPlayer(Creature* searcher, const bstring& name, const int num, bool exactMatch, int& match) {
    for(Player* ply : searcher->getRoom()->players) {
        if(isMatch(searcher, ply, name, exactMatch)) {
            match++;
            if(match == num) {
                return(ply);
            }
        }
    }
    return(NULL);
}
