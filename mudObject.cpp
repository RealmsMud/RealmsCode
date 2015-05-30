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
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
// Mud Includes
#include "mud.h"
#include "version.h"
#include "commands.h"
#include "effects.h"
#include "specials.h"
#include "calendar.h"
#include "quests.h"
#include "guilds.h"
#include "property.h"

// C++ includes
#include <sstream>
#include <iomanip>
#include <locale>
//#include <c++/4.3.3/bits/stl_list.h>
#include <list>


void MudObject::setName(bstring newName) {
	removeFromSet();
	name = newName;
	addToSet();
}

const bstring& MudObject::getName() const {
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


bool MudObject::registerMo() {
	if(registered) {
		//std::cout << "ERROR: Attempting to register a MudObject that thinks it is already registered" << std::endl;
		return(false);
	}

	if(gServer->registerMudObject(this)) {
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
	return;
}
void MudObject::unRegisterContainedItems() {
	return;
}

bool PlayerPtrLess::operator()(const Player* lhs, const Player* rhs) const {
    return *lhs < *rhs;
}

bool MonsterPtrLess::operator()(const Monster* lhs, const Monster* rhs) const {
    bstring lhsStr = lhs->getName() + lhs->id;
    bstring rhsStr = rhs->getName() + rhs->id;
    return(lhsStr.compare(rhsStr) < 0);
}

bool ObjectPtrLess::operator()(const Object* lhs, const Object* rhs) const {
	return *lhs < *rhs;
}

MudObject::MudObject() {
	moReset();
	registered = false;
}

MudObject::~MudObject() {
	unRegisterMo();
}
//***********************************************************************
//						moReset
//***********************************************************************

void MudObject::moReset() {
	id = "-1";
	name = "";
}

//*********************************************************************
//						moCopy
//*********************************************************************

void MudObject::moCopy(const MudObject& mo) {
	name = mo.getName();

	hooks = mo.hooks;
	hooks.setParent(this);
}

//***********************************************************************
//						moDestroy
//***********************************************************************

void MudObject::moDestroy() {
	unRegisterMo();
	moReset();
}

MudObject* MudObject::getAsMudObject() {
    return(dynamic_cast<MudObject*>(this));
}


//***********************************************************************
//						getMonster
//***********************************************************************

Monster* MudObject::getAsMonster() {
    return(dynamic_cast<Monster*>(this));
}

//***********************************************************************
//						getCreature
//***********************************************************************

Creature* MudObject::getAsCreature() {
    return(dynamic_cast<Creature*>(this));
}

//***********************************************************************
//						getPlayer
//***********************************************************************

Player* MudObject::getAsPlayer() {
	return(dynamic_cast<Player*>(this));
}

//***********************************************************************
//						getObject
//***********************************************************************

Object* MudObject::getAsObject() {
	return(dynamic_cast<Object*>(this));
}

//***********************************************************************
//						getUniqueRoom
//***********************************************************************

UniqueRoom* MudObject::getAsUniqueRoom() {
	return(dynamic_cast<UniqueRoom*>(this));
}

//***********************************************************************
//						getAreaRoom
//***********************************************************************

AreaRoom* MudObject::getAsAreaRoom() {
	return(dynamic_cast<AreaRoom*>(this));
}

//***********************************************************************
//						getRoom
//***********************************************************************

BaseRoom* MudObject::getAsRoom() {
	return(dynamic_cast<BaseRoom*>(this));
}

//***********************************************************************
//						getExit
//***********************************************************************

Exit* MudObject::getAsExit() {
	return(dynamic_cast<Exit*>(this));
}

//***********************************************************************
//						getConstMonster
//***********************************************************************

const Monster* MudObject::getAsConstMonster() const {
	return(dynamic_cast<const Monster*>(this));
}

//***********************************************************************
//						getConstPlayer
//***********************************************************************

const Player* MudObject::getAsConstPlayer() const {
	return(dynamic_cast<const Player*>(this));
}

//***********************************************************************
//						getConstCreature
//***********************************************************************

const Creature* MudObject::getAsConstCreature() const {
	return(dynamic_cast<const Creature*>(this));
}

//***********************************************************************
//						getConstObject
//***********************************************************************

const Object* MudObject::getAsConstObject() const {
	return(dynamic_cast<const Object*>(this));
}

//***********************************************************************
//						getConstUniqueRoom
//***********************************************************************

const UniqueRoom* MudObject::getAsConstUniqueRoom() const {
	return(dynamic_cast<const UniqueRoom*>(this));
}

//***********************************************************************
//						getConstAreaRoom
//***********************************************************************

const AreaRoom* MudObject::getAsConstAreaRoom() const {
	return(dynamic_cast<const AreaRoom*>(this));
}

//***********************************************************************
//						getConstRoom
//***********************************************************************

const BaseRoom* MudObject::getAsConstRoom() const {
	return(dynamic_cast<const BaseRoom*>(this));
}

//***********************************************************************
//						getConstExit
//***********************************************************************

const Exit* MudObject::getAsConstExit() const {
	return(dynamic_cast<const Exit*>(this));
}

//***********************************************************************
//						equals
//***********************************************************************

bool MudObject::equals(MudObject* other) {
    return(this == other);
}


void MudObject::setId(bstring newId, bool handleParentSet) {
	if(!id.equals("-1") && !newId.equals("-1")) {
	    throw std::runtime_error(bstring("Error, re-setting ID:") + getName() + ":" + getId() + ":" + newId);
	}

	if(newId.equals("-1"))
		handleParentSet = false;

	if(!newId.equals("")) {
		if(handleParentSet) removeFromSet();
		id = newId;
		if(handleParentSet) addToSet();

		// Handle registration elsewhere
//		// Register the ID with the server if we're not a player, handle player registration
//		// when adding the player to the server's list of players
//		if(!getAsPlayer()) {
//			if(getAsCreature() && gServer->lookupCrtId(newId)) {
//				// We already have a creature with this registered ID, so this creature needs a new ID
//				id = gServer->getNextMonsterId();
//				std::cout << "Changing ID for " << this->getName() << " from "<< newId << " to " << id << std::endl;
//			} else if(getAsObject() && gServer->lookupObjId(newId)) {
//				throw std::runtime_error("Duplicate Object ");
//
//			}
//			gServer->registerMudObject(this);
//		}
	}
}

const bstring& MudObject::getId() const {
    return(id);
}

// Return for Python which doesn't like bstring&
bstring MudObject::getIdPython() const {
	return(id);
}

bool MudObject::isRoom() const {
	return(this &&
	  	  (typeid(*this) == typeid(BaseRoom) ||
		   typeid(*this) == typeid(UniqueRoom) ||
		   typeid(*this) == typeid(AreaRoom)));
}
bool MudObject::isUniqueRoom() const {
	return(this && typeid(*this) == typeid(UniqueRoom));
}
bool MudObject::isAreaRoom() const {
	return(this && typeid(*this) == typeid(AreaRoom));
}
bool MudObject::isObject() const {
	return(this && typeid(*this) == typeid(Object));
}
bool MudObject::isPlayer() const {
	return(this && typeid(*this) == typeid(Player));
}
bool MudObject::isMonster() const {
	return(this && typeid(*this) == typeid(Monster));
}
bool MudObject::isCreature() const {
	return(this &&
		  (typeid(*this) == typeid(Creature) ||
		   typeid(*this) == typeid(Player) ||
		   typeid(*this) == typeid(Monster)));
}
bool MudObject::isExit() const {
    return(this && typeid(*this) == typeid(Exit));
}

