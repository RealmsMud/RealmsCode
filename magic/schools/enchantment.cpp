/*
 * enchantment.cpp
 *   Enchantment spells
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

#include <ctime>           // for time

#include "bstring.hpp"     // for bstring
#include "cmd.hpp"         // for cmd
#include "config.hpp"      // for Config, gConfig
#include "creatures.hpp"   // for Creature, Player, Monster
#include "deityData.hpp"   // for DeityData
#include "flags.hpp"       // for M_PERMENANT_MONSTER, M_RESIST_STUN_SPELL
#include "global.hpp"      // for CastType, CreatureClass, CastType::CAST
#include "magic.hpp"       // for SpellData, checkRefusingMagic, canEnchant
#include "money.hpp"       // for GOLD, Money
#include "mud.hpp"         // for DL_SILENCE, LT_ENCHA, LT_SPELL, DL_ENCHA
#include "objects.hpp"     // for Object, ObjectType, ObjectType::WEAPON
#include "proto.hpp"       // for broadcast, bonus, crtWisdom, dec_daily, dice
#include "random.hpp"      // for Random
#include "rooms.hpp"       // for BaseRoom, UniqueRoom
#include "utils.hpp"       // for MAX, MIN


//*********************************************************************
//                      splHoldPerson
//*********************************************************************

int splHoldPerson(Creature* player, cmd* cmnd, SpellData* spellData) {
    int     bns=0, nohold=0, dur=0;
    Creature* target=nullptr;


    if( player->getClass() !=  CreatureClass::CLERIC &&
        player->getClass() !=  CreatureClass::MAGE &&
        player->getClass() !=  CreatureClass::LICH &&
        player->getClass() !=  CreatureClass::PALADIN &&
        !player->isCt() &&
        spellData->how == CastType::CAST
    ) {
        player->print("You are unable to cast that spell.\n");
        return(0);
    }


    if(cmnd->num == 2) {
        if(spellData->how != CastType::POTION) {
            player->print("Hold whom?\n");
            return(0);
        } else if(player->getClass() == CreatureClass::LICH) {
            player->print("Nothing happens.\n");
            return(0);
        } else if(player->isEffected("vampirism") && !isDay()) {
            player->print("Nothing happens.\n");
            return(0);
        } else if(player->isEffected("mist")) {
            player->print("Nothing happens.\n");
            return(0);
        } else {

            player->print("You are suddenly unable to move!\n");
            broadcast(player->getSock(), player->getParent(), "%M becomes unable to move!", player);

            player->unhide();
            player->addEffect("hold-person", 0, 0, player, true, player);

        }

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

        if(player->isPlayer() && target->isPlayer() && !player->isCt() && target->isCt()) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        if(player->isPlayer() && target->isPlayer() && !player->isDm() && target->isDm()) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        player->print("You cast a hold-person spell on %N.\n", target);
        target->print("%M casts a hold-person spell on you.\n", player);
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a hold-person spell on %N.", player, target);


        if(target->getClass() == CreatureClass::LICH) {
            player->printColor("Liches cannot be held.\n^yYour spell failed.\n");
            return(0);
        }

        if(target->isEffected("vampirism") && !isDay()) {
            player->printColor("Vampires cannot be held at night.\n^yYour spell failed.\n");
            return(0);
        }

        if(target->isEffected("berserk")) {
            player->print("No one berserk can be held.\nYour spell failed.\n");
            return(0);
        }

        if( target->isPlayer() &&
            (target->flagIsSet(P_UNCONSCIOUS) || target->isEffected("petrification") || target->isEffected("mist"))
        ) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        if(target->isPlayer() && target->isEffected("hold-person")) {
            player->printColor("%M is already held fast.\n^yYour spell fails^x.\n", target);
            return(0);
        }

        if(target->isPlayer() && Random::get(1,100) <= 50 && target->isEffected("resist-magic")) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        if(spellData->how == CastType::CAST)
            bns = 5*(spellData->level - target->getLevel()) + 2*crtWisdom(target) - 2*bonus(
                    player->intelligence.getCur());

        if(target->isPlayer() && target->getClass() == CreatureClass::CLERIC && target->getDeity() == ARES)
            bns += 25;

        if(target->isMonster()) {
            if( target->flagIsSet(M_DM_FOLLOW) ||
                target->flagIsSet(M_PERMENANT_MONSTER) ||
                !target->getRace() ||
                target->isEffected("resist-magic") ||
                target->flagIsSet(M_RESIST_STUN_SPELL) ||
                target->isEffected("reflect-magic")
            )
                nohold = 1;

            if( target->flagIsSet(M_LEVEL_BASED_STUN) &&
                (((int)target->getLevel() - (int)spellData->level) > ((player->getClass() == CreatureClass::LICH || player->getClass() == CreatureClass::MAGE) ? 6:4))
            )
                nohold = 1;
        }

        if(target->isMonster()) {
            player->smashInvis();
            if((!target->chkSave(SPL, player, bns) && !nohold) || player->isCt()) {

                if(spellData->how == CastType::CAST)
                    dur = Random::get(9,18) + 2*bonus(player->intelligence.getCur()) - crtWisdom(target);
                else
                    dur = Random::get(9,12);

                target->stun(dur);


                player->print("%M is held in place!\n", target);
                broadcast(player->getSock(), player->getParent(), "%M's spell holds %N in place!", player, target);

                if(player->isCt())
                    player->print("*DM* %d seconds.\n", dur);

                if(target->isMonster())
                    target->getAsMonster()->addEnemy(player);

            } else {
                player->print("%M resisted your spell.\n", target);
                broadcast(player->getSock(), target->getRoomParent(), "%M resisted %N's hold-person spell.",target, player);
                if(target->isMonster())
                    target->getAsMonster()->addEnemy(player);
            }

        } else {
            player->smashInvis();
            if(!target->chkSave(SPL, player, bns) || player->isCt()) {

                if(spellData->how == CastType::CAST)
                    dur = Random::get(12,18) + 2*bonus(player->intelligence.getCur()) - crtWisdom(target);
                else
                    dur = Random::get(9,12);

                if(player->isCt())
                    player->print("*DM* %d seconds.\n", dur);

                target->addEffect("hold-person", 0, 0, player, true, player);

            } else {
                player->print("%M resisted your spell.\n", target);
                broadcast(player->getSock(), target->getSock(), target->getRoomParent(), "%M resisted %N's hold-person spell.",target, player);
                target->print("%M tried to cast a hold-person spell on you.\n", player);
            }
        }
    }
    return(1);
}

//*********************************************************************
//                      splScare
//*********************************************************************

int splScare(Creature* player, cmd* cmnd, SpellData* spellData) {
    Player  *target=nullptr;
    int     bns=0;
    long    t = time(nullptr);
    Object  *weapon=nullptr, *weapon2=nullptr;

    if(spellData->how == CastType::POTION &&
        player->getClass() == CreatureClass::PALADIN &&
        (player->getDeity() == ENOCH || player->getDeity() == LINOTHAN)
    ) {
        player->print("Your deity forbids such dishonorable actions!\n");
        return(0);
    }

    if(player->getClass() !=  CreatureClass::CLERIC && player->getClass() !=  CreatureClass::MAGE &&
        player->getClass() !=  CreatureClass::LICH && player->getClass() !=  CreatureClass::DEATHKNIGHT &&
        player->getClass() !=  CreatureClass::BARD && !player->isEffected("vampirism") &&
        !player->isCt() && spellData->how == CastType::CAST
    ) {
        player->print("You are unable to cast that spell.\n");
        return(0);
    }

    // Cast scare
    if(cmnd->num == 2) {
        if(spellData->how != CastType::POTION) {
            player->print("On whom?\n");
            return(0);
        }

        if(player->isEffected("berserk")) {
            player->print("You are too enraged to drink that right now.\n");
            return(0);
        }
        if(player->isEffected("courage")) {
            player->print("You feel a strange tingle.\n");
            return(0);
        }

        player->print("You are suddenly terrified to the bone!\n");
        broadcast(player->getSock(), player->getParent(), "%M becomes utterly terrified!", player);
        if(player->ready[WIELD-1]) {
            weapon = player->ready[WIELD-1];
            player->ready[WIELD-1] = nullptr;
        }
        if(player->ready[HELD-1]) {
            weapon2 = player->ready[HELD-1];
            player->ready[HELD-1] = nullptr;
        }

        target = player->getAsPlayer();

        // killed while fleeing?
        if(target->flee(true) == 2)
            return(1);

        if(weapon)
            player->ready[WIELD-1] = weapon;
        if(weapon2)
            player->ready[HELD-1] = weapon2;


    // Cast scare on another player
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);

        target = player->getParent()->findPlayer(player, cmnd, 2);
        if(!target) {
            player->print("On whom?\n");
            return(0);
        }


        if(target->inCombat(false) && !player->flagIsSet(P_OUTLAW)) {
            player->print("Not in the middle of combat.\n");
            return(0);
        }

        if(!player->isCt() && target->isCt()) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        if(!player->isDm() && target->isDm()) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        if(!player->isCt() && !dec_daily(&player->daily[DL_SCARES])) {
            player->print("You cannot cast that again today.\n");
            return(0);
        }


        player->print("You cast a scare spell on %N.\n", target);
        target->print("%M casts a scare spell on you.\n", player);
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a scare spell on %N.", player, target);


        if(target->getClass() == CreatureClass::PALADIN) {
            player->printColor("Paladins are immune to magical fear.\n^yYour spell failed.\n");
            return(0);
        }

        if(target->isEffected("berserk") || target->isUnconscious() || target->isEffected("petrification")) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        if(target->isEffected("courage")) {
            player->print("Your spell is ineffective against %N right now.\n", target);
            return(0);
        }

        if(Random::get(1,100) <= 50 && target->isEffected("resist-magic")) {
            player->printColor("^yYour spell failed.\n");
            return(0);
        }

        target->wake("Terrible nightmares disturb your sleep!");

        // Pets are immune.
        if(target->isPet()) {
            player->print("%M's %s is too loyal to be magically scared.\n", target->getMaster(), target->getCName());
            return(0);
        }

        bns = 5*((int)target->getLevel() - (int)spellData->level) + crtWisdom(target);
        if(!target->chkSave(SPL, player, bns) || player->isCt()) {

            if(spellData->how == CastType::CAST && player->isPlayer())
                player->getAsPlayer()->statistics.offensiveCast();

            target->clearFlag(P_SITTING);
            target->print("You are suddenly terrified to the bone!\n");
            broadcast(target->getSock(), player->getRoomParent(), "%M becomes utterly terrified!", target);

            target->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
            target->lasttime[LT_SPELL].ltime = t;
            target->lasttime[LT_SPELL].interval = 3L;


            /* the following code keeps jackasses from abusing re-wield triggers */
            /* to make people drop their weapons. Remove the weapon, store it in */
            /* temp variable...then have player attempt flee, then put weapons back. */
            //*********************************************************************
            if(target->ready[WIELD-1]) {
                weapon = target->ready[WIELD-1];
                target->ready[WIELD-1] = nullptr;
            }
            if(target->ready[HELD-1]) {
                weapon2 = target->ready[HELD-1];
                target->ready[HELD-1] = nullptr;
            }

            // killed while fleeing?
            if(target->flee(true) == 2)
                return(1);

            target->stun(Random::get(5,8));

            if(weapon)
                target->ready[WIELD-1] = weapon;
            if(weapon2)
                target->ready[HELD-1] = weapon2;

            //*********************************************************************
        } else {
            player->print("%M resisted your spell.\n", target);
            broadcast(player->getSock(), target->getSock(), target->getRoomParent(), "%M resisted %N's scare spell.",target, player);
            target->print("%M tried to cast a scare spell on you.\n", player);
        }
    }
    return(1);
}

