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
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "creatures.h"
#include "mud.h"
#include "rooms.h"

#define NUM_RESISTS_ALLOWED     2

//
// The get functions essentially store info about relationships between
// flags and realms.
//


//*********************************************************************
//                      getRandomRealm
//*********************************************************************

int getRandomRealm() {
    return(mrand(EARTH,COLD));
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

int Creature::checkRealmResist(int dmg, Realm realm) const {
    bstring resistEffect = "resist-" + getRealmSpellName(realm);
    if(isEffected(resistEffect))
        dmg /= 2;

    bstring immuneEffect = "immune-" + getRealmSpellName(realm);
    if(isEffected(immuneEffect))
        dmg = 1;

    bstring vulnEffect = "vuln-" + getRealmSpellName(realm);
    if(isEffected(vulnEffect))
        dmg = dmg + (dmg / 2);

    return(dmg);
}

//*********************************************************************
//                      checkNumResistSpells
//*********************************************************************

void Player::checkNumResistSpells() {
// TODO: Redo
//  Realm realm=0, i=0;
//  int num = countResistSpells();
//  bstring str = "";
//
//  if(num <= NUM_RESISTS_ALLOWED)
//      return;
//
//  // they have too many resist spells; make the ones with the shortest
//  // remaining duration wear off
//  while(num > NUM_RESISTS_ALLOWED) {
//
//      realm = WATER;
//
//      for(i=1; i<MAX_REALM; i++)
//          if(flagIsSet(getRealmPlayerResistFlag(i)) &&
//              ( !lasttime[getRealmSpellLT(realm)].ltime ||
//                lasttime[getRealmSpellLT(i)].ltime <= lasttime[getRealmSpellLT(realm)].ltime) )
//                realm = i;
//
//      clearFlag(getRealmPlayerResistFlag(realm));
//      lasttime[getRealmSpellLT(realm)].ltime = 0;
//
//      player->print("Your resist-%s spell wears off.\n", getRealmSpellName(realm).c_str());
//      num--;
//  }
}


bool Player::checkOppositeResistSpell(bstring effect) {
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
    Creature* target=0;
    Player* pTarget=0;

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
