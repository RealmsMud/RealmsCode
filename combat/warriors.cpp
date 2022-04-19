/*
 * warriors.cpp
 *   Functions that deal with warrior like abilities
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

#include <cstring>                     // for strlen
#include <ctime>                       // for time
#include <string>                      // for allocator, string, operator==
#include <type_traits>                 // for enable_if<>::type

#include "area.hpp"                    // for Area
#include "cmd.hpp"                     // for cmd
#include "delayedAction.hpp"           // for ActionTrack, DelayedAction
#include "flags.hpp"                   // for P_AFK, P_LAG_PROTECTION_ACTIVE
#include "global.hpp"                  // for WIELD, CreatureClass, HELD
#include "lasttime.hpp"                // for lasttime
#include "mud.hpp"                     // for LT_KICK, LT_LAY_HANDS, LT_DISARM
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/creatures.hpp"    // for Creature, ATTACK_BASH, ATTACK_...
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/mudObject.hpp"    // for MudObject
#include "mudObjects/objects.hpp"      // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "proto.hpp"                   // for broadcast, bonus, log_immort
#include "random.hpp"                  // for Random
#include "server.hpp"                  // for Server, gServer
#include "size.hpp"                    // for getSizeName, NO_SIZE, SIZE_HUGE
#include "statistics.hpp"              // for Statistics
#include "stats.hpp"                   // for Stat
#include "track.hpp"                   // for Track
#include "unique.hpp"                  // for Unique
#include "utils.hpp"                   // for MIN, MAX
#include "wanderInfo.hpp"              // for WanderInfo


//*********************************************************************
//                      disarmSelf
//*********************************************************************

void Player::disarmSelf() {
    bool removed = false;
    if(ready[WIELD-1] && ready[WIELD-1]->getType() == ObjectType::WEAPON && !ready[WIELD-1]->flagIsSet(O_CURSED)) {
        printColor("You removed %1P\n", ready[WIELD-1]);
        broadcast(getSock(),  getRoomParent(), "%M removed %1P.", this, ready[WIELD-1]);
        unequip(WIELD);
        removed = true;
    }

    if(ready[HELD-1] && ready[HELD-1]->getType() == ObjectType::WEAPON) {
        if(ready[HELD-1]->flagIsSet(O_CURSED)) {
            if(!ready[WIELD-1]) {
                ready[WIELD-1] = ready[HELD-1];
                ready[HELD-1] = nullptr;
                if(ready[WIELD-1]->flagIsSet(O_NO_PREFIX)) {
                    print("%s jumped to your primary hand! It's cursed!\n", ready[WIELD-1]->getCName());
                } else {
                    print("The %s jumped to your primary hand! It's cursed!\n", ready[WIELD-1]->getCName());
                }
            }
        } else {
            printColor("You removed %1P\n", ready[HELD-1]);
            broadcast(getSock(),  getRoomParent(), "%M removed %1P.", this, ready[HELD-1]);
            unequip(HELD);
            removed = true;
        }
    }
    if(!removed)
        print("Disarm whom?\n");
    else
        unhide();
}

//*********************************************************************
//                      cmdDisarm
//*********************************************************************

int cmdDisarm(Player* player, cmd* cmnd) {
    Creature* creature=nullptr;
    Player  *pCreature=nullptr;
    BaseRoom* room = player->getRoomParent();
    long    i=0, t=0;
    int     chance=0, drop=0;

    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num < 2) {
        player->disarmSelf();
        return(0);
    }

    if(!player->knowsSkill("disarm")) {
        player->print("You are not skilled in the art of disarming your opponents.\n");
        return(0);
    }

    creature = room->findCreature(player, cmnd->str[1], cmnd->val[1], true, true);
    if(creature)
        pCreature = creature->getAsPlayer();
    if(!creature || creature == player || (pCreature && strlen(cmnd->str[1]) < 3)) {
        player->print("They aren't here.\n");
        return(0);
    }

    if(!player->canAttack(creature))
        return(0);

    if(player->isBlind()) {
        player->printColor("^CYou're blind!\n");
        return(0);
    }

    if(!creature->ready[WIELD-1]) {
        player->print("%s isn't wielding anything to disarm!\n", creature->upHeShe());
        return(0);
    }

    if( !player->ready[WIELD-1] &&
        !player->checkStaff("You must be wielding a weapon in order to disarm %N.\n", creature)
    )
        return(0);

    if( player->getPrimaryWeaponCategory() == "ranged" &&
        !player->checkStaff("Ranged weapons may not be used to disarm.\n")
    )
        return(0);

    i = player->lasttime[LT_DISARM].ltime;
    t = time(nullptr);

    if((t - i < 30L) && !player->isCt()) {
        player->pleaseWait(30L-t+i);
        return(0);
    }

    player->smashInvis();
    player->interruptDelayedActions();
    player->unhide();

    if(pCreature) {

    } else {
        //addEnmCrt(player, creature);
        if(player->flagIsSet(P_LAG_PROTECTION_SET)) {    // Activates lag protection.
            player->setFlag(P_LAG_PROTECTION_ACTIVE);
        }
    }


    player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
    player->lasttime[LT_DISARM].ltime = t;
    player->lasttime[LT_DISARM].interval = 30L;
    player->lasttime[LT_KICK].ltime = t;
    player->lasttime[LT_KICK].interval = 3L;


    int level = (int)player->getSkillLevel("disarm");
    chance = 30 + (level - creature->getLevel()) * 10
            + bonus(player->dexterity.getCur()) * 5;
    if(player->getClass() == CreatureClass::ROGUE)
        chance += 10;
    chance = MIN(chance, 85);


    if(creature->ready[WIELD-1]->flagIsSet(O_CURSED))
        chance = 3;

    if(creature->flagIsSet(M_UN_DODGEABLE))
        chance /= 4;

    if(player->isDm())
        chance = 100;

    if(creature->isCt() && !player->isDm())
        chance = 0;



    if(Random::get(1, 100) > chance) {
        player->print("You fail to disarm %N.\n", creature);
        player->checkImprove("disarm", false);
        creature->print("%M tried to disarm you!\n",player);
        broadcast(player->getSock(),  creature->getSock(), room, "%M attempts to disarm %N.\n", player, creature);
        if(creature->isMonster())
            creature->getAsMonster()->addEnemy(player);
        return(0);
    } else {
        if(player->getClass() == CreatureClass::CARETAKER)
            log_immort(false,player, "%s disarms %s.\n", player->getCName(), creature->getCName());

        drop = 5 + ((level - creature->getLevel()) - (bonus(creature->dexterity.getCur()) * 2));

        player->checkImprove("disarm", true);
        if(player->isDm())
            drop = 101;

        // monsters can never be disarmed of their unique items
        if(creature->isMonster() && Unique::is(creature->ready[WIELD-1]))
            drop = 0;

        if( Random::get(1,100) <= drop &&
            !creature->ready[WIELD-1]->flagIsSet(O_NO_DROP)
        ) {
            player->print("You disarm %N!\n", creature);
            player->printColor("You force %N to drop %P!\n", creature, creature->ready[WIELD-1]);
            creature->print("%M disarmed you!\n", player);

            broadcast(player->getSock(),  creature->getSock(), room, "%M attempts to disarm %N.", player, creature);
            broadcast(player->getSock(),  creature->getSock(), room, "%M dropped %s weapon!", creature, player->hisHer());
            if(creature->isPlayer())
                creature->print("You drop your weapon!\n");

            Object* toDrop = creature->unequip(WIELD, UNEQUIP_NOTHING);
            toDrop->addToRoom(room);
            creature->stun(2);
            if(creature->isPlayer())
                creature->unhide();
            if(creature->isMonster())  {
                creature->getAsMonster()->addEnemy(player);
                //      creature->setFlag(MDRWPN);
                creature->setFlag(M_GUARD_TREATURE);   // Can't pick up weapon until you kill mob that dropped it.
            }
        } else {
            player->print("You disarm %N!\n", creature);
            creature->print("%M disarmed you!\n", player);
            player->print("%s fumbles %s weapon!\n", creature->upHeShe(), creature->hisHer());
            broadcast(player->getSock(),  creature->getSock(), room, "%M attempts to disarm %N.", player, creature);
            broadcast(player->getSock(),  creature->getSock(), room, "%M fumbles %s weapon!", creature, creature->hisHer());

            if(creature->ready[HELD-1] && creature->getClass() == CreatureClass::RANGER)
                creature->printColor("^gYou FUMBLE your primary weapon.\n");
            else
                creature->printColor("^gYou FUMBLE your weapon.\n");

            creature->unequip(WIELD);
            creature->stun(2);
            if(creature->isPlayer())
                creature->unhide();
            else
                creature->getAsMonster()->addEnemy(player);
        }
    }

    return(0);
}


//*********************************************************************
//                      cmdMistbane
//*********************************************************************

int cmdMistbane(Player* player, cmd* cmnd) {
    long    i=0, t=0;
    int     chance=0;

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("mistbane")) {
        player->print("You lack the ability to do that.\n");
        return(0);
    }
    if(player->isUndead()) {
        player->print("You cannot imbue yourself with positive energy.\n");
        return(0);
    }
    if(player->flagIsSet(P_MISTBANE)) {
        player->print("You're already empowered to hit mists.\n");
        return(0);
    }

    i = player->lasttime[LT_MISTBANE].ltime;
    t = time(nullptr);

    if(t - i < 600L) {
        player->pleaseWait(600L-t+i);
        return(0);
    }
    chance = MIN(80, (int)(player->getSkillLevel("mistbane") * 20) + bonus(player->piety.getCur()));


    if(Random::get(1, 100) > player->getLuck() + player->getSkillLevel("mistbane") * 2)
        chance = 10;

    if(Random::get(1, 100) <= chance) {
        player->print("You imbue yourself with positive energy.\n");
        broadcast(player->getSock(), player->getParent(), "%M imbues %sself with positive energy.", player, player->himHer());
        player->checkImprove("mistbane", true);
        player->setFlag(P_MISTBANE);
        player->lasttime[LT_MISTBANE].ltime = t;
        player->lasttime[LT_MISTBANE].interval = 180L;

    } else {
        player->print("You failed to imbue yourself with positive energy.\n");
        broadcast(player->getSock(), player->getParent(), "%M tried to imbue %sself with positive energy.",
            player, player->himHer());
        player->checkImprove("mistbane", false);
        player->lasttime[LT_MISTBANE].ltime = t - 590L;
    }

    return(0);
}

//*********************************************************************
//                      cmdSecond
//*********************************************************************
// This lets a ranger wield a weapon in their off-hand, getting 1/2
// damage, and 1/2 the thac0 bonus from it.

int cmdSecond(Player* player, cmd* cmnd) {
    Object  *object=nullptr;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("dual")) {
        player->print("You are not skilled in the art of dual wielding.\n");
        return(0);
    }

    if(player->isEffected("frenzy")) {
        player->print("You cannot do that while attacking in a frenzy.\n");
        return(0);
    }

    if(cmnd->num < 2) {
        player->print("Wield what in your off hand?\n");
        return(0);
    }
    if(!player->ready[WIELD - 1]) {
        player->print("You must first be wielding a weapon in your primary hand.\n");
        return(0);
    }

    player->unhide();
    if(cmnd->num > 1) {

        object = player->findObject(player, cmnd, 1);
        if(!object) {
            player->print("You don't have that.\n");
            return(0);
        }

        if(object->getWearflag() != WIELD) {
            player->print("You can't wield that.\n");
            return(0);
        }

        if(!player->isCt() && (object->getActualWeight() >= player->ready[WIELD - 1]->getActualWeight())) {
            player->print("Your second weapon must be lighter then your primary weapon.\n");
            return(0);
        }
        if(object->getLevel() > player->getLevel() && !player->isCt()) {
            player->print("You are not experienced enough to use that.\n");
            return(0);
        }


        if(!player->canUse(object) || !player->canWield(object, SECONDOBJ))
            return(0);

        if(!player->willFit(object)) {
            player->printColor("%O isn't the right size for you.\n", object);
            return(0);
        }

        if(object->getMinStrength() > player->strength.getCur()) {
            player->printColor("You are currently not strong enough to wield %P.\n", object);
            return(0);
        }

        if(player->checkHeavyRestrict("dual wield"))
            return(false);

        player->equip(object, true);
        player->computeAttackPower();

        object->clearFlag(O_JUST_BOUGHT);
    }

    return(0);
}

//*********************************************************************
//                      cmdBerserk
//*********************************************************************

int cmdBerserk(Player* player, cmd* cmnd) {
    long    i=0, t=0;
    int     chance=0;

    player->clearFlag(P_AFK);


    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("berserk")) {
        player->print("You don't know how to go berserk!\n");
        return(0);
    }

    if( player->getClass() == CreatureClass::CLERIC &&
        player->getDeity() == ARES &&
        (player->getAdjustedAlignment() < PINKISH || player->getAdjustedAlignment() > LIGHTBLUE)
    ) {
        player->print("Your soul must be disciplined or you cannot go berserk.\n");
        return(0);
    }

    if(player->isEffected("berserk")) {
        player->print("You are already berserk.\n");
        return(0);
    }


    if(player->isEffected("strength")) {
        player->print("Your magically enhanced strength prevents you from going berserk.\n");
        return(0);
    }
    if(player->isEffected("enfeeblement")) {
        player->print("Your magically reduced strength prevents you from going berserk.\n");
        return(0);
    }
    if(player->isEffected("fortitude")) {
        player->print("Your magically enhanced health prevents you from going berserk.\n");
        return(0);
    }
    if(player->isEffected("weakness")) {
        player->print("Your magically reduced health prevents you from going berserk.\n");
        return(0);
    }


    if(player->flagIsSet(P_SITTING)) {
        player->print("You have to be standing to go berserk.\n");
        return(0);
    }

    i = player->lasttime[LT_BERSERK].ltime;
    t = time(nullptr);

    if(t - i < 600L && !player->isCt()) {
        player->pleaseWait(600L-t+i);
        return(0);
    }

    chance = MIN(85, (int)(player->getSkillLevel("berserk") * 10) + (bonus(player->strength.getCur()) * 5));

    if(Random::get(1, 100) <= chance) {
        player->print("You go berserk.\n");
        broadcast(player->getSock(), player->getParent(), "%M goes berserk!", player);
        player->checkImprove("berserk", true);
        // TODO: SKILLS: Add a modifier based on berserk skill level
        player->addEffect("berserk", 120L, 30);
        if(player->getClass() == CreatureClass::CLERIC && player->getDeity() == ARES)
            player->addEffect("berserk", 120L, 30);
        else
            player->addEffect("berserk", 120L, 50);
        player->computeAC();
        player->computeAttackPower();
        player->lasttime[LT_BERSERK].ltime = t;
        player->lasttime[LT_BERSERK].interval = 600L;
        player->interruptDelayedActions();
    } else {
        player->print("You failed to gather your strength.\n");
        player->checkImprove("berserk", false);
        broadcast(player->getSock(), player->getParent(), "%M runs around trying to go berserk.", player);
        player->lasttime[LT_BERSERK].ltime = t - 590L;
    }
    return(0);
}


//*********************************************************************
//                      cmdCircle
//*********************************************************************
// This function allows fighters and barbarians to run circles about an
// enemy, confusing it for several seconds.

int cmdCircle(Player* player, cmd* cmnd) {
    Creature* target=nullptr;
    Monster *mTarget=nullptr;
    Player  *pTarget=nullptr;
    int     chance=0, delay=0;
    double level = 0.0;

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("circle")) {
        player->print("You lack the skills to circle anything effectively!\n");
        return(0);
    }

    if(!(target = player->findVictim(cmnd, 1, true, false, "Circle whom?\n", "You don't see that here.\n")))
        return(0);
    
    pTarget = target->getAsPlayer();

    mTarget = target->getAsMonster();

    if(!player->canAttack(target))
        return(0);

    if(!player->isCt()) {
        if(!player->checkAttackTimer(true))
            return(0);

        if(pTarget) {
            if(pTarget->isEffected("mist")) {
                player->print("You can't circle a mist?!\n");
                return(0);
            }

            if(player->vampireCharmed(pTarget) || (pTarget->hasCharm(player->getName()) && player->flagIsSet(P_CHARMED))) {
                player->print("You are too fond of %N to do that.\n", pTarget);
                return(0);
            }
        }
    }


    player->smashInvis();
    player->interruptDelayedActions();

    if(player->flagIsSet(P_LAG_PROTECTION_SET)) // Activates Lag protection.
        player->setFlag(P_LAG_PROTECTION_ACTIVE);


    level = player->getSkillLevel("circle");
    if( player->getClass() == CreatureClass::FIGHTER &&
        (player->getSecondClass() == CreatureClass::THIEF || player->getSecondClass() == CreatureClass::MAGE)
    )
        level = MAX(1, (int)level-2);
    if(player->getClass() == CreatureClass::CLERIC && player->getSecondClass() == CreatureClass::FIGHTER)
        level = MAX(1, (int)level-2);

    chance = 50 + (int)((level-target->getLevel())*20) +
             (bonus(player->dexterity.getCur()) - bonus(target->dexterity.getCur())) * 2;

    // Having less then average dex has a major effect on circling chances.
    // This is being done because there is no save against circle, and there is one
    // against stun, thus making barbs and fighters extremely powerful comparatively
    // if this is not done. - TC
    if(player->dexterity.getCur()/10 < 9 && (target->dexterity.getCur()/10 > player->dexterity.getCur()/10))
        chance -= 10*(9 - (player->dexterity.getCur()/10));

    if(mTarget) {
        if(mTarget->isUndead() || mTarget->flagIsSet(M_RESIST_CIRCLE))
            chance -= (5 + target->getLevel()*2);
        chance = MIN(80, chance);

        mTarget->addEnemy(player);
    }



    if((mTarget && mTarget->flagIsSet(M_NO_CIRCLE)) || player->isBlind())
        chance = 1;
    //player->checkTarget(target);

    if(player->isCt()) chance = 101;

    if(Random::get(1,100) <= chance && (Random::get(1,100) > ((target->dexterity.getCur()/10)/2))) {
        if(mTarget) {
            if(!mTarget->flagIsSet(M_RESIST_CIRCLE) && !mTarget->isUndead())
                delay = MAX(6, (Random::get(6,10) + (MIN(3,((bonus(player->dexterity.getCur()) -
                                                             bonus(target->dexterity.getCur())) / 2)))));
            else
                delay = Random::get(6,9);
        } else
            delay = Random::get(6,10);



        target->stun(delay);

        player->print("You circle %N.\n", target);
        player->checkImprove("circle", true);
        log_immort(false, player, "%s circled %s.\n", player->getCName(), target->getCName());

        if(mTarget && player->isPlayer()) {
            if(mTarget->flagIsSet(M_YELLED_FOR_HELP) && (Random::get(1,100) <= (MAX(15, (mTarget->inUniqueRoom() ? mTarget->getUniqueRoomParent()->wander.getTraffic() : 15)/2)))) {
                mTarget->summonMobs(player);
                mTarget->clearFlag(M_YELLED_FOR_HELP);
                mTarget->setFlag(M_WILL_YELL_FOR_HELP);
            }

            if(mTarget->flagIsSet(M_WILL_YELL_FOR_HELP) && !mTarget->flagIsSet(M_YELLED_FOR_HELP)) {
                mTarget->checkForYell(player);
            }
        }

        target->print("%M circles you.\n", player);
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M circles %N.", player, target);
        player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY - 10);

        if(mTarget) {
            // A successful circle gives 5% of the target's max health as threat
            mTarget->adjustThreat(player, MAX<long>((long)(mTarget->hp.getMax()*0.05), 2));
        }


    } else {
        player->print("You failed to circle %N.\n", target);
        player->checkImprove("circle", false);
        target->print("%M tried to circle you.\n", player);
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to circle %N.", player, target);
        player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
        if(mTarget) {
            // An un-successful circle gives 2.5% of the target's max health as threat
            mTarget->adjustThreat(player, MAX<long>((long)(mTarget->hp.getMax()*0.025),1) );
        }
    }

    return(0);
}

//*********************************************************************
//                      cmdBash
//*********************************************************************
// This function allows fighters and berserkers to "bash" an opponent,
// doing less damage than a normal attack, but knocking the opponent
// over for a few seconds, leaving them unable to attack back.

int cmdBash(Player* player, cmd* cmnd) {
    Creature* creature=nullptr;
    Player  *pCreature=nullptr;
    long    t = time(nullptr);
    int     chance=0;
    double level=0.0;

    if(!player->ableToDoCommand())
        return(0);


    if(!player->knowsSkill("bash")) {
        player->print("You lack the skills to effectively bash anything!\n");
        return(0);
    }
    if(!player->isCt()) {
        if(!player->ready[WIELD-1] && player->getSize() < SIZE_LARGE) {
            player->print("You are too small to bash without a weapon.\n");
            return(0);
        }
        if(player->getPrimaryWeaponCategory() == "ranged") {
            player->print("You can't use a ranged weapon to bash someone!\n");
            return(0);
        }
    }
    if(!(creature = player->findVictim(cmnd, 1, true, false, "Bash whom?\n", "You don't see that here.\n")))
        return(0);

    if(creature)
        pCreature = creature->getAsPlayer();

    if(!player->canAttack(creature))
        return(0);

    if(!player->isCt()) {
        if(!pCreature) {
            if(creature->getAsMonster()->isEnemy(player)) {
                player->print("Not while you're already fighting %s.\n", creature->himHer());
                return(0);
            }
        }
    }

    if(!player->checkAttackTimer(true))
        return(0);

    player->updateAttackTimer();
    player->lasttime[LT_KICK].ltime = t;
    player->lasttime[LT_KICK].interval = (player->getPrimaryDelay()/10);

    if(player->getClass() == CreatureClass::CLERIC && player->getDeity() == ARES) {
        player->lasttime[LT_SPELL].ltime = t;
        player->lasttime[LT_SPELL].interval = 3L;
    }
    // All the logic to check if a weapon can hit the target is inside the attackCreature function
    // so only put anything specific to bash here, or check for ATTACK_BASH in attackCreature

    player->unhide();
    player->smashInvis();
    player->interruptDelayedActions();

    if(creature->isMonster()) {
        creature->getAsMonster()->addEnemy(player);
    }

    if(player->flagIsSet(P_LAG_PROTECTION_SET)) // Activates Lag protection.
        player->setFlag(P_LAG_PROTECTION_ACTIVE);

    level = player->getSkillLevel("bash");

    if(player->getClass() == CreatureClass::CLERIC && player->getSecondClass() == CreatureClass::FIGHTER)
        level = MAX(1, (int)level-2);
    else if(player->getClass() == CreatureClass::CLERIC && player->getDeity() == ARES)
        level = MAX(1, (int)level-3);


    chance = 50 + (int)((level-creature->getLevel())*10) +
             bonus(player->strength.getCur()) * 3 +
             (bonus(player->dexterity.getCur()) - bonus(creature->dexterity.getCur())) * 2;
    chance += player->getClass() == CreatureClass::BERSERKER ? 10:0;

    chance = player->getClass() == CreatureClass::BERSERKER ? MIN(90, chance) : MIN(85, chance);

    if(player->isBlind())
        chance = MIN(20, chance);

    if(creature->isMonster() && (player->ready[WIELD-1] &&
            player->ready[WIELD-1]->flagIsSet(O_ALWAYS_CRITICAL)) && creature->flagIsSet(M_NO_AUTO_CRIT))
        chance = 0; // automatic miss with autocrit weapon.

    if(player->isCt()) chance = 101;
    // For bash we have a bash chance, and then a normal attack miss chance
    if(Random::get(1,100) <= chance) {
        // We made the bash check, do the attack
        player->attackCreature(creature, ATTACK_BASH);
    }
    else {
        player->print("Your bash failed.\n");
        player->checkImprove("bash", false);
        creature->print("%M tried to bash you.\n", player);
        broadcast(player->getSock(),  creature->getSock(), creature->getRoomParent(),
            "%M tried to bash %N.", player, creature);
    }

    return(0);

}

//*********************************************************************
//                      cmdKick
//*********************************************************************
// This function allows fighters to kick their oopponents every 12
// seconds, in order to do a little extra damage. -- TC

int cmdKick(Player* player, cmd* cmnd) {
    Creature* creature=nullptr;
    long    i=0, t=0;
    int     chance=0;


    if(!player->ableToDoCommand())
        return(0);

    if(!player->isStaff()) {
        if(!player->knowsSkill("kick")) {
            player->print("You lack the skills to kick effectively.\n");
            return(0);
        }
    }

    if(!(creature = player->findVictim(cmnd, 1, true, false, "Kick whom?\n", "You don't see that here.\n")))
        return(0);


    if(!player->canAttack(creature))
        return(0);


    i = LT(player, LT_KICK);
    t = time(nullptr);

    if(i > t && !player->isDm()) {
        player->pleaseWait(i-t);
        return(0);
    }

    // Kick
    player->lasttime[LT_KICK].ltime = t;
    if(player->getClass() == CreatureClass::FIGHTER || player->getClass() == CreatureClass::MONK)
        player->lasttime[LT_KICK].interval = 12L;
    else
        player->lasttime[LT_KICK].interval = 15L;


    if(player->getClass() !=  CreatureClass::MONK) {
        player->lasttime[LT_DISARM].ltime = t;
        player->lasttime[LT_DISARM].interval = 6;
    }

    player->unhide();
    player->smashInvis();
    player->interruptDelayedActions();

    if(player->flagIsSet(P_LAG_PROTECTION_SET)) // Activates Lag protection.
        player->setFlag(P_LAG_PROTECTION_ACTIVE);

    if(creature->isMonster()) {
        creature->getAsMonster()->addEnemy(player);
    }

    chance = 40 + (int)((player->getSkillLevel("kick") - creature->getLevel())*10) +
             (3*bonus(player->dexterity.getCur()) - bonus(creature->dexterity.getCur())) * 2;

    if(player->getClass() == CreatureClass::ROGUE)
        chance -= (15 - (bonus(player->dexterity.getCur()) - bonus(creature->dexterity.getCur())));


    chance = MAX(0, MIN(95, chance));

    if(player->isBlind())
        chance = MIN(10, chance);

    if(creature->flagIsSet(M_NO_KICK))
        chance = 0;

    if(player->isDm())
        chance = 101;

    if(Random::get(1,100) <= chance) {
        // Like bash, two checks, first is to see if the kick was sucessfull, 2nd check
        // is to see if we actually hit the target

        player->attackCreature(creature, ATTACK_KICK);
    }
    else {
        player->print("Your kick was ineffective.\n");
        player->checkImprove("kick", false);
        creature->print("%M tried to kick you.\n", player);
        broadcast(player->getSock(),  creature->getSock(), creature->getRoomParent(), "%M tried to kick %N.", player, creature);
    }

    return(0);
}


//*********************************************************************
//                      scentTrack
//*********************************************************************
// determines if this player finds tracks by smell or by sight

bool scentTrack(const Player* player) {
    return(player->getClass() == CreatureClass::WEREWOLF || player->getRace() == MINOTAUR);
}

//*********************************************************************
//                      canTrack
//*********************************************************************

bool canTrack(const Player* player) {
    if(!player->ableToDoCommand())
        return(false);

    if(player->isBlind() && !scentTrack(player) && !player->isStaff()) {
        player->printColor("^CYou can't see any tracks! You're blind!\n");
        return(false);
    }

    if(!player->knowsSkill("track")) {
        player->print("You lack the proper training to find tracks.\n");
        return(false);
    }

    if(player->checkHeavyRestrict("find tracks"))
        return(false);

    if(player->flagIsSet(P_SITTING)) {
        player->print("You must stand up first!\n");
        return(false);
    }

    return(true);
}

//*********************************************************************
//                      doTrack
//*********************************************************************

void doTrack(Player* player) {
    Track*  track=nullptr;
    int     chance=0;
    int     skLevel = (int)player->getSkillLevel("track");

    player->clearFlag(P_AFK);
    player->unhide();

    if(!canTrack(player))
        return;

    if(player->inAreaRoom())
        track = player->getAreaRoomParent()->area->getTrack(&player->getAreaRoomParent()->mapmarker);
    else if(player->inUniqueRoom())
        track = &player->getUniqueRoomParent()->track;

    chance = 25 + (bonus(player->dexterity.getCur()) + skLevel) * 5;

    if(!track || Random::get(1,100) > chance) {
        player->print("You fail to find any tracks.\n");
        player->checkImprove("track", false);
        return;
    }

    if(track->getDirection().empty()) {
        player->print("There are no tracks in this room.\n");
        return;
    }

    if(scentTrack(player))
        player->printColor("You smell tracks leading to the %s^x.\n", track->getDirection().c_str());
    else
        player->printColor("You find tracks leading to the %s^x.\n", track->getDirection().c_str());

    if(skLevel >= 5 && track->getSize() != NO_SIZE)
        player->print("The tracks are %s.\n", getSizeName(track->getSize()).c_str());
    if(skLevel >= 10 && track->getNum())
        player->print("You find %d unique sets of tracks.\n", track->getNum());

    player->checkImprove("track", true);
}

void doTrack(const DelayedAction* action) {
    doTrack(action->target->getAsPlayer());
}

//*********************************************************************
//                      cmdTrack
//*********************************************************************
// This function is the routine that allows players to search
// for tracks in a room. If successful, they will be told what
// direction the last person who was in the room went.

int cmdTrack(Player* player, cmd* cmnd) {
    long    i=0, t=0;

    player->clearFlag(P_AFK);
    
    if(!canTrack(player))
        return(0);
    if(gServer->hasAction(player, ActionTrack)) {
        player->print("You are already looking for tracks!\n");
        return(0);
    }

    t = time(nullptr);
    i = LT(player, LT_TRACK);

    if(!player->isStaff() && t < i) {
        player->pleaseWait(i-t);
        return(0);
    }

    player->lasttime[LT_TRACK].ltime = t;
    player->lasttime[LT_TRACK].interval = 5 - bonus(player->dexterity.getCur());

    if(player->isStaff())
        player->lasttime[LT_TRACK].interval = 1;

    player->interruptDelayedActions();

    // big rooms take longer to search
    if(player->getRoomParent()->getSize() >= SIZE_HUGE) {
        // doSearch calls unhide, no need to do it twice
        player->unhide();

        gServer->addDelayedAction(doTrack, player, nullptr, ActionTrack, player->lasttime[LT_TRACK].interval);

        if(scentTrack(player)) {
            player->print("You begin sniffing for tracks.\n");
            broadcast(player->getSock(), player->getParent(), "%M sniffs for tracks.", player);
        } else {
            player->print("You begin searching for tracks.\n");
            broadcast(player->getSock(), player->getParent(), "%M searches for tracks.", player);
        }
    } else {
        if(scentTrack(player))
            broadcast(player->getSock(), player->getParent(), "%M sniffs for tracks.", player);
        else
            broadcast(player->getSock(), player->getParent(), "%M searches for tracks.", player);

        doTrack(player);
    }

    return(0);
}

//*********************************************************************
//                      cmdHarmTouch
//*********************************************************************
// This will allow death knights to harm touch once reaching 16th level.

int cmdHarmTouch(Player* player, cmd* cmnd) {
    Creature* creature=nullptr;
    int     num=0, chance=0, modifier=0;
    long    t=time(nullptr), i=0;


    player->clearFlag(P_AFK);

    if(!player->knowsSkill("harm")) {
        *player << "You are unable to use harm touch.\n";
        return(0);
    }

    if(player->getAdjustedAlignment() != BLOODRED && !player->isCt()) {
        *player << "The corruption in your soul needs to be more vile.\n";
        return(0);
    }

    i = player->lasttime[LT_LAY_HANDS].ltime + player->lasttime[LT_LAY_HANDS].interval;
      
    if(i > t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    if(!(creature = player->findVictim(cmnd, 1, true, false, "Harm-touch whom?\n", "You don't see that here.\n")))
        return(0);

    if(!player->canAttack(creature))
        return(0);

    player->unhide();
    player->smashInvis();
    player->interruptDelayedActions();

    if (creature->isEffected("resist-magic")) {
        if(!player->isCt() && Random::get(1,100) > 50) {
            *player << "Your harm touch failed!\n";
            player->checkImprove("harm", true);
            player->lasttime[LT_LAY_HANDS].interval = 45L;
            return(0);
        }
    }



    *creature << setf(CAP) << player << " touches you with " << player->hisHer() << " malevolent hand!\n";
    broadcast(player->getSock(),  creature->getSock(), creature->getRoomParent(), "%M touches %N with %s malevolent hand!", player, creature, player->hisHer());


    chance = 6500;

    modifier += ((player->getLevel() - creature->getLevel())*100);          //Level difference
    modifier += 20*(player->piety.getCur() - creature->piety.getCur());     //Piety difference

    chance += modifier;

    chance = MIN(9500,chance);

    if(player->isCt()) {
        *player << "Chance: " << chance << "\n";
        chance = 10000;
    }

    
    if(creature->isMonster())
        creature->getAsMonster()->addEnemy(player);

    if(Random::get(1,10000) <= chance) {
        //TODO: Adjust damage calc to use harm touch skill level once skill trainers are put in - for now, use player level
        num = Random::get( (int)(player->getLevel()*4), (int)(player->getLevel()*5) ) + Random::get(1,10);
        *player << "The darkness within you flows into " << creature << ".\n";
        *player << "Your harmful touch did " << ColorOn << player->customColorize("*CC:DAMAGE*") << num << ColorOff << " damage!\n";
        player->checkImprove("harm", true);
        if (creature->isPlayer() && (creature->getClass() != CreatureClass::LICH)) 
            *creature << setf(CAP) << player << "'s harmful touch tears through your body for " << ColorOn << creature->customColorize("*CC:DAMAGE*") << num << ColorOff << " damage!\n";
        player->lasttime[LT_LAY_HANDS].interval = 600L;

        player->statistics.attackDamage(num,"harm touch");

        if(creature->getClass() == CreatureClass::LICH) {
            if (creature->hp.getCur() < creature->hp.getMax()) {
                player->doHeal(creature, MIN<int>(num,(creature->hp.getMax() - creature->hp.getCur())));
                *player << "Your harmful touch healed " << creature << "!\n";
                *creature << "You are healed by " << setf(CAP) << player << "'s evil touch.\n";
            }
        } 
        else 
            player->doDamage(creature, num, CHECK_DIE);

    } else {
        *player << "Your harmful touch failed!\n";
        player->checkImprove("harm", false);
        *creature << player->upHisHer() << " harmful touch had no effect.\n";
        broadcast(player->getSock(),  creature->getSock(), creature->getRoomParent(), "%M's harmful touch had no effect.", player);
        player->lasttime[LT_LAY_HANDS].interval = 45L;
    }


    player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
    player->lasttime[LT_LAY_HANDS].ltime = t;
    player->lasttime[LT_SPELL].ltime = t;
    player->lasttime[LT_READ_SCROLL].ltime = t;
    if(LT(player, LT_RENOUNCE) <= t) {
        player->lasttime[LT_RENOUNCE].ltime = t;
        player->lasttime[LT_RENOUNCE].interval = 3L;
    }


    return(0);
}



//*********************************************************************
//                      cmdBloodsacrifice
//*********************************************************************
// This function allows Death Knights to drain over their max hp for
// 2 minutes out of every 10.

int cmdBloodsacrifice(Player* player, cmd* cmnd) {
    long    i=0, t=0;
    int     chance=0;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("bloodsac")) {
        player->print("You cannot sacrifice your blood.\n");
        return(0);
    }

    if(player->isEffected("bloodsac")) {
        player->print("Your last blood sacrifice is still in effect!\n");
        return(0);
    }

    i = player->lasttime[LT_BLOOD_SACRIFICE].ltime;
    t = time(nullptr);

    if(t - i < 600L && !player->isStaff()) {
        player->pleaseWait(600L-t+i);
        return(0);
    }

    int level = (int)player->getSkillLevel("bloodsac");
    chance = MIN(85, level * 20 + bonus(player->piety.getCur()));
    player->interruptDelayedActions();
    player->hp.decrease(10);

    if(player->hp.getCur() < 1) {
        player->hp.setCur(1);
        player->print("You run out of blood before your ritual completes.\n");
        chance = 0;
    }

    if(Random::get(1, 100) <= chance) {
        player->print("Your blood sacrifice infuses your body with increased vitality.\n");
        player->checkImprove("bloodsac", true);
        player->addEffect("bloodsac", 120L + 60L * (level / 5));
        player->lasttime[LT_BLOOD_SACRIFICE].ltime = t;
        player->lasttime[LT_BLOOD_SACRIFICE].interval = 600L;
    } else {
        player->print("Your sacrifice fails.\n");
        player->checkImprove("bloodsac", false);
        player->lasttime[LT_BLOOD_SACRIFICE].ltime = t - 590L;
    }

    broadcast(player->getSock(), player->getParent(), "%M sacrifices %s blood.",
        player, player->hisHer());
    return(0);
}
