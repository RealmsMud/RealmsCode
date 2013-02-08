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
//# Container
//################################################################################

Container::Container() {
    // TODO Auto-generated constructor stub

}


bool Container::purge(bool includePets) {
	bool purgedAll = true;
	purgedAll &= purgeMonsters(includePets);
	purgedAll &= purgeObjects();

	return(purgedAll);
}

bool Container::purgeMonsters(bool includePets) {
	bool purgedAll = true;
	MonsterSet::iterator mIt, prevIt;
	Monster *mons = 0;
	for(mIt = monsters.begin() ; mIt != monsters.end() ; ) {
		prevIt = mIt;
		mons = (*mIt++);
		if(includePets == false && mons->isPet()) {
			purgedAll = false;
			continue;
		}
        if(mons->flagIsSet(M_DM_FOLLOW)) {
            Player* master = mons->getPlayerMaster();
            if(master) {
                master->clearFlag(P_ALIASING);

                master->setAlias(0);
                master->print("%1M's soul was purged.\n", mons);
                master->delPet(mons->getAsMonster());
            }
        }

		monsters.erase(prevIt);
		free_crt(mons);
	}
	return(purgedAll);
}
bool Container::purgeObjects() {
	bool purgedAll = true;
	ObjectSet::iterator oIt, prevIt;
	Object* obj;
	for(oIt = objects.begin() ; oIt != objects.end() ; ) {
		prevIt = oIt;
		obj = (*oIt++);
		if(!obj->flagIsSet(O_TEMP_PERM)) {
			objects.erase(prevIt);
			delete obj;
		} else {
			purgedAll = false;
		}
	}
	return(purgedAll);
}

Container* Container::remove(Containable* toRemove) {
    if(!toRemove)
        return(NULL);

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
        std::cout << "Don't know how to remove " << toRemove << std::endl;
        toReturn = false;
    }

    std::cout << "Removing " << toRemove->getName() << " to " << this->getName() << std::endl;

    if(toReturn) {
    	Container* retVal = toRemove->getParent();
        toRemove->setParent(NULL);
        return(retVal);
    }

    return(NULL);
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
    std::cout << "Adding " << toAdd->getName() << " to " << this->getName() << std::endl;
    return(toReturn);
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

MudObject* Container::findTarget(const Creature* searcher, const cmd* cmnd, int num) const {
	return(findTarget(searcher, cmnd->str[num], cmnd->val[num]));
}

MudObject* Container::findTarget(const Creature* searcher, const bstring& name, const int num,  bool monFirst, bool firstAggro, bool exactMatch) const {
	int match=0;
	return(findTarget(searcher, name, num, monFirst, firstAggro, exactMatch, match));
}

MudObject* Container::findTarget(const Creature* searcher, const bstring& name, const int num,  bool monFirst, bool firstAggro, bool exactMatch, int& match) const {
	MudObject* toReturn = 0;

	if((toReturn = findCreature(searcher, name, num, monFirst, firstAggro, exactMatch, match))) {
		return(toReturn);
	}
	// TODO: Search the container's objects
	return(toReturn);
}

// Wrapper for the real findObject to support legacy callers
Object* Container::findObject(const Creature *searcher, const cmd* cmnd, int val) const {
	return(findObject(searcher, cmnd->str[val], cmnd->val[val]));
}
Object* Container::findObject(const Creature* searcher, const bstring& name, const int num, bool exactMatch ) const {
	int match = 0;
	return(findObject(searcher, name, num,exactMatch, match));
}
Object* Container::findObject(const Creature* searcher, const bstring& name, const int num, bool exactMatch, int& match) const {
	Object *target = 0;
	for(Object* obj : objects) {
        if(isMatch(searcher, obj, name, exactMatch)) {
            match++;
            if(match == num) {
                if(exactMatch)
                    return(obj);
                else {
                    target = obj;
                    break;
                }

            }
        }
	}

	return(target);
}


