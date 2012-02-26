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

void checkEffect(Player* player, const bstring& effName, int& stat, bool positive)  {
	EffectInfo* eff = player->getEffect(effName);
	if(eff) {
		int str = eff->getStrength();
		if(!positive)
			str *= -1;
		stat += str;
	}
}
void Player::upgradeStats() {
	std::cout << "Upgrading stats for " << getName() << std::endl;
	*this << "Upgrading your stats to the new format.\n";

	loseRage();
	loseFrenzy();
	losePray();

	int cStr = strength.getCur(false);
	int cDex = dexterity.getCur(false);
	int cCon = constitution.getCur(false);
	int cInt = intelligence.getCur(false);
	int cPie = piety.getCur(false);

	checkEffect(this, "strength", cStr, true);
	checkEffect(this, "enfeeblement", cStr, false);
	checkEffect(this, "haste", cDex, true);
	checkEffect(this, "slow", cDex, false);

	checkEffect(this, "fortitude", cCon, true);
	checkEffect(this, "weakness", cCon, false);

	checkEffect(this, "insight", cInt, true);
	checkEffect(this, "feeblemind", cInt, false);

	checkEffect(this, "prayer", cPie, true);
	checkEffect(this, "damnation", cPie, false);

	PlayerClass *pClass = gConfig->classes[getClassString()];
	LevelGain *lGain = 0;

	int hpAmt = pClass->getBaseHp();

    if(cClass != LICH) {
        if(cClass == BERSERKER && cCon >= 70)
            hpAmt++;
        if(cCon >= 130)
            hpAmt++;
        if(cCon >= 210)
            hpAmt++;
        if(cCon >= 250)
            hpAmt++;
    }

	hp.setInitial(hpAmt);
	mp.setInitial(pClass->getBaseMp());



	for(int l = level ; l > 1 ; l--) {
		lGain = pClass->getLevelGain(l);
		if(!lGain)
			continue;
		bstring modName = bstring("Level") + l;
		hpAmt = lGain->getHp();
        // Make constitution actually worth something
        if(cClass != LICH) {
            if(cClass == BERSERKER && cCon >= 70)
                hpAmt++;
            if(cCon >= 130)
                hpAmt++;
            if(cCon >= 210)
                hpAmt++;
            if(cCon >= 250)
                hpAmt++;
        }
		hp.addModifier(modName, hpAmt, MOD_CUR_MAX );

		if(cClass != BERSERKER && cClass != LICH) {
			mp.addModifier(modName, lGain->getMp(), MOD_CUR_MAX );
		}
		int switchNum = lGain->getStat();

		StatModifier* newMod = new StatModifier(modName, 10, MOD_CUR_MAX, false);
		switch(switchNum) {
		case STR:
			strength.addModifier(newMod);
			cStr -= 10;
			break;
		case DEX:
			dexterity.addModifier(newMod);
			cDex -= 10;
			break;
		case CON:
			constitution.addModifier(newMod);
			cCon -= 10;
			break;
		case INT:
			intelligence.addModifier(newMod);
			cInt -= 10;
			break;
		case PTY:
			piety.addModifier(newMod);
			cPie -= 10;
			break;
		}
		// Track level history
		statistics.setLevelInfo(l, new LevelInfo(hpAmt, lGain->getMp(), lGain->getStat(), lGain->getSave(), time(0)));

	}

    if(cClass == FIGHTER && !cClass2 && flagIsSet(P_PTESTER)) {
        focus.setInitial(100);
        focus.clearModifiers(true);
        focus.addModifier("UnFocused", -100, MOD_CUR, false);
        mp.setInitial(0);
        mp.clearModifiers(true);

    }
	strength.setInitial(cStr);
	dexterity.setInitial(cDex);
	constitution.setInitial(cCon);
	intelligence.setInitial(cInt);
	piety.setInitial(cPie);


	std::cout << "Str: O: " << cStr << " N: " << strength.getCur() << "\n";
	std::cout << "Dex: O: " << cDex << " N: " << dexterity.getCur() << "\n";
	std::cout << "Con: O: " << cCon << " N: " << constitution.getCur() << "\n";
	std::cout << "Int: O: " << cInt << " N: " << intelligence.getCur() << "\n";
	std::cout << "Pie: O: " << cPie << " N: " << piety.getCur() << "\n";

}
//#####################################################################
// Stat Modifier
//#####################################################################
StatModifier::StatModifier(bstring pName, int pModAmt, ModifierType pModType, bool pTemporary) {
	name = pName;
	modAmt = pModAmt;
	modType = pModType;
	temporary = pTemporary;
}

bool StatModifier::isTemporary() {
    return(temporary);
}
void StatModifier::adjust(int adjAmount) {
	modAmt += adjAmount;
}

bstring StatModifier::getName() {
	return(name);
}
int StatModifier::getModAmt() {
	return(modAmt);
}
ModifierType StatModifier:: getModType() {
	return(modType);
}

