/*
 * illusion.cpp
 *   Illusion spells
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

#include "bstring.hpp"    // for bstring
#include "cmd.hpp"        // for cmd
#include "commands.hpp"   // for getFullstrText
#include "config.hpp"     // for Config, gConfig
#include "creatures.hpp"  // for Creature, Player
#include "effects.hpp"    // for EffectInfo
#include "exits.hpp"      // for Exit
#include "flags.hpp"      // for X_CONCEALED
#include "global.hpp"     // for CastType, CastType::CAST, CastType::POTION
#include "magic.hpp"      // for SpellData, splGeneric, checkRefusingMagic
#include "monType.hpp"    // for noLivingVulnerabilities
#include "objects.hpp"    // for Object
#include "proto.hpp"      // for broadcast, findExit, up
#include "raceData.hpp"   // for RaceData
#include "rooms.hpp"      // for BaseRoom



//*********************************************************************
//                      splInvisibility
//*********************************************************************
// This function allows a player to cast an invisibility spell on themself
// or on another player.

int splInvisibility(Creature* player, cmd* cmnd, SpellData* spellData) {
    if(!player->isStaff()) {
        if(player->inCombat()) {
            player->print("Not in the middle of combat.\n");
            return(0);
        }
    }

    return(splGeneric(player, cmnd, spellData, "an", "invisibility", "invisibility"));
}

//*********************************************************************
//                      splInvisibility
//*********************************************************************
// This function allows a player to cast an invisibility spell on themself
// or on another player.

int splGreaterInvisibility(Creature* player, cmd* cmnd, SpellData* spellData) {
    if(!player->isStaff()) {
        if(!player->isMageLich())
            return(0);
        if(player->getLevel() < 20 && spellData->how == CastType::CAST) {
            player->print("You are not experienced enough to cast that spell.\n");
            return(0);
        }
        if(player->inCombat()) {
            player->print("Not in the middle of combat.\n");
            return(0);
        }
    }

    return(splGeneric(player, cmnd, spellData, "a", "greater invisibility", "greater-invisibility"));
}

bool Creature::isInvisible() const {
    return(isEffected("invisibility") || isEffected("greater-invisibility"));
}

//*********************************************************************
//                      splCamouflage
//*********************************************************************
// This function allows players to cast camouflage on one another or
// themselves. Camouflage lessens the chance of ranger track spell.

int splCamouflage(Creature* player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "camouflage", "camouflage"));
}

//*********************************************************************
//                      splIllusion
//*********************************************************************

int splIllusion(Creature* creature, cmd* cmnd, SpellData* spellData) {
    bstring txt = "";
    Player* pPlayer=nullptr, *target=nullptr;
    const RaceData* race=nullptr;

    pPlayer = creature->getAsPlayer();

    if(spellData->how == CastType::CAST) {
        // if the spell was cast
        if(pPlayer && !pPlayer->isMageLich())
            return(0);

        txt = getFullstrText(cmnd->fullstr, cmnd->num == 3 ? 2 : 3);

        if(txt.empty()) {
            creature->print("Illusion whom to what race?\n");
            creature->print("Syntax: cast illusion [target] <race>\n");
            return(0);
        }

        race = gConfig->getRace(txt);

        if(!race) {
            creature->print("Race not understood or not unique.\n");
            creature->print("Syntax: cast illusion [target] <race>\n");
            return(0);
        }
    } else if(spellData->object) {
        // if we're using an object to get data
        if(spellData->object->getExtra() > 0 && spellData->object->getExtra() < RACE_COUNT)
            race = gConfig->getRace(spellData->object->getExtra());

        if(!race) {
            creature->printColor("%O is doesn't taste quite right.\n", spellData->object);
            return(0);
        }
    }


    if(cmnd->num == 3 || (cmnd->num == 2 && spellData->how == CastType::POTION)) {
        target = pPlayer;

        if(spellData->how != CastType::POTION) {
            creature->print("You cast an illusion spell.\n");
            broadcast(pPlayer ? pPlayer->getSock() : nullptr, creature->getRoomParent(), "%M casts an illusion spell.", creature);
        }
    } else {
        if(creature->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = creature->getParent()->findPlayer(creature, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            creature->print("You don't see that creature here.\n");
            return(0);
        }

        if(pPlayer && checkRefusingMagic(creature, target))
            return(0);

        broadcast(pPlayer ? pPlayer->getSock() : nullptr, target->getSock(), creature->getRoomParent(), "%M casts an illusion spell on %N.",
                creature, target);
        target->print("%M casts illusion on you.\n", creature);
        creature->print("You cast an illusion spell on %N.\n", target);
    }

    if(creature->getRoomParent()->magicBonus())
        creature->print("The room's magical properties increase the power of your spell.\n");

    if(target->isEffected("illusion"))
        target->removeEffect("illusion", false);
    target->addEffect("illusion", -2, -2, creature, true, creature);


    EffectInfo* effect = target->getEffect("illusion");
    effect->setExtra(race->getId());
    effect->setStrength(creature->getLevel());

    return(1);
}

//*********************************************************************
//                      willIgnoreIllusion
//*********************************************************************

bool Creature::willIgnoreIllusion() const {
    if(isStaff())
        return(true);
    if(isUndead() || monType::noLivingVulnerabilities(type))
        return(true);
    if(isEffected("true-sight"))
        return(true);
    return(false);
}

//*********************************************************************
//                      splBlur
//*********************************************************************

int splBlur(Creature* player, cmd* cmnd, SpellData* spellData) {
    // this number is the % miss chance
    int strength = 7;
    if(spellData->how == CastType::CAST)
        strength += player->getLevel()/10;
    return(splGeneric(player, cmnd, spellData, "a", "blur", "blur", strength));
}


//*********************************************************************
//                      splIllusoryWall
//*********************************************************************

int splIllusoryWall(Creature* player, cmd* cmnd, SpellData* spellData) {
    Exit *exit=nullptr;
    int strength = spellData->level;
    long duration = 300;

    if(player->noPotion( spellData))
        return(0);

    if(cmnd->num > 2)
        exit = findExit(player, cmnd, 2);
    if(!exit) {
        player->print("Cast an illusory wall spell on which exit?\n");
        return(0);
    }

    player->printColor("You cast an illusory wall spell on the %s^x.\n", exit->getCName());
    broadcast(player->getSock(), player->getParent(), "%M casts an illusory wall spell on the %s^x.",
        player, exit->getCName());

    if(exit->flagIsSet(X_CONCEALED) || exit->hasPermEffect("illusory-wall")) {
        player->print("The spell didn't take hold.\n");
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus())
            player->print("The room's magical properties increase the power of your spell.\n");
    }

    exit->addEffect("illusory-wall", duration, strength, player, true, player);
    return(1);
}
