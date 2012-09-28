/*
 * rogues.cpp
 *   Functions that deal with rouge like abilities
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
#include "commands.h"
#include "dm.h"
#include "move.h"
#include "factions.h"
#include "calendar.h"


//*********************************************************************
//						cmdPrepareForTraps
//*********************************************************************
// This function allows players to prepare themselves for traps that
// might be in the next room they enter.  This way, they can hope to
// avoid a trap that they already know is there.

int cmdPrepareForTraps(Player* player, cmd* cmnd) {
	long	i=0, t = time(0);


	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(player->flagIsSet(P_PREPARED)) {
		player->print("You've already prepared.\n");
		return(0);
	}


	if(player->inCombat()) {
		player->print("You are too busy trying to keep from dying.\n");
		return(0);
	}

	i = LT(player, LT_PREPARE);

	if(t < i) {
		player->pleaseWait(i-t);
		return(0);
	}

	player->lasttime[LT_PREPARE].ltime = t;
	player->lasttime[LT_PREPARE].interval = player->isDm() ? 0:15;

	player->print("You prepare yourself for traps.\n");
	broadcast(player->getSock(), player->getParent(), "%M prepares for traps.", player);
	player->setFlag(P_PREPARED);
	if(player->isBlind())
		player->clearFlag(P_PREPARED);

	return(0);
}

//*********************************************************************
//						cmdBribe
//*********************************************************************
// This function allows players to bribe monsters. If they give the
// monster enough money, it will leave the room. If not, the monster
// keeps the money and stays.

int cmdBribe(Player* player, cmd* cmnd) {
	Monster *creature=0;
	unsigned long amount=0;
	Money cost;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(player->getClass() == BUILDER) {
		player->print("You cannot do that.\n");
		return(0);
	}
	if(cmnd->num < 2) {
		player->print("Bribe whom?\n");
		return(0);
	}
	if(cmnd->num < 3 || cmnd->str[2][0] != '$') {
		player->print("Syntax: bribe <monster> $<amount>\n");
		return(0);
	}

	creature = player->getParent()->findMonster(player, cmnd);
	if(!creature) {
		player->print("That is not here.\n");
		return(0);
	}

	if(!Faction::willDoBusinessWith(player, creature->getPrimeFaction())) {
		player->print("%M refuses to do business with you.\n", creature);
		return(0);
	}

	if(creature->isPet()) {
		player->print("%M is too loyal to %s for you to bribe %s.\n", creature,
			creature->getMaster()->getName(), creature->himHer());
		creature->getMaster()->print("%M tried to bribe %N.\n", player, creature);
		return(0);
	}

	amount = strtoul(&cmnd->str[2][1], 0, 0);
	if(amount < 1 || amount > player->coins[GOLD]) {
		player->print("Please enter the amount of the bribe.\n");
		return(0);
	}

	cost.set(creature->getExperience()*50, GOLD);
	cost = Faction::adjustPrice(player, creature->getPrimeFaction(), cost, true);

	player->coins.sub(amount, GOLD);

	gServer->logGold(GOLD_OUT, player, Money(amount, GOLD), creature, "Bribe");

	if(amount < cost[GOLD] || creature->flagIsSet(M_PERMENANT_MONSTER)) {
		player->print("%M takes your money, but stays.\n", creature);
		broadcast(player->getSock(), player->getParent(), "%M tried to bribe %N.", player, creature);
		creature->coins.add(amount, GOLD);
	} else {
		player->print("%M takes your money and leaves.\n", creature);
		broadcast(player->getSock(), player->getParent(), "%M bribed %N.", player, creature);

		log_immort(true, player, "%s bribed %s.\n", player->name, creature->name);

		creature->deleteFromRoom();
		gServer->delActive(creature);
		free_crt(creature);
	}

	return(0);
}


//*********************************************************************
//						cmdGamble
//*********************************************************************
// Code for people to gamble money

int cmdGamble(Player* player, cmd* cmnd) {
	if(!player->getRoomParent()->flagIsSet(R_CASINO) && !player->isCt()) {
		player->print("You can't gamble here.\n");
		return(0);
	}

	player->unhide();

	player->print("Not implemented yet.\n");
	return(0);
}

//*********************************************************************
//						canSearch
//*********************************************************************

bool canSearch(const Player* player) {
	if(!player->ableToDoCommand())
		return(false);

	if(player->isBlind()) {
		player->print("You're blind! How can you do that?\n");
		return(false);
	}

	if(player->flagIsSet(P_SITTING)) {
		player->print("You must stand up first!\n");
		return(false);
	}

	return(true);
}

//*********************************************************************
//						doSearch
//*********************************************************************

void doSearch(Player* player, bool immediate) {
	BaseRoom* room = player->getRoomParent();
	otag	*op=0;
	int		chance=0;
	bool	found=false, detectMagic = player->isEffected("detect-magic");

	player->clearFlag(P_AFK);

	if(!canSearch(player))
		return;

	player->unhide();


	int level = (int)(player->getSkillLevel("search"));

	// TODO: SKILLS: tweak the formula
	chance = 15 + 5*bonus((int)player->piety.getCur()) + level*2;
	chance = MIN(chance, 90);
	if(player->getClass() == RANGER)
		chance += level*8;
	if(player->getClass() == WEREWOLF)
		chance += level*7;
	if(player->getClass() == DRUID)
		chance += level*6;

	if(player->getClass() == CLERIC && (
		(player->getDeity() == KAMIRA && player->getAdjustedAlignment() >= NEUTRAL) ||
		(player->getDeity() == ARACHNUS && player->alignInOrder())
	) )
		chance += level*3;

	if(player->isStaff())
		chance = 100;

	
	for(Exit* ext : room->exits) {
		if(	(ext->flagIsSet(X_SECRET) && mrand(1,100) <= (chance + searchMod(ext->getSize()))) ||
			(ext->isConcealed(player) && mrand(1,100) <= 5))
		{
			// canSee doesnt handle DescOnly
			if(player->canSee(ext) && !ext->flagIsSet(X_DESCRIPTION_ONLY)) {
				found = true;
				player->printColor("You found an exit: %s^x.\n", ext->name);

				if(ext->isWall("wall-of-fire"))
					player->printColor("%s", ext->blockedByStr('R', "wall of fire", "wall-of-fire", detectMagic, true).c_str());
				if(ext->isWall("wall-of-force"))
					player->printColor("%s", ext->blockedByStr('s', "wall of force", "wall-of-force", detectMagic, true).c_str());
				if(ext->isWall("wall-of-thorns"))
					player->printColor("%s", ext->blockedByStr('o', "wall of thorns", "wall-of-thorns", detectMagic, true).c_str());
			}
		}
	}

	op = room->first_obj;
	while(op) {
		if(	op->obj->flagIsSet(O_HIDDEN) &&
			player->canSee(op->obj) &&
			mrand(1,100) <= (chance + searchMod(op->obj->getSize()))
		) {
			found = true;
			player->printColor("You found %1P.\n", op->obj);
		}
		op = op->next_tag;
	}

	for(Player* ply : room->players) {
		if(	ply->flagIsSet(P_HIDDEN) &&
			player->canSee(ply) &&
			mrand(1,100) <= (chance + searchMod(ply->getSize())))
		{
			found = true;
			player->print("You found %s hiding.\n", ply->name);
		}
	}

	for(Monster* mons : room->monsters) {
		if(	mons->flagIsSet(M_HIDDEN) &&
			player->canSee(mons) &&
			mrand(1,100) <= (chance + searchMod(mons->getSize())))
		{
			found = true;
			player->print("You found %1N hiding.\n", mons);
		}
	}

	// reuse chance as chance to find herbs
	// make it a 15-85% scale, range from 1-MAXALVL*10
	chance = level * (MAXALVL*10 / (85-15)) + 15;

	if(player->isStaff())
		chance = 100;

	if(chance >= mrand(1,100)) {
		if(player->inAreaRoom() && player->getAreaRoomParent()->spawnHerbs()) {
			player->print("You found some herbs!\n");
			found = true;
		}
	}


	if(found) {
		if(immediate)
			broadcast(player->getSock(), room, "%s found something!", player->upHeShe());
		else
			broadcast(player->getSock(), room, "%M found something!", player);
		player->checkImprove("search", true);
	} else {
		player->print("You didn't find anything.\n");
		player->checkImprove("search", false);
	}
}

void doSearch(const DelayedAction* action) {
	doSearch(action->target->getAsPlayer(), false);
}

//*********************************************************************
//						cmdSearch
//*********************************************************************
// This function allows a player to search a room for hidden objects,
// exits, monsters and players.

int cmdSearch(Player* player, cmd* cmnd) {
	long	i=0, t = time(0);

	player->clearFlag(P_AFK);

	if(!canSearch(player))
		return(0);
	if(gServer->hasAction(player, ActionSearch)) {
		player->print("You are already searching!\n");
		return(0);
	}

	i = LT(player, LT_SEARCH);

	if(t < i) {
		player->pleaseWait(i-t);
		return(0);
	}

	player->lasttime[LT_SEARCH].ltime = t;
	if( player->isDm() )
		player->lasttime[LT_SEARCH].interval = 0;
	else if(player->getClass() == RANGER || player->getClass() == WEREWOLF)
		player->lasttime[LT_SEARCH].interval = 3;
	else if((player->getClass() == CLERIC && player->getDeity() == KAMIRA && player->getAdjustedAlignment() >= NEUTRAL) || player->getClass() == THIEF || player->getClass() == ROGUE)
		player->lasttime[LT_SEARCH].interval = 5;
	else
		player->lasttime[LT_SEARCH].interval = 7;
	
	player->interruptDelayedActions();

	broadcast(player->getSock(), player->getParent(), "%M searches the room.", player);

	// big rooms take longer to search
	if(player->getRoomParent()->getSize() >= SIZE_GARGANTUAN) {
		// doSearch calls unhide, no need to do it twice
		player->unhide();

		player->print("You begin searching.\n");
		gServer->addDelayedAction(doSearch, player, 0, ActionSearch, player->lasttime[LT_SEARCH].interval);
	} else {
		doSearch(player, true);
	}
	return(0);
}

//*********************************************************************
//						spawnHerbs
//*********************************************************************

bool AreaRoom::spawnHerbs() {
	if(!area)
		return(false);
	const TileInfo* tile = area->getTile(area->getTerrain(0, &mapmarker, 0, 0, 0, true), area->getSeasonFlags(&mapmarker));
	if(!tile)
		return(false);
	return(tile->spawnHerbs(this));
}

//*********************************************************************
//						spawnHerbs
//*********************************************************************

bool TileInfo::spawnHerbs(BaseRoom* room) const {
	Object*	object=0;
	otag*	op = room->first_obj;
	int		max = herbs.size();
	short	num = mrand(1,MAX(1,max)), i=0, n=0, k=0;
	std::list<CatRef>::const_iterator it;
	bool	found=false;

	if(!max)
		return(false);

	// don't spawn if there's already herbs here
	while(op) {
		if(op->obj->flagIsSet(O_DISPOSABLE))
			return(false);
		op = op->next_tag;
	}

	for(; i<num; i++) {
		k = 1;
		n = mrand(1, max);

		for(it = herbs.begin() ; it != herbs.end() ; it++) {
			if(k++==n)
				break;
		}

		if(loadObject(*it, &object)) {
			found = true;
			object->setFlag(O_DISPOSABLE);
			object->setDroppedBy(room, "SpawnHerb");
			object->addToRoom(room);
		}
	}

	return(found);
}

//*********************************************************************
//						cmdHide
//*********************************************************************
// This command allows a player to try and hide themself in the shadows
// or it can be used to hide an object in a room.

int cmdHide(Player* player, cmd* cmnd) {
	Object	*object=0;
	long	i = LT(player, LT_HIDE), t = time(0);
	int		chance=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("hide")) {
		player->print("You don't know how to hide effectively.\n");
		return(0);
	}
	if(player->flagIsSet(P_SITTING)) {
		player->print("You cannot do that while sitting down.\n");
		return(0);
	}

	if(player->isEffected("mist")) {
		player->print("You are already hidden as a mist.\n");
		return(0);
	}

	if(i > t && !player->isCt()) {
		player->pleaseWait(i-t);
		return(0);
	}

	player->lasttime[LT_HIDE].ltime = t;
	if(	player->getClass() == THIEF ||
		player->getClass() == ASSASSIN ||
		player->getClass() == ROGUE ||
		player->getClass() == RANGER
	)
		player->lasttime[LT_HIDE].interval = 5;
	else
		player->lasttime[LT_HIDE].interval = 15;


	if(player->getSecondClass() == THIEF || player->getSecondClass() == ASSASSIN || (player->getClass() == CLERIC && player->getDeity() == KAMIRA))
		player->lasttime[LT_HIDE].interval = 6L;

	int level = (int)player->getSkillLevel("hide");
	if(cmnd->num == 1) {
		switch(player->getClass()) {
		case THIEF:
			if(player->getSecondClass() == MAGE) {
				chance = MIN(90, 5 + 4*level + 3*bonus((int) player->dexterity.getCur()));
				player->lasttime[LT_HIDE].interval = 8;
			} else
				chance = MIN(90, 5 + 6*level + 3*bonus((int) player->dexterity.getCur()));
			break;
		case ASSASSIN:
			chance = MIN(90, 5 + 6*level + 3*bonus((int) player->dexterity.getCur()));
			break;
		case FIGHTER:
			if(player->getSecondClass() == THIEF)
				chance = MIN(90, 5 + 4*level + 3*bonus((int) player->dexterity.getCur()));
			else
				chance = MIN(90, 5 + 2*level + 3*bonus((int) player->dexterity.getCur()));
			break;
		case MAGE:
			if(player->getSecondClass() == ASSASSIN || player->getSecondClass() == THIEF) {
				chance = MIN(90, 5 + 4*level + 3*bonus((int) player->dexterity.getCur()));
				player->lasttime[LT_HIDE].interval = 8;
			} else
				chance = MIN(90, 5 + 2*level +
						3*bonus((int) player->dexterity.getCur()));
			break;
		case CLERIC:
			if(player->getSecondClass() == ASSASSIN) {
				chance = MIN(90, 5 + 5*level + 3*bonus((int) player->dexterity.getCur()));
			} else if( (player->getDeity() == KAMIRA && player->getAdjustedAlignment() >= NEUTRAL) ||
				(player->getDeity() == ARACHNUS && player->alignInOrder())
			) {
				chance = MIN(90, 5 + 4*level + 3*bonus((int) player->piety.getCur()));
			} else
				chance = MIN(90, 5 + 2*level + 3*bonus((int) player->dexterity.getCur()));

			break;
		case RANGER:
		case DRUID:
			chance = 5 + 10*level + 3*bonus((int) player->dexterity.getCur());
			break;
		case ROGUE:
			chance = MIN(90, 5 + 5*level + 3*bonus((int) player->dexterity.getCur()));
			break;
		default:
			chance = MIN(90, 5 + 2*level + 3*bonus((int) player->dexterity.getCur()));
			break;
		}


		if(player->isStaff())
			chance = 101;


		if(player->isEffected("camouflage") && player->getRoomParent()->isOutdoors())
			chance += 20;

		if(	(player->getRoomParent()->flagIsSet(R_DARK_AT_NIGHT) && !isDay()) ||
			player->getRoomParent()->flagIsSet(R_DARK_ALWAYS) ||
			player->getRoomParent()->isEffected("dense-fog")
		)
			chance += 10;

		if(player->dexterity.getCur()/10 < 9 && !(player->getClass() == CLERIC && player->getDeity() == KAMIRA))
			chance -= 10*(9 - player->dexterity.getCur()/10); // Having less then average dex

		player->print("You attempt to hide in the shadows.\n");

		if((player->getClass() == RANGER || player->getClass() == DRUID) && !player->getRoomParent()->isOutdoors()) {
			chance /= 2;
			chance = MAX(25, chance);
			player->print("You have trouble hiding while inside.\n");
		}

		if(player->inCombat())
			chance = 0;

		if(player->isBlind())
			chance = MIN(chance, 20);

		if(mrand(1,100) <= chance || player->isEffected("mist")) {
			player->setFlag(P_HIDDEN);
			player->print("You slip into the shadows unnoticed.\n");
			player->checkImprove("hide", true);
		} else {
			player->unhide();
			broadcast(player->getSock(), player->getParent(), "%M tries to hide in the shadows.", player);
			player->checkImprove("hide", false);
		}

		return(0);
	}

	object = player->getRoomParent()->findObject(player, cmnd, 1);

	if(!object) {
		player->print("You don't see that here.\n");
		return(0);
	}

	player->unhide();

	if(	object->flagIsSet(O_NO_TAKE) &&
		!player->checkStaff("You cannot hide that.\n")
	)
		return(0);

	if(isGuardLoot(player->getRoomParent(), player, "%M will not let you hide that.\n"))
		return(0);

	if(player->isDm())
		chance = 100;
	else if(player->getClass() == THIEF || player->getClass() == ASSASSIN || player->getClass() == ROGUE)
		chance = MIN(90, 10 + 5*level + 5*bonus((int) player->dexterity.getCur()));
	else if(player->getClass() == RANGER || player->getClass() == DRUID)
		chance = 5 + 9*level + 3*bonus((int) player->dexterity.getCur());
	else
		chance = MIN(90, 5 + 3*level + 3*bonus((int) player->dexterity.getCur()));



	player->print("You attempt to hide it.\n");
	broadcast(player->getSock(), player->getParent(), "%M attempts to hide %1P.", player, object);

	if(mrand(1, 100) <= chance) {
		object->setFlag(O_HIDDEN);
		player->printColor("You tuck %1P into a corner.\n", object);
		player->checkImprove("hide", true);
	} else {
		object->clearFlag(O_HIDDEN);
		player->checkImprove("hide", false);
	}

	return(0);
}

//*********************************************************************
//						doScout
//*********************************************************************

bool doScout(Player* player, const Exit* exit) {
	BaseRoom *room=0;


	if(exit->flagIsSet(X_STAFF_ONLY) && !player->isStaff()) {
		player->print("A magical force prevents you from seeing into the room.\n");
		return(false);
	}


	Move::getRoom(player, exit, &room, true);

	if(player->getClass() == BUILDER && room && !room->isConstruction()) {
		player->print("A magical force prevents you from seeing into the room.\n");
		return(false);
	}

	if(	room &&
		room->isEffected("dense-fog")&&
		!player->checkStaff("That room is filled with a dense fog.\n"))
		return(false);

	if(!room && exit->target.mapmarker.getArea()) {
		Area* area = gConfig->getArea(exit->target.mapmarker.getArea());
		if(!area) {
			player->print("Off the map in that direction.\n");
			if(player->isStaff())
				player->printColor("^eArea does not exist.\n");
			return(false);
		}
		// There is no room, but we're going to pretend there is.
		player->print("\n");
		if(area->name != "")
			player->printColor("%s%s^x\n\n",
				(!player->flagIsSet(P_NO_EXTRA_COLOR) && area->isSunlight(&exit->target.mapmarker) ? "^C" : "^c"),
				area->name.c_str());

		player->printColor("%s", area->showGrid(player, &exit->target.mapmarker, false).c_str());
		player->printColor("^g%s exits: north, east, south, west, northeast, northwest, southeast, southwest.^w\n\n",
			player->isStaff() ? "All" : "Obvious");

	} else if(room) {
		display_rom(player, room);
	} else {
		player->print("Off the map in that direction.\n");
		if(player->isStaff())
			player->printColor("^eRoom does not exist.\n");
		return(false);
	}

	return(true);
}

//*********************************************************************
//						cmdScout
//*********************************************************************

int cmdScout(Player* player, cmd* cmnd) {
	Exit	*exit=0;
	int		chance=0;
	long	t=0, i=0;
	bool	alwaysSucceed=false;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(!player->isStaff() && !player->knowsSkill("scout")) {
		player->print("You lack the training to properly scout exits.\n");
		return(0);
	}

	if(!player->isStaff() && player->isBlind()) {
		player->printColor("^CYou're blind!\n");
		return(0);
	}

	if(	player->getRoomParent()->isEffected("dense-fog") &&
		!player->checkStaff("This room is filled with a dense fog.\nYou cannot scout effectively.\n")
	)
		return(0);

	if(cmnd->num < 2) {
		player->print("Scout where?\n");
		return(0);
	}

	i = player->lasttime[LT_SCOUT].ltime + player->lasttime[LT_SCOUT].interval;
	t = time(0);

	if(!player->isStaff() && t < i) {
		player->pleaseWait(i - t);
		return(0);
	}

	exit = findExit(player, Move::formatFindExit(cmnd), cmnd->val[1], player->getRoomParent());


	if(!exit) {
		player->print("You don't see that exit.\n");
		player->lasttime[LT_SCOUT].ltime = t;
		player->lasttime[LT_SCOUT].interval = 10L;
		return(0);
	}


	alwaysSucceed = (exit->flagIsSet(X_CAN_LOOK) || exit->flagIsSet(X_LOOK_ONLY));

	if(	!alwaysSucceed &&
		exit->flagIsSet(X_NO_SCOUT) &&
		!player->checkStaff("You were unable to scout it.\n"))
		return(0);

	const Monster* guard = player->getRoomParent()->getGuardingExit(exit, player);
	if(guard && !player->checkStaff("%M %s.\n", guard, guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you scout there" : "won't let you scout there"))
		return(0);

	if(!alwaysSucceed) {
		player->lasttime[LT_SCOUT].ltime = t;
		player->lasttime[LT_SCOUT].interval = 20L;

		chance = 40 + (bonus((int) player->dexterity.getCur()) + (int)player->getSkillLevel("scout")) * 4;

		if(exit->isEffected("wall-of-fire"))
			chance -= 15;
		if(exit->isEffected("wall-of-thorns"))
			chance -= 15;

		chance = MIN(85, chance);

		if(!player->isStaff() && mrand(1, 100) > chance) {
			player->print("You fail scout in that direction.\n");
			player->checkImprove("scout", false);
			return(0);
		}

		if(	(exit->flagIsSet(X_LOCKED) || exit->flagIsSet(X_CLOSED) ) &&
			!player->checkStaff("You cannot scout through a closed exit.\n")
		)
			return(0);

		if(	exit->flagIsSet(X_NEEDS_FLY) &&
			!player->isEffected("fly") &&
			!player->checkStaff("You must fly to scout there.\n")
		)
			return(0);
	}

	// can't scout where you can't go
	if(exit->target.mapmarker.getArea()) {
		Area *area = gConfig->getArea(exit->target.mapmarker.getArea());
		if(!area) {
			player->print("Off the map in that direction.\n");
			if(player->isStaff())
				player->printColor("^eArea does not exist.\n");
			return(0);
		}
		if(	!area->canPass(player, &exit->target.mapmarker, true) &&
			!player->checkStaff("You cannot scout that way.\n") )
			return(0);
	}

	if(!alwaysSucceed)
		player->checkImprove("scout", true);
	player->printColor("You scout the %s^x exit.\n", exit->name);

	if(player->isStaff() && player->flagIsSet(P_DM_INVIS))
		broadcast(isStaff, player->getSock(), player->getRoomParent(), "%M scouts the %s^x exit.", player, exit->name);
	else if(exit->flagIsSet(X_SECRET) || exit->isConcealed() || exit->flagIsSet(X_DESCRIPTION_ONLY))
		broadcast(player->getSock(), player->getParent(), "%M scouts the area.", player);
	else
		broadcast(player->getSock(), player->getParent(), "%M scouts the %s^x exit.", player, exit->name);

	doScout(player, exit);
	return(0);
}

//*********************************************************************
//						cmdEnvenom
//*********************************************************************
// This will allow an assassin to put poison on a weapon and use it to
// poison other players and monsters. -- TC

int cmdEnvenom(Player* player, cmd* cmnd) {
	Object		*weapon=0, *object=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("envenom")) {
		player->print("You lack the training to envenom your weapons.\n");
		return(0);
	}

	if(player->isBlind()) {
		player->printColor("^CYou can't do that! You're blind!\n");
		return(0);
	}

	if(cmnd->num < 3) {
		player->print("Syntax: envenom (weapon) (poison)\n");
		return(0);
	}

	weapon = player->findObject(player, cmnd, 1);

	if(!weapon) {
		player->print("You do not have that weapon in your inventory.\n");
		return(0);
	}

	if(weapon->flagIsSet(O_ENVENOMED)) {
		player->printColor("%O is already envenomed.\n", weapon);
		return(0);
	}

	if(weapon->getShotsCur() < 1) {
		player->print("You cannot envenom a broken weapon.\n");
		return(0);
	}

	bstring category = weapon->getWeaponCategory();
	if(category != "slashing" && category != "piercing"&&
		!player->checkStaff("You can only envenom slashing and piercing weapons.\n"))
		return(0);

	object = player->findObject(player, cmnd, 2);

	if(!object) {
		player->print("You do not have that poison in your inventory.\n");
		return(0);
	}

	if(object->getType() != POISON) {
		player->print("That is not poison.\n");
		return(0);
	}

	if(object->getShotsCur() < 1) {
		player->printColor("%O is all used up.\n", object);
		return(0);
	}

	player->printColor("You envenom %P with %P.\n", weapon, object);
	player->checkImprove("envenom", true);
	broadcast(player->getSock(), player->getParent(), "%M envenoms %P with %P.", player, weapon, object);


	// TODO: make poison more powerful with better envenom skill
	object->decShotsCur();
	weapon->setFlag(O_ENVENOMED);
	if(object->getEffect() != "")
		weapon->setEffect(object->getEffect());
	else
		weapon->setEffect("poison");
	weapon->setEffectDuration(object->getEffectDuration()); // Sets poison duration.
	weapon->setEffectStrength(object->getEffectStrength()); // Sets poison tick damage.

	// Did this because some poisons are better then others, and assassins
	// will be buying the poison. The more expensive, the better it is. Having
	// them have to buy it makes it more of a pain for some idiot to go around
	// poisoning everyone with a small knife just for fun.

	weapon->lasttime[LT_ENVEN].ltime = time(0);
	weapon->lasttime[LT_ENVEN].interval = 60*(mrand(3,5)+weapon->getAdjustment());

	return(0);
}


//*********************************************************************
//						cmdShoplift
//*********************************************************************
// This function will allow thieves to steal from shops.

bool isValidShop(const UniqueRoom* shop, const UniqueRoom* storage);

int cmdShoplift(Player* player, cmd* cmnd) {
	UniqueRoom	*room=0, *storage=0;
	Object	*object=0, *object2=0;
	int		chance=0, guarded=0;


	if(!player->ableToDoCommand())
		return(0);

	if(!needUniqueRoom(player))
		return(0);

	if(player->flagIsSet(P_SITTING)) {
		player->print("You have to stand up first.\n");
		return(0);
	}

	if(player->getClass() == BUILDER)
		return(0);

	room = player->getUniqueRoomParent();

	if(!player->isCt() && player->getLevel() < 7 && player->getClass() != THIEF) {
		player->print("You couldn't possibly succeed. Wait until level 7.\n");
		return(0);
	}

	if(player->getClass() == THIEF && player->getLevel() < 4) {
		player->print("You couldn't possibly succeed. Wait until level 4.\n");
		return(0);
	}

	if(player->getClass() == PALADIN) {
		player->print("Paladins do not allow themselves to fall to the level of petty theft.\n");
		return(0);
	}

	if(player->isEffected("berserk")) {
		player->print("You can't shoplift while your going berserk! Go break something!\n");
		return(0);
	}

	if(!room->flagIsSet(R_SHOP)) {
		player->print("This is not a shop; there's nothing to shoplift.\n");
		return(0);
	}

	if(!room->flagIsSet(R_CAN_SHOPLIFT)) {
		player->print("Someone surely would see you if you tried that here.\n");
		return(0);
	}

	if(player->inCombat()) {
		player->print("You can't shoplift in the middle of combat!\n");
		return(0);
	}

	if(player->isEffected("mist")) {
		player->print("You can't shoplift while you are a mist.\n");
		return(0);
	}



	loadRoom(shopStorageRoom(room), &storage);

	if(!isValidShop(room, storage)) {
		player->print("This is not a shop; there's nothing to shoplift.\n");
		return(0);
	}


	
	storage->addPermCrt();
	for(Monster* mons : storage->monsters) {
		if(mons->getLevel() >= 10 && mons->flagIsSet(M_PERMENANT_MONSTER))
			guarded++;
	}

	// This is done in order that a catastrophic building mistake cannot be made.
	// If someone forgot to perm mobs in the storeroom, and there were no perms
	// in the shop itself to guard the items, then players would be able to steal
	// all they wanted from the shop all day long whenever they wanted to.

	if(guarded < 2) {
		player->print("Someone surely would see you if you tried that here.\n");
		return(0);
	}

	// This has to be done to prevent baiting.
	// The mobs will wander after a time.
	for(Monster* mons : room->monsters) {
		if(mons->flagIsSet(M_ATTACKING_SHOPLIFTER) && !player->isDm()) {
			player->print("The shopkeep or shop's guards are too alert right now.\n");
			return(0);
		}
	}


	if(cmnd->num < 2) {
		player->print("Shoplift what?\n");
		return(0);
	}

	object = findObject(player, storage->first_obj, cmnd);

	if(!object) {
		player->print("That item isn't on display.\n");
		return(0);
	}
	//	if(!strcmp(object->name, "lottery ticket"))
	//	{
	//		player->print("Shoplifting a lottery ticket would bring down the wrath of the gods.\n");
	//		return(0);
	//	}

	if(!strcmp(object->name, "storage room")) {
		player->print("You can't shoplift that!\n");
		return(0);
	}

	if(object->flagIsSet(O_NO_SHOPLIFT)) {
		player->print("That item is too well guarded for you to shoplift.\n");
		return(0);
	}

	if(object->getActualWeight() > 50) {
		player->print("That object is too heavy and bulky to shoplift.\n");
		return(0);
	}

	if(player->getWeight() + object->getActualWeight() > player->maxWeight()) {
		player->print("That would be too much for you to carry.\n");
		return(0);
	}

	player->unhide();

	switch (player->getClass()) {
	case THIEF:
	case CARETAKER:
	case DUNGEONMASTER:
		chance = ((player->getLevel()+35 - (2*object->getActualWeight())) - (object->value[GOLD]/1500))
				+ (bonus((int) player->dexterity.getCur())*2);
		break;

	case ASSASSIN:
	case BARD:
	case ROGUE:
		chance = ((player->getLevel()+20 - (2*object->getActualWeight())) - (object->value[GOLD]/1500))
				+ (bonus((int) player->dexterity.getCur())*2);
		break;

	// Casting classes just aren't cut out for it.
	case MAGE:
	case LICH:
	case DRUID:
	case CLERIC:
		chance = (((int)player->getLevel() - (2*object->getActualWeight())) - (object->value[GOLD]/1500))
				+ (bonus((int) player->dexterity.getCur())*2);
	default:
		chance = (((int)player->getLevel()+10 - (2*object->getActualWeight())) - (object->value[GOLD]/1500))
				+ (bonus((int) player->dexterity.getCur())*2);
		break;
	}

	// Armor makes a lot of noise
	if(object->getType() == ARMOR && object->getWearflag() != FINGER)
		chance -= 10;

	// An invisible person would have it easier.
	if(player->isInvisible())
		chance += 5;


	chance = MIN(70, MAX(2,chance));
	if(player->isCt())
		player->print("Chance: %d%\n", chance);

	if(player->isDm())
		chance = 101;

	player->statistics.attemptSteal();
	player->interruptDelayedActions();

	if(mrand(1,100) > chance) {
		player->setFlag(P_CAUGHT_SHOPLIFTING);
		player->print("You were caught!\n");
		broadcast(player->getSock(), room, "%M tried to shoplift %1P.\n%M was seen!", player, object, player);

		player->smashInvis();

		// Activates lag protection.
		if(player->flagIsSet(P_LAG_PROTECTION_SET))
			player->setFlag(P_LAG_PROTECTION_ACTIVE);

		for(Monster* mons : room->monsters) {
			if(mons->flagIsSet(M_PERMENANT_MONSTER)) {

				gServer->addActive(mons);
				mons->clearFlag(M_DEATH_SCENE);
				mons->clearFlag(M_FAST_WANDER);
				mons->addPermEffect("detect-invisible");
				mons->addPermEffect("true-sight");
				mons->setFlag(M_BLOCK_EXIT);
				mons->setFlag(M_ATTACKING_SHOPLIFTER);

				mons->addEnemy(player, true);
				mons->diePermCrt();
			}
		}
		MonsterSet::iterator mIt = storage->monsters.begin();

		while(mIt != storage->monsters.end()) {

		    Monster *mons = (*mIt++);
			if(mons->flagIsSet(M_PERMENANT_MONSTER)) {
				if(!storage->players.empty())
					broadcast(mons->getSock(), mons->getRoomParent(),
						"%M runs out after a shoplifter.", mons);
				else
					gServer->addActive(mons);
				mons->clearFlag(M_PERMENANT_MONSTER);  // This is all done to prevent people from getting
				mons->clearFlag(M_FAST_WANDER);  // killed mostly by others baiting these mobs in.
				mons->clearFlag(M_DEATH_SCENE);
				mons->addPermEffect("detect-invisible");
				mons->addPermEffect("true-sight");
				mons->setFlag(M_BLOCK_EXIT);
				mons->setFlag(M_WILL_ASSIST);
				mons->setFlag(M_WILL_BE_ASSISTED);
				mons->clearFlag(M_AGGRESSIVE_GOOD);
				mons->clearFlag(M_AGGRESSIVE_EVIL);
				mons->clearFlag(M_AGGRESSIVE);

				mons->setFlag(M_OUTLAW_AGGRO);
				mons->setFlag(M_ATTACKING_SHOPLIFTER);

				mons->setExperience(10);
				mons->coins.zero();

				mons->addEnemy(player, true);
				mons->diePermCrt();
				mons->deleteFromRoom();
				mons->addToRoom(room);
			}
		}

		// prevents shoptlift ; flee
		player->stun(mrand(3,5));

	} else {

		object2 = new Object;
		if(!object2)
			merror("shoplift", FATAL);

		*object2 = *object;
		object2->clearFlag(O_PERM_INV_ITEM);
		object2->clearFlag(O_PERM_ITEM);
		object2->clearFlag(O_TEMP_PERM);

		object2->setFlag(O_WAS_SHOPLIFTED);
		object2->setDroppedBy(room, "Shoplift");

		player->addObj(object2);
		player->printColor("You manage to conceal %1P on your person.\n", object2);
		broadcast(isCt, player->getSock(), player->getRoomParent(), "*DM* %M just shoplifted %1P.", player, object2);
		player->statistics.steal();
	}

	return(0);
}

//*********************************************************************
//						cmdBackstab
//*********************************************************************
// This function allows thieves and assassins to backstab a monster.
// If successful, a damage multiplier is given to the player. The
// player must be successfully hidden for the backstab to work. If
// the backstab fails, then the player is forced to wait double the
// normal amount of time for their next attack.

int cmdBackstab(Player* player, cmd* cmnd) {
	Player	*pTarget=0;
	Creature* target=0;
	int		 n=0;
	int		disembowel=0, dur=0, dmg=0;
	float	stabMod=0.0, cap=0.0;
	Damage damage;

	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("backstab")) {
		player->print("You don't know how to backstab.\n");
		return(0);
	}


	if(player->isBlind()) {
		player->print("Backstab what?\n");
		return(0);
	}

	Object* weapon = player->ready[WIELD - 1];

	if(!weapon && !player->checkStaff("Backstabing requires a weapon.\n"))
		return(0);

	bstring category = weapon->getWeaponCategory();

	if(	category != "slashing" && category != "piercing" &&
		!player->checkStaff("Backstabing requires a slashing or piercing weapon.\n")
	)
		return(0);

	if(	weapon && weapon->flagIsSet(O_NO_BACKSTAB) &&
		!player->checkStaff("%O cannot be used to backstab.\n", weapon)
	)
		return(0);


	if(!player->checkAttackTimer())
		return(0);

	if(player->checkHeavyRestrict("backstab"))
		return(0);

    if(!(target = player->findVictim(cmnd, 1, true, false, "Backstab what?\n", "You don't see that here.\n")))
		return(0);

	if(target)
		pTarget = target->getAsPlayer();

	if(!pTarget && target->getAsMonster()->isEnemy(player)) {
		player->print("Not while you're already fighting %s.\n", target->himHer());
		return(0);
	}
	if(!player->canAttack(target))
		return(0);

	player->updateAttackTimer();

	if(player->dexterity.getCur() > 180)
		player->modifyAttackDelay(-10);

	player->print("You attempt to backstab %N.\n", target);
	target->print("%M attempts to backstab you!\n", player);
	broadcast(player->getSock(), target->getSock(), player->getParent(), "%M attempts to backstab %N.", player, target);

	if(target->isMonster())
		target->getAsMonster()->addEnemy(player);

	if(player->breakObject(player->ready[WIELD-1], WIELD)) {
		broadcast(player->getSock(), player->getParent(), "%s backstab failed.", player->upHisHer());
		player->setAttackDelay(player->getAttackDelay()*2);
		return(0);
	}

	int skillLevel = (int)((player->getWeaponSkill(weapon) + (player->getSkillGained("backstab")*2)) / 3);

	AttackResult result = player->getAttackResult(target, weapon, DOUBLE_MISS, skillLevel);

	if(result == ATTACK_HIT || result == ATTACK_CRITICAL || result == ATTACK_BLOCK || result == ATTACK_GLANCING) {
		if(!player->isHidden() && !player->isEffected("mist"))
			result = ATTACK_MISS;

		if(!pTarget && target->flagIsSet(M_NO_BACKSTAB))
			result = ATTACK_MISS;
	}

	player->smashInvis();
	player->unhide();

	int level = (int)player->getSkillLevel("backstab");
	bstring with = Statistics::damageWith(player, player->ready[WIELD-1]);

	if(player->isDm() && result != ATTACK_CRITICAL)
		result = ATTACK_HIT;

	player->statistics.swing();

	if(result == ATTACK_HIT || result == ATTACK_CRITICAL || result == ATTACK_BLOCK || result == ATTACK_GLANCING) {
//		int dmg = 0;

		// Progressive backstab code for thieves and assassins....
		if(player->getClass() == THIEF || player->getSecondClass() == THIEF) {
			if(level < 10)
				stabMod = 2.75;
			else if(level < 16)
				stabMod = 3.25;
			else if(level < 22)
				stabMod = 3.75;
			else if(level < 28)
				stabMod = 4.5;
			else if(level < 36)
				stabMod = 5.0;
			else if(level < 40)
				stabMod = 5.5;
			else
				stabMod = 5.75;
		} else if(player->getClass() == ASSASSIN || player->getSecondClass() == ASSASSIN) {
			if(level < 10)
				stabMod = 3.25;
			else if(level < 13)
				stabMod = 3.75;
			else if(level < 16)
				stabMod = 4;
			else if(level < 19)
				stabMod = 4.25;
			else if(level < 22)
				stabMod = 4.5;
			else if(level < 25)
				stabMod = 5.0;
			else if(level < 28)
				stabMod = 5.25;
			else if(level < 30)
				stabMod = 5.5;
			else if(level < 34)
				stabMod = 6.0;
			else if (level < 38)
				stabMod = 6.25;
			else
				stabMod = 6.5;
		} else
			stabMod = 5.25;



		if(	player->getSecondClass() == THIEF &&
			(player->getClass() == FIGHTER || player->getClass() == MAGE)
		) {
			cap = 3.0;
		}

		if(	player->getSecondClass() == ASSASSIN ||
			(player->getClass() == MAGE && player->getSecondClass() == THIEF)
		) {
			cap = 4.0;
		}

		if(cap > 0)
			stabMod = tMIN<float>(cap, stabMod-1);

		// Version 2.43c Update -- I'm multiplying the stabMod by 2.0 to compensate for the increase
		// in armor absorb, and the removal of multiplier for attackPower
		stabMod *= 2.0;


		// Return of 1 means the weapon was shattered or otherwise rendered unsuable
		int drain = 0;
		bool wasKilled = false, meKilled = false;
		if(player->computeDamage(target, weapon, ATTACK_BACKSTAB, result, damage, true, drain, stabMod) == 1) {
			player->unequip(WIELD, UNEQUIP_DELETE);
			weapon = NULL;
			player->computeAttackPower();
		}
		damage.includeBonus();
		n = damage.get();

		if(result == ATTACK_BLOCK) {
			player->printColor("^C%M partially blocked your attack!\n", target);
			target->printColor("^CYou manage to partially block %N's attack!\n", player);
		}


		if(result == ATTACK_CRITICAL)
			player->statistics.critical();
		else
			player->statistics.hit();
		if(target->isPlayer())
			target->getAsPlayer()->statistics.wasHit();


		disembowel = target->hp.getCur() * 2;

		player->printColor("You backstabbed %N for %s%d^x damage.\n", target, player->customColorize("*CC:DAMAGE*").c_str(), damage.get());
		player->checkImprove("backstab", true);
		log_immort(false,player, "%s backstabbed %s for %d damage.\n", player->name, target->name, damage.get());


		target->printColor( "^M%M^x backstabbed you%s for %s%d^x damage.\n", player,
			target->isBrittle() ? "r brittle body" : "", target->customColorize("*CC:DAMAGE*").c_str(), damage.get());

		broadcastGroup(false, target, "^M%M^x backstabbed ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n",
			player, target, damage.get(), target->heShe(), target->getStatusStr(damage.get()));

		player->statistics.attackDamage(damage.get(), with);

		if(	weapon && weapon->getMagicpower() &&
			weapon->flagIsSet(O_WEAPON_CASTS) &&
			mrand(1,100) <= 50 && weapon->getChargesCur() > 0)
		{
		    n += player->castWeapon(target, weapon, meKilled);
		}

		if(	weapon &&
			(	(weapon->flagIsSet(O_ENVENOMED) && player->getClass() == ASSASSIN && level >=7) ||
				(weapon->flagIsSet(O_ENVENOMED) && player->isCt())
			)
		)
			n += player->checkPoison(target, weapon);

		meKilled = player->doReflectionDamage(damage, target) || meKilled;

		player->doDamage(target, damage.get(), NO_CHECK);
		wasKilled = target->hp.getCur() < 1;

		// Check for disembowel
		if(wasKilled && n > disembowel && mrand(1,100) < 50) {
			switch(mrand(1,4)) {
			case 1:
				player->printColor("^cYou completely disemboweled %N! %s's dead!\n", target, target->upHeShe());
				broadcast(player->getSock(), player->getParent(), "%M completely disembowels %N! %s's dead!",
					player, target, target->upHeShe());
				if(target->isPlayer())
					target->print("%M completely disemboweled you! You're dead!\n", player);

				break;
			case 2:
				player->printColor("^cYou impaled %N through %s back! %s's dead!\n",
					target, target->hisHer(), target->upHeShe());
				broadcast(player->getSock(), player->getParent(), "%M impales %N through %s back! %s's dead!",
					player, target, target->hisHer(), target->upHeShe());
				if(target->isPlayer())
					target->print("%M impaled you through the back! You're dead!\n", player);

				break;
			case 3:
				if(weapon) {
					player->printColor("^cThe %s went completely through %N! %s's dead!\n",
						weapon->name, target, target->upHeShe());
					broadcast(player->getSock(), player->getParent(), "%M's %s goes completely through %N! %s's dead!",
						player, weapon->name, target, target->upHeShe());

					if(target->isPlayer())
						target->print("%M's %s sticks out of your chest! You're dead!\n",
							player, weapon->name);
				}

				break;
			case 4:
				player->printColor("^cYou cut %N in half from behind! %s's dead!\n",
					target, target->upHeShe());
				broadcast(player->getSock(), player->getParent(), "%M cut %N in half from behind! %s's dead!",
					player, target, target->upHeShe());
				if(target->isPlayer())
					target->print("%M cut you in half from behind! You're dead!\n", player);

				break;
			}
		}

		if(weapon)
			weapon->decShotsCur();
		Creature::simultaneousDeath(player, target, false, false);

		// only monsters flee, and not when their attacker was killed
		if(!wasKilled && !meKilled && target->getAsMonster()) {
			if(target->flee() == 2)
				return(0);
		}

	} else if(result == ATTACK_MISS) {
		player->statistics.miss();
		if(target->isPlayer())
			target->getAsPlayer()->statistics.wasMissed();
		player->print("You missed.\n");
		player->checkImprove("backstab", false);
		broadcast(player->getSock(), player->getParent(), "%s backstab failed.", player->upHisHer());
		player->setAttackDelay(mrand(30,90));
	} else if(result == ATTACK_DODGE) {
		target->dodge(player);
	} else if(result == ATTACK_PARRY) {
		int parryResult = target->parry(player);
		if(parryResult == 2) {
			// oh no, we died from a riposte...
			return(0);
		}
	} else if(result == ATTACK_FUMBLE) {
		player->statistics.fumble();
		player->printColor("^gYou FUMBLED your weapon.\n");
		broadcast(player->getSock(), player->getParent(), "^g%M fumbled %s weapon.", player, player->hisHer());

		if(weapon->flagIsSet(O_ENVENOMED)) {
			if(!player->immuneToPoison() &&
				!player->chkSave(POI, player, -5) && !induel(player, target->getAsPlayer())
			) {
				player->printColor("^G^#You poisoned yourself!!\n");
				broadcast(player->getSock(), player->getParent(), "%M poisoned %sself!!",
					player, player->himHer());

				if(weapon->getEffectStrength()) {
					dmg = (mrand(1,3) + (weapon->getEffectStrength()/10));
					player->hp.decrease(dmg);
					player->print("You take %d damage as the poison takes effect!\n", dmg);
				}

				weapon->clearFlag(O_ENVENOMED);

				if(player->hp.getCur() < 1) {
					n = 0;
					player->addObj(weapon);
					weapon = 0;
					player->computeAttackPower();
					player->die(POISON_PLAYER);
					return(0);
				}

				dur = standardPoisonDuration(weapon->getEffectDuration(), player->constitution.getCur());
				player->poison(player, weapon->getEffectStrength(), dur);

			} else {
				player->print("You almost poisoned yourself!\n");
				broadcast(player->getSock(), player->getParent(), "%M almost poisoned %sself!", player, player->himHer());
			}
		}

		n = 0;
		player->addObj(weapon);
		player->ready[WIELD-1] = 0;
		weapon = 0;
		player->computeAttackPower();
		player->computeAttackPower();
	} else {
		player->printColor("^RError!!! Unhandled attack result: %d\n", result);
	}

	return(0);
}

//*********************************************************************
//						checkPoison
//*********************************************************************
// This will check for envenom poisoning against the target with the given weapon

int Player::checkPoison(Creature* target, Object* weapon) {
	if(!weapon)
		return(0);

	int dmg = 0;
	int level = (int)getSkillLevel("backstab");
	int poisonchance = 50+((level - (target->getLevel()-1))*10);
	poisonchance -= (2*bonus(target->constitution.getCur()));

	if(target->mFlagIsSet(M_WILL_POISON))
		poisonchance /= 2;

	poisonchance = MAX(0,MIN(poisonchance, 80));

	// Can't poison them if they're immune to poison
	if(target->immuneToPoison())
		poisonchance = 0;

	// No poisoning people if they're in a duel
	if(induel(this, target->getAsPlayer()))
		poisonchance = 0;

	// If they're already poisoned, don't poison them again
	if(target && target->isPoisoned())
		poisonchance = 0;

	// Less of a chance to poison a perm
	if(!target && target->flagIsSet(M_PERMENANT_MONSTER))
		poisonchance = MIN(poisonchance, 10);

	// We can always poison someone if we're a ct
	if(isCt())
		poisonchance = 101;

	if(mrand(1,100) <= poisonchance && !target->chkSave(POI, this, -1)) {
		// 75% chance to use up the poison
		if(mrand (1,100) > 25)
			weapon->clearFlag(O_ENVENOMED);

		printColor("^gYou poisoned %N!\n", target);
		target->printColor( "^g^#%M poisoned you!!\n", this);
		broadcast(getSock(), target->getSock(), target->getRoomParent(), "%M poisoned %N!", this, target);

		unsigned int dur = standardPoisonDuration(weapon->getEffectDuration(), target->constitution.getCur());

		if(weapon->getEffectStrength()) {
			dmg = (mrand(1,3) + (weapon->getEffectStrength()/10));
			printColor("^gThe poison did ^G%d^g onset damage!\n", dmg);
			target->printColor("^gYou take ^G%d^g damage as the poison takes effect!\n", dmg);
		}

		target->poison(this, weapon->getEffectStrength(), dur);
	}
	return(dmg);
}

//*********************************************************************
//						cmdAmbush
//*********************************************************************
// This function allows rogues to ambush their opponents from
// hiding. It is similar to backstab, but somewhat weakened and
// more limited. -- TC

int cmdAmbush(Player* player, cmd* cmnd) {
	Player	*pCreature=0;
	Creature* creature=0;


	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("ambush")) {
		player->print("You don't know how to ambush people!\n");
		return(0);
	}

	if(player->isBlind()) {
		player->print("Ambush whom?\n");
		return(0);
	}

	if(!player->isCt()) {
		if(	!player->ready[WIELD-1] ||
			!player->ready[HELD-1] ||
			player->ready[HELD-1]->getType() != WEAPON
		) {
			player->print("You must have two weapons wielded to properly ambush.\n");
			return(0);
		}

		if(!player->flagIsSet(P_HIDDEN) && !player->isEffected("mist") && !player->isCt()) {
			player->print("How do you expect to ambush when you aren't hiding?\n");
			return(0);
		}

		if(!player->checkAttackTimer())
			return(0);

		if(player->checkHeavyRestrict("ambush"))
			return(0);
	}


    if(!(creature = player->findVictim(cmnd, 1, true, false, "Ambush whom?\n", "You don't see that here.\n")))
		return(0);

    if(creature)
		pCreature = creature->getAsPlayer();

	if(!pCreature && creature->getAsMonster()->isEnemy(player) && !player->isCt()) {
		player->print("Not while you're already fighting %s.\n",
		      creature->himHer());
		return(0);
	}

	if(!player->canAttack(creature))
		return(0);

	player->smashInvis();

	if(creature->isMonster()) {
		creature->getAsMonster()->addEnemy(player);
	}

	player->updateAttackTimer();

	if(player->dexterity.getCur() > 180)
		player->modifyAttackDelay(-10);

	player->print("You ambush %N!\n", creature);
	broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M ambushes %N!", player, creature);
	creature->print("%M ambushes you!\n", player);

	player->unhide();

	player->attackCreature(creature, ATTACK_AMBUSH);

	return(0);
}



//*********************************************************************
//						cmdPickLock
//*********************************************************************
// This function is called when a thief or assassin attempts to pick a
// lock.  If the lock is pickable, there is a chance (depending on the
// player's level) that the lock will be picked.

int cmdPickLock(Player* player, cmd* cmnd) {
	Exit	*exit=0;
	long	i=0, t=0;
	int		chance=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("pick")) {
		player->print("You don't know how to pick locks.\n");
		return(0);
	}

	if(!player->isCt()) {
		if(player->flagIsSet(P_SITTING)) {
			player->print("You have to stand up first.\n");
			return(0);
		}
		if(player->isBlind()) {
			player->printColor("^CHow can you pick a lock when blind?\n");
			return(0);
		}
	}


	if(cmnd->num < 2) {
		player->print("Pick what?\n");
		return(0);
	}
	exit = findExit(player, cmnd);

	if(!exit) {
		player->print("You don't see that here.\n");
		return(0);
	}


	const Monster* guard = player->getRoomParent()->getGuardingExit(exit, player);
	if(guard && !player->checkStaff("%M %s.\n", guard, guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you pick it" : "won't let you pick it"))
		return(0);

	if(!exit->flagIsSet(X_LOCKED)) {
		player->print("It's not locked.\n");
		return(0);
	}

	if(exit->isWall("wall-of-force")) {
		player->printColor("The %s^x is blocked by a wall of force.\n", exit->name);
		return(0);
	}
	// were they killed by exit effect damage?
	if(exit->doEffectDamage(player))
		return(0);
	
	player->unhide();

	if(!player->isCt()) {

		i = LT(player, LT_PICKLOCK);
		t = time(0);

		if(t < i) {
			player->pleaseWait(i-t);
			return(0);
		}

		player->lasttime[LT_PICKLOCK].ltime = t;
		player->lasttime[LT_PICKLOCK].interval = (player->getClass() == ROGUE ? 20 : 10);

	}
	int level = (int)player->getSkillLevel("pick");
	if(player->getClass() == THIEF)
		chance = 10*(level - exit->getLevel()) + (2*bonus((int)player->dexterity.getCur()));
	else if(player->getSecondClass() == THIEF && (player->getClass() == MAGE || player->getClass() == FIGHTER) )
		chance = 5*(level - exit->getLevel()) + (2*bonus((int)player->dexterity.getCur()));
	else if(player->getClass() == ROGUE || (player->getClass() == THIEF && player->getSecondClass() == MAGE))
		chance = 7*(level - exit->getLevel()) + (2*bonus((int)player->dexterity.getCur()));
	else
		chance = 2*(level - exit->getLevel()) + (2*bonus((int)player->dexterity.getCur()));


	if(exit->flagIsSet(X_UNPICKABLE))
		chance = 0;

	if((exit->getLevel() > level) && !player->isCt()) {
		player->print("The lock's mechanism is currently beyond your experience.\n");
		chance = 0;
	}

	chance = MAX(0, chance);

	if(player->isCt())
		chance = 101;

	broadcast(player->getSock(), player->getParent(), "%M attempts to pick the %s^x.", player, exit->name);

	if(mrand(1,100) <= chance) {
		log_immort(false, player, "%s picked the %s in room %s.\n", player->name, exit->name,
			player->getRoomParent()->fullName().c_str());

		player->print("You successfully picked the lock.\n");
		player->checkImprove("pick", true);
		exit->clearFlag(X_LOCKED);
		broadcast(player->getSock(), player->getParent(), "%s succeeded.", player->upHeShe());

		Hooks::run(player, "succeedPickExit", exit, "succeedPickByCreature");

	} else {
		player->print("You failed.\n");
		player->checkImprove("pick", false);

		Hooks::run(player, "failPickExit", exit, "failPickByCreature");
	}

	return(0);
}


//*********************************************************************
//						cmdPeek
//*********************************************************************
// This function allows a thief to peek at the inventory of
// another player.  If successful, they will be able to see it and
// another roll is made to see if they get caught.

int cmdPeek(Player* player, cmd* cmnd) {
	Creature* creature=0;
	Player	*pCreature=0;
	Monster* mCreature=0;
	bstring str = "";
	long	i=0, t=0;
	int		chance=0, goldchance=0, ok=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(!player->isStaff() && !player->knowsSkill("peek")) {
		player->print("You don't know how to peek at other people's inventories.\n");
		return(0);
	}

	if(cmnd->num < 2) {
		player->print("Peek at who?\n");
		return(0);
	}

	int level = (int)player->getSkillLevel("peek");

	if(player->isBlind()) {
		player->printColor("^CYou can't do that! You're blind!\n");
		return(0);
	}

	creature = player->getParent()->findCreature(player, cmnd);
	if(!creature || creature == player) {
		player->print("That person is not here.\n");
		return(0);
	}
	mCreature = creature->getAsMonster();
	pCreature = creature->getAsPlayer();

	if(player->getClass() == BUILDER) {
		if(pCreature) {
			player->print("You cannot peek players.\n");
			return(0);
		}
	if(!player->canBuildMonsters() && !player->canBuildObjects())
			return(cmdNoAuth(player));
		if(!player->checkBuilder(player->getUniqueRoomParent())) {
			player->print("Error: Room number not inside any of your alotted ranges.\n");
			return(0);
		}
		if(mCreature && mCreature->info.id && !player->checkBuilder(mCreature->info)) {
			player->print("Error: Monster not inside any of your alotted ranges.\n");
			return(0);
		}
	}

	if(!pCreature && player->isStaff())
		return(dmMobInventory(player, cmnd));


	if(pCreature && pCreature->isEffected("mist")) {
		player->print("You cannot peek at the inventory of a mist.\n");
		return(0);
	}

	if(player->checkHeavyRestrict("peek"))
		return(0);

	i = LT(player, LT_PEEK);
	t = time(0);

	if(i > t && !player->isStaff()) {
		player->pleaseWait(i-t);
		return(0);
	}

	player->lasttime[LT_PEEK].ltime = t;
	player->lasttime[LT_PEEK].interval = 5;

	if(!player->isCt() && creature->isStaff()) {
		player->print("You failed.\n");
		return(0);
	}

	if(!pCreature && (creature->flagIsSet(M_CANT_BE_STOLEN_FROM) || creature->flagIsSet(M_TRADES) || creature->flagIsSet(M_CAN_PURCHASE_FROM)) && !player->isDm()) {
		player->print("%M manages to keep %s inventory hidden from you.\n", creature, creature->hisHer());
		return(0);
	}

	if(cmnd->num > 2 && pCreature && (player->getClass() == THIEF || player->isCt())) {
		peek_bag(player, pCreature, cmnd, 0);
		return(0);
	}

	if(!ok && (player->getClass() == THIEF || player->isStaff()))
		chance = (25 + level*10)-(creature->getLevel()*5);
	else
		chance = (level*10)-(creature->getLevel()*5);

	if(chance<0)
		chance=0;

	if(creature->getLevel() >= level + 6)
		chance = 0;

	if(player->isStaff())
		chance=100;
	if(mrand(1,100) > chance) {
		player->print("You failed.\n");
		if(mrand(1,50) <= 50)
			player->unhide();
		player->checkImprove("peek", false);
		return(0);
	}

	chance = MIN(90, (player->getClass() == ASSASSIN ? (level*4):(15 + level*5)));

	if(mrand(1,100) > chance && !player->isStaff()) {
		creature->print("%M peeked at your inventory.\n", player);
		broadcast(player->getSock(), creature->getSock(), player->getRoomParent(),
			"%M peeked at %N's inventory.", player, creature);
		player->print("%s noticed!\n", creature->upHeShe());
	} else {
		player->print("%s's oblivious to your rummaging.\n",
		      creature->upHeShe());
	}
	player->checkImprove("peek", true);

	str = listObjects(player, creature->first_obj, player->isStaff());
	if(str != "")
		player->printColor("%s is carrying: %s.\n", creature->upHeShe(), str.c_str());
	else
		player->print("%s isn't holding anything.\n", creature->upHeShe());


	goldchance = (5+(level*5)) - creature->getLevel();
	goldchance = MIN(MAX(1,goldchance),90);

	if(player->isCt())
		goldchance = 101;

	if(mrand(1,100) <= goldchance && !pCreature && !creature->flagIsSet(M_PERMENANT_MONSTER))
		player->print("%s has %ld gold coins.\n", creature->upHeShe(), creature->coins[GOLD]);

	return(0);
}


//*********************************************************************
//						peek_bag
//*********************************************************************

int peek_bag(Player* player, Player* target, cmd* cmnd, int inv) {
	Object	*container=0;
	bstring str = "";
	int		chance=0;

	if(!player->isStaff()) {
		int level = (int)player->getSkillLevel("peek");

		if(level < 10) {
			player->print("You are are not experienced enough at peeking to do that.\n");
			return(0);
		}

		chance = (5 + level*10)-(target->getLevel()*5);
		if(chance<0)
			chance=0;

		if(target->getLevel() >= level + 6)
			chance = 0;

		if(mrand(1,100) > chance) {
			player->print("You failed.\n");
			player->checkImprove("peek", false);
			return(0);
		}
	}

	container = target->findObject(player, cmnd, 2);

	if(!container) {
		player->print("%s doesn't have that.\n", target->upHeShe());
		return(0);
	}

	if(container->getType() != CONTAINER) {
		player->print("That isn't a container.\n");
		return(0);
	}

	if(!inv) {
		if(mrand(1,100) > chance && !player->isStaff()) {

			player->print("You manage to peek inside %N's %s.\n", target, container->name);
			target->print("%M managed to peek in your %s!\n", player, container->name);
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M peeked at %N's inventory.",
				player, target);
			player->print("%s noticed!\n", target->upHeShe());

		} else {
			player->print("You manage to peek inside %N's %s.\n", target, container->name);
			player->print("%s's oblivious to your rummaging.\n", target->upHeShe());
		}
		player->checkImprove("peek", true);
	}

	if(container->getType() == CONTAINER) {
		str = listObjects(player, container->first_obj, false);
		if(str != "")
			player->printColor("It contains: %s.\n", str.c_str());
		else
			player->print("It is empty.\n");
	}

	return(0);
}

