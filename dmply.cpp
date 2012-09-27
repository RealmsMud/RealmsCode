/*
 * dmply.cpp
 *	 Staff functions related to players
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 *	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "commands.h"
#include "dm.h"
#include "guilds.h"
#include "bank.h"
#include "login.h"
#include "unique.h"
#include "web.h"
#include <sys/stat.h>


//*********************************************************************
//						dmLastCommand
//*********************************************************************

bstring dmLastCommand(const Player* player) {
	if(player->isPassword(player->getLastCommand()))
		return("**********");
	return(escapeColor(player->getLastCommand()));
}


//*********************************************************************
//						dmForce
//*********************************************************************
// This function allows a DM to force another user to do a command.

int dmForce(Player* player, cmd* cmnd) {
	Player	*target=0;
	int		index=0;
	bstring::size_type i=0;
	char	str[IBUFSIZE+1];

	if(cmnd->num < 2) {
		player->print("*force whom?\n");
		return(0);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);
	if(!target) {
		player->print("%s is not on.\n", cmnd->str[1]);
		return(0);
	}
	if(target == player) {
		player->print("You can't *force yourself!\n");
		return(0);
	}

	if(!target->getSock()->canForce()) {
		player->print("Can not force %s right now.\n", cmnd->str[1]);
		return(0);
	}

	for(i=0; i<cmnd->fullstr.length(); i++)
		if(cmnd->fullstr[i] == ' ') {
			index = i+1;
			break;
		}
	for(i=index; i<cmnd->fullstr.length(); i++)
		if(cmnd->fullstr[i] != ' ') {
			index = i+1;
			break;
		}
	for(i=index; i<cmnd->fullstr.length(); i++)
		if(cmnd->fullstr[i] == ' ') {
			index = i+1;
			break;
		}
	for(i=index; i<cmnd->fullstr.length(); i++)
		if(cmnd->fullstr[i] != ' ') {
			index = i;
			break;
		}
	strcpy(str, &cmnd->fullstr[index]);
	if(target->getClass() >= player->getClass())
		target->print("%s forces you to \"%s\".\n", player->name, str);

	bstring txt = escapeColor(str);
	log_immort(true, player, "%s forced %s to \"%s\".\n", player->name, target->name, txt.c_str());
	command(target->getSock(), str);

	return(0);
}


//*********************************************************************
//						dmSpy
//*********************************************************************

int dmSpy(Player* player, cmd* cmnd) {
	Player	*target=0;


	if(cmnd->num < 2 && !player->flagIsSet(P_SPYING)) {
		player->print("Spy on whom?\n");
		return(0);
	}

	if(player->flagIsSet(P_SPYING)) {
		player->getSock()->clearSpying();
		player->clearFlag(P_SPYING);
		player->print("Spy mode off.\n");
		if(!player->isDm())
			log_immort(false,player, "%s turned spy mode off.\n", player->name);
		return(0);
	}

	cmnd->str[1][0] = up(cmnd->str[1][0]);
	target = gServer->findPlayer(cmnd->str[1]);
	if(!target || !player->canSee(target)) {
		player->print("Spy on whom?  Use full names.\n");
		return(0);
	}

	if(target->isDm() && !player->isDm()) {
		player->print("You cannot spy on DM's!\n");
		target->printColor("^r%M tried to spy on you.\n", player);
		return(0);
	}

	if(target == player) {
		player->print("You can't spy on yourself.\n");
		return(0);
	}

	if(player->getClass() <= target->getClass())
		target->printColor("^r%s is observing you.\n", player->name);

	player->getSock()->setSpying(target->getSock());

	player->setFlag(P_SPYING);
	player->setFlag(P_DM_INVIS);

	player->print("Spy on. Type *spy to turn it off.\n");
	if(!player->isDm())
		log_immort(false,player, "%s started spying on %s.\n", player->name, target->name);
	return(0);
}


//*********************************************************************
//						dmSilence
//*********************************************************************
// This function sets it so a player can't broadcast for 10 minutes or
// however long specified by [minutes]

int dmSilence(Player* player, cmd* cmnd) {
	Creature* target=0;


	if(cmnd->num < 2) {
		player->print("syntax: *silence <player> [minutes]/[r]\n");
		return(0);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target || !player->canSee(target)) {
		player->print("That player is not logged on.\n");
		return(0);
	}

	target->print("You have been silenced by the gods.\n");
	target->lasttime[LT_NO_BROADCAST].ltime = time(0);


	if(cmnd->num > 2 && low(cmnd->str[2][0]) == 'r') {
		if(target->flagIsSet(P_CANT_BROADCAST)) {
			player->print("%s can now broadcast again.\n", target->name);
			target->clearFlag(P_CANT_BROADCAST);
		} else {
			player->print("But %s can already broadcast!\n", target->name);
		}
	} else {
		if(cmnd->val[1]) {
			target->lasttime[LT_NO_BROADCAST].interval = cmnd->val[1] * 60L;
			player->print("%s silenced for %d minute(s).\n", target->name, cmnd->val[1]);
		} else {
			player->print("%s silenced for 10 minutes.\n", target->name);
			target->lasttime[LT_NO_BROADCAST].interval = 600L;
		}
		target->setFlag(P_CANT_BROADCAST);
		log_immort(true, player, "%s removed %s's ability to broadcast.\n", player->name, target->name);
	}
	return(0);
}


//*********************************************************************
//						dmTitle
//*********************************************************************

int dmTitle(Player* player, cmd* cmnd) {
	Player	*target=0;
	bstring title = "";

	if(cmnd->num < 3) {
		player->print("\nSyntax: *title <player> [<title>|-d]\n");
		return(PROMPT);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target || !player->canSee(target)) {
		player->print("Player not found.\n");
		return(0);
	}

	if(!player->isDm() && target != player) {
		player->print("You can only set your own title.\n");
		return(0);
	}

	title = getFullstrText(cmnd->fullstr, 2);

	if(title == "-d") {
		target->setTitle("");
		player->print("\nTitle cleared.\n");
	} else {
		title = escapeColor(title);
		target->setTitle(title);
		player->print("\nTitle Set\n");
	}
	return(0);
}

//*********************************************************************
//						dmSurname
//*********************************************************************

int dmSurname(Player* player, cmd* cmnd) {
	Player	*target=0;
	int		i=0;
	char	which=0;

	if(cmnd->num < 3) {
		player->print("\nSyntax: *surname (player) [(surname)|-d|-l]\n");
		return(PROMPT);
	}

	// Parse the flag here
	while(isspace(cmnd->fullstr[i]))
		i++;
	while(!isspace(cmnd->fullstr[i]))
		i++;
	while(isspace(cmnd->fullstr[i]))
		i++;
	while(isalpha(cmnd->fullstr[i]))
		i++;
	while(isspace(cmnd->fullstr[i]))
		i++;

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target || !player->canSee(target)) {
		player->print("Player not found.\n");
		return(PROMPT);
	}

	if(!player->isDm() && target->isStaff()) {
		player->print("You can only set the surnames of players.\n");
		return(0);
	}



	while(isdigit(cmnd->fullstr[i]))
		i++;
	while(isspace(cmnd->fullstr[i]))
		i++;

	// parse flag
	if(cmnd->fullstr[i] == '-') {
		if(cmnd->fullstr[i + 1] == 'd') {
			which = 1;
			i += 2;
		} else if(cmnd->fullstr[i + 1] == 'l') {
			which = 2;
			i += 2;
		}
		while(isspace(cmnd->fullstr[i]))
			i++;
	}



	switch (which) {
	case 0:
		target->setSurname(&cmnd->fullstr[i]);
		player->print("\nSurname set\n");
		target->setFlag(P_CHOSEN_SURNAME);
		break;
	case 1:
		target->setSurname("");
		player->print("\nSurname cleared.\n");
		target->clearFlag(P_CHOSEN_SURNAME);
		break;
	case 2:
		target->setSurname("");
		player->print("\n%s's surname command is now locked.\n", target->name);
		target->setFlag(P_NO_SURNAME);
		break;
	default:
		player->print("\nNothing Done.\n");
		break;
	}
	return(0);
}


//*********************************************************************
//						dmGroup
//*********************************************************************
// This function allows you to see who is in a group or party of people
// who are following you.

int dmGroup(Player* player, cmd* cmnd) {
	Creature *target=0;

	if(cmnd->num < 2) {
	    player->printColor("Active Groups: \n%s", gServer->getGroupList().c_str());
		return(PROMPT);
	}

	target = player->getParent()->findCreature(player, cmnd);

	if(!target) {
		lowercize(cmnd->str[1], 1);
		target = gServer->findPlayer(cmnd->str[1]);
	}

	if(!target || !player->canSee(target)) {
		player->print("That player is not logged on.\n");
		return(PROMPT);
	}

	std::ostringstream oStr;

	Group* group = target->getGroup(false);
	if(group) {
	    if(target->getGroupStatus() == GROUP_INVITED) {
	        oStr << "Invited to join " << group->getName() << " (" << group->getLeader()->getName() << ")." << std::endl;
	    } else {
	        switch(target->getGroupStatus()) {
	            case GROUP_MEMBER:
	                oStr << "Member of ";
	                break;
	            case GROUP_LEADER:
	                oStr << "Leader of ";
	                break;
	            default:
	                oStr << "Unknown in ";
	                break;
	        }
	        oStr << group;
        }
	} else {
	    player->print("%M is not a member of a group.\n", target);
	}
	return(0);
}

//*********************************************************************
//						dmDust
//*********************************************************************
// This function allows staff to delete a player.

int dmDust(Player* player, cmd* cmnd) {
	Player	*target=0;
	char	buf[120];

	if(cmnd->num < 2) {
		player->print("\nDust whom?\n");
		return(PROMPT);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target || !player->canSee(target)) {
		player->print("%s is not on.\n", cmnd->str[1]);
		return(0);
	}

	if(target->isCt() && !player->isDm()) {
		target->printColor("^r%s tried to dust you!\n", player->name);
		return(0);
	}

	if(player->getClass() == CARETAKER && target->isStaff()) {
		target->printColor("^r%s tried to dust you!\n", player->name);
		return(0);
	}

	if(isdm(target->name)) {
		target->printColor("^r%s tried to dust you!\n", player->name);
		return(0);
	}

	if(player->getClass() == CARETAKER && target->getLevel() > 10) {
		player->print("You are only able to dust people levels 10 and below.\n");
		return(0);
	}

	logn("log.dust", "%s was dusted by %s.\n", target->name, player->name);

	// TODO: Handle guild creations
	sprintf(buf, "\n%c[35mLightning comes down from on high! You have angered the gods!%c[35m\n", 27, 27);
	target->getSock()->write(buf);

	if(!strcmp(cmnd->str[2], "-n")) {
		broadcast(isCt, "^y### %s has been turned to dust!", target->name);
	} else {
		broadcast("### %s has been turned to dust!", target->name);
		broadcast(target->getSock(), target->getRoomParent(), "A bolt of lightning strikes %s from on high.", target->name);
		last_dust_output = time(0) + 15L;
	}

	deletePlayer(target);
	return(0);
}

//*********************************************************************
//						dmFlash
//*********************************************************************
//  This function allows a DM to output a string to an individual
//  players screen.

int dmFlash(Player* player, cmd* cmnd) {
	bstring text = "";
	Player	*target=0;


	if(cmnd->num < 2) {
		player->print("DM flash to whom?\n");
		return(0);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target) {
		player->print("Send to whom?\n");
		return(0);
	}

	text = getFullstrText(cmnd->fullstr, 2);
	if(text == "") {
		player->print("Send what?\n");
		return(0);
	}

	player->printColor("^cYou flashed: \"%s^c\" to %N.\n", text.c_str(), target);

	target->printColor("%s\n", text.c_str());
	return(0);
}

//*********************************************************************
//						dmAward
//*********************************************************************

int dmAward(Player* player, cmd* cmnd) {
	Player	*target;
	long	i=0, t=0, amount=0, gp=0;
	char	temp[80];

	if(!player->flagIsSet(P_CAN_AWARD) && !player->isDm())
		return(cmdNoAuth(player));

	if(cmnd->num < 2) {
		player->print("*award whom?\n");
		return(PROMPT);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target || !player->canSee(target)) {
		player->print("%s is not on.\n", cmnd->str[1]);
		return(0);
	}

	if(target->isStaff()) {
		player->print("Why award a staff member with roleplaying xp?\n");
		return(0);
	}

	sprintf(temp, "%s", target->name);

	if(!target->getSock()->canForce()) {
		player->print("You can't award %s right now.\n", cmnd->str[1]);
		return(0);
	}

	amount = cmnd->val[1];


	if(amount < 2) {
		amount = target->getExperience() / 100;     // 1% of current xp is award.
		amount = MAX(amount, 500);
	}

	gp = MAX(500, MIN(1000000, amount));



	i = LT(target, LT_RP_AWARDED);
	t = time(0);

	if(i > t && !player->isDm()) {
		if(i - t > 3600)
			player->print("%s cannot be awarded for %02d:%02d:%02d more hours.\n", target->name,(i - t) / 3600L, ((i - t) % 3600L) / 60L, (i - t) % 60L);
		else if((i - t > 60) && (i - t < 3600))
			player->print("%s cannot be awarded for %d:%02d more minutes.\n", target->name, (i - t) / 60L, (i - t) % 60L);
		else
			player->pleaseWait(i-t);
		return(0);
	}



	player->print("%d xp awarded to %s for roleplaying.\n", amount, target->name);
	target->printColor("^yYou have been awarded %d xp for good roleplaying!\n", amount);
	target->addExperience(amount);


	target->lasttime[LT_RP_AWARDED].ltime = t;
	target->lasttime[LT_RP_AWARDED].interval = 500L; // 45 minutes.

	if(!strcmp(cmnd->str[2], "-g")) {
		player->print("%ld gold awarded to %s for roleplaying.\n", gp, target->name);
		target->printColor("^yYou have been awarded %ld gold as well!\nIt was put in your bank account!\n", gp);
		target->bank.add(gp, GOLD);
		logn("log.bank", "%s was awarded %ld gold for roleplaying. (Balance=%s)\n",target->name, gp, target->bank.str().c_str());
		Bank::log(target->name, "ROLEPLAY AWARD: %ld [Balance: %s]\n", gp, target->bank.str().c_str());
	}

	log_immort(true, player, "%s awarded %s %d xp for roleplaying.\n", player->name, temp, amount);

	if(!strcmp(cmnd->str[2], "-g"))
		log_immort(true, player, "%s was awarded %ld gold for roleplaying. (Balance=%s)\n",target->name, gp, target->bank.str().c_str());

	return(0);
}


//*********************************************************************
//						dmBeep
//*********************************************************************
int dmBeep(Player* player, cmd* cmnd) {
	Player	*target=0;

	if(cmnd->num < 2) {
		player->print("\n*Beep whom?\n");
		return(PROMPT);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target) {
		player->print("%s is not on.\n", cmnd->str[1]);
		return(0);
	}

	if(!target->getSock()->canForce()) {
		player->print("Can't beep %s right now.\n", cmnd->str[1]);
		return(0);
	}

	target->print("\a\a\a\a\a\a\a\a\a\a");
	return(0);
}


//*********************************************************************
//						dmAdvance
//*********************************************************************

int dmAdvance(Player* player, cmd* cmnd) {
	Player	*target=0;
	int		lev=0, i=0;

	if(cmnd->num < 2) {
		player->print("Advance who at what level?\n");
		return(0);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target) {
		player->print("Advance whom?\n");
		return(0);
	}

	if(target->getNegativeLevels()) {
		player->print("Clear %s's negative levels first.\n", target->name);
		return(0);
	}
	if(cmnd->val[1] <= 0 || cmnd->val[1] > MAXALVL) {
		player->print("Only levels between 1 and %d!\n", MAXALVL);
		return(0);
	}
	lev = cmnd->val[1] - target->getLevel();
	if(lev == 0) {
		player->print("But %N is already level %d!\n", target, target->getLevel());
		return(0);
	}
	if(lev > 0) {
		for(i = 0; i < lev; i++)
			target->upLevel();
		player->print("%M has been raised to level %d.\n", target, target->getLevel());
	}
	if(lev < 0) {
		for(i = 0; i > lev; i--)
			target->downLevel();
		player->print("%M has been lowered to level %d.\n", target, target->getLevel());
	}
	if(target->getLevel() > 1)
		target->setExperience(gConfig->expNeeded(target->getLevel()-1)+1);
	else
		target->setExperience(0);
	return(0);
}

//*********************************************************************
//						dmFinger
//*********************************************************************

int dmFinger(Player* player, cmd* cmnd) {
	struct stat     f_stat;
	Player	*target=0;
	char	tmp[80];

	if(cmnd->num < 2) {
		player->print("Dmfinger who?\n");
		return(0);
	}

	cmnd->str[1][0] = up(cmnd->str[1][0]);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target) {

		if(!loadPlayer(cmnd->str[1], &target)) {
			player->print("Player does not exist.\n");
			return(0);
		}

		player->print("\n");
		target->information(player, false);
		free_crt(target, false);

	} else {
		player->print("\n");
		target->information(player, true);
	}

	sprintf(tmp, "%s/%s", Path::Post, cmnd->str[1]);
	if(stat(tmp, &f_stat)) {
		player->print("No mail.\n");
		return(PROMPT);
	}

	if(f_stat.st_atime > f_stat.st_ctime)
		player->print("No unread mail since: %s", ctime(&f_stat.st_atime));
	else
		player->print("New mail since: %s", ctime(&f_stat.st_ctime));
	return(PROMPT);
}

//*********************************************************************
//						dmDisconnect
//*********************************************************************

int dmDisconnect(Player* player, cmd* cmnd) {
	Player	*creature=0;

	if(cmnd->num < 2) {
		player->print("\nDisconnect whom?\n");
		return(PROMPT);
	}

	lowercize(cmnd->str[1], 1);
	creature = gServer->findPlayer(cmnd->str[1]);

	if(!creature || !player->canSee(creature)) {
		player->print("%s is not on.\n", cmnd->str[1]);
		return(0);
	}
	if(creature->isCt() && !player->isDm()) {
		creature->printColor("^r%s tried to disconnect you!\n", player->name);
		return(0);
	}

	log_immort(true, player, "%s disconnected %s.\n", player->name, creature->name);

	creature->getSock()->disconnect();
	return(0);

}

//*********************************************************************
//						dmTake
//*********************************************************************
// This function allows staff to steal items from a player remotely.

int dmTake(Player* player, cmd* cmnd) {
	Player	*target=0;
	Object	*object=0, *container=0;
	bool	online=false;

	if(cmnd->num < 2) {
		player->print("*take what?\n");
		return(0);
	}

	if(cmnd->num < 3) {
		player->print("*take what from whom?\n");
		return(0);
	}


	cmnd->str[2][0]=up(cmnd->str[2][0]);
	target = gServer->findPlayer(cmnd->str[2]);


	if(!target || !player->canSee(target)) {
		if(player->isDm()) {
			if(!loadPlayer(cmnd->str[2], &target)) {
				player->print("Player does not exist.\n");
				return(0);
			}
		} else {
			player->print("That player is not logged on.\n");
			return(0);
		}
	} else
		online = true;



	if(cmnd->num > 3) {

		container = findObject(player, target->first_obj, cmnd, 3);

		if(!container) {
			player->print("%s doesn't have that container.\n", target->upHeShe());
			if(!online)
				free_crt(target);
			return(0);
		}

		if(container->getType() != CONTAINER) {
			player->print("That isn't a container.\n");
			if(!online)
				free_crt(target);
			return(0);
		}

		object = findObject(player, container->first_obj, cmnd);
		if(!object) {
			player->print("That is not in that container.\n");
			if(!online)
				free_crt(target);
			return(0);
		}

		del_obj_obj(object, container);
		Limited::deleteOwner(target, object);
		// don't need to run addUnique on staff
		player->addObj(object);

		player->printColor("You remove a %P from %N's %s.\n", object, target, container->name);

		if(!player->isDm())
			log_immort(true, player, "%s removed %s from %s's %s.\n", player->name, object->name, target->name, container->name);

		target->save(online);
		if(!online)
			free_crt(target);
		return(0);
	}


	object = findObject(player, target->first_obj, cmnd);

	if(!object) {
		player->print("%s doesn't have that.\n", target->upHeShe());

		if(!online)
			free_crt(target);
		return(0);
	}


	player->printColor("You remove %P from %N's inventory.\n", object, target);

	if(!player->isDm())
		log_immort(true, player, "%s removed %s from %s's inventory.\n", player->name, object->name, target->name);

	target->delObj(object, false, true, true, true, true);
	// don't need to run addUnique on staff
	player->addObj(object);

	target->save(online);
	if(!online)
		free_crt(target);
	return(0);
}

//*********************************************************************
//						dmRemove
//*********************************************************************
// This function allows staff to unwield/unwear items from a player remotely.

int dmRemove(Player* player, cmd* cmnd) {
	Player	*target=0;
	Object	*object=0;
	int     i=0, j=0, found=0;

	if(cmnd->num < 2) {
		player->print("*remove what?\n");
		return(0);
	}
	if(cmnd->num < 3) {
		player->print("*remove what from whom?\n");
		return(0);
	}

	cmnd->str[2][0] = up(cmnd->str[2][0]);
	target = gServer->findPlayer(cmnd->str[2]);


	if(!target || !player->canSee(target)) {
		player->print("That player is not logged on.\n");
		return(0);
	}


	for(i=0; i<MAXWEAR; i++)
		if(target->ready[i])
			found = 1;

	if(!found) {
		player->print("%s isn't wearing anything.\n", target->upHeShe());
		return(0);
	}



	for(i=0,j=0; i<MAXWEAR; i++) {
		if(target->ready[i] && keyTxtEqual(target->ready[i], cmnd->str[1])) {
			j++;
			if(j == cmnd->val[1]) {
				object = target->ready[i];
				if(target->inCombat() && object->getType() <=4) {
					player->print("Not while %s is in combat.\n", target->upHeShe());
					return(0);
				}
				// i is wearloc-1, so add 1  -- We might be add it to someone else's
				// inventory, so do nothing afterwards
				target->unequip(i+1, UNEQUIP_NOTHING);
				Limited::deleteOwner(target, object);
				target->computeAttackPower();
				target->computeAC();
				break;
			}
		}
	}



	if(!object) {
		player->print("%s isn't wearing/wielding that.\n", target->upHeShe());
		return(0);
	}


	player->printColor("You remove %P from %N's worn equipment.\n", object, target);

	if(!player->isDm())
		log_immort(true, player, "%s removed %s from %s's worn equipment.\n", player->name, object->name, target->name);

	player->addObj(object);

	target->save(true);
	return(0);
}

//*********************************************************************
//						dmPut
//*********************************************************************
// This function allows a CT or DM to remotely add items to a player.

int dmPut(Player* player, cmd* cmnd) {
	Player	*target=0;
	Object	*object=0, *container=0;

	bool	online=false;

	if(cmnd->num < 2) {
		player->print("*put what?\n");
		return(0);
	}
	if(cmnd->num < 3) {
		player->print("*put what on whom?\n");
		return(0);
	}

	cmnd->str[2][0] = up(cmnd->str[2][0]);
	target = gServer->findPlayer(cmnd->str[2]);


	if(!target || !player->canSee(target)) {
		if(player->isDm()) {
			if(!loadPlayer(cmnd->str[2], &target)) {
				player->print("Player does not exist.\n");
				return(0);
			}
		} else {
			player->print("That player is not logged on.\n");
			return(0);
		}
	} else
		online = true;



	if(cmnd->num > 3) {

		container = findObject(player, target->first_obj, cmnd, 3);

		if(!container) {
			player->print("%s doesn't have that container.\n", target->upHeShe());
			if(!online)
				free_crt(target);
			return(0);
		}

		if(container->getType() != CONTAINER) {
			player->print("That isn't a container.\n");
			if(!online)
				free_crt(target);
			return(0);
		}

		object = player->findObject(player, cmnd, 1);
		if(!object) {
			player->print("You do not have that.\n");
			if(!online)
				free_crt(target);
			return(0);
		}


		if((container->getShotsCur() + 1) > container->getShotsMax()) {
			player->printColor("You will exceed the maximum allowed items for %P(%d).\n", container->name, container->getShotsMax());
			player->print("Aborted.\n");
			if(!online)
				free_crt(target);
			return(0);
		}

		player->delObj(object, false, false, true, true, true);
		container->incShotsCur();
		container->addObj(object);
		Limited::addOwner(target, object);

		player->printColor("You put %P into %N's %s.\n", object, target, container->name);

		target->save(online);
		if(!online)
			free_crt(target);
		return(0);
	}


	object = player->findObject(player, cmnd, 1);
	if(!object) {
		player->print("You don't have that.\n");
		if(!online)
			free_crt(target);
		return(0);
	}

	if(object->flagIsSet(O_DARKMETAL) && target->getRoomParent()->isSunlight()) {
		player->printColor("You cannot give %P to %N now!\nIt would surely be destroyed!\n", object, target);
		return(0);
	}

	player->printColor("You add %P to %N's inventory.\n", object, target);
	player->delObj(object, false, false, true, true, true);
	Limited::addOwner(target, object);
	target->addObj(object);

	target->save(online);
	if(!online)
		free_crt(target);
	return(0);
}


//*********************************************************************
//						dmMove
//*********************************************************************
// this function allows a DM char to put a player into a totally new room
// when logged off. If logged on it will do nothing as teleport handles
// the online function.

int dmMove(Player* player, cmd* cmnd) {
	Player	*creature=0;

	MapMarker mapmarker;
	std::ostringstream log;
	CatRef	cr;

	// normal checks
	if(cmnd->num < 2) {
		player->print("Offline move whom?\n");
		return(0);
	}

	// already logged on?
	cmnd->str[1][0] = up(cmnd->str[1][0]);
	creature = gServer->findPlayer(cmnd->str[1]);
	if(creature) {
		if(player->canSee(creature))
			player->print("%s is already logged on, use *teleport!\n",  cmnd->str[1]);
		else
			player->print("You cannot move that player.\n");
		return(0);
	}

	// now load 'em in
	if(!loadPlayer(cmnd->str[1], &creature)) {
		player->print("player %s does not exist\n", cmnd->str[1]);
		return(0);
	}
	if(creature->isCt() && !player->isDm()) {
		player->print("You cannot move that player.\n");
		free_crt(creature);
		return(0);
	}

	// put them in their new location
	log << player->name << " moved player " << creature->name << " from room ";
	// TODO: area_room isn't valid
	if(creature->currentLocation.mapmarker.getArea() != 0)
		log << creature->currentLocation.mapmarker.str(false);
	else
		log << creature->currentLocation.room.str();

	log << " to room ";

	getDestination(getFullstrText(cmnd->fullstr, 2), &mapmarker, &cr, player);

	if(!cr.id) {
		Area *area = gConfig->getArea(mapmarker.getArea());
		if(!area) {
			player->print("That area does not exist.\n");
			free_crt(creature);
			return(0);
		}
		creature->currentLocation.room.clear();
		creature->currentLocation.mapmarker = mapmarker;

		log << creature->currentLocation.mapmarker.str(false);
		player->print("Player %s moved to location %s.\n", creature->name, creature->currentLocation.mapmarker.str(false).c_str());
	} else {
		if(!validRoomId(cr)) {
			player->print("Can only put players in the range of 1-%d.\n", RMAX);
			free_crt(creature);
			return(0);
		}
		*&creature->currentLocation.room = *&cr;
		log << cr.str();
		player->print("Player %s moved to location %s.\n", creature->name, cr.str().c_str());
	}

	log_immort(true, player, "%s.\n", log.str().c_str());

	creature->save();
	free_crt(creature);
	return(0);
}


//*********************************************************************
//						dmWordAll
//*********************************************************************
// This function is to be used to quickly move ALL players online to
// one place for whatever reason.

int dmWordAll(Player* player, cmd* cmnd) {
	BaseRoom	*room=0;

	Player* target=0;

	for(std::pair<bstring, Player*> p : gServer->players) {
		target = p.second;

		if(!target->isConnected())
			continue;
		if(target->isStaff())
			continue;

		room = target->getRecallRoom().loadRoom(target);

		target->print("An astral vortex just arrived.\n");
		broadcast(target->getSock(), target->getRoomParent(), "An astral vortex just arrived.");

		target->print("The astral vortex sucks you in!\n");
		broadcast(target->getSock(), target->getRoomParent(), "The astral vortex sucks %N into it!", target);

		broadcast(target->getSock(), target->getRoomParent(), "The astral vortex disappears.");

		broadcast("^Y### Strangely, %N was sucked into an astral vortex.", target);

		// Optionally, can knock everyone out for one minute.
		if(!strcmp(cmnd->str[1], "-u"))
			target->knockUnconscious(60);


		if(!target->inJail()) {
			target->deleteFromRoom();
			target->addToRoom(room);
		}

		target->save(true);
	}
	return(0);
}

//*********************************************************************
//						dmRename
//*********************************************************************

int dmRename(Player* player, cmd* cmnd) {
	Player	*target=0;
	int		i=0;
	FILE	*fp;
	char	old_name[25], new_name[25];
	char	file[80];

	if(!player->isStaff() && !player->isWatcher())
		return(cmdNoExist(player, cmnd));
	if(!player->isWatcher())
		return(cmdNoAuth(player));

	if(cmnd->num < 2) {
		player->print("Rename who?\n");
		return(0);
	}
	if(cmnd->num < 3) {
		player->print("Rename to what?\n");
		return(0);
	}

	cmnd->str[1][0] = up(cmnd->str[1][0]);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target || !player->canSee(target)) {
		player->print("%s is not logged on!\n", cmnd->str[1]);
		return(0);
	}

	if(target->isCt() && !player->isDm()) {
		player->print("You cannot rename that player.\n");
		return(0);
	}

	if(target->getLevel() > 9 && player->isWatcher() && !player->isCt()) {
		player->print("You may only rename players under 10th level.\n");
		return(0);
	}

	strcpy(old_name, target->name);
	strcpy(new_name, cmnd->str[2]);
	new_name[0] = up(new_name[0]);

	if(!strcmp(new_name, old_name)) {
		player->print("Names are the same: hoose a different name.\n");
		return(0);
	}
	if(!strcmp(player->name, old_name)) {
		player->print("You can't rename yourself.\n");
		return(0);
	}
	if(!parse_name(new_name)) {
		player->print("The new name is not allowed, pick another.\n");
		return(0);
	}
	if(strlen(new_name) >= 20) {
		player->print("The name must be less than 20 characters.\n");
		return(0);
	}
	if(strlen(new_name) < 3) {
		player->print("The name must be more than 3 characters.\n");
		return(0);
	}
	for(i=0; i< (int)strlen(new_name); i++)
		if(!isalpha(new_name[i]) && (new_name[i] != '\'')) {
			player->print("Name must be alphabetic, pick another.\n");
			return(0);
		}

	// See if a player with the new name exists
	sprintf(file, "%s/%s.xml", Path::Player, new_name);
	fp = fopen(file, "r");
	if(fp) {
		player->print("A player with that name already exists.\n");
		fclose(fp);
		return(0);
	}

	strcpy(target->name, new_name);

	if(target->getGuild()) {
		Guild* guild = gConfig->getGuild(target->getGuild());
		guild->renameMember(old_name, new_name);
	}
	gConfig->guildCreationsRenameSupporter(old_name, new_name);
	gConfig->saveGuilds();


	gConfig->renamePropertyOwner(old_name, target);
	renamePlayerFiles(old_name, new_name);
	gServer->clearPlayer(old_name);
	gServer->addPlayer(target);

	player->print("%s has been renamed to %s.\n", old_name, target->name);
	target->print("You have been renamed to %s.\n", target->name);
	broadcast(isDm, "^g### %s renamed %s to %s.", player->name, old_name, target->name);
	logn("log.rename", "%s renamed %s to %s.\n", player->name, old_name, target->name);

	// unassociate
	if(target->getForum() != "") {
		//target->printColor("Your forum account ^C%s^x is no longer unassociated with this character.\n", target->getForum().c_str());
		//target->print("Use the \"forum\" command to reassociate this character with your forum account.\n");
		//target->setForum("");
		callWebserver((bstring)"mud.php?type=forum&char=" + old_name + "&rename=" + new_name);
	}

	target->save(true);
	return(0);
}


//*********************************************************************
//						dmPassword
//*********************************************************************

int dmPassword(Player* player, cmd* cmnd) {
	Player	*target=0;

	bool	online=false;
	bstring pass = "";

	if(cmnd->num < 2) {
		player->print("Change who's password?\n");
		return(0);
	}

	pass = getFullstrText(cmnd->fullstr, 2);
	if(pass == "") {
		player->print("Change the password to what?\n");
		return(0);
	}

	cmnd->str[1][0] = up(cmnd->str[1][0]);
	target = gServer->findPlayer(cmnd->str[1]);
	if(!target) {
		if(!loadPlayer(cmnd->str[1], &target)) {
			player->print("Player does not exist.\n");
			return(0);
		}
	} else
		online = true;


	if(target->isPassword(pass)) {
		player->print("Passwords must be different.\n");
		return(0);
	}
	if(pass.getLength() > PASSWORD_MAX_LENGTH) {
		player->print("The password must be %d characters or less.\n", PASSWORD_MAX_LENGTH);
		return(0);
	}
	if(pass.getLength() < PASSWORD_MIN_LENGTH) {
		player->print("Password must be at least %d characters.\n", PASSWORD_MAX_LENGTH);
		return(0);
	}

	target->setPassword(pass);

	player->print("%s's password has been changed to %s.\n", target->name, pass.c_str());
	logn("log.passwd", "### %s changed %s's password to %s.\n", player->name, target->name, pass.c_str());

	target->save(online);
	if(!online)
		free_crt(target);
	return(0);
}


//*********************************************************************
//						dmRestorePlayer
//*********************************************************************

int dmRestorePlayer(Player* player, cmd* cmnd) {
	Player	*target=0;
	int		n=0;
	long	old_xp=0;
	float	exp=0;
	bool	online=false;

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target) {
		if(!loadPlayer(cmnd->str[1], &target)) {
			player->print("Player does not exist.\n");
			return(0);
		}
	} else
		online = true;

	if(target->getNegativeLevels()) {
		player->print("Clear %s's negative levels first.\n", target->name);
		return(0);
	}

	old_xp = target->getExperience();

	exp = (float)target->getExperience();
	exp /= 0.85;
	target->setExperience((long)exp);

	n = exp_to_lev(target->getExperience());
	while(target->getLevel() < n)
		target->upLevel();


	player->print("%M has been restored.\n", target);

	log_immort(true, player, "%s restored %s from %ld to %ld xp.\n", player->name,
		target->name, old_xp, target->getExperience());

	target->save(online);
	if(!online)
		free_crt(target);
	else
		target->print("Your experience has been restored.\n");
	return(0);
}

//*********************************************************************
//				dmGeneric
//*********************************************************************
// This is a generic function that handles getting information or doing
// something minor to a player who may be online or offline.
// Authorization is not done here.

#define DM_GEN_BANK		1
#define DM_GEN_INVVAL	2
#define DM_GEN_PROXY	3
#define DM_GEN_WARN		4
#define DM_GEN_BUG		5

int dmGeneric(Player* player, cmd* cmnd, bstring action, int what) {
	Player	*target=0;

	bool	online=false;

	if(cmnd->num < 2) {
		player->print("%s whom?\n", action.c_str());
		return(0);
	}

	cmnd->str[1][0] = up(cmnd->str[1][0]);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target || !player->canSee(target)) {
		if(player->isCt()) {
			if(!loadPlayer(cmnd->str[1], &target)) {
				player->print("Player does not exist.\n");
				return(DOPROMPT);
			}
		} else {
			player->print("That player is not logged on.\n");
			return(DOPROMPT);
		}
	} else
		online = true;


	// time to actually do what they want us to do
	if(what == DM_GEN_BANK)
		player->print("%s's bank balance is: %ldgp.\n", target->name, target->bank[GOLD]);
	else if(what == DM_GEN_INVVAL)
		player->print("%s's total inventory assets: %ldgp.\n", target->name, target->getInventoryValue());
	else if(what == DM_GEN_PROXY) {

		if(!target->flagIsSet(P_ON_PROXY)) {
			player->print("%s is now allowed to multi-log for proxy purposes.\n", target->name);
			target->setFlag(P_ON_PROXY);
		} else {
			player->print("%s is no longer allowed to multi-log.\n", target->name);
			target->clearFlag(P_ON_PROXY);
		}
		target->save(online);

	} else if(what == DM_GEN_WARN) {

		if(!player->isDm() && target->isDm()) {
			player->print("Don't be silly.\n");
			if(!online == 1)
				free_crt(target);
			return(0);
		}

		if(cmnd->num < 3) {
			player->print("%M has %d total warning%s.\n", target, target->getWarnings(), (target->getWarnings()==1 ? "":"s") );
			if(!online)
				free_crt(target);
			return(0);
		}

		if(!strcmp(cmnd->str[2], "-r")) {
			if(!player->isCt()) {
				player->print("You are unable to remove warnings.\n");
				if(!online)
					free_crt(target);
				return(0);
			}
			if(target->getWarnings() < 1) {
				player->print("%M has no warnings.\n", target);
				if(!online)
					free_crt(target);
				return(0);
			} else {
				target->subWarnings(1);
				if(online)
					target->print("%M removed one of your warnings. You now have %d.\n", player, target->getWarnings());
				player->print("One warning removed. %M now has %d warning%s remaining.\n", target, target->getWarnings(), (target->getWarnings()==1 ? "":"s"));
			}
			log_immort(false,player, "%s removed a warning from %s.\n", player->name, target->name);
			logn("log.warn", "%s removed a warning from %s.\n", player->name, target->name);

			broadcast(isCt, "^y%s removed a warning from %s. New total: %d", player->name, target->name, target->getWarnings());
		} else if(!strcmp(cmnd->str[2], "-a")) {
			if(target->getWarnings() > 2) {
				player->print("%M already has 3 warnings. %s cannot be warned further.\n",
				      target, target->upHeShe());
				if(!online)
					free_crt(target);
				return(0);
			} else {
				target->addWarnings(1);
				if(online)
					target->print("%M has formally warned you. You now have %d warning%s.\n",
					      player, target->getWarnings(), (target->getWarnings()>1 ? "s":""));
				player->print("Warning added. %M now has %d total warning%s.\n", target, target->getWarnings(), (target->getWarnings()>1 ? "s":""));

				logn("log.warn", "%s added a warning to %s.\n", player->name, target->name);
				log_immort(false,player, "%s added a warning to %s.\n", player->name, target->name);

				broadcast(isCt, "^y%s added a warning to %s. New total: %d", player->name, target->name, target->getWarnings());
			}

		} else {
			player->print("Syntax: *warn (player) [-r|-a]\n");
			if(!online)
				free_crt(target);
			return(0);
		}
		target->save(online);

	} else if(what == DM_GEN_BUG) {

		if(!target->flagIsSet(P_BUGGED)) {
			player->print("%s is now bugged for surveillance.\n", target->name);
			target->setFlag(P_BUGGED);
		} else {
			player->print("%s is no longer bugged for surveillance.\n", target->name);
			target->clearFlag(P_BUGGED);
		}
		target->save(online);

	} else
		player->print("Invalid option for dmGetInfo().\n");


	if(!online)
		free_crt(target);
	return(0);
}

//*********************************************************************
//						dmBank
//*********************************************************************
int dmBank(Player* player, cmd* cmnd) {
	return(dmGeneric(player, cmnd, "*bank", DM_GEN_BANK));
}
//*********************************************************************
//						dmInventoryValue
//*********************************************************************
int dmInventoryValue(Player* player, cmd* cmnd) {
	return(dmGeneric(player, cmnd, "*inv", DM_GEN_INVVAL));
}
//*********************************************************************
//						dmProxy
//*********************************************************************
int dmProxy(Player* player, cmd* cmnd) {
	return(dmGeneric(player, cmnd, "*proxy", DM_GEN_PROXY));
}
//*********************************************************************
//						dmWarn
//*********************************************************************
int dmWarn(Player* player, cmd* cmnd) {

	// watchers can use *warn
	if(!player->isStaff() && !player->isWatcher())
		return(cmdNoExist(player, cmnd));
	if(!player->isWatcher())
		return(cmdNoAuth(player));

	return(dmGeneric(player, cmnd, "*warn", DM_GEN_WARN));
}
//*********************************************************************
//						dmBugPlayer
//*********************************************************************
int dmBugPlayer(Player* player, cmd* cmnd) {
	return(dmGeneric(player, cmnd, "*bug", DM_GEN_BUG));
}



// dmKill function decides which of these are player-kill,
// room-kill or all-kill
#define DM_KILL			0
#define DM_COMBUST		1
#define DM_SLIME		2
#define DM_GNATS		3
#define DM_TRIP			4
#define DM_BOMB			5
#define DM_NUCLEAR		6
#define DM_ARMAGEDDON	7
#define DM_IGMOO		8
#define DM_RAPE		    9
#define DM_DRAIN		10
#define DM_CRUSH		11
#define DM_MISSILE		12
#define DM_SUPERNOVA	13
#define DM_NOMNOM		14

//*********************************************************************
//						dmKillAll
//*********************************************************************
// not declared in proto.h, only used below
// this function handles and *kill commands that kill everyone

int dmKillAll(int type, int silent, int unconscious, int uncon_length) {
	BaseRoom *room=0;

	Player* target;
	for(std::pair<bstring, Player*> p : gServer->players) {
		target = p.second;

		if(!target->isConnected())
			continue;
		if(target->isStaff())
			continue;

		room = target->getLimboRoom().loadRoom(target);

		if(!room)
			continue;

		switch(type) {
		case DM_ARMAGEDDON:
			target->print("Meteors begin falling from the sky!\n");
			broadcast(target->getSock(), target->getRoomParent(), "Meteors begin falling from the sky!\n");

			target->print("A freak meteor strikes you!\n");
			broadcast(target->getSock(), target->getRoomParent(), "A freak meteors strikes %N!", target);

			target->print("The freak meteor slams into you for %d damage!\n", mrand(5000,10000));
			broadcast(target->getSock(), target->getRoomParent(), "The freak meteor blasts right through %N!", target);
			target->print("You die as your ashes fall to the ground.\n");
			broadcast(target->getSock(), target->getRoomParent(), "The freak meteor vaporized %N!", target);

			if(!silent)
				broadcast("### Sadly, %N was killed by a freak meteor shower.", target);

			break;
		case DM_IGMOO:
			target->print("Igmoo the Mad just arrived.\n");

			target->printColor("^GIgmoo the Mad casts a disintegration spell on you.\n");
			target->print("You are engulfed in an eerie green light.\n");
			target->printColor("You've been disintegrated!\n");

			target->print("Igmoo the Mad killed you.\n");

			if(!silent)
				broadcast("### Sadly, %N was disintegrated by Igmoo the Mad.", target);
			break;
		case DM_SUPERNOVA:
			target->printColor("\n^YThe searing rays of the sun grow uncomfortably hot.\n\n");

			target->printColor("^YA blinding flash of burning light engulfs the sky!\n");
			target->printColor("^YYour eyes are burned from their sockets and your skin begins to boil!\n");
			target->printColor("^YYou've been disintegrated!\n\n");

			if(!silent)
				broadcast("^Y### Sadly, %N was disintegrated by an exploding sun.", target);

		default:
			break;
		}


		target->hp.setCur(1);
		target->mp.setCur(1);

		if(unconscious)
			target->knockUnconscious(uncon_length);

		if(!target->inJail()) {
			target->deleteFromRoom();
			target->addToRoom(room);
			target->doPetFollow();
		}

	}
	return(0);
}

//*********************************************************************
//						dmKill
//*********************************************************************
// not declared in proto.h, only used below
// handles player-targeted *kills

int dmKill(Player* player, Player *victim, int type, int silent, int unconscious, int uncon_length) {
	int		kill_room=0, no_limbo=0;
	BaseRoom *newRoom=0;

	char filename[80];
	snprintf(filename, 80, "%s/crash.txt", Path::Config);

	// room kill or no limbo?
	switch(type) {
		case DM_BOMB:
		case DM_NUCLEAR:
			kill_room = 1;
			break;
		case DM_RAPE:
		case DM_DRAIN:
			no_limbo = 1;
			break;
		default:
			break;
	}

	if(victim->isDm()) {
		switch(type) {
		case DM_COMBUST:
			player->print("You tried to make %N spontaneously combust!\n", victim);
			victim->printColor("^r%M tried to make you spontaneously combust!\n", player);
			break;
		case DM_SLIME:
			player->print("You tried to make %N dissolve in green slime!\n", victim);
			victim->printColor("^g%M tried to dissolve you in green slime!\n", player);
			break;
		case DM_GNATS:
			player->print("You tried to make a swarm of gnats eat %N!\n", victim);
			victim->printColor("^m%M tried to send a swarm of ravenous gnats after you!\n", player);
			break;
		case DM_TRIP:
			player->print("You tried to make %N trip and break %s neck!\n", victim, victim->hisHer());
			victim->printColor("^y%M tried to make you trip and break your neck!\n", player);
			break;
		case DM_BOMB:
			player->print("You tried to suicide bomb %N!\n", victim);
			victim->printColor("^r%M tried to suicide bomb you!\n", player);
			break;
		case DM_NUCLEAR:
			player->print("You tried to make %N meltdown!\n", victim);
			victim->printColor("^g%M tried to make you meltdown!\n", player);
			break;
		case DM_RAPE:
			player->print("You tried to rape %N!\n", victim);
			victim->printColor("^m%M tried to rape you!\n", player);
			break;
		case DM_DRAIN:
			player->print("You tried to drain %N!\n", victim);
			victim->printColor("^c%M tried to drain you!\n", player);
			break;
		case DM_CRUSH:
			player->print("You tried to crush %N!\n", victim);
			victim->printColor("^c%M tried to crush you!\n", player);
			break;
		case DM_MISSILE:
			player->print("You tried to hit %N with a cruise missile!\n", victim);
			victim->printColor("^r%M tried to hit you with a cruise missile!\n", player);
			break;
		case DM_NOMNOM:
			player->print("You tried to make a fearsome monster eat %N!\n", victim);
			victim->printColor("^r%M tried to make a fearsome monster!\n", player);
			break;
		default:
			player->print("You tried to strike down %N!\n", victim);
			victim->printColor("^r%M tried to strike you down with lightning!\n", player);
			break;
		}
		return(0);
	}

	switch(type) {
	case DM_COMBUST:
		victim->printColor("^rYou just spontaneously combusted!\n");
		break;
	case DM_SLIME:
		victim->printColor("^gYou have been dissolved by green slime!\n");
		break;
	case DM_GNATS:
		victim->printColor("^mYou have been eaten by a ravenous demonic gnat swarm!\n");
		break;
	case DM_TRIP:
		victim->printColor("^yYou slip on an oversized banana and break your neck.\nYou're dead!\n");
		break;
	case DM_BOMB:
		victim->printColor("^rYou blow yourself to smithereens!\n");
		break;
	case DM_NUCLEAR:
		victim->printColor("^gYou begin to meltdown!\n");
		char filename[80];
		snprintf(filename, 80, "%s/crash.txt", Path::Config);
		viewLoginFile(victim->getSock(), filename);
		break;
	case DM_RAPE:
		victim->printColor("^mYou have been raped by the gods!\n");
		break;
	case DM_DRAIN:
		victim->printColor("^cYour life force drains away!\n");
		break;
	case DM_CRUSH:
		victim->printColor("^cA giant stone slab falls from the sky!\n");
		break;
	case DM_MISSILE:
		victim->printColor("^rYou are struck by a cruise missile!\nYou explode!\n");
		break;
	case DM_KILL:
		victim->printColor("^rA giant lightning bolt strikes you!\n");
		break;
	case DM_NOMNOM:
		victim->print("A fearsome monster leaps out of the shadows!\n");
		victim->print("It tears off your limbs and greedily devours them!\n");
		break;
	default:
		break;
	}

	if(victim->getClass() < BUILDER && !silent) {
		switch(type) {
		case DM_COMBUST:
			broadcast(victim->getSock(), victim->getRoomParent(), "^r%s bursts into flames!", victim->name);
			broadcast("^r### Sadly, %s spontaneously combusted.", victim->name);
			break;
		case DM_SLIME:
			broadcast(victim->getSock(), victim->getRoomParent(),
				"^gA massive green slime just arrived.\nThe massive green slime attacks %s!", victim->name);
			broadcast("^g### Sadly, %s was dissolved by a massive green slime.", victim->name);
			break;
		case DM_GNATS:
			broadcast(victim->getSock(), victim->getRoomParent(),
				"^mA demonic gnat swarm just arrived.\nThe demonic gnat swarm attacks %s!", victim->name);
			broadcast("^m### Sadly, %s was eaten by a ravenous demonic gnat swarm!", victim->name);
			break;
		case DM_TRIP:
			broadcast(victim->getSock(), victim->getRoomParent(), "^m%s slips on an oversized banana peel!", victim->name);
			broadcast("^m### Sadly, %s tripped and broke %s neck.", victim->name, victim->hisHer());
			break;
		case DM_BOMB:
			broadcast(victim->getSock(), victim->getRoomParent(), "^r%s begins to tick!", victim->name);
			broadcast("^r### Sadly, %s blew %sself up!", victim->name, victim->himHer());
			break;
		case DM_NUCLEAR:
			broadcast(victim->getSock(), victim->getRoomParent(), "^g%s begins to meltdown!", victim->name);
			broadcast("^g### Sadly, %s was killed in a nuclear explosion.", victim->name);
			break;
		case DM_KILL:
			broadcast(victim->getSock(), victim->getRoomParent(), "^rA giant lightning bolt strikes %s!", victim->name);
			broadcast("^r### Sadly, %s has been incinerated by lightning from the heavens!", victim->name);
			break;
		case DM_RAPE:
			broadcast(victim->getSock(), victim->getRoomParent(), "^m%s was raped by the gods!", victim->name);
			broadcast(isStaff, "^m*** %s raped %s.", player->name, victim->name);
			break;
		case DM_DRAIN:
			broadcast(victim->getSock(), victim->getRoomParent(), "^c%s's life force drains away!", victim->name);
			broadcast(isStaff, "^g*** %s drained %s.", player->name, victim->name);
			break;
		case DM_CRUSH:
			broadcast(victim->getSock(), victim->getRoomParent(), "^cA giant stone slab falls on %s!", victim->name);
			broadcast("^c### Sadly, %s was crushed under a giant stone slab.", victim->name);
			break;
		case DM_MISSILE:
			broadcast(victim->getSock(), victim->getRoomParent(),
				"^r%s was struck by a cruise missile!\n%s exploded!", victim->name, victim->name);
			broadcast("^r### Sadly, %s was killed by a precision-strike cruise missile.", victim->name);
			break;
		case DM_NOMNOM:
			broadcast(victim->getSock(), victim->getRoomParent(),
				"A fearsome monster leaps out of the shadows!\nIt tears off %s's limbs and greedily devours them!", victim->name);
			broadcast("### Sadly, %s was devoured by a fearsome monster.", victim->name);
			break;
		default:
			break;
		}
	} else {
		switch(type) {
		case DM_COMBUST:
			broadcast(isStaff, "^G### Sadly, %s spontaneously combusted.", victim->name);
			break;
		case DM_SLIME:
			broadcast(isStaff, "^G### Sadly, %s was dissolved by a massive green slime.", victim->name);
			break;
		case DM_GNATS:
			broadcast(isStaff, "^G### Sadly, %s was eaten by a ravenous demonic gnat swarm!", victim->name);
			break;
		case DM_TRIP:
			broadcast(isStaff, "^G### Sadly, %s tripped and broke %s neck.", victim->name, victim->hisHer());
			break;
		case DM_BOMB:
			broadcast(isStaff, "^G### Sadly, %s blew %sself up!!", victim->name, victim->himHer());
			break;
		case DM_NUCLEAR:
			broadcast(isStaff, "^G### Sadly, %s was killed in a nuclear explosion.", victim->name);
			break;
		case DM_KILL:
			broadcast(isStaff, "^G### Sadly, %s has been incinerated by lightning from the heavens!", victim->name);
			break;
		case DM_RAPE:
			broadcast(isStaff, "^g*** %s raped %s.", player->name, victim->name);
			break;
		case DM_DRAIN:
			broadcast(isStaff, "^g*** %s drained %s.", player->name, victim->name);
			break;
		case DM_CRUSH:
			broadcast(isStaff, "^G### Sadly, %s was crushed under a giant stone slab.", victim->name);
			break;
		case DM_MISSILE:
			broadcast(isStaff, "^G### Sadly, %s was killed by a precision-strike cruise missile.", victim->name);
			break;
		case DM_NOMNOM:
			broadcast(isStaff, "^G### Sadly, %s was devoured by a fearsome monster.", victim->name);
			break;
		default:
			break;
		}
	}

	PlayerSet::iterator pIt = victim->getRoomParent()->players.begin();
	PlayerSet::iterator pEnd = victim->getRoomParent()->players.end();
	while(pIt != pEnd) {
		player = (*pIt++);
		if(!player || (player->isStaff() && player!=victim))
			continue;


		if(player==victim || kill_room) {

			if(kill_room && player!=victim && !silent) {
				switch(type) {
				case DM_NUCLEAR:
					viewLoginFile(player->getSock(), filename);
					broadcast("^g### Sadly, %s was killed by %s's radiation.", player->name, victim->name);
					break;
				default:
					broadcast("^r### Sadly, %s died in %s's explosion!", player->name, victim->name);
					break;
				}
			}

			player->hp.setCur(1);
			player->mp.setCur(1);

			if(unconscious)
				player->knockUnconscious(uncon_length);

			// do we send them to limbo?
			if(!no_limbo && !player->isStaff()) {
				newRoom = player->getLimboRoom().loadRoom(player);
				if(newRoom) {
					player->deleteFromRoom();
					player->addToRoom(newRoom);
					player->doPetFollow();
				}
			}
		}
	}
	return(0);
}

//*********************************************************************
//						dmKillSwitch
//*********************************************************************
// all *kill commands are routed through this function; dmKillAll
// and dmKill are passed the appropriate arguments

int dmKillSwitch(Player* player, cmd* cmnd) {
	int		i, silent=0, unconscious=0, uncon_length=0, type=DM_KILL;
	Player	*victim;

	// -s and -u can be in any position, so we have to look for them
	for(i=1; i<cmnd->num; i++) {
		if(!strcmp(cmnd->str[i], "-s"))
			silent=1;
		if(!strcmp(cmnd->str[i], "-u")) {
			unconscious=1;
			if(i < cmnd->num)
				uncon_length = atoi(cmnd->str[i+1]);

			uncon_length = MIN(600, MAX(15, uncon_length ) );
		}
	}


	// check for world-killers!
	if(!strcasecmp(cmnd->str[0], "*arma") || !strcasecmp(cmnd->str[0], "*armageddon"))
		type = DM_ARMAGEDDON;
	else if(!strcasecmp(cmnd->str[0], "*supernova"))
		type = DM_SUPERNOVA;
	else if(!strcasecmp(cmnd->str[0], "*igmoo"))
		type = DM_IGMOO;
	else if(cmnd->num < 2) {
		player->print("Kill whom?\n");
		return(0);
	} else {
		if(!strcasecmp(cmnd->str[1], "-arma") || !strcasecmp(cmnd->str[1], "-armageddon"))
			type = DM_ARMAGEDDON;
		else if(!strcasecmp(cmnd->str[1], "-supernova"))
			type = DM_SUPERNOVA;
		else if(!strcasecmp(cmnd->str[1], "-igmoo"))
			type = DM_IGMOO;
	}


	// if it's a world-killer
	if(type != DM_KILL)
		return(dmKillAll(type, silent, unconscious, uncon_length));


	// if it won't kill everyone, who will it kill?
	cmnd->str[1][0]=up(cmnd->str[1][0]);
	victim = gServer->findPlayer(cmnd->str[1]);
	if(!victim) {
		player->print("That person is not logged on.\n");
		return(0);
	}


	if(!strcasecmp(cmnd->str[0], "*kill")) {
		// default
	} else if(!strcasecmp(cmnd->str[0], "*combust"))
		type = DM_COMBUST;
	else if(!strcasecmp(cmnd->str[0], "*slime"))
		type = DM_SLIME;
	else if(!strcasecmp(cmnd->str[0], "*gnat"))
		type = DM_GNATS;
	else if(!strcasecmp(cmnd->str[0], "*trip"))
		type = DM_TRIP;
	else if(!strcasecmp(cmnd->str[0], "*bomb"))
		type = DM_BOMB;
	else if(!strcasecmp(cmnd->str[0], "*nuke"))
		type = DM_NUCLEAR;
	else if(!strcasecmp(cmnd->str[0], "*rape"))
		type = DM_RAPE;
	else if(!strcasecmp(cmnd->str[0], "*drain"))
		type = DM_DRAIN;
	else if(!strcasecmp(cmnd->str[0], "*crush"))
		type = DM_CRUSH;
	else if(!strcasecmp(cmnd->str[0], "*missile"))
		type = DM_MISSILE;
	else if(!strcasecmp(cmnd->str[0], "*nomnom"))
		type = DM_NOMNOM;

	if(cmnd->num >  2) {
		if(!strcasecmp(cmnd->str[2], "-combust"))
			type = DM_COMBUST;
		else if(!strcasecmp(cmnd->str[2], "-slime"))
			type = DM_SLIME;
		else if(!strcasecmp(cmnd->str[2], "-gnats"))
			type = DM_GNATS;
		else if(!strcasecmp(cmnd->str[2], "-trip"))
			type = DM_TRIP;
		else if(!strcasecmp(cmnd->str[2], "-bomb"))
			type = DM_BOMB;
		else if(!strcasecmp(cmnd->str[2], "-nuclear") || !strcmp(cmnd->str[2], "-nuke"))
			type = DM_NUCLEAR;
		else if(!strcasecmp(cmnd->str[2], "-rape"))
			type = DM_RAPE;
		else if(!strcasecmp(cmnd->str[2], "-drain"))
			type = DM_DRAIN;
		else if(!strcasecmp(cmnd->str[2], "-crush"))
			type = DM_CRUSH;
		else if(!strcasecmp(cmnd->str[2], "-missile"))
			type = DM_MISSILE;
		else if(!strcasecmp(cmnd->str[2], "-nomnom"))
			type = DM_NOMNOM;
	}

	dmKill(player, victim, type, silent, unconscious, uncon_length);
	return(0);
}

//*********************************************************************
//						dmRepair
//*********************************************************************

int dmRepair(Player* player, cmd* cmnd) {
	Player	*creature=0;
	int		a=0, count=0, check=0;

	if(cmnd->num < 2) {
		player->print("Repair who's armor?\n");
		return(0);
	}

	if(!strcmp(cmnd->str[2], "-c"))
		check=1;

	cmnd->str[1][0] = up(cmnd->str[1][0]);
	creature = gServer->findPlayer(cmnd->str[1]);

	if(!creature) {
		player->print("Player not found online.\n");
		return(0);
	}

	if(check)
		player->print("%s's current armor status:\n", creature->name);

	for(a=0;a<MAXWEAR;a++) {
		if(!creature->ready[a])
			continue;

		if(check) {
			player->print("%s: [%d/%d](%2f)\n", creature->ready[a]->name,
			      creature->ready[a]->getShotsCur(), creature->ready[a]->getShotsMax(),
			      ((float)creature->ready[a]->getShotsCur()/(float)creature->ready[a]->getShotsMax()));
			continue;
		}
		if(creature->ready[a]->getShotsCur() < creature->ready[a]->getShotsMax()) {
			creature->ready[a]->setShotsCur(creature->ready[a]->getShotsMax());

			player->print("%s's %s repaired to full shots (%d/%d).\n", creature->name, creature->ready[a]->name,
			      creature->ready[a]->getShotsCur(), creature->ready[a]->getShotsMax());
			count++;

		}

	}


	if(count) {
		broadcast(isDm, "^g### %s repaired all of %s's armor.", player->name, creature->name);
		log_immort(true, player, "%s repaired all of %s's armor.", player->name, creature->name);
	}

	if(!count)
		player->print("%s is wearing nothing that needs to be repaired.\n", creature->name);

	return(0);
}

//*********************************************************************
//						dmMax
//*********************************************************************

int dmMax(Player* player, cmd* cmnd) {
	int i=0;

	if(cmnd)
		player->print("Maxing your stats out.\n");

	player->strength.setMax(MAX_STAT_NUM);
	player->dexterity.setMax(MAX_STAT_NUM);
	player->intelligence.setMax(MAX_STAT_NUM);
	player->constitution.setMax(MAX_STAT_NUM);
	player->piety.setMax(MAX_STAT_NUM);

	player->strength.setCur(MAX_STAT_NUM);
	player->dexterity.setCur(MAX_STAT_NUM);
	player->intelligence.setCur(MAX_STAT_NUM);
	player->constitution.setCur(MAX_STAT_NUM);
	player->piety.setCur(MAX_STAT_NUM);

	for(Realm r = MIN_REALM; r<MAX_REALM; r = (Realm)((int)r + 1))
		player->setRealm(10000000, r);
	player->coins.set(100000000, GOLD);

	player->hp.setMax(30000);
	player->hp.setCur(30000);
	player->mp.setMax(30000);
	player->mp.setCur(30000);

	if(!strcmp(player->name, "Bane")) {
		player->setLevel(69);
		player->setExperience(2100000000);
	} else {
		player->setLevel(MAXALVL);
		player->setExperience(900000000);
	}

	for(i=0; i<MAXSPELL; i++)
		player->learnSpell(i);
	for(i=0; i<gConfig->getMaxSong(); i++)
		player->learnSong(i);

	if(player->getClass() == BUILDER) {
		player->forgetSpell(S_PLANE_SHIFT);
		player->forgetSpell(S_JUDGEMENT);
		player->forgetSpell(S_ANIMIATE_DEAD);
	}

	player->setAlignment(0);
	return(0);
}


//*********************************************************************
//						dmBackupPlayer
//*********************************************************************

int dmBackupPlayer(Player* player, cmd* cmnd) {

	char    filename[80], restoredFile[80];
	Player	*target=0;
	bool	online=false;

	if(cmnd->num < 2) {
		player->print("Backup who?\n");
		return(0);
	}


	if(cmnd->num > 2 && !strcmp(cmnd->str[2], "-r")) {
		cmnd->str[1][0] = up(cmnd->str[1][0]);
		sprintf(filename, "%s/%s.bak.xml", Path::PlayerBackup, cmnd->str[1]);
		sprintf(restoredFile, "%s/%s.xml", Path::Player, cmnd->str[1]);

		if(file_exists(filename)) {

			target = gServer->findPlayer(cmnd->str[1]);
			if(target) {
				player->print("%s is currently online.\n%s must be offline for auto-backup restore.\n", target->name, target->upHeShe());
				return(0);
			}

			if(file_exists(restoredFile))
				unlink(restoredFile);

			link(filename, restoredFile);
			broadcast(isDm, "^g*** %s restored %s from auto-backup.", player->name, cmnd->str[1]);
			player->print("Restoring %s from last auto-backup.\n", cmnd->str[1]);
			log_immort(false,player, "%s restored %s from auto-backup file.\n", player->name, cmnd->str[1]);
		} else {
			player->print("Backup file for that player does not exist.\n");
		}
		return(0);
	}


	cmnd->str[1][0] = up(cmnd->str[1][0]);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target) {
		if(!loadPlayer(cmnd->str[1], &target)) {
			player->print("Player does not exist.\n");
			return(0);
		}
	} else
		online = true;


	if(cmnd->num > 2 && !strcmp(cmnd->str[2], "-d")) {
		sprintf(filename, "%s/%s.bak.xml", Path::PlayerBackup, target->name);
		if(file_exists(filename)) {
			unlink(filename);
			broadcast(isDm, "^g*** %s deleted %s's backup file.", player->name, target->name);
			player->print("Deleted %s.bak from disk.\n", target->name);

			log_immort(false,player, "%s deleted backup file for player %s.\n", player->name, target->name);
		} else {
			player->print("Backup file for player %s does not exist.\n", target->name);

		}

		if(!online)
			free_crt(target);
		return(0);
	}


	if(target->save(online, LS_BACKUP) > 0) {
		player->print("Backup failed.\n");
		if(!online)
			free_crt(target);
		return(0);
	}

	broadcast(isDm, "^g*** %s backed up %s to disk.", player->name, target->name);
	player->print("%s has been backed up.\n", target->name);

	log_immort(false,player, "%s backed up player %s to disk.\n", player->name, target->name);

	if(!online)
		free_crt(target);
	return(0);
}


//*********************************************************************
//						dmChangeStats
//*********************************************************************

int dmChangeStats(Player* player, cmd* cmnd) {
	Player	*target=0;


	if(cmnd->num < 2) {
		player->print("Allow whom to change their stats?\n");
		return(0);
	}

	cmnd->str[1][0]=up(cmnd->str[1][0]);
	target = gServer->findPlayer(cmnd->str[1]);
	if(!target) {
		player->print("Player not online.\n");
		return(0);
	}

	if(target->flagIsSet(P_DM_INVIS) && !isDm(player)) {
		player->print("Player not online.\n");
		return(0);
	}

	if(!target->flagIsSet(P_CAN_CHANGE_STATS)) {
		target->setFlag(P_CAN_CHANGE_STATS);
		player->print("%s can now choose new stats.\n", target->name);
	} else {
		target->clearFlag(P_CAN_CHANGE_STATS);
		player->print("%s can no longer choose new stats.\n", target->name);
	}

	log_immort(false, player, "%s set %s to choose new stats\n", player->name, target->name);

	return(0);
}


//*********************************************************************
//						dmJailPlayer
//*********************************************************************

int dmJailPlayer(Player* player, cmd* cmnd) {
	int			tm=0, i=0, iTmp=0, len=0, strLen=0;
	long		t=0;
	UniqueRoom	*newRoom=0;
	Player	*target=0;
	char		*reason=0;
	bool		online=false;

	if(!player->isStaff() && !player->isWatcher())
		return(cmdNoExist(player, cmnd));
	if(!player->isWatcher())
		return(cmdNoAuth(player));

	if(cmnd->num < 2) {
		player->print("Jail whom?\n");
		player->print("Syntax: *jail (player) (minutes) [-b|-r]\n");
		return(0);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);
	if(!target) {
		if(!loadPlayer(cmnd->str[1], &target)) {
			player->print("Player does not exist.\n");
			return(0);
		}
	} else
		online = true;

	if(target->isStaff()) {
		player->print("Don't use *jail on staff members.\n");
		if(!online)
			free_crt(target);
		return(0);
	}
	if(player == target) {
		player->print("Do not jail yourself.\n");
		if(!online)
			free_crt(target);
		return(0);
	}

	if(player->isWatcher() && !isCt(player) && target->isWatcher()) {
		player->print("Do not jail other watchers.\n");
		if(!online)
			free_crt(target);
		return(0);
	}

	strLen = cmnd->fullstr.length();

	// This kills all leading whitespace
	while(i<strLen && isspace(cmnd->fullstr[i]))
		i++;

	// This kills the command itself
	while(i<strLen && !isspace(cmnd->fullstr[i]))
		i++;

	// This kills all the white space before the guild
	while(i<strLen && isspace(cmnd->fullstr[i]))
		i++;

	len = strlen(&cmnd->fullstr[i]);

	reason = strstr(&cmnd->fullstr[i], "-r ");
	if(reason) { // There is a reason string then
		iTmp = i;
		while(iTmp < strLen && &cmnd->fullstr[iTmp] != reason)
			iTmp++;
		reason += 3;
		while(isspace(*reason))
			reason++;
		// Kill trailing whitespace
		len = strlen(reason);
		while(isspace(reason[len-1]))
			len--;
		reason[len] = '\0';
	}
	if(iTmp) { // We have a reason, everything else stops at the beginging of the -r
		strLen = iTmp;
		cmnd->fullstr[iTmp] = '\0';
	}
	if(!reason && player->isWatcher()) {
		player->print("You must enter a reason. Use the -r option.\n");
		if(!online)
			free_crt(target);
		return(0);
	}

	if(!reason && !strcmp(player->name, "Bane")) {
		player->print("Use the -r option and enter a reason, assbandit!\n");
		if(!online)
			free_crt(target);
		return(0);
	}

	tm = MAX(1, cmnd->val[1]);

	if(player->isWatcher())
		tm = MIN(60, cmnd->val[1]);

	t = (long)tm*60;

	target->lasttime[LT_JAILED].ltime = time(0);
	target->lasttime[LT_JAILED].interval = t;

	target->setFlag(P_JAILED);


	//log_immort(true, player, "%s jailed %s for %d minutes.\n", player->name, target->name, tm);
	if(!reason) {
		logn("log.jail", "%s jailed %s for %d minutes.%s\n", player->name, target->name, tm);
		broadcast(isCt, "^y*** %s jailed %s for %d minutes.%s", player->name, target->name, tm);
	} else {
		logn("log.jail", "%s jailed %s for %d minutes. Reason: %s\n", player->name, target->name, tm, reason);
		broadcast(isCt, "^y*** %s jailed %s for %d minutes. Reason: %s", player->name, target->name, tm, reason);
	}

	if(player->isWatcher())
		logn("log.wjail", "%s jailed %s(%s) for %d minutes. Reason: %s\n",
			player->name, target->name, target->currentLocation.room.str().c_str(), tm, reason);

	CatRef	cr;
	cr.setArea("jail");
	cr.id = 1;

	if(!online) {

		if(!strcmp(cmnd->str[2], "-b"))
			broadcast("^R### Cackling demons drag %s to the Dungeon of Despair.", target->name);
		target->currentLocation.room = cr;
		target->currentLocation.mapmarker.reset();

		player->print("%s is now jailed.\n", target->name);

	} else {

		if(!loadRoom(cr, &newRoom)) {
			player->print("Problem loading room %s.\nAborting.\n", cr.str().c_str());
			return(0);
		} else {
			player->print("%s is now jailed.\n", target->name);
			broadcast(NULL, target->getRoomParent(),
				"^RA demonic jailer just arrived.\nThe demonic jailer opens a portal to Hell.\nThe demonic jailer drags %s screaming to the Dungeon of Despair.", target->name);

			target->printColor("^RThe demonic jailer grips your soul and drags you to the Dungeon of Despair.\n");
			broadcast(target->getSock(), target->getRoomParent(), "^RThe portal closes with a puff of smoke.");

			target->deleteFromRoom();
			target->addToRoom(newRoom);
			target->printColor("^RThe demonic jailer says, \"Pathetic mortal!\"\nThe demonic jailer spits on you.\nThe demonic jailer vanishes.\n");
//			target->doPetFollow();
		}

		if(!strcmp(cmnd->str[2], "-b"))
			broadcast("^R### Cackling demons drag %s to the Dungeon of Despair.", target->name);

	}

	target->save();
	if(!online)
		free_crt(target);
	return(0);
}


//*********************************************************************
//						dmLts
//*********************************************************************

int dmLts(Player* player, cmd* cmnd) {
	Player	*target=0;
	int		i=0;
	long	t = time(0);

	if(cmnd-> num < 2) {
		player->print("syntax: *lt <player>\n");
		return(0);
	}
	cmnd->str[1][0] = up(cmnd->str[1][0]);
	target = gServer->findPlayer(cmnd->str[1]);
	if(target == NULL) {
		player->print("%s is not on.\n", cmnd->str[1]);
		return(0);
	}
	player->print("Now: %ld\n", t);
	for(i = 0 ; i < MAX_LT ; i++ ) {
		player->print("LT %d - %ld %ld %ld (%ld)\n", i, target->lasttime[i].interval,
		      target->lasttime[i].ltime, target->lasttime[i].misc, LT(player, i) - t);
	}
	return(0);
}



//*********************************************************************
//						dmLtClear
//*********************************************************************

int dmLtClear(Player* player, cmd* cmnd) {
	Player	*target=0;
//	int		i=0;

	if(cmnd->num < 2) {
		player->print("syntax: *ltclear <player>\n");
		return(0);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target) {
		player->print("%s is not on.\n", cmnd->str[1]);
		return(0);
	}
	target->lasttime[LT_INVOKE].interval = 0;
//	for(i=0; i<MAX_LT; i++) {
//		target->lasttime[i].interval = 0;
//	}
//
	player->print("Last times reset!\n");
	return(0);

}

//*********************************************************************
//						dm2x
//*********************************************************************

int dm2x(Player* player, cmd* cmnd) {
	bstring forum1;
	bstring forum2;

	if(cmnd->num == 1) {

		// give them a list
		player->printColor("Syntax: *2x                          - list accounts that may double log.\n");
		player->printColor("Syntax: *2x -add ^e<account> <account>^x - allow accounts to double log.\n");
		player->printColor("Syntax: *2x -del ^e<account> <account>^x - prevent accounts to double log.\n");
		player->print("\n");
		gConfig->listDoubleLog(player);

	} else if(!strcmp(cmnd->str[1], "-add")) {

		forum1 = getFullstrText(cmnd->fullstr, 2);
		forum2 = getFullstrText(cmnd->fullstr, 3);
		forum1 = forum1.substr(0, forum1.getLength() - forum2.getLength() - 1);

		// add a new account pair
		if(forum1 == "" || forum2 == "") {
			player->print("Account names not provided.\n");
		} else {
			player->printColor("^G*ADD*^X Accounts \"%s\" and \"%s\" may now double log.\n",
				forum1.c_str(), forum2.c_str());
			gConfig->addDoubleLog(forum1, forum2);
		}

	} else if(!strcmp(cmnd->str[1], "-del") || !strcmp(cmnd->str[1], "-rem")) {

		forum1 = getFullstrText(cmnd->fullstr, 2);
		forum2 = getFullstrText(cmnd->fullstr, 3);
		forum1 = forum1.substr(0, forum1.getLength() - forum2.getLength() - 1);

		// remove an account pair
		if(forum1 == "" || forum2 == "") {
			player->print("Account names not provided.\n");
		} else {
			player->printColor("^R*DEL*^X Accounts \"%s\" and \"%s\" may no longer double log.\n",
				forum1.c_str(), forum2.c_str());
			gConfig->remDoubleLog(forum1, forum2);
		}

	} else {
		player->printColor("Command not understood.\n");
		player->printColor("Syntax: *2x                          - list accounts that may double log.\n");
		player->printColor("Syntax: *2x -add ^e<account> <account>^x - allow accounts to double log.\n");
		player->printColor("Syntax: *2x -del ^e<account> <account>^x - prevent accounts to double log.\n");
	}
	return(0);
}
