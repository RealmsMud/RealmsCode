/*
 * undead.cpp
 *	 Functions for undead players.
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

//*********************************************************************
//						cmdBite
//*********************************************************************

int cmdBite(Player* player, cmd* cmnd) {
	Creature* target=0;
	Player	*pTarget=0;
	long	i=0, t=0;
	int		chance=0, dmgnum=0;
	Damage damage;


	if(!player->ableToDoCommand() || !player->checkAttackTimer())
		return(0);

	if((!player->knowsSkill("bite") || !player->isEffected("vampirism")) && !player->isStaff()) {
		player->print("You lack the fangs!\n");
		return(0);
	}

	if(isDay() && !player->isCt())	{
		player->print("You may only bite people during the night.\n");
		return(0);
	}

	if(player->inCombat() && !player->checkStaff("Not while you're in combat.\n"))
		return(0);

    if(!(target = player->findVictim(cmnd, 1, true, false, "Bite what?\n", "You don't see that here.\n")))
		return(0);

	pTarget = target->getAsPlayer();

	if(!player->canAttack(target))
		return(0);

	if(player->isBlind()) {
		player->printColor("^CYou're blind!\n");
		return(0);
	}

	if(target->isEffected("drain-shield") && target->isMonster()) {
		player->print("Your bite would be ineffective.\n");
		return(0);
	}

	if(	target->isUndead() &&
		!player->checkStaff("That won't work on the undead.\n")
	) {
		if(target->isMonster())
			target->getAsMonster()->addEnemy(player);
		return(0);
	}

	i = player->lasttime[LT_PLAYER_BITE].ltime;
	t = time(0);

	if((t - i < 30L) && !player->isDm()) {
		player->pleaseWait(30L-t+i);
		return(0);
	}

	player->smashInvis();
	player->interruptDelayedActions();

	if(pTarget) {

	} else {
		target->getAsMonster()->addEnemy(player);
		// Activates lag protection.
		if(player->flagIsSet(P_LAG_PROTECTION_SET))
			player->setFlag(P_LAG_PROTECTION_ACTIVE);
	}

	player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
	player->lasttime[LT_PLAYER_BITE].ltime = t;
	player->lasttime[LT_READ_SCROLL].interval = 3L;
	player->lasttime[LT_READ_SCROLL].ltime = t;
	player->lasttime[LT_PLAYER_BITE].interval = 30L;


	chance = 35 + ((int)((player->getSkillLevel("bite") - target->getLevel()) * 20) + bonus((int) player->strength.getCur()) * 5);
	chance = MIN(chance, 85);

	if(target->isEffected("drain-shield"))
		chance /= 2;

	if(player->isDm())
		chance = 100;

	if(mrand(1, 100) > chance) {
		player->print("%s eludes your bite.\n", target->upHeShe());
		player->checkImprove("bite", false);
		target->print("%M tried to bite you!\n",player);
		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to bite %N.", player, target);
		return(0);
	}


	damage.set(mrand((int)(player->getSkillLevel("bite")*3), (int)(player->getSkillLevel("bite")*4)));

	target->modifyDamage(player, NEGATIVE_ENERGY, damage);

	dmgnum = damage.get();

	damage.set(MIN(damage.get(), target->hp.getCur() + 1));
	if(damage.get() < 1)
		damage.set(1);

	player->printColor("You bite %N for %s%d^x damage.\n", target, player->customColorize("*CC:DAMAGE*").c_str(), dmgnum);
	player->checkImprove("bite", true);

	if(player->hp.getCur() < player->hp.getMax() && damage.getDrain()) {
		player->print("The blood of %N revitalizes your strength.\n", target);
		player->hp.increase(damage.getDrain());
	}

	if(player->getClass() == CARETAKER)
		log_immort(false,player, "%s bites %s.\n", player->name, target->name);

	target->printColor("%M bites you for %s%d^x damage.\n", player, target->customColorize("*CC:DAMAGE*").c_str(), dmgnum);
	target->stun((mrand(5, 8) + bonus((int) player->strength.getCur())));
	broadcast(player->getSock(), target->getSock(), player->getParent(), "%M bites %N!", player, target);
	broadcastGroup(false, target, "^M%M^x bites ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n",
		player, target, dmgnum, target->heShe(), target->getStatusStr(dmgnum));

	player->statistics.attackDamage(dmgnum, "bite");

	if(player->doDamage(target, damage.get(), CHECK_DIE)) {
		if(player->getClass() == CARETAKER)
			log_immort(true, player, "%s killed %s with a bite.\n", player->name, target->name);
	} else {
		if(!induel(player, pTarget)) {
			// 5% chance to get porphyria when bitten by a vampire
			target->addPorphyria(player, 5);
		}
	}

	return(0);
}

//*********************************************************************
//						`Mist
//*********************************************************************

int cmdMist(Player* player, cmd* cmnd) {
	long	i=0, t=0;
	t = time(0);
	i = player->getLTAttack() > LT(player, LT_SPELL) ? player->getLTAttack() : LT(player, LT_SPELL);

	if(LT(player, LT_MIST) > i)
		i = LT(player, LT_MIST);



	if(!player->ableToDoCommand())
		return(0);

	if((!player->knowsSkill("mist") || !player->isEffected("vampirism")) && !player->isStaff()) {
		player->print("You are unable to turn to mist.\n");
		return(0);
	}
	if(player->getRoomParent()->flagIsSet(R_ETHEREAL_PLANE)) {
		player->print("That is not possible here.\n");
		return(0);
	}

	if(t < i) {
		player->pleaseWait(i-t);
		return(0);
	}

	if(!player->canMistNow()) {
		player->print("You may only turn to mist during the night.\n");
		return(0);
	}

	if(player->flagIsSet(P_OUTLAW)) {
		player->print("You are unable to mist right now.\n");
		return(0);
	}

	if(player->getRoomParent()->flagIsSet(R_DISPERSE_MIST)) {
		player->print("Swirling vapors prevent you from entering mist form.\n");
		return(0);
	}

	if(player->getGroup()) {
		player->print("You can't do that while in a group.\n");
		return(0);
	}
	if(player->isEffected("mist")) {
		player->print("You are already a mist.\n");
		return(0);
	}

	if(player->inCombat()) {
		player->print("Not in the middle of combat.\n");
		return(0);
	}

	broadcast(player->getSock(), player->getParent(), "%M turns to mist.", player);
	player->addEffect("mist", -1);
	player->print("You turn to mist.\n");
	//player->checkImprove("mist", true);
	player->clearFlag(P_SITTING);
	player->interruptDelayedActions();

	player->lasttime[LT_MIST].ltime = time(0);
	player->lasttime[LT_MIST].interval = 3L;

	return(0);
}

//*********************************************************************
//						canMistNow
//*********************************************************************
// whether or not the player can currently turn to mist

bool Player::canMistNow() const {
	if(isStaff())
		return(true);
	// only people who know how to do it can mist
	if((!knowsSkill("mist") || !isEffected("vampirism")))
		return(false);
	// does the room let them mist anyway?
	// BUG: callin flagIsSet() with possibly invalid getRoom()
	if(getConstRoomParent() && (getConstRoomParent()->flagIsSet(R_VAMPIRE_COVEN) || getConstRoomParent()->flagIsSet(R_CAN_ALWAYS_MIST)))
		return(true);
	// otherwise, not during the day
	if(isDay())
		return(false);
	return(true);
}

//*********************************************************************
//						cmdUnmist
//*********************************************************************

int cmdUnmist(Player* player, cmd* cmnd) {
	if(player->isEffected("mist")) {
		player->unmist();
	} else {
		if(!player->isEffected("vampirism") && !player->isStaff())
			return(0);

		player->print("You are not in mist form.\n");
	}
	return(0);
}

//*********************************************************************
//						cmdHypnotize
//*********************************************************************

int cmdHypnotize(Player* player, cmd* cmnd) {
	Creature* target=0;
	int		dur=0, chance=0;
	long	i=0, t=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("hypnotize")) {
		player->print("You lack the training to hypnotize anyone!\n");
		return(0);
	}
	if(!player->isEffected("vampirism") && !player->isStaff()) {
		player->print("Only vampires may hypnotize people.\n");
		return(0);
	}

	i = LT(player, LT_HYPNOTIZE);
	t = time(0);
	if(i > t && !player->isDm()) {
		player->pleaseWait(i-t);
		return(0);
	}

	dur = 300 + mrand(1, 30) * 10 + bonus((int) player->constitution.getCur()) * 30 +
	      (int)(player->getSkillLevel("hypnotize") * 5);

    if(!(target = player->findVictim(cmnd, 1, true, false, "Hypnotize whom?\n", "You don't see that here.\n")))
   		return(0);

	if(!target->canSee(player) && !player->checkStaff("Your victim must see you to be hypnotized.\n"))
		return(0);

	if(!player->canAttack(target))
		return(0);

	if(target->isMonster()) {
		if(target->getAsMonster()->isEnemy(player)) {
			player->print("Not while you are already fighting %s.\n", target->himHer());
			return(0);
		}
	}



	if(	target->isPlayer() &&
		(	player->vampireCharmed(target->getAsPlayer()) ||
			(target->hasCharm(player->name) && player->flagIsSet(P_CHARMED))
		)
	) {
		player->print("But they are already your good friend!\n");
		return(0);
	}

	if(target->isPlayer() && target->getClass() == RANGER && target->getLevel() > player->getLevel()) {
		player->print("%M is immune to your hypnotizing gaze.\n", target);
		return(0);
	}

	if(target->isMonster() && (target->flagIsSet(M_NO_CHARM) || (target->intelligence.getCur() < 3)) ) {
		player->print("Your hypnotism fails.\n");
		return(0);
	}

	if(	target->isUndead() &&
		!player->checkStaff("You cannot hypnotize undead beings.\n") )
		return(0);

	player->smashInvis();


	player->lasttime[LT_HYPNOTIZE].ltime = t;
	player->lasttime[LT_HYPNOTIZE].interval = 600L;
	chance = MIN(90, 40 + (int)(player->getSkillLevel("hypnotize") - target->getLevel()) * 20 + 4 *
			bonus((int) player->intelligence.getCur()));
	if(target->flagIsSet(M_PERMENANT_MONSTER))
		chance-=25;
	if(player->isDm())
		chance = 101;
	if(mrand(1, 100) > chance && !player->isCt()) {
		player->print("You fail to hypnotize %N.\n", target);
		player->checkImprove("hypnotize", false);
		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M attempts to hypnotize %N.",player, target);
		if(target->isMonster()) {
			target->getAsMonster()->addEnemy(player);
			if(player->flagIsSet(P_LAG_PROTECTION_SET)) {    // Activates lag protection.
				player->setFlag(P_LAG_PROTECTION_ACTIVE);
			}
			return(0);
		}
		target->printColor("^m%M tried to hypnotize you.\n", player);
		return(0);
	}

	if(	(target->isPlayer() && target->isEffected("resist-magic")) ||
		(target->isMonster() && target->isEffected("resist-magic"))
	)
		dur /= 2;


	if(!player->isCt()) {
		if(target->chkSave(MEN, player,0)) {
			player->print("%M avoided your hypnotizing gaze.\n", target);
			player->checkImprove("hypnotize", false);
			target->print("You avoided %s's hypnotizing gaze.\n", player->name);
			return(0);
		}
	}

	log_immort(false,player, "%s hypnotizes %s.\n", player->name, target->name);

	player->print("You hypnotize %N.\n", target);
	player->checkImprove("hypnotize", true);
	broadcast(player->getSock(), target->getSock(), player->getParent(), "%M hypnotizes %N!", player, target);
	target->print("%M hypnotizes you.\n", player);


	player->addCharm(target);

	target->lasttime[LT_CHARMED].ltime = time(0);
	target->lasttime[LT_CHARMED].interval = dur;

	if(target->isPlayer())
		target->setFlag(P_CHARMED);
	else
		target->setFlag(M_CHARMED);


	return(0);
}


//*********************************************************************
//						cmdRegenerate
//*********************************************************************
// This command is for liches.

int cmdRegenerate(Player* player, cmd* cmnd) {
	int		chance=0, xtra=0, pasthalf=0;
	long	i=0, t=0;

	if(!player->ableToDoCommand())
		return(0);


	player->clearFlag(P_AFK);
	if(!player->knowsSkill("regenerate")) {
		player->print("You lack the training to regenerate.\n");
		return(0);
	}

	if(player->inCombat() &&
		!player->checkStaff("You are too busy to draw from the life energy around you.\n"))
		return(0);

	if(!player->isCt()) {
		if(player->hp.getCur() > player->hp.getMax() * 3 / 4) {
			player->printColor("^cYou cannot regenerate when past 3/4 of your life force.\n");
			return(0);
		}
		i = player->lasttime[LT_REGENERATE].ltime + player->lasttime[LT_REGENERATE].interval;
		t = time(0);
		if(i > t) {
			player->pleaseWait(i-t);
			return(0);
		}
	}

	if( player->getAlignment() > 100 &&
		!player->checkStaff("There isn't enough evil in your soul to regenerate.\n")
	)
		return(0);

	int level = (int)player->getSkillLevel("regenerate");
	// get a base number:
	//	 10 = -36.66
	//	120 = 0
	//	400 = 93.3
	chance = (player->constitution.getCur() - 120) / 3;
	chance = MIN(85, level * 4 + chance);
	if(player->isCt())
		chance = 101;

	if(mrand(1, 100) <= chance) {
		player->print("You feel the evil in your soul rebuilding.\n");
		player->checkImprove("regenerate", true);
		broadcast(player->getSock(), player->getParent(), "%M regenerates.", player);

		for(Player* ply : player->getRoomParent()->players) {
			if(!ply->isUndead())
				ply->print("You shiver from a sudden deathly coldness.\n");
		}

		player->hp.increase(mrand(level+4,level*(5/2)+5));

		// Prevents players from avoiding the 75% hp limit to allow regenerate
		// by casting on themselves to get below half. A lich may never regenerate
		// past 75% their max HP. This totally evens out their ticktime with mages
		if(player->hp.getCur() > player->hp.getMax() * 3 / 4) {
			player->hp.setCur((player->hp.getMax() * 3 / 4) + 1);
			pasthalf = 1;
		}

		// Extra regeneration from being extremely evil.
		if(player->getAlignment() <= -250 && !pasthalf) {
			if(player->getAlignment() <= -400)
				xtra = mrand(8,10);
			else
				xtra = mrand(4,6);
			player->print("Your life force regenerates %d more due to your extreme evil.\n",xtra);
			player->hp.increase(xtra);
		}


		player->lasttime[LT_REGENERATE].ltime = t;
		player->lasttime[LT_REGENERATE].interval = 120L;
	} else {
		player->print("You fail to regenerate.\n");
		player->checkImprove("regenerate", false);
		broadcast(player->getSock(), player->getParent(), "%M tried to regenerate.", player);
		player->lasttime[LT_REGENERATE].ltime = t;
		player->lasttime[LT_REGENERATE].interval = 10L;
	}

	return(0);
}

//*********************************************************************
//						cmdDrainLife
//*********************************************************************
// This allows a lich to drain hp from someone else and add half the
// damage to their own

int cmdDrainLife(Player* player, cmd* cmnd) {
	Creature* target=0;
	Player	*pTarget=0;
	long	i=0, t=0;
	int		chance=0;
	Damage damage;

	if(!player->ableToDoCommand())
		return(0);


	if(!player->knowsSkill("drain")) {
		player->print("You haven't learned how to drain someone's life.\n");
		return(0);
	}

	if(player->inCombat()) {
		player->print("Not while you're already in combat.\n");
		return(0);
	}


    if(!(target = player->findVictim(cmnd, 1, true, false, "Drain whom?\n", "You don't see that here.\n")))
   		return(0);

	pTarget = target->getAsPlayer();

    player->smashInvis();
	player->interruptDelayedActions();

	if(!player->canAttack(target))
		return(0);

	if(	target->isUndead() &&
		!player->checkStaff("You may only drain the life of the living.\n") )
		return(0);

	if(!pTarget) {
		// Activates lag protection.
		if(player->flagIsSet(P_LAG_PROTECTION_SET))
			player->setFlag(P_LAG_PROTECTION_ACTIVE);

		if(target->isPet()) {
			if(!player->flagIsSet(P_CHAOTIC) && !player->isCt() && !target->getPlayerMaster()->flagIsSet(P_OUTLAW)) {
				player->print("Sorry, you're lawful.\n");
				return(0);
			}
			if(!target->getPlayerMaster()->flagIsSet(P_CHAOTIC) && !player->isCt() && !target->getPlayerMaster()->flagIsSet(P_OUTLAW)) {
				player->print("Sorry, that creature is lawful.\n");
				return(0);
			}
			if(player->getRoomParent()->isPkSafe() && !target->getPlayerMaster()->flagIsSet(P_OUTLAW)) {
				player->print("No killing allowed in this room.\n");
				return(0);
			}
		}
	}

	i = LT(player, LT_DRAIN_LIFE);
	t = time(0);
	if(i > t && !player->isDm()) {
		player->pleaseWait(i-t);
		return(0);
	}

	if(target->isMonster())
		target->getAsMonster()->addEnemy(player);

	player->lasttime[LT_DRAIN_LIFE].ltime = t;
	player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
	player->lasttime[LT_DRAIN_LIFE].interval = 120L;

	int level = (int)player->getSkillLevel("drain");

	chance = (level - target->getLevel()) * 20 + bonus((int) player->constitution.getCur()) * 5 + 25;
	chance = MIN(chance, 80);

	if(target->isEffected("drain-shield"))
		chance /= 2;

	if(mrand(1, 100) > chance) {
		player->print("You failed to drain %N's life.\n", target);
		player->checkImprove("drain", false);

		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to drain %N's life.", player, target);
		target->print("%M tried drain your life!\n", player);
		return(0);
	}


	damage.set(mrand(level * 2, level * 4));
	if(target->isEffected("drain-shield"))
		damage.set(mrand(1, level));

	// Berserked barbarians have more energy to drain
	if(pTarget && pTarget->isEffected("berserk"))
		damage.set(damage.get() + (damage.get() / 5));

	damage.set(MIN(damage.get(), target->hp.getCur() + 1));
	if(damage.get() < 1)
		damage.set(1);

	if(!target->isCt()) {
		if(target->chkSave(DEA, player, 0)) {
			player->printColor("^yYour life-drain failed!\n");
			target->print("%M failed to drain your life.\n", player);
			return(0);
		}
	}

	target->modifyDamage(player, NEGATIVE_ENERGY, damage);

	// drain heals us
	if(!target->isEffected("drain-shield"))
		player->hp.increase(damage.get() / 2);

	player->printColor("You drained %N for %s%d^x damage.\n", target, player->customColorize("*CC:DAMAGE*").c_str(), damage.get());
	player->checkImprove("drain", true);

	player->statistics.attackDamage(damage.get(), "drain-life");

	if(target->isEffected("drain-shield"))
		player->print("%M resisted your drain!\n", target);

	target->printColor("%M drained you for %s%d^x damage.\n", player, target->customColorize("*CC:DAMAGE*").c_str(), damage.get());
	broadcast(player->getSock(), target->getSock(), player->getParent(), "%M drained %N's life.", player, target);

	if(player->doDamage(target, damage.get(), CHECK_DIE)) {
		if(player->getClass() == CARETAKER && pTarget)
			log_immort(true, player, "%s killed %s with the drain.\n", player->name, target->name);
	}

	return(0);
}
