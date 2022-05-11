/*
 * mudObjects.cpp
 *   Functions that work on mudObjects etc
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

#include <fmt/format.h>                // for format
#include <stdexcept>                   // for runtime_error
#include <string>                      // for string, allocator, char_traits
#include <string_view>                 // for string_view, operator==, basic...

#include <typeinfo>                    // for type_info
#include "hooks.hpp"                   // for Hooks
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/container.hpp"    // for MonsterPtrLess, ObjectPtrLess
#include "mudObjects/creatures.hpp"    // for Creature
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/mudObject.hpp"    // for MudObject
#include "mudObjects/objects.hpp"      // for Object
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "server.hpp"                  // for Server, gServer



void MudObject::setName(std::string_view newName) {
    removeFromSet();
    name = newName;
    addToSet();
}

const std::string & MudObject::getName() const {
    return(name);
}
const char* MudObject::getCName() const {
    return(name.c_str());
}

void MudObject::removeFromSet() {

}
void MudObject::addToSet() {

}


bool MudObject::isRegistered() {
    return(registered);
}
void MudObject::setRegistered() {
    if(registered)
        throw std::runtime_error("Already Registered!");
    registered = true;

}
void MudObject::setUnRegistered() {
    registered = false;
}


bool MudObject::registerMo(const std::shared_ptr<MudObject>& mo) {
    if(registered) {
        //std::clog << "ERROR: Attempting to register a MudObject that thinks it is already registered" << std::endl;
        return(false);
    }

    if(gServer->registerMudObject(mo)) {
        registerContainedItems();
        return(true);
    }

    return(false);
}

bool MudObject::unRegisterMo() {
    if(!registered)
        return(false);
    if(gServer->unRegisterMudObject(this)) {
        unRegisterContainedItems();
        return(true);
    }
    return(false);
}

void MudObject::registerContainedItems() {
}
void MudObject::unRegisterContainedItems() {
}

bool PlayerPtrLess::operator()(const std::shared_ptr<Player>& lhs, const std::shared_ptr<Player>& rhs) const {
    return *lhs < *rhs;
}

bool MonsterPtrLess::operator()(const std::shared_ptr<Monster>&  lhs, const std::shared_ptr<Monster>&  rhs) const {
    std::string lhsStr = lhs->getName() + lhs->id;
    std::string rhsStr = rhs->getName() + rhs->id;
    return(lhsStr.compare(rhsStr) < 0);
}

bool ObjectPtrLess::operator()(const std::shared_ptr<Object>&  lhs, const std::shared_ptr<Object>&  rhs) const {
    return *lhs < *rhs;
}

MudObject::MudObject() {
    moReset();
    registered = false;
}

MudObject::~MudObject() {
    unRegisterMo();
    moReset();
}
//***********************************************************************
//                      moReset
//***********************************************************************

void MudObject::moReset() {
    id = "-1";
    name = "";
}

//*********************************************************************
//                      moCopy
//*********************************************************************

void MudObject::moCopy(const MudObject& mo) {
    name = mo.getName();

    hooks = mo.hooks;
    hooks.setParent(this);
}

std::shared_ptr<MudObject> MudObject::getAsMudObject() {
    return(shared_from_this());
}


//***********************************************************************
//                      getMonster
//***********************************************************************

std::shared_ptr<Monster>  MudObject::getAsMonster() {
    return(std::dynamic_pointer_cast<Monster>(shared_from_this()));
}

//***********************************************************************
//                      getCreature
//***********************************************************************

std::shared_ptr<Creature> MudObject::getAsCreature() {
    return(std::dynamic_pointer_cast<Creature>(shared_from_this()));
}

//***********************************************************************
//                      getPlayer
//***********************************************************************

std::shared_ptr<Player> MudObject::getAsPlayer() {
    return(std::dynamic_pointer_cast<Player>(shared_from_this()));
}

//***********************************************************************
//                      getObject
//***********************************************************************

std::shared_ptr<Object>  MudObject::getAsObject() {
    return(std::dynamic_pointer_cast<Object>(shared_from_this()));
}

//***********************************************************************
//                      getUniqueRoom
//***********************************************************************

std::shared_ptr<UniqueRoom> MudObject::getAsUniqueRoom() {
    return(std::dynamic_pointer_cast<UniqueRoom>(shared_from_this()));
}

//***********************************************************************
//                      getAreaRoom
//***********************************************************************

std::shared_ptr<AreaRoom> MudObject::getAsAreaRoom() {
    return(std::dynamic_pointer_cast<AreaRoom>(shared_from_this()));
}

//***********************************************************************
//                      getRoom
//***********************************************************************

std::shared_ptr<BaseRoom> MudObject::getAsRoom() {
    return(std::dynamic_pointer_cast<BaseRoom>(shared_from_this()));
}

//***********************************************************************
//                      getExit
//***********************************************************************

std::shared_ptr<Exit> MudObject::getAsExit() {
    return(std::dynamic_pointer_cast<Exit>(shared_from_this()));
}

//***********************************************************************
//                      getConstMonster
//***********************************************************************

std::shared_ptr<const Monster>  MudObject::getAsConstMonster() const {
    return(std::dynamic_pointer_cast<const Monster>(shared_from_this()));
}

//***********************************************************************
//                      getConstPlayer
//***********************************************************************

std::shared_ptr<const Player> MudObject::getAsConstPlayer() const {
    return(std::dynamic_pointer_cast<const Player>(shared_from_this()));
}

//***********************************************************************
//                      getConstCreature
//***********************************************************************

std::shared_ptr<const Creature> MudObject::getAsConstCreature() const {
    return(std::dynamic_pointer_cast<const Creature>(shared_from_this()));
}

//***********************************************************************
//                      getConstObject
//***********************************************************************

std::shared_ptr<const Object>  MudObject::getAsConstObject() const {
    return(std::dynamic_pointer_cast<const Object>(shared_from_this()));
}

//***********************************************************************
//                      getConstUniqueRoom
//***********************************************************************

std::shared_ptr<const UniqueRoom> MudObject::getAsConstUniqueRoom() const {
    return(std::dynamic_pointer_cast<const UniqueRoom>(shared_from_this()));
}

//***********************************************************************
//                      getConstAreaRoom
//***********************************************************************

std::shared_ptr<const AreaRoom> MudObject::getAsConstAreaRoom() const {
    return(std::dynamic_pointer_cast<const AreaRoom>(shared_from_this()));
}

//***********************************************************************
//                      getConstRoom
//***********************************************************************

std::shared_ptr<const BaseRoom> MudObject::getAsConstRoom() const {
    return(std::dynamic_pointer_cast<const BaseRoom>(shared_from_this()));
}

//***********************************************************************
//                      getConstExit
//***********************************************************************

std::shared_ptr<const Exit> MudObject::getAsConstExit() const {
    return(std::dynamic_pointer_cast<const Exit>(shared_from_this()));
}

//***********************************************************************
//                      equals
//***********************************************************************

bool MudObject::equals(const std::shared_ptr<MudObject>& other) {
    return(this == other.get());
}


void MudObject::setId(std::string_view newId, bool handleParentSet) {
    if(id != "-1" && newId != "-1") {
        throw std::runtime_error(fmt::format("Error, re-setting ID:{}:{}:{}", getName(), getId(), newId));
    }

    if(newId == "-1")
        handleParentSet = false;

    if(!newId.empty()) {
        if(handleParentSet) removeFromSet();
        id = newId;
        if(handleParentSet) addToSet();
    }
}

const std::string & MudObject::getId() const {
    return(id);
}

bool MudObject::isRoom() const {
    return(typeid(*this) == typeid(BaseRoom) ||
           typeid(*this) == typeid(UniqueRoom) ||
           typeid(*this) == typeid(AreaRoom));
}
bool MudObject::isUniqueRoom() const {
    return(typeid(*this) == typeid(UniqueRoom));
}
bool MudObject::isAreaRoom() const {
    return(typeid(*this) == typeid(AreaRoom));
}
bool MudObject::isObject() const {
    return(typeid(*this) == typeid(Object));
}
bool MudObject::isPlayer() const {
    return(typeid(*this) == typeid(Player));
}
bool MudObject::isMonster() const {
    return(typeid(*this) == typeid(Monster));
}
bool MudObject::isCreature() const {
    return(typeid(*this) == typeid(Creature) ||
           typeid(*this) == typeid(Player) ||
           typeid(*this) == typeid(Monster));
}
bool MudObject::isExit() const {
    return(typeid(*this) == typeid(Exit));
}

std::string MudObject::getFlagList(std::string_view sep) const {
    return "";
}
