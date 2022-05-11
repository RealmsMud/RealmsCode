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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <fmt/format.h>              // for format
#include <cmath>                     // for ceil, floor, round
#include <ctime>                     // for time
#include <map>                       // for allocator, operator==, _Rb_tree_...
#include <ostream>                   // for operator<<, basic_ostream, char_...
#include <string>                    // for operator==, string, basic_string
#include <string_view>               // for operator==, string_view, basic_s...
#include <utility>                   // for pair

#include "config.hpp"                // for Config, gConfig
#include "creatureStreams.hpp"       // for Streamable
#include "effects.hpp"               // for EffectInfo
#include "flags.hpp"                 // for P_PTESTER, P_JUST_STAT_MOD
#include "global.hpp"                // for CreatureClass, CON, CreatureClas...
#include "levelGain.hpp"             // for LevelGain
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/players.hpp"    // for Player
#include "playerClass.hpp"           // for PlayerClass
#include "statistics.hpp"            // for Statistics, LevelInfo
#include "stats.hpp"                 // for Stat, StatModifier, ModifierMap
#include "utils.hpp"                 // for MAX, MIN



//#####################################################################
// Stat Modifier
//#####################################################################
StatModifier::StatModifier() {
    name = "none";
    modAmt = 0;
    modType = MOD_NONE;
}

