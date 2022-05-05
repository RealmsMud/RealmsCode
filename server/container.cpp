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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <boost/algorithm/string/trim.hpp>          // for trim
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <ostream>                                  // for operator<<, basic...
#include <set>                                      // for operator==, _Rb_t...
#include <string>                                   // for string, allocator
#include <utility>                                  // for pair

#include "area.hpp"                                 // for MapMarker
#include "catRef.hpp"                               // for CatRef
#include "cmd.hpp"                                  // for cmd
#include "flags.hpp"                                // for M_ANTI_MAGIC_AURA
#include "location.hpp"                             // for Location
#include "mudObjects/areaRooms.hpp"                 // for AreaRoom
#include "mudObjects/container.hpp"                 // for Container, Contai...
#include "mudObjects/creatures.hpp"                 // for Creature
#include "mudObjects/monsters.hpp"                  // for Monster
#include "mudObjects/mudObject.hpp"                 // for MudObject
#include "mudObjects/objects.hpp"                   // for Object
#include "mudObjects/players.hpp"                   // for Player
#include "mudObjects/uniqueRooms.hpp"               // for UniqueRoom
#include "proto.hpp"                                // for keyTxtEqual, broa...
#include "toNum.hpp"                                // for toNum

class BaseRoom;

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
    std::shared_ptr<Monster> mons = nullptr;
    for(mIt = monsters.begin() ; mIt != monsters.end() ; ) {
        prevIt = mIt;
        mons = (*mIt++);
        if(!includePets && mons->isPet()) {
            purgedAll = false;
            continue;
        }
        if(mons->flagIsSet(M_DM_FOLLOW)) {
            std::shared_ptr<Player> master = mons->getPlayerMaster();
            if(master) {
                master->clearFlag(P_ALIASING);

                master->setAlias(nullptr);
                master->print("%1M's soul was purged.\n", mons.get());
                master->delPet(mons->getAsMonster());
            }
        }

        monsters.erase(prevIt);
    }
    return(purgedAll);
}
bool Container::purgeObjects() {
    bool purgedAll = true;
    ObjectSet::iterator oIt, prevIt;
    std::shared_ptr<Object>  obj;
    for(oIt = objects.begin() ; oIt != objects.end() ; ) {
        prevIt = oIt;
        obj = (*oIt++);
        if(!obj->flagIsSet(O_TEMP_PERM)) {
            objects.erase(prevIt);
        } else {
            purgedAll = false;
        }
    }
    return(purgedAll);
}

std::shared_ptr<Container> Container::remove(const std::shared_ptr<Containable>& toRemove) {
    if(!toRemove)
        return(nullptr);

    auto remObject = dynamic_pointer_cast<Object>(toRemove);
    auto remPlayer = dynamic_pointer_cast<Player>(toRemove);
    auto remMonster = dynamic_pointer_cast<Monster>(toRemove);

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
        std::shared_ptr<Container> retVal = toRemove->getParent();
        toRemove->setParent(nullptr);
        return(retVal);
    }

    return(nullptr);
}

bool Container::add(const std::shared_ptr<Containable>& toAdd) {
    if(!toAdd)
        return(false);

    auto addObject = dynamic_pointer_cast<Object>(toAdd);
    auto addPlayer = dynamic_pointer_cast<Player>(toAdd);
    auto addMonster = dynamic_pointer_cast<Monster>(toAdd);

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
        toAdd->setParent(shared_from_this());
    }
    //std::clog << "Adding " << toAdd->getName() << " to " << this->getName() << std::endl;
    return(toReturn);
}


void Container::registerContainedItems() {
    // Player registration is handled by the server
    MonsterSet::iterator mIt;
    for(mIt = monsters.begin() ; mIt != monsters.end() ; ) {
        std::shared_ptr<Monster>  mons = *mIt++;
        mons->registerMo();
    }
    ObjectSet::iterator oIt;
    for(oIt = objects.begin() ; oIt != objects.end() ; ) {
        std::shared_ptr<Object>  obj = *oIt++;
        obj->registerMo();
    }
}
void Container::unRegisterContainedItems() {
    // Player registration is handled by the server
    for(const auto& mons : monsters) {
        mons->unRegisterMo();
    }
    for(const auto& obj : objects) {
        obj->unRegisterMo();
    }
}


