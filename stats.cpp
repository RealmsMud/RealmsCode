/*
 * stat.cpp
 *	 Stat code.
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


//*********************************************************************
//						Stat
//*********************************************************************

Stat::Stat() {
	 cur = max = tmpMax = initial = 0; 
}

Stat::~Stat() {
}

//*********************************************************************
//						adjustMax
//*********************************************************************

int Stat::adjustMax(int amt) {
	if(amt > 0)
		return(increaseMax(amt));
	else
		return(-1 * decreaseMax(amt*-1));
}

//*********************************************************************
//						increaseMax
//*********************************************************************

int Stat::increaseMax(int amt) {
	if(max + amt > 30000)
		amt = 30000 - max;
	
	max += amt;
	return(amt);
}

//*********************************************************************
//						decreaseMax
//*********************************************************************

int Stat::decreaseMax(int amt) {
	if(max - amt < 0)
		amt = 0 - max;
	
	max -= amt;
	
	if(cur > max)
		cur = max;
	
	return(amt);
}

//*********************************************************************
//						adjust
//*********************************************************************

int Stat::adjust(int amt, bool overMaxOk) {
	if(amt > 0)
		return(increase(amt, overMaxOk));
	else
		return(-1 * decrease(amt*-1));
}

//*********************************************************************
//						increase
//*********************************************************************

int Stat::increase(int amt, bool overMaxOk) {
	if(amt < 0)
		return(0);
		
	
	int increaseAmt;
	// One possibility is we're a DK under blood sacrifice.  Max of 1 1/2 max
	if(overMaxOk)
		increaseAmt = MAX(0, MIN(amt, (max*3/2) - cur));
	else
		increaseAmt = MAX(0, MIN(amt, max - cur));
		
	cur += increaseAmt;
	
	return(increaseAmt);
}

//*********************************************************************
//						decrease
//*********************************************************************

int Stat::decrease(int amt) {
	if(amt < 0)
		return(0);
	
	int decreaseAmt = MIN(amt, cur);
	
	cur -= decreaseAmt;
	
	return(decreaseAmt);
}

//*********************************************************************
//						getCur
//*********************************************************************

short Stat::getCur() const { return(cur); }

//*********************************************************************
//						getMax
//*********************************************************************

short Stat::getMax() const { return(max); }

//*********************************************************************
//						getInitial
//*********************************************************************

short Stat::getInitial() const { return(initial); }

//*********************************************************************
//						addCur
//*********************************************************************

void Stat::addCur(short a) { cur = MAX(1, cur + a); }

//*********************************************************************
//						addMax
//*********************************************************************

void Stat::addMax(short a) { max = MAX(1, max + a); }

//*********************************************************************
//						setMax
//*********************************************************************

int Stat::setMax(short newMax, bool allowZero) {
	newMax = MAX(allowZero ? 0 : 1, MIN(newMax, 30000));
	max = newMax;
	if(cur > max)
		cur = max;
	return(max);
}

//*********************************************************************
//						setCur
//*********************************************************************

int Stat::setCur(short newCur) {
	cur = MIN(newCur, max);
	return(cur);
}

//*********************************************************************
//						setInitial
//*********************************************************************

void Stat::setInitial(short i) { initial = i; }

//*********************************************************************
//						restore
//*********************************************************************

int Stat::restore() {
	if(cur < max)
		cur = max;
	return(cur);
}

//*********************************************************************
//						modifyStatTotalByEffect
//*********************************************************************

int modifyStatTotalByEffect(const Player* player, const bstring& effect) {
	const EffectInfo* ef = player->getEffect(effect);
	if(ef)
		return(ef->getStrength());
	return(0);
}

//*********************************************************************
//						statsAddUp
//*********************************************************************

bool Player::statsAddUp() const {
	if(isStaff())
		return(true);
	if(flagIsSet(P_PTESTER))
		return(true);

	int has = strength.getCur() +
		dexterity.getCur() +
		constitution.getCur() +
		intelligence.getCur() +
		piety.getCur(); 

	int should = 560 + (level - 1) * 10;

	if(flagIsSet(P_PRAYED))
		should += cClass == DEATHKNIGHT ? 30 : 50;
	if(flagIsSet(P_FRENZY))
		should += 50;
	if(flagIsSet(P_BERSERKED))
		should += cClass == CLERIC && deity == ARES ? 30 : 50;

	should += modifyStatTotalByEffect(this, "enfeeblement");
	should += modifyStatTotalByEffect(this, "slow");
	should += modifyStatTotalByEffect(this, "weakness");
	should += modifyStatTotalByEffect(this, "feeblemind");
	should += modifyStatTotalByEffect(this, "damnation");

	should += modifyStatTotalByEffect(this, "strength");
	should += modifyStatTotalByEffect(this, "haste");
	should += modifyStatTotalByEffect(this, "fortitude");
	should += modifyStatTotalByEffect(this, "insight");
	should += modifyStatTotalByEffect(this, "prayer");

	// goblins with min intelligence should be given a little leeway
	if(race == GOBLIN && (should + 10) == has)
		return(true);
	return(should == has);
}

//*********************************************************************
//						adjustStat
//*********************************************************************

int adjustStat(Creature* creature, Stat* stat, int amt) {
	bool maxed = (stat->getCur() == stat->getMax());

	amt = stat->adjustMax(amt);

	if(maxed && creature->pFlagIsSet(P_JUST_STAT_MOD))
		maxed = false;

	if(maxed || stat->getCur() > stat->getMax())
		stat->setCur(stat->getMax());

	return(amt);
}

//*********************************************************************
//						addStatModEffect
//*********************************************************************

bool Creature::addStatModEffect(EffectInfo* effect) {
	Stat* stat=0;
	Player* pThis = getAsPlayer();
	bool good;
	int hpMod = 0;
	int mpMod = 0;

	if(effect->getName() == "strength") {
		if(pThis) {
			if(flagIsSet(P_BERSERKED))
				pThis->loseRage();
			if(cClass == DEATHKNIGHT && flagIsSet(P_PRAYED))
				pThis->losePray();
		}
		good = true;
		stat = &strength;
	} else if(effect->getName() == "enfeeblement") {
		good = false;
		stat = &strength;
	} else if(effect->getName() == "haste") {
		if(pThis && flagIsSet(P_FRENZY))
			pThis->loseFrenzy();
		good = true;
		stat = &dexterity;
	} else if(effect->getName() == "slow") {
		good = false;
		stat = &dexterity;
	} else if(effect->getName() == "insight") {
		if(pThis && isEffected("confusion"))
			pThis->removeEffect("confusion");
		good = true;
		mpMod = (int)(level * 1.5);
		stat = &intelligence;
	} else if(effect->getName() == "feeblemind") {
		good = false;
		mpMod = (int)(level * -1.5);
		stat = &intelligence;
	} else if(effect->getName() == "prayer") {
		if(pThis && cClass != DEATHKNIGHT && flagIsSet(P_PRAYED))
			pThis->losePray();
		good = true;
		stat = &piety;
	} else if(effect->getName() == "damnation") {
		good = false;
		stat = &piety;
	} else if(effect->getName() == "fortitude") {
		good = true;
		hpMod = (int)(level * 1.5);
		stat = &constitution;
	} else if(effect->getName() == "weakness") {
		good = false;
		hpMod = (int)(level * -1.5);
		stat = &constitution;
	} else {
		print("Unknown stat effect: %s\n", effect->getName().c_str());
		return(false);
	}

	int addAmt = effect->getStrength();
	if(good == false && addAmt > 0)
		addAmt *= -1;

	// Can't go outside the range
	addAmt = MIN(addAmt, MAX_STAT_NUM - stat->getCur());
	addAmt = MAX(addAmt, MIN_STAT_NUM - stat->getCur());
	
	effect->setStrength(addAmt);
	stat->addCur(addAmt);

	if(hpMod) {
		hpMod = adjustStat(this, &hp, hpMod);
		effect->setExtra(hpMod);
	} else if(mpMod) {
		mpMod = adjustStat(this, &mp, mpMod);
		effect->setExtra(mpMod);
	}

	if(pThis) {
		pThis->computeAttackPower();
		pThis->computeAC();
	}
	return(true);
}

//*********************************************************************
//						remStatModEffect
//*********************************************************************

bool Creature::remStatModEffect(EffectInfo* effect) {
	Stat* stat=0;
	Player* pThis = getAsPlayer();
	int hpMod = 0;
	int mpMod = 0;

	if(effect->getName() == "strength" || effect->getName() == "enfeeblement") {
		stat = &strength;
	} else if(effect->getName() == "haste" || effect->getName() == "slow") {
		stat = &dexterity;
	} else if(effect->getName() == "insight" || effect->getName() == "feeblemind") {
		mpMod = -1 * effect->getExtra();
		stat = &intelligence;
	} else if(effect->getName() == "prayer" || effect->getName() == "damnation") {
		stat = &piety;
	} else if(effect->getName() == "fortitude" || effect->getName() == "weakness") {
		hpMod = -1 * effect->getExtra();
		stat = &constitution;
	} else {
		print("Unknown stat effect: %s\n", effect->getName().c_str());
		return(false);
	}

	stat->adjust(effect->getStrength() * -1);

	if(hpMod) {
		adjustStat(this, &hp, hpMod);
	} else if(mpMod) {
		adjustStat(this, &mp, mpMod);
	}

	if(pThis) {
		pThis->computeAttackPower();
		pThis->computeAC();
		pThis->setFlag(P_JUST_STAT_MOD);
	}
	return(true);
}
