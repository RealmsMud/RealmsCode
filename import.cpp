/*
 * import.cpp
 *	 Functions for viewing and restoring old characters.
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
#include "import.h"
#include "commands.h"

int lookup(Player* player, cmd* cmnd) {
	Player	*target=0;
	
	
	// dms can still lookup
	if(!player->isDm()) {
		player->print("Lookup has been disabled!\n");
		return(0);
	}

	// Let dms look up any character
	if((cmnd->num < 3 && !player->isDm()) || (cmnd->num < 2 && player->isDm())) {
		player->print("Syntax: lookup <player> <password>\n");
		return(0);
	}
	cmnd->str[1][0] = up(cmnd->str[1][0]);

	if(!loadPlayer(cmnd->str[1], &target, LS_CONVERT)) {
		player->print("Player does not exist.\n");
		return(0);
	} 

	if(!player->isStaff() && target->isStaff()) {
		player->print("You cannot lookup that person at this time.\n");
		free_crt(target);
		return(0);
	}
	if(!player->isDm()) {
		if(!player->isPassword(getFullstrText(cmnd->fullstr, 2))) {
			player->print("Incorrect password!\n");
			free_crt(target, false);
			return(0);	
		}
	}

	target->information(player, false);
	free_crt(target);
	return(0);
}


int restore(Player* player, cmd* cmnd) {
	Player	*target=0;
	
	
	// dms can still restore
	if(!player->isDm()) {
		player->print("Restore has been disabled!\n");
		return(0);
	}
	
	if(cmnd->num < 4) {
		player->print("Syntax: restore <player> <password> <newname>\n");
		return(0);
	}
	cmnd->str[1][0] = up(cmnd->str[1][0]);
	cmnd->str[3][0] = up(cmnd->str[3][0]);

	if(!nameIsAllowed(cmnd->str[3], player->getSock())) {
		player->print("Please pick an appropriate name to restore to!\n");
		return(0);			
	}
	
	if(loadPlayer(cmnd->str[3], &target)) {
		player->print("%s already exists, please pick a different name or suicide that character!\n", cmnd->str[3]);
		free_crt(target);
		return(0);
	} 

	if(!loadPlayer(cmnd->str[1], &target, LS_CONVERT)) {
		player->print("Player does not exist.\n");
		return(0);
	} 

	if(!player->isStaff() && target->isStaff()) {
		player->print("You cannot restore that person at this time.\n");
		free_crt(player);
		return(0);
	}
	if(!player->isPassword(getFullstrText(cmnd->fullstr, 2))) {
		player->print("Incorrect password!\n");
		free_crt(target, false);
		return(0);	
	}

	target->bound.room.id = 2;
	target->setGuild(0);
	target->setGuildRank(0);
	target->currentLocation.room.id = 1;
	target->clearFlag(P_WATCHER);

	target->setName( cmnd->str[3]);
	//strcpy(target->password, cmnd->str[2]);
	target->saveToFile();
	free_crt(target);
	player->print("%s sucessfully restored to %s!\n", cmnd->str[1], cmnd->str[3]);
	logn("log.import", "%s restored %s to %s.\n", player->getCName(), cmnd->str[1], cmnd->str[3]);
	
	// Rename the character to prevent multi-restoring of characters
	char oldName[200], newName[200];
	sprintf(oldName, "%s/convert/%s.xml", Path::Player, cmnd->str[1]);
	sprintf(newName, "%s/convert/%s.converted", Path::Player, cmnd->str[1]);
	rename(oldName, newName);
	 	
	return(0);
}
