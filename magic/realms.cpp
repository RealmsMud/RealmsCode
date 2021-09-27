/*
 * realms.cpp
 *   Code relating to realms (ie, cold, fire, water, etc).
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
#include <string>         // for operator==, basic_string

#include "bstring.hpp"    // for bstring, operator+
#include "cmd.hpp"        // for cmd
#include "creatures.hpp"  // for Creature, Player, Monster
#include "flags.hpp"      // for R_AIR_BONUS, R_COLD_BONUS, R_EARTH_BONUS
#include "global.hpp"     // for CastType, CastType::CAST
#include "magic.hpp"      // for SpellData, checkRefusingMagic, realmSkill
#include "mud.hpp"        // for ospell
#include "proto.hpp"      // for broadcast, up, getOffensiveSpell, getOpposi...
#include "random.hpp"     // for Random
#include "realm.hpp"      // for Realm, COLD, EARTH, ELEC, FIRE, WATER, WIND
#include "rooms.hpp"      // for BaseRoom

//
// The get functions essentially store info about relationships between
// flags and realms.
//

//*********************************************************************
//                      getRandomRealm
//*********************************************************************

int getRandomRealm() {
    return(Random::get<int>(EARTH,COLD));
}

//*********************************************************************
//                      getOffensiveSpell
//*********************************************************************

int getOffensiveSpell(Realm realm, int level) {
    return(ospell[level * 6 + realm - 1].splno);
}

//*********************************************************************
//                      realmResistPet
//*********************************************************************
bool Creature::checkResistPet(Creature *pet, bool& resistPet, bool& immunePet, bool& vulnPet) {
    if(!pet->isPet())
        return(false);
    bstring realm = getRealmSpellName(pet->getAsConstMonster()->getBaseRealm());
    resistPet = isEffected("resist-" + realm);
    immunePet = isEffected("immune-" + realm);
    vulnPet = isEffected("vuln-" + realm);
    return(true);
}

//*********************************************************************
//                      getRealmRoomBonusFlag
//*********************************************************************

int getRealmRoomBonusFlag(Realm realm) {
    if(realm==EARTH) return(R_EARTH_BONUS);
    if(realm==WIND) return(R_AIR_BONUS);

    if(realm==WATER) return(R_WATER_BONUS);
    if(realm==ELEC) return(R_ELEC_BONUS);

    if(realm==FIRE) return(R_FIRE_BONUS);
    if(realm==COLD) return(R_COLD_BONUS);
    return(-1);
}


//*********************************************************************
//                      getRealmSpellName
//*********************************************************************

bstring getRealmSpellName(Realm realm) {
    if(realm==EARTH) return("earth");
    else if(realm==WIND) return("air");
    else if(realm==WATER) return("water");
    else if(realm==ELEC) return("electric");
    else if(realm==FIRE) return("fire");
    else if(realm==COLD) return("cold");
    else
        return("");
}

//*********************************************************************
//                      getOppositeRealm
//*********************************************************************

Realm getOppositeRealm(Realm realm) {
    if(realm==EARTH) return(WIND);
    if(realm==WIND) return(EARTH);

    if(realm==WATER) return(ELEC);
    if(realm==ELEC) return(WATER);

    if(realm==FIRE) return(COLD);
    if(realm==COLD) return(FIRE);

    return(NO_REALM);
}

//
// Now we're down to the functions that use the realms.
//

//*********************************************************************
//                      checkRealmResist
//*********************************************************************

unsigned int Creature::checkRealmResist(unsigned int dmg, Realm pRealm) const {
    bstring resistEffect = "resist-" + getRealmSpellName(pRealm);
    if(isEffected(resistEffect))
        dmg /= 2;

    bstring immuneEffect = "immune-" + getRealmSpellName(pRealm);
    if(isEffected(immuneEffect))
        dmg = 1;

    bstring vulnEffect = "vuln-" + getRealmSpellName(pRealm);
    if(isEffected(vulnEffect))
        dmg = dmg + (dmg / 2);

    return(dmg);
}

bool Player::checkOppositeResistSpell(std::string_view effect) {
    if(effect == "resist-cold" )
        removeEffect("resist-fire");
    else if(effect == "resist-fire" )
        removeEffect("resist-cold");
    else if(effect == "resist-earth" )
        removeEffect("resist-air");
    else if(effect == "resist-air" )
        removeEffect("resist-earth");
    else if(effect == "resist-water" )
        removeEffect("resist-electric");
    else if(effect == "resist-electric" )
        removeEffect("resist-water");
    else
        return(false);

    return(true);
}


//*********************************************************************
//                      genericResist
//*********************************************************************
// This function replaces the code for the resist element functions

int genericResist(Creature* player, cmd* cmnd, SpellData* spellData, Realm realm) {
    Creature* target=nullptr;
    Player* pTarget=nullptr;

    //int       lt = getRealmSpellLT(realm);
    bstring name = getRealmSpellName(realm);

    bstring effect = "resist-" + name;

    if(cmnd->num == 2) {
        target = player;
        pTarget = player->getAsPlayer();
        broadcast(player->getSock(), player->getParent(), "%M casts resist-%s.", player, name.c_str());

        if(spellData->how == CastType::CAST) {
            player->print("You cast a resist-%s spell.\n", name.c_str());
        }
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findPlayer(player, cmnd, 2);

        if(!target || !target->getAsPlayer()) {
            player->print("You don't see that player here.\n");
            return(0);
        }

        if(checkRefusingMagic(player, target))
            return(0);

        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a resist-%s spell on %N.", player, name.c_str(), target);
        target->print("%M casts resist-%s on you.\n", player, name.c_str());

        if(spellData->how == CastType::CAST) {
            player->print("You cast a resist-%s spell on %N.\n", name.c_str(), target);

            if(player->getRoomParent()->magicBonus()) {
                player->print("The room's magical properties increase the power of your spell.\n");
            }
        } else {
            player->print("%M resists %s.\n", target, name.c_str());
        }
    }

    if(target->hasPermEffect(effect))  {
        player->print("The spell didn't take hold.\n");
        return(0);
    }


    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus()) {
            player->print("The room's magical properties increase the power of your spell.\n");
        }
        target->addEffect(effect, -2, -2, player, true, player);

    } else {
        target->addEffect(effect);
    }
    if(pTarget)
        pTarget->checkOppositeResistSpell(effect);

    return(1);
}


//*********************************************************************
//                      splResistWater
//*********************************************************************

int splResistWater(Creature* player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, WATER));
}

//*********************************************************************
//                      splResistFire
//*********************************************************************

int splResistFire(Creature* player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, FIRE));
}

//*********************************************************************
//                      splResistLightning
//*********************************************************************

int splResistLightning(Creature* player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, ELEC));
}

//*********************************************************************
//                      splResistCold
//*********************************************************************

int splResistCold(Creature* player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, COLD));
}

//*********************************************************************
//                      splResistAir
//*********************************************************************

int splResistAir(Creature* player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, WIND));
}

//*********************************************************************
//                      splResistEarth
//*********************************************************************

int splResistEarth(Creature* player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, EARTH));
}

//*********************************************************************
//                      realmSkill
//*********************************************************************

bstring realmSkill(Realm realm) {
    switch(realm) {
    case FIRE:
        return("fire");
    case WATER:
        return("water");
    case EARTH:
        return("earth");
    case WIND:
        return("air");
    case COLD:
        return("cold");
    case ELEC:
        return("electric");
    default:
        return("");
    }
}
