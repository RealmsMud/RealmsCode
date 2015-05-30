/*
 * duel.cpp
 *	 Duel code
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
#include "mud.h"
#include <math.h>


//*********************************************************************
//						induel
//*********************************************************************

bool induel(const Player* player, const Player* target) {
	return(player && target && player->isDueling(target->getName()) && target->isDueling(player->getName()));
}


//*********************************************************************
//						wantsDuelMessages
//*********************************************************************

bool wantsDuelMessages(Socket* sock) {
	return(sock->getPlayer() && sock->getPlayer()->fd >= 0 && !sock->getPlayer()->flagIsSet(P_NO_DUEL_MESSAGES));
}


//*********************************************************************
//						duel
//*********************************************************************

int cmdDuel(Player* player, cmd* cmnd) {
	Player	*creature=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(player->getClass() == BUILDER) {
		player->print("You are not allowed to do that.\n");
		return(0);
	}

	if(player->getLevel() < 4 && !player->isCt() && !player->isHardcore()) {
		player->print("You must be level 4 or higher to be able to duel.\n");
		return(0);
	}



	if(cmnd->num == 1) {
		player->print("You are dueling: %s\n", player->showDueling().c_str());
		return(0);
	}

	cmnd->str[1][0] = up(cmnd->str[1][0]);
	creature = gServer->findPlayer(cmnd->str[1]);

	if(creature && player->canSee(creature)) {

		if(player->isDueling(creature->getName())) {
			player->delDueling(creature->getName());
			player->printColor("^R%s is now removed from your duel list.\n", creature->getCName());
			creature->printColor("^R%s has removed you from %s duel list.\n",
			      player->getCName(), player->hisHer());

			player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
			player->lasttime[LT_SPELL].ltime = time(0);
			player->lasttime[LT_SPELL].interval = 30L;

			return(0);
		}
	}



	creature = 0;

	if(cmnd->num == 2) {
		if(!strcmp(cmnd->str[1], "clear")) {
			player->print("Option disabled. Log off/on to clear your entire duel list at once.\n");
			return(0);
		}
	}

	cmnd->str[1][0] = up(cmnd->str[1][0]);
	creature = gServer->findPlayer(cmnd->str[1]);
	if(!creature || !player->canSee(creature)) {
		player->print("That player is not on.\n");
		return(0);
	}



	if(player->isDueling(creature->getName())) {
		player->printColor("^R%s is now removed from your duel list.\n", creature->getCName());
		creature->printColor("^R%s has removed you from %s duel list.\n",
		      player->getCName(), player->hisHer());
		logn("log.duel", "%s removed %s from %s duel list.\n",
		     player->getCName(), creature->getCName(), player->hisHer());
		player->delDueling(creature->getName());
		player->updateAttackTimer(true, 30);
		player->lasttime[LT_SPELL].ltime = time(0);
		player->lasttime[LT_SPELL].interval = 30L;
	} else {
		if(player == creature) {
			player->print("You cannot duel yourself.\n");
			return(0);
		}
		if(creature->isStaff() && !player->isDm()) {
			player->print("You cannot duel that player.\n");
			return(0);
		}
		if(creature->getLevel() < 4 && !player->isCt()) {
			player->print("Your opponent must be at least level 4.\n");
			return(0);
		}
		player->printColor("^R%s is now added to your duel list.\n", creature->getCName());
		creature->printColor("^R%s has added you to %s duel list.\n",
		      player->getCName(), player->hisHer());
		logn("log.duel", "%s added %s to %s duel list.\n",
		     player->getCName(), creature->getCName(), player->hisHer());
		player->addDueling(creature->getName());

		if(creature->isDueling(player->getName())) {

			broadcast(wantsDuelMessages, "### %s and %s have entered a duel to the death!",
				player->getCName(), creature->getCName());
			
		}
	}

	return(0);
}


