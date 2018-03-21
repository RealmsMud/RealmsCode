/*
 * stat.cpp
 *   Stat code.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <math.h>

#include "config.hpp"
#include "creatures.hpp"
#include "effects.hpp"
#include "levelGain.hpp"
#include "mud.hpp"
#include "playerClass.hpp"
#include "stats.hpp"



//#####################################################################
// Stat Modifier
//#####################################################################
StatModifier::StatModifier() {
    name = "none";
    modAmt = 0;
    modType = MOD_NONE;
}

StatModifier::StatModifier(bstring pName, int pModAmt, ModifierType pModType) {
    name = pName;
    modAmt = pModAmt;
    modType = pModType;
}

void StatModifier::adjust(int adjAmount) {
    modAmt += adjAmount;
}

void StatModifier::set(int newAmt) {
    modAmt = newAmt;
}
void StatModifier::setType(ModifierType newType) {
    modType = newType;
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

double getConBonusPercentage(int pCon) {
//    const double a = 0.000002102555823;
//    const double b = 0.001366762953;
//    const double c = 0.982621217;
    const double a = 0.000007672564844;
    const double b = -0.0004485369366;
    const double c = 0.9939294404;
    const int x = pCon;
    double percentage = ((a*x*x)+(b*x)+c);
    percentage = MAX<double>(1.0, percentage)-1.0;
    return(percentage);

}

double getIntBonusPercentage(int pInt) {

    const double a =  0.000004207194636;
    const double b = -0.0004146634469;
    const double c =  1.00300606;

    const int x = pInt;
    double percentage = ((a*x*x)+(b*x)+c);
    percentage = MAX<double>(1.0, percentage)-1.0;
    return(percentage);

}

void Stat::reCalc() {
    if(!dirty)
        return;

    if(influencedBy) {
        if(name.equals("Hp"))
            setModifier("ConBonus", 0, MOD_MAX);
        else if(name.equals("Mp"))
            setModifier("IntBonus", 0, MOD_MAX);
    }

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


    if(influencedBy && name.equals("Hp")) {
        double percentage = getConBonusPercentage(influencedBy->getCur());
        int rounding = this->getModifierAmt("Rounding");
        int conBonus = (max-rounding) * percentage;
        setModifier("ConBonus", conBonus, MOD_MAX);

        max += conBonus;
    }
    if(influencedBy && name.equals("Mp")) {
        double percentage = getIntBonusPercentage(influencedBy->getCur());
        int rounding = this->getModifierAmt("Rounding");
        int intBonus = (max-rounding) * percentage;
        setModifier("IntBonus", intBonus, MOD_MAX);

        max += intBonus;
    }

    if(cur > max) {
        adjustModifier("CurModifier", max - cur);
        cur -= (cur - max);
    }
    dirty = false;
}
StatModifier* Stat::getModifier(bstring name) {
    ModifierMap::iterator it = modifiers.find(name);
    if(it == modifiers.end())
        return(nullptr);
    else
        return(it->second);
}
int Stat::getModifierAmt(bstring name) {
    StatModifier* mod = getModifier(name);
    if(mod)
        return(mod->getModAmt());
    else
        return(0);
}
Stat* Creature::getStat(bstring statName) {
    if(statName == "strength") {
        return(&strength);
    } else if(statName == "dexterity") {
        return(&dexterity);
    } else if(statName == "constitution") {
        return(&constitution);
    } else if(statName == "intelligence") {
        return(&intelligence);
    } else if(statName == "piety") {
        return(&piety);
    } else if(statName == "hp") {
        return(&hp);
    } else if(statName == "mp") {
        return(&mp);
    } else if(statName == "focus" && isPlayer()) {
        return(&getAsPlayer()->focus);
    } else {
        return(nullptr);
    }

}

bool Creature::addStatModifier(bstring statName, bstring modifierName, int modAmt, ModifierType modType) {
    return(addStatModifier(statName, new StatModifier(modifierName, modAmt, modType)));
}
bool Creature::addStatModifier(bstring statName, StatModifier* statModifier) {
    Stat* stat = getStat(statName);

    if(!stat) {
        delete(statModifier);
        return(false);
    }

    stat->addModifier(statModifier);
    if(statName.equals("constitution"))
        hp.setDirty();
    else if(statName.equals("intelligence"))
        mp.setDirty();

    return(true);
}
bool Creature::setStatDirty(bstring statName) {
    Stat* stat = getStat(statName);

    if(!stat)
        return(false);

    stat->setDirty();
    return(true);
}

bool Stat::addModifier(StatModifier* toAdd) {
    if(!toAdd)
        return(false);

    if(getModifier(toAdd->getName()) != nullptr) {
        std::clog << "Not adding modifer " << toAdd->getName() << std::endl;
        delete toAdd;
        return(false);
    }
    modifiers.insert(ModifierMap::value_type(toAdd->getName(), toAdd));
    setDirty();
    return(true);
}
void Stat::setDirty() {
    dirty = true;
    if(influences)
        influences->setDirty();
}
bool Stat::addModifier(bstring name, int modAmt, ModifierType modType) {
    if(getModifier(name) != nullptr)
        return(false);
    return(addModifier(new StatModifier(name, modAmt, modType)));
}

bool Stat::removeModifier(bstring name) {
    ModifierMap::iterator it = modifiers.find(name);
    if(it == modifiers.end())
        return(false);

    delete it->second;
    modifiers.erase(it);
    setDirty();
    return(true);
}
void Stat::clearModifiers() {
    ModifierMap::iterator it = modifiers.begin();

    while(it != modifiers.end()) {
        delete it->second;
        modifiers.erase(it++);
    }
}
bool Stat::adjustModifier(bstring name, int modAmt, ModifierType modType) {
    StatModifier* mod = getModifier(name);
    if(!mod) {
        if(modAmt == 0)
            return(true);
        mod = new StatModifier(name, 0, modType);
        modifiers.insert(ModifierMap::value_type(name, mod));
    }
    mod->adjust(modAmt);

    if(mod->getModAmt() == 0) {
        return(removeModifier(name));
    }
    else
        mod->setType(modType);
    setDirty();
    return(true);
}

bool Stat::setModifier(bstring name, int newAmt, ModifierType modType) {
    StatModifier* mod = getModifier(name);
    if(newAmt == 0) {
        if(!mod) {
            return(true);
        }
        else {
            return(removeModifier(name));
        }
    }
    if(!mod) {
        mod = new StatModifier(name, 0, modType);
        modifiers.insert(ModifierMap::value_type(name, mod));
    }
    mod->set(newAmt);
    mod->setType(modType);
    setDirty();
    return(true);

}

//*********************************************************************
//                      Stat
//*********************************************************************
Stat::Stat() {
     cur = max = initial = 0;
     dirty = true;
     influences = influencedBy = 0;
}

StatModifier::StatModifier(StatModifier &sm) {
    name = sm.name;
    modAmt = sm.modAmt;
    modType = sm.modType;
}

Stat& Stat::operator=(const Stat& st) {
    doCopy(st);
    return(*this);
}
void Stat::doCopy(const Stat& st) {
    StatModifier* mod = 0;
    for(ModifierMap::value_type p : st.modifiers) {
        mod = new StatModifier();
        (*mod) = (*p.second);
        modifiers.insert(ModifierMap::value_type(mod->getName(), mod));
    }
    name = st.name;
    cur = st.cur;
    max = st.max;
    initial = st.initial;
    dirty = st.dirty;
    influences = 0;
    influencedBy = 0;
}

Stat::~Stat() {
    for(ModifierMap::value_type p : modifiers) {
        delete p.second;
    }
    modifiers.clear();
}

void Stat::setName(bstring pName) {
    name = pName;
}

//*********************************************************************
//                      adjust
//*********************************************************************

int Stat::adjust(int amt) {
    if(amt > 0)
        return(increase(amt));
    else
        return(-1 * decrease(amt*-1));
}

//*********************************************************************
//                      increase
//*********************************************************************

// Legacy for hp.increase()
int Stat::increase(int amt) {
    if(amt < 0)
        return(0);
        
    
    int increaseAmt = MAX(0, MIN(amt, getMax() - getCur()));
        
    adjustModifier("CurModifier", increaseAmt);
    
    return(increaseAmt);
}

//*********************************************************************
//                      decrease
//*********************************************************************

// Legacy for hp.decrease()
int Stat::decrease(int amt) {
    if(amt < 0)
        return(0);
    
    int decreaseAmt = MIN(amt, getCur());
    
    adjustModifier("CurModifier", -decreaseAmt);
    
    return(decreaseAmt);
}

//*********************************************************************
//                      getCur
//*********************************************************************

int Stat::getCur(bool recalc) {
    if(recalc)
        reCalc();
    return(cur);
}

//*********************************************************************
//                      getMax
//*********************************************************************

int Stat::getMax() {
    reCalc();
    return(max);
}

//*********************************************************************
//                      getInitial
//*********************************************************************

int Stat::getInitial() const { return(initial); }

//*********************************************************************
//                      addInitial
//*********************************************************************

void Stat::addInitial(int a) { initial = MAX(1, initial + a); setDirty(); }

//*********************************************************************
//                      setMax
//*********************************************************************
double round(double r) {
    return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5);
}

void Stat::setMax(int newMax, bool allowZero) {
    newMax = MAX(allowZero ? 0 : 1, MIN(newMax, 30000));

    int dmSet = getModifierAmt("DmSet");
    int rounding = getModifierAmt("Rounding");
    int adjustment = 0;
    bool setHp = false, setMp = false;
    if(name.equals("Hp"))
        setHp = true;
    else if(name.equals("Mp"))
        setMp = true;

    if((setHp || setMp) && influencedBy) {
        double percentage =  0;
        int bonus = 0;
        if(setHp) {
            bonus = getModifierAmt("ConBonus");
            percentage = getConBonusPercentage(influencedBy->getCur());
        }
        if(setMp) {
            bonus = getModifierAmt("IntBonus");
            percentage = getIntBonusPercentage(influencedBy->getCur());
        }

        // Target max value we want
        double targetMax = newMax;

        // Target max value we want before any bonus
        double target = targetMax / (1.0+percentage);
        // Current amount, less bonus and any set amounts
        int curMax = getMax() - bonus - dmSet - rounding;
        // Adjustment needed to get to the target value before bonus
        adjustment = round(target) - curMax;

        // Calculated max based on new adjustment, without any rounding modifier
        int adjMax = (curMax + adjustment) * (1.0+percentage);

        this->setModifier("DmSet", adjustment, MOD_MAX);

        // Due to rounding with doubles we might miss the target by 1, this will adjust it
        if(adjMax != newMax) {
            setModifier("Rounding", newMax - adjMax, MOD_MAX);
        } else {
            setModifier("Rounding", 0, MOD_MAX);
        }
    } else {
        int curMax = getMax() - dmSet;
        adjustment = newMax - curMax;
        this->setModifier("DmSet", adjustment, MOD_MAX);
    }


}

//*********************************************************************
//                      setCur
//*********************************************************************

void Stat::setCur(int newCur) {
    newCur = MIN(newCur, getMax());
    int modCur = newCur - getCur();
    adjustModifier("CurModifier", modCur);
}

void Stat::setInfluences(Stat* pInfluences) {
    influences = pInfluences;
}
void Stat::setInfluencedBy(Stat* pInfluencedBy) {
    influencedBy = pInfluencedBy;
}

std::ostream& operator<<(std::ostream& out, Stat& stat) {
    out << stat.toString();
    return(out);
}

bstring Stat::toString() {
    std::ostringstream oStr;

    oStr << "^C" << name << ": ^c" << getCur() << "/" << getMax() << "(" << getInitial() << ")\n";
    int i = 1;
    for(ModifierMap::value_type p : modifiers) {
        StatModifier* mod = p.second;
        oStr << "\t" << i++ << ") ";
        oStr << "^C" << mod->getName() << "^c ";
        switch(mod->getModType()) {
            case MOD_CUR:
                oStr << "CUR ";
                break;
            case MOD_MAX:
                oStr << "MAX ";
                break;
            case MOD_CUR_MAX:
                oStr << "CUR+MAX ";
                break;
            default:
                oStr << "UNKNOWN ";
                break;
        }
        if(mod->getModAmt() >= 0)
            oStr << "+";
        oStr << mod->getModAmt() << "\n";
    }
    return(oStr.str());
}

//*********************************************************************
//                      setInitial
//*********************************************************************

void Stat::setInitial(int i) { initial = i; setDirty(); }

//*********************************************************************
//                      restore
//*********************************************************************

int Stat::restore() {
    if(cur < max)
        setCur(max);
    return(cur);
}

//*********************************************************************
//                      modifyStatTotalByEffect
//*********************************************************************

int modifyStatTotalByEffect(const Player* player, const bstring& effect) {
    const EffectInfo* ef = player->getEffect(effect);
    if(ef)
        return(ef->getStrength());
    return(0);
}

//*********************************************************************
//                      statsAddUp
//*********************************************************************

bool Player::statsAddUp() const {
    if(isStaff())
        return(true);
    if(flagIsSet(P_PTESTER))
        return(true);

    return(true);
}

//*********************************************************************
//                      addStatModEffect
//*********************************************************************

bool Creature::addStatModEffect(EffectInfo* effect) {
    Stat* stat=0;
    Player* pThis = getAsPlayer();
    bool good;
    ModifierType modType = MOD_CUR_MAX;

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
        stat = &intelligence;
    } else if(effect->getName() == "feeblemind") {
        good = false;
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
        stat = &constitution;
    } else if(effect->getName() == "weakness") {
        good = false;
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
    } else if(effect->getName() == "bloodsac") {
        modType = MOD_MAX;
        good = true;
        stat = &hp;
    }

    else {
        print("Unknown stat effect: %s\n", effect->getName().c_str());
        return(false);
    }

    int addAmt = effect->getStrength();
    if(good == false && addAmt > 0)
        addAmt *= -1;

    // Can't go outside the range
    int statMax = MAX_STAT_NUM;
    int statMin = MIN_STAT_NUM;

    // Different limits for hp & mp
    if(stat == &hp || stat == &mp) {
        statMax = 30000;
        statMin = 1;
    }
    addAmt = MIN(addAmt, statMax - stat->getCur());
    addAmt = MAX(addAmt, statMin - stat->getCur());
    
    effect->setStrength(addAmt);
    stat->addModifier(effect->getName(), addAmt, modType);

    if(pThis) {
        pThis->computeAttackPower();
        pThis->computeAC();
    }
    return(true);
}

//*********************************************************************
//                      remStatModEffect
//*********************************************************************

bool Creature::remStatModEffect(EffectInfo* effect) {
    Stat* stat=0;
    Player* pThis = getAsPlayer();

    if(effect->getName() == "strength" || effect->getName() == "enfeeblement") {
        stat = &strength;
    } else if(effect->getName() == "haste" || effect->getName() == "slow") {
        stat = &dexterity;
    } else if(effect->getName() == "insight" || effect->getName() == "feeblemind") {
        stat = &intelligence;
    } else if(effect->getName() == "prayer" || effect->getName() == "damnation") {
        stat = &piety;
    } else if(effect->getName() == "fortitude" || effect->getName() == "weakness") {
        stat = &constitution;
    } else if(effect->getName() == "berserk") {
        stat = &strength;
    } else if(effect->getName() == "frenzy") {
        stat = &dexterity;
    } else if(effect->getName() == "pray") {
        stat = &piety;
    } else if(effect->getName() == "dkpray") {
        stat = &strength;
    } else if(effect->getName() == "bloodsac") {
        stat = &hp;
    }
    else {
        print("Unknown stat effect: %s\n", effect->getName().c_str());
        return(false);
    }

    stat->removeModifier(effect->getName());


    if(pThis) {
        pThis->computeAttackPower();
        pThis->computeAC();
        pThis->setFlag(P_JUST_STAT_MOD);
    }
    return(true);
}

//*********************************************************************
//                      upgradeStats
//*********************************************************************

// Only used for upgradeStats
void Stat::upgradeSetCur(int newCur) {
    cur = newCur;
}

// Note: Used for upgradeStats
void checkEffect(Creature* creature, const bstring& effName, int& stat, bool positive)  {
    EffectInfo* eff = creature->getEffect(effName);
    if(eff) {
        int str = eff->getStrength();
        if(!positive)
            str *= -1;
        stat += str;
        creature->removeEffect(eff, false);
    }
}

void Player::recordLevelInfo() {
    std::clog << "Recording level info for " << getName() << std::endl;

    statistics.startLevelHistoryTracking();

    PlayerClass *pClass = gConfig->classes[getClassString()];
    LevelGain *lGain = 0;
    int hpAmt = 0;
    for(int l = level ; l > 1 ; l--) {
        lGain = pClass->getLevelGain(l);
        if(!lGain)
            continue;

        hpAmt = lGain->getHp();

        // Track level history
        statistics.setLevelInfo(l, new LevelInfo(l, lGain->getHp(), lGain->getMp(), lGain->getStat(), lGain->getSave(), time(0)));

    }
}
void Player::upgradeStats() {
    std::clog << "Upgrading stats for " << getName() << std::endl;
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

    hp.setInitial(hpAmt);
    mp.setInitial(pClass->getBaseMp());



    for(int l = level ; l > 1 ; l--) {
        lGain = pClass->getLevelGain(l);
        if(!lGain)
            continue;
        bstring modName = bstring("Level") + l;
        hpAmt = lGain->getHp();
        hp.addModifier(modName, hpAmt, MOD_CUR_MAX );

        if(cClass != CreatureClass::BERSERKER && cClass != CreatureClass::LICH) {
            mp.addModifier(modName, lGain->getMp(), MOD_CUR_MAX );
        }
        int switchNum = lGain->getStat();

        StatModifier* newMod = new StatModifier(modName, 10, MOD_CUR_MAX);
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

    }

    if(cClass == CreatureClass::FIGHTER && !hasSecondClass() && flagIsSet(P_PTESTER)) {
        focus.setInitial(100);
        focus.clearModifiers();
        focus.addModifier("UnFocused", -100, MOD_CUR);
        mp.setInitial(0);
        mp.clearModifiers();

    }
    strength.setInitial(cStr);
    dexterity.setInitial(cDex);
    constitution.setInitial(cCon);
    intelligence.setInitial(cInt);
    piety.setInitial(cPie);

//
//  std::clog << "Str: O: " << cStr << " N: " << strength.getCur() << "\n";
//  std::clog << "Dex: O: " << cDex << " N: " << dexterity.getCur() << "\n";
//  std::clog << "Con: O: " << cCon << " N: " << constitution.getCur() << "\n";
//  std::clog << "Int: O: " << cInt << " N: " << intelligence.getCur() << "\n";
//  std::clog << "Pie: O: " << cPie << " N: " << piety.getCur() << "\n";

}


void Monster::upgradeStats() {
    std::clog << "Upgrading stats for " << getName() << std::endl;
    *this << "Upgrading your stats to the new format.\n";

    int cStr = strength.getCur(false);
    int cDex = dexterity.getCur(false);
    int cCon = constitution.getCur(false);
    int cInt = intelligence.getCur(false);
    int cPie = piety.getCur(false);

    int cHp = hp.getCur(false);
    int cMp = mp.getCur(false);

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

    hp.setInitial(cHp);
    mp.setInitial(cMp);

    strength.setInitial(cStr);
    dexterity.setInitial(cDex);
    constitution.setInitial(cCon);
    intelligence.setInitial(cInt);
    piety.setInitial(cPie);

}
