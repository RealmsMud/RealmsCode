/*
 * objectAttr.cpp
 *   Object attribute functions
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

#include <ctime>          // for time

#include "bstring.hpp"    // for bstring
#include "creatures.hpp"  // for Player
#include "effects.hpp"    // for EFFECT_MAX_DURATION, EFFECT_MAX_STRENGTH
#include "objects.hpp"    // for Object, ObjectType, ObjectType::ARMOR, Mate...
#include "size.hpp"       // for Size
#include "utils.hpp"      // for MAX, MIN



//*********************************************************************
//                      track
//*********************************************************************

void Object::track(Player* player) { lastMod = player->getName(); }

void Object::setDelay(int newDelay) { delay = newDelay; }
void Object::setExtra(int x) { extra = x; }
void Object::setWeight(short w) { weight = w; }
void Object::setBulk(short b) { bulk = MAX<short>(0, b); }
void Object::setMaxbulk(short b) { maxbulk = b; }
void Object::setSize(Size s) { size = s; }
void Object::setType(ObjectType t) { type = t; }
void Object::setWearflag(short w) { wearflag = w; }
void Object::setArmor(short a) { armor = MAX<short>(0, MIN<short>(a, 1000)); }
void Object::setQuality(short q) { quality = q; }
void Object::setAdjustment(short a) {
    removeFromSet();
    adjustment = MAX<short>(-127, MIN<short>(a, 127));
    addToSet();
}

void Object::setNumAttacks(short n) { numAttacks = n; }
void Object::setShotsMax(short s) { shotsMax = s; }
void Object::setShotsCur(short s) { shotsCur = s; }
void Object::decShotsCur(short s) { shotsCur -= s; }
void Object::incShotsCur(short s) { shotsCur += s; }
void Object::setChargesMax(short s) { chargesMax = s; }
void Object::setChargesCur(short s) { chargesCur = s; }
void Object::decChargesCur(short s) { chargesCur -= s; }
void Object::incChargesCur(short s) { chargesCur += s; }

void Object::setMagicpower(short m) { magicpower = m; }
void Object::setLevel(short l) { level = l; }
void Object::setRequiredSkill(int s) { requiredSkill = s; }
void Object::setMinStrength(short s) { minStrength = s; }
void Object::setClan(short c) { clan = c; }
void Object::setSpecial(short s) { special = s; }
void Object::setQuestnum(short q) { questnum = q; }
void Object::setEffect(const bstring& e) { effect = e; }
void Object::setEffectDuration(long d) { effectDuration = MAX<long>(-1, MIN<long>(d, EFFECT_MAX_DURATION)); }
void Object::setEffectStrength(short s) { effectStrength = MAX<long>(0, MIN<long>(s, EFFECT_MAX_STRENGTH)); }
void Object::setCoinCost(unsigned long c) { coinCost = c; }

void Object::setShopValue(unsigned long v) {
    removeFromSet();
    shopValue = MIN<unsigned long>(200000000, v);
    addToSet();
}

void Object::setLotteryCycle(int c) { lotteryCycle = c; }
void Object::setLotteryNumbers(short i, short n) { lotteryNumbers[i] = n; }
void Object::setRecipe(int r) { recipe = r; }
void Object::setMaterial(Material m) { material = m; }
void Object::setQuestOwner(const Player* player) { questOwner = player->getName(); }

void Object::clearEffect() {
    setEffect("");
    setEffectDuration(0);
    setEffectStrength(0);
}
bool Object::flagIsSet(int flag) const {
    return(flags[flag/8] & 1<<(flag%8));
}

void Object::setFlag(int flag) {
    flags[flag/8] |= 1<<(flag%8);
}

void Object::clearFlag(int flag) {
    flags[flag/8] &= ~(1<<(flag%8));
}

bool Object::toggleFlag(int flag) {
    if(flagIsSet(flag))
        clearFlag(flag);
    else
        setFlag(flag);
    return(flagIsSet(flag));
}

bstring Object::getVersion() const {
    return(version);
}

void Object::setMade() {
    made = time(nullptr);
}

bool Object::isHeavyArmor() const {
//  return(type == ObjectType::ARMOR && (subType == "chain" || subType == "plate"));
    return(type == ObjectType::ARMOR && subType == "plate");
}

bool Object::isMediumArmor() const {
    return(type == ObjectType::ARMOR && (subType == "chain" || subType == "scale" || subType == "ring"));
}

bool Object::isLightArmor() const {
    return(type == ObjectType::ARMOR && (subType == "cloth" || subType == "leather"));
}

bool Object::isBroken() const {
    return(shotsCur == 0 && shotsMax >= 0 && (type == ObjectType::ARMOR || type == ObjectType::KEY || type == ObjectType::WEAPON));
}
