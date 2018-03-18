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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <sstream>

#include "commands.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "specials.hpp"


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
                case ARES:
                    attackPower += (level * 8);
                    break;
                case ENOCH:
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

unsigned int Creature::getBaseDamage() const {
    return(getAttackPower()/15);
}

//****************************************************************************
//                      getDamageReduction
//****************************************************************************
// Returns the damage reduction factor when attacking the given target

float Creature::getDamageReduction(const Creature* target) const {
    float reduction = 0.0;
    float targetArmor = target->getArmor();
    float myLevel = level == 1 && isPlayer() ? 2 : level;
    reduction = ((targetArmor)/((targetArmor) + 43.24 + 18.38*myLevel));
    reduction = MIN<float>(.75, reduction); // Max of 75% reduction
    return(reduction);
}

//**********************************************************************
//                      getWeaponType
//**********************************************************************

const bstring Object::getWeaponType() const {
    if(type != ObjectType::WEAPON)
        return("none");
    else
        return(subType);
}

//**********************************************************************
//                      getArmorType
//**********************************************************************

const bstring Object::getArmorType() const {
    if(type != ObjectType::ARMOR)
        return("none");
    else
        return(subType);
}

//**********************************************************************
//                      getWeaponCategory
//**********************************************************************

const bstring Object::getWeaponCategory() const {
    SkillInfo* weaponSkill = gConfig->getSkill(subType);

    if(type != ObjectType::WEAPON || !weaponSkill)
        return("none");

    bstring category = weaponSkill->getGroup();
    // Erase the 'weapons-' to leave the category
    category.erase(0, 8);

    return(category);
}

//**********************************************************************
//                      getWeaponVerb
//**********************************************************************


const bstring  Creature::getWeaponVerb() const {
    if(ready[WIELD-1])
        return(ready[WIELD-1]->getWeaponVerb());
    else
        return("thwack");

}

