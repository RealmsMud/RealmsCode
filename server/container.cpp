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
    Monster *mons = nullptr;
    for(mIt = monsters.begin() ; mIt != monsters.end() ; ) {
        prevIt = mIt;
        mons = (*mIt++);
        if(!includePets && mons->isPet()) {
            purgedAll = false;
            continue;
        }
        if(mons->flagIsSet(M_DM_FOLLOW)) {
            Player* master = mons->getPlayerMaster();
            if(master) {
                master->clearFlag(P_ALIASING);

                master->setAlias(nullptr);
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
        return(nullptr);

    auto* remObject = dynamic_cast<Object*>(toRemove);
    auto* remPlayer = dynamic_cast<Player*>(toRemove);
    auto* remMonster = dynamic_cast<Monster*>(toRemove);

    bool toReturn;
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
        std::clog << "Don't know how to remove " << toRemove << std::endl;
        toReturn = false;
    }

    //std::clog << "Removing " << toRemove->getName() << " from " << this->getName() << std::endl;

    if(toReturn) {
        Container* retVal = toRemove->getParent();
        toRemove->setParent(nullptr);
        return(retVal);
    }

    return(nullptr);
}

bool Container::add(Containable* toAdd) {
    if(!toAdd)
        return(false);

    auto* addObject = dynamic_cast<Object*>(toAdd);
    auto* addPlayer = dynamic_cast<Player*>(toAdd);
    auto* addMonster = dynamic_cast<Monster*>(toAdd);

    // If we're adding an object or a monster, we're registered and the item we're adding is not,
    // then register the item
    if((addObject || addMonster) && isRegistered() && !toAdd->isRegistered()) {
        toAdd->registerMo();
    }


    bool toReturn;
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
        std::clog << "Don't know how to add " << toAdd << std::endl;
        toReturn = false;
    }

    if(toReturn) {
        toAdd->setParent(this);
    }
    //std::clog << "Adding " << toAdd->getName() << " to " << this->getName() << std::endl;
    return(toReturn);
}


void Container::registerContainedItems() {
    // Player registration is handled by the server
    MonsterSet::iterator mIt;
    for(mIt = monsters.begin() ; mIt != monsters.end() ; ) {
        Monster* mons = *mIt++;
        mons->registerMo();
    }
    ObjectSet::iterator oIt;
    for(oIt = objects.begin() ; oIt != objects.end() ; ) {
        Object* obj = *oIt++;
        obj->registerMo();
    }
}
void Container::unRegisterContainedItems() {
    // Player registration is handled by the server
    for(Monster* mons : monsters) {
        mons->unRegisterMo();
    }
    for(Object* obj : objects) {
        obj->unRegisterMo();
    }
}


//*********************************************************************
//                      wake
//*********************************************************************

void Container::wake(const bstring& str, bool noise) const {
    for(Player* ply : players) {
        ply->wake(str, noise);
    }
}

bool Container::checkAntiMagic(Monster* ignore) {
    for(Monster* mons : monsters) {
        if(mons == ignore)
            continue;
        if(mons->flagIsSet(M_ANTI_MAGIC_AURA)) {
            broadcast(nullptr,  this, "^B%M glows bright blue.", mons);
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
    MudObject* toReturn = nullptr;

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
    Object *target = nullptr;
    for(Object* obj : objects) {
        if(isMatch(searcher, obj, name, exactMatch, true)) {
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
    bstring::size_type sLoc = newName.ReverseFind(" ");
    if(sLoc != bstring::npos) {
        num = newName.right(newName.length() - sLoc).toInt();
        if(num != 0) {
            newName = newName.left(sLoc);
        } else {
            num = 1;
        }
    }

    std::clog << "Looking for '" << newName << "' #" << num << "\n";

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
        return(nullptr);

    Creature* target=nullptr;
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
    Monster* target = nullptr;
    for(Monster* mons : searcher->getParent()->monsters) {
        if(isMatch(searcher, mons, name, exactMatch, true)) {
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
        if(isMatch(searcher, ply, name, exactMatch, true)) {
            match++;
            if(match == num) {
                return(ply);
            }
        }
    }
    return(nullptr);
}


//################################################################################
//# Containable
//################################################################################

Containable::Containable() {
    parent = nullptr;
    lastParent = nullptr;
}


void Containable::removeFromSet() {
    lastParent = removeFrom();
}
void Containable::addToSet() {
    addTo(lastParent);
    lastParent = nullptr;
}

bool Containable::addTo(Container* container) {
    if(container == nullptr)
        return(removeFrom());

    if(this->parent != nullptr && parent->getAsMudObject() != container->getAsMudObject()) {
        std::clog << "Non Null Parent" << std::endl;
        return(false);
    }

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
        return(nullptr);
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
        return(nullptr);
    return(parent->getAsRoom());
}
UniqueRoom* Containable::getUniqueRoomParent() {
    if(!parent)
            return(nullptr);
    return(parent->getAsUniqueRoom());
}
AreaRoom* Containable::getAreaRoomParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsAreaRoom());
}
Object* Containable::getObjectParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsObject());
}
Player* Containable::getPlayerParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsPlayer());
}
Monster* Containable::getMonsterParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsMonster());
}
Creature* Containable::getCreatureParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsCreature());
}

const BaseRoom* Containable::getConstRoomParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstRoom());
}
const UniqueRoom* Containable::getConstUniqueRoomParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstUniqueRoom());
}
const AreaRoom* Containable::getConstAreaRoomParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstAreaRoom());
}
const Object* Containable::getConstObjectParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstObject());
}
const Player* Containable::getConstPlayerParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstPlayer());
}
const Monster* Containable::getConstMonsterParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstMonster());
}

const Creature* Containable::getConstCreatureParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstCreature());
}
