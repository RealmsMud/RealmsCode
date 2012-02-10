/*
 * healers.cpp
 *	 Functions that deal with healer class abilities.
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
//						cmdEnthrall
//*********************************************************************
// This ability allows a cleric of Aramon to charm undead creatures
// and players. -- TC

int cmdEnthrall(Player* player, cmd* cmnd) {
	Creature* creature=0;
	int		dur=0, chance=0;
	long	i=0, t=0;

	player->clearFlag(P_AFK);
	if(!player->ableToDoCommand())
		return(0);

	if(!player->isCt()) {
		if(!player->knowsSkill("enthrall")) {
			player->print("You don't possess the skills to enthrall the undead!\n");
			return(0);
		}
		if(player->isBlind()) {
			player->printColor("^CYou must be able to see your enemy to do that!\n");
			return(0);
		}
	}

	i = LT(player, LT_HYPNOTIZE);
	t = time(0);
	if(i > t && !player->isDm()) {
		player->pleaseWait(i-t);
		return(0);
	}

	dur = 300 + mrand(1, 30) * 10 + bonus((int) player->piety.getCur()) * 30 +
	      (int)(player->getSkillLevel("enthrall") * 5);


    if(!(creature = player->findVictim(cmnd, 1, true, false, "Enthrall whom?\n", "You don't see that here.\n")))
		return(0);

    if(creature->isMonster()) {
		if(creature->getMonster()->isEnemy(player)) {
			player->print("Not while you are already fighting %s.\n", creature->himHer());
			return(0);
		}
	}
	if(!player->canAttack(creature))
		return(0);

	if(	!player->alignInOrder() &&
		!player->checkStaff("You are not wicked enough in %s's eyes to do that.\n", gConfig->getDeity(player->getDeity())->getName().c_str())
	)
		return(0);

	if( !creature->isUndead() &&
		!player->checkStaff("You may not enthrall the living with %s's power.\n", gConfig->getDeity(player->getDeity())->getName().c_str())
	)
		return(0);

	player->smashInvis();
	player->lasttime[LT_HYPNOTIZE].ltime = t;
	player->lasttime[LT_HYPNOTIZE].interval = 300L;

	chance = MIN(90, 40 + (int)((player->getSkillLevel("enthrall") - creature->getLevel()) * 10) +
			4 * bonus((int) player->piety.getCur()));

	if(creature->flagIsSet(M_PERMENANT_MONSTER))
		chance -= 25;

	if(player->isDm())
		chance = 101;

	if((chance < mrand(1, 100)) && (chance != 101)) {
		player->print("You were unable to enthrall %N.\n", creature);
		player->checkImprove("enthrall",false);
		broadcast(player->getSock(), player->getParent(), "%M tried to enthrall %N.",player, creature);
		if(creature->isMonster()) {
			creature->getMonster()->addEnemy(player);
			return(0);
		}
		creature->printColor("^r%M tried to enthrall you with the power of %s.\n", player, gConfig->getDeity(player->getDeity())->getName().c_str());

		return(0);
	}


	if(!creature->isCt() &&
		creature->chkSave(MEN, player, 0))
	{
		player->printColor("^yYour enthrall failed!\n");
		creature->print("Your mind tingles as you brush off %N's enthrall.\n", player);
		player->checkImprove("enthrall",false);
		return(0);
	}

	if(	(creature->isPlayer() && creature->isEffected("resist-magic")) ||
		(creature->isMonster() && creature->isEffected("resist-magic"))
	)
		dur /= 2;

	player->print("You enthrall %N with the power of %s.\n", creature, gConfig->getDeity(player->getDeity())->getName().c_str());
	player->checkImprove("enthrall",true);

	broadcast(player->getSock(), creature->getSock(), player->getRoom(), "%M flashes %s symbol of %s to %N, enthralling %s.",
		player, player->hisHer(), gConfig->getDeity(player->getDeity())->getName().c_str(), creature,
	    player->himHer());
	creature->print("%M's chant of %s's word enthralls you.\n", player, gConfig->getDeity(player->getDeity())->getName().c_str());

	player->addCharm(creature);

	creature->stun(MAX(1,7+mrand(1,2)+bonus((int) player->piety.getCur())));

	creature->lasttime[LT_CHARMED].ltime = time(0);
	creature->lasttime[LT_CHARMED].interval = dur;

	creature->setFlag(creature->isPlayer() ? P_CHARMED : M_CHARMED);

	return(0);
}


//*********************************************************************
//						cmdEarthSmother
//*********************************************************************
// This command allows a dwarven cleric/paladin of Gradius to smother enemies

int cmdEarthSmother(Player* player, cmd* cmnd) {
	Creature* creature=0;
	Player	*pCreature=0;
	Monster *mCreature=0;
	long	i=0, t=0;
	int		chance=0, dmg=0;

	player->clearFlag(P_AFK);
	if(!player->ableToDoCommand())
		return(0);


	if(!player->isCt()) {
		if(!player->knowsSkill("smother")) {
			player->print("You lack the skills to smother people with earth.\n");
			return(0);
		}
		if(!player->alignInOrder()) {
			player->print("%s requires that you are neutral or light blue to do that.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
			return(0);
		}
	}
    if(!(creature = player->findVictim(cmnd, 1, true, false, "Smother whom?\n", "You don't see that here.\n")))
		return(0);

	pCreature = creature->getPlayer();
	mCreature = creature->getMonster();

	player->smashInvis();
	player->interruptDelayedActions();

	if(!player->canAttack(creature))
		return(0);

	if(mCreature && mCreature->getBaseRealm() == EARTH &&
		!player->checkStaff("Your attack would be ineffective.\n"))
		return(0);


	double level = player->getSkillLevel("smother");
	i = LT(player, LT_SMOTHER);
	t = time(0);
	if(i > t && !player->isCt()) {
		player->pleaseWait(i-t);
		return(0);
	}

	player->lasttime[LT_SMOTHER].ltime = t;
	player->updateAttackTimer();
	player->lasttime[LT_SMOTHER].interval = 120L;


	chance = ((int)(level - creature->getLevel()) * 20) + bonus((int) player->piety.getCur()) * 5 + 25;
	chance = MIN(chance, 80);
	if(creature->isEffected("resist-earth"))
		chance /= 2;
	if(!pCreature && creature->isEffected("immune-earth"))
		chance = 0;

	dmg = mrand((int)(level*3), (int)(level*4));
	if(creature->isEffected("resist-earth"))
		dmg = mrand(1, (int)level);
	if(player->getRoom()->flagIsSet(R_EARTH_BONUS) && player->getRoom()->flagIsSet(R_ROOM_REALM_BONUS))
		dmg = dmg*3/2;

	if(mCreature)
		mCreature->addEnemy(player);

	if(!player->isCt()) {
		if(mrand(1, 100) > chance) {
			player->print("You failed to earth smother %N.\n", creature);
			broadcast(player->getSock(), creature->getSock(), player->getRoom(), "%M tried to earth smother %N.", player, creature);
			creature->print("%M tried to earth smother you!\n", player);
			player->checkImprove("smother", false);
			return(0);
		}
		if(creature->chkSave(BRE, player, 0)) {
			player->printColor("^yYour earth-smother was interrupted!\n");
			creature->print("You manage to partially avoid %N's smothering earth.\n", player);
			dmg /= 2;
		}
	}

	player->printColor("You smothered %N for %s%d^x damage.\n", creature, player->customColorize("*CC:DAMAGE*").c_str(), dmg);
	player->checkImprove("smother", true);
	player->statistics.attackDamage(dmg, "earth smother");

	if(creature->isEffected("resist-earth"))
		player->print("%M resisted your smother!\n", creature);

	creature->printColor("%M earth-smothered you for %s%d^x damage!\n", player, creature->customColorize("*CC:DAMAGE*").c_str(), dmg);
	broadcast(player->getSock(), creature->getSock(), player->getRoom(), "%M earth-smothered %N!", player, creature);

	player->doDamage(creature, dmg, CHECK_DIE);
	return(0);
}

//*********************************************************************
//						cmdLayHands
//*********************************************************************
// This will allow paladins to lay on hands

int cmdLayHands(Player* player, cmd* cmnd) {
	Creature* creature=0;
	int		num=0;
	long	t=0, i=0;

	player->clearFlag(P_AFK);

	if(!player->knowsSkill("hands")) {
		player->print("You don't know how to lay your hands on someone properly.\n");
		return(0);
	}
	if(!player->isCt()) {
		if(player->getAdjustedAlignment() < ROYALBLUE) {
			player->print("You may only do that when your alignment is totally pure.\n");
			return(0);
		}

		i = player->lasttime[LT_LAY_HANDS].ltime;
		t = time(0);
		if(t-i < 3600L) {
			player->pleaseWait(3600L-t+i);
			return(0);
		}
	}


	// Lay on self
	if(cmnd->num == 1) {

		num = player->getLevel()*4 + mrand(1,6);
		player->print("You heal yourself with the power of %s.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
		player->print("You regain %d hit points.\n", MIN((player->hp.getMax() - player->hp.getCur()), num));


		player->doHeal(player, num);

		broadcast(player->getSock(), player->getParent(), "%M heals %sself with the power of %s.",
			player, player->himHer(), gConfig->getDeity(player->getDeity())->getName().c_str());

		player->print("You feel much better now.\n");
		player->checkImprove("hands", true);
		player->lasttime[LT_LAY_HANDS].ltime = t;
		player->lasttime[LT_LAY_HANDS].interval = 3600L;

	} else {
		// Lay hands on another player or monster

		cmnd->str[1][0] = up(cmnd->str[1][0]);
		creature = player->getRoom()->findCreature(player, cmnd->str[1], cmnd->val[1], false);
		if(!creature) {
			player->print("That person is not here.\n");
			return(0);
		}

		if(creature->pFlagIsSet(P_LINKDEAD) && creature->getClass() != LICH) {
			player->print("That won't work on %N right now.\n", creature);
			return(0);
		}

		if(!player->isCt() && creature->isUndead()) {
			player->print("That will not work on undead.\n");
			return(0);
		}

		if(creature->getAdjustedAlignment() < NEUTRAL) {
			player->print("%M is not pure enough of heart for that to work.\n", creature);
			return(0);
		}

		num = player->getLevel()*4 + mrand(1,6);
		player->print("You heal %N with the power of %s.\n", creature, gConfig->getDeity(player->getDeity())->getName().c_str());
		creature->print("%M lays %s hand upon your pate.\n", player, player->hisHer());
		creature->print("You regain %d hit points.\n", MIN((creature->hp.getMax() - creature->hp.getCur()), num));


		player->doHeal(creature, num);

		broadcast(player->getSock(), creature->getSock(), creature->getRoom(), "%M heals %N with the power of %s.",
			player, creature, gConfig->getDeity(player->getDeity())->getName().c_str());

		creature->print("You feel much better now.\n");

		player->lasttime[LT_LAY_HANDS].ltime = t;
		player->lasttime[LT_LAY_HANDS].interval = 600L;
		player->checkImprove("hands", true);

	}

	return(0);
}

//*********************************************************************
//						cmdPray
//*********************************************************************
// This function allows clerics, paladins, and druids to pray every 10
// minutes to increase their piety for a duration of 5 minutes.

int cmdPray(Player* player, cmd* cmnd) {
	long	i=0, t=0;
	int		chance=0;

	player->clearFlag(P_AFK);


	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("pray")) {
		player->print("You say a quick prayer.\n");
		return(0);
	}
	if(player->flagIsSet(P_PRAYED)) {
		player->print("You've already prayed.\n");
		return(0);
	}

	if(player->getClass() == DEATHKNIGHT) {
		if(player->isEffected("strength")) {
			player->print("Your magically enhanced strength prevents you from praying.\n");
			return(0);
		}
		if(player->isEffected("enfeeblement")) {
			player->print("Your magically reduced strength prevents you from praying.\n");
			return(0);
		}
	} else {
		if(player->isEffected("prayer")) {
			player->print("Your spiritual blessing prevents you from praying.\n");
			return(0);
		}
		if(player->isEffected("damnation")) {
			player->print("Your spiritual condemnation prevents you from praying.\n");
			return(0);
		}
	}

	i = player->lasttime[LT_PRAY].ltime;
	t = time(0);

	if(t - i < 600L) {
		player->pleaseWait(600L-t+i);
		return(0);
	}

	if(player->getClass()==DEATHKNIGHT)
		chance = MIN(85, (int)(player->getSkillLevel("pray") * 10) + (bonus((int) player->strength.getCur()) * 5));
	else
		chance = MIN(85, (int)(player->getSkillLevel("pray") * 20) + bonus((int) player->piety.getCur()));

	if(mrand(1, 100) <= chance) {
		player->setFlag(P_PRAYED);
		player->lasttime[LT_PRAY].ltime = t;

		if(player->getClass() != DEATHKNIGHT) {
			player->print("You feel extremely pious.\n");
			broadcast(player->getSock(), player->getParent(), "%M bows %s head in prayer.", player, player->hisHer());
			player->piety.addCur(50);
			player->lasttime[LT_PRAY].interval = 450L;
		} else {
			player->print("The evil in your soul infuses your body with power.\n");
			broadcast(player->getSock(), player->getParent(), "%M glows with evil.", player);
			player->strength.addCur(30);
			player->computeAC();
			player->computeAttackPower();
			player->lasttime[LT_PRAY].interval = 240L;
		}
		player->checkImprove("pray", true);
	} else {
		if(player->getClass() != DEATHKNIGHT) {
			player->print("Your prayers were not answered.\n");
		} else {
			player->print("The evil in your soul fails to aid you.\n");
		}
		player->checkImprove("pray", false);
		broadcast(player->getSock(), player->getParent(), "%M prays.", player);
		player->lasttime[LT_PRAY].ltime = t - 590L;
	}

	return(0);
}


bool Creature::kamiraLuck(Creature *attacker) {
	int	chance=0;

	if(!(cClass == CLERIC && deity == KAMIRA) && !isCt())
		return(false);

	chance = (level / 10)+3;
	if(mrand(1,100) <= chance) {
		broadcast(getSock(), getRoom(), "^YA feeling of deja vous comes over you.");
		printColor("^YThe luck of %s was with you!\n", gConfig->getDeity(KAMIRA)->getName().c_str());
		printColor("^C%M missed you.\n", attacker);
		attacker->print("You thought you hit, but you missed!\n");
		return(true);
	}
	return(false);
}



int getTurnChance(Creature* player, Creature* target) {
	double	level = 0.0;
	int		chance=0, bns=0;


	level = player->getSkillLevel("turn");

	switch (player->getDeity()) {
	case CERIS:
		if(player->getAdjustedAlignment() == NEUTRAL)
			level+=2;
		if(player->getAdjustedAlignment() == ROYALBLUE || player->getAdjustedAlignment() == BLOODRED)
			level-=2;
		break;
	case LINOTHAN:
	case ENOCH:
		if(player->getAdjustedAlignment() < NEUTRAL)
			level -=4;
		break;
	default:
		break;
	}

	level = MAX(1,level);


	bns = bonus((int)player->piety.getCur());

	chance = (int)((level - target->getLevel())*20) +
			bns*5 + (player->getClass() == PALADIN ? 15:25);
	chance = MIN(chance, 80);

	if(target->isPlayer()) {
		if(player->isDm())
			chance = 101;
	} else {
		if(target->flagIsSet(M_SPECIAL_UNDEAD))
			chance = MIN(chance, 15);
	}


	return(chance);
}


//*********************************************************************
//						cmdTurn
//*********************************************************************

int cmdTurn(Player* player, cmd* cmnd) {
	Creature* target=0;
	long	i=0, t=0;
	int		m=0, dmg=0, dis=5, chance=0;
	int		roll=0, disroll=0, bns=0;


	player->clearFlag(P_AFK);

	bns = bonus((int)player->piety.getCur());

	if(!player->ableToDoCommand())
		return(0);

	if(!player->isCt() && !player->knowsSkill("turn")) {
		player->print("You don't know how to turn undead.\n");
		return(0);
	}

    if(!(target = player->findVictim(cmnd, 1, true, false, "Turn whom?\n", "You don't see that here.\n")))
   		return(0);

	
	if(!player->canAttack(target))
		return(0);
	if(target->isMonster()) {
		if(player->flagIsSet(P_LAG_PROTECTION_SET)) {    // Activates lag protection.
			player->setFlag(P_LAG_PROTECTION_ACTIVE);
		}

		if(	!target->isUndead() &&
			!player->checkStaff("You may only turn undead monsters.\n"))
			return(0);

	} else {

		if( !target->isUndead() &&
			!player->checkStaff("%M is not undead.\n", target))
			return(0);

	}

	player->smashInvis();
	player->interruptDelayedActions();

	i = LT(player, LT_TURN);
	t = time(0);

	if(i > t && !player->isDm()) {
		player->pleaseWait(i-t);
		return(0);
	}


	if(target->isMonster())
		target->getMonster()->addEnemy(player);

	player->lasttime[LT_TURN].ltime = t;
	player->updateAttackTimer(false);

	if(	(player->getDeity() == ENOCH || player->getDeity() == LINOTHAN) &&
		player->getLevel() >= 10
	) {
		if(LT(player, LT_HOLYWORD) <= t) {
			player->lasttime[LT_HOLYWORD].ltime = t;
			player->lasttime[LT_HOLYWORD].interval = 3L;
		}
	}

	player->lasttime[LT_TURN].interval = 30L;


	// determine damage turn will do
	dmg = MAX(1, target->hp.getCur() / 2);


	switch(player->getDeity()) {
	case CERIS:
		if(player->getAdjustedAlignment() == NEUTRAL) {
			dis = 10;
			player->print("The power of Ceris flows through you.\n");
		}
		if(player->getAdjustedAlignment() == ROYALBLUE || player->getAdjustedAlignment() == BLOODRED) {
			dis = -1;
			dmg /= 2;
		}
		break;
	case LINOTHAN:
	case ENOCH:
		if(player->getAdjustedAlignment() < NEUTRAL) {
			dis = -1;
			dmg /= 2;
		}
		break;

	default:
		break;
	}

	chance = getTurnChance(player, target);

	roll = mrand(1,100);

	if(roll > chance && !player->isStaff()) {
		player->print("You failed to turn %N.\n", target);
		player->checkImprove("turn", false);
		if(target->mFlagIsSet(M_SPECIAL_UNDEAD))
			player->print("%M greatly resisted your efforts to turn %s!\n",
			      target, target->himHer());
		broadcast(player->getSock(), player->getParent(), "%M failed to turn %N.", player, target);
		return(0);
	}

	disroll = mrand(1,100);

	if((disroll < (dis + bns) && !target->flagIsSet(M_SPECIAL_UNDEAD)) || player->isDm()) {
		player->printColor("^BYou disintegrated %N.\n", target);

		broadcast(player->getSock(), player->getParent(), "^B%M disintegrated %N.", player, target);
		// TODO: SKILLS: add a bonus to this
		player->checkImprove("turn", true);
		if(target->isMonster())
			target->getMonster()->adjustThreat(player, target->hp.getCur());
		//player->statistics.attackDamage(target->hp.getCur(), "turn undead");

		target->die(player);
	} else {

		m = MIN(target->hp.getCur(), dmg);
		//player->statistics.attackDamage(dmg, "turn undead");

		if(target->isMonster())
			target->getMonster()->adjustThreat(player, m);

		player->printColor("^YYou turned %N for %d damage.\n", target, dmg);
		player->checkImprove("turn", true);

		broadcast(player->getSock(), player->getParent(), "^Y%M turned %N.", player, target);
		player->doDamage(target, dmg, CHECK_DIE);

	}

	return(0);

}

//*********************************************************************
//						cmdRenounce
//*********************************************************************
// This function allows paladins and death knights to renounce one
// another with the power of their respective gods, doing damage or
// destroying one another utterly.  --- TC

int cmdRenounce(Player* player, cmd* cmnd) {
	Creature* target=0;
	long	i=0, t=0;
	int		chance=0, dmg=0;

	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("renounce")) {
		player->print("You lack the understanding to renounce.\n");
		return(0);
	}
    
    if(!(target = player->findVictim(cmnd, 1, true, false, "Renounce whom?\n", "You don't see that here.\n")))
   		return(0);

	double level = player->getSkillLevel("renounce");
	if(target->isMonster()) {
		Monster* mTarget = target->getMonster();
		if( (!player->isCt()) &&
			(((player->getClass() == PALADIN) && (target->getClass() != DEATHKNIGHT)) ||
			((player->getClass() == DEATHKNIGHT) && (target->getClass() != PALADIN)))
		) {
			player->print("%s is not of your opposing class.\n", target->upHeShe());
			return(0);
		}

		player->smashInvis();
		player->interruptDelayedActions();

		if(player->flagIsSet(P_LAG_PROTECTION_SET)) {    // Activates lag protection.
			player->setFlag(P_LAG_PROTECTION_ACTIVE);
		}

		i = LT(player, LT_RENOUNCE);
		t = time(0);

		if(i > t && !player->isCt()) {
			player->pleaseWait(i-t);
			return(0);
		}

		if(!player->checkAttackTimer(true))
			return(0);

		if(!player->canAttack(target))
			return(0);

		if(mTarget)
			mTarget->addEnemy(player);

		player->updateAttackTimer(false);
		player->lasttime[LT_RENOUNCE].ltime = t;
		player->lasttime[LT_RENOUNCE].interval = 60L;

		chance = ((int)(level - target->getLevel())*20) + bonus((int)player->piety.getCur())*5 + 25;
		if(target->flagIsSet(M_PERMENANT_MONSTER))
			chance -= 15;
		chance = MIN(chance, 90);
		if(player->isDm())
			chance = 101;

		if(mrand(1,100) > chance) {
			player->print("Your god refuses to renounce %N.\n", target);
			player->checkImprove("renounce", false);
			broadcast(player->getSock(), player->getParent(), "%M tried to renounce %N.", player, target);
			return(0);
		}


		if(!player->isCt()) {
			if(target->chkSave(DEA, player, 0)) {
				player->printColor("^yYour renounce failed!\n");
				player->checkImprove("renounce", false);
				target->print("%M failed renounce you.\n", player);
				return(0);
			}
		}

		if(mrand(1,100) > 90 - bonus((int)player->piety.getCur()) || player->isDm()) {
			player->print("You destroy %N with your faith.\n", target);
			player->checkImprove("renounce", true);
			broadcast(player->getSock(), player->getParent(), "The power of %N's faith destroys %N.",
				player, target);
			mTarget->adjustThreat(player, target->hp.getCur());

			//player->statistics.attackDamage(target->hp.getCur(), "renounce");
			target->die(player);
		}
		else {
			dmg = MAX(1, target->hp.getCur() / 2);
			//player->statistics.attackDamage(dmg, "renounce");

			player->printColor("You renounced %N for %s%d^x damage.\n", target, player->customColorize("*CC:DAMAGE*").c_str(), dmg);
			player->checkImprove("renounce", true);
			//target->print("%M renounced you for %d damage!\n", player, dmg);
			broadcast(player->getSock(), player->getParent(), "%M renounced %N.", player, target);
			player->doDamage(target, dmg, CHECK_DIE);
		}

		return(0);
	} else {
		if( (!player->isCt()) &&
			(((player->getClass() == PALADIN) && (target->getClass() != DEATHKNIGHT)) ||
			((player->getClass() == DEATHKNIGHT) && (target->getClass() != PALADIN)))
		) {
			player->print("%s is not of your opposing class.\n", target->upHeShe());
			return(0);
		}
		if(!player->canAttack(target))
			return(0);


		player->smashInvis();
		player->interruptDelayedActions();

		i = LT(player, LT_RENOUNCE);
		t = time(0);

		if(i > t && !player->isCt()) {
			player->pleaseWait(i-t);
			return(0);
		}

		if(!player->checkAttackTimer())
			return(0);

		player->updateAttackTimer(false);
		player->lasttime[LT_RENOUNCE].ltime = t;
		player->lasttime[LT_RENOUNCE].interval = 60L;

		chance = ((int)(level - target->getLevel())*20) +
			bonus((int)player->piety.getCur())*5 + 25;
		chance = MIN(chance, 90);
		if(player->isDm())
			chance = 101;
		if(mrand(1,100) > chance) {
			player->print("Your god refuses to renounce %N.\n", target);
			player->checkImprove("renounce", false);
			broadcast(player->getSock(), target->getSock(), player->getRoom(),
				"%M tried to renounce %N.", player, target);
			target->print("%M tried to renounce you!\n", player);
			return(0);
		}


		if(!target->isCt()) {
			if(target->chkSave(DEA, player, 0)) {
				player->printColor("^yYour renounce failed!\n");
				player->checkImprove("renounce", false);
				target->print("%M failed renounce you.\n", player);
				return(0);
			}
		}

		if(mrand(1,100) > 90 - bonus((int)player->piety.getCur()) || player->isDm()) {
			player->print("You destroyed %N with your faith.\n", target);
			player->checkImprove("renounce", true);
			target->print("The power of %N's faith destroys you!\n", player);
			broadcast(player->getSock(), target->getSock(), player->getRoom(), "%M destroys %N with %s faith!",
				player, target, target->hisHer());
			//player->statistics.attackDamage(target->hp.getCur(), "renounce");
			target->die(player);

		} else {
			dmg = MAX(1, target->hp.getCur() / 2);
			//player->statistics.attackDamage(dmg, "renounce");
			player->printColor("You renounced %N for %s%d^x damage.\n", target, player->customColorize("*CC:DAMAGE*").c_str(), dmg);
			player->checkImprove("renounce", true);
			target->printColor("%M renounced you for %s%d^x damage.\n", player, target->customColorize("*CC:DAMAGE*").c_str(), dmg);
			broadcast(player->getSock(), target->getSock(), player->getRoom(), "%M renounced %N!", player, target);
			player->doDamage(target, dmg, CHECK_DIE);
		}

		return(0);
	}

}

//*********************************************************************
//						cmdHolyword
//*********************************************************************
// This function will allow a cleric of Enoch to use a holyword power
// upon reaching level 13 or higher. ---TC

int cmdHolyword(Player* player, cmd* cmnd) {
	Creature* target=0;
	long	i=0, t=0;
	int		chance=0, dmg=0, alnum=0;

	if(!player->ableToDoCommand())
		return(0);

	if(!player->isCt()) {
		if(!player->knowsSkill("holyword")) {
			printf("You haven't learned how to pronounce a holy word.\n");
			return(0);
		}
		// Must be VERY good to pronounce a holy word.
		if(player->getAdjustedAlignment() < ROYALBLUE) {
			player->print("You are not good enough in your heart to pronounce a holy word.\n");
			return(0);
		}
	}


    if(!(target = player->findVictim(cmnd, 1, true, false, "Pronounce a holy word on whom?\n", "You don't see that here.\n")))
   		return(0);

	double level = player->getSkillLevel("holyword");
	if(target->isMonster()) {
		Monster* mTarget = target->getMonster();
		if(target->getAdjustedAlignment() >= NEUTRAL && !player->isCt()) {
			player->print("A holy word cannot be pronounced on a good creature.\n");
			return(0);
		}

		if(!player->canAttack(target))
			return(0);

		// Activates lag protection.
		if(player->flagIsSet(P_LAG_PROTECTION_SET))
			player->setFlag(P_LAG_PROTECTION_ACTIVE);


		player->smashInvis();
		player->interruptDelayedActions();


		i = LT(player, LT_HOLYWORD);
		t = time(0);

		if(i > t && !player->isDm()) {
			player->pleaseWait(i-t);
			return(0);
		}

		if(mTarget)
			mTarget->addEnemy(player);

		player->updateAttackTimer(false);
		player->lasttime[LT_HOLYWORD].ltime = t;
		if(LT(player, LT_TURN) <= t) {
			player->lasttime[LT_TURN].ltime = t;
			player->lasttime[LT_TURN].interval = 3L;
		}
		player->lasttime[LT_SPELL].ltime = t;
		player->lasttime[LT_HOLYWORD].interval = 240L;

		chance = ((int)(level - target->getLevel())*20) +
				bonus((int)player->piety.getCur())*5 + 25;

		chance = MIN(chance, 90);
		if(player->isDm())
			chance = 101;

		if(mrand(1,100) > chance) {
			player->print("Your holy word is ineffective on %N.\n", target);
			player->checkImprove("holyword", false);
			broadcast(player->getSock(), player->getParent(), "%M tried to pronounce a holy word on %N.", player, target);
			return(0);
		}

		if(!target->isCt()) {
			if(target->chkSave(LCK, player, 0)) {
				player->printColor("^yYour holyword failed!\n");
				player->checkImprove("holyword", false);
				target->print("You avoided %N's holy word.\n", player);
				return(0);
			}
		}

		if((mrand(1,100) > (90 - bonus((int)player->piety.getCur()))) || (player->isDm())) {
			player->print("Your holy word utterly destroys %N.\n", target);
			player->checkImprove("holyword", true);
			broadcast(player->getSock(), player->getParent(), "%M's holy word utterly destroys %N.",
				player, target);
			if(mTarget)
				mTarget->adjustThreat(player, target->hp.getCur());
			//player->statistics.attackDamage(target->hp.getCur(), "holyword");
			target->die(player);
		} else {
			alnum = player->getAlignment() / 100;
			dmg = mrand((int)level + alnum, (int)(level*4)) + bonus((int)player->piety.getCur());
			//player->statistics.attackDamage(dmg, "holyword");

			player->print("Your holy word does %d damage to %N.\n", dmg, target);
			player->checkImprove("holyword", true);
			target->stun((bonus((int)player->piety.getCur()) + mrand(2,6)) );

			broadcast(player->getSock(), player->getParent(), "%M pronounces a holy word on %N.", player, target);

			player->doDamage(target, dmg, CHECK_DIE);
		}

	} else {

		if(!player->isCt()) {
			if(target->getAdjustedAlignment() >= NEUTRAL) {
				player->print("%s is not evil enough for your holy word to be effective.\n", target->upHeShe());
				return(0);
			}
			if(player->getDeity() == target->getDeity()) {
				player->print("Surely you do not want to harm a fellow follower of %s.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
				return(0);
			}

			if(!player->canAttack(target))
				return(0);
		}

		player->smashInvis();
		player->interruptDelayedActions();

		i = LT(player, LT_HOLYWORD);
		t = time(0);


		if(i > t && !player->isDm()) {
			player->pleaseWait(i-t);
			return(0);
		}

		player->updateAttackTimer(false);
		player->lasttime[LT_HOLYWORD].ltime = t;
		player->lasttime[LT_SPELL].ltime = t;
		if(LT(player, LT_TURN) <= t) {
			player->lasttime[LT_TURN].ltime = t;
			player->lasttime[LT_TURN].interval = 3L;
		}
		player->lasttime[LT_HOLYWORD].interval = 240L;

		chance = ((int)(level - target->getLevel())*20) +
				bonus((int)player->piety.getCur())*5 + 25;
		chance = MIN(chance, 90);
		if(player->isDm())
			chance = 101;
		if(mrand(1,100) > chance) {
			player->print("Your holy word is ineffective on %N.\n", target);
			player->checkImprove("holyword", false);
			broadcast(player->getSock(), target->getSock(), player->getRoom(),
				"%M tried to pronounce a holy word on %N.", player, target);
			target->print("%M tried to pronounce a holy word you!\n", player);
			return(0);
		}

		if(!target->isCt()) {
			if(target->chkSave(LCK, player, 0)) {
				player->printColor("^yYour holyword failed!\n");
				player->checkImprove("holyword", false);
				target->print("You avoided %N's holy word.\n", player);
				return(0);
			}
		}

		if((mrand(1,100) > (90 - bonus((int)player->piety.getCur()))) || player->isDm()) {

			player->print("Your holy word utterly destroys %N.\n", target);
			player->checkImprove("holyword", true);
			target->print("You are utterly destroyed by %N's holy word!\n", player);
			broadcast(player->getSock(), target->getSock(), player->getRoom(),
				"%M utterly destroys %N with %s holy word!", player, target, target->hisHer());
			player->statistics.attackDamage(target->hp.getCur(), "holyword");
			target->die(player);

		} else {
			alnum = player->getAlignment() / 100;
			dmg = (mrand((int)level + alnum, (int)(level*4))
			       + bonus((int)player->piety.getCur())) - bonus((int)target->piety.getCur());
			player->statistics.attackDamage(dmg, "holyword");

			player->printColor("Your holy word does %s%d^x damage to %N.\n", player->customColorize("*CC:DAMAGE*").c_str(), dmg, target);
			player->checkImprove("holyword", true);
			target->stun((bonus((int)player->piety.getCur()) + mrand(2,6)) );

			target->printColor("%M pronounced a holy word on you for %s%d^x damage.\n", player, target->customColorize("*CC:DAMAGE*").c_str(), dmg);
			broadcast(player->getSock(), target->getSock(), player->getRoom(),
				"%M pronounced a holy word on %N!", player, target);
			player->doDamage(target, dmg, CHECK_DIE);
		}

	}

	return(0);
}


//*********************************************************************
//						cmdBandage
//*********************************************************************

int cmdBandage(Player* player, cmd* cmnd) {
	Creature* creature=0;
	Object	*object=0;
	int		heal=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(player->inCombat()) {
		player->print("Not while you are fighting.\n");
		return(0);
	}

	if(player->getClass() == BUILDER) {
		if(!player->canBuildObjects()) {
			player->print("You are not allowed to use items.\n");
			return(0);
		}
		if(!player->checkBuilder(player->parent_rom)) {
			player->print("Error: Room number not inside any of your alotted ranges.\n");
			return(0);
		}
	}

	if(!player->checkAttackTimer())
		return(0);

	if(cmnd->num < 2) {
		player->print("Syntax: bandage (bandagetype)\n");
		player->print("        bandage (target) (bandagetype)\n");
		return(0);
	}

	if(cmnd->num == 2) {

		object = findObject(player, player->first_obj, cmnd);
		if(!object) {
			player->print("Use what bandages?\n");
			return(0);
		}

		if(player->getClass() == LICH) {
			player->print("Liches don't bleed. Stop wasting your time and regenerate!\n");
			object->decShotsCur();
			return(0);
		}

		if(object->getType() != BANDAGE) {
			player->print("You cannot bandage yourself with that.\n");
			return(0);
		}

		if(object->getShotsCur() < 1) {
			player->print("You can't bandage yourself with that. It's used up.\n");
			return(0);
		}

		if(player->hp.getCur() >= player->hp.getMax()) {
			player->print("You are at full health! You don't need to bandage yourself!\n");
			return(0);
		}


		if(player->isEffected("stoneskin")) {
			player->print("Your stoneskin spell prevents bandaging efforts.\n");
			return(0);
		}

		object->decShotsCur();

		heal = object->damage.roll();

		heal = player->hp.increase(heal);
		player->print("You regain %d hit points.\n", heal);


		if(object->getShotsCur() < 1)
			player->printColor("Your %s %s all used up.\n", object->name,
			      (object->flagIsSet(O_SOME_PREFIX) ? "are":"is"));

		broadcast(player->getSock(), player->getParent(), "%M bandages %sself.",
			player, player->himHer());

		player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);

	} else {

		if(!cmnd->str[2][0]) {
			player->print("Which bandages will you use?\n");
			player->print("Syntax: bandage (target)[#] (bandages)[#]\n");
			return(0);
		}


		cmnd->str[2][0] = up(cmnd->str[2][0]);
		creature = player->getRoom()->findCreature(player, cmnd->str[1], cmnd->val[1], false);
		if(!creature) {
			player->print("That person is not here.\n");
			return(0);
		}



		object = findObject(player, player->first_obj, cmnd, 2);
		if(!object) {
			player->print("You don't have that in your inventory.\n");
			return(0);
		}

		if(object->getType() != BANDAGE) {
			player->print("You cannot bandage %N with that.\n", creature);
			return(0);
		}

		if(object->getShotsCur() < 1) {
			player->print("You can't bandage %N with that. It's used up.\n", creature);
			return(0);
		}


		if(creature->hp.getCur() == creature->hp.getMax()) {
			player->print("%M doesn't need to be bandaged right now.\n", creature);
			return(0);
		}

		if(creature->getClass() == LICH) {
			player->print("The aura of death around %N causes bandaging to fail.\n",creature);
			object->decShotsCur();
			return(0);
		}

		if(creature->isPlayer() && creature->inCombat()) {
			player->print("You cannot bandage %N while %s's fighting a monster.\n", creature, creature->heShe());
			return(0);
		}

		if(creature->getType() !=PLAYER && !creature->isPet() && creature->getMonster()->nearEnemy()
				&& !creature->getMonster()->isEnemy(player)) {
			player->print("Not while %N is in combat with someone.\n", creature);
			return(0);
		}


		if(creature->isPlayer() && creature->isEffected("stoneskin")) {
			player->print("%M's stoneskin spell resists your bandaging efforts.\n", creature);
			return(0);
		}

		if(creature->pFlagIsSet(P_LINKDEAD)) {
			player->print("%M doesn't want to be bandaged right now.\n", creature);
			return(0);
		}


		object->decShotsCur();

		heal = object->damage.roll();

		heal = creature->hp.increase(heal);

		player->print("%M regains %d hit points.\n", creature, heal);


		if(object->getShotsCur() < 1)
			player->printColor("Your %s %s all used up.\n", object->name,
			      (object->flagIsSet(O_SOME_PREFIX) ? "are":"is"));

		broadcast(player->getSock(), creature->getSock(), player->getRoom(), "%M bandages %N.", player, creature);
		player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
	}

	return(0);
}

//*********************************************************************
//						splHallow
//*********************************************************************

int splHallow(Creature* player, cmd* cmnd, SpellData* spellData) {
	int strength = 10;
	long duration = 600;

	if(noPotion(player, spellData))
		return(0);

	if(spellData->how == CAST) {
		if(player->getClass() != CLERIC && !player->isCt()) {
			player->print("Only clerics may cast that spell.\n");
			return(0);
		}
		player->print("You cast a hallow spell.\n");
		broadcast(player->getSock(), player->getParent(), "%M casts a hallow spell.", player);
	}

	if(player->getRoom()->hasPermEffect("hallow")) {
		player->print("The spell didn't take hold.\n");
		return(0);
	}

	if(spellData->how == CAST) {
		if(player->getRoom()->magicBonus())
			player->print("The room's magical properties increase the power of your spell.\n");
	}

	player->getRoom()->addEffect("hallow", duration, strength, player, true, player);
	return(1);
}

//*********************************************************************
//						splUnhallow
//*********************************************************************

int splUnhallow(Creature* player, cmd* cmnd, SpellData* spellData) {
	int strength = 10;
	long duration = 600;

	if(noPotion(player, spellData))
		return(0);

	if(spellData->how == CAST) {
		if(player->getClass() != CLERIC && !player->isCt()) {
			player->print("Only clerics may cast that spell.\n");
			return(0);
		}
	}
	player->print("You cast an unhallow spell.\n");
	broadcast(player->getSock(), player->getParent(), "%M casts an unhallow spell.", player);

	if(player->getRoom()->hasPermEffect("unhallow")) {
		player->print("The spell didn't take hold.\n");
		return(0);
	}

	if(spellData->how == CAST) {
		if(player->getRoom()->magicBonus())
			player->print("The room's magical properties increase the power of your spell.\n");
	}

	player->getRoom()->addEffect("unhallow", duration, strength, player, true, player);
	return(1);
}
