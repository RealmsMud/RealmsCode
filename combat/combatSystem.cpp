/*
 * combatSystem.cpp
 *   Functions for the new combat mechanics
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * This software is distributed in accordance with the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <cstdlib>                   // for abs
#include <cstring>                   // for strcpy
#include <ctime>                     // for time, time_t
#include <set>                       // for operator==, _Rb_tree_const_iterator
#include <sstream>                   // for operator<<, basic_ostream, endl
#include <string>                    // for string, operator==, allocator

#include "config.hpp"                // for Config, gConfig
#include "damage.hpp"                // for Damage
#include "dice.hpp"                  // for Dice
#include "effects.hpp"               // for EffectInfo
#include "fighters.hpp"              // for FOCUS_DODGE, FOCUS_PARRY, FOCUS_...
#include "flags.hpp"                 // for M_ENCHANTED_WEAPONS_ONLY, M_PLUS...
#include "global.hpp"                // for CreatureClass, WIELD, CreatureCl...
#include "group.hpp"                 // for Group
#include "lasttime.hpp"              // for lasttime
#include "monType.hpp"               // for ASTRAL, AVIAN, DEMON, DEVA, DEVIL
#include "mud.hpp"                   // for LT_RIPOSTE, LT
#include "mudObjects/container.hpp"  // for MonsterSet
#include "mudObjects/creatures.hpp"  // for Creature, AttackResult, ATTACK_C...
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/objects.hpp"    // for Object, ObjectType, ObjectType::...
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, bonus, broadcastGroup
#include "random.hpp"                // for Random
#include "realm.hpp"                 // for NO_REALM
#include "skills.hpp"                // for SkillInfo, Skill
#include "statistics.hpp"            // for Statistics
#include "stats.hpp"                 // for Stat
#include "timer.hpp"                 // for Timer
#include "deityData.hpp"             // for deity names


const std::string NONE_STR = "none";

//**********************************************************************
//                      computeAttackPower
//**********************************************************************

int Player::computeAttackPower() {
    attackPower = 0;

    switch(cClass) {
        case CreatureClass::FIGHTER:
        case CreatureClass::DUNGEONMASTER:
            if(cClass2 == CreatureClass::MAGE)
                attackPower = (strength.getCur() * 2) + (level);
            else if(cClass2 == CreatureClass::THIEF)
                attackPower = (strength.getCur() * 2) + (level * 4);
            else
                attackPower = (strength.getCur() * 2) + (level * 8);
            break;
        case CreatureClass::BERSERKER:
            attackPower = (strength.getCur() * 2) + (level * 8);
            break;
        case CreatureClass::PALADIN:
        case CreatureClass::DEATHKNIGHT:
            // TODO: This might need some tweaking with pray
            attackPower = strength.getCur() + piety.getCur() + (level * 4);
            break;
        case CreatureClass::BARD:
        case CreatureClass::WEREWOLF:
        case CreatureClass::PUREBLOOD:
            attackPower = (strength.getCur() * 2) + (level * 4);
            break;
        case CreatureClass::RANGER:
        case CreatureClass::THIEF:
        case CreatureClass::ASSASSIN:
        case CreatureClass::ROGUE:
        case CreatureClass::MONK:
            attackPower = strength.getCur() + dexterity.getCur() + (level * 4);
            break;
        case CreatureClass::DRUID:
            attackPower = strength.getCur() + (level * 2);
            break;
        case CreatureClass::CLERIC:
            attackPower = strength.getCur();

            if(cClass2 == CreatureClass::ASSASSIN)
                attackPower += dexterity.getCur();

            switch(deity) {
                case CERIS:
                    attackPower += (level);
                    break;
                case LINOTHAN:
                    attackPower += (level * 4);
                case ARES:
                    attackPower += (level * 8);
                    break;
                case ENOCH:
                case KAMIRA:
                case JAKAR:
                case MARA:
                case GRADIUS:
                case ARAMON:
                case ARACHNUS:
                default:
                    attackPower += (level * 2);
                    break;
            }
            break;
        case CreatureClass::LICH:
        case CreatureClass::MAGE:
            attackPower = strength.getCur();
            if(cClass2 == CreatureClass::THIEF || cClass2 == CreatureClass::ASSASSIN)
                attackPower += dexterity.getCur() + (level*2);
            break;
        default:
            attackPower = strength.getCur();
            break;
    }

    // Check equipment here for higher attack power



    return(attackPower);
}

//****************************************************************************
//                      getBaseDamage
//****************************************************************************

int Creature::getBaseDamage() const {
    return(getAttackPower()/15);
}

//****************************************************************************
//                      getDamageReduction
//****************************************************************************
// Returns the damage reduction factor when attacking the given target

float Creature::getDamageReduction(const std::shared_ptr<Creature> & target) const {
    float reduction;
    auto targetArmor = (float)target->getArmor();
    float myLevel = level == 1 && isPlayer() ? 2 : (float)level;
    reduction = (float)((targetArmor)/((targetArmor) + 43.24 + 18.38*myLevel));
    reduction = std::min<float>(.75, reduction); // Max of 75% reduction
    return(reduction);
}

//**********************************************************************
//                      getWeaponType
//**********************************************************************

const std::string & Object::getWeaponType() const {
    if(type != ObjectType::WEAPON)
        return(NONE_STR);
    else
        return(subType);
}

//**********************************************************************
//                      getArmorType
//**********************************************************************

const std::string & Object::getArmorType() const {
    if(type != ObjectType::ARMOR)
        return(NONE_STR);
    else
        return(subType);
}

//**********************************************************************
//                      getWeaponCategory
//**********************************************************************

std::string Object::getWeaponCategory() const {
    const SkillInfo* weaponSkill = gConfig->getSkill(subType);

    if(type != ObjectType::WEAPON || !weaponSkill)
        return(NONE_STR);

    std::string category = weaponSkill->getGroup();
    // Erase the 'weapons-' to leave the category
    category.erase(0, 8);

    return(category);
}

//**********************************************************************
//                      getWeaponVerb
//**********************************************************************


std::string  Creature::getWeaponVerb() const {
    if(ready[WIELD-1])
        return(ready[WIELD-1]->getWeaponVerb());
    else
        return("thwack");

}

std::string Object::getWeaponVerb() const {
    const std::string category = getWeaponCategory();

    if(category == "crushing")
        return("crush");
    else if(category == "piercing")
        return("stab");
    else if(category == "slashing")
        return("slash");
    else if(category == "ranged")
        return("shoot");
    else if(category == "chopping")
        return("chop");
    else
        return("thwack");
}

//**********************************************************************
//                      getWeaponVerbPlural
//**********************************************************************


std::string Creature::getWeaponVerbPlural() const {
    if(ready[WIELD-1])
        return(ready[WIELD-1]->getWeaponVerbPlural());
    else
        return("thwacks");

}

std::string  Object::getWeaponVerbPlural() const {
    const std::string category = getWeaponCategory();

    if(category == "crushing")
        return("crushes");
    else if(category == "piercing")
        return("stabs");
    else if(category == "slashing")
        return("slashes");
    else if(category == "ranged")
        return("shoots");
    else if(category == "chopping")
        return("chops");
    else
        return("thwacks");
}

//**********************************************************************
//                      getWeaponVerbPast
//**********************************************************************

std::string Object::getWeaponVerbPast() const {
    const std::string category = getWeaponCategory();

    if(category == "crushing") {
        if(Random::get(0,1))
            return("crushed");
        else
            return("bludgeoned");
    }
    else if(category == "piercing") {
        if(Random::get(0,1))
            return("impaled");
        else
            return("stabbed");
    }
    else if(category == "slashing") {
        if(Random::get(0,1))
            return("gutted");
        else
            return("slashed");
    }
    else if(category == "ranged")
        return("shot");
    else if(category == "chopping") {
        if(Random::get(0,1))
            return("chopped");
        else
            return("cleaved");
    }
    else
        return("hit");
}

//**********************************************************************
//                      needsTwoHands
//**********************************************************************

bool Object::needsTwoHands() const {
    const std::string weaponType = getWeaponType();

    if (flagIsSet(O_TWO_HANDED))
        return (true);

    if (weaponType == "great-axe" || weaponType == "great-mace" ||
        weaponType == "great-hammer" || weaponType == "staff" ||
        weaponType == "great-sword" || weaponType == "polearm" || weaponType == "bow")
        return (true);

    if (weaponType == "crossbow" && !flagIsSet(O_SMALL_BOW))
        return (true);

    return (false);
}

//**********************************************************************
//                      setWeaponType
//**********************************************************************

bool Object::setWeaponType(const std::string &newType) {
    const SkillInfo* weaponSkill = gConfig->getSkill(newType);
    std::string category;
    if(weaponSkill) {
        category = weaponSkill->getGroup().substr(0, 6);
    }

    if(!weaponSkill || category != "weapon") {
        std::clog << "Invalid weapon type: " << newType << std::endl;
        return(false);
    }
    subType = newType;
    return(true);
}

//**********************************************************************
//                      setArmorType
//**********************************************************************

bool Object::setArmorType(const std::string &newType) {
    // Armor type must be ring, shield, or one of the armor type skills
    if(newType != "ring" && newType != "shield") {
        const SkillInfo* weaponSkill = gConfig->getSkill(newType);
        std::string category;
        if(weaponSkill) {
            category = weaponSkill->getGroup().substr(0, 5);
        }

        if(!weaponSkill || category != "armor") {
            std::clog << "Invalid armor type: " << newType << std::endl;
            return(false);
        }
    }
    subType = newType;
    return(true);
}

//**********************************************************************
//                      setSubType
//**********************************************************************

bool Object::setSubType(const std::string &newType) {
    // These must be set with the appropriate function
    if(type == ObjectType::ARMOR || type == ObjectType::WEAPON)
        return(false);

    subType = newType;
    return(true);
}

//**********************************************************************
//                      getWeaponSkill
//**********************************************************************

int Monster::getWeaponSkill(const std::shared_ptr<Object>  weapon) const {
    // Same skills for all weapons
    return(weaponSkill);
}

//**********************************************************************
//                      getClassWeaponskillBonus
//**********************************************************************

int Player::getClassWeaponskillBonus(const std::shared_ptr<Object>  weapon) const {
    int cBonus = 0;

    std::string weaponType;
    if(weapon)
        weaponType = weapon->getWeaponType();
    else
        weaponType = getUnarmedWeaponSkill();

    // we're very confused about what type of weapon this is
    if(weaponType.empty())
        weaponType = "bare-hand";

    if (getClass() == CreatureClass::CLERIC) {
        switch (getDeity()) {
        case MARA:
            if (weaponType == "bow")
                cBonus = 10;
            break;
        case LINOTHAN:
            if (weaponType == "great-sword")
                cBonus = 20;
            break;
        case ARACHNUS:
            if (weaponType == "whip")
                cBonus = 20;
            break;
        default:
            break;
        }
    }

    return(cBonus);
}

//**********************************************************************
//                      getRacialWeaponskillBonus
//**********************************************************************

int Player::getRacialWeaponskillBonus(const std::shared_ptr<Object>  weapon) const {
    int rBonus = 0;

    std::string weaponType;
    if(weapon)
        weaponType = weapon->getWeaponType();
    else
        weaponType = getUnarmedWeaponSkill();

    // we're very confused about what type of weapon this is
    if(weaponType.empty())
        weaponType = "bare-hand";

    // Go through races and add bonus for various weapon types:
    switch (getRace()) {
    case DWARF:
    case HILLDWARF:
    case DUERGAR:
        if (weaponType=="axe" || weaponType=="hammer")
            rBonus=10;
        break;
    case ELF:
        if (weaponType=="sword" || weaponType=="bow")
            rBonus=10;
        break;
    case GREYELF:
        if (weaponType=="arcane-weapon" || weaponType=="bow")
            rBonus=10;
        break;
    case WILDELF:
        if (weaponType=="sword" || weaponType=="bow" || weaponType=="spear")
            rBonus=10;
        break;
    case AQUATICELF:
        if (weaponType=="polearm")
            rBonus=10;
        break;
    case HALFLING:
        if (weaponType=="sling")
            rBonus=10;
        break;
    case ORC:
        if (weaponType=="axe" || weaponType=="great-axe")
            rBonus=10;
        break;
    case GNOME:
        if (weaponType=="staff")
            rBonus=10;
        break;
    case OGRE:
        if (weaponType=="club")
            rBonus=10;
        break;
    case DARKELF:
        if ( !isPureArcaneCaster() && !isPureDivineCaster() && 
            ((weaponType=="sword" || weaponType=="rapier") ||
            (weaponType=="crossbow" && weapon->flagIsSet(O_SMALL_BOW))))
            rBonus=10;
        break;
    case MINOTAUR:
        if (weaponType=="great-axe" || weaponType=="great-hammer")
            rBonus=10;
        break;
    case SERAPH:
        if (weaponType=="divine-weapon")
            rBonus=10;
        break;
    case KOBOLD:
        if (weaponType=="thrown" || weaponType=="crossbow")
            rBonus=10;
        break;
    case BARBARIAN:
        if (weaponType=="spear")
            rBonus=10;
        break;
    case HALFELF:
    case HUMAN:
    case HALFGIANT:
    case HALFFROSTGIANT:
    case HALFFIREGIANT:
    case TROLL:
    case HALFORC:
    case GOBLIN:
    case CAMBION:
    case KATARAN:
    case TIEFLING:
    case KENKU:
    default:
        break;
    }

    return(rBonus);

}

//**********************************************************************
//                      getWeaponSkill
//**********************************************************************

int Player::getWeaponSkill(const std::shared_ptr<Object>  weapon) const {
    int bonus = 0;

    // Bless improves your chance to hit
    if(isEffected("bless"))
        bonus += 10;

    if (weapon)
        bonus += (getClassWeaponskillBonus(weapon) + getRacialWeaponskillBonus(weapon));

    std::string weaponType;
    if(weapon)
        weaponType = weapon->getWeaponType();
    else
        weaponType = getUnarmedWeaponSkill();

    // we're very confused about what type of weapon this is
    if(weaponType.empty())
        weaponType = "bare-hand";


    Skill* weaponSkill = getSkill(weaponType);
    if(!weaponSkill)
        return(0);
    else
        return(weaponSkill->getGained() + bonus);
}

//**********************************************************************
//                      getDefenseSkill
//**********************************************************************

int Monster::getDefenseSkill() const {
    return(defenseSkill);
}

//**********************************************************************
//                      getDefenseSkill
//**********************************************************************

int Player::getDefenseSkill() const {
    int bonus = 0;
    // Protection makes you harder to hit
    if(isEffected("protection"))
        bonus += 10;
    Skill* defenseSkill = getSkill("defense");
    if(!defenseSkill)
        return(-1);
    else
        return(defenseSkill->getGained() + bonus);
}

double Creature::getMisschanceModifier(const std::shared_ptr<Creature>& victim, double& missChance) {
    double mod = 0.0;
    EffectInfo* effect=nullptr;
    auto cThis = Containable::downcasted_shared_from_this<Creature>();

    if (!victim)
        return(mod);

    if (isPlayer() && (isEffected("death-sickness") || isEffected("confusion")))
        mod += (missChance/10);

    if(victim->isEffected("blur") && !isEffected("true-sight"))
        mod += victim->getEffect("blur")->getStrength();

    if (victim->isEffected("faerie-fire"))
        mod -= victim->getEffect("faerie-fire")->getStrength();

    if(victim->getRoomParent()->isEffected("dense-fog")) {
        effect = victim->getRoomParent()->getEffect("dense-fog");
        if(!effect->isOwner(cThis))
            mod += effect->getStrength();
    }

     //Clerics of Linothan miss undead 10% less often
    if (getClass() == CreatureClass::CLERIC && getDeity() == LINOTHAN && victim->isUndead()) {
        mod -= (missChance/10);
    }

    // Check for various racial quirks based around attacks
    // Dwarves have specialized training for fighting giants. As such, they miss 10% less often
    if (getRace() == DWARF && victim->isMonster() && victim->getType() == GIANTKIN)
            mod -= (missChance/10.0); 

    //Giants have trouble hitting these races due to their specialized training in using their size to avoid being hit
    if (isMonster() && getType() == GIANTKIN && (victim->getRace() == DWARF || victim->getRace() == GOBLIN || 
                                                 victim->getRace() == KOBOLD || victim->getRace() == GNOME ||
                                                 victim->getRace() == HALFLING || victim->getRace() == HILLDWARF ||
                                                 victim->getRace() == DUERGAR) )
        mod -= (missChance/10.0);


    //TODO: Add others as thought up


    return(mod);
}

//**********************************************************************
//                      AttackResult
//**********************************************************************
// defense - skill
// 295 - 300 = -5 * .01% less chance to miss
// 320 - 300 = 20 * .01% more chance to miss

AttackResult Creature::getAttackResult(const std::shared_ptr<Creature>& victim, const std::shared_ptr<Object>&  weapon, int resultFlags, int altSkillLevel) {
    
    int mySkill=0;
    auto cThis = Containable::downcasted_shared_from_this<Creature>();

    if(altSkillLevel != -1) {
        // For when we want to look at an alternative skill, like kick
        mySkill = altSkillLevel;
    } else {
        mySkill = getWeaponSkill(weapon);
    }
    int defense = victim->getDefenseSkill();
    int difference = defense - mySkill;


    double missChance=0;
    double missModifier=0;
    double dodgeChance=0;
    double parryChance=0;
    double glancingChance=0;
    double blockChance=0;
    double criticalChance=0;
    double fumbleChance=0;
   

    missChance = victim->getMissChance(difference);

    missModifier = getMisschanceModifier(victim, missChance);
    missChance += missModifier;

    
    if(resultFlags & DOUBLE_MISS)
        missChance *= 2;

    int missCutoff = (int)(missChance * 100);

    if(resultFlags & NO_DODGE)
        dodgeChance = 0;
    else
        dodgeChance = victim->getDodgeChance(cThis, difference);
    int dodgeCutoff = (int)(missCutoff + (dodgeChance * 100));

    if(resultFlags & NO_PARRY)
        parryChance = 0;
    else
        parryChance = victim->getParryChance(cThis, difference);
    int parryCutoff = dodgeCutoff + (int)(parryChance * 100);

    if(resultFlags & NO_GLANCING)
        glancingChance = 0;
    else
        glancingChance = victim->getGlancingBlowChance(cThis, difference);
    int glancingCutoff = (int)(parryCutoff + (glancingChance * 100));

    if(resultFlags & NO_BLOCK)
        blockChance = 0;
    else
        blockChance = victim->getBlockChance(cThis, difference);
    int blockCutoff = (int)(glancingCutoff + (blockChance * 100));

    if((resultFlags & NO_CRITICAL) || victim->immuneCriticals())
        criticalChance = 0;
    else {
        criticalChance = getCriticalChance(difference);
        // Cleric of Mara has +10% to crit chance with bows at night
        if (weapon && weapon->getWeaponType()=="bow" && !isDay() && getClass()==CreatureClass::CLERIC && getDeity()==MARA)
            criticalChance += (criticalChance/10.0);
        // Cleric of Linothan always has +5% crit chance vs undead enemies
        if (getClass()==CreatureClass::CLERIC && getDeity()==LINOTHAN && victim->isUndead())
            criticalChance += (criticalChance/20.0);
    }

    int criticalCutoff = (int)(blockCutoff + (criticalChance * 100));

    if(resultFlags & NO_FUMBLE)
        fumbleChance = 0;
    else
        fumbleChance = getFumbleChance(weapon);
    int fumbleCutoff = (int)(criticalCutoff + (fumbleChance * 100));

    // Auto miss with an always crit weapon vs a no crit monster
    if(weapon && weapon->flagIsSet(O_ALWAYS_CRITICAL)) {
        if(victim->mFlagIsSet(M_NO_AUTO_CRIT) || victim->immuneCriticals())
            missCutoff = 10000;
        else {
            missCutoff = dodgeCutoff = parryCutoff = glancingCutoff = blockCutoff = -1;
            criticalCutoff = 10000;
        }

    }

    int randNum = Random::get(0, 10000);

    AttackResult result;

    if(randNum < missCutoff)
        result = (ATTACK_MISS);
    else if(randNum < dodgeCutoff)
        result = (ATTACK_DODGE);
    else if(randNum < parryCutoff)
        result = (ATTACK_PARRY);
    else if(randNum < glancingCutoff)
        result = (ATTACK_GLANCING);
    else if(randNum < blockCutoff)
        result = (ATTACK_BLOCK);
    else if(randNum < criticalCutoff)
        result = (ATTACK_CRITICAL);
    else if(randNum < fumbleCutoff)
        result = (ATTACK_FUMBLE);
    else
        result = (ATTACK_HIT);

    if(result == ATTACK_HIT || result == ATTACK_CRITICAL)
        if(victim->kamiraLuck(cThis))
            result = ATTACK_MISS;

    return(result);
}

//**********************************************************************
//                      adjustChance
//**********************************************************************

int Creature::adjustChance(const int &difference) const {
    double adjustment = 0;
    // defense - skill
    // 295 - 300 = -5 * .01% less chance to miss == negative difference, defense LOWER than weapon
    // 320 - 300 = 20 * .01% more chance to miss == positive difference, defense HIGHER than weapon

    // Now adjust it based on weapon skill & defense
    if(difference > 0) {
        // Defense is HIGHER than weapon skill
        if(isMonster()) {
            // Mobs get more out of higher defence than players
            if(difference > 10)
                adjustment = 0.4;
            else
                adjustment = 0.1;
        } else {
            adjustment = 0.1;
        }
    } else {
        // Rating < 0
        // Weapon skill is HIGHER than defense skill
        adjustment = 0.04;
    }
    return((int)(difference * adjustment));
}

//**********************************************************************
//                      getFumbleChance
//**********************************************************************

double Creature::getFumbleChance(const std::shared_ptr<Object>&  weapon) const {
    double chance = 2.0;

    if(!weapon || weapon->flagIsSet(O_CURSED) || isDm()) {
        chance = 0.0;
    }
    else {
        double weaponSkill = getSkillLevel(weapon->getWeaponType());
        // Spread the reduction over all 300 skill points, leaving a .01 chance to fumble
        // even at 300 skill
        chance -= (weaponSkill / 151.0);

        chance = std::max(0.0, chance);
    }
    return(chance);
}

//**********************************************************************
//                      getCriticalChance
//**********************************************************************

double Creature::getCriticalChance(const int& difference) const {
    double chance = 5.0;

    chance -= adjustChance(difference);
    chance = std::max(0.0, chance);
    return(chance);
}

//**********************************************************************
//                      getBlockChance
//**********************************************************************

double Creature::getBlockChance(std::shared_ptr<Creature> attacker, const int& difference) {
    if(isPlayer()) {
        if(!knowsSkill("block"))
            return(0);

        // Players need a shield to block
        if(!ready[SHIELD-1])
            return(0);
    }
    double chance = 5.0;
    chance += adjustChance(difference);
    // Max chance of 5.0% if a monster
    if(isMonster())
        chance = std::min(5.0, chance);

    chance = std::max(0.0, chance);
    return(chance);
}

//**********************************************************************
//                      getGlancingBlowChance
//**********************************************************************
// A glancing blow is the opposite of a critical hit; reduced damage
// when a player or pet is attacking a monster but only against
// monsters of the same level or higher

double Creature::getGlancingBlowChance(const std::shared_ptr<Creature>& attacker, const int& difference) const {
    if(isPlayer() || isPet() || (attacker->isMonster() && !attacker->isPet()))
        return(0);

    if(level < attacker->getLevel())
        return(0);

    double chance = 10.0 + (difference * 0.5);
    chance = std::max(0.0, chance);
    return(chance);
}

//**********************************************************************
//                      getParryChance
//**********************************************************************
// Chance that they will parry or riposte an attack.
// 50% of parry will be a riposte

double Creature::getParryChance(const std::shared_ptr<Creature>& attacker, const int& difference) {
    if(isPlayer()) {
        if(!canDodge(attacker) || !canParry(attacker))
            return(0);

        if(!knowsSkill("parry"))
            return(0);

    }
   
    double chance = 0.0;
    if (getClass() == CreatureClass::CLERIC && getDeity() == LINOTHAN)
        chance = (std::max<int>(piety.getCur(), 80) - 80) * .03;
    else
        chance = (std::max<int>(dexterity.getCur(), 80) - 80) * .03;

    chance += adjustChance(difference);

    // Riposte is harder against some attacker types
    if( attacker->type == DRAGON || attacker->type == DEMON || attacker->type == DEVIL ||
        attacker->type == GIANTKIN || attacker->type == DEVA || attacker->type == DINOSAUR ||
        attacker->type == ELEMENTAL || attacker->type == MODRON || attacker->type == DAEMON
    )
        chance /= 2;

    int numEnm = 0;
    for(const auto& mons : getRoomParent()->monsters) {
        if(mons->isPet())
            continue;
        if(mons->isEnemy(Containable::downcasted_shared_from_this<Creature>()))
            if(numEnm++ > 1)
                chance -= .02;
    }
    chance = std::max(0.0, chance);
    return(chance);
}

//**********************************************************************
//                      canParry
//**********************************************************************

bool Creature::canParry(const std::shared_ptr<Creature>& attacker) {
    if(attacker->isDm())
        return(false);

    if(isMagicallyHeld(false))
        return(false);

    if(isMonster())
        return(true);

    if(!ready[WIELD - 1])
        return(false);          // must have a weapon wielded in order to parry.

    std::string weaponCategory = getPrimaryWeaponCategory();
    std::string weaponType = ready[WIELD - 1]->getWeaponType();
    if(weaponCategory == "ranged")
        return(false);          // Can't parry with a ranged weapon

    int combatPercent=0;
//   If overwhelmed by mass amounts of enemies, it is difficult if
//   not impossible to find time to pause and parry an attack. The following
//   code will calculate the chance of failure into a "combat percent". If a check
//   against this percent fails, then there was not enough time to pause to
//   try a parry, so parry returns 0 without going off.

    // +3% fail for every monster mad at this besides the one we are currently fighting
    combatPercent = 3*(std::max<int>(0, numEnemyMonInRoom()-1));
    // Group members are assumed to fight together to help one another.
    // -2% fail for every member in the this's group besides themself in the same room
    if(getGroup())
        combatPercent -= 2*(std::max(0,getGroup()->getNumInSameRoom(Containable::downcasted_shared_from_this<Creature>())));

    combatPercent = std::max(0,combatPercent);

    if(Random::get(1,100) <= combatPercent)
        return(false); // Did not find a moment to parry due to excessive combat. No parry.

    // Parry is not possible against some types of creatures
    if( attacker->type == INSECT || attacker->type == AVIAN ||
        attacker->type == FISH || attacker->type == REPTILE || attacker->type == PLANT ||
        attacker->type == ETHEREAL || attacker->type == ASTRAL ||
        attacker->type == GASEOUS || attacker->type == ENERGY ||
        attacker->type == PUDDING || attacker->type == SLIME || attacker->type == OOZE
    )
        return(false);

    long t = time(nullptr);
    long i = LT(this, LT_RIPOSTE);

    if(t < i) {
        return(false);
    }
    if(ready[WIELD-1]->getShotsCur() < 1) // weapon must not be broken
        return(false);

    return(true);
}

//**********************************************************************
//                      canDodge
//**********************************************************************

bool Creature::canDodge(const std::shared_ptr<Creature>& attacker) {
    if(attacker->isDm())
        return(false);

    if(isMagicallyHeld(false))
        return(false);

    if(attacker->isMonster()) {
        if(attacker->flagIsSet(M_UN_DODGEABLE))
            return(false);
        if(attacker->flagIsSet(M_UNKILLABLE) && !isCt())
            return(false);
    }
    if(isPlayer()) {
        if(flagIsSet(P_UNCONSCIOUS)) // Can't dodge while unconscious
            return(false);

        if(getRoomParent()->isUnderwater() && !isEffected("free-action"))
            return(false);

        if(getRoomParent()->flagIsSet(R_NO_DODGE))
            return(false);

        // Players cannot dodge if stunned in any way.
        // Circle, bash, maul, bite, and stun will all
        // prevent dodge from functioning.
        if(flagIsSet(P_STUNNED))
            return(false);

        // Can't dodge what you cannot see
        if(!canSee(attacker))
            return(false);

        // Players waiting and hiding will not dodge so they won't auto unhide themselves.
        if(flagIsSet(P_HIDDEN))
            return(false);

    }
    return(true);
}

//**********************************************************************
//                      getDodgeChance
//**********************************************************************
// What chance do we have to dodge the attacker?

double Creature::getDodgeChance(const std::shared_ptr<Creature>& attacker, const int& difference) {
    if(!canDodge(attacker))
        return(false);

    double chance = 0.0;
    std::shared_ptr<Player> pPlayer = getAsPlayer();

    // Find the base chance
    if(pPlayer) {
        switch(cClass) {
            case CreatureClass::RANGER:
                chance += (dexterity.getCur() * .06);
                break;
            case CreatureClass::THIEF:
                chance += (dexterity.getCur() * .075);
                break;
            case CreatureClass::ASSASSIN:
                chance += (dexterity.getCur() * .06);
                break;
            case CreatureClass::ROGUE:
                chance += (dexterity.getCur() * .08);
                break;
            case CreatureClass::FIGHTER:
            case CreatureClass::BERSERKER:
                if(pPlayer->getSecondClass() == CreatureClass::THIEF || pPlayer->getSecondClass() == CreatureClass::ASSASSIN)
                    chance += (dexterity.getCur() * .055);
                else
                    chance += (1.0 + (dexterity.getCur() * .045));
                break;
            case CreatureClass::BARD:
            case CreatureClass::PALADIN:
            case CreatureClass::DEATHKNIGHT:
            case CreatureClass::WEREWOLF:
            case CreatureClass::PUREBLOOD:
                chance += (2.0 + (dexterity.getCur() * .05));
                break;
            case CreatureClass::MONK:
                chance += (2.0 + (dexterity.getCur() * .06));
                break;
            case CreatureClass::CLERIC:
                if(pPlayer->getSecondClass() == CreatureClass::ASSASSIN) {
                    chance += (dexterity.getCur() * .055);
                    break;
                }
                switch(deity) {
                    case KAMIRA:
                    case ARACHNUS:
                    case LINOTHAN:
                        chance += (piety.getCur() * .07);
                        break;
                    case CERIS:
                    case ARES:
                    case ENOCH:
                    case GRADIUS:
                    case MARA:
                    case JAKAR:
                    default:
                        chance += (2.0 + dexterity.getCur() * .05);
                        break;
                }
                break;
            case CreatureClass::LICH:
            case CreatureClass::MAGE:
                if(pPlayer->getSecondClass() == CreatureClass::THIEF || pPlayer->getSecondClass() == CreatureClass::ASSASSIN)
                    chance += (dexterity.getCur() * .07);
                else
                    chance += (1.0 + dexterity.getCur() * .06);
                break;
            default:
                break;
        }
    } else {
        // Not a player
        chance = 5.0;
    }

    // TODO: Adjust dodge based on armor weight


    chance += adjustChance(difference);
    chance = std::max(0.0, chance);
    return(chance);
}

//**********************************************************************
//                      getMissChance
//**********************************************************************
//  What chance do we have to be missed?

double Creature::getMissChance(const int& difference) {
    // Base chance of 5% to miss
    double chance;

    if(difference > 10)
        chance = 7.0;
    else
        chance = 5.0;

    chance += adjustChance(difference);
    std::shared_ptr<Player> pPlayer = getAsPlayer();
    // Class modification for miss
    if(pPlayer) {
        switch(cClass) {
            case CreatureClass::RANGER:
            case CreatureClass::THIEF:
            case CreatureClass::ASSASSIN:
            case CreatureClass::ROGUE:
                chance *= 1.0;
                break;
            case CreatureClass::FIGHTER:
            case CreatureClass::BERSERKER:
                chance *= 0.7;
                break;
            case CreatureClass::MONK:
            case CreatureClass::PALADIN:
            case CreatureClass::DEATHKNIGHT:
            case CreatureClass::WEREWOLF:
                chance *= 0.8;
                break;
            case CreatureClass::PUREBLOOD:
            case CreatureClass::BARD:
                chance *= 0.9;
                break;
            case CreatureClass::CLERIC:
                switch(deity) {
                    case ARES:
                    case LINOTHAN:
                    case ARACHNUS:
                        chance *= 0.8;
                        break;
                    case KAMIRA:
                    case CERIS:
                    case MARA:
                    case JAKAR:
                    case ENOCH:
                    case GRADIUS:
                        chance *= 1.0;
                    default:
                        chance *= 1.0;
                        break;
                }
                break;
            case CreatureClass::LICH:
            case CreatureClass::MAGE:
                    chance *= 1.0;
                break;
            default:
                break;
        }
    } else {
        // Not a player
        chance *= 1.0;
    }

    chance = std::max(0.0, chance);
    return(chance);
}

//**********************************************************************
//                      setWeaponSkill
//**********************************************************************

void Monster::setWeaponSkill(int amt) {
    weaponSkill = amt;
}

//**********************************************************************
//                      setDefenseSkill
//**********************************************************************

void Monster::setDefenseSkill(int amt) {
    defenseSkill = amt;
}

//**********************************************************************
//                      canHit
//**********************************************************************

bool Creature::canHit(const std::shared_ptr<Creature>& victim, std::shared_ptr<Object>  weapon, bool glow, bool showFail) {
    //std::shared_ptr<Monster>  mVictim = victim->getMonster();
    std::shared_ptr<Player> pVictim = victim->getAsPlayer();

    if(isMonster()) {

    } else {
        // Player attacking something
        bool silver = false;
        bool focused = false;
        int enchant = 0;

        if(cClass == CreatureClass::MONK && !weapon && flagIsSet(P_FOCUSED))
            focused = true;

        if(cClass == CreatureClass::MONK && !weapon && !ready[HELD-1] && ready[HANDS-1]) {
            enchant = abs(ready[HANDS-1]->getAdjustment());
            weapon = ready[HANDS-1];
        } else if(weapon) {
            enchant = abs(weapon->getAdjustment());
            if(weapon->isSilver())
                silver = true;
        }

        if(pVictim) {
        // It's a player

            // DMs can hit vampires
            if(!isDm()) {
                if(pVictim->isEffected("mist")) {
                    if(weapon) {
                        if( !weapon->flagIsSet(O_CAN_HIT_MIST) && !flagIsSet(P_MISTBANE) ) {
                            if(showFail) *this << ColorOn << "You cannot hit a misted creature with your " << weapon << ".\n" << ColorOff;
                            return(false);
                        }
                    } else if(!flagIsSet(P_MISTBANE)) {
                        if(showFail) *this << "You cannot physically hit a misted creature.\n";
                        return(false);
                    }
                }
            }

            if(!isDm() &&
                (// pVictim is a werewolf and higher than 10 and no silver
                (pVictim->isEffected("lycanthropy") && pVictim->getLevel() >=10 && !silver) ||
                // or pVictim is a lich and higher than 7
                (pVictim->getClass() == CreatureClass::LICH && pVictim->getLevel() >=7 ) ||
                // or they have enchant only flag set
                pVictim->flagIsSet(P_ENCHANT_ONLY)
            )
                && // and the attacker is
                // Not a werewolf
                !isEffected("lycanthropy") &&
                // and no weapon and not focused
                (  (!weapon && !focused) ||
                    // or weapon and has no adjustment
                    (weapon && enchant < 1)
                )
            ) {
                if(showFail) {
                    *this << ColorOn << "Your " << (weapon ? weapon->getName() : "attack") << " " << (!weapon || weapon->getType() == ObjectType::WEAPON ? "has" : "have") << " no effect on " << pVictim << "\n" << ColorOff;
                    pVictim->print("%M's attack on you was uneffective.\n", this);
                }
                return(false);
            }
        } else {
            // Player vs Monster
            // Check if we can hit the monster
            if( victim->flagIsSet(M_ENCHANTED_WEAPONS_ONLY) ||
                victim->flagIsSet(M_PLUS_TWO) ||
                victim->flagIsSet(M_PLUS_THREE)
            ) {
                // At night level 19+ wolves can hit
                if( isEffected("lycanthropy") &&
                    !isDay() &&
                    level > 19 &&
                    victim->flagIsSet(M_ENCHANTED_WEAPONS_ONLY)
                ) {
                    if(glow) printColor("^WYour claws glow radiantly in the night against %N.\n", victim.get());
                }
                else if(cClass == CreatureClass::MONK &&
                    flagIsSet(P_FOCUSED) &&
                    ( (level >= 16 && victim->flagIsSet(M_ENCHANTED_WEAPONS_ONLY)) ||
                            (level >= 16 && victim->flagIsSet(M_PLUS_TWO))
                    )
                ) {
                    if(glow) *this << ColorOn << "^WYour fists glow with power against " << victim << ".\n" << ColorOff;
                }
                else if(weapon && (
                        (victim->flagIsSet(M_ENCHANTED_WEAPONS_ONLY) && enchant > 0) ||
                        (victim->flagIsSet(M_PLUS_TWO) && enchant > 1) ||
                        (victim->flagIsSet(M_PLUS_THREE) && enchant > 2)
                    )
                ) {
                    if(glow && weapon) {
                        *this << ColorOn << "^WYour " << weapon->getName() << "^W glow" << (weapon->getType() == ObjectType::WEAPON ? "s" : "") << " with power against " << victim << ".\n" << ColorOff;
                    }
                } else if(isDm()) {
                    if(glow)
                        *this << ColorOn << "^WYou ignore the laws of the land and attack " << victim << " anyway.\n" << ColorOff;
                } else {
                    if(showFail) {
                        *this << ColorOn << "Your " << (weapon ? weapon->getName() : "attack") << " " << (!weapon || weapon->getType() == ObjectType::WEAPON ? "has" : "have") << " no effect on " << victim << "\n" << ColorOff;
                    }
                    return(false);
                }
            }
        }

    }
    return(true);
}

//*********************************************************************
//                      applyMagicalArmor
//*********************************************************************
// This function will check for any magical armor effects and modify the
// damage if warranted. 

void Creature::applyMagicalArmor(Damage& dmg, int dmgType) {
    int vHp=0;
    EffectInfo* checkEffect=nullptr;

    //No magical armor effects checked for monsters...yet
    if (!isPlayer())
        return;

    if (dmg.get() <= 0)
        return;

    if (dmgType == DMG_NONE)
        return;

    if (isEffected("stoneskin") && dmgType == PHYSICAL_DMG) {
        checkEffect = getEffect("stoneskin");
        vHp = checkEffect->getStrength();
        vHp = std::max(0,vHp);
        vHp--;
        if (vHp <= 0) {
            *this << ColorOn << "^y^#Your stoneskin spell has been depleted.\n" << ColorOff;
            broadcast(getSock(), getRoomParent(), "%M's stoneskin spell has been depleted.", this);
            removeEffect("stoneskin");
        }
        else
        {
            checkEffect->setStrength(vHp);
            dmg.set(dmg.get()/2); // stoneskin halves physical damage
            return; // stoneskin will keep any follow on magical armor checks from happening, protecting those effects
        }
    }

    if (isEffected("armor") && dmgType == PHYSICAL_DMG) {
        checkEffect = getEffect("armor");       
        vHp = checkEffect->getStrength();
        vHp = std::max(0,vHp);

        vHp -= dmg.get();

        if (vHp <= 0) {
            *this << ColorOn << "^y^#Your armor spell has been depleted.\n" << ColorOff;
            broadcast(getSock(), getRoomParent(), "%M's armor spell has been depleted.", this);
            removeEffect("armor");
            getAsPlayer()->computeAC();        
        }
        else {
            checkEffect->setStrength(vHp);
        }
    }

    return;

    //TODO: Put entries here for any future magical armor effects

}

//********************************************************************************
//                      computeDamage
//********************************************************************************
// Returns 0 on normal computation, 1 if the weapon was shattered

int Player::computeDamage(std::shared_ptr<Creature> victim, std::shared_ptr<Object>  weapon, AttackType attackType, AttackResult& result, Damage& attackDamage, bool computeBonus, int &drain, float multiplier) {
    int retVal = 0;
    Damage bonusDamage;
    std::string weaponCategory = weapon ? weapon->getWeaponCategory() : "none";
    drain = 0;

    if(attackType == ATTACK_KICK) {
        if(computeBonus)
            bonusDamage.set(getBaseDamage()/2);
        attackDamage.set(Random::get(2,6) + ::bonus(strength.getCur()));
        if((cClass == CreatureClass::FIGHTER || cClass == CreatureClass::MONK) && !hasSecondClass()) {
            attackDamage.add((int)(getSkillLevel("kick") / 4));
        }
    } else if(attackType == ATTACK_GORE) {
        if(computeBonus)
            bonusDamage.set(getBaseDamage()/2);
        attackDamage.set(Random::get(4,9));
        if(getAsCreature()->isMartial())
            attackDamage.add((strength.getCur() / 15) + (int)(getSkillLevel("gore") / 4));
        else
            attackDamage.add((int)(getSkillLevel("gore") / 5));

    } else if(attackType == ATTACK_MAUL) {
        if(computeBonus)
            bonusDamage.set(getBaseDamage()/2);
        attackDamage.set((Random::get(level / 2, level + 1) + (strength.getCur() / 10)));
        attackDamage.add(Random::get(2, 4));
    } 
    else {
        // Any non kick attack for now
        if(computeBonus)
            bonusDamage.set(getBaseDamage());

         if(!weapon) {
            if(cClass == CreatureClass::MONK) {
                attackDamage.set(Random::get(1,2) + level/3 + Random::get((1+level/4),(1+level)/2));
                if(strength.getCur() < 90) {
                    attackDamage.set(attackDamage.get() - (90-strength.getCur())/10);
                    attackDamage.set(std::max<int>(1, attackDamage.get()));
                }
            } else {
                attackDamage.set(damage.roll());
                bonusDamage.set(bonusDamage.get() * 3 / 4);
            }
        }
        // TODO: Add in modifier based on weapon skill
    }

    if(weapon) {
        attackDamage.add(weapon->damage.roll() + weapon->getAdjustment());
        // Linothan clerics do +20% damage with great-swords/two-handed swords
        if (cClass == CreatureClass::CLERIC && getDeity() == LINOTHAN && (weapon->getWeaponType() == "great-sword" || (weapon->getWeaponType() == "sword" && weapon->flagIsSet(O_TWO_HANDED))))
            attackDamage.set(attackDamage.get() + (attackDamage.get())/5);
        // At night, clercis of Mara do +25% damage with bows
        if (cClass == CreatureClass::CLERIC && getDeity() == MARA && !isDay() && weapon->getWeaponType() == "bow")
            attackDamage.set(attackDamage.get() + (attackDamage.get())/4);
    }

    if(isEffected("lycanthropy"))
        attackDamage.add(packBonus());

    // Mara and Linothan clerics do +5% damage to targets they hate, so long as their alignment is in order and the target is not good aligned (except undead can be good or evil)
    if((cClass == CreatureClass::CLERIC && (getDeity() == LINOTHAN || getDeity() == MARA)) && alignInOrder() && hatesEnemy(victim) && (victim->isEvil() || victim->isUndead())) {
        attackDamage.set(attackDamage.get() + (attackDamage.get())/20);
        *this << ColorOn << "^r" << gConfig->getDeity(getDeity())->getName() << "'s vicious hatred of " << victim << " increases your damage.\n" << ColorOff;
    }

    attackDamage.set(std::max<int>(1, attackDamage.get()));

    if((cClass == CreatureClass::PALADIN && getAlignment() <= REDDISH) ||
                (cClass == CreatureClass::DEATHKNIGHT && getAlignment() >= BLUISH)) {
        *this << "Your " << (cClass==CreatureClass::DEATHKNIGHT?"goodness":"evilness") << " reduces your damage.\n";
        multiplier /= 2;
    }

    // Bonus spread out over entire series of attack for multi attack weapons
    if(computeBonus) {
        if(cClass == CreatureClass::PALADIN && getAdjustedAlignment() >= BLUISH && victim->getAdjustedAlignment() <= REDDISH) {
            int goodDmg = Random::get(1, 1 + level / 3);
            *this << ColorOn << "^WYour purity of heart increased your damage by ^y" << goodDmg << "^W.\n" << ColorOff;
            bonusDamage.add(goodDmg);
        }
        if(cClass == CreatureClass::DEATHKNIGHT) {
            // Only drain on 1st attack if a multi weapon
            if(getAdjustedAlignment() <= REDDISH && victim->getAdjustedAlignment() >= LIGHTBLUE)
                drain = Random::get(1, 1 + level / 3);
        }
        if( (   cClass == CreatureClass::BERSERKER ||
                (cClass == CreatureClass::CLERIC && deity == ARES) ||
                isStaff()
            ) &&
            isEffected("berserk"))
        {
            // Only do a bonus if not a multi weapon, or is a multi weapon and it's the first attack
            if(weapon && weaponCategory != "ranged")
                bonusDamage.add(attackDamage.get() / 2);
        }

        if( (   (cClass == CreatureClass::DEATHKNIGHT && getAdjustedAlignment() <= REDDISH) ||
                (cClass == CreatureClass::PALADIN && getAdjustedAlignment() >= BLUISH)
            ) &&
            (isEffected("pray") || isEffected("dkpray"))
        )
            bonusDamage.add(Random::get(1,3));

        if((isEffected("lycanthropy") || isCt()) && isEffected("frenzy"))
            bonusDamage.add(Random::get(3,5));

        if((cClass == CreatureClass::MONK || isCt()) && flagIsSet(P_FOCUSED))
            bonusDamage.add(Random::get(1,3));
    }



    // If this is a monk, and we're not wielding a monk weapon
    // Or this is a werewolf, and we're not wielding a werewolf weapon or a claw weapon****
    //  **** Although I'm pretty sure werewolves can only use claw weapons now...but put here
    //      in case this changes in the future
    if(attackType != ATTACK_KICK && (cClass == CreatureClass::MONK || isEffected("lycanthropy"))) {
        if( weapon && (
                (cClass == CreatureClass::MONK && !weapon->flagIsSet(O_SEL_MONK)) ||
                (isEffected("lycanthropy") && (
                    !weapon->flagIsSet(O_SEL_WEREWOLF) ||
                    weapon->getWeaponType() != "claw")
                )
            )
        ) {
            if(cClass == CreatureClass::MONK)
                print("How can you attack well with your hands full?\n");
            else
                print("How can you attack well with your paws full?\n");
            multiplier /= 2;
        }
    }

    if(multiplier > 0.0) {
        attackDamage.set((int) ((float)attackDamage.get() * multiplier));
        if(computeBonus) {
            if(attackType != ATTACK_BACKSTAB)
                bonusDamage.set((int)((float)bonusDamage.get() * multiplier));
//          else
//              bonus = static_cast<int>(static_cast<float>(bonus) * (multiplier/2.0));
        }
    }


    if(result == ATTACK_CRITICAL) {
        int mult=0;
        char atk[25];
        switch(attackType) {
            case ATTACK_BASH:
                strcpy(atk, "bash");
                break;
            case ATTACK_KICK:
                strcpy(atk, "kick");
                break;
            case ATTACK_GORE:
                strcpy(atk, "gore");
                break;
            default:
                strcpy(atk, "hit");
                break;
        }

        printColor("^gCRITICAL %s!\n", atk);
        broadcast(getSock(), getRoomParent(), "^g%M made a critical %s.", this, atk);
        mult = Random::get(3, 5);
        attackDamage.set(attackDamage.get() * mult);
        drain *= mult;
        // We'll go with the assumption that if you critical on the first hit, the remaining
        // ones in that series will hurt a little more, so this will work just fine
        // if the first of a series of hits is a critical
        if(computeBonus)
            bonusDamage.set(bonusDamage.get() * mult);
        broadcastGroup(false, victim, "^g%M made a critical %s!\n", this, atk);
        if( attackType != ATTACK_KICK && attackType != ATTACK_GORE && weapon && !isDm() && weapon->flagIsSet(O_ALWAYS_CRITICAL) && !weapon->flagIsSet(O_NEVER_SHATTER)) {
            printColor("^YYour %s shatters.\n", weapon->getCName());
            broadcast(getSock(), getRoomParent(),"^Y%s %s shattered.", upHisHer(), weapon->getCName());
            retVal = 1;
        }
    } else if(result == ATTACK_GLANCING) {
        printColor("^CYou only managed to score a glancing blow!\n");
        broadcast(getSock(), getRoomParent(), "^C%M scored a glancing blow!", this);
        if(Random::get(1,2)) {
            attackDamage.set(attackDamage.get() / 2);
            drain /= 2;
            if(computeBonus)
                bonusDamage.set(bonusDamage.get() / 2);
        } else {
            attackDamage.set(attackDamage.get() * 2 / 3);
            drain = (drain * 2) / 3;
            if(computeBonus)
                bonusDamage.set(bonusDamage.get() * 2 / 3);
        }

    } else if(result == ATTACK_BLOCK) {
        attackDamage.set(victim->computeBlock(attackDamage.get()));
        if(computeBonus)
            bonusDamage.set(victim->computeBlock(bonusDamage.get()));
    }

    victim->modifyDamage(Containable::downcasted_shared_from_this<Player>(), PHYSICAL, attackDamage, NO_REALM, weapon, 0, computeBonus ? OFFGUARD_NOREMOVE : OFFGUARD_REMOVE);

    

    // Don't forget to modify the bonus
    if(computeBonus) {
        // Make bonus proportional to weapon speed, otherwise it's too much with fast weapons, and
        // too little with slow weapons.  (Bonus / 3) * weapon speed
        bonusDamage.set((int)(((float)bonusDamage.get()/(float)DEFAULT_WEAPON_DELAY) * (weapon ? weapon->getWeaponDelay() : DEFAULT_WEAPON_DELAY)));

        victim->modifyDamage(Containable::downcasted_shared_from_this<Player>(), PHYSICAL, bonusDamage, NO_REALM, weapon, 0, OFFGUARD_NOPRINT, true);
        attackDamage.setBonus(bonusDamage);
    }

    

    if(retVal != 1) {
        // If we didn't shatter, minimum of 1 damage
        attackDamage.set(std::max<int>(attackDamage.get(), 1));

    }
    
    
    

    return(retVal);
}

//**********************************************************************
//                      computeDamage
//**********************************************************************

int Monster::computeDamage(std::shared_ptr<Creature> victim, std::shared_ptr<Object>  weapon, AttackType attackType, AttackResult& result, Damage& attackDamage, bool computeBonus, int &drain, float multiplier) {
    Damage bonusDamage;

    if(computeBonus)
        bonusDamage.set(getBaseDamage());

    if(ready[WIELD-1])
        attackDamage.add(ready[WIELD-1]->damage.roll() + ready[WIELD-1]->getAdjustment());
    else
        attackDamage.add(damage.roll());

    attackDamage.add(::bonus(strength.getCur()));

    if(result == ATTACK_CRITICAL) {
        broadcast((std::shared_ptr<Socket>)nullptr, getRoomParent(), "%M made a critical hit.", this);
        int mult = Random::get(2, 5);
        attackDamage.set(attackDamage.get() * mult);
        drain *= mult;
        // No shatter for mobs :)
    }
    else if(result == ATTACK_BLOCK) {
        attackDamage.set(Creature::computeBlock(attackDamage.get()));
        if(computeBonus) {
            bonusDamage.set(Creature::computeBlock(bonusDamage.get()));
        }
    }

    victim->modifyDamage(Containable::downcasted_shared_from_this<Monster>(), PHYSICAL, attackDamage);
    victim->modifyDamage(Containable::downcasted_shared_from_this<Monster>(), PHYSICAL, bonusDamage);
    attackDamage.setBonus(bonusDamage);

    

    attackDamage.set(std::max<int>(1, attackDamage.get()));
    return(0);
}

//**********************************************************************
//                      computeBlock
//**********************************************************************

int Creature::computeBlock(int dmg) {
    // TODO: Tweak amount of blockage based on the shield
    // for now, just half the damage
    dmg /= 2;
    return(dmg);
}

//**********************************************************************
//                      dodge
//**********************************************************************
// This function prints the messages for dodging, chance is now
// computed elsewhere

int Creature::dodge(const std::shared_ptr<Creature>& target) {
    int     i = Random::get(1,10);

    unhide();
    smashInvis();

    if(isPlayer()) {
        std::shared_ptr<Player> player = getAsPlayer();
        player->statistics.dodge();
        player->increaseFocus(FOCUS_DODGE);
    }

    if(target->isPlayer())
        target->getAsPlayer()->statistics.miss();

    if(i == 1) {
        printColor("^cYou barely manage to dodge %N's attack.\n", target.get());
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M barely dodges %N's attack.", this, target.get());
        if(target->isPlayer())
            target->printColor("^c%M somehow manages to dodge your attack.\n", this);
    } else if(i == 2) {
        printColor("^cYou deftly dodge %N's attack.\n", target.get());
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M deftly dodges %N's attack.", this, target.get());
        if(target->isPlayer())
            target->printColor("^c%M deftly dodges your attack.\n", this);
    } else if(i == 3 && (target->isPlayer())) {
        printColor("^cYou side-step %N's attack and smack %s on the back of the head.\n", target.get(),
            target->himHer());
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M side-steps %N's attack and smacks %s on the back of the head.", this,
            target.get(), target->himHer());
        if(target->isPlayer())
            target->printColor("^c%M side-steps your attack and smacks you on the back of the head.\n", this);
    } else if(i > 3 && i < 6) {
        printColor("^cYou dance gracefully around %N's attack.\n", target.get());
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M dances gracefully around %N's attack.", this, target.get());
        if(target->isPlayer())
            target->printColor("^c%M dances gracefully around your attack.\n", this);
    } else if(i >= 6 && i < 8) {
        printColor("^cYou easily dodge %N's attack.\n", target.get());
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M easily dodges %N's attack.", this, target.get());
        if(target->isPlayer())
            target->printColor("^c%M easily dodges your attack.\n",this);
    }
    else if(i >= 8 && i < 9) {
        printColor("^cYou easily duck under %N's pitifully executed attack.\n", target.get());
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M easily ducks under %N's pitifully executed attack.", this, target.get());
        if(target->isPlayer())
            target->printColor("^c%M easily ducks under your pitifully executed attack.\n",this);
    } else if(i >= 9 && i <= 10) {
        printColor("^cYou laugh at %N as you easily dodge %s attack.\n",
            target.get(), target->hisHer());
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M laughs as %s easily dodges %N's attack.", this, heShe(), target.get());
        if(target->isPlayer())
            target->printColor("^c%M laughs as %s dodges your attack.\n", this, heShe());
    } else {
        printColor("^cYou deftly dodge %N's attack.\n", target.get());
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M deftly dodges %N's attack.", this, target.get());
        if(target->isPlayer())
            target->printColor("^c%M deftly dodges your attack.\n",this);
    }


    return(1);
}

//**********************************************************************
//                      canRiposte
//**********************************************************************

bool Creature::canRiposte() const {
    if(isPlayer())
        return(true);
    if(flagIsSet(M_CAN_RIPOSTE))
        return(true);
    return(level >= 15 && (
        cClass == CreatureClass::FIGHTER ||
        cClass == CreatureClass::BERSERKER ||
        cClass == CreatureClass::ASSASSIN ||
        cClass == CreatureClass::THIEF ||
        cClass == CreatureClass::ROGUE
    ));
}

//*********************************************************************
//                      parry
//*********************************************************************

int Creature::parry(const std::shared_ptr<Creature>& target) {
    long    t=0;
    std::shared_ptr<Object>  weapon = ready[WIELD-1];
    Damage attackDamage;

    if(!weapon && isPlayer()) {
        broadcast(::isDm, "*** Parry error: called with null weapon for %s.\n", getCName());
        return(0);
    }

    t = time(nullptr);

    if(isPlayer()) {
        t=time(nullptr);
        lasttime[LT_RIPOSTE].ltime = t;

        switch(cClass) {
            case CreatureClass::THIEF:
            case CreatureClass::ASSASSIN:
            case CreatureClass::FIGHTER:
                lasttime[LT_RIPOSTE].interval = 9L;
                break;

            default:
                lasttime[LT_RIPOSTE].interval = 6L;
                break;
        }
    }
    if(isMonster()) {
        unhide();
        smashInvis();
    }

    // If we get a hit, it's a riposte, otherwise just a parry
    AttackResult result;


    // If we're a player, or a monster that's over 15 and is a certain class,

    // we have a chance to riposte, otherwise only do a parry
    if(canRiposte())
        result = getAttackResult(target, weapon, DOUBLE_MISS|NO_DODGE|NO_PARRY|NO_BLOCK|NO_CRITICAL|NO_FUMBLE|NO_GLANCING);
    else
        result = ATTACK_MISS; // Parry


    //checkImprove("parry", true);

    // if result == miss || can't hit target, parry
    if(result == ATTACK_MISS || !canHit(target, weapon, true, false)) {
        printColor("^cYou parry %N's attack.\n", target.get());
        broadcast(getSock(), target->getSock(), getRoomParent(), "%M parries %N's attack.", this, target.get());
        if(target->isPlayer())
            target->printColor("^c%M parries your attack.\n", this);
        if(isPlayer()) {
            getAsPlayer()->increaseFocus(FOCUS_PARRY);
        }
    } else {
        // We have a riposte, calculate damage and such
        std::string verb = getWeaponVerb();
        std::string verbPlural = getWeaponVerbPlural();
        int drain=0;
        bool wasKilled = false, freeTarget = false, meKilled = false;
        //int enchant = 0;
        //if(weapon)
        //  enchant = abs(weapon->adjustment);

        computeDamage(target, weapon, ATTACK_NORMAL, result, attackDamage, true, drain);
        attackDamage.add(attackDamage.getBonus());

        // So mob riposte isn't soo mean
        if(isMonster()) {
            attackDamage.set(attackDamage.get() / 2);
            attackDamage.set(std::max<int>(1, attackDamage.get()));
        }

        switch(Random::get(1,7)) {
        case 1:
            printColor("^cYou side-step ^M%N's^c attack and %s %s for %s%d^c damage.\n",
                    target.get(), verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M side-steps %N's attack and %s %s.",
                this, target.get(), verbPlural.c_str(), target->himHer());
            if(target->isPlayer())
                target->printColor("^M%M^x side-steps your attack and %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 2:
            printColor("^cYou lunge and viciously %s ^M%N^c for %s%d^c damage.\n",
                verb.c_str(), target.get(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M lunges and viciously %s %N.",
                this, verbPlural.c_str(), target.get() );
            if(target->isPlayer())
                target->printColor("^M%M^x lunges at you and viciously %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 3:
            printColor("^cYou slide around ^M%N's^c attack and %s %s for %s%d^c damage.\n",
                target.get(), verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M slides around %N's attack and %s %s.",
                this, target.get(), verbPlural.c_str(), target->himHer());
            if(target->isPlayer())
                target->printColor("^M%M^x slides around your attack and %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 4:
            printColor("^cYou spin away from ^M%N^c's attack and %s %s for %s%d^c damage.\n",
                target.get(), verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M spins away from %N's attack and %s %s.",
                this, target.get(), verbPlural.c_str(), target->himHer());
            if(target->isPlayer())
                target->printColor("^M%M^x spins away from your attack and %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 5:
            printColor("^cYou duck under ^M%N's^c attack and %s %s for %s%d^c damage.\n",
                target.get(), verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M ducks under %N's attack and %s %s.",
                this, target.get(), verbPlural.c_str(), target->himHer() );
            if(target->isPlayer())
                target->printColor("^M%M^x ducks under your attack and %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 6:
            printColor("^cYou laugh at ^M%N^c and quickly %s %s for %s%d^c damage.\n",
                target.get(), verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M laughs at %N and quickly %s %s.",
                this, target.get(), verbPlural.c_str(), target->himHer() );
            if(target->isPlayer())
                target->printColor("^M%M^x laughs at you and quickly %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 7:
            printColor("^cYou riposte ^M%N's^c attack and %s %s for %s%d^c damage.\n",
                target.get(), verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M ripostes %N's attack and %s %s.",
                this, target.get(), verbPlural.c_str(), target->himHer() );
            if(target->isPlayer())
                target->printColor("^M%M^x ripostes your attack and %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        }
        if(weapon) {
            if(!Random::get(0, 7))
                weapon->decShotsCur();

            // die check moved right before return.
            if(weapon->getShotsCur() <= 0) {
                *this << ColorOn << setf(CAP) << weapon << " just broke.\n" << ColorOff;
                broadcast(getSock(), target->getSock(), getRoomParent(), "%M's just broke %P.", this, weapon.get());
                unequip(WIELD);
            }
        }

        if(target->isPlayer()) {
            std::shared_ptr<Player> pCrt = target->getAsPlayer();
            pCrt->updateAttackTimer();
        }

        if(isPlayer()) {
            getAsPlayer()->increaseFocus(FOCUS_RIPOSTE, attackDamage.get(), target);
        }



        meKilled = doReflectionDamage(attackDamage, target);

        if( doDamage(target, attackDamage.get(), CHECK_DIE, PHYSICAL_DMG, freeTarget) ||
            wasKilled ||
            meKilled)
        {
            Creature::simultaneousDeath(Containable::downcasted_shared_from_this<Creature>(), target, false, freeTarget);
            return(2);
        }
    }

    return(1);
}

//**********************************************************************
//                      autoAttackEnabled
//**********************************************************************

bool Player::autoAttackEnabled() const {
    return(!flagIsSet(P_NO_AUTO_ATTACK));
}

//**********************************************************************
//                      setFleeing
//**********************************************************************

void Player::setFleeing(bool pFleeing) {
    fleeing = pFleeing;
}

//**********************************************************************
//                      autoAttackEnabled
//**********************************************************************

bool Player::isFleeing() const {
    return(fleeing);
}

//*********************************************************************
//                      isHidden
//*********************************************************************

bool Creature::isHidden() const {
    return(flagIsSet(isPlayer() ? P_HIDDEN : M_HIDDEN));
}

//*********************************************************************
//                      negAuraRepel
//*********************************************************************

bool Creature::negAuraRepel() const {
    return(cClass == CreatureClass::LICH && !isEffected("resist-magic"));
}

//*********************************************************************
//                      doResistMagic
//*********************************************************************

int Creature::doResistMagic(int dmg, const std::shared_ptr<Creature>& enemy) {
    double resist=0;
    dmg = std::max<int>(1, dmg);

    if(negAuraRepel() && enemy && this != enemy.get()) {
        resist = (10.0 + Random::get(1,3) + level + bonus(constitution.getCur()))
            - (bonus(enemy->intelligence.getCur()) + bonus(enemy->piety.getCur()));
        resist /= 100; // percentage
        resist *= dmg;
        dmg -= (int)resist;
    }

    if(isEffected("resist-magic")) {
        resist = (piety.getCur() / 10.0 + intelligence.getCur() / 10.0) * 2;
        resist = std::max<float>(50, std::min<float>(resist, 100));
        resist /= 100; // percentage
        resist *= dmg;
        dmg -= (int)resist;
    }

    return(std::max<int>(0, dmg));
}

int Creature::getPrimaryDelay() {
    return (ready[WIELD-1] ? ready[WIELD-1]->getWeaponDelay() : DEFAULT_WEAPON_DELAY);
}

int Creature::getSecondaryDelay() {
    return ((ready[HELD-1] && ready[HELD-1]->getWearflag() == WIELD) ?
                          ready[HELD-1]->getWeaponDelay() : DEFAULT_WEAPON_DELAY);
}

std::string Creature::getPrimaryWeaponCategory() const {
    if(ready[WIELD-1])
        return(ready[WIELD-1]->getWeaponCategory());
    else
        return(NONE_STR);
}

std::string Creature::getSecondaryWeaponCategory() const {
    if(ready[HELD-1] && ready[HELD-1]->getWearflag() == WIELD)
        return(ready[HELD-1]->getWeaponCategory());
    else
        return(NONE_STR);
}


//*********************************************************************
//                      updateAttackTimer
//*********************************************************************

void Creature::updateAttackTimer(bool setDelay, int delay) {
    if(!setDelay) {
        attackTimer.update();
    } else {
        if(delay == 0) {
            if (ready[HELD-1] && ready[HELD-1]->getWearflag() == WIELD)
                delay = std::max(getPrimaryDelay(), getSecondaryDelay());
            else
                delay = getPrimaryDelay();
        }
        attackTimer.update(delay);
    }
}

//*********************************************************************
//                      getAttackDelay
//*********************************************************************

int Creature::getAttackDelay() const {
    return(attackTimer.getDelay());
}

//*********************************************************************
//                      checkAttackTimer
//*********************************************************************
// Return: true if the attack timer has expired, false if not

bool Creature::checkAttackTimer(bool displayFail) const {
    long i;
    if(((i = attackTimer.getTimeLeft()) != 0) && !isDm()) {
        if(displayFail) pleaseWait((double)i/10.0);
        return(false);
    }
    return(true);
}

//*********************************************************************
//                      modifyAttackDelay
//*********************************************************************

void Creature::modifyAttackDelay(int amt) {
    attackTimer.modifyDelay(amt);
}

//*********************************************************************
//                      setAttackDelay
//*********************************************************************

void Creature::setAttackDelay(int newDelay) {
    attackTimer.setDelay(newDelay);
}

//*********************************************************************
//                      pleaseWait
//*********************************************************************

void Creature::pleaseWait(long duration) const {
    if(duration < 0)
        duration *= -1;
    if(duration == 1)
        print("Please wait 1 more second.\n");
    else if(duration > 60)
        print("Please wait %d:%02d minutes.\n", duration / 60L, duration % 60L);
    else
        print("Please wait %d more seconds.\n", duration);
}

void Creature::pleaseWait(int duration) const {
    pleaseWait((long)duration);
}

void Creature::pleaseWait(double duration) const {
    // Floating point equality is unreliable
    if(duration <= 1 && duration >= 1)
        print("Please wait 1.0 more second.\n");
    else
        print("Please wait %.1f more seconds.\n", duration);
}

//*********************************************************************
//                      getLTAttack
//*********************************************************************

time_t Creature::getLTAttack() const {
    return(attackTimer.getLT());
}




/*          Web Editor
 *           _   _  ____ _______ ______
 *          | \ | |/ __ \__   __|  ____|
 *          |  \| | |  | | | |  | |__
 *          | . ` | |  | | | |  |  __|
 *          | |\  | |__| | | |  | |____
 *          |_| \_|\____/  |_|  |______|
 *
 *      If you change anything here, make sure the changes are reflected in the web
 *      editor! Either edit the PHP yourself or tell Dominus to make the changes.
 */