//*********************************************************************
//                      wake
//*********************************************************************

void Container::wake(const std::string &str, bool noise) const {
    for(const auto& ply: players) {
        ply->wake(str, noise);
    }
}

bool Container::checkAntiMagic(const std::shared_ptr<Monster>&  ignore) {
    for(const auto& mons : monsters) {
        if(mons == ignore)
            continue;
        if(mons->flagIsSet(M_ANTI_MAGIC_AURA)) {
            broadcast(nullptr,  this, "^B%M glows bright blue.", mons.get());
            return(true);
        }
    }
    return(false);
}

std::shared_ptr<MudObject> Container::findTarget(const std::shared_ptr<const Creature>& searcher,  cmd* cmnd, int num) const {
    return(findTarget(searcher, cmnd->str[num], cmnd->val[num]));
}

std::shared_ptr<MudObject> Container::findTarget(const std::shared_ptr<const Creature>& searcher, const std::string& name, const int num,  bool monFirst, bool firstAggro, bool exactMatch) const {
    int match=0;
    return(findTarget(searcher, name, num, monFirst, firstAggro, exactMatch, match));
}

std::shared_ptr<MudObject> Container::findTarget(const std::shared_ptr<const Creature>& searcher, const std::string& name, const int num,  bool monFirst, bool firstAggro, bool exactMatch, int& match) const {
    std::shared_ptr<MudObject> toReturn = nullptr;

    if((toReturn = findCreature(searcher, name, num, monFirst, firstAggro, exactMatch, match))) {
        return(toReturn);
    }
    // TODO: Search the container's objects
    return(toReturn);
}