//*********************************************************************
//                      splCourage
//*********************************************************************

int splCourage(Creature* player, cmd* cmnd, SpellData* spellData) {
    Creature* target=nullptr;
    long    dur=0;
    int     noCast=0;

    if(spellData->how != CastType::POTION) {
        if(!player->isCt() && player->getClass() !=  CreatureClass::PALADIN && player->getClass() !=  CreatureClass::CLERIC) {
            player->print("Your class is unable to cast that spell.\n");
            return(0);
        }

        if(player->getClass() == CreatureClass::CLERIC) {
            switch(player->getDeity()) {
            case CERIS:
            case ARAMON:
            case GRADIUS:
            case ARACHNUS:
            case JAKAR:
            case MARA:
                noCast=1;
            break;
            }
        }

        if(noCast) {
            player->print("%s does not allow you to cast that spell.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
            return(0);
        }
    }

    if(cmnd->num == 2) {
        target = player;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Courage spell cast.\nYou feel unnaturally brave.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts a courage spell on %sself.", player, player->himHer());
        } else if(spellData->how == CastType::POTION)
            player->print("You feel unnaturally brave.\n");

    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);

        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print("You don't see that person here.\n");
            return(0);
        }

        if(checkRefusingMagic(player, target))
            return(0);

        player->print("Courage cast on %s.\n", target->getCName());
        target->print("%M casts a courage spell on you.\n", player);
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a courage spell on %N.", player, target);
    }

    target->removeEffect("fear", true, false);

    if(target->isEffected("fear")) {
        player->print("Your spell fails.\n");
        target->print("The spell fails.\n");
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        dur = MAX(300, 900 + bonus(player->intelligence.getCur()) * 300);

        if(player->getRoomParent()->magicBonus()) {
            player->print("The room's magical properties increase the power of your spell.\n");
            dur += 300L;
        }
    } else
        dur = 600;

    target->addEffect("courage", dur, 1, player, true, player);
    return(1);
}

