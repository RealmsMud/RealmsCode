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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <ctime>                   // for time
#include <string>                  // for operator==, string
#include <string_view>             // for string_view

#include "effects.hpp"             // for EFFECT_MAX_DURATION, EFFECT_MAX_ST...
#include "mudObjects/objects.hpp"  // for Object, ObjectType, ObjectType::ARMOR
#include "mudObjects/players.hpp"  // for Player
#include "size.hpp"                // for Size
#include "utils.hpp"               // for MAX, MIN
#include "proto.hpp"               // for keyTxtCompare



//*********************************************************************
//                      track
//*********************************************************************

void Object::track(std::shared_ptr<Player> player) { lastMod = player->getName(); }

void Object::setDelay(int newDelay) { delay = newDelay; }
void Object::setExtra(int x) { extra = x; }
void Object::setWeight(short w) { weight = w; }
void Object::setBulk(short b) { bulk = std::max<short>(0, b); }
void Object::setMaxbulk(short b) { maxbulk = b; }
void Object::setSize(Size s) { size = s; }
void Object::setType(ObjectType t) { type = t; }
void Object::setWearflag(short w) { wearflag = w; }
void Object::setArmor(short a) { armor = std::max<short>(0, std::min<short>(a, 1000)); }
void Object::setQuality(short q) { quality = q; }
void Object::setAdjustment(short a) {
    removeFromSet();
    adjustment = std::max<short>(-127, std::min<short>(a, 127));
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
void Object::setEffect(std::string_view e) { effect = e; }
void Object::setEffectDuration(long d) { effectDuration = std::max<long>(-1, std::min<long>(d, EFFECT_MAX_DURATION)); }
void Object::setEffectStrength(short s) { effectStrength = std::max<long>(0, std::min<long>(s, EFFECT_MAX_STRENGTH)); }
void Object::setCoinCost(unsigned long c) { coinCost = c; }

void Object::setShopValue(unsigned long v) {
    removeFromSet();
    shopValue = std::min<unsigned long>(200000000, v);
    addToSet();
}

void Object::setLotteryCycle(int c) { lotteryCycle = c; }
void Object::setLotteryNumbers(short i, short n) { lotteryNumbers[i] = n; }
void Object::setRecipe(int r) { recipe = r; }
void Object::setMaterial(Material m) { material = m; }
void Object::setQuestOwner(const std::shared_ptr<Player> player) { questOwner = player->getName(); }

void Object::setLabel(const Player* player, std::string text) {
    label.playerId = player->getId();
    label.label = text;
}

void Object::clearEffect() {
    setEffect("");
    setEffectDuration(0);
    setEffectStrength(0);
}
bool Object::flagIsSet(int flag) const {
    return flags.test(flag);
}

void Object::setFlag(int flag) {
    flags.set(flag);
}

void Object::clearFlag(int flag) {
    flags.reset(flag);
}

bool Object::toggleFlag(int flag) {
    flags.flip(flag);
    return(flagIsSet(flag));
}

const std::string & Object::getVersion() const {
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

bool Object::isLabeledBy(const Creature* creature) const {
    return creature->getId() == label.playerId;
}

bool Object::isLabelMatch(std::string str) const {
    return keyTxtCompare(label.label.c_str(), str.c_str(), str.length());
}