StatModifier::StatModifier(std::string_view pName, int pModAmt, ModifierType pModType) {
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

std::string StatModifier::getName() {
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
        if(name == "Hp")
            setModifier("ConBonus", 0, MOD_MAX);
        else if(name == "Mp")
            setModifier("IntBonus", 0, MOD_MAX);
    }

    cur = initial;
    max = initial;

    for(const auto& [modId, mod] : modifiers) {
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


    if(influencedBy && name == "Hp") {
        double percentage = getConBonusPercentage(influencedBy->getCur());
        int rounding = this->getModifierAmt("Rounding");
        int conBonus = (int)((double)(max-rounding) * percentage);
        setModifier("ConBonus", conBonus, MOD_MAX);

        max += conBonus;
    }
    if(influencedBy && name == "Mp") {
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
bool Stat::hasModifier(const std::string &pName) {
    return (modifiers.find(pName) != modifiers.end());
}
std::shared_ptr<StatModifier>& Stat::getModifier(const std::string &pName) {
    auto it = modifiers.find(pName);
    if(it == modifiers.end())
        throw std::runtime_error("Requested unknown modifier " + pName);
    return(it->second);
}
int Stat::getModifierAmt(const std::string &pName) {
    if(!hasModifier(pName)) return(0);

    return getModifier(pName)->getModAmt();
}
Stat* Creature::getStat(std::string_view statName) {
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

bool Creature::addStatModifier(std::string_view statName, std::string_view modifierName, int modAmt, ModifierType modType) {
    return(addStatModifier(statName, new StatModifier(modifierName, modAmt, modType)));
}
bool Creature::addStatModifier(std::string_view statName, StatModifier* statModifier) {
    Stat* stat = getStat(statName);

    if(!stat) {
        delete(statModifier);
        return(false);
    }

    stat->addModifier(statModifier);
    if(statName == "constitution")      hp.setDirty();
    else if(statName == "intelligence") mp.setDirty();

    return(true);
}
bool Creature::setStatDirty(std::string_view statName) {
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
bool Stat::addModifier(const std::string &pName, int modAmt, ModifierType modType) {
    if(getModifier(pName) != nullptr) return(false);
    return(addModifier(new StatModifier(pName, modAmt, modType)));
}

bool Stat::removeModifier(const std::string &pName) {
    auto it = modifiers.find(pName);
    if(it == modifiers.end()) return(false);

    modifiers.erase(it);
    setDirty();
    return(true);
}
void Stat::clearModifiers() {
    modifiers.clear();
}
bool Stat::adjustModifier(const std::string &pName, int modAmt, ModifierType modType) {
    if(!hasModifier(pName)) {
        if(modAmt == 0) return(true);
        auto mod = std::make_shared<StatModifier>(pName, 0, modType);
        modifiers.insert(ModifierMap::value_type(pName, mod));
    }
    auto mod = getModifier(pName);
    mod->adjust(modAmt);

    if(mod->getModAmt() == 0) return(removeModifier(pName));
    else mod->setType(modType);

    setDirty();
    return(true);
}

bool Stat::setModifier(const std::string &pName, int newAmt, ModifierType modType) {
    bool hasMod = hasModifier(pName);
    if(newAmt == 0) {
        if(!hasMod) return(true);
        else return(removeModifier(pName));
    }
    if(!hasMod) {
        auto mod = new StatModifier(pName, 0, modType);
        modifiers.insert(ModifierMap::value_type(pName, mod));
    }
    auto mod = getModifier(pName);
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
    for(const auto& [modId, mod] : st.modifiers) {
        auto newMod = std::make_shared<StatModifier>(*mod.get());
        modifiers.insert(ModifierMap::value_type(newMod->getName(), newMod));
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
    modifiers.clear();
}

void Stat::setName(std::string_view pName) {
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
    if(name == "Hp")
        setHp = true;
    else if(name == "Mp")
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

std::string Stat::toString() {
    std::ostringstream oStr;

    oStr << "^C" << name << ": ^c" << getCur() << "/" << getMax() << "(" << getInitial() << ")\n";
    int i = 1;
    for(const auto& [modId, mod] : modifiers) {
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

int modifyStatTotalByEffect(const std::shared_ptr<Player>& player, std::string_view effect) {
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
    std::shared_ptr<Player> pThis = getAsPlayer();
    bool good;
    ModifierType modType = MOD_CUR_MAX;
    const auto& effectName = effect->getName();

    if(effectName == "strength") {
        if(pThis) {
            if(isEffected("berserk")) removeEffect("berserk");
            if(isEffected("dkpray"))  removeEffect("dkpray");
        }
        good = true;
        stat = &strength;
    } else if(effectName == "enfeeblement") {
        good = false;
        stat = &strength;
    } else if(effectName == "haste") {
        if(pThis && isEffected("frenzy")) removeEffect("frenzy");
        good = true;
        stat = &dexterity;
    } else if(effectName == "slow") {
        good = false;
        stat = &dexterity;
    } else if(effectName == "insight") {
        if(pThis && isEffected("confusion")) pThis->removeEffect("confusion");
        good = true;
        stat = &intelligence;
    } else if(effectName == "feeblemind") {
        good = false;
        stat = &intelligence;
    } else if(effectName == "prayer") {
        if(isEffected("pray")) removeEffect("pray");
        good = true;
        stat = &piety;
    } else if(effectName == "damnation") {
        good = false;
        stat = &piety;
    } else if(effectName == "fortitude") {
        good = true;
        stat = &constitution;
    } else if(effectName == "weakness") {
        good = false;
        stat = &constitution;
    } else if(effectName == "berserk") {
        good = true;
        stat = &strength;
    } else if(effectName == "frenzy") {
        good = true;
        stat = &dexterity;
    } else if(effectName == "pray") {
        good = true;
        stat = &piety;
    } else if(effectName == "dkpray") {
        good = true;
        stat = &strength;
    } else if(effectName == "bloodsac") {
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
    std::shared_ptr<Player> pThis = getAsPlayer();
    const auto& effectName = effect->getName();

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
        bPrint(fmt::format("Unknown stat effect: {}\n", effectName));
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
void checkEffect(const std::shared_ptr<Creature>& creature, std::string_view effName, unsigned int &stat, bool positive)  {
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

    auto pThis = Containable::downcasted_shared_from_this<Player>();
    checkEffect(pThis, "strength", cStr, true);
    checkEffect(pThis, "enfeeblement", cStr, false);
    checkEffect(pThis, "haste", cDex, true);
    checkEffect(pThis, "slow", cDex, false);

    checkEffect(pThis, "fortitude", cCon, true);
    checkEffect(pThis, "weakness", cCon, false);

    checkEffect(pThis, "insight", cInt, true);
    checkEffect(pThis, "feeblemind", cInt, false);

    checkEffect(pThis, "prayer", cPie, true);
    checkEffect(pThis, "damnation", cPie, false);

    PlayerClass *pClass = gConfig->classes[getClassString()];
    LevelGain *lGain = nullptr;

    int hpAmt = pClass->getBaseHp();

    hp.setInitial(hpAmt);
    mp.setInitial(pClass->getBaseMp());


    for(int l = level ; l > 1 ; l--) {
        lGain = pClass->getLevelGain(l);
        if(!lGain) continue;
        std::string modName = fmt::format("Level{}", l);
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

    auto mThis = Containable::downcasted_shared_from_this<Monster>();
    checkEffect(mThis, "strength", cStr, true);
    checkEffect(mThis, "enfeeblement", cStr, false);
    checkEffect(mThis, "haste", cDex, true);
    checkEffect(mThis, "slow", cDex, false);

    checkEffect(mThis, "fortitude", cCon, true);
    checkEffect(mThis, "weakness", cCon, false);

    checkEffect(mThis, "insight", cInt, true);
    checkEffect(mThis, "feeblemind", cInt, false);

    checkEffect(mThis, "prayer", cPie, true);
    checkEffect(mThis, "damnation", cPie, false);

    hp.setInitial(cHp);
    mp.setInitial(cMp);

    strength.setInitial(cStr);
    dexterity.setInitial(cDex);
    constitution.setInitial(cCon);
    intelligence.setInitial(cInt);
    piety.setInitial(cPie);

}
