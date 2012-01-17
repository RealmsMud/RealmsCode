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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
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


//***********************************************************************
//						moReset
//***********************************************************************

void MudObject::moReset() {
	id = "-1";
}

//***********************************************************************
//						moDestroy
//***********************************************************************

void MudObject::moDestroy() {
	// If it's a valid ID and it's not a player, unregister their ID.  Players
	// are unregistered by the server
	if(!id.equals("-1") && !getPlayer()) {
		gServer->unRegisterMudObject(this);
	}
	moReset();
}



//***********************************************************************
//						getMonster
//***********************************************************************

Monster* MudObject::getMonster() {
	return(dynamic_cast<Monster*>(this));
}

//***********************************************************************
//						getCreature
//***********************************************************************

Creature* MudObject::getCreature() {
	return(dynamic_cast<Creature*>(this));
}

//***********************************************************************
//						getPlayer
//***********************************************************************

Player* MudObject::getPlayer() {
	return(dynamic_cast<Player*>(this));
}

//***********************************************************************
//						getObject
//***********************************************************************

Object* MudObject::getObject() {
	return(dynamic_cast<Object*>(this));
}

//***********************************************************************
//						getUniqueRoom
//***********************************************************************

UniqueRoom* MudObject::getUniqueRoom() {
	return(dynamic_cast<UniqueRoom*>(this));
}

//***********************************************************************
//						getAreaRoom
//***********************************************************************

AreaRoom* MudObject::getAreaRoom() {
	return(dynamic_cast<AreaRoom*>(this));
}

//***********************************************************************
//						getRoom
//***********************************************************************

BaseRoom* MudObject::getRoom() {
	return(dynamic_cast<BaseRoom*>(this));
}

//***********************************************************************
//						getExit
//***********************************************************************

Exit* MudObject::getExit() {
	return(dynamic_cast<Exit*>(this));
}

//***********************************************************************
//						getConstMonster
//***********************************************************************

const Monster* MudObject::getConstMonster() const {
	return(dynamic_cast<const Monster*>(this));
}

//***********************************************************************
//						getConstPlayer
//***********************************************************************

const Player* MudObject::getConstPlayer() const {
	return(dynamic_cast<const Player*>(this));
}

//***********************************************************************
//						getConstCreature
//***********************************************************************

const Creature* MudObject::getConstCreature() const {
	return(dynamic_cast<const Creature*>(this));
}

//***********************************************************************
//						getConstObject
//***********************************************************************

const Object* MudObject::getConstObject() const {
	return(dynamic_cast<const Object*>(this));
}

//***********************************************************************
//						getConstUniqueRoom
//***********************************************************************

const UniqueRoom* MudObject::getConstUniqueRoom() const {
	return(dynamic_cast<const UniqueRoom*>(this));
}

//***********************************************************************
//						getConstAreaRoom
//***********************************************************************

const AreaRoom* MudObject::getConstAreaRoom() const {
	return(dynamic_cast<const AreaRoom*>(this));
}

//***********************************************************************
//						getConstRoom
//***********************************************************************

const BaseRoom* MudObject::getConstRoom() const {
	return(dynamic_cast<const BaseRoom*>(this));
}

//***********************************************************************
//						getConstExit
//***********************************************************************

const Exit* MudObject::getConstExit() const {
	return(dynamic_cast<const Exit*>(this));
}

//***********************************************************************
//						getName
//***********************************************************************

const char* MudObject::getName() const {
	return(name);
}

//***********************************************************************
//						equals
//***********************************************************************

bool MudObject::equals(MudObject* other) {
    return(this == other);
}


void MudObject::setId(bstring newId) {
	if(!id.equals("-1") && !newId.equals("-1")) {
	    bstring error = bstring("Error, re-setting ID:") + getName() + ":" + getId() + ":" + newId;
	    throw error;
	}
	if(!newId.equals("")) {
		id = newId;
		// Register the ID with the server if we're not a player, handle player registration
		// when adding the player to the server's list of players
		if(!getPlayer()) {
			gServer->registerMudObject(this);
		}
	}
}

const bstring& MudObject::getId() const {
    return(id);
}
// Return for Python which doesn't like bstring&
bstring MudObject::getIdPython() const {
	return(id);
}