//*********************************************************************
//                      getTypeModifier
//*********************************************************************

float Object::getTypeModifier() const {
    std::string armorType = getArmorType();

    if(armorType == "plate" || armorType == "shield")
        return(42.89);
    else if(armorType == "chain")
        return(34.14);
    else if(armorType == "scale")
        return(31.12);
    else if(armorType == "ring")
        return(28.24);
    else if(armorType == "leather")
        return(24.46);
    else if(armorType == "cloth")
        return(12.25);
    else
        return(0.00);

     //Cloth-> Crayola sent, "how about 15.04, corresponding to 45%?"
}

//*********************************************************************
//                      getLocationModifier
//*********************************************************************

float Object::getLocationModifier() const {
    switch(wearflag) {
        case BODY:
            return(0.24);
        case ARMS:
            return(0.08);
        case LEGS:
            return(0.12);
        case NECK:
            return(0.08);
        case BELT:
            return(0.06);
        case HANDS:
            return(0.04);
        case HEAD:
            return(0.08);
        case FEET:
            return(0.06);
        case FINGER:
            return(0.00);
        case SHIELD:
            return(0.12);
        case FACE:
            return(0.08);
//      case BRACER:
//          return(0.04);
        default:
            return(0.00);
    }
}

//*********************************************************************
//                      adjustArmor
//*********************************************************************

int Object::adjustArmor() {
    if(type != ObjectType::ARMOR || quality <= 0 || wearflag == WIELD || wearflag == FINGER)
        return(-1);

    return(armor = (short)(getTypeModifier() * ((quality/10.0) + 2.3525) * getLocationModifier()));
}

//*********************************************************************
//                      adjustWeapon
//*********************************************************************

int Object::adjustWeapon() {
    if(type != ObjectType::WEAPON || quality <= 0 || wearflag != WIELD) {
        damage.setMean(0);
        return(-1);
    }

    // Quality for weapons directly affects damage. If any of these factors change,
    // we need to readjust quality.
    //  attack delay
    //  numAttacks

    // For a given quality, convert to damage-per-second
    double dps = (quality / 11.4 + 5) / 2.6;
    short num = (numAttacks ? numAttacks : (short)1);

    // For a given damage-per-second, convert it to average damage.
    // This is the equation from Object::statObj backwards.
    double avg = ((getWeaponDelay()/10.0) * dps) / ((1.0 + num) / 2.0);

    // Set the average damage for this weapon
    damage.clear();
    damage.setMean(avg);

    return((int)avg);
}