//*********************************************************************
//                      splFear
//*********************************************************************
// The fear spell causes the monster to have a high wimpy / flee
// percentage and a penality of -2 on all attacks

int splFear(Creature* player, cmd* cmnd, SpellData* spellData) {
    Creature* target=nullptr;
    int     dur=0;

    if(spellData->how == CastType::CAST) {
        dur = 600 + Random::get(1, 30) * 10 + bonus(player->intelligence.getCur()) * 150;
    } else if(spellData->how == CastType::SCROLL)
        dur = 600 + Random::get(1, 15) * 10 + bonus(player->intelligence.getCur()) * 50;
    else
        dur = 600 + Random::get(1, 30) * 10;

    if(spellData->how == CastType::POTION)
        dur = Random::get(1,120) + 180L;

    if(player->spellFail( spellData->how))
        return(0);

    // fear on self
    if(cmnd->num == 2) {
        target = player;

        if(player->isEffected("resist-magic"))
            dur /= 2;

        if(spellData->how == CastType::CAST || spellData->how == CastType::WAND || spellData->how == CastType::SCROLL) {
            player->print("You cast a fear spell on yourself.\n");
            player->print("Nothing happens.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts a fear spell on %sself.", player, player->himHer());
            return(0);
        }

        if(spellData->how == CastType::POTION && player->getClass() == CreatureClass::PALADIN) {
            player->print("You feel a jitter, then shrug it off.\n");
            return(0);
        } else if(spellData->how == CastType::POTION)
            player->print("You begin to shake in terror.\n");

    // fear a monster or player
    } else {
        if(player->noPotion( spellData))
            return(0);

        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);
        if(!target || target == player) {
            player->print("That's not here.\n");
            return(0);
        }

        if(spellData->how == CastType::WAND || spellData->how == CastType::SCROLL || spellData->how == CastType::CAST) {
            if( player->isPlayer() &&
                target->isPlayer() &&
                player->getRoomParent()->isPkSafe() &&
                !target->flagIsSet(P_OUTLAW)
            ) {
                player->print("You cannot cast that spell here.\n");
                return(0);
            }
        }

        if(!player->canAttack(target))
            return(0);


        if( (target->mFlagIsSet(M_PERMENANT_MONSTER)) ||
            (target->getClass() == CreatureClass::PALADIN && target->isMonster())
        ) {
            player->print("%M seems unaffected by fear.\n", target);
            broadcast(player->getSock(), target->getSock(), player->getParent(),
                "%M casts a fear spell on %N.\n", player, target);
            broadcast(player->getSock(), target->getSock(), player->getParent(),
                "%M brushes it off and attacks %N.\n", player, target);

            target->getAsMonster()->addEnemy(player, true);
            return(0);
        }



        if( (target->isPlayer() && target->isEffected("resist-magic")) ||
            (target->isMonster() && target->isEffected("resist-magic"))
        )
            dur /= 2;

        player->smashInvis();
        target->wake("Terrible nightmares disturb your sleep!");

        if(target->isPlayer() && target->getClass() == CreatureClass::PALADIN) {
            player->print("Fear spell cast on %s.\n", target->getCName());
            player->print("It doesn't do anything noticeable.\n");
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts fear on %N.",
                player, target);
            target->print("%M casts a fear spell on you.\n", player);
            target->print("It has no apparent effect.\n");
            return(0);
        }

        if(target->chkSave(SPL, player, 0) && !player->isCt()) {
            target->print("%M tried to cast a fear spell on you!\n", player);
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to cast a fear spell on %N!", player, target);
            player->print("Your spell fizzles.\n");
            return(0);
        }

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Fear spell cast on %s.\n", target->getCName());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts fear on %N.",
                player, target);
            target->print("%M casts a fear spell on you.\n", player);
        }

        if(target->isMonster())
            target->getAsMonster()->addEnemy(player);
    }

    target->addEffect("fear", dur, 1, player, true, player);

    if(spellData->how == CastType::CAST && player->isPlayer())
        player->getAsPlayer()->statistics.offensiveCast();
    return(1);
}


