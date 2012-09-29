/*
 * players.cpp
 *	 Player routines.
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
#include "effects.h"
#include "calendar.h"
//#include <libxml/xmlmemory.h>
//#include <libxml/parser.h>

#include <sstream>

bool Player::operator <(const Player& t) const {
    return(strcmp(this->name, t.name) < 0);
}

//*********************************************************************
//						regenModifyDuration
//*********************************************************************

long Player::tickInterval(Stat& stat, bool fastTick, bool deathSickness, bool vampAndDay, const bstring& effectName) {
	long interval = 45 - 5*bonus(stat.getCur());

	if(fastTick)
		interval /= 2;
	if(deathSickness && mrand(1,100) < 50)
		interval += 10;
	if(vampAndDay)
		interval += 15;

	// effectName will be regeneration or well-of-magic
	EffectInfo* effect = getEffect(effectName);
	if(effect) {
		double reduction = effect->getStrength(); // 1-40
		reduction = 1 - (reduction / 100);
		interval *= reduction;
	}

	return(interval);
}

//*********************************************************************
//						pulseTick
//*********************************************************************
// return true if they were destroyed, false if they still exist

void Player::pulseTick(long t) {
	BaseRoom* room = getRoomParent();
	bool ill = false;
	bool noTick = false;
	bool fastTick = room && room->isFastTick();
	bool vampAndDay = isDay() && isNewVampire();
	bool vampAndSunlight = vampAndDay && room && room->isSunlight();
	bool hardcore = isHardcore();
	bool deathSickness = isEffected("death-sickness");
	bool wounded = isEffected("wounded");

	long mainTick = LT(this, LT_TICK);
	long secTick = LT(this, LT_TICK_SECONDARY);
	long badTick = LT(this, LT_TICK_HARMFUL);

	if(!getSock())
		return;


	ill = isPoisoned() || isDiseased() || wounded || isEffected("petrification");
	noTick = (	vampAndSunlight ||
				isEffected("petrification") ||
				wounded
			);

	// ****** Main Tick ******
	if(mainTick < t && !flagIsSet(P_NO_TICK_HP) && !noTick) {
		int hpTickAmt = 0;
		if(cClass != LICH) {
			hpTickAmt = MAX(1, 3 + bonus(constitution.getCur()) + (cClass == BERSERKER ? 2:0));
			if(!ill && !deathSickness) {
				if(!isEffected("bloodsac")) {
					if(flagIsSet(P_SITTING))
						hpTickAmt += 1;
					else if(flagIsSet(P_SLEEPING))
						hpTickAmt += 3;
				}
				if(fastTick)
					hpTickAmt += 3;
			}
		} else if(cClass == LICH && !flagIsSet(P_NO_TICK_MP)) {
			hpTickAmt = MAX(1, 4+(constitution.getCur() > 170 ? 1:0));
			if(!ill && !deathSickness) {
				if(flagIsSet(P_SITTING))
					hpTickAmt += 1;
				if(fastTick)
					hpTickAmt += 2;
			}
		}

		int hpIncrease = hp.increase(hpTickAmt);
		if(!ill && !deathSickness && !vampAndDay)
			hpIncrease += hp.increase(getHpTickBonus());

		lasttime[LT_TICK].ltime = t;
		lasttime[LT_TICK].interval = tickInterval(constitution, fastTick, deathSickness, vampAndDay, "regeneration");

		if(flagIsSet(P_SHOW_TICK) && hpIncrease)
			print("TICK: HP - %d(+%d)\n", hp.getCur(), hpIncrease);

		clearFlag(P_JUST_STAT_MOD);
	}
	// ****** End Main Tick ******

	// ****** Secondary Tick ******
	// It's time to tick and (they don't have NO_MP_TICK set OR they're a pure fighter) and they're not a lich
	if(secTick < t) {
		if(!flagIsSet(P_NO_TICK_MP) && cClass != LICH && !noTick &&
				!(cClass == FIGHTER && !cClass2 && flagIsSet(P_PTESTER)))
		{
			// Non fighters handle here
			int mpTickAmt = 0;
			if(cClass == MAGE ||
				(cClass2 == MAGE && (cClass == FIGHTER || cClass == THIEF)) )
				mpTickAmt += 2;

			mpTickAmt += MAX(1, 2+(intelligence.getCur() > 170 ? 1:0));

			if(!ill && !deathSickness) {
				if(flagIsSet(P_SITTING))
					mpTickAmt += 1;
				else if(flagIsSet(P_SLEEPING))
					mpTickAmt += 3;

				if(fastTick)
					mpTickAmt += 2;
			}
			int mpIncrease = mp.increase(mpTickAmt);
			if(!ill && !deathSickness && !vampAndDay)
				mpIncrease += mp.increase(getMpTickBonus());

			lasttime[LT_TICK_SECONDARY].ltime = t;
			lasttime[LT_TICK_SECONDARY].interval = tickInterval(piety, fastTick, deathSickness, vampAndDay, "well-of-magic");

			if(flagIsSet(P_SHOW_TICK) && mpIncrease)
				print("TICK: MP - %d(+%d)\n", mp.getCur(), mpIncrease);

		} else if(cClass == FIGHTER && !cClass2 && flagIsSet(P_PTESTER)) {
			// Pure fighters handled here
			if(!inCombat())
				decreaseFocus();

			// Set secondary tick timer here for fighters
			lasttime[LT_TICK_SECONDARY].ltime = t;
			lasttime[LT_TICK_SECONDARY].interval = 2;

		}

		clearFlag(P_JUST_STAT_MOD);
	}
	// ****** End Secondary Tick ******

	// ****** Bad Tick ******
	// Poison and room harms and such
	if(badTick < t) {
		// Handle DoT effects
		// a hardcore death will invalidate us
		if(doDoTs() && hardcore) {
			//zero(this, sizeof(this));
			return;
		}

		// a hardcore death will invalidate us
		if(doPlayerHarmRooms() && hardcore) {
			//zero(this, sizeof(this));
			return;
		}

		// Set bad tick timer here
		lasttime[LT_TICK_HARMFUL].ltime = t;
		if(flagIsSet(P_OUTLAW) && room && room->isOutlawSafe()) {
			// Outlaws hiding in an outlaw safe room get hurt more often
			lasttime[LT_TICK_HARMFUL].interval = 20;
		} else {
			// If you have higher piety, you get hurt less often
			if(cClass != LICH)
				lasttime[LT_TICK_HARMFUL].interval = 45 + 5*bonus((int)piety.getCur());
			else
				lasttime[LT_TICK_HARMFUL].interval = 45 + 5*bonus((int)constitution.getCur());

			if(isEffected("vampirism") && isDay())
				lasttime[LT_TICK_HARMFUL].interval -= 15;
		}
	}
	// ****** End Bad Tick ******

	// a hardcore death will invalidate us
	if(vampAndSunlight && sunlightDamage() && hardcore) {
	//	zero(this, sizeof(this));
		return;
	}

	if(wounded && isEffected("regeneration")) {
		print("Your regeneration spell heals your festering wounds.\n");
		removeEffect("wounded");
		removeEffect("regeneration");
	}
}

//*********************************************************************
//						getHpTickBonus
//*********************************************************************

int Player::getHpTickBonus() const {
	int bonus = 0;
	switch(cClass) {
	case FIGHTER:
		if(!cClass2 || cClass2 == THIEF)
			bonus = level/6;
		break;
	// More or less pure fighter classes - fall through cases
	case ASSASSIN:
	case BERSERKER:
	case WEREWOLF:
	case ROGUE:
	case MONK:
	case THIEF:
		bonus = level/6;
		break;
	// Now handle the hybrids
	case RANGER:
	case PALADIN:
	case DEATHKNIGHT:
	case BARD:
	case PUREBLOOD:
		bonus = (level+4)/10;
		break;
	// And then the pure casters
	case MAGE:
	case CLERIC:
	case DRUID:
		bonus = 0;
		break;
	// Lich is a special case, using hp as mp...so give them a bonus here
	case LICH:
		bonus = level/6;
		break;
	default:
		bonus = 0;
		break;
	}
	return(bonus);
}

//*********************************************************************
//						getMpTickBonus
//*********************************************************************

int Player::getMpTickBonus() const {
	int bonus = 0;
	switch(cClass) {
	case FIGHTER:
		if(cClass2 && cClass2 == MAGE)
			bonus = level/6;
		break;
	// More or less pure fighter classes - fall through cases
	case ASSASSIN:
	case BERSERKER:
	case WEREWOLF:
	case ROGUE:
	case MONK:
	case THIEF:
		bonus = 0;
		break;
	// Now handle the hybrids
	case RANGER:
	case PALADIN:
	case DEATHKNIGHT:
	case BARD:
	case PUREBLOOD:
		bonus = (level+4)/10;
		break;
	// And then the pure casters
	case MAGE:
	case CLERIC:
	case DRUID:
		bonus = level/6;
		break;
	// Lich is a special case, using hp as mp...so no bonus here
	case LICH:
		bonus = 0;
		break;
	default:
		bonus = 0;
		break;
	}
	return(bonus);
}

//*********************************************************************
//						doDoTs
//*********************************************************************
// return true if they died, false if they lived

bool Player::doDoTs() {
	if(isStaff())
		return(false);

	if(doPetrificationDmg())
		return(true);

	return(false);
}

//*********************************************************************
//						winterProtection
//*********************************************************************

double Object::winterProtection() const {
	double percent = 0;

	if(subType != "cloth" && subType != "leather")
		return(0);

	switch(wearflag) {
	case BODY:
		percent = 1;
		break;
	case ARMS:
	case LEGS:
	case NECK:
	case HANDS:
	case HEAD:
	case FEET:
	case FACE:
		percent = getLocationModifier() * 2;
		break;
	default:
		break;
	}

	if(subType == "cloth")
		percent /= 2;

	return(percent);
}

double Player::winterProtection() const {
	double	percent = 0;
	int		i=0;

	if(cClass == LICH || isEffected("lycantrhopy") || race == MINOTAUR)
		return(1);

	// how much protection do this person's equipment provide them?
	for(i=0; i<MAXWEAR; i++)
		if(ready[i])
			percent += ready[i]->winterProtection();

	return(MAX(0, MIN(1, percent)));
}

//*********************************************************************
//						doPlayerHarmRooms
//*********************************************************************
// return true if they died, false if they lived

bool Player::doPlayerHarmRooms() {
	DeathType dt;
	bool	prot = true;
	// gives us a range of 10-18 dmg
	int		dmg = 15 - (constitution.getCur() - 150)/50;

	BaseRoom* room = getRoomParent();

	// Poison rooms
	if(room->flagIsSet(R_POISONS) && !isPoisoned() && !immuneToPoison()) {
		wake("Terrible nightmares disturb your sleep!");
		printColor("^#^GThe toxic air poisoned you.\n");
		if(!isStaff()) {
			poison(0, 15, 210 + standardPoisonDuration(15, constitution.getCur()));
			poisonedBy = "noxious fumes";
		}
	}

	// Stun rooms
	if(room->flagIsSet(R_STUN)) {
		wake("You awaken suddenly!");
		print("The room starts to spin around you.\nYou feel confused.\n");
		if(!isStaff()) {
			updateAttackTimer(true, MAX(dice(2,6,0), 6));
			stun(mrand(3,9));
		}
	}

	// Mp Drain rooms
	if(room->flagIsSet(R_DRAIN_MANA) && !isStaff()) {
		wake("Terrible nightmares disturb your sleep!");
		if(cClass != LICH)
			mp.decrease(MIN(mp.getCur(),6));

		if(cClass == LICH) {
			hp.decrease(MIN(hp.getCur(), 6));
			if(hp.getCur() < 1)
				die(DRAINED);
		}
	}

	if(	room->flagIsSet(R_PLAYER_HARM) ||
		// these flags don't need PHARM set to work
		room->flagIsSet(R_DEADLY_VINES) ||
		room->flagIsSet(R_WINTER_COLD) ||
		room->flagIsSet(R_DESERT_HARM) ||
		room->flagIsSet(R_ICY_WATER)
	) {

		if( (	room->flagIsSet(R_FIRE_BONUS) ||
				(room->flagIsSet(R_DESERT_HARM) && isDay())
			)
		    && !isEffected("heat-protection") && !isEffected("alwayswarm")
		) {
			wake("You awaken suddenly");
			printColor("^rThe searing heat burns your flesh.\n");
			dt = BURNED;
			prot = false;
		} else if(	!isEffected("breathe-water") &&
					!doesntBreathe() && (
						room->flagIsSet(R_WATER_BONUS) ||(
						room->flagIsSet(R_UNDER_WATER_BONUS) ||
						(inAreaRoom() && getAreaRoomParent()->isWater() && !isEffected("fly"))
					)
		) ) {
			wake("You awaken suddenly!");
			printColor("^bWater fills your lungs.\n");
			dt = DROWNED;
			prot = false;
		} else if(room->flagIsSet(R_ICY_WATER) && cClass != LICH && !isEffected("warmth") && !isEffected("alwayscold")) {
			wake("You awaken suddenly!");
			printColor("^CThe ice cold water freezes your blood.\n");
			dt = COLDWATER;
			prot = false;
		} else if(room->flagIsSet(R_EARTH_BONUS) && !isEffected("earth-shield")) {
			wake("You awaken suddenly!");
			printColor("^YThe earth swells up around you and smothers you.\n");
			dt = SMOTHER;
			prot = false;
		} else if(room->flagIsSet(R_ELEC_BONUS) && !isEffected("static-field")) {
			wake("You awaken suddenly!");
			printColor("^WZAP! You are stuck by bolts of static electricity!\n");
			dt = LIGHTNING;
			prot = false;
		} else if(room->flagIsSet(R_AIR_BONUS) && !isEffected("fly")) {
			wake("You awaken suddenly!");
			printColor("^yTurbulent winds batter and thrash you about.\n");
			dt = WINDBATTERED;
			prot = false;
		} else if( ( room->flagIsSet(R_COLD_BONUS) ||
					(room->flagIsSet(R_DESERT_HARM) && !isDay() && cClass != LICH) ||
					(room->isWinter() && cClass != LICH)
				) &&
				!isEffected("warmth") &&
				!isEffected("alwayscold")
		) {
			prot = false;

			// winter cold is not as severe and is easier to protect against
			if(room->flagIsSet(R_WINTER_COLD)) {
				dmg -= 5;
				dmg -= (int)(dmg * winterProtection());

				if(!dmg)
					prot = true;
			}

			if(!prot) {
				wake("You awaken suddenly!");
				printColor("^bThe freezing temperature chills you to the bone.\n");
				dt = FROZE;
			}

		} else if(room->flagIsSet(R_DEADLY_VINES)) {
			if(cClass != DRUID && !isStaff() && deity != LINOTHAN && !isEffected("pass-without-trace")) {
				wake("You awaken suddenly!");
				printColor("^gLiving vines reach out and crush the air from your lungs.\n");
				dt = VINES;
				prot = false;
			} else {
				printColor("^gLiving vines crawl dangerously close to you, but ignore you.\n");
				prot = true;
			}
		}
		else if(!room->flagIsSet(R_AIR_BONUS) &&
				!room->flagIsSet(R_EARTH_BONUS) &&
				!room->flagIsSet(R_FIRE_BONUS) &&
				!room->flagIsSet(R_WATER_BONUS) &&
				!room->flagIsSet(R_POISONS) &&
				!room->flagIsSet(R_STUN) &&
				!room->flagIsSet(R_DRAIN_MANA) &&
				!room->flagIsSet(R_COLD_BONUS) &&
				!room->flagIsSet(R_ELEC_BONUS) &&
				!room->flagIsSet(R_AIR_BONUS) &&

				!room->flagIsSet(R_DEADLY_VINES) &&
				!room->flagIsSet(R_WINTER_COLD) &&
				!room->flagIsSet(R_DESERT_HARM) &&
				!room->flagIsSet(R_ICY_WATER)
		) {
			wake("You awaken suddenly!");
			printColor("^MAn invisible force saps your life force.\n");
			dt = DRAINED;
			prot = false;
		}
	}

	if(isStaff())
		prot = true;

	// Not protected
	if(!prot) {
		hp.decrease(MAX(1,dmg));
		if(hp.getCur() < 1) {
			die(dt);
			return(true);
		}
	}

	if(flagIsSet(P_OUTLAW) && room->isOutlawSafe()) {
		wake("Terrible nightmares disturb your sleep!");
		printColor("^#^rEnergy bolts blast you!\n");
		printColor("Don't stay in this room!\n");

		hp.decrease(hp.getMax()/4);
//		lasttime[LT_TICK].ltime = t;
//		lasttime[LT_TICK].interval = 20L;
		if(hp.getCur() < 1) {
			print("You are blown away by energy bolts. You died!\n");
			broadcast(getSock(), getRoomParent(), "%M was blown to bits by energy bolts. %s died.", this, heShe());
			die(BOLTS);
			return(true);
		}
	}

	return(false);
}

//*********************************************************************
//						getIdle
//*********************************************************************

time_t Player::getIdle() {
	return(time(0) - mySock->ltime);
}




//**********************************************************************
//						chooseItem
//**********************************************************************
// This function randomly chooses an item that the player pointed to
// by the first argument is wearing.

int Player::chooseItem() {
	char    checklist[MAXWEAR];
	int numwear=0, i;

	for(i=0; i<MAXWEAR; i++) {
		checklist[i] = 0;
		if(i==WIELD-1 || i==HELD-1)
			continue;
		if(ready[i])
			checklist[numwear++] = i+1;
	}

	if(!numwear)
		return(0);

	i = mrand(0, numwear-1);
	return(checklist[i]);
}