// Wrapper for the real findObject to support legacy callers
std::shared_ptr<Object>  Container::findObject(const std::shared_ptr<const Creature>& searcher, const cmd* cmnd, int val) const {
    return(findObject(searcher, cmnd->str[val], cmnd->val[val]));
}
std::shared_ptr<Object>  Container::findObject(const std::shared_ptr<const Creature>& searcher, const std::string& name, int num, bool exactMatch ) const {
    int match = 0;
    return(findObject(searcher, name, num,exactMatch, match));
}
std::shared_ptr<Object>  Container::findObject(const std::shared_ptr<const Creature>& searcher, const std::string& name, int num, bool exactMatch, int& match) const {
    std::shared_ptr<Object>target = nullptr;
    for(const auto& obj : objects) {
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
std::shared_ptr<Creature> Container::findCreature(const std::shared_ptr<const Creature>& searcher,  cmd* cmnd, int num) const {
    return(findCreature(searcher, cmnd->str[num], cmnd->val[num]));
}

std::shared_ptr<Creature> Container::findCreaturePython(const std::shared_ptr<Creature>& searcher, const std::string& name, bool monFirst, bool firstAggro, bool exactMatch ) {
    int ignored=0;
    int num = 1;

    std::string newName = name;
    boost::trim(newName);
    std::string::size_type sLoc = newName.find_last_of(' ');
    if(sLoc != std::string::npos) {
        num = toNum<int>(newName.substr(sLoc));
        if(num != 0) {
            newName = newName.substr(0, sLoc);
        } else {
            num = 1;
        }
    }

    std::clog << "Looking for '" << newName << "' #" << num << "\n";

    return(findCreature(searcher, newName, num, monFirst, firstAggro, exactMatch, ignored));
}

// Wrapper for the real findCreature for callers that don't care about the value of match
std::shared_ptr<Creature> Container::findCreature(const std::shared_ptr<const Creature>& searcher, const std::string& name, const int num, bool monFirst, bool firstAggro, bool exactMatch )  const {
    int ignored=0;
    return(findCreature(searcher, name, num, monFirst, firstAggro, exactMatch, ignored));
}
bool isMatch(const std::shared_ptr<const Creature>& searcher, const std::shared_ptr<MudObject>& target, const std::string& name, bool exactMatch, bool checkVisibility) {
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
std::shared_ptr<Creature> Container::findCreature(const std::shared_ptr<const Creature>& searcher, const std::string& name, const int num, bool monFirst, bool firstAggro, bool exactMatch, int& match) const {

    if(!searcher || name.empty())
        return(nullptr);

    std::shared_ptr<Creature> target=nullptr;
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

std::shared_ptr<Monster>  Container::findMonster(std::shared_ptr<const Creature> searcher,  cmd* cmnd, int num) const {
    return(findMonster(searcher, cmnd->str[num], cmnd->val[num]));
}
std::shared_ptr<Monster>  Container::findMonster(std::shared_ptr<const Creature> searcher, const std::string& name, const int num, bool firstAggro, bool exactMatch) const {
    int match = 0;
    return(findMonster(searcher, name, num, firstAggro, exactMatch, match));
}
std::shared_ptr<Monster>  Container::findMonster(const std::shared_ptr<const Creature>& searcher, const std::string& name, const int num, bool firstAggro, bool exactMatch, int& match) const {
    std::shared_ptr<Monster>  target = nullptr;
    for(const auto& mons : searcher->getParent()->monsters) {
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
std::shared_ptr<Player> Container::findPlayer(const std::shared_ptr<const Creature>& searcher, cmd* cmnd, int num) const {
    return(findPlayer(searcher, cmnd->str[num], cmnd->val[num]));
}
std::shared_ptr<Player> Container::findPlayer(const std::shared_ptr<const Creature>& searcher, const std::string& name, const int num, bool exactMatch) const {
    int match = 0;
    return(findPlayer(searcher, name, num, exactMatch, match));
}
std::shared_ptr<Player> Container::findPlayer(const std::shared_ptr<const Creature>& searcher, const std::string& name, const int num, bool exactMatch, int& match) const {
    for(const auto& ply: searcher->getParent()->players) {
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

bool Containable::addTo(const std::shared_ptr<Container>& container) {
    if(container == nullptr)
        return(removeFrom() != nullptr);

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
    return(container->add(shared_from_this()));
}

std::shared_ptr<Container> Containable::removeFrom() {
    if(!parent) {
        return(nullptr);
    }

    if(isCreature()) {
        getAsCreature()->currentLocation.room.clear();
        getAsCreature()->currentLocation.mapmarker.reset();
    }
    return(parent->remove(shared_from_this()));
}

void Containable::setParent(std::shared_ptr<Container> container) {
    parent = container;
}
std::shared_ptr<Container> Containable::getParent() const {
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

std::shared_ptr<BaseRoom> Containable::getRoomParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsRoom());
}
std::shared_ptr<UniqueRoom> Containable::getUniqueRoomParent() {
    if(!parent)
            return(nullptr);
    return(parent->getAsUniqueRoom());
}
std::shared_ptr<AreaRoom> Containable::getAreaRoomParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsAreaRoom());
}
std::shared_ptr<Object>  Containable::getObjectParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsObject());
}
std::shared_ptr<Player> Containable::getPlayerParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsPlayer());
}
std::shared_ptr<Monster>  Containable::getMonsterParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsMonster());
}
std::shared_ptr<Creature> Containable::getCreatureParent() {
    if(!parent)
        return(nullptr);
    return(parent->getAsCreature());
}

std::shared_ptr<const BaseRoom> Containable::getConstRoomParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstRoom());
}
std::shared_ptr<const UniqueRoom> Containable::getConstUniqueRoomParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstUniqueRoom());
}
std::shared_ptr<const AreaRoom> Containable::getConstAreaRoomParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstAreaRoom());
}
std::shared_ptr<const Object>  Containable::getConstObjectParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstObject());
}
std::shared_ptr<const Player> Containable::getConstPlayerParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstPlayer());
}
std::shared_ptr<const Monster>  Containable::getConstMonsterParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstMonster());
}

std::shared_ptr<const Creature> Containable::getConstCreatureParent() const {
    if(!parent)
        return(nullptr);
    return(parent->getAsConstCreature());
}