//*********************************************************************
//                      splSilence
//*********************************************************************
// Silence causes a player or monster to lose their voice, making them
// unable to casts spells, use scrolls, speak, yell, or broadcast

int splSilence(Creature* player, cmd* cmnd, SpellData* spellData) {
    Creature* target=nullptr;
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

    if(player->getClass() == CreatureClass::MAGE || player->getClass() == CreatureClass::LICH)
        mpCost = 35;
    else
        mpCost = 20;

    if(spellData->how == CastType::CAST && !player->checkMp(mpCost))
        return(0);

    player->smashInvis();

    if(spellData->how == CastType::CAST) {
        dur = Random::get(180,300) + 3*bonus(player->intelligence.getCur());
    } else if(spellData->how == CastType::SCROLL)
        dur = Random::get(30,120) + bonus(player->intelligence.getCur());
    else
        dur = Random::get(30,60);



    if(player->spellFail( spellData->how))
        return(0);

    // silence on self
    if(cmnd->num == 2) {
        if(spellData->how == CastType::CAST)
            player->subMp(mpCost);

        target = player;
        if(player->isEffected("resist-magic"))
            dur /= 2;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            broadcast(player->getSock(), player->getParent(), "%M casts silence on %sself.", player, player->himHer());
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

        bns = ((int)target->getLevel() - (int)spellData->level)*10;

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

        if(spellData->how == CastType::CAST)
            player->subMp(mpCost);

        target->wake("Terrible nightmares disturb your sleep!");

        if(target->chkSave(SPL, player, bns) && !player->isCt()) {
            target->print("%M tried to cast a silence spell on you!\n", player);
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to cast a silence spell on %N!", player, target);
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
            player->print("Silence casted on %s.\n", target->getCName());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a silence spell on %N.", player, target);

            logCast(player, target, "silence");

            target->print("%M casts a silence spell on you.\n", player);
        }

        if(target->isMonster())
            target->getAsMonster()->addEnemy(player);
    }

    target->addEffect("silence", dur, 1, player, true, player);

    if(spellData->how == CastType::CAST && player->isPlayer())
        player->getAsPlayer()->statistics.offensiveCast();
    if(player->isCt() && target->isPlayer())
        target->setFlag(P_DM_SILENCED);

    return(1);
}

