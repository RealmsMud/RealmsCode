/*
 * mccp.cpp
 *	 Handle Mud Client Compression Protocol, Modified for use with mordor by Bane
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
// Copyright (c) 1999, Oliver Jowett <icecube@ihug.co.nz>.
// This code may be freely distributed and used if this copyright notice is retained intact.

#include "mud.h"

int mccp(Player* player, cmd* cmnd) {
	if(!player)
		return(0);

	if(player->getSock()->getMccp() == 0) {
		player->print("Attempting to enable MCCP.\n");
		player->print("%s", telnet::will_comp2);
		player->print("%s", telnet::will_comp1);
	} else {
		player->print("Ending compression.\n");
		player->getSock()->endCompress();
	}
	return(0);
}