void Stat::reCalc() {
	if(!dirty)
		return;

	cur = initial;
	max = initial;

	for(ModifierMap::value_type p : modifiers) {
		StatModifier *mod = p.second;
		if(!mod)
			continue;
		switch(mod->getModType()) {
		case MOD_MAX:
			max += mod->getModAmt();
			break;
		case MOD_CUR:
			cur += mod->getModAmt();
			break;
		case MOD_CUR_MAX:
			max += mod->getModAmt();
			cur += mod->getModAmt();
			break;
		default:
			break;
		}
	}
	dirty = false;
}
StatModifier* Stat::getModifier(bstring name) {
	ModifierMap::iterator it = modifiers.find(name);
	if(it == modifiers.end())
		return(NULL);
	else
		return(it->second);
}
bool Stat::addModifier(StatModifier* toAdd) {
	if(!toAdd)
		return(false);

	if(getModifier(toAdd->getName()) != NULL) {
	    std::cout << "Not adding modifer " << toAdd->getName() << std::endl;
		delete toAdd;
		return(false);
	}

	modifiers.insert(ModifierMap::value_type(toAdd->getName(), toAdd));
	dirty = true;
	return(true);
}
bool Stat::addModifier(bstring name, int modAmt, ModifierType modType, bool temporary) {
	if(getModifier(name) != NULL)
		return(false);
	StatModifier* mod = new StatModifier(name, modAmt, modType);
	modifiers.insert(ModifierMap::value_type(name, mod));
	dirty = true;
	return(true);
}

bool Stat::removeModifier(bstring name) {
	ModifierMap::iterator it = modifiers.find(name);
	if(it == modifiers.end())
		return(false);

	modifiers.erase(it);
	dirty = true;
	return(true);
}
void Stat::clearModifiers(bool removePermanent = false) {
    ModifierMap::iterator it = modifiers.begin();

    while(it != modifiers.end()) {
        if(removePermanent || it->second->isTemporary()) {
            delete it->second;
            modifiers.erase(it++);
        }
    }
}
bool Stat::adjustModifier(bstring name, int modAmt) {
	StatModifier* mod = getModifier(name);
	if(!mod) {
		mod = new StatModifier(name, 0, MOD_CUR);
		modifiers.insert(ModifierMap::value_type(name, mod));
	}
	mod->adjust(modAmt);
	dirty = true;
	return(true);
}

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

// Legacy for hp.increase()
int Stat::increase(int amt, bool overMaxOk) {
	if(amt < 0)
		return(0);
		
	
	int increaseAmt;
	// One possibility is we're a DK under blood sacrifice.  Max of 1 1/2 max
	if(overMaxOk)
		increaseAmt = MAX(0, MIN(amt, (getMax()*3/2) - getCur()));
	else
		increaseAmt = MAX(0, MIN(amt, getMax() - getCur()));
		
	adjustModifier("Damage", -increaseAmt);
	
	return(increaseAmt);
}

//*********************************************************************
//						decrease
//*********************************************************************

// Legacy for hp.decrease()
int Stat::decrease(int amt) {
	if(amt < 0)
		return(0);
	
	int decreaseAmt = MIN(amt, getCur());
	
	adjustModifier("Damage", -decreaseAmt);
	
	return(decreaseAmt);
}

//*********************************************************************
//						getCur
//*********************************************************************

short Stat::getCur(bool recalc) {
	if(recalc)
		reCalc();
	return(cur);
}

//*********************************************************************
//						getMax
//*********************************************************************

short Stat::getMax() {
    reCalc();
    return(max);
}

short Stat::getPermMax() const {
    int toReturn = initial;

    for(ModifierMap::value_type p : modifiers) {
        StatModifier *mod = p.second;
        // Count only permenant modifiers (leveling)
        if(!mod || mod->isTemporary())
            continue;
        switch(mod->getModType()) {
        case MOD_MAX:
        case MOD_CUR_MAX:
            toReturn += mod->getModAmt();
            break;
        default:
            break;
        }
    }
    return(toReturn);
}
//*********************************************************************
//						getInitial
//*********************************************************************

short Stat::getInitial() const { return(initial); }

//*********************************************************************
//						addInitial
//*********************************************************************

void Stat::addInitial(short a) { initial = MAX(1, initial + a); }

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
	newCur = MIN(newCur, max);
	int modCur = newCur - getCur();
	adjustModifier("Damage", modCur);
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

	return(true);
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
			if(isEffected("berserk"))
			    removeEffect("berserk");
			if(isEffected("dkpray"))
				removeEffect("dkpray");
		}
		good = true;
		stat = &strength;
	} else if(effect->getName() == "enfeeblement") {
		good = false;
		stat = &strength;
	} else if(effect->getName() == "haste") {
		if(pThis && isEffected("frenzy"))
			removeEffect("frenzy");
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
		if(isEffected("pray"))
		    removeEffect("pray");
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
	} else if(effect->getName() == "berserk") {
	    good = true;
	    stat = &strength;
	} else if(effect->getName() == "frenzy") {
	    good = true;
	    stat = &dexterity;
	} else if(effect->getName() == "pray") {
	    good = true;
        stat = &piety;
	} else if(effect->getName() == "dkpray") {
	    good = true;
	    stat = &strength;
	}

	else {
		print("Unknown stat effect: %s\n", effect->getName().c_str());
		return(false);
	}

	int addAmt = effect->getStrength();
	if(good == false && addAmt > 0)
		addAmt *= -1;

	// Can't go outside the range
	addAmt = tMIN(addAmt, MAX_STAT_NUM - stat->getCur());
	addAmt = tMAX(addAmt, stat->getCur() - MIN_STAT_NUM);
	
	effect->setStrength(addAmt);
	stat->addModifier(effect->getName(), addAmt, MOD_CUR_MAX);

	if(hpMod) {
	    hp.addModifier(effect->getName(), hpMod, MOD_CUR_MAX);
		effect->setExtra(hpMod);
	} else if(mpMod) {
	    mp.addModifier(effect->getName(), mpMod, MOD_CUR_MAX);
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
	    hp.removeModifier(effect->getName());
	} else if(mpMod) {
        mp.removeModifier(effect->getName());
	}

	if(pThis) {
		pThis->computeAttackPower();
		pThis->computeAC();
		pThis->setFlag(P_JUST_STAT_MOD);
	}
	return(true);
}
