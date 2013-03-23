/*
 * combat.cpp
 *   Routines to handle monster combat
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
#include "effects.h"
#include "specials.h"
#include "unique.h"

//*********************************************************************
//						doLagProtect
//*********************************************************************
// comes in as a creature, our job to turn into a player

int Creature::doLagProtect() {
	Player	*pThis = getAsPlayer();

	if(!pThis)
		return(0);

	if(pThis->flagIsSet(P_LAG_PROTECTION_SET) && pThis->flagIsSet(P_LAG_PROTECTION_ACTIVE))
		if(pThis->lagProtection())
			return(1);

	return(0);
}


//*********************************************************************
//						updateCombat
//*********************************************************************

int Monster::updateCombat() {
	BaseRoom* room=0;
	Creature* target=0;
	Player*	pTarget=0;
	Monster* mTarget=0;
	char	atk[30];
	int		n=1, rtn=0, yellchance=0, num=0, breathe=0;
	int		x=0;
	bool	monstervmonster=false, casted=false;
	bool	resistPet=false, immunePet=false, vulnPet=false;
	bool	willCast=false, antiMagic=false, isCharmed=false;

	ASSERTLOG(this);
	room = getRoomParent();

	strcpy(atk, "");

	target = getTarget();

	if(!target) return(0);

	// is this a player?
	pTarget = target->getAsPlayer();
	mTarget = target->getAsMonster();

	// If we're fighting a pet, see if we'll ignore the pet and attack a player instead
	if(target->isPet() && target->getMaster()) {
		if(target->inSameRoom(target->getMaster())) {
			if(mrand(1,100) < 20 || target->intelligence.getCur() >= 150 || target->flagIsSet(M_KILL_MASTER_NOT_PET)) {
				target = target->getMaster();
				addEnemy(target);
			}
		}
	}

	// No fighting in a safe room!
	if(	pTarget &&
		isPet() &&
		!getMaster()->isCt() &&
		getRoomParent()->isPkSafe()
	)
		return(0);

	// Stop fighting if it's a no exp loss monster
	if(	pTarget &&
		flagIsSet(M_NO_EXP_LOSS) &&
		!isPet() &&
		target->inCombat(this)
	)
		return(0);

	monstervmonster = (!pTarget && !target->isPet() && isMonster() && !isPet());

	room->wake("Loud noises disturb your sleep.", true);


	if(target == this)
		return(0);
	if(target->hasCharm(getName()) && flagIsSet(M_CHARMED))
		return(0);


	target->checkTarget(this);

	NUMHITS++;
	n = 20;
	if(flagIsSet(M_CAST_PRECENT))
		n = cast;

	// Make it so more intelligent monsters won't sit and cast spells all day
	// on a person who is more than 50% r-m!
	int resistAmount = 100 - doResistMagic(100);
	if(resistAmount < 100) {
		if(intelligence.getCur() > 250)  {
			if(resistAmount >= 90)		n /= 5;
			else if(resistAmount >= 75)	n /= 3;
			else						n = n * 2 / 3;
		} else if(intelligence.getCur() > 210) {
			if(resistAmount >= 90)		n /= 3;
			else if(resistAmount >= 75)	n /= 2;
			else 						n = n * 5 / 6;
		} else if(intelligence.getCur() > 170) {
			if(resistAmount >= 80)		n /= 2;
			else						n = n * 5 / 6;
		}
	}
	// Also if the target is reflecting magic
	if(target->isEffected("reflect-magic")) {
		if(intelligence.getCur() >= 280)
			n /= 5;
		else if(intelligence.getCur() >= 250)
			n /= 3;
		else if(intelligence.getCur() >= 210)
			n /= 2;
		else if(intelligence.getCur() >= 170)
			n = n * 5 / 6;
	}


	if(mrand(1,100) < n) {
		willCast = true;
		if(room->checkAntiMagic(this))
		    antiMagic = true;
	}
	if(isUndead() && target->isEffected("undead-ward"))
		willCast = false;

	// handle spell-invisible monsters
	EffectInfo* effect = getEffect("invisibility");
	if(effect && effect->getDuration() != -1) {
		broadcast(getSock(), room, "%M fades into view.", this);
		removeEffect("invisibility");
	}

	lasttime[LT_AGGRO_ACTION].ltime = time(0);

	if(	willCast &&
		flagIsSet(M_CAN_CAST) &&
		canSpeak() &&
		!getRoomParent()->flagIsSet(R_NO_MAGIC) &&
		!antiMagic &&
		!isCharmed
	) {


		if(target->doLagProtect())
			return(1);

		rtn = castSpell( target );

		if(rtn == 2)
			return(1);
		else if(rtn == 1) {
			casted = true;
			if(pTarget && pTarget->flagIsSet(P_WIMPY) && !pTarget->flagIsSet(P_LAG_PROTECTION_OPERATING)) {
				if(pTarget->hp.getCur() <= pTarget->getWimpy()) {
					pTarget->flee(false, true);
					return(1);
				}
			}
		}
	}


// Don't jump to your aid if it is itself...lol
	if(pTarget) {
	    for(Monster* pet : pTarget->pets) {
			if(	pet->isPet() && getMaster() != pTarget && pet->getMaster() == pTarget &&
				!pet->isEnemy(this) && pet != this )
			{
				pet->addEnemy(findFirstEnemyCrt(pet));
				pTarget->print("%M jumps to your aid!!\n", pet);
				break;
			}
		}
	}

	if(hp.getCur() <= hp.getMax()/3 && flagIsSet(M_WILL_BERSERK))
		berserk();


	if(casted)
		return(1);

	// Try running special attacks if we have any
	if(!specials.empty() && runSpecialAttacks(target))
		return(1);



	if(!isCharmed) {
		if(canSpeak()) {

			if(flagIsSet(M_YELLED_FOR_HELP))
				yellchance = 15+(2*bonus((int) intelligence.getCur()));
			else
				yellchance = 25+(2*bonus((int) intelligence.getCur()));

			if(flagIsSet(M_WILL_YELL_FOR_HELP) && ((mrand(1,100) < yellchance) || (hp.getCur() <= hp.getCur()/5))) {
				if(!flagIsSet(M_WILL_BE_HELPED))
					broadcast(getSock(), room, "%M yells for help!", this);
				setFlag(M_YELLED_FOR_HELP);
				setFlag(M_WILL_BE_ASSISTED);
				clearFlag(M_WILL_YELL_FOR_HELP);
				return(0);
			}
		}
	}

	if(	flagIsSet(M_YELLED_FOR_HELP) &&
		(mrand(1,100) < (MAX(15, inUniqueRoom() ? getUniqueRoomParent()->wander.getTraffic() : 15))) &&
		!flagIsSet(M_WILL_YELL_FOR_HELP)
	) {
		setFlag(M_WILL_YELL_FOR_HELP);
		if(summonMobs(target))
			return(0);
	}

//	if(pTarget && isPet() && target->inCombat())
//		getMonster()->delEnmCrt(target->name);

	// Check resisting of elemental pets
	if(isPet()) {
		target->checkResistPet(this, resistPet, immunePet, vulnPet);
		if( !pTarget &&
			!casted &&
			(target->flagIsSet(M_ONLY_HARMED_BY_MAGIC) || immunePet))
		{
			getMaster()->print("%M's attack has no effect on %N.\n", this, target);
			return(0);
		}
	}
	// We'll be nice for now...no critical hits from mobs :)
	int resultFlags = NO_CRITICAL | NO_FUMBLE;
	AttackResult result = getAttackResult(target, NULL, resultFlags);

	if(isPet()) {
		// We are a pet, and we're attacking.  Smash invis of our owner.
		getMaster()->smashInvis();
	}


	if(result == ATTACK_HIT || result == ATTACK_CRITICAL || result == ATTACK_BLOCK) {
		Damage attackDamage;
		int drain = 0;
		bool wasKilled = false, freeTarget = false, meKilled = false;

		computeDamage(target, ready[WIELD - 1], ATTACK_NORMAL, result, attackDamage, true, drain);
		attackDamage.includeBonus();

		target->printColor("^r");

		if(!breathe) {
			if(attack[0][0] || attack[1][0] || attack[2][0]) {
				do {
					num = mrand(1,3);
					if(attack[num-1][0])
						strcpy(atk, attack[num-1]);
				} while(!attack[num-1][0]);
			} else {
				strcpy(atk, "hit");
			}
		}

		if(!atk[0])	// just in case
			strcpy(atk, "hit");

		if(result == ATTACK_BLOCK) {
			printColor("^C%M partially blocked your attack!\n", target);
			target->printColor("^CYou manage to partially block %N's attack!\n", this);
		}

		target->printColor("^r%M %s you%s for ^R%d^r damage.\n", this, atk,
			target->isBrittle() ? "r brittle body" : "", MAX(1, attackDamage.get()));

		if(!isPet())
			target->checkImprove("defense", false);

		if(pTarget) {
			pTarget->damageArmor(attackDamage.get());
			pTarget->statistics.wasHit();
		}

		if(ready[WIELD - 1]) {
			if(ready[WIELD - 1]->getMagicpower() && ready[WIELD - 1]->flagIsSet(O_WEAPON_CASTS) && ready[WIELD - 1]->getChargesCur() > 0)
				attackDamage.add(castWeapon(target, ready[WIELD - 1], wasKilled));
		}

		broadcastGroup(false, target, "^M%M^x %s ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n", this, atk,
			target, attackDamage.get(), target->heShe(), target->getStatusStr(attackDamage.get()));

		if(target->pFlagIsSet(P_LAG_PROTECTION_SET))
			target->pSetFlag(P_LAG_PROTECTION_ACTIVE); // lagprotect auto-activated on being hit.

		if(target->isPet())
			target->getMaster()->printColor("%M hit ^M%N^x for %s%d^x damage.\n", this, target, target->getMaster()->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());

		if(isPet()) {
			getMaster()->printColor("%M hit %N for %s%d^x damage.\n", this, target, getMaster()->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());

			castDelay(PET_CAST_DELAY);

			if(mTarget) {
				mTarget->addEnemy(this);
			}

		} else if(monstervmonster) {
			// Output only when monster v. monster
			broadcast(getSock(), target->getSock(), room, "%M hits %N.", this, target);
			mTarget->addEnemy(this);
		}



		if(flagIsSet(M_WILL_POISON))
			tryToPoison(target);
		if(flagIsSet(M_CAN_STONE))
			tryToStone(target);
		if(flagIsSet(M_DISEASES))
			tryToDisease(target);

		if(	(flagIsSet(M_WOUNDING) && x <= 15) &&
			(target->getClass() != LICH) &&
			!target->isEffected("stoneskin")
		) {
			if(!target->chkSave(DEA,this,0)) {
				target->printColor("^RThe wound is festering and unclean.\n");
				broadcastGroup(false, target, "%M wounds are festering and unclean.\n", target);
				broadcast(getSock(), target->getSock(), room, "%M's wounds are festering and unclean.\n", target);

				target->addEffect("wounded", -1, 1, this, false, target);
			}
		}

		meKilled = doReflectionDamage(attackDamage, target);

		if(	pTarget && (
				flagIsSet(M_STEAL_WHEN_ATTACKING) ||
				flagIsSet(M_STEAL_ALWAYS)
			) &&
			mrand(1,100) <= 10 && (
				!pTarget->hasCharm(getName()) &&
				flagIsSet(M_CHARMED)
			)
		) {
			if(steal(pTarget)) {
				if(wasKilled)
					pTarget->checkDie(this);
				if(meKilled)
					checkDie(pTarget);
				return(0);
			}
		}

		if(flagIsSet(M_WILL_BLIND))
			tryToBlind(target);

		if(pTarget && flagIsSet(M_DISOLVES_ITEMS) && mrand(1,100) <= 15)
			pTarget->dissolveItem(this);

		if(	doDamage(target, attackDamage.get(), CHECK_DIE, PHYSICAL_DMG, freeTarget) ||
			wasKilled ||
			meKilled
		) {
			Creature::simultaneousDeath(this, target, false, freeTarget);
			return(1);
		}

		if(	pTarget &&
			pTarget->flagIsSet(P_WIMPY) &&
			!pTarget->flagIsSet(P_LAG_PROTECTION_OPERATING) &&
			!pTarget->flagIsSet(P_LAG_PROTECTION_ACTIVE))
		{
			if(pTarget->hp.getCur() <= pTarget->getWimpy()) {
				pTarget->flee(false, true);
				return(1);
			}
		} else if(pTarget && pTarget->isEffected("fear")) {
			int ff;
			ff = 40 + (1- (pTarget->hp.getCur()/pTarget->hp.getMax()))*40 +
				::bonus((int)pTarget->constitution.getCur())*3 +
				((pTarget->getClass() == PALADIN ||
						 pTarget->getClass() == DEATHKNIGHT) ? -10 : 0);
			if(ff < mrand(1,100)) {
				pTarget->flee(true);
				return(1);
			}
		}

		if(target->doLagProtect())
			return(1);

	} else if(result == ATTACK_MISS) {
		target->printColor("^c%M missed you.\n", this);
		if(!isPet())
			target->checkImprove("defense", true);

		if(target->doLagProtect())
			return(1);

		// Output only when monster v. monster
		if(monstervmonster) {
			broadcast(target->getSock(), target->getSock(), room, "%M misses %N.", this, target);
		} else if(isPet()) {
			getMaster()->printColor("^c%M misses %N.\n", this, target);
		}

		if(mTarget)
            mTarget->addEnemy(this);

		if(pTarget) {
			pTarget->statistics.wasMissed();
			if(	pTarget->flagIsSet(P_WIMPY) &&
				!pTarget->flagIsSet(P_LAG_PROTECTION_OPERATING)
			) {
				if(pTarget->hp.getCur() <= pTarget->getWimpy()) {
					pTarget->flee(false, true);
					return(1);
				}
			}
		}
	} else if(result == ATTACK_DODGE) {
		if(!isPet())
			target->checkImprove("defense", true);
		target->dodge(this);
		return(1);
	}
	else if(result == ATTACK_PARRY) {
		if(!isPet())
			target->checkImprove("defense", true);
		target->parry(this);
		return(1);
	}

	return(0);
}

//*********************************************************************
//						zapMp
//*********************************************************************

int Monster::zapMp(Creature *victim, SpecialAttack* attack) {
	//Player	*pVictim = victim->getPlayer();
	int		n=0;

	if(victim->mp.getCur() <= 0)
		return(0);

	n = MIN(victim->mp.getCur(), (mrand(1+level/2, level) + bonus((int)intelligence.getCur())));

	victim->printColor("^M%M zaps your magical talents!\n", this);
	victim->printColor("%M stole %d magic points!\n", this, n);
	broadcast(victim->getSock(), victim->getSock(), victim->getRoomParent(), "^M%M zapped %N!", this, victim);

	if(victim->chkSave(MEN, this, 0))
		n /= 2;

	victim->mp.decrease(n);

	return(1);
}

//*********************************************************************
//						regenerate
//*********************************************************************

void Monster::regenerate() {
	hp.increase(mrand(4, 15) + level * 7/2);
}

//*********************************************************************
//						steal
//*********************************************************************
// Returns 0 if unable to steal for some reason but returns 1 if
// succeeded or steal failed

int Monster::steal(Player *victim) {
	int chance=0, inventory=0, i=0;
	Object* object=0;

	ASSERTLOG( victim );

	if(!victim || !canSee(victim) || victim->isStaff())
		return(0);
	if(victim->isEffected("petrification") || victim->isEffected("mist"))
		return(0);
	if(victim->objects.empty())
		return(0);

	lasttime[LT_STEAL].ltime = time(0);

	i = victim->countInv();
	if(i < 1)
		return(0);

	if(i == 1)
		inventory = 1;
	else
		inventory = mrand(1, i - 1);

	i = 1;
	for(Object *obj : victim->objects) {
		if(i++ == inventory) {
			object = obj;
			break;
		}
	}

	chance = 4 * level + bonus((int) dexterity.getCur()) * 3;
	if(victim->getLevel() > level)
		chance -= 15 * (victim->getLevel() - level);
	chance = MIN(chance, 65);

	if(	object->getQuestnum() ||
		(flagIsSet(M_CANT_BE_STOLEN_FROM))
	)
		chance = 0;

	if(mrand(1, 100) <= chance) {
		victim->delObj(object, false, true);
		addObj(object);

		logn("log.msteal", "%s(L%d) stole %s from %s(L%d) in room %s.\n",
			getCName(), level, object->getCName(), victim->getCName(), victim->getLevel(), getRoomParent()->fullName().c_str());
	} else {
		broadcast(victim->getSock(), getRoomParent(), "%M tried to steal from %N.", this, victim);
		victim->printColor("^Y%M tried to steal %P^Y from you.\n", this, object);
	}
	return(1);
}

//*********************************************************************
//						berserk
//*********************************************************************

void Monster::berserk() {

	// TODO: Change Berserk into an effect
	if(isEffected("berserk"))
		return;
	addEffect("berserk", 60, 50);
	clearFlag(M_WILL_BERSERK);
	strength.addModifier("Berserk", 50, MOD_CUR_MAX);

	broadcast(NULL, getRoomParent(), "^R%M goes berserk!", this);
	return;
}

//*********************************************************************
//						summonMobs
//*********************************************************************

int Monster::summonMobs(Creature *victim) {
	int		mob=0, arrive=0, i=0, found=0;
	Monster	*monster=0;

	// Calls for help will be suspended if max mob allowance
	// in room is already reached.

	for(i=0; i<NUM_RESCUE; i++) {
		if(validMobId(rescue[i]))
			found++;
	}

	if(!found)
		return(0);

	if(getRoomParent()->countCrt() >= getRoomParent()->getMaxMobs())
		return(0);

	found = mrand(1, found)-1;

	for(mob=0; found; mob++) {
		if(validMobId(rescue[mob]))
			found--;
	}


	arrive = mrand(1,2);

	if(victim->pFlagIsSet(P_LAG_PROTECTION_SET))
		victim->pSetFlag(P_LAG_PROTECTION_ACTIVE);


	for(i=0; i < arrive; i++) {
		monster = 0;
		if(!loadMonster(rescue[mob], &monster))
			return(0);

		gServer->addActive(monster);
		monster->addToRoom(victim->getRoomParent());
		monster->addEnemy(victim);

		if(victim->getMaster() && victim->isMonster())
			monster->addEnemy(victim->getMaster());

		broadcast(getSock(), victim->getRoomParent(), "%M runs to the aid of %N!", monster, this);

		monster->setFlag(M_WILL_ASSIST);
		monster->setFlag(M_WILL_BE_ASSISTED);
		monster->clearFlag(M_PERMENANT_MONSTER);
	}

	//clearFlag(M_YELLED_FOR_HELP);
	return(1);
}

//*********************************************************************
//						check_for_yell
//*********************************************************************

int Monster::checkForYell(Creature* target) {
	int	yellchance=0;

	if(!(flagIsSet(M_WILL_YELL_FOR_HELP) || flagIsSet(M_YELLED_FOR_HELP)))
		return(0);

	if(!canSpeak())
		return(0);


	if(flagIsSet(M_YELLED_FOR_HELP))
		yellchance = 35+(2*bonus((int) intelligence.getCur()));
	else
		yellchance = 50+(2*bonus((int) intelligence.getCur()));


	if(flagIsSet(M_WILL_YELL_FOR_HELP) && ((mrand(1,100) < yellchance) || (hp.getCur() <= hp.getCur()/5))) {
		if(!flagIsSet(M_WILL_BE_HELPED))
			broadcast(getSock(), target->getRoomParent(), "%M yells for help!", this);
		setFlag(M_YELLED_FOR_HELP);
		setFlag(M_WILL_BE_ASSISTED);
		clearFlag(M_WILL_YELL_FOR_HELP);
		return(1);  // Monster yells
	}


	return(0); // Monster does not yell
}

//*********************************************************************
//						lagProtection
//*********************************************************************

int Player::lagProtection() {
	BaseRoom* room = getRoomParent();
	int		t=0, idle=0;

	// Can't do anything while unconsious!
	if(flagIsSet(P_UNCONSCIOUS))
		return(0);

	t = time(0);
	idle = t - getSock()->ltime;


	// This number can be changed.
	if(idle < 10L) {
		//print("testing for idle..\n");
		return(0);
	}

	// Only lag protect below wimpy if auto attack is on
	if(autoAttackEnabled() && hp.getCur() > getWimpy())
		return(0);


	printColor("^cPossible lagout detected.\n");
	statistics.lagout();
	if(!flagIsSet(P_NO_LAG_HAZY) && hp.getCur() < hp.getMax()/2) {

		setFlag(P_LAG_PROTECTION_OPERATING);

		if(useRecallPotion(1, 1))
			return(1);
	}

	broadcast(getSock(), room, "%M enters autoflee lag protection routine.", this);

	print("Auto flee routine initiated.\n");
	if(flagIsSet(P_SITTING))
		stand();
	printColor("Searching for suitable exit........\n");


	if(ready[WIELD-1]) {
		if(!ready[WIELD-1]->flagIsSet(O_CURSED)) {
			ready[WIELD-1]->clearFlag(O_WORN);
			printColor("Removing %s.\n", ready[WIELD-1]->getCName());
			unequip(WIELD);
		}
	}

	if(ready[HELD-1]) {
		if(!ready[HELD-1]->flagIsSet(O_CURSED)) {
			ready[HELD-1]->clearFlag(O_WORN);
			printColor("Removing %s.\n", ready[HELD-1]->getCName());
			unequip(HELD);
		}
	}

	for(Exit* ext : room->exits) {
		// Opens all unlocked exits.
		if(!ext->flagIsSet(X_LOCKED))
			ext->clearFlag(X_CLOSED);
	}

	setFlag(P_LAG_PROTECTION_OPERATING);
	if(flee(false, true)) {
		if(isStaff()) {
			broadcast(::isStaff, "^C### %s(L%d) fled due to lag protection. HP: %d/%d. Room: %s.",
			getCName(), level, hp.getCur(), hp.getMax(), getRoomParent()->fullName().c_str());
		} else {
			broadcast(::isWatcher, "^C### %s(L%d) fled due to lag protection. HP: %d/%d. Room: %s.",
			getCName(), level, hp.getCur(), hp.getMax(), getRoomParent()->fullName().c_str());
		}
		logn("log.lprotect","### %s(L%d) fled due to lag protection. HP: %d/%d. Room: %s.\n",
			getCName(), level, hp.getCur(), hp.getMax(), getRoomParent()->fullName().c_str());

		return(1);
	}
	return(0);
}


//*********************************************************************
//						chkSave
//*********************************************************************
// This function is used for saving throws. It is sent savetype, which
// is the save category, creature, which is the creature saving,
// and if an this is involved, target is sent. If there is
// no this, then target will be 0. The bonus variable is
// a special bonus which can be sent, based on conditions set in
// the calling function. the chksave function returns 0
// if the creature does not save, and 1 if it does. How the save
// affects creature is determined in the code of the calling function. -- TC

int Creature::chkSave(short savetype, Creature* target, short bns) {
	Player	*pCreature=0;
	int		chance, gain, i, ringbonus=0, opposing=0, upchance = 0, natural=1;
	int		roll=0, nogain=0;
	long	j=0, t=0;


	pCreature = getAsPlayer();

	if(target && target != this)
		opposing = 1;


	chance = saves[savetype].chance;
	gain = saves[savetype].gained;


	for(i=9; i < 17; i++) {	// Determine save bonus from any magical rings.
		if(!ready[i]) // Only the highest + ring is used.
			continue;

		if(ready[i]->getAdjustment() > 0 && ready[i]->getAdjustment() > ringbonus) {
			ringbonus = ready[i]->getAdjustment();
			continue;
		}
	}

	chance += ringbonus*5;

	if(	ready[HELD-1] && ready[HELD-1]->flagIsSet(O_LUCKY) &&
		ready[HELD-1]->getType() > 4 && ready[HELD-1]->getAdjustment() > 0
	)
		chance += ready[HELD-1]->getAdjustment()*5;

	switch(savetype) {
	case POI:
		chance += bonus((int) constitution.getCur());
		if(hp.getCur() <= hp.getMax()/3)
			chance /= 2;	// More vulnerable to poison if weakened severely.
		if(pCreature && pCreature->isEffected("berserk"))
			chance += 15;	// Poison doesn't harm berserk players as much.

		break;
	case DEA: // bonuses set for death traps and death saves are sent with the
		// bonus variable from the calling function.

		break;
	case BRE:
		chance += bonus((int) dexterity.getCur())*2;
		if(ready[BODY-1])
			chance += 5*ready[BODY-1]->getAdjustment();
		break;
	case MEN:
		if(opposing)
			chance -= 5*(crtWisdom(target) - crtWisdom(this));
		else
			chance += crtWisdom(this);

		break;
	case SPL:
		if(opposing)
			chance -= 2*(bonus((int) target->intelligence.getCur())-bonus((int)intelligence.getCur()));
		else
			chance -= 2*bonus((int) target->intelligence.getCur());

		if(pCreature && pCreature->isEffected("resist-magic")) {
			natural = 0;
			chance += 25;
		}

		break;
	case LCK:
		chance += ((saves[POI].chance + saves[DEA].chance +
				saves[BRE].chance + saves[MEN].chance +
				saves[SPL].chance)/5);
		break;
	default:
		break;
	}

	chance += bns;

	if(pCreature && pCreature->getClass() == CLERIC && pCreature->getDeity() == KAMIRA)
		chance += 5;

	chance = MAX(1,MIN(95, chance));

	roll = mrand(1,100);
	if(isCt())
		print("Roll: %d.\n", roll);



	if((roll >= 95 || roll <= 5) && gain < 5) {
		upchance = 99 - saves[savetype].chance;
		if(bns == -1 || savetype == LCK || level < 7 || (target->isPlayer() && savetype == SPL))
			upchance = 0;


		if(mrand(1,100) <= upchance) {
			t = time(0);
			j = LT(this, LT_SAVES);
			if(t < j) {
				nogain = 1;
			}
			else
			{
				if(pCreature) {
					lasttime[LT_SAVES].ltime = t;
					lasttime[LT_SAVES].interval = 60L;
				}
			}

			// This keeps people from abusing traps to
			// increase their saves. -TC

			if(!nogain) {
				switch(savetype) {
				case POI:
					if(!opposing || (!(opposing && target->isPlayer()))) {
						if(!immuneToPoison() || !immuneToDisease()) {
							printColor("^BYour resistance against poison and disease has increased.\n");
							saves[POI].chance += 2;
							saves[POI].gained += 1;
						}
					}
					break;
				case DEA:
					printColor("^BYour chances of avoiding deadly traps and death magic have increased.\n");
					saves[DEA].chance += 2;
					saves[DEA].gained += 1;
					break;
				case BRE:
					printColor("^BYour ability to avoid breath weapons and explosions has increased.\n");
					saves[BRE].chance += 2;
					saves[BRE].gained += 1;
					break;
				case MEN:
					printColor("^BYou are better able to avoid mental attacks.\n");
					saves[MEN].chance += 2;
					saves[MEN].gained += 1;
					break;

				case SPL:
					if(opposing) {
						if(target->isMonster()) {
							if(natural == 1) {
								printColor("^BYour resistance against spells and magical attacks has increased.\n");
								saves[SPL].chance += 2;
								saves[SPL].gained += 1;
							}
						} else
							break;
					} else {
						if(natural == 1) {
							printColor("^BYour resistance against spells and magical attacks has increased.\n");
							saves[SPL].chance += 2;
							saves[SPL].gained += 1;
						}
					}
					break;

				default:
					break;
				}

			}//end nogain check
		}
	}

	if(isPlayer())
		getAsPlayer()->statistics.attemptSave();
	if(roll <= chance && natural == 1) {
		// save made
		if(isPlayer())
			getAsPlayer()->statistics.save();
		return(1);
	}
	return(0);
}

//*********************************************************************
//						clearAsEnemy
//*********************************************************************
// This function clears the creature sent to it from the enemy list of every
// other creature in the vicinity. -TC

void Creature::clearAsEnemy() {
	if(!getRoomParent())
		return;

	for(Monster* mons : getRoomParent()->monsters) {
	    mons->clearEnemy(this);
	}
	return;
}

//*********************************************************************
//						damageArmor
//*********************************************************************

void Player::damageArmor(int dmg) {
	Object	*armor=0;

	// Damage armor 1/10th of the time
	if(mrand(1, 10) == 1)
		return;

	int wear = chooseItem();
	if(!wear)
		return;

	armor = ready[wear-1];
	if(!armor)
		return;

	// Rings will remain undamageable
	if(armor->getWearflag() >= FINGER && armor->getWearflag() <= FINGER8)
		return;

	if(armor->getType() != ARMOR)
		return;

	bstring armorType = armor->getArmorType();
	int armorSkill = (int)getSkillGained(armorType);
	int avoidChance = armorSkill / 4;


	// Make armor skill worth something, the higher the skill, the lower the chance it'll take damage
	if(mrand(1,100) > avoidChance) {
		printColor("Your ^W%s^x just got a little more scratched.\n", armor->getCName());
		armor->decShotsCur();
		if(armorType != "shield")
			checkImprove(armorType, true);
	}

	checkArmor(wear);
}

//*********************************************************************
//						checkArmor
//*********************************************************************

void Player::checkArmor(int wear) {
	if(!ready[wear-1])
		return;

	if(ready[wear-1]->getShotsCur() < 1) {
		printColor("Your %s fell apart.\n", ready[wear-1]->getCName());
		if(ready[wear-1]->flagIsSet(O_CURSED)) {
			logn("log.curseabuse", "%s's %s fell apart. Room: %s\n",
				getCName(), ready[wear-1], getRoomParent()->fullName().c_str());
		}
		broadcast(getSock(), getRoomParent(), "%M's %s fell apart.", this, ready[wear-1]->getCName());

		Limited::remove(this, ready[wear-1]);
		unequip(wear, Limited::isLimited(ready[wear-1]) ? UNEQUIP_DELETE : UNEQUIP_ADD_TO_INVENTORY);
		computeAC();
	}
}

//*********************************************************************
//						findFirstEnemyCrt
//*********************************************************************

Creature *Creature::findFirstEnemyCrt(Creature *pet) {
	if(!pet->getMaster())
		return(this);

	for(Monster* mons : pet->getRoomParent()->monsters) {
	    if(mons == pet)
	        continue;
	    if(mons->getAsMonster()->isEnemy(pet->getMaster()) && mons->getName() == getName())
	        return(mons);

	}

	return(this);
}


//*********************************************************************
//						doDamage
//*********************************************************************
// Handles actually dealing damage to someone.  Will check if the target died
// and will increase battle focus for fighters
//	1 = died, 0 = didn't die

int Creature::doDamage(Creature* target, int dmg, DeathCheck shouldCheckDie, DamageType dmgType) {
	bool freeTarget=true;
	return(doDamage(target, dmg, shouldCheckDie, dmgType, freeTarget));
}
int Creature::doDamage(Creature* target, int dmg, DeathCheck shouldCheckDie, DamageType dmgType, bool &freeTarget) {
	ASSERTLOG( target );

	Player* pTarget = target->getAsPlayer();
	Monster* mTarget = target->getAsMonster();
	Player* pThis = getAsPlayer();
	Monster* mThis = getAsMonster();

	int m = MIN(target->hp.getCur(), dmg);

	target->hp.decrease(dmg);
	//checkTarget(target);
	if(mTarget) {
		mTarget->lasttime[LT_AGGRO_ACTION].ltime = time(0);
		mTarget->adjustThreat(this, m);
	}
	if(mThis) {
	    mThis->adjustContribution(target, dmg/2);
	}

	if(this != target) {
		// If we're a player and a fighter, give them some focus
		if(pThis && pThis->getClass() == FIGHTER && !pThis->getSecondClass()) {
			// Increase battle focus here
			pThis->increaseFocus(FOCUS_DAMAGE_OUT, dmg, target);
		}
		// If the target is a player and isn't dead give them some focus
		if(pTarget && pTarget->hp.getCur() > 0 && pTarget->getClass() == FIGHTER && !pTarget->getSecondClass()) {
			// Increase battle focus here
			pTarget->increaseFocus(FOCUS_DAMAGE_IN, dmg);
		}
	}

	if(shouldCheckDie == CHECK_DIE)
		return(target->checkDie(this, freeTarget));
	else if(shouldCheckDie == CHECK_DIE_ROB)
		return(target->checkDieRobJail(mThis, freeTarget));

	freeTarget = false;
	return(0);
}

//*********************************************************************
//						simultaneousDeath
//*********************************************************************
// Due to spells like reflect-magic and fire-shield, it is possible for, during
// an attack, both attacker and defender to be killed. This function handles
// properly freeing the combatants in those scenarios.

void Creature::simultaneousDeath(Creature* attacker, Creature* target, bool freeAttacker, bool freeTarget) {
	// be very careful: we have to free the targets in the right order

	// !freeTarget means 1) we haven't check if they've died or 2) they didnt die. Either way, check again.
	if(!freeTarget) {
		target->checkDie(attacker, freeTarget);
	}
	// !freeAttacker means 1) we haven't check if they've died or 2) they didnt die. Either way, check again.
	if(!freeAttacker) {
		attacker->checkDie(target, freeAttacker);
	}

	// clean up anything that might still need cleaning
	if(freeTarget && target->isMonster()) {
		target->getAsMonster()->finishMobDeath(attacker);
		target = null;
	}
	if(freeAttacker && attacker->isMonster()) {
		attacker->getAsMonster()->finishMobDeath(target);
		attacker = null;
	}
}

//*********************************************************************
//						tryToPoison
//*********************************************************************
// noRandomChance = always can poison if they don't save (for breath attacks)
// Return: true if they are poisoned, false if they aren't

bool Monster::tryToPoison(Creature* target, SpecialAttack* attack) {
	Player* pTarget = target->getAsPlayer();
	BaseRoom* room=getRoomParent();
	int saveBonus = 0;
	if(attack)
		saveBonus = attack->saveBonus;

	// If this is from a special attack, we don't need the WILL_POISON flag
	if(!flagIsSet(M_WILL_POISON) && !attack)
		return(false);
	if(pTarget && target->isEffected("stoneskin"))
		return(false);
	if(target->isPoisoned())
		return(false);

	if(!attack && mrand(1,100) > 15)
		return(false);

	if(target->immuneToPoison()) {
		target->printColor("^G%M tried to poison you!\n", this);
		broadcastGroup(false, target, "%M tried to poison %N.\n", this, target);
		broadcast(getSock(), target->getSock(), room, "%M tried to poison %N.", this, target);
		return(false);
	}

	int duration=0;

	if(!target->chkSave(POI, this, saveBonus)) {
		target->printColor("^G%M poisons you!\n", this);
		broadcastGroup(false, target, "^G%M poisons %N.\n", this, target);
		broadcast(getSock(), target->getSock(), room, "^G%M poisons %N.", this, target);

		if(poison_dur) {
			duration = poison_dur - 12*bonus((int)target->constitution.getCur());
			duration = MAX(120, MIN(duration, 1200));
		} else {
			duration = (mrand(2,3)*60) - 12*bonus((int)target->constitution.getCur());
		}

		target->poison(isPet() ? getMaster() : this, poison_dmg ? poison_dmg : level, duration);
		return(true);
	} else if(attack) {
		target->print("You avoided being poisoned!\n");
	}

	return(false);
}

//*********************************************************************
//						tryToStone
//*********************************************************************
// Return: true if they have been petrified, false if they haven't

bool Monster::tryToStone(Creature* target, SpecialAttack* attack) {
	Player* pTarget = target->getAsPlayer();
	int		bns=0;
	bool	avoid=false;

	// If a special attack we don't need the can stone flag
	if(!flagIsSet(M_CAN_STONE) && !attack)
		return(false);

	if(!pTarget)
		return(false);
	if(pTarget->isEffected("petrification") || pTarget->isUnconscious())
		return(false);

	if(!attack && mrand(1,100) > 5)
		return(false);

	if((pTarget->isEffected("resist-earth") || pTarget->isEffected("resist-magic")) && mrand(1,100) <= 50)
		avoid = true;

	pTarget->wake("Terrible nightmares disturb your sleep!");
	bns = 10*(pTarget->getLevel()-level) + 3*bonus((int)pTarget->constitution.getCur());
	if(attack)
		bns += attack->saveBonus;

	bns = MAX(0,MIN(bns,75));

	if(pTarget->isStaff() || avoid || pTarget->chkSave(DEA, pTarget, bns)) {
		broadcast(getSock(), getRoomParent(), "%M tried to petrify %N!", this, pTarget);
		target->printColor("^c%M tried to petrify you!^x\n", this);
	} else {
		pTarget->addEffect("petrification", 0, 0, NULL, true, this);
		broadcast(target->getSock(), getRoomParent(), "%M turned %N to stone!", this, pTarget);
		target->printColor("^D%M turned you to stone!^x\n", this);
		pTarget->clearAsEnemy();
		pTarget->removeFromGroup();
		//remFromGroup(pTarget);
		return(true);
	}

	return(false);
}

//*********************************************************************
//						tryToDisease
//*********************************************************************
// Return: true if they have been diseased, false if they haven't

bool Monster::tryToDisease(Creature* target, SpecialAttack* attack) {
	int bns = 0;

	// Don't need the diseases flag if this is a special attack
	if(!flagIsSet(M_DISEASES) && !attack)
		return(false);
	if(target->isPlayer() && target->isEffected("stoneskin"))
		return(false);
	if(!attack && mrand(1,100) > 15)
		return(false);

	if(target->immuneToDisease()) {
		target->printColor("^c%M tried to infect you!\n", this);
		broadcastGroup(false, target, "%M tried to infect %N.\n", this, target, 1);
		broadcast(getSock(), target->getSock(), getRoomParent(), "%M tried to infect %N.", this, target);
		return(false);
	}

	if(attack)
		bns = attack->saveBonus;

	if(!target->chkSave(POI, this, bns)) {
		target->printColor("^D%M infects you.\n", this);
		broadcastGroup(false, target, "%M infects %N.\n", this, target, 1);
		broadcast(getSock(), target->getSock(), getRoomParent(), "%M infected %N.", this, target);
		if(isPet())
			getMaster()->printColor("^D%M infects %N.\n", this, target);
		else
			target->disease(this, target->hp.getMax()/20);
		return(true);
	} else if(attack) {
		target->print("You narrowly avoid catching a disease!\n");
	}

	return(false);
}

//*********************************************************************
//						tryToBlind
//*********************************************************************
// Return: true if they have been blinded, false if they haven't

bool Monster::tryToBlind(Creature* target, SpecialAttack* attack) {
	// Don't need the blind flag if it's a special attack
	if(!flagIsSet(M_WILL_BLIND) && !attack)
		return(false);

	if(!attack && mrand(1,100) > 15)
		return(false);
	int bns = 0;
	if(attack)
		bns = attack->saveBonus;
	if(!target->chkSave(LCK, this, bns)) {
		target->printColor("^y%M blinds your eyes.\n",this);
		broadcastGroup(false, target, "%M blinds %N.\n", this, target);
		if(isPet()) {
			getMaster()->printColor("^y%M blinds %N.\n", this, target);
		}
		target->addEffect("blindness", 180 - (target->constitution.getCur()/10), 1, this, true, this);
		return(true);
	} else if(attack) {
		target->print("You narrowly avoided going blind!\n");
	}
	return(false);
}

