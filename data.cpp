/*
 * data.cpp
 *	 Data storage file
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
#include "mud.h"
#include "calendar.h"
//#include "factions.h"
//#include "commands.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <iostream>
#include <fstream>

//*********************************************************************
//						moCopy
//*********************************************************************

void MudObject::moCopy(const MudObject& mo) {
	strcpy(name, mo.name);

	hooks = mo.hooks;
	hooks.setParent(this);
}