//*********************************************************************
//                      canEnchant
//*********************************************************************

bool canEnchant(Player* player, SpellData* spellData) {
    if(!player->isStaff()) {
        if(spellData->how == CastType::CAST && player->getClass() !=  CreatureClass::MAGE && player->getClass() !=  CreatureClass::LICH) {
            player->print("Only mages may enchant objects.\n");
            return(false);
        }
        if(spellData->how == CastType::CAST && player->getClass() == CreatureClass::MAGE && player->hasSecondClass()) {
            player->print("Only pure mages may enchant objects.\n");
            return(false);
        }
        
        if(spellData->level < 16 && !player->isCt()) {
            player->print("You are not experienced enough to cast that spell yet.\n");
            return(false);
        }
    }
    return(true);
}

bool canEnchant(Creature* player, Object* object) {
    if(player->isStaff())
        return(true);
    if( object->flagIsSet(O_CUSTOM_OBJ) ||
        (object->getType() != ObjectType::ARMOR && object->getType() != ObjectType::WEAPON && object->getType() != ObjectType::MISC) ||
        object->flagIsSet(O_NULL_MAGIC) ||
        object->flagIsSet(O_NO_ENCHANT)
    ) {
        player->print("That cannot be enchanted.\n");
        return(false);
    }
    if(object->getAdjustment()) {
        player->printColor("%O is already enchanted.\n", object);
        return(false);
    }
    return(true);
}

//*********************************************************************
//                      decEnchant
//*********************************************************************

bool decEnchant(Player* player, CastType how) {
    if(how == CastType::CAST) {
        if(!dec_daily(&player->daily[DL_ENCHA]) && !player->isCt()) {
            player->print("You have enchanted enough today.\n");
            return(false);
        }
    }
    return(true);
}


//*********************************************************************
//                      splEnchant
//*********************************************************************
// This function allows mages to enchant items.