// Wrapper for the real findCreature to support legacy callers
Creature* Container::findCreature(const Creature* searcher, const cmd* cmnd, int num) const {
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
Creature* Container::findCreature(const Creature* searcher, const bstring& name, const int num, bool monFirst, bool firstAggro, bool exactMatch )  const {
	int ignored=0;
	return(findCreature(searcher, name, num, monFirst, firstAggro, exactMatch, ignored));
}
bool isMatch(const Creature* searcher, MudObject* target, const bstring& name, bool exactMatch, bool checkVisibility) {
    if(!target)
        return(false);
    if(checkVisibility && !searcher->canSee(target))
        return(false);

    // ID match is exact, regardless of exactMatch option
    if(target->getId() == name)
    	return(true);

    if(exactMatch) {
        if(target->getName() == name) {
            return(true);
        }

    } else {
    	if(target->isCreature() && keyTxtEqual(target->getAsCreature(), name.c_str())) {
    		return(true);
    	} else if(target->isObject() && keyTxtEqual(target->getAsObject(), name.c_str())) {
            return(true);
        }
    }
    return(false);
}

// The real findCreature, will take and return the value of match as to allow for findTarget to find the 3rd thing named
// gold with a monster name goldfish, player named goldmine, and some gold in the room!
Creature* Container::findCreature(const Creature* searcher, const bstring& name, const int num, bool monFirst, bool firstAggro, bool exactMatch, int& match) const {

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

Monster* Container::findMonster(const Creature* searcher, const cmd* cmnd, int num) const {
	return(findMonster(searcher, cmnd->str[num], cmnd->val[num]));
}
Monster* Container::findMonster(const Creature* searcher, const bstring& name, const int num, bool firstAggro, bool exactMatch) const {
    int match = 0;
    return(findMonster(searcher, name, num, firstAggro, exactMatch, match));
}
Monster* Container::findMonster(const Creature* searcher, const bstring& name, const int num, bool firstAggro, bool exactMatch, int& match) const {
    Monster* target = 0;
    for(Monster* mons : searcher->getParent()->monsters) {
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
Player* Container::findPlayer(const Creature* searcher, const cmd* cmnd, int num) const {
	return(findPlayer(searcher, cmnd->str[num], cmnd->val[num]));
}
Player* Container::findPlayer(const Creature* searcher, const bstring& name, const int num, bool exactMatch) const {
    int match = 0;
    return(findPlayer(searcher, name, num, exactMatch, match));
}
Player* Container::findPlayer(const Creature* searcher, const bstring& name, const int num, bool exactMatch, int& match) const {
    for(Player* ply : searcher->getParent()->players) {
        if(isMatch(searcher, ply, name, exactMatch)) {
            match++;
            if(match == num) {
                return(ply);
            }
        }
    }
    return(NULL);
}


//################################################################################
//# Containable
//################################################################################

Containable::Containable() {
    parent = 0;
    lastParent = 0;
}


void Containable::removeFromSet() {
	lastParent = removeFrom();
}
void Containable::addToSet() {
	addTo(lastParent);
	lastParent = 0;
}

bool Containable::addTo(Container* container) {
    if(this->parent != NULL) {
        std::cout << "Non Null Parent" << std::endl;
        return(0);
    }

    if(container == NULL)
        return(removeFrom());

    if(this->isCreature()) {
    	if(container->isUniqueRoom()) {
    		getAsCreature()->currentLocation.room = container->getAsUniqueRoom()->info;
    	} else if (container->isAreaRoom()) {
    		getAsCreature()->currentLocation.mapmarker = container->getAsAreaRoom()->mapmarker;
    	}
    }
    return(container->add(this));
}

Container* Containable::removeFrom() {
    if(!parent) {
        return(NULL);
    }

    if(isCreature()) {
    	getAsCreature()->currentLocation.room.clear();
    	getAsCreature()->currentLocation.mapmarker.reset();
    }
    return(parent->remove(this));
}

void Containable::setParent(Container* container) {
    parent = container;
}
Container* Containable::getParent() const {
    return(parent);
}


bool Containable::inRoom() const {
	return(parent && parent->isRoom());
}
bool Containable::inUniqueRoom() const {
	return(parent && parent->isUniqueRoom());
}
bool Containable::inAreaRoom() const {
	return(parent && parent->isAreaRoom());
}
bool Containable::inObject() const {
	return(parent && parent->isObject());
}
bool Containable::inPlayer() const {
	return(parent && parent->isPlayer());
}
bool Containable::inMonster() const {
	return(parent && parent->isMonster());
}
bool Containable::inCreature() const {
	return(parent && parent->isCreature());
}

BaseRoom* Containable::getRoomParent() {
	if(!parent)
		return(NULL);
	return(parent->getAsRoom());
}
UniqueRoom* Containable::getUniqueRoomParent() {
	if(!parent)
			return(NULL);
	return(parent->getAsUniqueRoom());
}
AreaRoom* Containable::getAreaRoomParent() {
	if(!parent)
		return(NULL);
	return(parent->getAsAreaRoom());
}
Object* Containable::getObjectParent() {
	if(!parent)
		return(NULL);
	return(parent->getAsObject());
}
Player* Containable::getPlayerParent() {
	if(!parent)
		return(NULL);
	return(parent->getAsPlayer());
}
Monster* Containable::getMonsterParent() {
	if(!parent)
		return(NULL);
	return(parent->getAsMonster());
}
Creature* Containable::getCreatureParent() {
	if(!parent)
		return(NULL);
	return(parent->getAsCreature());
}

const BaseRoom* Containable::getConstRoomParent() const {
	if(!parent)
		return(NULL);
	return(parent->getAsConstRoom());
}
const UniqueRoom* Containable::getConstUniqueRoomParent() const {
	if(!parent)
		return(NULL);
	return(parent->getAsConstUniqueRoom());
}
const AreaRoom* Containable::getConstAreaRoomParent() const {
	if(!parent)
		return(NULL);
	return(parent->getAsConstAreaRoom());
}
const Object* Containable::getConstObjectParent() const {
	if(!parent)
		return(NULL);
	return(parent->getAsConstObject());
}
const Player* Containable::getConstPlayerParent() const {
	if(!parent)
		return(NULL);
	return(parent->getAsConstPlayer());
}
const Monster* Containable::getConstMonsterParent() const {
	if(!parent)
		return(NULL);
	return(parent->getAsConstMonster());
}

const Creature* Containable::getConstCreatureParent() const {
	if(!parent)
		return(NULL);
	return(parent->getAsConstCreature());
}