const bstring Object::getWeaponVerb() const {
    const bstring category = getWeaponCategory();

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


const bstring Creature::getWeaponVerbPlural() const {
    if(ready[WIELD-1])
        return(ready[WIELD-1]->getWeaponVerbPlural());
    else
        return("thwacks");

}

const bstring  Object::getWeaponVerbPlural() const {
    const bstring category = getWeaponCategory();

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

const bstring Object::getWeaponVerbPast() const {
    const bstring category = getWeaponCategory();

    if(category == "crushing") {
        if(mrand(0,1))
            return("crushed");
        else
            return("bludgeoned");
    }
    else if(category == "piercing") {
        if(mrand(0,1))
            return("impaled");
        else
            return("stabbed");
    }
    else if(category == "slashing") {
        if(mrand(0,1))
            return("gutted");
        else
            return("slashed");
    }
    else if(category == "ranged")
        return("shot");
    else if(category == "chopping") {
        if(mrand(0,1))
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
    const bstring weaponType = getWeaponType();

    if(flagIsSet(O_TWO_HANDED))
        return(true);
    if( weaponType == "great-axe" || weaponType == "great-mace" ||
        weaponType == "great-hammer" || weaponType == "staff" ||
        weaponType == "great-sword" || weaponType == "polearm"
    )
        return(true);
    if((weaponType == "bow" || weaponType == "crossbow") && !flagIsSet(O_SMALL_BOW))
        return(true);

    return(false);
}

//**********************************************************************
//                      setWeaponType
//**********************************************************************

bool Object::setWeaponType(const bstring& newType) {
    SkillInfo* weaponSkill = gConfig->getSkill(newType);
    bstring category = "";
    if(weaponSkill) {
        category = weaponSkill->getGroup();
        category = category.left(6);
    }

    if(!weaponSkill || category != "weapon") {
        std::clog << "Invalid weapon type: " << newType.c_str() << std::endl;
        return(false);
    }
    subType = newType;
    return(true);
}

//**********************************************************************
//                      setArmorType
//**********************************************************************

bool Object::setArmorType(const bstring& newType) {
    // Armor type must be ring, shield, or one of the armor type skills
    if(newType != "ring" && newType != "shield") {
        SkillInfo* weaponSkill = gConfig->getSkill(newType);
        bstring category = "";
        if(weaponSkill) {
            category = weaponSkill->getGroup();
            category = category.left(5);
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

bool Object::setSubType(const bstring& newType) {
    // These must be set with the appropriate function
    if(type == ObjectType::ARMOR || type == ObjectType::WEAPON)
        return(false);

    subType = newType;
    return(true);
}

//**********************************************************************
//                      getWeaponSkill
//**********************************************************************

int Monster::getWeaponSkill(const Object* weapon) const {
    // Same skills for all weapons
    return(weaponSkill);
}

//**********************************************************************
//                      getWeaponSkill
//**********************************************************************

int Player::getWeaponSkill(const Object* weapon) const {;
    int bonus = 0;

    // Bless improves your chance to hit
    if(isEffected("bless"))
        bonus += 10;

//  print("Looking at weapon skill for %s.\n", weapon ? weapon->getCName() : "null object");
    bstring weaponType;
    if(weapon)
        weaponType = weapon->getWeaponType();
    else
        weaponType = getUnarmedWeaponSkill();

    // we're very confused about what type of weapon this is
    if(weaponType == "")
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

//**********************************************************************
//                      AttackResult
//**********************************************************************
// defense - skill
// 295 - 300 = -5 * .01% less chance to miss
// 320 - 300 = 20 * .01% more chance to miss

AttackResult Creature::getAttackResult(Creature* victim, const Object* weapon, int resultFlags, int altSkillLevel) {
    /*
    int pFd;
    if(isPlayer())
        pFd = fd;
    else
        pFd = victim->getSock();
    */
    int mySkill=0;

    if(altSkillLevel != -1) {
        // For when we want to look at an alternative skill, like kick
        mySkill = altSkillLevel;
    } else {
        mySkill = getWeaponSkill(weapon);
    }
    int defense = victim->getDefenseSkill();
    int difference = defense - mySkill;


    double missChance=0;
    double dodgeChance=0;
    double parryChance=0;
    double glancingChance=0;
    double blockChance=0;
    double criticalChance=0;
    double fumbleChance=0;
    //double hitChance=0;

    EffectInfo* effect=0;

    missChance = victim->getMissChance(difference);

    if(victim->isEffected("blur") && !isEffected("true-sight"))
        missChance += victim->getEffect("blur")->getStrength();

    if(victim->getRoomParent()->isEffected("dense-fog")) {
        effect = victim->getRoomParent()->getEffect("dense-fog");
        if(!effect->isOwner(this))
            missChance += effect->getStrength();
    }

    if(resultFlags & DOUBLE_MISS)
        missChance *= 2;

    int missCutoff = (int)(missChance * 100);

    if(resultFlags & NO_DODGE)
        dodgeChance = 0;
    else
        dodgeChance = victim->getDodgeChance(this, difference);
    int dodgeCutoff = (int)(missCutoff + (dodgeChance * 100));

    if(resultFlags & NO_PARRY)
        parryChance = 0;
    else
        parryChance = victim->getParryChance(this, difference);
    int parryCutoff = (int)(dodgeCutoff + (parryChance * 100));

    if(resultFlags & NO_GLANCING)
        glancingChance = 0;
    else
        glancingChance = victim->getGlancingBlowChance(this, difference);
    int glancingCutoff = (int)(parryCutoff + (glancingChance * 100));

    if(resultFlags & NO_BLOCK)
        blockChance = 0;
    else
        blockChance = victim->getBlockChance(this, difference);
    int blockCutoff = (int)(glancingCutoff + (blockChance * 100));

    if((resultFlags & NO_CRITICAL) || victim->immuneCriticals())
        criticalChance = 0;
    else
        criticalChance = getCriticalChance(difference);
    int criticalCutoff = (int)(blockCutoff + (criticalChance * 100));

    if(resultFlags & NO_FUMBLE)
        fumbleChance = 0;
    else
        fumbleChance = getFumbleChance(weapon);
    int fumbleCutoff = (int)(criticalCutoff + (fumbleChance * 100));

    //hitChance = MAX(0, 100.0 - missChance - dodgeChance - parryChance - glancingChance - blockChance - criticalChance - fumbleChance);
    //int hitCutoff = (int)(fumbleCutoff + (hitChance * 100));

    // Auto miss with an always crit weapon vs a no crit monster
    if(weapon && weapon->flagIsSet(O_ALWAYS_CRITICAL)) {
        if(victim->mFlagIsSet(M_NO_AUTO_CRIT) || victim->immuneCriticals())
            missCutoff = 10000;
        else {
            missCutoff = dodgeCutoff = parryCutoff = glancingCutoff = blockCutoff = -1;
            criticalCutoff = 10000;
        }

    }

//  printColor("^y%d attack skill %d defense skill, %d difference.\n", mySkill, defense, difference);
//  printColor("^cChances: ^C%f^c miss, ^C%f^c dodge, ^C%f^c parry, ^C%f^c glancing, ^C%f^c block, ^C%f^c critical, ^C%f^c fumble, ^C%f^c leftover\n",
//          missChance, dodgeChance, parryChance, glancingChance, blockChance, criticalChance, fumbleChance, hitChance);
//  printColor("^gCutoffs: ^G%d^g miss,  ^G%d^g dodge,  ^G%d^g parry,  ^G%d^g glancing,  ^G%d^g block, ^G%d^g critical ^G%d^g fumble ^G%d^g hit.\n",
//          missCutoff, dodgeCutoff, parryCutoff, glancingCutoff, blockCutoff, criticalCutoff, fumbleCutoff, hitCutoff);
//
    int randNum = mrand(0, 10000);
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
        if(victim->kamiraLuck(this))
            result = ATTACK_MISS;

    return(result);
}

//**********************************************************************
//                      adjustChance
//**********************************************************************

int Creature::adjustChance(const int &difference) {
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
        if(isPlayer())
            adjustment = 0.04;
        else
            adjustment = 0.04;
    }
    return((int)(difference * adjustment));
    //chance += (difference * adjustment);
}

//**********************************************************************
//                      getFumbleChance
//**********************************************************************

double Creature::getFumbleChance(const Object* weapon) {
    double chance = 2.0;

    if(!weapon || weapon->flagIsSet(O_CURSED) || isDm()) {
        chance = 0.0;
    }
    else {
        double weaponSkill = getSkillLevel(weapon->getWeaponType());
        // Spread the reduction over all 300 skill points, leaving a .01 chance to fumble
        // even at 300 skill
        chance -= (weaponSkill / 151.0);

        chance = MAX(0.0, chance);
    }
    return(chance);
}

//**********************************************************************
//                      getCriticalChance
//**********************************************************************

double Creature::getCriticalChance(const int& difference) {
    double chance = 5.0;

    chance -= adjustChance(difference);
    chance = MAX(0.0, chance);
    return(chance);
}

//**********************************************************************
//                      getBlockChance
//**********************************************************************

double Creature::getBlockChance(Creature* attacker, const int& difference) {
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
        chance = MIN(5.0, chance);

    chance = MAX(0.0, chance);
    return(chance);
}

//**********************************************************************
//                      getGlancingBlowChance
//**********************************************************************
// A glancing blow is the opposite of a critical hit; reduced damage
// when a player or pet is attacking a monster but only against
// monsters of the same level or higher

double Creature::getGlancingBlowChance(Creature* attacker, const int& difference) {
    if(isPlayer() || isPet() || (attacker->isMonster() && !attacker->isPet()))
        return(0);

    if(level < attacker->getLevel())
        return(0);

    double chance = 10.0 + (difference * 0.5);
    chance = MAX(0.0, chance);
    return(chance);
}

//**********************************************************************
//                      getParryChance
//**********************************************************************
// Chance that they will parry or riposte an attack.
// 50% of parry will be a riposte

double Creature::getParryChance(Creature* attacker, const int& difference) {
    if(isPlayer()) {
        if(!canDodge(attacker) || !canParry(attacker))
            return(0);

        if(!knowsSkill("parry"))
            return(0);

    } else {
//      if(level < 7)
//          return(false);
    }
    double chance = (dexterity.getCur() - 80) * .03;
    chance += adjustChance(difference);

    // Riposte is harder against some attacker types
    if( attacker->type == DRAGON || attacker->type == DEMON || attacker->type == DEVIL ||
        attacker->type == GIANTKIN || attacker->type == DEVA || attacker->type == DINOSAUR ||
        attacker->type == ELEMENTAL
    )
        chance /= 2;

    int numEnm = 0;
    for(Monster* mons : getRoomParent()->monsters) {
        if(mons->isPet())
            continue;
        if(mons->isEnemy(this))
            if(numEnm++ > 1)
                chance -= .02;
    }
    chance = MAX(0.0, chance);
    return(chance);
}

//**********************************************************************
//                      canParry
//**********************************************************************

bool Creature::canParry(Creature* attacker) {
    if(attacker->isDm())
        return(false);
    if(isMonster())
        return(true);

    if(!ready[WIELD - 1])
        return(false);          // must have a weapon wielded in order to riposte.

    bstring weaponCategory = getPrimaryWeaponCategory();
    bstring weaponType = ready[WIELD - 1]->getWeaponType();
    if((weaponCategory != "piercing" && weaponCategory != "slashing") || weaponType == "polearm")
        return(false);          // must use a piercing or slashing weapon to riposte

    if(ready[WIELD-1]->needsTwoHands())
        return(false);          // Cannot riposte with a two-handed weapon.

    int combatPercent=0;
//   If a this is overwhelmed by mass amounts of enemies, it is difficult if
//   not impossible to find an opening to move enough to do a parry. The following
//   code will calculate the chance of failure into a "combat percent". If a check
//   against this percent fails, the this did not find enough of an opening to
//   try a parry, so parry returns 0 without going off. -TC

    // +3% fail for every monster mad at this besides the one he's currently hitting
    combatPercent = 3*(MAX(0,numEnemyMonInRoom(this)-1));
    // Group members are assumed to fight together to help one another.
    // -2% fail for every member in the this's group besides themself in the same room
    if(getGroup())
        combatPercent -= 2*(MAX(0,getGroup()->getNumInSameRoom(this)));

    combatPercent = MAX(0,combatPercent);

    if(mrand(1,100) <= combatPercent)
        return(0); // Did not find a parry opening due to excessive combat. No parry.

    // Parry is not possible against some types of creatures
    if( attacker->type == INSECT || attacker->type == AVIAN ||
        attacker->type == FISH || attacker->type == REPTILE || attacker->type == PLANT ||
        attacker->type == ETHEREAL || attacker->type == ASTRAL ||
        attacker->type == GASEOUS || attacker->type == ENERGY ||
        attacker->type == PUDDING || attacker->type == SLIME
    )
        return(false);

    long t = time(0);
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

bool Creature::canDodge(Creature* attacker) {
    if(attacker->isDm())
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

        if(getRoomParent()->isUnderwater() && !flagIsSet(P_FREE_ACTION))
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

        // Players waiting and hiding will not dodge, now so they won't auto unhide themselves.
        if(flagIsSet(P_HIDDEN))
            return(false);

    }
    return(true);
}

//**********************************************************************
//                      getDodgeChance
//**********************************************************************
// What chance do we have to dodge the attacker?

double Creature::getDodgeChance(Creature* attacker, const int& difference) {
    if(!canDodge(attacker))
        return(false);

//  if(!knowsSkill("dodge"))
//      return(0);
//
    double chance = 0.0;
    Player* pPlayer = getAsPlayer();

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
                    chance += (dexterity.getCur() * .07);
                else
                    chance += (1.0 + (dexterity.getCur() * .045));
                break;
            case CreatureClass::BARD:
            case CreatureClass::PALADIN:
            case CreatureClass::DEATHKNIGHT:
            case CreatureClass::WEREWOLF:
            case CreatureClass::PUREBLOOD:
            case CreatureClass::MONK:
                chance += (2.0 + (dexterity.getCur() * .05));
                break;
            case CreatureClass::CLERIC:
                if(pPlayer->getSecondClass() == CreatureClass::ASSASSIN) {
                    chance += (dexterity.getCur() * .06);
                    break;
                }
                switch(deity) {
                    case KAMIRA:
                    case ARACHNUS:
                        chance += (piety.getCur() * .05);
                        break;
                    case CERIS:
                    case ARES:
                    case ENOCH:
                    case GRADIUS:
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
    chance = MAX(0.0, chance);
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
    Player* pPlayer = getAsPlayer();
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
                        chance *= 0.8;
                        break;
                    case ARACHNUS:
                    case KAMIRA:
                    case CERIS:
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
                if(pPlayer->getSecondClass() == CreatureClass::THIEF || pPlayer->getSecondClass() == CreatureClass::ASSASSIN)
                    chance *= 0.8;
                else
                    chance *= 1.0;
                break;
            default:
                break;
        }
    } else {
        // Not a player
        chance *= 1.0;
    }

    chance = MAX(0.0, chance);
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

bool Creature::canHit(Creature* victim, Object* weapon, bool glow, bool showFail) {
    //Monster* mVictim = victim->getMonster();
    Player* pVictim = victim->getAsPlayer();

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
            if(weapon->flagIsSet(O_SILVER_OBJECT))
                silver = true;
        }

        if(pVictim) {
        // It's a player

            // DMs can hit vampires
            if(!isDm()) {
                if(pVictim->isEffected("mist")) {
                    if(weapon) {
                        if( !weapon->flagIsSet(O_CAN_HIT_MIST) && !flagIsSet(P_MISTBANE) )
                        {
                            if(showFail) printColor("Your cannot hit a misted creature with your %1P.\n", weapon);
                            return(false);
                        }
                    } else if(!flagIsSet(P_MISTBANE)) {
                        if(showFail) print("You cannot physically hit a misted creature.\n");
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
                    if(weapon) {
                        if(weapon->getType() == ObjectType::WEAPON)
                            print("Your %s has no effect on %N.\n", weapon->getCName(), pVictim);
                        else
                            print("Your %s have no effect on %N.\n", weapon->getCName(), pVictim);
                    }
                    else
                        print("Your attack has no effect on %N.\n", pVictim);

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
                    if(glow) printColor("^WYour claws glow radiantly in the night against %N.\n", victim);
                }
                else if(cClass == CreatureClass::MONK &&
                    flagIsSet(P_FOCUSED) &&
                    ( (level >= 16 && victim->flagIsSet(M_ENCHANTED_WEAPONS_ONLY)) ||
                            (level >= 16 && victim->flagIsSet(M_PLUS_TWO))
                    )
                ) {
                    if(glow) printColor("^WYour fists glow with power against %N.\n", victim);
                }
                else if(weapon && (
                        (victim->flagIsSet(M_ENCHANTED_WEAPONS_ONLY) && enchant > 0) ||
                        (victim->flagIsSet(M_PLUS_TWO) && enchant > 1) ||
                        (victim->flagIsSet(M_PLUS_THREE) && enchant > 2)
                    )
                ) {
                    if(glow && weapon) {
                        if(weapon->getType() == ObjectType::WEAPON)
                            printColor("^WYour %s^W glows with power against %N.\n", weapon->getCName(), victim);
                        else
                            printColor("^WYour^W %s glow with power against %N.\n", weapon->getCName(), victim);
                    }
                } else if(isDm()) {
                    if(glow)
                        printColor("^WYou ignore the laws of the land and attack %N anyway.\n", victim);
                } else {
                    if(showFail) {
                        if(weapon) {
                            if(weapon->getType() == ObjectType::WEAPON)
                                printColor("^cYour %s has no effect on %N.\n", weapon->getCName(), victim);
                            else
                                printColor("^cYour %s have no effect on %N.\n", weapon->getCName(), victim);
                        } else
                            printColor("^cYour attack has no effect on %N.\n", victim);
                    }
                    return(false);
                }
            }
        }

    }
    return(true);
}

//********************************************************************************
//                      computeDamage
//********************************************************************************
// Returns 0 on normal computation, 1 if the weapon was shattered

int Player::computeDamage(Creature* victim, Object* weapon, AttackType attackType, AttackResult& result, Damage& attackDamage, bool computeBonus, int& drain, float multiplier) {
    int retVal = 0;
    Damage bonusDamage;
    bstring weaponCategory = weapon ? weapon->getWeaponCategory() : "none";
    drain = 0;

    if(attackType == ATTACK_KICK) {
        if(computeBonus)
            bonusDamage.set(getBaseDamage()/2);
        attackDamage.set(mrand(2,6) + ::bonus(strength.getCur()));
        if((cClass == CreatureClass::FIGHTER || cClass == CreatureClass::MONK) && !hasSecondClass()) {
            attackDamage.add((int)(getSkillLevel("kick") / 4));
        }
    } else if(attackType == ATTACK_MAUL) {
        if(computeBonus)
            bonusDamage.set(getBaseDamage()/2);
        attackDamage.set(( mrand( (int)(level/2), (int)(level + 1)) + (strength.getCur()/10)));
        attackDamage.add(mrand(2, 4));
    } else {
        // Any non kick attack for now
        if(computeBonus)
            bonusDamage.set(getBaseDamage());

         if(!weapon) {
            if(cClass == CreatureClass::MONK) {
                attackDamage.set(mrand(1,2) + level/3 + mrand((1+level/4),(1+level)/2));
                if(strength.getCur() < 90) {
                    attackDamage.set(attackDamage.get() - (90-strength.getCur())/10);
                    attackDamage.set(MAX(1,attackDamage.get()));
                }
            } else {
                attackDamage.set(damage.roll());
                bonusDamage.set(bonusDamage.get() * 3 / 4);
            }
        }
        // TODO: Add in modifier based on weapon skill
    }

    if(weapon)
        attackDamage.add(weapon->damage.roll() + weapon->getAdjustment());

    if(isEffected("lycanthropy"))
        attackDamage.add(packBonus());

    attackDamage.set(MAX(1, attackDamage.get()));

    // Damage reduction on every hit
    if(cClass == CreatureClass::PALADIN) {
        if(deity == GRADIUS) {
            if(getAdjustedAlignment() < PINKISH || getAdjustedAlignment() > BLUISH) {
                multiplier /= 2;
                print("Your dissonance with earth reduces your damage.\n");
            }
        } else {
            if(getAlignment() < 0) {
                multiplier /= 2;
                print("Your evilness reduces your damage.\n");
            }
        }
    }
    if(cClass == CreatureClass::DEATHKNIGHT) {
        if(getAdjustedAlignment() > NEUTRAL) {
            multiplier /= 2;
            print("Your goodness reduces your damage.\n");
        }
    }

    // Bonus spread out over entire series of attack for multi attack weapons
    if(computeBonus) {
        if(cClass == CreatureClass::PALADIN) {
            int goodDmg = mrand(1, 1 + level / 3);
            if(deity == GRADIUS) {
                if(alignInOrder() && victim->getRace() != DWARF && victim->getDeity() != GRADIUS) {
                    bonusDamage.add(goodDmg);
                    print("Your attunement with earth increased your damage by %d.\n", goodDmg);
                }
            } else {
                if(getAdjustedAlignment() >= BLUISH && victim->getAlignment() <= 0) {
                    bonusDamage.add(goodDmg);
                    print("Your goodness increased your damage by %d.\n", goodDmg);
                }
            }
        }
        if(cClass == CreatureClass::DEATHKNIGHT) {
            // Only drain on 1st attack if a multi weapon
            if(getAdjustedAlignment() <= REDDISH && victim->getAdjustedAlignment() >= NEUTRAL)
                drain = mrand(1, 1 + level / 3);
        }
        if( (   cClass == CreatureClass::BERSERKER ||
                (cClass == CreatureClass::CLERIC && deity == ARES) ||
                isStaff()
            ) &&
            isEffected("berserk"))
        {
            // Only do a bonus if not a multi weapon, or is a multi weapon and it's the first attack
            if(weapon && weaponCategory != "ranged")
                bonusDamage.add((int)(attackDamage.get()/2));
        }

        if( (   (cClass == CreatureClass::DEATHKNIGHT && getAdjustedAlignment() <= REDDISH) ||
                (cClass == CreatureClass::PALADIN && getAdjustedAlignment() == ROYALBLUE)
            ) &&
            (isEffected("pray") || isEffected("dkpray"))
        )
            bonusDamage.add(mrand(1,3));

        if((isEffected("lycanthropy") || isCt()) && isEffected("frenzy"))
            bonusDamage.add(mrand(3,5));

        if((cClass == CreatureClass::MONK || isCt()) && flagIsSet(P_FOCUSED))
            bonusDamage.add(mrand(1,3));
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
        attackDamage.set((int)((float)(attackDamage.get() * multiplier)));
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
            default:
                strcpy(atk, "hit");
                break;
        }

        printColor("^gCRITICAL %s!\n", atk);
        broadcast(getSock(), getRoomParent(), "^g%M made a critical %s.", this, atk);
        mult = mrand(3, 5);
        attackDamage.set(attackDamage.get() * mult);
        drain *= mult;
        // We'll go with the assumption that if you critical on the first hit, the remaining
        // ones in that series will hurt a little more, so this will work just fine
        // if the first of a series of hits is a critical
        if(computeBonus)
            bonusDamage.set(bonusDamage.get() * mult);
        broadcastGroup(false, victim, "^g%M made a critical %s!\n", this, atk);
        if( attackType != ATTACK_KICK && weapon && !isDm() && (
                weapon->flagIsSet(O_ALWAYS_CRITICAL) ||
                (   !weapon->flagIsSet(O_NEVER_SHATTER) &&
                    !weapon->flagIsSet(O_STARTING) &&
                    // using half-percentages here:
                    // +0 = 3.5%, +1 = 2.5%, +2 = 1.5%, +3 = 0.5%
                    mrand(1, 200) <= (7 - weapon->getAdjustment()*2)
                )
            )
        ) {
            printColor("^YYour %s shatters.\n", weapon->getCName());
            broadcast(getSock(), getRoomParent(),"^Y%s %s shattered.", upHisHer(), weapon->getCName());
            retVal = 1;
        }
    } else if(result == ATTACK_GLANCING) {
        printColor("^CYou only managed to score a glancing blow!\n");
        broadcast(getSock(), getRoomParent(), "^C%M scored a glancing blow!", this);
        if(mrand(1,2)) {
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

    victim->modifyDamage(this, PHYSICAL, attackDamage, NO_REALM, weapon, 0, computeBonus ? OFFGUARD_NOREMOVE : OFFGUARD_REMOVE);

    // Don't forget to modify the bonus
    if(computeBonus) {
        // Make bonus proportional to weapon speed, otherwise it's too much with fast weapons, and
        // too little with slow weapons.  (Bonus / 3) * weapon speed
        bonusDamage.set((int)(((float)bonusDamage.get()/(float)DEFAULT_WEAPON_DELAY) * (weapon ? weapon->getWeaponDelay() : DEFAULT_WEAPON_DELAY)));

        victim->modifyDamage(this, PHYSICAL, bonusDamage, NO_REALM, weapon, 0, OFFGUARD_NOPRINT, true);
        attackDamage.setBonus(bonusDamage);
    }

    if(retVal != 1) {
        // If we didn't shatter, minimum of 1 damage
        attackDamage.set(MAX(attackDamage.get(), 1));
    }

    return(retVal);
}

//**********************************************************************
//                      computeDamage
//**********************************************************************

int Monster::computeDamage(Creature* victim, Object* weapon, AttackType attackType, AttackResult& result, Damage& attackDamage, bool computeBonus, int& drain, float multiplier) {
    Damage bonusDamage;

    if(computeBonus)
        bonusDamage.set(getBaseDamage());

    if(ready[WIELD-1])
        attackDamage.add(ready[WIELD-1]->damage.roll() + ready[WIELD-1]->getAdjustment());
    else
        attackDamage.add(damage.roll());

    attackDamage.add(::bonus(strength.getCur()));

    if(result == ATTACK_CRITICAL) {
        broadcast(nullptr, getRoomParent(), "%M made a critical hit.", this);
        int mult = mrand(2, 5);
        attackDamage.set(attackDamage.get() * mult);
        drain *= mult;
        // No shatter for mobs :)
    }
    else if(result == ATTACK_BLOCK) {
        attackDamage.set(victim->computeBlock(attackDamage.get()));
        if(computeBonus) {
            bonusDamage.set(victim->computeBlock(bonusDamage.get()));
        }
    }

    victim->modifyDamage(this, PHYSICAL, attackDamage);
    victim->modifyDamage(this, PHYSICAL, bonusDamage);
    attackDamage.setBonus(bonusDamage);

    attackDamage.set(MAX(1, attackDamage.get()));
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

int Creature::dodge(Creature* target) {
    int     i = mrand(1,10);

    unhide();
    smashInvis();

    if(isPlayer()) {
        Player* player = getAsPlayer();
        player->statistics.dodge();
        player->increaseFocus(FOCUS_DODGE);
    }

    if(target->isPlayer())
        target->getAsPlayer()->statistics.miss();

    if(i == 1) {
        printColor("^cYou barely manage to dodge %N's attack.\n", target);
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M barely dodges %N's attack.", this, target);
        if(target->isPlayer())
            target->printColor("^c%M somehow manages to dodge your attack.\n", this);
    } else if(i == 2) {
        printColor("^cYou deftly dodge %N's attack.\n", target);
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M deftly dodges %N's attack.", this, target);
        if(target->isPlayer())
            target->printColor("^c%M deftly dodges your attack.\n", this);
    } else if(i == 3 && (target->isPlayer())) {
        printColor("^cYou side-step %N's attack and smack %s on the back of the head.\n", target,
            target->himHer());
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M side-steps %N's attack and smacks %s on the back of the head.", this,
            target, target->himHer());
        if(target->isPlayer())
            target->printColor("^c%M side-steps your attack and smacks you on the back of the head.\n", this);
    } else if(i > 3 && i < 6) {
        printColor("^cYou dance gracefully around %N's attack.\n", target);
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M dances gracefully around %N's attack.", this, target);
        if(target->isPlayer())
            target->printColor("^c%M dances gracefully around your attack.\n", this);
    } else if(i >= 6 && i < 8) {
        printColor("^cYou easily dodge %N's attack.\n", target);
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M easily dodges %N's attack.", this, target);
        if(target->isPlayer())
            target->printColor("^c%M easily dodges your attack.\n",this);
    }
    else if(i >= 8 && i < 9) {
        printColor("^cYou easily duck under %N's pitifully executed attack.\n", target);
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M easily ducks under %N's pitifully executed attack.", this, target);
        if(target->isPlayer())
            target->printColor("^c%M easily ducks under your pitifully executed attack.\n",this);
    } else if(i >= 9 && i <= 10) {
        printColor("^cYou laugh at %N as you easily dodge %s attack.\n",
            target, target->hisHer());
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M laughs as %s easily dodges %N's attack.", this, heShe(), target);
        if(target->isPlayer())
            target->printColor("^c%M laughs as %s dodges your attack.\n", this, heShe());
    } else {
        printColor("^cYou deftly dodge %N's attack.\n", target);
        broadcast(getSock(), target->getSock(), getRoomParent(),
            "%M deftly dodges %N's attack.", this, target);
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
// This function allows a creature to either parry a blow, or to
// strike back after parry. It was made for the Rogue class. -- TC
// Returns 2 on death of person being reposited, returns 1 on non-death sucess
// This is only called when success has already been determined

int Creature::parry(Creature* target) {
    long    t=0;
    Object* weapon = ready[WIELD-1];
    Damage attackDamage;

    if(!weapon && isPlayer()) {
        broadcast(::isDm, "*** Parry error: called with null weapon for %s.\n", getCName());
        return(0);
    }

    t = time(0);
    //i = LT(this, LT_RIPOSTE);

    if(isPlayer()) {
        t=time(0);
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
        printColor("^cYou parry %N's attack.\n", target);
        broadcast(getSock(), target->getSock(), getRoomParent(), "%M parries %N's attack.", this, target);
        if(target->isPlayer())
            target->printColor("^c%M parries your attack.\n", this);
        if(isPlayer()) {
            getAsPlayer()->increaseFocus(FOCUS_PARRY);
        }
    } else {
        // We have a riposte, calculate damage and such
        bstring verb = getWeaponVerb();
        bstring verbPlural = getWeaponVerbPlural();
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
            attackDamage.set(MAX(1, attackDamage.get()));
        }

        switch(mrand(1,7)) {
        case 1:
            printColor("^cYou side-step ^M%N's^c attack and %s %s for %s%d^c damage.\n",
                    target, verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M side-steps %N's attack and %s %s.",
                this, target, verbPlural.c_str(), target->himHer());
            if(target->isPlayer())
                target->printColor("^M%M^x side-steps your attack and %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 2:
            printColor("^cYou lunge and viciously %s ^M%N^c for %s%d^c damage.\n",
                verb.c_str(), target, customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M lunges and viciously %s %N.",
                this, verbPlural.c_str(), target );
            if(target->isPlayer())
                target->printColor("^M%M^x lunges at you and viciously %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 3:
            printColor("^cYou slide around ^M%N's^c attack and %s %s for %s%d^c damage.\n",
                target, verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M slides around %N's attack and %s %s.",
                this, target, verbPlural.c_str(), target->himHer());
            if(target->isPlayer())
                target->printColor("^M%M^x slides around your attack and %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 4:
            printColor("^cYou spin away from ^M%N^c's attack and %s %s for %s%d^c damage.\n",
                target, verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M spins away from %N's attack and %s %s.",
                this, target, verbPlural.c_str(), target->himHer());
            if(target->isPlayer())
                target->printColor("^M%M^x spins away from your attack and %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 5:
            printColor("^cYou duck under ^M%N's^c attack and %s %s for %s%d^c damage.\n",
                target, verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M ducks under %N's attack and %s %s.",
                this, target, verbPlural.c_str(), target->himHer() );
            if(target->isPlayer())
                target->printColor("^M%M^x ducks under your attack and %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 6:
            printColor("^cYou laugh at ^M%N^c and quickly %s %s for %s%d^c damage.\n",
                target, verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M laughs at %N and quickly %s %s.",
                this, target, verbPlural.c_str(), target->himHer() );
            if(target->isPlayer())
                target->printColor("^M%M^x laughs at you and quickly %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        case 7:
            printColor("^cYou riposte ^M%N's^c attack and %s %s for %s%d^c damage.\n",
                target, verb.c_str(), target->himHer(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            broadcast(getSock(), target->getSock(), getRoomParent(), "%M ripostes %N's attack and %s %s.",
                this, target, verbPlural.c_str(), target->himHer() );
            if(target->isPlayer())
                target->printColor("^M%M^x ripostes your attack and %s you for %s%d^x damage.\n",
                    this, verbPlural.c_str(), target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());
            break;
        }
        if(weapon) {
            if(!mrand(0, 3))
                weapon->decShotsCur();

            // die check moved right before return.
            if(weapon->getShotsCur() <= 0) {
                printColor("%O just broke.\n", weapon);
                broadcast(getSock(), target->getSock(), getRoomParent(), "%M's just broke %P.", this, weapon);
                unequip(WIELD);
            }
        }

        if(target->isPlayer()) {
            Player* pCrt = target->getAsPlayer();
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
            Creature::simultaneousDeath(this, target, false, freeTarget);
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

int Creature::doResistMagic(int dmg, Creature* enemy) {
    float resist=0;
    dmg = MAX(1, dmg);

    if(negAuraRepel() && enemy && this != enemy) {
        resist = (10 + mrand(1,3) + level + bonus((int) constitution.getCur()))
            - (bonus((int)enemy->intelligence.getCur()) + bonus((int)enemy->piety.getCur()));
        resist /= 100; // percentage
        resist *= dmg;
        dmg -= (int)resist;
    }

    if(isEffected("resist-magic")) {
        resist = (piety.getCur() / 10 + intelligence.getCur() / 10) * 2;
        resist = MAX<float>(50, MIN<float>(resist, 100));
        resist /= 100; // percentage
        resist *= dmg;
        dmg -= (int)resist;
    }

    return(MAX(0, dmg));
}

int Creature::getPrimaryDelay() {
    return (ready[WIELD-1] ? ready[WIELD-1]->getWeaponDelay() : DEFAULT_WEAPON_DELAY);
}

int Creature::getSecondaryDelay() {
    return ((ready[HELD-1] && ready[HELD-1]->getWearflag() == WIELD) ?
                          ready[HELD-1]->getWeaponDelay() : DEFAULT_WEAPON_DELAY);
}

const bstring Creature::getPrimaryWeaponCategory() const {
    if(ready[WIELD-1])
        return(ready[WIELD-1]->getWeaponCategory());
    else
        return("none");
}

const bstring Creature::getSecondaryWeaponCategory() const {
    if(ready[HELD-1] && ready[HELD-1]->getWearflag() == WIELD)
        return(ready[HELD-1]->getWeaponCategory());
    else
        return("none");
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
                delay = MAX(getPrimaryDelay(), getSecondaryDelay());
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

bool Creature::checkAttackTimer(bool displayFail) {
    long i;
    if(!((i = attackTimer.getTimeLeft()) == 0) && !isDm()) {
        if(displayFail)
            pleaseWait(i/10.0);
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
    if(!(duration > 1) && !(duration < 1))
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
    bstring armorType = getArmorType();

    if(armorType == "plate" || armorType == "shield")
        return(42.89);
    else if(armorType == "chain")
        return(34.14);
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
    short num = (numAttacks ? numAttacks : 1);

    // For a given damage-per-second, convert it to average damage.
    // This is the equation from Object::statObj backwards.
    double avg = ((getWeaponDelay()/10.0) * dps) / ((1.0 + num) / 2.0);

    // Set the average damage for this weapon
    damage.clear();
    damage.setMean(avg);

    return(avg);
}
