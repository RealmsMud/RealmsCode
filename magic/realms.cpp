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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <string>                    // for allocator, string, char_traits
#include <string_view>               // for operator==, basic_string_view

#include "cmd.hpp"                   // for cmd
#include "flags.hpp"                 // for R_AIR_BONUS, R_COLD_BONUS, R_EAR...
#include "global.hpp"                // for CastType, CastType::CAST
#include "magic.hpp"                 // for SpellData, checkRefusingMagic
#include "mud.hpp"                   // for ospell
#include "mudObjects/container.hpp"  // for Container
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, up, getOffensiveSpell
#include "random.hpp"                // for Random
#include "realm.hpp"                 // for Realm, COLD, EARTH, ELEC, FIRE
#include "structs.hpp"               // for osp_t

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
bool Creature::checkResistPet(const std::shared_ptr<Creature>&pet, bool& resistPet, bool& immunePet, bool& vulnPet) {
    if(!pet->isPet())
        return(false);
    std::string chkRealm = getRealmSpellName(pet->getAsConstMonster()->getBaseRealm());
    resistPet = isEffected("resist-" + chkRealm);
    immunePet = isEffected("immune-" + chkRealm);
    vulnPet = isEffected("vuln-" + chkRealm);
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

std::string getRealmSpellName(Realm realm) {
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
    std::string resistEffect = "resist-" + getRealmSpellName(pRealm);
    if(isEffected(resistEffect))
        dmg /= 2;

    std::string immuneEffect = "immune-" + getRealmSpellName(pRealm);
    if(isEffected(immuneEffect))
        dmg = 1;

    std::string vulnEffect = "vuln-" + getRealmSpellName(pRealm);
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

int genericResist(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData, Realm realm) {
    std::shared_ptr<Creature> target=nullptr;
    std::shared_ptr<Player> pTarget=nullptr;

    //int       lt = getRealmSpellLT(realm);
    std::string name = getRealmSpellName(realm);

    std::string effect = "resist-" + name;

    if(cmnd->num == 2) {
        target = player;
        pTarget = player->getAsPlayer();
        broadcast(player->getSock(), player->getParent(), "%M casts resist-%s.", player.get(), name.c_str());

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

        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a resist-%s spell on %N.", player.get(), name.c_str(), target.get());
        target->print("%M casts resist-%s on you.\n", player.get(), name.c_str());

        if(spellData->how == CastType::CAST) {
            player->print("You cast a resist-%s spell on %N.\n", name.c_str(), target.get());

            if(player->getRoomParent()->magicBonus()) {
                player->print("The room's magical properties increase the power of your spell.\n");
            }
        } else {
            player->print("%M resists %s.\n", target.get(), name.c_str());
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

int splResistWater(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, WATER));
}

//*********************************************************************
//                      splResistFire
//*********************************************************************

int splResistFire(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, FIRE));
}

//*********************************************************************
//                      splResistLightning
//*********************************************************************

int splResistLightning(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, ELEC));
}

//*********************************************************************
//                      splResistCold
//*********************************************************************

int splResistCold(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, COLD));
}

//*********************************************************************
//                      splResistAir
//*********************************************************************

int splResistAir(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, WIND));
}

//*********************************************************************
//                      splResistEarth
//*********************************************************************

int splResistEarth(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(genericResist(player, cmnd, spellData, EARTH));
}

//*********************************************************************
//                      realmSkill
//*********************************************************************

std::string realmSkill(Realm realm) {
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
