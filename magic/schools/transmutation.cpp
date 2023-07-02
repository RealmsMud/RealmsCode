/*
 * transmutation.cpp
 *   Transmutation spells
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

#include <set>                       // for operator==, set<>::iterator, _Rb...
#include <string>                    // for allocator, string

#include "cmd.hpp"                   // for cmd
#include "effects.hpp"               // for EffectInfo
#include "flags.hpp"                 // for M_PERMENANT_MONSTER, X_PORTAL
#include "global.hpp"                // for CastType, CreatureClass, CastTyp...
#include "magic.hpp"                 // for SpellData, splGeneric, checkRefu...
#include "money.hpp"                 // for Money
#include "move.hpp"                  // for deletePortal
#include "mud.hpp"                   // for DL_SILENCE
#include "mudObjects/container.hpp"  // for Container, ObjectSet
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/exits.hpp"      // for Exit
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/objects.hpp"    // for Object
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, bonus, up, dec_daily
#include "random.hpp"                // for Random
#include "statistics.hpp"            // for Statistics
#include "stats.hpp"                 // for Stat
#include "structs.hpp"               // for saves, daily

//*********************************************************************
//                      splLevitate
//*********************************************************************
// This function allows players to cast the levitate spell, thus allowing
// them to levitate over traps or up mountain cliffs.

int splLevitate(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "levitation", "levitate"));
}

//*********************************************************************
//                      splEntangle
//*********************************************************************

int splEntangle(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int     bns=0, nohold=0, dur=0, statmod=0;
    std::shared_ptr<Creature> target=nullptr;


    if( player->getClass() !=  CreatureClass::DRUID &&
        player->getClass() !=  CreatureClass::RANGER &&
        !player->isCt() && spellData->how == CastType::CAST
    ) {
        player->print("You are unable to cast that spell.\n");
        return(0);
    }


    if(!player->getRoomParent()->isOutdoors() && !player->isCt()) {
        player->print("You cannot cast that spell indoors.\n");
        return(0);
    }



    // Cast entangle
    if(cmnd->num == 2) {
        if(spellData->how != CastType::POTION) {
            player->print("Entangle whom?\n");
            return(0);
        } else if(player->isEffected("mist")) {
            player->print("Nothing happens.\n");
            return(0);
        } else {

            player->print("Weeds and vines reach out to engangle you!\n");
            broadcast(player->getSock(), player->getParent(), "%M becomes entangled by vines and weeds!", player.get());

            player->unhide();
            player->addEffect("entangled", 0, 0, player, true, player);
            return(1);
        }

    // Cast entangle on another player
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);

        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], true, true);

        if(!target) {
            player->print("Cast on whom?\n");
            return(0);
        }


        if(player->isPlayer() && target->isPlayer() && target->inCombat(false) && !player->flagIsSet(P_OUTLAW)) {
            player->print("Not in the middle of combat.\n");
            return(0);
        }

        if(target->isPlayer() && !player->isCt() && target->isCt()) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        if(target->isPlayer() && target->isEffected("hold-person")) {
            player->printColor("%M is already held fast.\n^yYour spell fails.\n", target.get());
            return(0);
        }

        if(target->isPlayer() && !player->isDm() && target->isDm()) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }


        statmod = (bonus(target->strength.getCur()) + bonus(target->dexterity.getCur())) / 2;

        player->print("You cast an entangle spell on %N.\n", target.get());
        target->print("%M casts an entangle spell on you.\n", player.get());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts an entangle spell on %N.", player.get(), target.get());


        if( target->isPlayer() &&
            (   target->flagIsSet(P_UNCONSCIOUS) ||
                target->isEffected("petrification") ||
                (   player->getClass() !=  CreatureClass::RANGER &&
                    player->getClass() !=  CreatureClass::DRUID &&
                    target->isEffected("mist")
                )
            )
        ) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        if(target->isPlayer() && Random::get(1,100) <= 50 && target->isEffected("resist-magic")) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        if(spellData->how == CastType::CAST)
            bns = 5*((int)player->getLevel() - (int)target->getLevel()) + 2*crtWisdom(target) - 2*bonus(
                    player->intelligence.getCur());

        if(target->isMonster()) {
            if( target->flagIsSet(M_DM_FOLLOW) ||
                target->flagIsSet(M_PERMENANT_MONSTER) ||
                target->isEffected("resist-magic") ||
                target->flagIsSet(M_RESIST_STUN_SPELL) ||
                target->isEffected("reflect-magic")
            )
                nohold = 1;

            if( target->flagIsSet(M_LEVEL_BASED_STUN) &&
                (((int)target->getLevel() - (int)player->getLevel()) > ((player->getClass() == CreatureClass::LICH || player->getClass() == CreatureClass::MAGE) ? 6:4))
            )
                nohold = 1;
        }

        if(target->isMonster()) {
            player->smashInvis();
            if((!target->chkSave(SPL, player, bns) && !nohold) || player->isCt()) {

                if(spellData->how == CastType::CAST)
                    dur = Random::get(9,18) + 2*bonus(player->intelligence.getCur()) - statmod;
                else
                    dur = Random::get(9,12);

                target->stun(dur);


                player->print("%M is held in place by vines and weeds!\n", target.get());
                broadcast(player->getSock(), player->getParent(), "%M's spell entangles %N in place!", player.get(), target.get());


                if(player->isCt())
                    player->print("*DM* %d seconds.\n", dur);

                if(target->isMonster())
                    target->getAsMonster()->addEnemy(player);

            } else {
                player->print("%M resisted your spell.\n", target.get());
                broadcast(player->getSock(), target->getRoomParent(), "%M resisted %N's entangle spell.",target.get(), player.get());
                if(target->isMonster())
                    target->getAsMonster()->addEnemy(player);
            }

        } else {
            player->smashInvis();
            if(!target->chkSave(SPL, player, bns) || player->isCt()) {

                target->print("You are entangled in vines and weeds!\nYou are unable to move!\n");

                broadcast(target->getSock(), player->getRoomParent(), "%M becomes entangled in vines and weeds!", target.get());

                if(spellData->how == CastType::CAST)
                    dur = Random::get(12,18) + 2*bonus(player->intelligence.getCur()) - statmod;
                else
                    dur = Random::get(9,12);

                if(player->isCt())
                    player->print("*DM* %d seconds.\n", dur);

                target->addEffect("hold-person", 0, 0, player, true, player);
                //*********************************************************************
            } else {
                player->print("%M resisted your spell.\n", target.get());
                broadcast(player->getSock(), target->getSock(), target->getRoomParent(), "%M resisted %N's hold-person spell.",target.get(), player.get());
                target->print("%M tried to cast a hold-person spell on you.\n", player.get());
            }
        }
    }

    return(1);
}

//*********************************************************************
//                      splPassWithoutTrace
//*********************************************************************
// This function makes a target player able to pass through rooms without
// leaving a trace

int splPassWithoutTrace(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    if( spellData->how == CastType::CAST &&
        !player->isStaff() &&
        player->getClass() !=  CreatureClass::DRUID &&
        player->getClass() !=  CreatureClass::RANGER &&
        player->getDeity() != LINOTHAN
    ) {
        player->print("Only druids, rangers, and followers of Linothan may cast this spell.\n");
        return(0);
    }

    return(splGeneric(player, cmnd, spellData, "a", "pass-without-trace", "pass-without-trace"));
}

//*********************************************************************
//                      splFly
//*********************************************************************
// This function allows players to cast the fly spell. It will
// allow the player to fly to areas that are otherwise unreachable
// by foot.

int splFly(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "fly", "fly", std::max<int>(1,spellData->level/3)));
}

//*********************************************************************
//                      splInfravision
//*********************************************************************
// This spell allows a player to cast an infravision spell which will
// allow them to see in a dark/magically dark room

int splInfravision(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;

    if(cmnd->num == 2) {
        target = player;
        if(spellData->how == CastType::CAST && !player->checkMp(5))
            return(0);

        player->print("You cast an infravision spell.\n");
        broadcast(player->getSock(), player->getParent(), "%M casts an infravision spell.", player.get());

        if(spellData->how == CastType::CAST) {
            player->subMp(5);
            if(player->getRoomParent()->magicBonus())
                player->print("The room's magical properties increase the power of your spell.\n");
        }

    } else {
        if(player->noPotion( spellData))
            return(0);

        if(spellData->how == CastType::CAST && !player->checkMp(10))
            return(0);

        if( spellData->how == CastType::CAST &&
            player->getClass() !=  CreatureClass::MAGE &&
            player->getClass() !=  CreatureClass::LICH &&
            player->getClass() !=  CreatureClass::BARD &&
            player->getClass() !=  CreatureClass::CLERIC &&
            player->getClass() !=  CreatureClass::DRUID &&
            !player->isStaff()
        ) {
            player->print("Your class is unable to cast that spell upon others.\n");
            return(0);
        }

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print("You don't see that here.\n");
            return(0);
        }

        if(checkRefusingMagic(player, target))
            return(0);

        if(target->isMonster()) {
            player->print("%M doesn't need that spell right now.\n", target.get());
            return(0);
        }

        if(spellData->how == CastType::CAST) {
            player->subMp(10);
            if(player->getRoomParent()->magicBonus())
                player->print("The room's magical properties increase the power of your spell.\n");

        }

        player->print("You cast an infravision spell on %N.\n", target.get());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts an infravision spell on %N.", player.get(), target.get());
        target->print("%M casts an infravision spell on you.\n", player.get());

    }
    if(target->inCombat(false))
        player->smashInvis();

    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus())
            player->print("The room's magical properties increase the power of your spell.\n");
        target->addEffect("infravision", -2, -2, player, true, player);
    } else {
        target->addEffect("infravision");
    }

    return(1);
}

//*********************************************************************
//                      splKnock
//*********************************************************************

int splKnock(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Player> pPlayer = player->getAsPlayer();
    int         chance=0, canCast=0, mpNeeded=0;
    std::shared_ptr<Exit> exit=nullptr;

    if(spellData->how == CastType::CAST) {
        if( player->getClass() == CreatureClass::MAGE ||
            player->getClass() == CreatureClass::LICH ||
            player->getClass() == CreatureClass::BARD ||
            (!pPlayer || pPlayer->getSecondClass() == CreatureClass::MAGE) ||
            player->isStaff()
        )
            canCast=1;
        if(player->getClass() == CreatureClass::CLERIC && player->getDeity() == KAMIRA)
            canCast=1;
        if(!player->spellIsKnown(S_KNOCK)) {
            player->print("You do not know that spell.\n");
            return(0);
        }
        if(!canCast) {
            player->print("You cannot cast that spell.\n");
            return(0);
        }
        if(cmnd->num < 3) {
            player->print("Syntax: cast knock <exit>\n");
            return(0);
        }
    } else {
        if(cmnd->num < 3) {
            player->print("Syntax: use <object> <exit>\n");
            return(0);
        }
    }

    //if(!canCast && spellData->how != CastType::CAST) {
    //  player->print("The magic will not work for you.\n");
    //  return(0);
    //}


    exit = findExit(player, cmnd, 2);

    if(!exit || !exit->flagIsSet(X_LOCKED)) {
        player->print("You don't see that locked exit here.\n");
        return(0);
    }

    const std::shared_ptr<Monster>  guard = player->getRoomParent()->getGuardingExit(exit, player->getAsConstPlayer());
    if(guard && !player->checkStaff("%M %s %s that.\n", guard.get(), guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you" : "won't let you", spellData->how == CastType::CAST ? "cast" : "do"))
        return(0);


    if(exit->getLevel() > player->getLevel()) {
        player->print("You aren't powerful enough to magically unlock that exit.\n");
        return(0);
    }
    if(exit->flagIsSet(X_UNPICKABLE) || exit->flagIsSet(X_PORTAL)) {
        player->print("The spell fizzles.\n");
        return(0);
    }

    if(exit->flagIsSet(X_NO_KNOCK_SPELL)) {
        player->print("The %s glows brightly, resisting your magic.\n", exit->getCName());
        broadcast(player->getSock(), player->getParent(), "The %s glows brightly, resisting %M's magical knock.^x",exit->getCName(), player.get());
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        mpNeeded = 10 + exit->getLevel();
        if( pPlayer &&
            pPlayer->getClass() !=  CreatureClass::MAGE &&
            pPlayer->getSecondClass() != CreatureClass::MAGE &&
            pPlayer->getClass() !=  CreatureClass::LICH &&
            !pPlayer->isStaff()
        )
            mpNeeded *=2;

        if(player->getClass() !=  CreatureClass::LICH) {
            if(player->mp.getCur() < mpNeeded) {
                player->print("You need %d magic points to cast that spell.\n", mpNeeded);
                return(0);
            }
            player->mp.decrease(mpNeeded);
        } else {
            if(player->hp.getCur() < mpNeeded) {
                player->print("You need %d hit points to cast that spell.\n", mpNeeded);
                return(0);
            }
            player->hp.decrease(mpNeeded);
        }
    }

    if(player->getClass() == CreatureClass::CLERIC && player->getDeity() == KAMIRA)
        chance = 10*((int)player->getLevel() - exit->getLevel()) + (2*bonus(player->piety.getCur()));
    else
        chance = 10*((int)player->getLevel() - exit->getLevel()) + (2*bonus(player->intelligence.getCur()));

    if(spellData->how == CastType::CAST)
        player->printColor("You cast a knock spell on the \"%s^x\" exit.\n", exit->getCName());
    broadcast(player->getSock(), player->getParent(), "%M casts a knock spell at the %s^x.", player.get(), exit->getCName());


    if(player->isStaff() || Random::get(1,100) <= chance) {
        player->print("Click!");
        if(spellData->how == CastType::CAST)
            player->print(" Your knock spell was successful!");
        player->print("\n");
        broadcast(player->getSock(), player->getParent(), "%M's knock spell was successful.", player.get());
        exit->clearFlag(X_LOCKED);
    } else {
        player->printColor("^yYour spell failed.\n");
        broadcast(player->getSock(), player->getParent(), "%M's knock spell failed.", player.get());
    }

    return(1);
}

//*********************************************************************
//                      splDisintegrate
//*********************************************************************

int splDisintegrate(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature>target=nullptr;
    int saved=0, bns=0;

    if(spellData->how == CastType::CAST && !player->isMageLich()) {
        *player << "The arcane nature of that spell eludes you.\n";
        return(0);
    }

    if(player->getLevel() < 13) {
        *player <<"You are not powerful enough to cast that spell.\n";
        return(0);
    }


    if(cmnd->num < 2) {
        if(spellData->how == CastType::CAST) {
            *player << "Casting a disintegrate spell on yourself would not be very smart.\n";
            return(0);
        } else if(spellData->how == CastType::WAND) {
            *player << "Nothing happens.\n";
            return(0);
        } else {
            if(!player->chkSave(DEA, player, 30+(50-player->getLevel()))) {
                *player << ColorOn << "^GYou are engulfed in an eerie green light.\nYou've been disintegrated!\n" << ColorOff;
                return(0);
            } else {
                *player << "Ewww! That tastes HORRIBLE.\nYou vomit uncontrollably.";
                player->stun(Random::get(20,30));
                return(0);
            }
        }
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target || player == target) {
            // disintegrate can destroy walls of force
            cmnd->str[2][0] = low(cmnd->str[2][0]);
            std::shared_ptr<Exit> exit = findExit(player, cmnd, 2);
            EffectInfo* effect=nullptr;

            if(exit) {
                effect = exit->getEffect("wall-of-force");
                *player << ColorOn << "You cast a ^Gdisintegration^x spell on the '" << exit->getCName() << "' exit^x.\n" << ColorOff;
                broadcast(player->getSock(), player->getParent(), "%M casts a ^Gdisintegration^x spell on the '%s' exit^x.", player.get(), exit->getCName());

                if (exit->flagIsSet(X_PORTAL)) {
                    broadcast((std::shared_ptr<Socket> )nullptr, player->getRoomParent(), "^GAn eerie green light engulfs the '%s' exit^x!", exit->getCName());
                    Move::deletePortal(player->getRoomParent(), exit);
                    return(0);
                }
                else {
                    if ( effect && !effect->getExtra()) {
                        if (effect->getStrength() > player->getLevel() && !player->isCt()) {
                            *player << ColorOn << "Your ^Gdisintegrate^x spell is not powerful enough yet to remove the wall-of-force on the '" << exit->getCName() << "' exit.\n" << ColorOff;
                            broadcast(player->getSock(), player->getParent(), "%M's ^Gdisintegration^x spell failed to remove the wall-of-force on the '%s' exit.^x", player.get(), exit->getCName());
                            return(0);
                        }
                        broadcast((std::shared_ptr<Socket> )nullptr, player->getRoomParent(), "^GAn eerie green light engulfs the wall-of-force blocking the '%s' exit!^x", exit->getCName());
                        bringDownTheWall(effect, player->getRoomParent(), exit);
                        return(0);
                    }
                }
            }

            *player << "You don't see that here.\n";
            return(0);
        }

        if(!player->canAttack(target))
            return(0);

        *player << ColorOn << "^GYou cast a disintegration spell on " << target << ".\n" << ColorOff;
        *target << ColorOn << "^G" << player << " casts a disintegration spell on you.\n" << ColorOff;
        broadcast(player->getSock(), target->getSock(), player->getParent(), "^G%M casts a disintegration spell on %N.", player.get(), target.get());

        bns = ((int)target->getLevel() - (int)player->getLevel())  * 25;

        if(target->mFlagIsSet(M_PERMENANT_MONSTER))
            bns = 50;

        if(target->isPlayer() && target->isEffected("resist-magic"))
            bns += std::min<short>(30, target->saves[DEA].chance);

        if(spellData->how == CastType::CAST && player->isPlayer())
            player->getAsPlayer()->statistics.offensiveCast();

        saved = target->chkSave(DEA, player, bns);

        if(saved) {
            player->doDamage(target, target->hp.getCur()/2);
            *target << "Negative energy rips through your body.\n";
            target->stun(Random::get(10,15));
            *player << setf(CAP) << target << " avoided disintegration.\n";
        } else {
            *target << ColorOn << "^GYou are engulfed in an eerie green light.\nYou've been disintegrated!\n" << ColorOff;
            broadcast(target->getSock(), player->getRoomParent(),
                "^G%M is engulfed in an eerie green light!\n%M was disintegrated!", target.get(), target.get());

            target->hp.setCur(1);
            target->mp.setCur(1);

            // goodbye inventory
            if(target->getAsMonster()) {
                target->objects.clear();
                target->coins.zero();
            }
            target->die(player);
        }

    }

    return(1);
}

//*********************************************************************
//                      splStatChange
//*********************************************************************

int splStatChange(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData, const std::string& effect, bool good) {
    std::shared_ptr<Creature> target=nullptr;
    if( spellData->how == CastType::CAST &&
        player->getClass() !=  CreatureClass::MAGE &&
        player->getClass() !=  CreatureClass::LICH &&
        player->getClass() !=  CreatureClass::DRUID &&
        player->getClass() !=  CreatureClass::CLERIC &&
        !player->isCt()
    ) {
        player->print( "You cannot cast that spell.\n");
        return(0);
    }

    // cast on self
    if(cmnd->num == 2) {
        target = player;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("You cast %s on yourself.\n", effect.c_str());
            broadcast(player->getSock(), player->getParent(), "%M casts %s on %sself.", player.get(), effect.c_str(), player->himHer());
        }
    // cast on another
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print( "That person is not here.\n");
            return(0);
        }

        if(checkRefusingMagic(player, target))
            return(0);

        player->print( "You cast %s on %N.\n", effect.c_str(), target.get());
        target->print("%M casts %s on you.\n", player.get(), effect.c_str());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts %s on %N.", player.get(), effect.c_str(), target.get());

    }

    if(target->inCombat(false))
        player->smashInvis();

    if(!good && !player->isDm() && target != player && target->chkSave(SPL, player, 0)) {
        player->print( "%M avoided your spell.\n", target.get());
        return(1);
    }

    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus())
            player->print( "The room's magical properties increase the power of your spell.\n");
        target->addEffect(effect, -2, -2, player, true, player);
    } else {
        target->addEffect(effect);
    }

    return(1);
}

//*********************************************************************
//                      stat changing spells
//*********************************************************************

int splHaste(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splStatChange(player, cmnd, spellData, "haste", true));
}
int splSlow(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splStatChange(player, cmnd, spellData, "slow", false));
}
int splStrength(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splStatChange(player, cmnd, spellData, "strength", true));
}
int splEnfeeblement(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splStatChange(player, cmnd, spellData, "enfeeblement", false));
}
int splInsight(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splStatChange(player, cmnd, spellData, "insight", true));
}
int splFeeblemind(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splStatChange(player, cmnd, spellData, "feeblemind", false));
}
int splPrayer(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splStatChange(player, cmnd, spellData, "prayer", true));
}
int splDamnation(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splStatChange(player, cmnd, spellData, "damnation", false));
}
int splFortitude(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splStatChange(player, cmnd, spellData, "fortitude", true));
}
int splWeakness(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splStatChange(player, cmnd, spellData, "weakness", false));
}

//*********************************************************************
//                      splStoneToFlesh
//*********************************************************************

int splStoneToFlesh(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;

    if(cmnd->num < 3) {
        player->print("On whom?\n");
        return(0);
    }

    if( spellData->how == CastType::CAST &&
        player->getClass() !=  CreatureClass::MAGE &&
        player->getClass() !=  CreatureClass::LICH &&
        player->getClass() !=  CreatureClass::DRUID &&
        player->getClass() !=  CreatureClass::CLERIC &&
        player->getClass() !=  CreatureClass::BARD &&
        player->getClass() !=  CreatureClass::PALADIN &&
        player->getClass() !=  CreatureClass::DEATHKNIGHT &&
        player->getClass() !=  CreatureClass::RANGER &&
        !player->isCt()
    ) {
        player->print("You are unable to cast that spell.\n");
        return(0);
    }

    if(player->noPotion( spellData))
        return(0);

    cmnd->str[2][0] = up(cmnd->str[2][0]);
    target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);
    if(!target) {
        player->print("That's not here.\n");
        return(0);
    }

    if(checkRefusingMagic(player, target))
        return(0);

    if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
        player->print("You cast stone-to-flesh on %N.\n", target.get());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts stone-to-flesh on %N.", player.get(), target.get());
        target->print("%M casts stone-to-flesh on you.\n", player.get());
        if(target->isEffected("petrification"))
            target->print("Your body returns to flesh.\n");
        else
            player->print("Nothing happens.\n");
    }

    target->removeEffect("petrification");
    return(1);
}

//*********************************************************************
//                      splDeafness
//*********************************************************************

int splDeafness(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;
    int     bns=0, canCast=0, mpCost=0;
    long    dur=0;

    if(player->getClass() == CreatureClass::BUILDER) {
        player->print("You cannot cast this spell.\n");
        return(0);
    }

    if( player->getClass() == CreatureClass::LICH ||
        player->getClass() == CreatureClass::MAGE ||
        player->getClass() == CreatureClass::CLERIC ||
        player->isCt()
    )
        canCast = 1;

    if(!canCast && player->isPlayer() && spellData->how == CastType::CAST) {
        player->print("You are unable to cast that spell.\n");
        return(0);
    }

    player->smashInvis();

    if(spellData->how == CastType::CAST) {
        dur = Random::get(180,300) + 3*bonus(player->intelligence.getCur());

        player->subMp(mpCost);
    } else if(spellData->how == CastType::SCROLL)
        dur = Random::get(30,120) + bonus(player->intelligence.getCur());
    else
        dur = Random::get(30,60);



    if(player->spellFail( spellData->how))
        return(0);

    // silence on self
    if(cmnd->num == 2) {
        target = player;
        if(player->isEffected("resist-magic"))
            dur /= 2;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            broadcast(player->getSock(), player->getParent(), "%M casts deafness on %sself.", player.get(), player->himHer());
        } else if(spellData->how == CastType::POTION)
            player->print("Your throat goes dry and you cannot speak.\n");

    // silence a monster or player
    } else {
        if(player->noPotion( spellData))
            return(0);

        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target || target == player) {
            player->print("That's not here.\n");
            return(0);
        }

        if(!player->canAttack(target))
            return(0);

        if(player->isPlayer() && target->mFlagIsSet(M_PERMENANT_MONSTER)) {
            if(!dec_daily(&player->daily[DL_SILENCE]) && !player->isCt()) {
                player->print("You have done that enough times for today.\n");
                return(0);
            }
        }

        bns = ((int)target->getLevel() - (int)player->getLevel())*10;

        if( (target->isPlayer() && target->isEffected("resist-magic")) ||
            (target->isMonster() && target->isEffected("resist-magic"))
        ) {
            bns += 50;
            dur /= 2;
        }

        if(target->isPlayer() && player->isCt())
            dur = 600L;

        if(target->mFlagIsSet(M_PERMENANT_MONSTER))
            bns = (target->saves[SPL].chance)/3;

        target->wake("Terrible nightmares disturb your sleep!");

        if(target->chkSave(SPL, player, bns) && !player->isCt()) {
            target->print("%M tried to cast a deafness spell on you!\n", player.get());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to cast a deafness spell on %N!", player.get(), target.get());
            player->print("Your spell fizzles.\n");
            return(0);
        }


        if(player->isPlayer() && target->isPlayer()) {
            if(!dec_daily(&player->daily[DL_SILENCE]) && !player->isCt()) {
                player->print("You have done that enough times for today.\n");
                return(0);
            }
        }


        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Deafness casted on %s.\n", target->getCName());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a deafness spell on %N.", player.get(), target.get());

            logCast(player, target, "silence");

            target->print("%M casts a deafness spell on you.\n", player.get());
        }

        if(target->isMonster())
            target->getAsMonster()->addEnemy(player);
    }

    target->addEffect("deafness", dur, player->getLevel());

    if(spellData->how == CastType::CAST && player->isPlayer())
        player->getAsPlayer()->statistics.offensiveCast();

    return(1);
}

//*********************************************************************
//                      splRegeneration
//*********************************************************************

int splRegeneration(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    if(spellData->how != CastType::POTION && player->isPlayer() && !player->isStaff()) {
        switch(player->getClass()) {
            case CreatureClass::CLERIC:
            case CreatureClass::DRUID:
                break;
            default:
                player->print("Only clerics and druids may cast this spell.\n");
                return(0);
        }
    }
    return(splGeneric(player, cmnd, spellData, "a", "regeneration", "regeneration", spellData->level));
}

//*********************************************************************
//                      splWellOfMagic
//*********************************************************************

int splWellOfMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "well-of-magic", "well-of-magic", spellData->level));
}
