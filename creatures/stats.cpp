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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <cmath>                  // for ceil, floor, round
#include <ctime>                  // for time
#include <map>                    // for map
#include <ostream>                // for operator<<, basic_ostream, basic_os...

#include "bstring.hpp"            // for bstring, operator+
#include "config.hpp"             // for Config, gConfig
#include "creatures.hpp"          // for Player, Creature, Monster
#include "effects.hpp"            // for EffectInfo
#include "flags.hpp"              // for P_PTESTER, P_JUST_STAT_MOD
#include "global.hpp"             // for CreatureClass, CON, CreatureClass::...
#include "levelGain.hpp"          // for LevelGain
#include "playerClass.hpp"        // for PlayerClass
#include "statistics.hpp"         // for Statistics, LevelInfo
#include "stats.hpp"              // for Stat, StatModifier, ModifierMap
#include "utils.hpp"              // for MAX, MIN



//#####################################################################
// Stat Modifier
//#####################################################################
StatModifier::StatModifier() {
    name = "none";
    modAmt = 0;
    modType = MOD_NONE;
}

StatModifier::StatModifier(const bstring& pName, int pModAmt, ModifierType pModType) {
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

double getConBonusPercentage(unsigned int pCon) {
    const double a = 0.000007672564844;
    const double b = -0.0004485369366;
    const double c = 0.9939294404;
    const unsigned int x = pCon;
    double percentage = ((a*x*x)+(b*x)+c);
    percentage = MAX<double>(1.0, percentage)-1.0;
    return(percentage);

}

double getIntBonusPercentage(unsigned int pInt) {
    const double a =  0.000004207194636;
    const double b = -0.0004146634469;
    const double c =  1.00300606;

    const unsigned int x = pInt;
    double percentage = ((a*x*x)+(b*x)+c);
    percentage = MAX<double>(1.0, percentage)-1.0;
    return(percentage);

}

void Stat::reCalc() {
    if(!dirty) return;

    if(influencedBy) {
        if(name.equals("Hp"))
            setModifier("ConBonus", 0, MOD_MAX);
        else if(name.equals("Mp"))
            setModifier("IntBonus", 0, MOD_MAX);
    }

    cur = initial;
    max = initial;

    for(const ModifierMap::value_type& p : modifiers) {
        StatModifier *mod = p.second;
        if(!mod) continue;
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
        int conBonus = (int)((double)(max-rounding) * percentage);
        setModifier("ConBonus", conBonus, MOD_MAX);

        max += conBonus;
    }
    if(influencedBy && name.equals("Mp")) {
        double percentage = getIntBonusPercentage(influencedBy->getCur());
        int rounding = this->getModifierAmt("Rounding");
        int intBonus = (int)((double)(max-rounding) * percentage);
        setModifier("IntBonus", intBonus, MOD_MAX);

        max += intBonus;
    }

    if(cur > max) {
        adjustModifier("CurModifier", max - cur);
        cur -= (cur - max);
    }
    dirty = false;
}
StatModifier* Stat::getModifier(const bstring& pName) {
    auto it = modifiers.find(pName);
    if(it == modifiers.end()) return(nullptr);
    else                      return(it->second);
}
int Stat::getModifierAmt(const bstring& pName) {
    StatModifier* mod = getModifier(pName);
    if(mod) return(mod->getModAmt());
    else    return(0);
}
Stat* Creature::getStat(const bstring& statName) {
    if     (statName == "strength")            return(&strength);
    else if(statName == "dexterity")           return(&dexterity);
    else if(statName == "constitution")        return(&constitution);
    else if(statName == "intelligence")        return(&intelligence);
    else if(statName == "piety")               return(&piety);
    else if(statName == "hp")                  return(&hp);
    else if(statName == "mp")                  return(&mp);
    else if(statName == "focus" && isPlayer()) return(&getAsPlayer()->focus);
    else                                       return(nullptr);

}

bool Creature::addStatModifier(const bstring& statName, const bstring& modifierName, int modAmt, ModifierType modType) {
    return(addStatModifier(statName, new StatModifier(modifierName, modAmt, modType)));
}
bool Creature::addStatModifier(const bstring& statName, StatModifier* statModifier) {
    Stat* stat = getStat(statName);

    if(!stat) {
        delete(statModifier);
        return(false);
    }

    stat->addModifier(statModifier);
    if(statName.equals("constitution"))      hp.setDirty();
    else if(statName.equals("intelligence")) mp.setDirty();

    return(true);
}
bool Creature::setStatDirty(const bstring& statName) {
    Stat* stat = getStat(statName);

    if(!stat) return(false);

    stat->setDirty();
    return(true);
}

bool Stat::addModifier(StatModifier* toAdd) {
    if(!toAdd) return(false);

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
    if(influences) influences->setDirty();
}
bool Stat::addModifier(const bstring& name, int modAmt, ModifierType modType) {
    if(getModifier(name) != nullptr) return(false);
    return(addModifier(new StatModifier(name, modAmt, modType)));
}

bool Stat::removeModifier(const bstring& name) {
    auto it = modifiers.find(name);
    if(it == modifiers.end()) return(false);

    delete it->second;
    modifiers.erase(it);
    setDirty();
    return(true);
}
void Stat::clearModifiers() {
    auto it = modifiers.begin();

    while(it != modifiers.end()) {
        delete it->second;
        modifiers.erase(it++);
    }
}
bool Stat::adjustModifier(const bstring& name, int modAmt, ModifierType modType) {
    StatModifier* mod = getModifier(name);
    if(!mod) {
        if(modAmt == 0) return(true);
        mod = new StatModifier(name, 0, modType);
        modifiers.insert(ModifierMap::value_type(name, mod));
    }
    mod->adjust(modAmt);

    if(mod->getModAmt() == 0) return(removeModifier(name));
    else mod->setType(modType);

    setDirty();
    return(true);
}

bool Stat::setModifier(const bstring& name, int newAmt, ModifierType modType) {
    StatModifier* mod = getModifier(name);
    if(newAmt == 0) {
        if(!mod) return(true);
        else return(removeModifier(name));
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
     influences = influencedBy = nullptr;
}

StatModifier::StatModifier(StatModifier &sm) {
    name = sm.name;
    modAmt = sm.modAmt;
    modType = sm.modType;
}

Stat& Stat::operator=(const Stat& st) {
    if(this != &st)
        doCopy(st);

    return(*this);
}
void Stat::doCopy(const Stat& st) {
    StatModifier* mod = nullptr;
    for(const ModifierMap::value_type& p : st.modifiers) {
        mod = new StatModifier();
        (*mod) = (*p.second);
        modifiers.insert(ModifierMap::value_type(mod->getName(), mod));
    }
    name = st.name;
    cur = st.cur;
    max = st.max;
    initial = st.initial;
    dirty = st.dirty;
    influences = nullptr;
    influencedBy = nullptr;
}

Stat::~Stat() {
    for(const ModifierMap::value_type& p : modifiers) {
        delete p.second;
    }
    modifiers.clear();
}

void Stat::setName(const bstring& pName) {
    name = pName;
}

//*********************************************************************
//                      adjust
//*********************************************************************

unsigned int Stat::adjust(int amt) {
    if(amt > 0) return(increase(amt));
    else        return(-1 * decrease(amt * -1));
}

//*********************************************************************
//                      increase
//*********************************************************************

// Legacy for hp.increase()
unsigned int Stat::increase(unsigned int amt) {
    int increaseAmt = MAX<int>(0, MIN(amt, getMax() - getCur()));
        
    adjustModifier("CurModifier", increaseAmt);
    
    return(increaseAmt);
}

//*********************************************************************
//                      decrease
//*********************************************************************

// Legacy for hp.decrease()
unsigned int Stat::decrease(unsigned int amt) {
    int decreaseAmt = MIN(amt, getCur());
    
    adjustModifier("CurModifier", -decreaseAmt);
    
    return(decreaseAmt);
}

//*********************************************************************
//                      getCur
//*********************************************************************

unsigned int Stat::getCur(bool recalc) {
    if(recalc) reCalc();
    return(cur);
}

//*********************************************************************
//                      getMax
//*********************************************************************

unsigned int Stat::getMax() {
    reCalc();
    return(max);
}

//*********************************************************************
//                      getInitial
//*********************************************************************

unsigned int Stat::getInitial() const { return(initial); }

//*********************************************************************
//                      addInitial
//*********************************************************************

void Stat::addInitial(unsigned int a) { initial = MAX<int>(1, initial + a); setDirty(); }

//*********************************************************************
//                      setMax
//*********************************************************************
double round(double r) {
    return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5);
}

void Stat::setMax(unsigned int newMax, bool allowZero) {
    newMax = MAX<int>(allowZero ? 0 : 1, MIN<int>(newMax, 30000));

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
        unsigned int curMax = getMax() - bonus - dmSet - rounding;
        // Adjustment needed to get to the target value before bonus
        adjustment = (int)round(target) - (int)curMax;

        // Calculated max based on new adjustment, without any rounding modifier
        int adjMax = (int)((curMax + adjustment) * (1.0+percentage));

        this->setModifier("DmSet", adjustment, MOD_MAX);

        // Due to rounding with doubles we might miss the target by 1, this will adjust it
        if(adjMax != newMax) {
            setModifier("Rounding", newMax - adjMax, MOD_MAX);
        } else {
            setModifier("Rounding", 0, MOD_MAX);
        }
    } else {
        unsigned int curMax = getMax() - dmSet;
        adjustment = (int)newMax - (int)curMax;
        this->setModifier("DmSet", adjustment, MOD_MAX);
    }


}

