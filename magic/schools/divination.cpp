/*
 * divination.cpp
 *   Divination spells
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

#include <memory>                    // for allocator

#include "cmd.hpp"                   // for cmd
#include "flags.hpp"                 // for P_SLEEPING, R_LIMBO, R_NO_CLAIR_...
#include "global.hpp"                // for CastType, CreatureClass, CastTyp...
#include "magic.hpp"                 // for splGeneric, SpellData, splClairv...
#include "move.hpp"                  // for tooFarAway
#include "mudObjects/container.hpp"  // for Container
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for bonus, broadcast, display_rom
#include "random.hpp"                // for Random
#include "server.hpp"                // for Server, gServer
#include "stats.hpp"                 // for Stat


//*********************************************************************
//                      splTrueSight
//*********************************************************************
// This function allows players to detect misted vampires.

int splTrueSight(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "true-sight", "true-sight"));
}

//*********************************************************************
//                      splFarsight
//*********************************************************************
// allows target to see further in the overland map

int splFarsight(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "farsight", "farsight"));
}

//*********************************************************************
//                      splDetectMagic
//*********************************************************************
// This function allows players to cast the detect-magic spell which
// allows the spell-castee to see magic items.

int splDetectMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "detect-magic", "detect-magic"));
}

//*********************************************************************
//                      splDetectInvisibility
//*********************************************************************
// This function allows players to cast the detect-invisibility spell which
// allows the target to see invisible items, monsters, and exits.

int splDetectInvisibility(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "detect-invisibility", "detect-invisible"));
}

//*********************************************************************
//                      splKnowAura
//*********************************************************************
// This spell allows the caster to determine what alignment another
// target or player is by looking at it.

int splKnowAura(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "know-aura", "know-aura"));
}

//*********************************************************************
//                      splFortune
//*********************************************************************
// This allows bards to tell the luck of a given player.

int splFortune(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;
    int     luk;

    std::shared_ptr<Player> pPlayer = player->getAsPlayer();

    if(player->getClass() !=  CreatureClass::BARD && !player->isCt()) {
        player->print("Only bards may cast that spell.\n");
        return(0);
    }

    if(cmnd->num == 2) {


        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Fortune spell cast on yourself.\n");
            luk = pPlayer->getLuck() / 10;
            luk = std::max(luk, 1);
            switch(luk) {
            case 1:
                player->print("Your death will be tragic.\n.");
                break;
            case 2:
                player->print("A black cat must have crossed your path.\n");
                break;
            case 3:
                player->print("If it weren't for bad luck you'd have no luck at all.\n");
                break;
            case 4:
                player->print("Your karma is imbalanced.\n");
                break;
            case 5:
                player->print("Your future is uncertain.\n");
                break;
            case 6:
                player->print("Without intervention you may find yourself in a dire situation.\n");
                break;
            case 7:
                player->print("Long range prospects look good.\n");
                break;
            case 8:
                player->print("Count your blessings, for others are less fortunate.\n");
                break;
            case 9:
                player->print("The fates have smiled upon you.\n");
                break;
            case 10:
                player->print("Your death would be tragic.\n");
                break;
            default:
                player->print("You can't tell right now.\n");
            }

            broadcast(player->getSock(), player->getParent(), "%M reads %s fortune.", player.get(), player->hisHer());
        } else if(spellData->how == CastType::POTION)
            player->print("Nothing happens.\n");


    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);


        if(!target) {
            player->print("That's not here.\n");
            return(0);
        }

        if(target->isMonster())
            luk = target->getAlignment() / 10;
        else
            luk = target->getAsPlayer()->getLuck() / 10;

        luk = std::max(luk, 1);
        player->print("Fortune spell cast on %N.\n", target.get());

        switch (luk) {
        case 1:
            player->print("%M's death will be swift and certain.\n", target.get());
            break;
        case 2:
            player->print("You sense %N's karma is imbalanced.\n", target.get());
            break;
        case 3:
            player->print("A black cat must have crossed %N's path.\n", target.get());
            break;
        case 4:
            player->print("%M's aura reeks of danger.\n", target.get());
            break;
        case 5:
            player->print("Without intervention %N may end up in a dire situation.\n", target.get());
            break;
        case 6:
            player->print("%M's future is uncertain.\n", target.get());
            break;
        case 7:
            player->print("Long range prospects look good for %N.\n", target.get());
            break;
        case 8:
            player->print("%M should count their blessings, as others are less fortunate.\n", target.get());
            break;
        case 9:
            player->print("The fates smile upon %N.\n", target.get());
            break;
        case 10:
            player->print("%M's death will be tragic and unexpected.\n", target.get());
            break;
        default:
            player->print("You can't tell right now.\n");
        }
        broadcast(player->getSock(), player->getParent(), "%M checks %N's fortune.", player.get(), target.get());

    }
    return(1);
}

//*********************************************************************
//                      splClairvoyance
//*********************************************************************

int splClairvoyance(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Player> pPlayer = player->getAsPlayer();
    if(!pPlayer)
        return(0);

    std::shared_ptr<Player> target=nullptr;
    int     chance=0;

    if(pPlayer->getClass() == CreatureClass::BUILDER) {
        *pPlayer << "You cannot cast this spell.\n";
        return(0);
    }

    if(cmnd->num < 3) {
        *pPlayer << "Cast clairvoyance on whom?\n";
        return(0);
    }

    lowercize(cmnd->str[2], 1);
    target = gServer->findPlayer(cmnd->str[2]);

    if(!target || pPlayer == target || !pPlayer->canSee(target)) {
        *pPlayer << "That player is not logged on.\n";
        return(0);
    }

    if(Move::tooFarAway(pPlayer, target, "clair"))
        return(0);

    broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%M casts clairvoyance.", pPlayer.get());
    if(spellData->how == CastType::CAST)
        *pPlayer << ColorOn << "^yYou attempt to scry " << target << "'s current location.\n" << ColorOff;

   //Compute chance of success
    chance = 500 + (spellData->level - target->getLevel()) * 50;
    chance += (pPlayer->intelligence.getCur() - target->intelligence.getCur()) * 3;
    chance += ((pPlayer->getClass() == CreatureClass::MAGE) || (pPlayer->getClass() == CreatureClass::LICH)) ? 100 : 0;
    //Chance of success is max 90%
    chance = std::min(900, chance);

    // The "non-detection" effect reduces chances of being scried on.
    // TODO: put in difference calculations based on caster vs target's class and divination vs abjuration magical skill levels
    // For now, chance is just reduced by 60%
    // Only print output if not a staff member under DM_INVIS or incognito, otherwise could exploit to see if staff is on
    if (target->isEffected("non-detection") && !(target->isStaff() && (target->flagIsSet(P_DM_INVIS) || target->isEffected("incognito")))) {
        *pPlayer << ColorOn << "^y" << setf(CAP) << target << "'s shielding from divination magic is making it difficult.\n" << ColorOff;
        chance = (chance*4)/10;
    }

    //Attempts to scry staff always fail
    if(target->isStaff())
        chance = 0;

    if( target->getRoomParent()->flagIsSet(R_NO_CLAIR_ROOM) ||
        target->getRoomParent()->flagIsSet(R_LIMBO) ||
        target->getRoomParent()->flagIsSet(R_VAMPIRE_COVEN) ||
        target->getRoomParent()->isConstruction() ||
        target->getRoomParent()->flagIsSet(R_SHOP_STORAGE)
    )
        chance = 0;


    if(pPlayer->isStaff() || Random::get(1, 1000) <= chance) {
        //chance to detect the scrying
        chance = 500 + (((int)target->getLevel() - (int)spellData->level) * 50);
        chance += (target->intelligence.getCur() - pPlayer->intelligence.getCur()) * 3;
        chance += ((target->getClass() == CreatureClass::MAGE) || (target->getClass() == CreatureClass::LICH)) ? 100 : 0;
        chance = std::min(900, chance);

        if(!pPlayer->isStaff()) {
            if(!target->chkSave(MEN, pPlayer, 0)) {
                display_rom(target, pPlayer);
                if(Random::get(1, 1000) <= chance) {
                    // display a different string if the target can't see
                    if(target->flagIsSet(P_SLEEPING) || target->isBlind()) {
                        *target << ColorOn << "^y" << setf(CAP) << pPlayer << " temporarily sees your surroundings.\n" << ColorOff;
                    } else {
                        *target << ColorOn << "^y" << setf(CAP) << pPlayer << " temporarily sees through your eyes.\n" << ColorOff;
                    }

                }
            } else {
                *pPlayer << ColorOn << "^yYou failed to scry " << target << "'s location.\n" << ColorOff;
                if(Random::get(1, 1000) <= chance)
                    *target << ColorOn << "^y" << setf(CAP) << pPlayer << " tried to scry your location.\n" << ColorOff;
            }
        } else {
            display_rom(target, pPlayer);
        }

    } else {
        *pPlayer << "Your scrying attempt failed.\n";

        chance = 500 + ((int)target->getLevel() - (int)spellData->level) * 50;
        chance += (target->intelligence.getCur() - pPlayer->intelligence.getCur()) * 3;
        chance += ((target->getClass() == CreatureClass::MAGE) || (target->getClass() == CreatureClass::LICH)) ? 100 : 0;

        if(!pPlayer->isStaff() && Random::get(1, 1000) <= chance)
            *target << ColorOn << "^y" << setf(CAP) << pPlayer << " attempts to scry your location.\n" << ColorOff;
    }

    return(1);
}

//*********************************************************************
//                      splComprehendLanguages
//*********************************************************************

int splComprehendLanguages(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    if(spellData->how == CastType::CAST && !player->isStaff() && 
                    player->getClass() != CreatureClass::MAGE && 
                    player->getClass() != CreatureClass::LICH && 
                    player->getClass() != CreatureClass::BARD &&
                    player->getClass() != CreatureClass::DRUID &&
                    player->getClass() != CreatureClass::CLERIC) 
    {
        player->print("Only bards, clerics, druids, liches, and mages may cast that spell.\n");
        return(0);
    }

    return(splGeneric(player, cmnd, spellData, "a", "comprehend-languages", "comprehend-languages"));
}

//*********************************************************************
//                      splTongues
//*********************************************************************

int splTongues(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    if(spellData->how == CastType::CAST && player->getClass() !=  CreatureClass::MAGE && player->getClass() !=  CreatureClass::BARD && !player->isStaff()) {
        player->print("Only mages and bards may cast that spell.\n");
        return(0);
    }

    return(splGeneric(player, cmnd, spellData, "a", "tongues", "tongues"));
}

//*********************************************************************
//                      splDetectHidden
//*********************************************************************

int splDetectHidden(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    if(!player->isPlayer())
        return(0);

    if(spellData->how == CastType::CAST) {
        if(!player->isMageLich())
            return(0);
        player->print("You cast a detect-hidden spell.\n");
        broadcast(player->getSock(), player->getParent(), "%M casts a detect-hidden spell.", player.get());
    }

    std::shared_ptr<Player> viewer = player->getAsPlayer();
    display_rom(viewer, viewer, spellData->level);
    return(1);
}