int splEnchant(Creature* player, cmd* cmnd, SpellData* spellData) {
    Player* pPlayer = player->getAsPlayer();
    if(!pPlayer)
        return(0);

    Object  *object=nullptr;
    int     adj=1;

    if(!canEnchant(pPlayer, spellData))
        return(0);

    if(cmnd->num < 3) {
        pPlayer->print("Cast the spell on what?\n");
        return(0);
    }

    object = pPlayer->findObject(pPlayer, cmnd, 2);
    if(!object) {
        pPlayer->print("You don't have that in your inventory.\n");
        return(0);
    }

    if(!canEnchant(pPlayer, object))
        return(1);

    if(!decEnchant(pPlayer, spellData->how))
        return(0);

    if((pPlayer->getClass() == CreatureClass::MAGE || pPlayer->isStaff()) && spellData->how == CastType::CAST)
        adj = MIN<int>(4, (spellData->level / 5));

    object->setAdjustment(MAX<int>(adj, object->getAdjustment()));

    if(object->getType() == ObjectType::WEAPON) {
        object->setShotsMax(object->getShotsMax() + adj * 10);
        object->incShotsCur(adj * 10);
    }
    object->value.set(500 * adj, GOLD);

    pPlayer->printColor("%O begins to glow brightly.\n", object);
    broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%M enchants %1P.", pPlayer, object);

    if(!pPlayer->isDm())
        log_immort(true, pPlayer, "%s enchants a %s^g in room %s.\n", pPlayer->getCName(), object->getCName(),
            pPlayer->getRoomParent()->fullName().c_str());

    return(1);
}


//*********************************************************************
//                      cmdEnchant
//*********************************************************************
// This function enables mages to randomly enchant an item
// for a short period of time.

int cmdEnchant(Player* player, cmd* cmnd) {
    Object* object=nullptr;
    int     dur;

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("enchant") || player->hasSecondClass()) {
        player->print("You lack the training to enchant objects.\n");
        return(0);
    }

    if(player->isBlind()) {
        player->printColor("^CYou can't do that! You're blind!\n");
        return(0);
    }
    if(cmnd->num < 2) {
        player->print("Enchant what?\n");
        return(0);
    }

    object = player->findObject(player, cmnd, 1);
    if(!object) {
        player->print("You don't have that item in your inventory.\n");
        return(0);
    }

    if(!canEnchant(player, object))
        return(0);

// I do suppose temp enchant sucks enough that we can let it go without daily limits -Bane

//  if(!dec_daily(&player->daily[DL_ENCHA]) && !player->isCt()) {
//      player->print("You have enchanted enough today.\n");
//      return(0);
//  }


    dur = MAX(600, MIN(7200, (int)(player->getSkillLevel("enchant")*100) + bonus(player->intelligence.getCur()) * 100));
    object->lasttime[LT_ENCHA].interval = dur;
    object->lasttime[LT_ENCHA].ltime = time(nullptr);

    object->randomEnchant((int)player->getSkillLevel("enchant")/2);

    player->printColor("%O begins to glow brightly.\n", object);
    broadcast(player->getSock(), player->getParent(), "%M enchants %1P.", player, object);
    player->checkImprove("enchant", true);
    if(!player->isDm())
        log_immort(true, player, "%s temp_enchants a %s in room %s.\n", player->getCName(), object->getCName(),
            player->getRoomParent()->fullName().c_str());

    object->setFlag(O_TEMP_ENCHANT);
    return(0);
}

//*********************************************************************
//                      splStun
//*********************************************************************