//*********************************************************************
//                      setCur
//*********************************************************************

void Stat::setCur(unsigned int newCur) {
    newCur = MIN(newCur, getMax());
    int modCur = (int)newCur - (int)getCur();
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
    for(const ModifierMap::value_type& p : modifiers) {
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

void Stat::setInitial(unsigned int i) { initial = i; setDirty(); }

//*********************************************************************
//                      restore
//*********************************************************************

unsigned int Stat::restore() {
    if(cur < max) setCur(max);
    return(cur);
}

//*********************************************************************
//                      modifyStatTotalByEffect
//*********************************************************************

int modifyStatTotalByEffect(const Player* player, const bstring& effect) {
    const EffectInfo* ef = player->getEffect(effect);
    if(ef) return(ef->getStrength());
    return(0);
}

//*********************************************************************
//                      statsAddUp
//*********************************************************************

bool Player::statsAddUp() const {
    if(isStaff()) return(true);
    if(flagIsSet(P_PTESTER)) return(true);

    return(true);
}

//*********************************************************************
//                      addStatModEffect
//*********************************************************************

bool Creature::addStatModEffect(EffectInfo* effect) {
    Stat* stat=nullptr;
    Player* pThis = getAsPlayer();
    bool good;
    ModifierType modType = MOD_CUR_MAX;

    if(effect->getName() == "strength") {
        if(pThis) {
            if(isEffected("berserk")) removeEffect("berserk");
            if(isEffected("dkpray"))  removeEffect("dkpray");
        }
        good = true;
        stat = &strength;
    } else if(effect->getName() == "enfeeblement") {
        good = false;
        stat = &strength;
    } else if(effect->getName() == "haste") {
        if(pThis && isEffected("frenzy")) removeEffect("frenzy");
        good = true;
        stat = &dexterity;
    } else if(effect->getName() == "slow") {
        good = false;
        stat = &dexterity;
    } else if(effect->getName() == "insight") {
        if(pThis && isEffected("confusion")) pThis->removeEffect("confusion");
        good = true;
        stat = &intelligence;
    } else if(effect->getName() == "feeblemind") {
        good = false;
        stat = &intelligence;
    } else if(effect->getName() == "prayer") {
        if(isEffected("pray")) removeEffect("pray");
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
    if(!good && addAmt > 0)
        addAmt *= -1;

    // Can't go outside the range
    int statMax = MAX_STAT_NUM;
    int statMin = MIN_STAT_NUM;

    // Different limits for hp & mp
    if(stat == &hp || stat == &mp) {
        statMax = 30000;
        statMin = 1;
    }
    addAmt = MIN<int>(addAmt, statMax - stat->getCur());
    addAmt = MAX<int>(addAmt, statMin - stat->getCur());
    
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
    Stat* stat=nullptr;
    Player* pThis = getAsPlayer();
    const bstring& effectName = effect->getName();
    
    if(effectName == "strength" || effectName == "enfeeblement" || effectName == "berserk") {
        stat = &strength;
    } else if(effectName == "haste" || effectName == "slow" || effectName == "frenzy") {
        stat = &dexterity;
    } else if(effectName == "insight" || effectName == "feeblemind") {
        stat = &intelligence;
    } else if(effectName == "prayer" || effectName == "damnation" || effectName == "pray") {
        stat = &piety;
    } else if(effectName == "fortitude" || effectName == "weakness") {
        stat = &constitution;
    } else if(effectName == "dkpray") {
        stat = &strength;
    } else if(effectName == "bloodsac") {
        stat = &hp;
    }
    else {
        print("Unknown stat effect: %s\n", effectName.c_str());
        return(false);
    }

    stat->removeModifier(effectName);


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
void Stat::upgradeSetCur(unsigned int newCur) {
    cur = newCur;
}

// Note: Used for upgradeStats
void checkEffect(Creature* creature, const bstring& effName, unsigned int &stat, bool positive)  {
    EffectInfo* eff = creature->getEffect(effName);
    if(eff) {
        int str = eff->getStrength();
        if(!positive) str *= -1;
        stat += str;
        creature->removeEffect(eff, false);
    }
}

void Player::recordLevelInfo() {
    std::clog << "Recording level info for " << getName() << std::endl;

    statistics.startLevelHistoryTracking();

    PlayerClass *pClass = gConfig->classes[getClassString()];
    LevelGain *lGain = nullptr;
    for(int l = level ; l > 1 ; l--) {
        lGain = pClass->getLevelGain(l);
        if(!lGain)
            continue;

        // Track level history
        statistics.setLevelInfo(l, new LevelInfo(l, lGain->getHp(), lGain->getMp(), lGain->getStat(), lGain->getSave(), time(nullptr)));

    }
}
void Player::upgradeStats() {
    std::clog << "Upgrading stats for " << getName() << std::endl;
    *this << "Upgrading your stats to the new format.\n";

    loseRage();
    loseFrenzy();
    losePray();

    unsigned int cStr = strength.getCur(false);
    unsigned int cDex = dexterity.getCur(false);
    unsigned int cCon = constitution.getCur(false);
    unsigned int cInt = intelligence.getCur(false);
    unsigned int cPie = piety.getCur(false);

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
    LevelGain *lGain = nullptr;

    int hpAmt = pClass->getBaseHp();

    hp.setInitial(hpAmt);
    mp.setInitial(pClass->getBaseMp());


    for(int l = level ; l > 1 ; l--) {
        lGain = pClass->getLevelGain(l);
        if(!lGain) continue;
        bstring modName = bstring("Level") + l;
        hpAmt = lGain->getHp();
        hp.addModifier(modName, hpAmt, MOD_CUR_MAX );

        if(cClass != CreatureClass::BERSERKER && cClass != CreatureClass::LICH) {
            mp.addModifier(modName, lGain->getMp(), MOD_CUR_MAX );
        }
        int switchNum = lGain->getStat();

        auto* newMod = new StatModifier(modName, 10, MOD_CUR_MAX);
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
        default:
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
}


void Monster::upgradeStats() {
    std::clog << "Upgrading stats for " << getName() << std::endl;
    *this << "Upgrading your stats to the new format.\n";

    unsigned int cStr = strength.getCur(false);
    unsigned int cDex = dexterity.getCur(false);
    unsigned int cCon = constitution.getCur(false);
    unsigned int cInt = intelligence.getCur(false);
    unsigned int cPie = piety.getCur(false);

    unsigned int cHp = hp.getCur(false);
    unsigned int cMp = mp.getCur(false);

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
