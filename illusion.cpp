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
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "effects.h"
#include "commands.h"


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

int splIllusion(Creature* player, cmd* cmnd, SpellData* spellData) {
    bstring txt = "";
    Player* pPlayer=0, *target=0;
    const RaceData* race=0;

    if(!player->isPlayer())
        return(0);
    pPlayer = player->getAsPlayer();

    if(spellData->how == CastType::CAST) {
        // if the spell was cast
        if(!pPlayer->isMageLich())
            return(0);

        txt = getFullstrText(cmnd->fullstr, cmnd->num == 3 ? 2 : 3);

        if(txt == "") {
            pPlayer->print("Illusion whom to what race?\n");
            pPlayer->print("Syntax: cast illusion [target] <race>\n");
            return(0);
        }

        race = gConfig->getRace(txt);

        if(!race) {
            pPlayer->print("Race not understood or not unique.\n");
            pPlayer->print("Syntax: cast illusion [target] <race>\n");
            return(0);
        }
    } else if(spellData->object) {
        // if we're using an object to get data
        if(spellData->object->getExtra() > 0 && spellData->object->getExtra() < RACE_COUNT)
            race = gConfig->getRace(spellData->object->getExtra());

        if(!race) {
            pPlayer->printColor("%O is doesn't taste quite right.\n", spellData->object);
            return(0);
        }
    }


    if(cmnd->num == 3 || (cmnd->num == 2 && spellData->how == CastType::POTION)) {
        target = pPlayer;

        if(spellData->how != CastType::POTION) {
            pPlayer->print("You cast an illusion spell.\n");
            broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%M casts an illusion spell.", pPlayer);
        }
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = pPlayer->getParent()->findPlayer(pPlayer, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            pPlayer->print("You don't see that player here.\n");
            return(0);
        }

        if(checkRefusingMagic(player, target))
            return(0);

        broadcast(pPlayer->getSock(), target->getSock(), pPlayer->getRoomParent(), "%M casts an illusion spell on %N.",
            pPlayer, target);
        target->print("%M casts illusion on you.\n", pPlayer);
        pPlayer->print("You cast an illusion spell on %N.\n", target);
    }

    if(pPlayer->getRoomParent()->magicBonus())
        pPlayer->print("The room's magical properties increase the power of your spell.\n");

    if(target->isEffected("illusion"))
        target->removeEffect("illusion", false);
    target->addEffect("illusion", -2, -2, pPlayer, true, player);


    EffectInfo* effect = target->getEffect("illusion");
    effect->setExtra(race->getId());
    effect->setStrength(pPlayer->getLevel());

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
    Exit *exit=0;
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