int splStun(Creature* player, cmd* cmnd, SpellData* spellData) {
    Player  *pPlayer = player->getAsPlayer();
    Creature* target=nullptr;
    Monster *mTarget=nullptr;
    int     dur=0, bns=0, mageStunBns=0;

    if(pPlayer)
        pPlayer->computeLuck();     // luck for stun

    player->smashInvis();

    if( player->getClass() == CreatureClass::LICH ||
         (pPlayer && pPlayer->getClass() == CreatureClass::MAGE && !pPlayer->hasSecondClass()) ||
         player->isCt())
        mageStunBns = Random::get(1,5);

    if((target = player->findMagicVictim(cmnd->str[2], cmnd->val[2], spellData, true, false, "Cast on what?\n", "You don't see that here.\n")) == nullptr)
        return(0);

    // stun self
    if(target == player) {

        if(spellData->how == CastType::CAST) {
            dur = bonus(player->intelligence.getCur()) * 2 +
                  (((Player*)player)->getLuck() / 10) + mageStunBns;
        } else
            dur = dice(2, 6, 0);

        dur = MAX(5, dur);
        player->updateAttackTimer(true, dur*10);

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("You're stunned.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts a stun spell on %sself.",
                player, player->himHer());
        } else if(spellData->how == CastType::POTION)
            player->print("You feel dizzy.\n");

    // Stun a monster or player
    } else {
        mTarget = target->getAsMonster();

        if(mTarget) {
            if(player->flagIsSet(P_LAG_PROTECTION_SET))
                player->setFlag(P_LAG_PROTECTION_ACTIVE);
        }

        if(!player->canAttack(target))
            return(0);

        if(mTarget) {
            if(Random::get(1,100) < mTarget->getMagicResistance()) {
                player->printColor("^MYour spell has no effect on %N.\n", mTarget);
                if(player->isPlayer())
                    broadcast(player->getSock(), player->getParent(), "%M's spell has no effect on %N.", player, mTarget);
                return(0);
            }

        }


        if(mTarget && player->isPlayer()) {
            if( !mTarget->flagIsSet(M_RESTRICT_EXP_LEVEL) &&
                mTarget->flagIsSet(M_LEVEL_RESTRICTED) &&
                mTarget->getMaxLevel() &&
                !mTarget->flagIsSet(M_AGGRESSIVE) &&
                !mTarget->isEnemy(player)
            ) {
                if( spellData->level > mTarget->getMaxLevel() &&
                    !player->checkStaff("You shouldn't be picking on %s.\n", mTarget->himHer())
                )
                    return(0);
            }
        }


        if(spellData->how == CastType::CAST) {
            dur = bonus(player->intelligence.getCur()) * 2 + dice(2, 6, 0) + mageStunBns;
        } else
            dur = dice(2, 5, 0);

        if(player->isPlayer() && !mTarget) {
            dur = ((spellData->level - target->getLevel()) +
                    (bonus(player->intelligence.getCur()) -
                     bonus(target->intelligence.getCur())));
            if(dur < 3)
                dur = 3;

            dur += mageStunBns;

            if(target->isEffected("resist-magic") || target->flagIsSet(P_RESIST_STUN))
                dur= 3;

        }

        if( target->isEffected("resist-magic") ||
            target->flagIsSet(mTarget ? M_RESIST_STUN_SPELL : P_RESIST_STUN))
        {
            if(spellData->how != CastType::CAST)
                dur = 0;
            else
                dur = 3;
        } else
            dur = MAX(5, dur);

        if(mTarget) {

            if(mTarget->flagIsSet(M_MAGE_LICH_STUN_ONLY) && player->getClass() !=  CreatureClass::MAGE && player->getClass() !=  CreatureClass::LICH && !player->isStaff()) {
                player->print("Your training in magic is insufficient to properly stun %N!\n", mTarget);
                if(spellData->how != CastType::CAST)
                dur = 0;
                else
                    dur = 3;
            }

            if( player->isPlayer() &&
                !mTarget->isEffected("resist-magic") &&
                !mTarget->flagIsSet(M_RESIST_STUN_SPELL) &&
                mTarget->flagIsSet(M_LEVEL_BASED_STUN)
            ) {
                if(((int)mTarget->getLevel() - (int)spellData->level) > ((player->getClass() == CreatureClass::LICH || player->getClass() == CreatureClass::MAGE) ? 6:4))
                    dur = MAX(3, bonus(player->intelligence.getCur()));
                else
                    dur = MAX(5, dur);
            }

            if(player->isEffected("reflect-magic") && mTarget->isEffected("reflect-magic")) {
                player->print("Your stun is cancelled by both your magical-shields!\n");
                mTarget->addEnemy(player);
                return(1);
            }
            if(mTarget->isEffected("reflect-magic")) {
                player->print("Your stun is reflected back at you!\n");
                broadcast(player->getSock(), mTarget->getSock(), player->getRoomParent(), "%M's stun is reflected back at %s!",
                    player, player->himHer());
                if(mTarget->isDm() && !player->isDm())
                    dur = 0;
                player->stun(dur);
                mTarget->addEnemy(player);
                return(1);
            }

        } else {
            if(player->isEffected("reflect-magic") && target->isEffected("reflect-magic")) {
                player->print("Your stun is cancelled by both your magical-shields!\n");
                target->print("Your magic-shield reflected %N's stun!\n", player);
                return(1);
            }
            if(target->isEffected("reflect-magic")) {
                player->print("Your stun is reflected back at you!\n");
                target->print("%M's stun is reflected back at %s!\n", player, player->himHer());
                if(player->flagIsSet(P_RESIST_STUN) || player->isEffected("resist-magic"))
                    dur = 3;
                broadcast(player->getSock(), target->getSock(), player->getParent(), "%M's stun is reflected back at %s!",
                    player, player->himHer());
                if(target->isDm() && !player->isDm())
                    dur = 0;
                player->stun(dur);
                return(1);
            }
        }

        if(player->isMonster() && !mTarget && target->isEffected("reflect-magic")) {
            if(player->isEffected("reflect-magic")) {
                target->print("Your magic-shield reflects %N's stun!\n", player);
                target->print("%M's magic-shield cancels it!\n", player);
                return(1);
            }
            if(player->flagIsSet(M_RESIST_STUN_SPELL) || player->isEffected("resist-magic"))
                dur = 3;
            target->print("%M's stun is reflected back at %s!\n", player, player->himHer());
            if(target->isDm() && !player->isDm())
                dur = 0;
            player->stun(dur);
            return(1);
        }

        if(!mTarget && player->isEffected("berserk"))
            dur /= 2;

        if(target->isDm() && !player->isDm())
            dur = 0;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Stun cast on %s.\n", target->getCName());
            broadcast(player->getSock(), target->getSock(), player->getParent(),
                "%M casts stun on %N.", player, target);

            logCast(player, target, "stun");

            if(mTarget && player->isPlayer()) {
                if(mTarget->flagIsSet(M_YELLED_FOR_HELP) && (Random::get(1,100) <= (MAX(25, mTarget->inUniqueRoom() ? mTarget->getUniqueRoomParent()->wander.getTraffic() : 25)))) {
                    mTarget->summonMobs(player);
                    mTarget->clearFlag(M_YELLED_FOR_HELP);
                    mTarget->setFlag(M_WILL_YELL_FOR_HELP);
                }

                if(mTarget->flagIsSet(M_WILL_YELL_FOR_HELP) && !mTarget->flagIsSet(M_YELLED_FOR_HELP)) {
                    mTarget->checkForYell(player);
                }

                if(mTarget->flagIsSet(M_LEVEL_BASED_STUN) && (((int)mTarget->getLevel() - (int)spellData->level) > ((player->getClass() == CreatureClass::LICH || player->getClass() == CreatureClass::MAGE) ? 6:4))) {
                    player->printColor("^yYour magic is currently too weak to fully stun %N!\n", mTarget);

                    switch(Random::get(1,9)) {
                    case 1:
                        player->print("%M laughs wickedly at you.\n", target);
                        break;
                    case 2:
                        player->print("%M shrugs off your weak attack.\n", target);
                        break;
                    case 3:
                        player->print("%M glares menacingly at you.\n",target);
                        break;
                    case 4:
                        player->print("%M grins defiantly at your weak stun magic.\n",target);
                        break;
                    case 5:
                        player->print("What were you thinking?\n");
                        break;
                    case 6:
                        player->print("Uh oh...you're in trouble now.\n");
                        break;
                    case 7:
                        player->print("%M says, \"That tickled. Now it's my turn...\"\n",target);
                        break;
                    case 8:
                        player->print("I hope you're prepared to meet your destiny.\n");
                        break;
                    case 9:
                        player->print("Your stun just seemed to bounce off. Strange....\n");
                        break;
                    default:
                        break;
                    }
                }
            }

            player->wake("You awaken suddenly!");
            target->print("%M stunned you.\n", player);


            if((!(mTarget && mTarget->flagIsSet(M_RESIST_STUN_SPELL)) && (spellData->level <= target->getLevel())
                && !target->isCt())
            ) {

                if(mTarget && mTarget->flagIsSet(M_PERMENANT_MONSTER))
                    bns = 10;

                if(!mTarget && target->getClass() == CreatureClass::CLERIC && target->getDeity() == ARES)
                    bns += 25;

                if(target->chkSave(SPL, player, bns)) {
                    player->printColor("^y%M avoided the full power of your stun!\n", target);
                    if(target->isPlayer())
                        broadcast(player->getSock(), player->getParent(), "%M avoided a full stun.", target);
                    dur /= 2;
                    dur = MIN(5,dur);
                    target->print("You avoided a full stun.\n");
                }
            }

            target->stun(dur);
        }

        if(mTarget)
            mTarget->addEnemy(player);
    }

    return(1);
}

//*********************************************************************
//                      splGlobeOfSilence
//*********************************************************************

int splGlobeOfSilence(Creature* player, cmd* cmnd, SpellData* spellData) {
    int strength = 1;
    long duration = 600;

    if(player->noPotion( spellData))
        return(0);

    if(player->getRoomParent()->isPkSafe() && !player->isCt()) {
        player->print("That spell is not allowed here.\n");
        return(0);
    }

    player->print("You cast a globe of silence spell.\n");
    broadcast(player->getSock(), player->getParent(), "%M casts a globe of silence spell.", player);

    if(player->getRoomParent()->hasPermEffect("globe-of-silence")) {
        player->print("The spell didn't take hold.\n");
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus())
            player->print("The room's magical properties increase the power of your spell.\n");
    }

    player->getRoomParent()->addEffect("globe-of-silence", duration, strength, player, true, player);
    return(1);
}
