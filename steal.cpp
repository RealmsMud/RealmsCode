/*
 * steal.cpp
 *   Routines all involved with stealing by both monsters and players
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
#include "unique.h"

//*********************************************************************
//						steal_gold
//*********************************************************************

void steal_gold(Player* player, Creature* creature) {
	int chance=0, amt=0;
	if(!player)
		return;
	if(!creature)
		return;
	if(!player->knowsSkill("steal")) {
		player->print("You'd surely be caught if you tried stealing something.\n");
		return;
	}
	if(	player->getSecondClass() == THIEF &&
		(player->getClass() == FIGHTER || player->getClass() == MAGE) )
	{
		player->print("Only pure thieves and thief/mages are able attempt to steal gold.\n");
		return;
	}

	if(player->getClass() != THIEF && !player->isCt()) {
		player->print("Only thieves are skilled enough to steal gold.\n");
		return;
	}


	if(creature->isPlayer() && creature->isStaff() && !player->isDm())
	{
		player->print("Stealing from an immortal is not a good thing for your health.\n");
		creature->print("%M tried to steal something from you.\n", player);
		return;
	}

	if(creature->isPlayer() && !player->isCt() && (player->getLevel() > creature->getLevel() + 6))
	{
		player->print("%M is probably poor. Go look for a better mark.\n", creature);
		return;
	}

	int level = (int)player->getSkillLevel("steal");


	if(creature->isMonster()) {
		if(creature->flagIsSet(M_UNKILLABLE) && !player->checkStaff("%s don't have anything you want.\n", creature->upHeShe()))
			return;
		if( creature->getAsMonster()->isEnemy(player) &&
			!player->checkStaff("Not while %s's attacking you.\n", creature->heShe()))
			return;

	} else {
		if(	player->getRoomParent()->isPkSafe() &&
			!player->isCt() &&
			!creature->flagIsSet(P_OUTLAW))
		{
			player->print("No stealing allowed in this room.\n");
			return;
		}


		if(!player->flagIsSet(P_PLEDGED) || !creature->flagIsSet(P_PLEDGED)) {
			if(!player->flagIsSet(P_CHAOTIC) && !player->isCt() && !creature->flagIsSet(P_OUTLAW)) {
				player->print("Sorry, you're lawful.\n");
				return;
			}
			if(	!creature->flagIsSet(P_CHAOTIC) &&
				!creature->flagIsSet(P_OUTLAW) &&
				!player->isCt())
			{
				player->print("Sorry, that player is lawful.\n");
				return;
			}
		}

		if(!player->isCt() && player->getLevel() > 10 && creature->getLevel() < 4 && creature->isPlayer() && !creature->flagIsSet(P_OUTLAW)) {
			player->print("Steal from someone lower level.\n");
			return;
		}

	}
	if(player->isBlind()) {
		player->printColor("^CHow do you do that? You're blind!\n");
		return;
	}

	if(	creature->isEffected("mist") && !
		!player->checkStaff("You cannot steal from a misted creature.\n") )
		return;


	chance = 5*(level - creature->getLevel()) + bonus((int)player->dexterity.getCur())*5 + bonus(player->getLuck() / 4)*5 + mrand(-15, 15);

	if(player->flagIsSet(P_HIDDEN))
		chance += 10;
	if((creature->isEffected("blindness")) && creature->isMonster())
		chance+=5;
	if(creature->pFlagIsSet(P_OUTLAW))
		chance += 10;
	chance = MIN(chance, 60);

	if(creature->getLevel() > level + 2)
		chance = 0;

	if(player->isDm())
		chance = 101;

	player->statistics.attemptSteal();
	player->interruptDelayedActions();

	if(mrand(1,100) < chance) {
		int formula;
		player->print("You succeeded.\n");
		player->checkImprove("steal", true);
		player->statistics.steal();
		if(creature->coins.isZero()) {
			player->print("%M doesn't have any coins!\n",creature);
			return;
		}

		formula = mrand(creature->coins[GOLD]/10, creature->coins[GOLD]/3);
		amt = MIN(creature->coins[GOLD], MAX(1, formula));
		player->print("You grabbed %d coins.\n", amt);
		creature->coins.sub(amt, GOLD);
		player->coins.add(amt, GOLD);

		if(creature->isPlayer())
			log_immort(false,player, "%s stole %d gold from %s.\n", player->name, amt, creature->name);
		if(creature->getAsMonster())
			gServer->logGold(GOLD_IN, player, Money(amt, GOLD), creature, "StealGold");


	} else {
		player->print("You failed to grab any coins!\n");
		player->checkImprove("steal", false);
		player->smashInvis();

		if(creature->isPlayer()) {
			if(!player->isEffected("blindness"))
				creature->print("%M tried to steal some of your gold!\n",player);
			else
				creature->print("Someone tried to steal something from you.\n");
		} else
			creature->getAsMonster()->addEnemy(player);

	}
}

//*********************************************************************
//						get_steal_chance
//*********************************************************************

int get_steal_chance(Player* player, Creature* target, Object* object) {
	Player	*pTarget = target->getAsPlayer();
	int		chance=0, classmod=0, bulk=0, weight=0, level=0;

	if(!target || !object || !player)
		return(0);

	if(player->isStaff())
		return(101);

	// return 0 right away so there is absolutely no chance of stealing.
	if(	object->flagIsSet(O_NO_STEAL) ||
		object->flagIsSet(O_BODYPART) ||
		!Lore::canHave(player, object) ||
		object->flagIsSet(O_NO_DROP) ||
		object->flagIsSet(O_STARTING) ||
		!Unique::canGet(player, object, true) ||
		// no stealing uniques from monsters
		(Unique::is(object) && !target->getAsPlayer()) ||
		object->getQuestnum()
	)
		return(0);

	// mob specific nosteal checks
	if(	target->isMonster() &&
		(	target->flagIsSet(M_CANT_BE_STOLEN_FROM) ||
			target->flagIsSet(M_TRADES)
		)
	)
		return(0);


	// Level modifications for multi-classed thieves
	level = (int)player->getSkillLevel("steal");
	if(player->getSecondClass() == THIEF && player->getClass() == FIGHTER)
		level = MAX(1, level-3);
	if(player->getSecondClass() == THIEF && player->getClass() == MAGE)
		level = MAX(1, level-3);
	if(player->getClass() == THIEF && player->getSecondClass() == MAGE)
		level = MAX(1, level-3);
	if(player->getClass() == CLERIC && player->getDeity() == KAMIRA)
		level = MAX(1, level-3);

	// Base success % chance set here
	chance = (player->getClass() == THIEF) ? 4*level : 3*level;

	if(player->getClass() == CLERIC && player->getDeity() == KAMIRA)
		chance += bonus((int)player->piety.getCur())*4;
	else
		chance += bonus((int)player->dexterity.getCur())*4;

	chance -= crtAwareness(target)*2; // Awareness = (int bonus + dex bonus)/2

	// Modify chance based on class of target.
	//*******************************************
	switch(target->getClass()) {
	case THIEF:		// A thief knows their trade and how to protect themself.
		if(pTarget && pTarget->getSecondClass() == MAGE)
			classmod = target->getLevel();
		else
			classmod= 3 * target->getLevel();
		break;
	case ASSASSIN:  // Roguish classes always are aware of their belongings,
	case ROGUE:     // thus they are always more aware.
	case BARD:
		classmod = 2 * target->getLevel();
		break;
	case MAGE:
		if(pTarget && (pTarget->getSecondClass() == THIEF || pTarget->getSecondClass() == ASSASSIN))
			classmod = target->getLevel();
		else
			classmod = 0;
		break;
	case FIGHTER:
		if(pTarget && pTarget->getSecondClass() == THIEF)
			classmod = target->getLevel();
		else
			classmod = 0;
		break;
	case LICH:      // Lichs' unnatural aura messes with thief.
	case PUREBLOOD:   // Vampires, rangers, druids, monks, and werewolves
	case RANGER:    // are always more accutely aware of their surroundings
	case DRUID:     // than any other non-roguish class.
	case WEREWOLF:
	case MONK:
		classmod = target->getLevel();
		break;
	default:
		classmod = 0;
		break;
	}

	chance -= classmod;
	//******************************************

	// If target is higher level, chance is -15% per level higher.
	if(target->getLevel() > level)
		chance -= 15*((int)target->getLevel() - level);

	// Camouflaged player has increased chance to steal.
	if(player->isEffected("camouflage") && !player->flagIsSet(P_HIDDEN))
		chance += 20;

	if(target->isPlayer()) {
		// Blinded players are way easier to steal from.
		if(target->isEffected("blindness"))
			chance += 30;

		// Sucks to be an outlaw and be around thieves!
		if(target->flagIsSet(P_OUTLAW))
			chance += 50;

		// Against PLAYERS, hiding first is a bonus.
		if(player->flagIsSet(P_HIDDEN))
			chance += 10;

		// Playerts fighting mobs are moving around alot.
		if(target->inCombat())
			chance /= 2;

		// Being unconscious is not a good idea around thieves.
		if(target->flagIsSet(P_UNCONSCIOUS))
			chance += 50;

		// Niether is being held magically...
		if(target->isEffected("hold-person"))
			chance += 25;

		// ...or stunned...
		if(target->flagIsSet(P_STUNNED))
			chance += 25;
	}

	// Bulk and weight of object make a difference.
	// The greater value is subtracted from the chance.
	// Dividing by 2 makes this less of a hinderance to game mechanics.
	bulk = object->getActualBulk()/2;
	weight = object->getActualWeight()/2;
	if(weight > bulk)
		chance -= weight/2;
	else
		chance -= bulk/2;

	// Maximum chance to steal ever is 90%
	// There's always a 10% chance to fail.
	chance = MIN(chance, 90);

	// If chance is less than 0, and thief is level 10+,
	// then there is always a 1% chance to succeed
	// against another player.
	if(	target->isPlayer() &&
		level >= 10 &&
		chance < 0
	)
		chance = MAX(1,chance);

	return(chance);
}


//*********************************************************************
//						cmdSteal
//*********************************************************************
// this function allows a player to steal from a monster or another player

int cmdSteal(Player* player, cmd* cmnd) {
	Creature* target=0;
	Monster	*mTarget=0;
	Player* pTarget=0;
	BaseRoom* room = player->getRoomParent();
	Object		*object=0;
	long		i=0, t = time(0);
	int			cantSteal=0;
	int			caught=0, chance=0, roll=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("steal")) {
		player->print("You lack the skills to steal from someone without being caught.\n");
		return(0);
	}

	if(!player->isCt()) {
		// Cannot steal if sitting.
		if(player->flagIsSet(P_SITTING)) {
			player->print("You have to stand up first.\n");
			return(0);
		}
		if(player->isEffected("mist")) {
			player->print("You must be in corporeal form to steal!\n");
			return(0);
		}

		// Cannot steal if stunned. Keeps people from stealing during
		// pkill combat while being stunned, which is stupid.
		if(player->flagIsSet(P_STUNNED)) {
			player->print("You are stunned! How can you do that?\n");
			return(0);
		}

		if(player->isBlind()) {
			player->printColor("^CHow can you steal when you are blind?\n");
			return(0);
		}

		// Thief cannot steal from players if they are fighting a non-pet monster
		if(player->inCombat()) {
			player->print("You're fighting! You have no time for that now!\n");
			return(0);
		}

		if(player->checkHeavyRestrict("steal"))
			return(0);
	}

	if(cmnd->num < 2) {
		player->print("Steal what?\n");
		return(0);
	}

	if(cmnd->num < 3) {
		player->print("Steal what from whom?\n");
		return(0);
	}



	if(!player->isCt()) {

		i = LT(player, LT_STEAL);
		if(t < i) {
			player->pleaseWait(i-t);
			return(0);
		}

		// Steal must wait for both the spell and attack timers
		//********************************************************
		i = LT(player, LT_SPELL);
		if(t < i) {
			player->pleaseWait(i-t);
			return(0);
		}

		if(!player->checkAttackTimer())
			return(0);
		//********************************************************

		// Lower dex means bad bonus, means more waiting between steals.
		player->lasttime[LT_STEAL].ltime = t;
		player->lasttime[LT_STEAL].interval = 9 - bonus((int)player->dexterity.getCur());

		player->updateAttackTimer(true, 10);
	}



	target = room->findCreature(player, cmnd, 2);

	if(!target || target == player) {
		player->print("That creature is not here.\n");
		return(0);
	}

	mTarget = target->getAsMonster();
	pTarget = target->getAsPlayer();

	if(pTarget) {
		// Cannot steal from those they are dueling with.
		if(induel(player, pTarget)) {
			player->print("You cannot steal from people you are dueling.\n");
			return(0);
		}

		// Staff cannot be stolen from by players. Ever.
		if(pTarget->isStaff() && !player->isDm()) {
			player->print("Stealing from an immortal is not a good thing for your health.\n");
			pTarget->print("%s tried to steal stuff from you.\n", player->name);
			return(0);
		}

		// Impossible to steal from an incorpreal mist.
		if(	pTarget->isEffected("mist") &&
			!player->checkStaff("You cannot steal from a misted vampire.\n") )
			return(0);

		// Stealing out of bags will be re-written later.
		/*
			if((player->isCt() || player->getClass() == THIEF) && cmnd->num > 3) {
				return(steal_bag, cmnd);
			} */

	} else {

		// Cannot steal from a mob while fighting it.
		if(mTarget->isEnemy(player)) {
			player->print("Not while %s's attacking you.\n", mTarget->heShe());
			return(0);
		}

	}

	if(!player->canAttack(target, true))
		return(0);

	// Check for stealing gold
	if(cmnd->str[1][0] == '$') {
		// Stealing gold makes steal timer set to 30 seconds.
		player->lasttime[LT_STEAL].interval = 30L;
		player->clearFlag(P_NO_PKILL);
		steal_gold(player, target);
		return(0);
	}

	object = target->findObject(player, cmnd, 1);
	if(!object) {
		player->print("%s doesn't have that.\n", target->upHeShe());
		return(0);
	}

	// Check if object stolen would be too heavy.
	if(player->getWeight() + object->getActualWeight() > player->maxWeight()) {
		player->print("You wouldn't be able to carry that.\n");
		return(0);
	}

	// Check if object stolen would be too bulky.
	if(player->tooBulky(object->getActualBulk())) {
		player->print("That would be too bulky.\n");
		return(0);
	}

	if(Unique::is(object) && !Unique::canGet(player, object, true)) {
		if(Unique::isUnique(object))
			player->print("You currently cannot steal that limited object.\n");
		else
			player->print("You currently cannot steal that item as it contains a limited object you cannot currently have.\n");
		return(0);
	}

	chance = get_steal_chance(player, target, object);

	// Offensive action by player clears their pkill protection flag,
	// but only if stealing from another player.
	if(!mTarget)
		player->clearFlag(P_NO_PKILL);

	roll = mrand(1,100);
	player->statistics.attemptSteal();

	// A roll of 100 always fails
	if(!cantSteal && ((roll <= chance && roll < 100) || player->isCt()) ) {
		player->print("You succeeded.\n");
		player->checkImprove("steal", true);
		player->statistics.steal();

		// Checks for quest, unique, nosteal, and no-drop items in bags.
		// Removes them from the bag and into target's inventory.
		object->popBag(target);

		log_immort(false, player, "%s stole %s from %s.\n",
			player->name, object->name, target->name);

		logn("log.steal", "%s(L%d) stole %s from %s(L%d) in room %s.\n",
		     player->name, player->getLevel(), object->name, target->name, target->getLevel(),
		     room->fullName().c_str());

		// Other people in the room will possibly notice what's going on.
		for(Player* bystander : room->players ) {

			// We only want to test bystanders.
			if(bystander == player || bystander == target)
				continue;

			// If player is in combat, they're too busy to notice.
			if(bystander->inCombat())
				continue;

			// Blind players will not notice.
			if(bystander->isEffected("blindness"))
				continue;

			// Staff who are DM-Invis and steal will not be noticed, other
			// than by those on the staff higher than they are.
			if(player->flagIsSet(P_DM_INVIS) && (bystander->getClass() < player->getClass()))
				continue;

			// Thieves cannot mist, but putting this here for completeness.
			if(player->isEffected("mist") && !bystander->isEffected("true-sight"))
				continue;

			// If thief is invisible, only those with d-i will possibly see.
			if(player->isInvisible() && !bystander->isEffected("detect-invisible"))
				continue;



			roll = mrand(1,100);
			// adjust roll to reflect chance for new observer.
			//***************************************************
			roll += crtAwareness(target)*2;
			roll -= crtAwareness(bystander)*2;
			roll = MAX(1,roll);
			//***************************************************

			caught = 100 - chance;
			caught = MAX(1,MIN(50,caught));
			if(roll <= caught || isCt(bystander)) {
				// If roll is less than 10% of chance, bystander will see
				// what was trying to be stolen.
				if(roll <= chance/10 || isCt(bystander))
					bystander->printColor("%M tried to steal %1P from %N.\n", player, object, target);
				else
					bystander->print("%M tried to steal something from %N.\n", player, target);
			}
		}

		// Assisting monsters will now all watch one another's backs against stealing.
		// If thief steals from one, others in room might notice and attack.
		if(mTarget && !player->flagIsSet(P_DM_INVIS)) {
		    for(Monster* assist : room->monsters) {

				// Victim doesn't assist itself
				if(!assist || assist == mTarget)
					continue;

				// If mob is blind it won't assist.
				if(assist->isEffected("blindness"))
					continue;

				// If assisting mob has an enemy and that enemy is NOT the
				// thief, they will not be watching because they are distracted.
				if(assist->hasEnemy() && !assist->getAsMonster()->isEnemy(player))
					continue;

				// If assisting monster can't see the thief,
				// then it will not catch the thief stealing.
				if(!assist->canSee(player))
					continue;

				// If monster doesn't assist the victim, it doesn't care.
				if(	!mTarget->willAssist(assist) &&
					!(mTarget->flagIsSet(M_WILL_BE_ASSISTED) && assist->flagIsSet(M_WILL_ASSIST))
				)
					continue;

				roll = mrand(1,100);
				// adjust roll to reflect chance for new observer.
				//**************************************************
				roll += crtAwareness(target)*2;
				roll -= crtAwareness(assist)*2;
				roll = MAX(1,roll);
				//***************************************************
				caught = 100 - chance;
				caught = MAX(1,MIN(50,caught));
				if(roll > caught && !player->isCt()) {
					player->printColor("^r%M notices you in the act and attacks!\n", assist);
					broadcast(player->getSock(), room, "^r%M catches %N stealing and attacks!", assist, player);
					assist->getAsMonster()->addEnemy(player);
				}
			}
		}

		if(target->isMonster() && object->flagIsSet(O_JUST_LOADED)) {
			// We're stealing from a monster, and the object was just loaded:
			// ie it hasn't been stolen, given, or taken then we can get some
			// experience for stealing this item
			long expGain = target->getExperience() / 10;
			object->setDroppedBy(target, "Theft");
			expGain = MAX(expGain, 1);
			if(!player->halftolevel()) {
				player->printColor("You %s ^Y%d^x experience for the theft of %P.\n", gConfig->isAprilFools() ? "lose" : "gain", expGain, object);
				player->addExperience(expGain);
			}
		}

		// BUGFIX:  Now stealing gold items (ie: pot of gold) add to your coins instead of your inventory
		// Moved here to avoid referecing object after calling doGetObject
		// Correctly handle quest items, objects and such
		// NOTE: Object MAY be undefined after this function, don't reference it any longer
		target->delObj(object);
		Limited::transferOwner(target->getAsPlayer(), player, object);
		doGetObject(object, player, false);

	} else {

		player->print("You failed.\n");
		player->checkImprove("steal", false);
		player->smashInvis();

		broadcast(player->getSock(), target->getSock(), room, "%M tried to steal from %N.", player, target);

		if(!mTarget) {
			if(!player->isEffected("blindness"))
				target->printColor("%M tried to steal %1P from you.\n", player, object);
			else
				target->print("Someone tried to steal something from you.\n");
		} else {
			player->print("You are attacked!\n");
			broadcast(player->getSock(), room, "%M attacks %N!", mTarget, player);
			mTarget->addEnemy(player);
		}
	}

	return(0);
}

