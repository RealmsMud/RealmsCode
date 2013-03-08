/*
 * translocation.cpp
 *	 Translocation spells
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
#include "move.h"
#include "unique.h"
#include "commands.h"


//*********************************************************************
//						splTransport
//*********************************************************************
// This spell allows the caster to magically transport an object to
// another player.

int splTransport(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player* pPlayer = player->getAsPlayer();
	Creature* target=0;
	Object	*object=0;
	int		cost;

	if(pPlayer->getClass() == BUILDER) {
		pPlayer->print("You cannot cast this spell.\n");
		return(0);
	}

	if(!pPlayer->spellIsKnown(S_TRANSPORT) && spellData->how == CAST) {
		pPlayer->print("You don't know that spell.\n");
		return(0);
	}

	if(pPlayer->getClass() != MAGE && pPlayer->getClass() != LICH && !pPlayer->isCt() && spellData->how == CAST) {
		pPlayer->print("Only mages and liches may cast that spell.\n");
		return(0);
	}

	if(pPlayer->getLevel() < 5) {
		pPlayer->print("You are not high enough level to cast that yet.\n");
		return(0);
	}

	if(cmnd->num < 4) {
		pPlayer->print("Transport what to whom?\n");
		return(0);
	}

	lowercize(cmnd->str[3], 1);
	target = gServer->findPlayer(cmnd->str[3]);

	if(!target || !pPlayer->canSee(target)) {
		pPlayer->print("That player is not logged on.\n");
		return(0);
	}


	if(target->isEffected("petrification")) {
		pPlayer->print("%M cannot receive items right now.\n", target);
		return(0);
	}

	object = pPlayer->findObject(pPlayer, cmnd, 2);

	if(!object) {
		pPlayer->print("You don't have that object.\n");
		return(0);
	}


	if(Move::tooFarAway(pPlayer, target, "transport objects to"))
		return(0);


	// most basic checks to see if they can give item away or not
	if(!canGiveTransport(pPlayer, target, object, false))
		return(0);


	cost = 5 + bonus((int) pPlayer->intelligence.getCur()) + ((int)pPlayer->getLevel() - 5) * 2;
	if(object->getActualWeight() > cost) {
		pPlayer->printColor("%O is too heavy to transport at your current level.\n", object);
		return(0);
	}

	cost = 8 + (object->getActualWeight()) / 4;

	if(spellData->how == CAST) {
		if(!pPlayer->checkMp(cost))
			return(0);
		pPlayer->subMp(cost);
	}

	if(pPlayer->spellFail( spellData->how))
		return(0);

	object->clearFlag(O_JUST_BOUGHT);

	if(!pPlayer->flagIsSet(P_DM_INVIS)) {
		broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%M transports an object to someone.", pPlayer);
		broadcast(isCt, pPlayer->getSock(), pPlayer->getRoomParent(), "*DM* %M transports %1P to %N.", pPlayer, object, target);
	}

	pPlayer->printColor("You concentrate intensely on %P as it disappears.\n", object);
	pPlayer->printColor("You sucessfully transported %1P to %N.\n", object, target);

	if(!pPlayer->isDm())
		log_immort(true, pPlayer, "%s transports a %s to %s in room %s.\n", pPlayer->getCName(), object->getCName(),
			target->getCName(), target->getRoomParent()->fullName().c_str());

	if(!pPlayer->flagIsSet(P_DM_INVIS) && (!target->isEffected("incognito") || pPlayer->inSameRoom(target))) {
		target->wake("You awaken suddenly!");
		target->printColor("%M magically sends you %1P.\n", pPlayer, object);
	}

	pPlayer->delObj(object);
	Limited::transferOwner(pPlayer, target->getAsPlayer(), object);
	target->addObj(object);

	return(1);
}

//*********************************************************************
//						hinderedByDimensionalAnchor
//*********************************************************************

bool hinderedByDimensionalAnchor(int splno) {
	return(	splno == S_TELEPORT ||
			splno == S_SUMMON ||
			splno == S_TRACK ||
			splno == S_WORD_OF_RECALL ||
			splno == S_BLINK ||
			splno == S_ETHREAL_TRAVEL );
}


//*********************************************************************
//						splDimensionalAnchor
//*********************************************************************
// This spell can either create a dimensional anchor in a room or make
// them resistant to magical movement spells

int splDimensionalAnchor(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* creature=0;
	Player*	target=0, *pPlayer = player->getAsPlayer();
	int		i=0, num=-1;
	bool	destroy=false;


	if(player->getClass() == BUILDER) {
		player->print("You cannot cast this spell.\n");
		return(0);
	}

	if(player->getClass() != MAGE && player->getClass() != LICH && !player->isCt()) {
		player->print("Your class is not arcanely attuned enough to cast that spell.\n");
		return(0);
	}

	if(player->getLevel() < 16) {
		player->print("You are not yet powerful enough to cast that spell.\n");
		return(0);
	}


	//
	// cast dimesional-anchor effect
	//
	if(cmnd->num <= 3 || spellData->how != CAST) {

		if(spellData->how == CAST && !player->checkMp(30))
			return(0);

		if(pPlayer->spellFail( spellData->how)) {
			if(spellData->how == CAST)
				pPlayer->subMp(30);
			return(0);
		}

		// Cast anchor on self
		if(cmnd->num == 2) {
			target = pPlayer;
			if(!target) {
				player->print("You were unable to cast the spell.\n");
				return(0);
			}
			if(spellData->how == POTION)
				player->print("You feel stable.\n");

		// Cast anchor on another pPlayer
		} else {
			if(player->noPotion( spellData))
				return(0);

			cmnd->str[2][0] = up(cmnd->str[2][0]);
			creature = pPlayer->getParent()->findCreature(pPlayer, cmnd->str[2], cmnd->val[2], false);
			if(creature)
				target = creature->getAsPlayer();
			if(!target) {
				player->print("That person is not here.\n");
				return(0);
			}


			if(target->inCombat(false)) {
				player->print("Not in the middle of combat.\n");
				return(0);
			}
			if(target->flagIsSet(P_NO_SUMMON) && !player->canAttack(target))
				return(0);
		}

		if(spellData->how == CAST)
			player->subMp(30);

		if(target != pPlayer) {
			if(target->flagIsSet(P_NO_SUMMON) && !player->isStaff()) {
				if(target->isStaff() || target->chkSave(SPL, player, 0)) {
					player->printColor("^y%M resisted your anchor spell!\n", target);
					target->printColor("^y%M tried to place a dimensional-anchor on you!^w\nIf you wish to be anchored, type \"set summon\".^x\n", player);
					return(1);
				}
			}
		}

		return(splGeneric(player, cmnd, spellData, "an", "anchor", "anchor"));
	}
	//
	// end dimensional-anchor effect
	//

	if(!pPlayer)
		return(0);


	if( !pPlayer->isCt() &&
		pPlayer->inUniqueRoom() &&
		(
			!pPlayer->getUniqueRoomParent()->canPortHere(pPlayer) ||
			pPlayer->getUniqueRoomParent()->flagIsSet(R_ELEC_BONUS)
		)
	) {
		player->print("Dimensional anchors do not work here.\n");
		return(0);
	}

	if(strlen(cmnd->str[2]) > 18) {
		player->print("Anchor name too long. Must be less than 19 characters.\n");
		return(0);
	}

	if(strlen(cmnd->str[2]) < 4) {
		player->print("Anchor name too short. Must be at least 4 characters.\n");
		return(0);
	}

	// they want to nuke their anchor
	if(cmnd->num != 4) {
		player->print("Syntax: cast anchor (alias name|target) [create|destroy]\n");
		return(0);
	} else if(!strcasecmp(cmnd->str[3], "destroy"))
		destroy = true;
	else if(!strcasecmp(cmnd->str[3], "create"))
		destroy = false;
	else {
		player->print("Syntax: cast anchor (alias name|target) [create|destroy]\n");
		return(0);
	}


	if(spellData->how == CAST && !pPlayer->checkMp(destroy ? 10 : 50))
		return(0);

	if(pPlayer->spellFail( spellData->how)) {
		if(spellData->how == CAST)
			pPlayer->subMp(destroy ? 10 : 50);
		return(0);
	}



	for(i=0; i<MAX_DIMEN_ANCHORS; i++) {
		if(!destroy) {
			if(pPlayer->hasAnchor(i)) {
				if(pPlayer->isAnchor(i, pPlayer->getRoomParent())) {
					player->print("You already have an anchor to this room.\n");
					return(0);
				}
				if(!strcmp(cmnd->str[2], pPlayer->getAnchorAlias(i).c_str())) {
					player->print("You already have an anchor named that.\n");
					return(0);
				}
			} else if(num < 0) {
				num = i;
			}
		// this is the anchor we want to destroy
		} else if(pPlayer->hasAnchor(i) && !strcmp(cmnd->str[2], pPlayer->getAnchorAlias(i).c_str())) {
			player->print("Anchor \"%s\" has been purged.\n", pPlayer->getAnchorAlias(i).c_str());

			pPlayer->delAnchor(i);

			if(spellData->how == CAST)
				pPlayer->subMp(10);
			return(0);
		}
	}

	if(destroy) {
		player->print("Anchor not found!\n");
		return(0);
	}

	if(num < 0) {
		player->print("You may only have %d anchors at a time.\nYou must destroy an anchor before creating another.\n", MAX_DIMEN_ANCHORS);
		return(0);
	}

	pPlayer->setAnchor(num, cmnd->str[2]);

	player->print("Anchor spell cast.\nDimensional anchor \"%s\" created.\n", pPlayer->getAnchorAlias(num).c_str());
	broadcast(player->getSock(), pPlayer->getRoomParent(),
			"%M forms a dimensional anchor in the room.", pPlayer);
	broadcast(isCt, "^y%M forms a dimensional anchor in room %s.",
			pPlayer, pPlayer->getRoomParent()->fullName().c_str());

	if(spellData->how == CAST)
		pPlayer->subMp(50);

	return(1);
}

//*********************************************************************
//						splPortal
//*********************************************************************
// This function creates a portal to the caster's anchor

int splPortal(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player*	pPlayer = player->getAsPlayer();
	const Anchor* destination=0;
	BaseRoom* newRoom=0;
	UniqueRoom*	uRoom=0;
	AreaRoom* aRoom=0;
	int		i=0;

	if(!pPlayer)
		return(0);
	if(spellData->how != CAST) {
		pPlayer->print("Nothing happens.\n");
		return(0);
	}

	if(player->flagIsSet(P_PORTAL)) {
		player->print("You already have an open portal.\n");
		return(0);
	}

	if(!player->isCt()) {
		if(player->getClass() != MAGE && player->getClass() != LICH) {
			pPlayer->print("Your class is unable to cast that spell.\n");
			return(0);
		}
		if(player->getLevel() < 31) {
			pPlayer->print("You are not strong enough to cast this spell.\n");
			return(0);
		}
		if(	player->getRoomParent()->flagIsSet(R_NO_CAST_TELEPORT) ||
			player->getRoomParent()->flagIsSet(R_LIMBO)
		) {
			player->print("The spell fizzles.\n");
			player->print("You cannot cast that spell in this room.\n");
			return(0);
		}
	}

	if(cmnd->num > 2) {
		cmnd->str[2][0] = low(cmnd->str[2][0]);
		for(i=0; i<MAX_DIMEN_ANCHORS; i++) {
			if(pPlayer->hasAnchor(i) && !strcmp(cmnd->str[2], pPlayer->getAnchorAlias(i).c_str())) {
				destination = pPlayer->getAnchor(i);
				break;
			}
		}
	}
	if(!destination) {
		player->print("Create a portal to what dimensional anchor?\n");
		return(0);
	}

	if(destination->is(pPlayer->getRoomParent())) {
		player->printColor("You are already there!\n^yYour spell fails.\n");
		return(0);
	}

	if(destination->getMapMarker()) {
		Area *area = gConfig->getArea(destination->getMapMarker()->getArea());
		if(!area) {
			player->printColor("Massive dimensional interference detected.\n^yYour spell fails.\n");
			return(0);
		}
		aRoom = area->loadRoom(pPlayer, destination->getMapMarker(), false);
		if(!aRoom) {
			player->printColor("Dimensional interference at destination detected.\n^yYour spell fails.\n");
			return(0);
		}
		newRoom = aRoom;
	} else {
		if(	!loadRoom(destination->getRoom(), &uRoom) ||
			!pPlayer->canEnter(uRoom)
		) {
			player->printColor("Dimensional interference at destination detected.\n^yYour spell fails.\n");
			return(0);
		}
		newRoom = uRoom;
	}

	if(Move::tooFarAway(pPlayer, newRoom))
		return(0);

	if(!player->isCt() &&
		!dec_daily(&pPlayer->daily[DL_TELEP])
	) {
		pPlayer->print("You are too weak to create another portal again today.\n");
		return(0);
	}

	player->print("You cast a portal spell.\n");
	broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%M casts a portal spell.", pPlayer);

	player->setFlag(P_PORTAL);
	Move::createPortal(pPlayer->getRoomParent(), newRoom, pPlayer);
	return(1);
}

//*********************************************************************
//						createPortal
//*********************************************************************

void Move::createPortal(BaseRoom* room, BaseRoom* target, const Player* player, bool initial) {
	// start with a unique exit name so we don't override any existing
	// portal exits in the room
	UniqueRoom *uRoom = target->getAsUniqueRoom();
	if(uRoom) {
		link_rom(room, uRoom->info, "uniqueportalname");
	} else {
		AreaRoom* aRoom = target->getAsAreaRoom();
		link_rom(room, &aRoom->mapmarker, "uniqueportalname");
	}

	// the new portal is always the last exit in the room
	Exit* ext = room->exits.back();
	ext->setName("^Yportal");
	ext->setLevel(7);
	ext->setFlag(X_PORTAL);
	ext->setFlag(X_NO_WANDER);
	ext->setFlag(X_CAN_LOOK);
	ext->setEnter("You step through the portal and find yourself in another location.");
	ext->setPassPhrase(player->getName());
	ext->setKey(MAX(1, ((int)player->getLevel() - 28) / 2));
	ext->setDescription("You see a shimmering portal of mystical origin.");

	broadcast(0, room, "^YA dimensional portal forms in the room!");

	if(initial)
		createPortal(target, room, player, false);
}

//*********************************************************************
//						usePortal
//*********************************************************************
// return true if the portal is used up

bool Move::usePortal(Creature* player, BaseRoom* room, Exit* exit, bool initial) {
	if(!exit->flagIsSet(X_PORTAL))
		return(false);

	exit->setKey(exit->getKey() - 1);

	if(exit->getKey() < 1)
		return(true);

	if(initial) {
		BaseRoom* target = exit->target.loadRoom();
		Exit* toUse = 0;
		for(Exit* ext : target->exits) {
			if(ext->flagIsSet(X_PORTAL) && ext->getPassPhrase() == exit->getPassPhrase()) {
				toUse = ext;
				break;
			}
		}

		if(toUse)
			return(Move::usePortal(player, target, toUse, false));
	}

	return(false);
}

//*********************************************************************
//						deletePortal
//*********************************************************************

bool Move::deletePortal(BaseRoom* room, Exit* exit, const Creature* leader, std::list<Creature*> *followers, bool initial) {
	return(Move::deletePortal(room, exit->getPassPhrase(), leader, followers, initial));
}

bool Move::deletePortal(BaseRoom* room, bstring name, const Creature* leader, std::list<Creature*> *followers, bool initial) {
	ExitList::iterator xit;
	for(xit = room->exits.begin() ; xit != room->exits.end() ; xit++) {
		Exit* ext = *xit;

		if(ext->flagIsSet(X_PORTAL) && ext->getPassPhrase() == name) {
			if(initial) {
				if(leader && leader->isPlayer())
					leader->printColor("The %s^x collapses into nothingness.\n", ext->getCName());
				if(followers) {
					std::list<Creature*>::const_iterator it;
					for(it = followers->begin(); it != followers->end(); it++) {
						if(!(*it)->isPlayer())
							continue;
						(*it)->printColor("The %s^x collapses into nothingness.\n", ext->getCName());
					}
				}
			}

			broadcast(0, room, "The %s^x collapses into nothingness.", ext->getCName());

			if(initial) {
				BaseRoom* target = ext->target.loadRoom();
				Move::deletePortal(target, name, 0, 0, false);
			}
			room->exits.erase(xit);
			delete ext;

			Player* owner = gServer->findPlayer(name.c_str());
			if(owner)
				owner->clearFlag(P_PORTAL);
			return(true);
		}
	}
	return(false);
}

//*********************************************************************
//						splTeleport
//*********************************************************************
// This function allows a player to teleport themself or another player
// to another room randomly.

int splTeleport(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* target=0;
	Player*	pPlayer = player->getAsPlayer(), *pTarget=0;
	Monster* mTarget=0;
	UniqueRoom	*uRoom=0;
	BaseRoom *newRoom=0;
	AreaRoom *aRoom=0;
	int		i=0;
	const Anchor* destination=0;
	char	dest[18];

	strcpy(dest, "");

	if(spellData->how == CAST && !player->checkMp(50))
		return(0);

	if(spellData->how == CAST) {
		if(	!player->isCt() &&
			player->getClass() != MAGE &&
			player->getClass() != LICH &&
			player->getClass() != BARD
		) {
			player->print("Your class is unable to cast that spell.\n");
			return(0);
		}
	}

	if(!player->isStaff()) {
		if(player->getLevel() < 13) {
			player->print("The spell fizzles.\n");
			player->print("You are not high enough level to cast this spell.\n");
			return(0);
		}
		if(	player->getRoomParent()->flagIsSet(R_NO_CAST_TELEPORT) ||
			player->getRoomParent()->flagIsSet(R_LIMBO)
		) {
			player->print("The spell fizzles.\n");
			player->print("You cannot cast that spell in this room.\n");
			if(spellData->how == CAST)
				player->subMp(50);
			return(0);
		}
	}


	// Only pPlayers may cast teleport on self
	if(cmnd->num == 2) {

		// Porting self only uses a port 1/3 of the time. (Unless they have no ports left!
		if(	pPlayer && !pPlayer->isCt() &&
			(pPlayer->daily[DL_TELEP].cur == 0 ||
			(mrand(1,100) <= 33 &&
			!dec_daily(&pPlayer->daily[DL_TELEP])))
		) {
			pPlayer->print("You are too weak to teleport again today.\n");
			return(0);
		}


		if(spellData->how == CAST) {
			if(!player->checkMp(30))
				return(0);
			player->subMp(30);
		}

		if(spellData->how == CAST || spellData->how == SCROLL)
			player->print("You cast a teleport spell on yourself.\n");

		if(spellData->how == CAST && !player->isStaff() && player->checkDimensionalAnchor()) {
			player->printColor("^yYour dimensional-anchor causes your spell to fizzle!^w\n");
			return(1);
		}

		broadcast(player->getSock(), player->getParent(), "%M disappears.", player);

		if(spellData->how != CAST && spellData->how != SCROLL)
			player->print("You become disoriented and find yourself in another place.\n");


		newRoom = player->teleportWhere();

		player->deleteFromRoom();
		if(pPlayer) {
			pPlayer->addToRoom(newRoom);
			pPlayer->doPetFollow();
		} else {
			player->getAsMonster()->addToRoom(newRoom);
			broadcast(isCt, "^y%s just teleported %sself to %s.", player->getCName(), player->himHer(),
				newRoom->fullName().c_str());
		}
		return(1);

	// Cast teleport on another person
	} else {
		if(player->noPotion( spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);


		if(pPlayer && !target) {
			cmnd->str[2][0] = low(cmnd->str[2][0]);
			for(i=0; i<MAX_DIMEN_ANCHORS; i++) {
				if(pPlayer->hasAnchor(i) && !strcmp(cmnd->str[2], pPlayer->getAnchorAlias(i).c_str())) {
					destination = pPlayer->getAnchor(i);
					break;
				}
			}
			if(!destination) {
				player->print("That player is not here.\n");
				return(0);
			}
		}


		if(pPlayer && destination) {
			if(spellData->how != CAST) {
				player->print("Nothing happens.\n");
				return(0);
			}

			if(destination->is(player->getRoomParent())) {
				player->printColor("You are already there!\n^yYour spell fails.\n");
				return(0);
			}

			if(destination->getMapMarker()) {
				Area *area = gConfig->getArea(destination->getMapMarker()->getArea());
				if(!area) {
					player->printColor("Massive dimensional interference detected.\n^yYour spell fails.\n");
					return(0);
				}
				aRoom = area->loadRoom(pPlayer, destination->getMapMarker(), false);
				if(!aRoom) {
					player->printColor("Dimensional interference at destination detected.\n^yYour spell fails.^x\n");
					return(0);
				}
				newRoom = aRoom;
			} else {
				if(	!loadRoom(destination->getRoom(), &uRoom) ||
					!pPlayer->canEnter(uRoom)
				) {
					player->printColor("Dimensional interference at destination detected.\n^yYour spell fails.^x\n");
					return(0);
				}
				newRoom = uRoom;
			}


			if(Move::tooFarAway(pPlayer, newRoom))
				return(0);


			if(!pPlayer->isCt() &&
				!dec_daily(&pPlayer->daily[DL_TELEP])
			) {
				player->print("You are too weak to teleport again today.\n");
				return(0);
			}

			// Directed teleports use more MP.
			if(spellData->how == CAST) {
				if(!pPlayer->checkMp(60))
					return(0);
				pPlayer->subMp(60);
			}

			player->print("You cast a teleport spell on yourself.\n");

			broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%M casts a teleport spell on %sself.",
				pPlayer, pPlayer->himHer());

			if(spellData->how == CAST && !player->isStaff() && player->checkDimensionalAnchor()) {
				player->printColor("^yYour dimensional-anchor causes your spell to fizzle!^w\n");
				return(1);
			}

			player->print("You teleport yourself to dimensional anchor \"%s\".\n",
				destination->getAlias().c_str());

			broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%s vanishes in a puff of swirling smoke.",
				pPlayer->upHeShe());
			if(!pPlayer->flagIsSet(P_DM_INVIS))
				broadcast(NULL, newRoom, "POOF!");


			pPlayer->deleteFromRoom();
			pPlayer->addToRoom(newRoom);
			pPlayer->doPetFollow();

			return(2);
		}
		// end anchor-teleport, on to targetting another player


		pTarget = target->getAsPlayer();
		mTarget = target->getAsMonster();


		if(mTarget) {
			if(!player->canAttack(mTarget))
				return(0);
			if(	mTarget->flagIsSet(M_PASSIVE_EXIT_GUARD) ||
				mTarget->flagIsSet(M_PERMENANT_MONSTER)
			) {
				player->print("Nothing happens.\n");
				mTarget->addEnemy(player);
				return(0);
			}
		}


		if(!player->isCt()) {
			if(pTarget && player->isMonster()) {
				if(player == target) {
					player->print("You cannot teleport yourself.\n");
					return(0);
				}
				if(pTarget->isStaff()) {
					player->print("They are too powerful to be affected by your magic.\n");
					return(0);
				}
				if(pTarget->flagIsSet(P_LINKDEAD)) {
					player->print("You cannot cast that spell on %N right now.\n", target);
					return(0);
				}
				if(pTarget->inCombat()) {
					player->print("Not while %N is in combat.\n", pTarget);
					return(0);
				}
				if(spellData->how == CAST && pTarget->checkDimensionalAnchor()) {
					player->printColor("^y%M's dimensional-anchor causes your spell to fizzle!^w\n", target);
					pTarget->printColor("^yYour dimensional-anchor protects you from %N's teleport spell!^w\n", pPlayer);
					return(1);
				}
			}

			if(target->chkSave(SPL, player, 0) || target->isCt()) {
				player->printColor("^yYour spell fizzled.\n");
				target->print("%M failed to teleport you.\n", player);
				return(1);
			}
			if(pPlayer && !dec_daily(&pPlayer->daily[DL_TELEP])) {
				player->print("You are too weak to teleport again today.\n");
				return(1);
			}
			if(target->isEffected("resist-magic") && (mrand(1, 60) + ((int)player->getLevel() - (int)target->getLevel()) * 10) > 80) {
				player->print("Your magic is too weak to teleport %N.\n", target);
				if(target->getSock())
					target->print("%M tried to teleport you.\n", player);
				if(spellData->how == CAST)
					player->subMp(50);
				return(1);
			}
		}



		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("Teleport cast on %N.\n", target);
			target->print("%M casts a teleport spell on you.\n", player);

			if(!player->isDm())
				broadcast(isCt, "^y### %M(L%d) just teleported %N(L%d).", player, player->getLevel(), target, target->getLevel());

			logCast(player, target, "teleport");

			newRoom = player->teleportWhere();
			if(spellData->how == CAST)
				player->subMp(50);

			if(pTarget) {
				pTarget->deleteFromRoom();
				pTarget->addToRoom(newRoom);
				pTarget->doPetFollow();
			} else {
				mTarget->setFlag(M_WAS_PORTED);
				broadcast(NULL, player->getRoomParent(), "%M vanishes!", mTarget);
				// being ported pisses mobs off! haha
				mTarget->addEnemy(player);
				mTarget->deleteFromRoom();
				mTarget->addToRoom(newRoom);
				broadcast(NULL, mTarget->getRoomParent(), "%M was thrown from %N's dimensional rift!", mTarget, player);
			}

			logn("log.teleport", "%s(L%dH%dM%dR:%s) was just teleported to room %s by %s(L%d)\n",
				target->getCName(), target->getLevel(), target->hp.getCur(), target->mp.getCur(), player->getRoomParent()->fullName().c_str(),
				newRoom->fullName().c_str(), player->getCName(), player->getLevel());
			broadcast(isCt, "^y%M(L%dH%dM%dR:%s) was just teleported to room %s by %s(L%d)",
				target, target->getLevel(), target->hp.getCur(), target->mp.getCur(), player->getRoomParent()->fullName().c_str(),
				newRoom->fullName().c_str(), player->getCName(), player->getLevel());

			return(2);
		}
	}

	return(1);
}

//*********************************************************************
//						splEtherealTravel
//*********************************************************************
// This function allows a player to teleport themself or another player
// to the Ethereal Plane

int splEtherealTravel(Creature* player, cmd* cmnd, SpellData* spellData) {
	if(!player->isStaff()) {
		const CatRefInfo* cri = gConfig->getCatRefInfo(player->getRoomParent());
		const CatRefInfo* eth = gConfig->getCatRefInfo(gConfig->defaultArea);
		// can only cast ethereal-travel if in same teleportZone
		if(!cri || !eth || cri->getTeleportZone() != eth->getTeleportZone()) {
			player->print("You are currently unable to cast this spell.\n");
			return(0);
		}
		if(	player->getRoomParent()->flagIsSet(R_NO_CAST_TELEPORT) ||
			player->getRoomParent()->flagIsSet(R_LIMBO)
		) {
			player->print("The spell fizzles.\n");
			return(1);
		}
	}

	Player	*target=0, *pCaster = player->getAsPlayer();
	UniqueRoom	*new_rom=0;
	CatRef	cr;

	if(player->getClass() == BUILDER) {
		player->print("You cannot cast this spell.\n");
		return(0);
	}

	// Cast e-travel on self
	if(pCaster && cmnd->num == 2) {

		if(spellData->how == CAST && !pCaster->isStaff() && pCaster->checkDimensionalAnchor()) {
			player->printColor("^yYour dimensional-anchor causes your spell to fizzle!^w\n");
			return(1);
		}

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("Ethereal-travel spell cast.\n");
			player->print("You turn translucent and fade away.\n");
			player->print("You have been transported to the ethereal plane.\n");
			broadcast(pCaster->getSock(), pCaster->getRoomParent(), "%M casts ethereal-travel on %sself.",
				pCaster, pCaster->himHer());
		} else if(spellData->how == POTION) {
			player->print("You turn translucent and fade away.\n");
			player->print("You have been transported to the ethereal plane.\n");
			broadcast(pCaster->getSock(), pCaster->getRoomParent(), "%M turns translucent and fades away.", player);
		}


		cr = getEtherealTravelRoom();
		if(!loadRoom(cr, &new_rom)) {
			player->print("Spell failure.\n");
			return(0);
		}

		pCaster->courageous();
		pCaster->deleteFromRoom();
		pCaster->addToRoom(new_rom);
		pCaster->doPetFollow();
		return(1);

	// Cast e-travel on another player
	} else {
		if(player->noPotion( spellData))
			return(0);

		if(player->getLevel() < 13 && !player->isCt()) {
			player->print("You can't control the magic well enough to cast that on someone else yet.\n");
			return(0);
		}

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findPlayer(player, cmnd, 2);
		if(!target || !player->canSee(target)) {
			player->print("That person is not here.\n");
			return(0);
		}

		if(target->isStaff() && !player->isDm()) {
			player->print("They are too powerful to be affected by your magic.\n");
			return(0);
		}

		if(player->getRoomParent()->isPkSafe() && !player->isDm()) {
			player->print("A mystical force prevents you from casting that spell in this room.\n");
			return(0);
		}

		// nosummon flag for lawfuls
		if(	(pCaster || player->isPet()) &&
			!target->flagIsSet(P_CHAOTIC) &&
			target->flagIsSet(P_NO_SUMMON) && !target->flagIsSet(P_OUTLAW) &&
			!player->checkStaff("The spell fizzles.\n%M's summon flag is not set.\n", target)  )
		{
			target->print("%s tried to send you to the ethereal plane\nIf you wish to be sent, type \"set summon\".\n", player->getCName());
			return(0);
		}

		if(!player->isCt()) {
			if(!target->flagIsSet(P_NO_SUMMON) && target->chkSave(SPL, player, 0)) {
				player->printColor("^yYour spell fizzled.\n");
				target->print("%M failed to send you to the ethereal plane.\n", player);
				return(1);
			}
		}

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {

			if(spellData->how == CAST && !player->isStaff() && target->checkDimensionalAnchor()) {
				player->printColor("^y%M's dimensional-anchor causes your spell to fizzle!^w\n", target);
				target->printColor("^yYour dimensional-anchor protects you from %N's ethereal-travel spell!^w\n", player);
				return(1);
			}

			player->print("Ethereal travel spell cast on %N.\n", target);
			player->print("%N turns translucent and fades away.\n", target);

			target->print("%M casts an ethereal travel spell on you.\n", player);
			target->print("You turn translucent and fade away.\n");
			target->print("You have been transported to the ethereal plane.\n");

			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts ethereal-travel on %N.",
				player, target);

			logCast(player, target, "ethereal-travel");
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%N turns translucent and fades away.", target);

			cr = getEtherealTravelRoom();

			if(!loadRoom(cr, &new_rom)) {
				player->print("Spell failure.\n");
				return(0);
			}
			target->deleteFromRoom();
			target->addToRoom(new_rom);
			target->doPetFollow();
			return(2);
		}
	}
	return(1);
}

//*********************************************************************
//						splSummon
//*********************************************************************
// This function allows players to cast summon spells on anyone who is
// in the game, taking that person to your current room.

int splSummon(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*target=0;

	if(player->getClass() == BUILDER) {
		player->print("You cannot cast this spell.\n");
		return(0);
	}

	if(!player->isCt()) {
		// wizard/lich/cleric can cast at level 7
		// everyone else can cast at 10
		if(	player->getClass() == MAGE ||
			player->getClass() == LICH ||
			player->getClass() == CLERIC
		) {
			if(player->getLevel() < 7) {
				player->print("You must atleast level 7 to cast this spell.\n");
				return(0);
			}
		} else if(player->getLevel() < 10) {
			player->print("You must atleast level 10 to cast this spell.\n");
			return(0);
		}
	}

	if(cmnd->num == 2) {
		player->print("You may not use that on yourself.\n");
		return(0);
	} else {
		if(player->noPotion( spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = gServer->findPlayer(cmnd->str[2]);

		if(!target || target == player || !player->canSee(target)) {
			player->print("Summon whom?\n");
			return(0);
		}

		if(player->inSameRoom(target)) {
			player->print("%s is already here!\n", target->getCName());
			return(0);
		}
		if(checkRefusingMagic(player, target))
			return(0);

		if(Move::tooFarAway(player, target, "summon"))
			return(0);

		if(	target->getLevel() == 1 &&
			!player->checkStaff("%s is too low level to be summoned.\n", target->getCName())
		)
			return(0);

		// the target cannot leave their room
		if( (	target->getRoomParent()->flagIsSet(R_NO_SUMMON_OUT) ||
				target->getRoomParent()->flagIsSet(R_IS_STORAGE_ROOM) ||
				target->getRoomParent()->flagIsSet(R_LIMBO)
			) &&
			!player->checkStaff("The spell fizzles.\nYour magic cannot locate %s.\n", target->getCName())
		)
			return(0);

		// you can never summon someone here
		// one person room always fails because the target will never fit
		if(	(	player->getRoomParent()->flagIsSet(R_NO_TELEPORT) ||
				player->getRoomParent()->flagIsSet(R_LIMBO) ||
				player->getRoomParent()->flagIsSet(R_VAMPIRE_COVEN) ||
				player->getRoomParent()->flagIsSet(R_NO_SUMMON_TO) ||
				player->getRoomParent()->flagIsSet(R_SHOP_STORAGE) ||
				player->flagIsSet(P_NO_CAST_SUMMON) ||
				player->getRoomParent()->flagIsSet(R_ONE_PERSON_ONLY) ||
				player->getRoomParent()->whatTraining()
			) &&
			!player->checkStaff("The spell fizzles.\nYou cannot summon anyone to this location.\n")
		)
			return(0);

		// you can't summon them right now
		if(	(	player->getRoomParent()->isConstruction() ||
				player->getRoomParent()->flagIsSet(R_SHOP_STORAGE) ||
				(player->inUniqueRoom() && !target->canEnter(player->getUniqueRoomParent(), false))
			) &&
			!player->checkStaff("The spell fizzles.\nYou cannot summon %s to this location at this time.\n", target->getCName())
		)
			return(0);



		// Makes people level summon slaves to summon their other characters.
		if( (target->getLevel()-3) > player->getLevel() &&
			player->getLevel() < 15 &&
			!target->flagIsSet(P_OUTLAW) &&
			!player->checkStaff("The spell fizzles.\nYou are not powerful enough to summon %s.\n", target->getCName())
		)
			return(0);

		// nosummon flag
		if(	target->flagIsSet(P_NO_SUMMON) && !target->flagIsSet(P_OUTLAW) &&
			!player->checkStaff("The spell fizzles.\n%M's summon flag is not set.\n", target)
		) {
			target->print("%s tried to summon you!\nIf you wish to be summoned, type \"set summon\".\n", player->getCName());
			return(0);
		}

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {

			if(spellData->how == CAST && !player->isStaff() && target->checkDimensionalAnchor()) {
				player->printColor("^y%M's dimensional-anchor causes your spell to fizzle!^w\n", target);
				target->printColor("^yYour dimensional-anchor protects you from %N's summon spell!^w\n", player);
				return(1);
			}

			player->print("You summon %N.\n", target);
			target->print("%M summons you.\n", player);
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M summons %N.", player, target);

			broadcast(target->getSock(), target->getRoomParent(), "%M vanishes!", target);

			target->deleteFromRoom();
			target->addToSameRoom(player);
			target->doPetFollow();

			return(1);
		}
	}

	return(1);
}

//*********************************************************************
//						splTrack
//*********************************************************************
// This function allows rangers to cast the track spell which takes them
// to any other player in the game.

int splTrack(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*pPlayer=0, *target=0, *follower=0;
	BaseRoom *oldRoom=0;
	UniqueRoom	*troom=0;
	int		chance=0;
	long	t = time(0);

	pPlayer = player->getAsPlayer();
	if(!pPlayer)
		return(0);
	oldRoom = pPlayer->getRoomParent();
	bool groupTrack = (pPlayer->getClass() == RANGER || pPlayer->getClass() == DRUID) && pPlayer->getLevel() >= 16;

	if(pPlayer->getClass() == BUILDER) {
		pPlayer->print("You cannot cast this spell.\n");
		return(0);
	}

	if(	pPlayer->getClass() != RANGER &&
		pPlayer->getClass() != DRUID &&
		!pPlayer->isCt() &&
		spellData->how == CAST
	) {
		pPlayer->print("Only druids and rangers may cast that spell.\n");
		return(0);
	}

	if(cmnd->num == 2) {
		pPlayer->print("You may not use that on yourself.\n");
		return(0);
	}
	
	if(pPlayer->checkHeavyRestrict("track someone"))
		return(0);
		
	if(player->noPotion( spellData))
		return(0);

	if(!pPlayer->isStaff() && pPlayer->checkDimensionalAnchor()) {
		pPlayer->printColor("^yYour dimensional-anchor causes your spell to fizzle!^w\n");
		return(1);
	}


	cmnd->str[2][0] = up(cmnd->str[2][0]);
	target = gServer->findPlayer(cmnd->str[2]);

	if(!target || target == pPlayer || !pPlayer->canSee(target)) {
		pPlayer->print("That player is not playing (Use full names).\n");
		return(0);
	}

	if(target->isStaff() && target->getClass() > pPlayer->getClass()) {
		pPlayer->print("The spell fizzles.\n");
		return(0);
	}

	troom = target->getUniqueRoomParent();
	if(!pPlayer->isStaff()) {

		if( (	target->getRoomParent()->flagIsSet(R_IS_STORAGE_ROOM) ||
				target->getRoomParent()->flagIsSet(R_SHOP_STORAGE) ||
				target->getRoomParent()->flagIsSet(R_NO_TRACK_TO) ||
				target->getRoomParent()->flagIsSet(R_NO_TELEPORT) ||
				target->getRoomParent()->flagIsSet(R_VAMPIRE_COVEN) ||
				target->getRoomParent()->flagIsSet(R_LIMBO) ||
				target->getRoomParent()->isFull() ||
				target->getRoomParent()->isConstruction() ||
				target->getRoomParent()->whatTraining()
			) || (
				pPlayer->getRoomParent()->flagIsSet(R_NO_TRACK_OUT) ||
				pPlayer->getRoomParent()->flagIsSet(R_LIMBO) ||
				pPlayer->getRoomParent()->flagIsSet(R_ETHEREAL_PLANE)
			)
		) {
			pPlayer->print("The spell fizzles.\n");
			return(0);
		}

		if(troom && !pPlayer->canEnter(troom)) {
			pPlayer->print("The spell fizzles.\n");
			return(0);
		}


		if(Move::tooFarAway(pPlayer, target, "track"))
			return(0);

		if(!dec_daily(&pPlayer->daily[DL_TRACK]) && spellData->how == CAST) {
			pPlayer->print("You have tracked enough today.\n");
			return(0);
		}

		if(target->flagIsSet(P_LINKDEAD)) {
			pPlayer->print("The spell fizzles.\n");
			return(0);
		}


		if(	target->isEffected("resist-magic") ||
			target->isEffected("camouflage") ||
			(target->isEffected("mist") && pPlayer->getLevel() <= target->getLevel()))
		{

			chance = (50 + (((int)pPlayer->getLevel() - (int)target->getLevel())*10)
					+ (bonus((int) pPlayer->intelligence.getCur()) - bonus((int) target->intelligence.getCur())));

			if(target->isEffected("mist"))
				chance -= 35;

			if(mrand(1,100) < chance) {
				if(target->isEffected("mist"))
					pPlayer->print("%s's mist form eluded your magic.\n", target->getCName());
				else
					pPlayer->print("%s manages to elude your track.\n", target->getCName());
				return(0);
			}
		}
	}


	Group* group = player->getGroup();
	// Check for trying to grouptrack with linkdead people following them - abuse
	if(groupTrack) {
		if(pPlayer->getGroupStatus() == GROUP_LEADER && group) {
			for(Creature* crt : group->members) {
				if(crt->pFlagIsSet(P_LINKDEAD) && pPlayer->inSameRoom(crt)) {
					pPlayer->print("%M's partial link with reality prevents you from tracking.\n", crt);
					return(0);
				}
			}
		}
	}

	pPlayer->print("You track %N.\n", target);
	target->print("%M has tracked you.\n", pPlayer);
	broadcast(pPlayer->getSock(), target->getSock(), oldRoom, "%M tracks %N.", pPlayer, target);

	pPlayer->deleteFromRoom();
	pPlayer->addToSameRoom(target);

	if(!pPlayer->isStaff()) {
		pPlayer->updateAttackTimer(true, 20);
		pPlayer->lasttime[LT_SPELL].ltime = t;
		pPlayer->lasttime[LT_SPELL].interval = 2L;
		pPlayer->lasttime[LT_HIDE].ltime = t;
		pPlayer->lasttime[LT_HIDE].interval = 2L;

		pPlayer->printColor("^gYou are temporarily disoriented.\n");
	}
	pPlayer->doPetFollow();

	if(groupTrack) {
		if(pPlayer->getGroupStatus() == GROUP_LEADER && group) {
			for(Creature* crt : group->members) {
				follower = crt->getAsPlayer();

				if(!follower || follower->getRoomParent() != oldRoom)
					continue;
				if(troom && !follower->canEnter(troom))
					continue;

				follower->deleteFromRoom();
				follower->addToSameRoom(target);
				follower->doPetFollow();

				if(!follower->flagIsSet(P_DM_INVIS)) {
					broadcast(pPlayer->getSock(), follower->getSock(), oldRoom, "%N follows %N.", follower, pPlayer);

					follower->printColor("^gYou are temporarily disoriented.\n");

					follower->updateAttackTimer(true, 20);
					follower->lasttime[LT_SPELL].ltime = t;
					follower->lasttime[LT_SPELL].interval = 2L;
					follower->lasttime[LT_HIDE].ltime = t;
					follower->lasttime[LT_HIDE].interval = 2L;
				}
			}
		}
	}

	return(1);
}

//*********************************************************************
//						splWordOfRecall
//*********************************************************************
// This function allows a player to teleport themself or another player
// to the recall room

int splWordOfRecall(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*caster = player->getAsPlayer();
	Player	*target=0;

	if(player->getClass() == BUILDER) {
		player->print("You cannot cast this spell.\n");
		return(0);
	}

	if(	player->getClass() != CLERIC &&
		player->getClass() != PALADIN &&
		player->getType()== PLAYER &&
		!player->isCt() && spellData->how == CAST
	) {
		player->print("Only clerics and paladins may cast that spell.\n");
		return(0);
	}


	if(player->getRoomParent()->flagIsSet(R_NO_WORD_OF_RECALL) && !player->isCt()) {
		player->print("Nothing happens.\n");
		return(0);
	}

	// Cast recall on self
	if(caster && cmnd->num <= 2) {

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("Word of recall spell cast.\n");
			broadcast(player->getSock(), player->getParent(), "%M casts word of recall on %sself.",
				caster, caster->himHer());
		}

		if(spellData->how == CAST && !caster->isStaff() && caster->checkDimensionalAnchor()) {
			player->printColor("^yYour dimensional-anchor causes your spell to fizzle!^w\n");
			return(1);
		}

		if(spellData->how == POTION) {
			player->print("You phase in and out of existence.\n");
			broadcast(player->getSock(), caster->getRoomParent(), "%M disappears.", caster);
		}

		caster->statistics.recall();
		caster->doRecall();
		return(1);

	// Cast word of recall on another player
	} else {
		if(player->noPotion( spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findPlayer(player, cmnd, 2);
		if(!target) {
			player->print("That person is not here.\n");
			return(0);
		}

		if(!player->isCt() && target->isStaff()) {
			player->print("%s is too powerful to be affected by your magic.\n", target->upHeShe());
			return(0);
		}

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {

			player->print("Word of recall cast on %N.\n", target);
			target->print("%M casts a word of recall spell on you.\n", player);
			broadcast(player->getSock(), target->getSock(), player->getParent(),
				"%M casts word of recall on %N.", player, target);

			if(spellData->how == CAST && !player->isStaff() && target->checkDimensionalAnchor()) {
				player->printColor("^y%M's dimensional-anchor causes your spell to fizzle!^w\n", target);
				target->printColor("^yYour dimensional-anchor protects you from %N's word-of-recall spell!^w\n", player);
				return(1);
			}

			// eh?
			if(target->flagIsSet(P_JAILED)) {
				if(player->isCt())
					player->print("%M's jailed flag has been cleared.\n", target);
				target->clearFlag(P_JAILED);
			}
			target->doRecall();
			return(2);
		}
	}
	return(1);
}

//*********************************************************************
//						splPlaneShift
//*********************************************************************
// This spell allows a player to travel to another plane

int splPlaneShift(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*pPlayer=0, *target=0;
	UniqueRoom	*new_rom=0;

	pPlayer = player->getAsPlayer();
	if(!pPlayer)
		return(0);

	if(!pPlayer->isCt()) {
		pPlayer->print("You cannot cast that spell.\n");
		return(0);
	}

	// Cast Plane-Shift on yourself
	if(cmnd->num == 2) {
		target = pPlayer;
		pPlayer->print("You fade out of existence.\n");
		broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%M slowly fades out of existence.", pPlayer);

	// Cast plane-shift on another pPlayer
	} else {

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = pPlayer->getParent()->findPlayer(pPlayer, cmnd, 2);
		if(!target) {
			pPlayer->print("That player is not here.\n");
			return(0);
		}

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			pPlayer->print("You cast plane-shift on %N.\n", target);
			target->print("%M casts plane-shift on you.\n", pPlayer);
			broadcast(pPlayer->getSock(), target->getSock(), pPlayer->getRoomParent(), "%M casts plane-shift on %N.", pPlayer, target);
			broadcast("");
		}
	}

	CatRef	cr;
	cr.setArea("plane");
	cr.id = 1;
	if(!loadRoom(cr, &new_rom)) {
		pPlayer->print("Spell failure.\n");
		return(0);
	}

	target->deleteFromRoom();
	target->addToRoom(new_rom);

	return(1);
}

//********************************************************************
//						checkDimensionalAnchor
//********************************************************************
// returns true on spell failure

bool Creature::checkDimensionalAnchor() const {
	if(isMonster())
		return(false);
	if(!isEffected("anchor"))
		return(false);

	if(mrand(1,10)>9)
		return(false);

	return(true);
}

//*********************************************************************
//						splBlink
//*********************************************************************

int splBlink(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*pPlayer = player->getAsPlayer();
	Exit	*exit=0;
	int		i=0;
	bool doMove = true;
	bool portalDestroyed = false;
	bool isPortal = false;
	bool canCast = false;
	BaseRoom* newRoom=0, *room = player->getRoomParent();

	if(!pPlayer)
		return(0);

	if(	pPlayer->getClass() == MAGE ||
		pPlayer->getClass() == LICH ||
		pPlayer->getSecondClass() == MAGE ||
		pPlayer->isStaff()
	)
		canCast = true;


	if(!pPlayer->spellIsKnown(S_BLINK)) {
		player->print("You do not know that spell.\n");
		return(0);
	}

	if(!canCast && spellData->how == CAST) {
		player->print("You cannot cast that spell.\n");
		return(0);
	}

	if(!canCast && spellData->how != CAST) {
		player->print("The magic will not work for you.\n");
		return(0);
	}

	if(cmnd->num < 3) {
		player->print("Syntax: cast blink (exit)\n");
		return(0);
	}

	exit = findExit(pPlayer, cmnd, 2);

	if(!exit) {
		player->print("That exit is not here.\n");
		return(0);
	}

	const Monster* guard = room->getGuardingExit(exit, pPlayer);
	if(guard && !player->checkStaff("You cannot see past %N.\n", guard))
		return(0);

	if(exit->flagIsSet(X_CLOSED)) {
		player->print("You cannot blink where you cannot see!\n");
		return(0);
	}



	if(!pPlayer->canFleeToExit(exit, false, true)) {
		player->print("The spell fizzled.\n");
		return(0);
	}

	if(spellData->how == CAST && !pPlayer->isStaff() && pPlayer->checkDimensionalAnchor()) {
		player->printColor("^yYour dimensional-anchor causes your spell to fizzle!^w\n");
		return(1);
	}


	if(exit->flagIsSet(X_PORTAL)) {
		isPortal = true;

		// 10% chance of going to the ethereal plane
		if(mrand(1,10) == 1) {
			CatRef cr = getEtherealTravelRoom();
			UniqueRoom* uRoom=0;
			if(loadRoom(cr, &uRoom))
				newRoom = uRoom;
		}

		// otherwise, it's a normal teleport
		if(!newRoom)
			newRoom = player->teleportWhere();
	} else {
		newRoom = pPlayer->getFleeableRoom(exit);
	}


	if(!newRoom) {
		player->print("The spell fizzled.\n");
		return(0);
	}


	if(isPortal) {
		player->print("You cast a blink spell.\n");
		broadcast(pPlayer->getSock(), room, "%M casts a blink spell.", player);

		broadcast(0, room, "^YThe %s^Y explodes violently as the dimensional tunnels converge!", exit->getCName());

		int dmg=0;
		for(Player* ply : room->players) {
			// more damage for caster
			if(ply->isStaff())
				dmg = 0;
			else if(ply == player)
				dmg = mrand(25,45);
			else
				dmg = mrand(5,25);

			if(ply->chkSave(BRE, player, -25))
				dmg /= 2;


			ply->printColor("You are blasted with energy for %s%d^x damage!\n", ply->customColorize("*CC:DAMAGE*").c_str(), dmg);
			broadcast(ply->getSock(), ply->getRoomParent(), "%M is blasted with energy!", ply);
			broadcastGroup(false, ply, "%M is blasted by energy for *CC:DAMAGE*%d^x damage, %s%s\n",
				ply, dmg, ply->heShe(), ply->getStatusStr(dmg));

			ply->hp.decrease(dmg);

			if(ply->hp.getCur() < 1) {
				// killing the owner of a portal will invalidate the exit
				if(!portalDestroyed && exit->getPassPhrase() == ply->getName())
					portalDestroyed = true;
				ply->die(EXPLOSION);
				if(ply == player)
					doMove = false;
			} else {
				if(ply == player) {
					player->print("You are violently ejected from the room!\n\n");
					broadcast(pPlayer->getSock(), room, "%M is violently ejected from the room!", player);
				}
			}
		}

	} else{
		player->printColor("You cast a blink spell. You blink away to the %s^x.\n", exit->getCName());
		broadcast(pPlayer->getSock(), room, "%M blinks away!", player);
		broadcast(isDm, pPlayer->getSock(), room, "*DM* %M blinks to the \"%s^x\" exit.", pPlayer, exit->getCName());
	}

	if(doMove) {
		// if the portal wasn't destroyed yet, this will mark it as so
		if(!portalDestroyed && isPortal && exit->getPassPhrase() == player->getName())
			portalDestroyed = true;

		i = pPlayer->deleteFromRoom();
		pPlayer->addToRoom(newRoom);
		pPlayer->doPetFollow();

		broadcast(pPlayer->getSock(), newRoom, "%M just blinked into existence.", player);

		// if we left an area room that got recycled, no need to do any more
		if(i & DEL_ROOM_DESTROYED)
			room = 0;
	}

	if(room && !isPortal)
		exit->checkReLock(player, false);

	if(isPortal && room) {
		room->scatterObjects();

		// if the portal hasn't been destroyed yet, do so now
		if(!portalDestroyed)
			Move::deletePortal(room, exit);
	}

	return(1);
}

//*********************************************************************
//						willScatterTo
//*********************************************************************

bool willScatterTo(const Object* object, const BaseRoom* room, const Exit* exit) {
	return(	!exit->flagIsSet(X_SECRET) &&
			!exit->isConcealed(0) &&
			!exit->flagIsSet(X_DESCRIPTION_ONLY) &&
			!exit->flagIsSet(X_CLOSED) &&
			!exit->flagIsSet(X_STAFF_ONLY) &&
			!exit->flagIsSet(X_NO_SEE) &&

			!exit->flagIsSet(X_NO_FLEE) &&
			!exit->flagIsSet(X_LOOK_ONLY) &&

			!exit->flagIsSet(X_TO_STORAGE_ROOM) &&
			!exit->flagIsSet(X_TO_BOUND_ROOM) &&
			!exit->flagIsSet(X_TOLL_TO_PASS) &&
			!exit->flagIsSet(X_WATCHER_UNLOCK) &&
			!exit->isWall("wall-of-force") &&
			!room->getGuardingExit(exit, 0) &&
			!(object->getSize() && exit->getSize() && object->getSize() > exit->getSize())
	);
}

//*********************************************************************
//						scatterObjects
//*********************************************************************

void BaseRoom::scatterObjects() {
	BaseRoom* newRoom=0;
	Object* object=0;
	Exit* exit=0;

	int pick=0;
	ObjectSet::iterator it;

	for( it = objects.begin() ; it != objects.end() ; ) {
		object = (*it++);

		// 10% chance of not moving this object
		if(!mrand(0,9))
			continue;

		if(	object->flagIsSet(O_NO_TAKE) ||
			object->flagIsSet(O_SCENERY)
		)
			continue;


		// pick an exit
		pick = 0;
		for(Exit* ext : exits) {
			if(willScatterTo(object, this, ext))
				pick++;
		}

		// no exits to scatter to
		if(!pick)
			return;

		pick = mrand(1, pick);
		for(Exit* ext : exits) {
			if(willScatterTo(object, this, ext)) {
				pick--;
				if(!pick) {
					exit = ext;
					break;
				}
			}
		}

		newRoom = exit->target.loadRoom();

		if(!newRoom)
			continue;

		broadcast(0, this, "%1O flies to the %s^x!", object, exit->getCName());

		// send it to exit
		object->clearFlag(O_HIDDEN);
		object->deleteFromRoom();

		broadcast(0, newRoom, "%1O comes flying into the room!", object);
		newRoom->wake("Loud noises disturb your sleep.", true);
		finishDropObject(object, newRoom, 0, false, false, true);

		//newRoom->killMortalObjectsOnFloor();
	}
}
