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
#include "wanderInfo.hpp"              // for WanderInfo
 #include "commands.hpp"               // for isPtester()


//*********************************************************************
//                      disarmSelf
//*********************************************************************

void Player::disarmSelf() {
    bool removed = false;
    if(ready[WIELD-1] && ready[WIELD-1]->getType() == ObjectType::WEAPON && !ready[WIELD-1]->flagIsSet(O_CURSED)) {
        printColor("You removed %1P\n", ready[WIELD-1].get());
        broadcast(getSock(),  getRoomParent(), "%M removed %1P.", this, ready[WIELD-1].get());
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
            printColor("You removed %1P\n", ready[HELD-1].get());
            broadcast(getSock(),  getRoomParent(), "%M removed %1P.", this, ready[HELD-1].get());
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

int cmdDisarm(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature;
    std::shared_ptr<Player> pCreature=nullptr;
    std::shared_ptr<BaseRoom> room = player->getRoomParent();
    long    i, t;
    int     chance, drop;

    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num < 2) {
        player->disarmSelf();
        return(0);
    }

    if(!player->knowsSkill("disarm")) {
        *player <<"You are not skilled in the art of disarming your opponents.\n";
        return(0);
    }

    creature = room->findCreature(player, cmnd->str[1], cmnd->val[1], true, true);
    if(creature)
        pCreature = creature->getAsPlayer();
    if(!creature || creature == player || (pCreature && strlen(cmnd->str[1]) < 3)) {
        *player << "They aren't here.\n";
        return(0);
    }

    if(!player->canAttack(creature))
        return(0);

    if(player->isBlind()) {
        *player << ColorOn << "^CYou're blind!\n" << ColorOff;
        return(0);
    }

    if(!creature->ready[WIELD-1]) {
        *player << setf(CAP) << creature << " isn't wielding anything to disarm!\n";
        return(0);
    }

    if( !player->ready[WIELD-1] &&
        !player->checkStaff("You must be wielding a weapon in order to disarm %N.\n", creature.get())
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
    chance = std::min(chance, 85);


    if(creature->ready[WIELD-1]->flagIsSet(O_CURSED))
        chance = 3;

    if(creature->flagIsSet(M_UN_DODGEABLE))
        chance /= 4;

    if(player->isDm())
        chance = 100;

    if(creature->isCt() && !player->isDm())
        chance = 0;



    if(Random::get(1, 100) > chance) {
        player->print("You fail to disarm %N.\n", creature.get());
        player->checkImprove("disarm", false);
        creature->print("%M tried to disarm you!\n",player.get());
        broadcast(player->getSock(),  creature->getSock(), room, "%M attempts to disarm %N.\n", player.get(), creature.get());
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

        player->print("You disarm %N!\n", creature.get());
        broadcast(player->getSock(),  creature->getSock(), room, "%M attempts to disarm %N.", player.get(), creature.get());
        if( Random::get(1,100) <= drop && !creature->ready[WIELD-1]->flagIsSet(O_NO_DROP)) {
            player->printColor("You force %N to drop %P!\n", creature.get(), creature->ready[WIELD-1].get());
            creature->print("%M disarmed you!\n", player.get());

            broadcast(player->getSock(),  creature->getSock(), room, "%M dropped %s weapon!", creature.get(), player->hisHer());
            if(creature->isPlayer())
                creature->print("You drop your weapon!\n");

            std::shared_ptr<Object>  toDrop = creature->unequip(WIELD, UNEQUIP_NOTHING);
            toDrop->addToRoom(room);
            if(creature->isMonster())  {
                creature->setFlag(M_GUARD_TREATURE);   // Can't pick up weapon until you kill mob that dropped it.
            }
        } else {
            creature->print("%M disarmed you!\n", player.get());
            player->print("%s fumbles %s weapon!\n", creature->upHeShe(), creature->hisHer());
            broadcast(player->getSock(),  creature->getSock(), room, "%M fumbles %s weapon!", creature.get(), creature->hisHer());

            if(creature->ready[HELD-1] && creature->getClass() == CreatureClass::RANGER)
                creature->printColor("^gYou FUMBLE your primary weapon.\n");
            else
                creature->printColor("^gYou FUMBLE your weapon.\n");

            creature->unequip(WIELD);
        }
        creature->stun(2);
        if(creature->isPlayer())
            creature->unhide();
        else if(creature->isMonster())
            creature->getAsMonster()->addEnemy(player);
    }

    return(0);
}


//*********************************************************************
//                      cmdMistbane
//*********************************************************************

int cmdMistbane(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    i, t;
    int     chance;

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
    chance = std::min(80, (int)(player->getSkillLevel("mistbane") * 20) + bonus(player->piety.getCur()));


    if(Random::get(1, 100) > player->getLuck() + player->getSkillLevel("mistbane") * 2)
        chance = 10;

    if(Random::get(1, 100) <= chance) {
        player->print("You imbue yourself with positive energy.\n");
        broadcast(player->getSock(), player->getParent(), "%M imbues %sself with positive energy.", player.get(), player->himHer());
        player->checkImprove("mistbane", true);
        player->setFlag(P_MISTBANE);
        player->lasttime[LT_MISTBANE].ltime = t;
        player->lasttime[LT_MISTBANE].interval = 180L;

    } else {
        player->print("You failed to imbue yourself with positive energy.\n");
        broadcast(player->getSock(), player->getParent(), "%M tried to imbue %sself with positive energy.",
            player.get(), player->himHer());
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

int cmdSecond(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> object=nullptr;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
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

        object = player->findObject(player, cmnd, 1, true);
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
            player->printColor("%O isn't the right size for you.\n", object.get());
            return(0);
        }

        if(object->getMinStrength() > player->strength.getCur()) {
            player->printColor("You are currently not strong enough to wield %P.\n", object.get());
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

int cmdBerserk(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    i, t=time(nullptr), timeBetweenBerserks=480;
    int     chance;

    player->clearFlag(P_AFK);


    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("berserk")) {
        *player << "You don't know how to go berserk!\n";
        return(0);
    }

    if(player->isEffected("berserk")) {
        *player << "You are already going berserk.\n";
        return(0);
    }

    if(player->isEffected("strength")) {
        *player << "Your magically enhanced strength prevents you from going berserk.\n";
        return(0);
    }
    if(player->isEffected("enfeeblement")) {
        *player << "Your magically reduced strength prevents you from going berserk.\n";
        return(0);
    }
    if(player->isEffected("fortitude")) {
        *player << "Your magically enhanced health prevents you from going berserk.\n";
        return(0);
    }
    if(player->isEffected("weakness")) {
        *player << "Your magically reduced health prevents you from going berserk.\n";
        return(0);
    }

    if(player->flagIsSet(P_SITTING)) {
        player->clearFlag(P_SITTING);
        *player << "You stand up.\n";
    }

    i = player->lasttime[LT_BERSERK].ltime + player->lasttime[LT_BERSERK].interval;
     
      
    if(i > t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    chance = std::min<int>(8500, (player->getSkillLevel("berserk") * 1000) + (player->strength.getCur()) * 5);

    // At level 7+, the higher skill level at berserk, the less time it takes between berserks
    if (player->getLevel() >=7) {
        
        if (player->getClass() == CreatureClass::CLERIC && player->getDeity() == ARES)
            timeBetweenBerserks -= ((long)(player->getSkillLevel("berserk") * 3));
        else
            timeBetweenBerserks -= ((long)(player->getSkillLevel("berserk") * 4));
        } 
    // Never more than every 5 minutes, even if we increase MAX level
    if (timeBetweenBerserks < 300)
        timeBetweenBerserks = 300;   

    if(Random::get(1, 10000) <= chance) {
        *player << ColorOn << "^RYou go berserk!\n" << ColorOff;
        broadcast(player->getSock(), player->getParent(), "%M goes berserk!", player.get());
        player->checkImprove("berserk", true);

        // TODO: SKILLS: Add more modifiers based on berserk skill level - will save for full class overhaul
        if(player->getClass() == CreatureClass::CLERIC && player->getDeity() == ARES)
            player->addEffect("berserk", 120L, 50); // 2 min, +5 STR
        else
            player->addEffect("berserk", 150L, 50); // 2.5 min, +5 STR

        player->computeAC();
        player->computeAttackPower();
        player->lasttime[LT_BERSERK].ltime = t;
        player->lasttime[LT_BERSERK].interval = timeBetweenBerserks;

        if(player->isEffected("fear"))
            player->removeEffect("fear");

        if(player->isEffected("hold-person"))
            player->removeEffect("hold-person");
        else if(player->isEffected("hold-monster"))
            player->removeEffect("hold-monster");
        else if(player->isEffected("hold-undead"))
            player->removeEffect("hold-undead");

        if(player->flagIsSet(P_STUNNED)) {
            *player << ColorOn << "^yYou are no longer stunned.\n" << ColorOff;
            player->clearFlag(P_STUNNED);
            player->lasttime[LT_MOVED].interval = 0;
            player->lasttime[LT_PLAYER_STUNNED].interval = 0;
            player->lasttime[LT_SPELL].interval = 0;
            player->lasttime[LT_READ_SCROLL].interval = 0;
            player->setAttackDelay(0);
        }

        player->interruptDelayedActions();
    } else {
        *player << "You failed to channel your rage.\n";
        player->checkImprove("berserk", false);
        broadcast(player->getSock(), player->getParent(), "%M's rage wells up inside %s!", player.get(), player->himHer());
        player->lasttime[LT_BERSERK].ltime = t;
        player->lasttime[LT_BERSERK].interval = 10L;
    }
    return(0);
}


//*********************************************************************
//                      cmdCircle
//*********************************************************************
// This function allows fighters and barbarians to run circles about an
// enemy, confusing it for several seconds.

int cmdCircle(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> target;
    std::shared_ptr<Monster> mTarget;
    std::shared_ptr<Player> pTarget;
    int     chance, delay;
    double level;

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
                player->print("You are too fond of %N to do that.\n", pTarget.get());
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
        level = std::max(1, (int)level-2);
    if(player->getClass() == CreatureClass::CLERIC && player->getSecondClass() == CreatureClass::FIGHTER)
        level = std::max(1, (int)level-2);

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
        chance = std::min(80, chance);

        mTarget->addEnemy(player);
    }



    if((mTarget && mTarget->flagIsSet(M_NO_CIRCLE)) || player->isBlind())
        chance = 1;
    //player->checkTarget(target);

    if(player->isCt()) chance = 101;

    if(Random::get(1,100) <= chance && (Random::get(1,100) > ((target->dexterity.getCur()/10)/2))) {
        if(mTarget) {
            if(!mTarget->flagIsSet(M_RESIST_CIRCLE) && !mTarget->isUndead())
                delay = std::max(6, (Random::get(6,10) + (std::min(3,((bonus(player->dexterity.getCur()) -
                                                             bonus(target->dexterity.getCur())) / 2)))));
            else
                delay = Random::get(6,9);
        } else
            delay = Random::get(6,10);



        target->stun(delay);

        player->print("You circle %N.\n", target.get());
        player->checkImprove("circle", true);
        log_immort(false, player, "%s circled %s.\n", player->getCName(), target->getCName());

        if(mTarget && player->isPlayer()) {
            if(mTarget->flagIsSet(M_YELLED_FOR_HELP) && (Random::get(1,100) <= (std::max(15, (mTarget->inUniqueRoom() ? mTarget->getUniqueRoomParent()->wander.getTraffic() : 15)/2)))) {
                mTarget->summonMobs(player);
                mTarget->clearFlag(M_YELLED_FOR_HELP);
                mTarget->setFlag(M_WILL_YELL_FOR_HELP);
            }

            if(mTarget->flagIsSet(M_WILL_YELL_FOR_HELP) && !mTarget->flagIsSet(M_YELLED_FOR_HELP)) {
                mTarget->checkForYell(player);
            }
        }

        target->print("%M circles you.\n", player.get());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M circles %N.", player.get(), target.get());
        player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY - 10);

        if(mTarget) {
            // A successful circle gives 5% of the target's max health as threat
            mTarget->adjustThreat(player, std::max<long>((long)(mTarget->hp.getMax()*0.05), 2));
        }


    } else {
        player->print("You failed to circle %N.\n", target.get());
        player->checkImprove("circle", false);
        target->print("%M tried to circle you.\n", player.get());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to circle %N.", player.get(), target.get());
        player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
        if(mTarget) {
            // An un-successful circle gives 2.5% of the target's max health as threat
            mTarget->adjustThreat(player, std::max<long>((long)(mTarget->hp.getMax()*0.025),1) );
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

int cmdBash(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature;
    std::shared_ptr<Player> pCreature=nullptr;
    long    i=0, t = time(nullptr);
    int     chance;
    double level;

    if(!player->ableToDoCommand())
        return(0);


    if(!player->knowsSkill("bash")) {
        *player << "You lack the skills to effectively shield bash enemies.\n";
        return(0);
    }

    if(!player->isCt() && !player->ready[SHIELD-1]) {
        *player << "Shield bashing enemies requires equipping a shield.\n";
        return(0);
    }

    if(!(creature = player->findVictim(cmnd, 1, true, false, "Shield bash whom?\n", "You don't see that here.\n")))
        return(0);

    if(creature)
        pCreature = creature->getAsPlayer();

    if(!player->canAttack(creature))
        return(0);

     // Certain mtypes cannot be smashed, and colossal and gargantuan mobs cannot ever be smashed
    if (creature->isMonster()) {
        switch(creature->getType()) {
        case ETHEREAL:
        case ENERGY:
        case GASEOUS:
        case INSECT:
        case SLIME:
        case PUDDING:
            *player << ColorOn << "^y" << "Creatures of type '" << monType::getName(creature->getType()) << "' cannot be shield bashed!\n" << ColorOff;
            return(0);
            break;
        default:
            break;
        }

        if(creature->getSize() == SIZE_COLOSSAL || creature->getSize() == SIZE_GARGANTUAN) {
            *player << "Are you kidding? " << setf(CAP) << creature << " is of " << 
                        getSizeName(creature->getSize()) << " size! You can't shield bash " << creature->himHer() << "!\n";
            return(0);
        }
    }

    i = LT(player, LT_BASH);
    t = time(nullptr);

    if(i > t && !player->isCt()) {
        player->pleaseWait(i - t);
        return(0);
    }

    level = player->getSkillLevel("bash");
    if(player->getClass() == CreatureClass::CLERIC && player->getSecondClass() == CreatureClass::FIGHTER)
        level = std::max(1, (int)level-2);
    else if(player->getClass() == CreatureClass::CLERIC && player->getDeity() == ARES)
        level = std::max(1, (int)level-3);
    else if((player->getClass() == CreatureClass::FIGHTER && player->getSecondClass() != CreatureClass::NONE) || player->getClass() == CreatureClass::PALADIN)
        level = std::max(1, (int)level-1);

    long bashDelay = 0;
    if (level <= 40)
        bashDelay = 13L;
    else if (level <= 30)
        bashDelay = 14L;
    else if (level <= 20)
        bashDelay = 15L;
    else if (level <= 10)
        bashDelay = 16L;
    
    player->updateAttackTimer();
    player->lasttime[LT_BASH].ltime = t;
    player->lasttime[LT_BASH].interval = bashDelay;

    i = LT(player, LT_SPELL);
    if (t >= i) 
        player->lasttime[LT_SPELL].interval = 3L;

    if(player->knowsSkill("slam")) {
        i = LT(player, LT_SLAM);
        if (t >= i) 
            player->lasttime[LT_SLAM].interval = player->getPrimaryDelay()/10;
    }

    if(player->knowsSkill("gore")) {
        i = LT(player, LT_GORE);
        if (t >= i) 
            player->lasttime[LT_GORE].interval = player->getPrimaryDelay()/10;
    }

    if(player->knowsSkill("smash")) {
        i = LT(player, LT_SMASH);
        if (t >= i) 
            player->lasttime[LT_SMASH].interval = 3L;
    }

    //player->lasttime[LT_KICK].ltime = t;
    //player->lasttime[LT_KICK].interval = (player->getPrimaryDelay()/10);
    //player->lasttime[LT_GORE].ltime = t;
    //player->lasttime[LT_GORE].interval = (player->getPrimaryDelay()/10);

    // All the logic to check if a weapon can hit the target is inside the attackCreature function
    // so only put anything specific to bash here, or check for ATTACK_BASH in attackCreature

    player->unhide();
    player->smashInvis();
    player->interruptDelayedActions();

    chance = 50 + (int)((level-creature->getLevel())*10) +
             bonus(player->strength.getCur()) * 3 +
             (bonus(player->dexterity.getCur()) - bonus(creature->dexterity.getCur())) * 2;
    chance += player->getClass() == CreatureClass::BERSERKER ? 10:0;

    chance = player->getClass() == CreatureClass::BERSERKER ? std::min(90, chance) : std::min(85, chance);

    if(player->isBlind())
        chance = std::min(20, chance);

    if(creature->isMonster()) {
        creature->getAsMonster()->addEnemy(player);

        if(player->flagIsSet(P_LAG_PROTECTION_SET)) // Activates Lag protection.
            player->setFlag(P_LAG_PROTECTION_ACTIVE);

        if (player->ready[SHIELD-1]->flagIsSet(O_ALWAYS_CRITICAL) && creature->flagIsSet(M_NO_AUTO_CRIT))
            chance = 0; // automatic miss if autocrit is set on shield.

        if (creature->flagIsSet(M_NO_BASH))
            chance = 0;

        // A shield bash, regardless of outcome, adds 2.5% of target's max hp to threat
        creature->getAsMonster()->adjustThreat(player, std::max<long>((long)(creature->hp.getMax()*0.025), 2));

    }

    if(player->isCt()) chance = 101;
    // For bash we have a bash chance, and then a normal attack miss chance
    if(Random::get(1,100) <= chance) {
        // We made the bash check, do the attack
        player->attackCreature(creature, ATTACK_BASH);

    }
    else {
        player->print("Your shield bash had no effect.\n");
        player->checkImprove("bash", false);
        creature->print("%M tried to shield bash you.\n", player.get());
        broadcast(player->getSock(),  creature->getSock(), creature->getRoomParent(),
            "%M tried to shield bash %N.", player.get(), creature.get());
    }

    return(0);

}

//*********************************************************************
//                      cmdSlam
//*********************************************************************
// This function allows a player to "slam" an opponent, doing a bit of
// extra damage, and possibly stunning them for 1-2 seconds. It is essentially
// like a pommel strike with a weapon, so the weapon subtype used is relevent.

int cmdSlam(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature;
    long    i=0, t = time(nullptr);
    int     chance;
    double level;
    bool noSlam = false;

    if(!player->ableToDoCommand())
        return(0);


    std::shared_ptr<Object> weapon = player->ready[WIELD-1];
    std::string category = player->getPrimaryWeaponCategory();

    if (weapon) {
        noSlam = (category == "ranged");
    }

    if(!player->isCt()) {

        if(!player->knowsSkill("slam")) {
            *player << "You lack the skills to effectively slam with a weapon.\n";
            return(0);
        }

        if(!weapon) {
            *player << "Slamming an opponent requires a wielded primary weapon.\n";
            return(0);

        }
        else if ((noSlam && !weapon->flagIsSet(O_CAN_USE_SLAM)) || weapon->flagIsSet(O_NO_SLAM) ) {
            *player << ColorOn << "You cannot slam an opponent with " << weapon << ".\n" << ColorOff;
            return(0);
        }

    }

    
    if(!(creature = player->findVictim(cmnd, 1, true, false, "Slam whom?\n", "You don't see that here.\n")))
        return(0);

    if(!player->canAttack(creature))
        return(0);

     // Certain mtypes cannot be slammed, and colossal and gargantuan mobs cannot ever be slammed
    if (creature->isMonster()) {
        switch(creature->getType()) {
        case ETHEREAL:
        case ENERGY:
        case GASEOUS:
        case INSECT:
        case SLIME:
        case PUDDING:
            *player << ColorOn << "^y" << "Creatures of type '" << monType::getName(creature->getType()) << "' cannot be slammed!\n" << ColorOff;
            return(0);
            break;
        default:
            break;
        }

        if(creature->getSize() == SIZE_COLOSSAL || creature->getSize() == SIZE_GARGANTUAN) {
            *player << "Are you kidding? " << setf(CAP) << creature << " is of " << 
                        getSizeName(creature->getSize()) << " size! You can't slam " << creature->himHer() << "!\n";
            return(0);
        }
    }

    i = LT(player, LT_SLAM);
    t = time(nullptr);

    if(i > t && !player->isCt()) {
        player->pleaseWait(i - t);
        return(0);
    }

    level = player->getSkillLevel("slam");

    switch (player->getClass()) {
    case CreatureClass::FIGHTER:
        if(player->getSecondClass() != CreatureClass::NONE)
            level = std::max(1, (int)level-1);
        break;
    case CreatureClass::PALADIN:
         level = std::max(1, (int)level-1);
         break;    
    case CreatureClass::CLERIC:
        if(player->getDeity() == ARES || player->getDeity() == LINOTHAN)
            level = std::max(1, (int)level-2);
        break;
    case CreatureClass::THIEF:
    case CreatureClass::ASSASSIN:
    case CreatureClass::ROGUE:
         level = std::max(1, (int)level-3);
         break;
    default:
        break;
    }

    long slamDelay = 0;
    if (level <= 40)
        slamDelay = 9L;
    else if (level <= 30)
        slamDelay = 10L;
    else if (level <= 20)
        slamDelay = 11L;
    else if (level <= 10)
        slamDelay = 12L;

    if(player->getClass() != CreatureClass::FIGHTER && player->getClass() != CreatureClass::BERSERKER)
        slamDelay += 2L;
    
    player->updateAttackTimer();
    player->lasttime[LT_SLAM].ltime = t;
    player->lasttime[LT_SLAM].interval = slamDelay;

    i = LT(player, LT_SPELL);
    if (t >= i) 
        player->lasttime[LT_SPELL].interval = 3L;

    if(player->knowsSkill("gore")) {
        i = LT(player, LT_GORE);
        if (t >= i) 
            player->lasttime[LT_GORE].interval = player->getPrimaryDelay()/10;
    }

    if(player->knowsSkill("smash")) {
        i = LT(player, LT_SMASH);
        if (t >= i) 
            player->lasttime[LT_SMASH].interval = 3L;
    }

    // All the logic to check if a weapon can hit the target is inside the attackCreature function
    // so only put anything specific to bash here, or check for ATTACK_BASH in attackCreature

    player->unhide();
    player->smashInvis();
    player->interruptDelayedActions();

    chance = 50 + (int)((level-creature->getLevel())*10) +
             bonus(player->strength.getCur()) * 3 +
             (bonus(player->dexterity.getCur()) - bonus(creature->dexterity.getCur())) * 2;
    chance += player->getClass() == CreatureClass::BERSERKER ? 10:0;

    chance = player->getClass() == CreatureClass::BERSERKER ? std::min(90, chance) : std::min(85, chance);

    if(player->isBlind())
        chance = std::min(20, chance);

    if(creature->isMonster()) {
        creature->getAsMonster()->addEnemy(player);

        if(player->flagIsSet(P_LAG_PROTECTION_SET)) // Activates Lag protection.
            player->setFlag(P_LAG_PROTECTION_ACTIVE);

        if (player->ready[WIELD-1]->flagIsSet(O_ALWAYS_CRITICAL) && creature->flagIsSet(M_NO_AUTO_CRIT))
            chance = 0; // automatic miss if autocrit is set on weapon.

        if (creature->flagIsSet(M_NO_SLAM))
            chance = 0;

    }

    if(player->isCt()) chance = 101;
    // For slam we have a slam chance, and then a normal attack miss chance
    if(Random::get(1,100) <= chance) {
        // We made the slam check, do the attack
        player->attackCreature(creature, ATTACK_SLAM);
        // A successful slam , regardless of whether it does damge, adds 1% of target's max hp to threat
        creature->getAsMonster()->adjustThreat(player, std::max<long>((long)(creature->hp.getMax()*0.01), 2));

    }
    else {
        player->print("Your slam was ineffective.\n");
        player->checkImprove("slam", false);
        creature->print("%M tried to slam you.\n", player.get());
        broadcast(player->getSock(),  creature->getSock(), creature->getRoomParent(),
            "%M tried to slam %N.", player.get(), creature.get());
    }

    return(0);

}

//****************************************************************************
//                      cmdGore
//****************************************************************************
// This command allows a player to use a gore attack on their opponents. 
// The skill is originally designed for minotaurs and is set in the races.xml 
// file in configs. No check for minotaur is made in this code, as there
// might be future instances where we allow gore skill outside minotaurs.

int cmdGore(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature;
    long    i=0, t=0;
    int     chance;


    if(!player->ableToDoCommand())
        return(0);


    if(!player->isStaff()) {
        if(!player->knowsSkill("gore")) {
            *player << "You don't know how to gore your opponent.\n";
            return(0);
        }
    }

    if(!(creature = player->findVictim(cmnd, 1, true, false, "Gore whom?\n", "You don't see that here.\n")))
        return(0);

    if(!player->canAttack(creature))
        return(0);

    //player->updateAttackTimer();

    i = LT(player, LT_GORE);
    t = time(nullptr);

    if(i > t && !player->isCt()) {
        player->pleaseWait(i - t);
        return(0);
    }

    // Gore
    long goreInterval = 0;
    int goreSkill = (int)player->getSkillLevel("gore");

    // Time between use decreases with skill level
    if (goreSkill >= 40)
        goreInterval = 14;
    else if (goreSkill >= 35)
        goreInterval = 15;
    else if (goreSkill >= 30)
        goreInterval = 16;
    else if (goreSkill >= 25)
        goreInterval = 17;
    else if (goreSkill >= 20)
        goreInterval = 18;
    else if (goreSkill >= 15)
        goreInterval = 19;
    else
        goreInterval = 20;

    player->lasttime[LT_GORE].ltime = t;
    player->lasttime[LT_GORE].interval = goreInterval;
    

    i = LT(player, LT_SPELL);
    if (t >= i) 
        player->lasttime[LT_SPELL].interval = 3L;

    if(player->knowsSkill("slam")) {
        i = LT(player, LT_SLAM);
        if (t >= i) 
            player->lasttime[LT_SLAM].interval = player->getPrimaryDelay()/10;
    }

    if(player->knowsSkill("kick")) {
        i = LT(player, LT_KICK);
        if (t >= i) 
            player->lasttime[LT_KICK].interval = player->getPrimaryDelay()/10;
    }

    if(player->knowsSkill("smash")) {
        i = LT(player, LT_SMASH);
        if (t >= i) 
            player->lasttime[LT_SMASH].interval = player->getPrimaryDelay()/10;
    }

    player->unhide();
    player->smashInvis();
    player->interruptDelayedActions();
    
    if(creature->isMonster()) {
        creature->getAsMonster()->addEnemy(player);

        if(player->flagIsSet(P_LAG_PROTECTION_SET)) // Activates Lag protection.
            player->setFlag(P_LAG_PROTECTION_ACTIVE);

    }

    // Agility = avg of dex + con
    chance = 400 + (5*((int)player->getSkillGained("gore") - creature->getLevel()*10)) + ((player->getAgility()*3) - (creature->getAgility()*2));

    if(player->isBlind())
        chance = std::min(50, chance);

    if(creature->isMagicallyHeld())
        chance *=2;

    // For every size difference 2 and beyond, chance is roughly 20% less.
    short sizeDiff = abs(player->getSize() - creature->getSize());
    if (sizeDiff > 1)
        chance -= sizeDiff*200;

    chance = std::max(0, std::min(950, chance));

    if (creature->isMonster()) {
        switch(creature->getType()) {
        case ETHEREAL:
        case ENERGY:
        case GASEOUS:
        case INSECT:
        case SLIME:
        case PUDDING:
        case UNDEAD: // NOTE: Only intelligent/powerful undead should be set to monType UNDEAD. 
                     // Other undead, like zombies or skeletons, etc, should be HUMANOID or other and
                     // have the M_UNDEAD mflag set. Those WOULD be affected by a gore attack.
            *player << ColorOn << "^R" << setf(CAP) << creature << " is unaffected by gore attacks!\n" << ColorOff;
            chance = 0;
            break;
        default:
            break;
        }
    }

    if(creature->flagIsSet(M_NO_GORE))
        chance = 0;

    if(player->isCt())
        chance = 1001;

    if(Random::get(1,1000) <= chance) {
        // Like bash, two checks, first is to see if the gore was sucessful, 2nd check
        // is to see if we actually hit the target

        player->attackCreature(creature, ATTACK_GORE);
    }
    else {
        *player << ColorOn << "^RYour gore attack was ineffective.\n" << ColorOff;
        player->checkImprove("gore", false);
        *creature << ColorOn << "^R" << setf(CAP) << player << " tried to gore you!\n" << ColorOff;
        broadcast(player->getSock(),  creature->getSock(), creature->getRoomParent(), "^R%M tried to gore %N!^x", player.get(), creature.get());
    }

    return(0);
}

//******************************************************************************************
//                      cmdSmash
//******************************************************************************************
// This allows some larger races to smash their opponents every so often, either with or without
// a weapon. It has a chance to stun, and size differences with attacker, target, and the
// weapon used also are considered. A smash attack is immune to parrying,
// turning those into either a miss or a dodge instead. The chance to block an attack is 
// still there, but difference between attacker and target size are considered. A successful
// smash has a chance to stun, but that also has bonus/penalty depending on size difference.
// As of v2.63, only ogres and half-giants have this ability. -TC

int cmdSmash(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> pTarget=nullptr;
    std::shared_ptr<Creature> target=nullptr;
    int      n=0;
    int     squish=0, dur=0, dmg=0;
    long    i=0, t=0;
    float   smashMod = 0.0;
    Damage damage;

     if(!player->ableToDoCommand())
        return(0);

    if(!player->isStaff()) {
        if(!player->knowsSkill("smash")) {
            *player << "You do not have the ability to smash your enemies.\n";
            return(0);
        }

        if(!player->flagIsSet(P_PTEST_SMASH)) {
            *player << "The smash ability is only available for ptesting right now.\n";
            return(0);
        }
    }



    std::shared_ptr<Object>  weapon = player->ready[WIELD - 1];
    std::string category = player->getPrimaryWeaponCategory();


    if(!player->isCt()) {
        if(weapon && category != "crushing" && !weapon->flagIsSet(O_CAN_USE_SMASH) && !player->isStaff()) {
            *player << setf(CAP) << weapon << " is not suitable for smashing! You need a crushing weapon or no weapon at all!\n";
            return(0);
        }
    }

    int weaponSizeDiff = 0;

    // Size of weapon being used compared to size of attacker matters
    if(weapon) {
       
        weaponSizeDiff = (weapon->getSize()?(player->getSize() - weapon->getSize()):0);

        if(abs(weaponSizeDiff) >= 2 && !player->isStaff()) {
            *player << ColorOn << "^y" << setf(CAP) << weapon << " is too " 
                    << (weaponSizeDiff>0?"small and awkward":"large and unwieldy") << " for you to smash with.^x\n" << ColorOff;
            return(0);
        } 
        
        if (weapon->flagIsSet(O_NO_SMASH) && !player->checkStaff("%O cannot be used to smash.\n", weapon.get()))
            return(0);
    }

    if(!(target = player->findVictim(cmnd, 1, true, false, "Smash what?\n", "You don't see that here.\n")))
        return(0);

    if(!player->canAttack(target))
        return(0);

    // Certain mtypes cannot be smashed, and colossal and gargantuan mobs cannot ever be smashed
    if (target->isMonster()) {
        switch(target->getType()) {
        case ETHEREAL:
        case ENERGY:
        case GASEOUS:
        case INSECT:
        case SLIME:
        case PUDDING:
            *player << ColorOn << "^y" << "Creatures of type '" << monType::getName(target->getType()) << "' cannot be smashed!\n" << ColorOff;
            return(0);
            break;
        default:
            break;
        }

        if(target->getSize() == SIZE_COLOSSAL || target->getSize() == SIZE_GARGANTUAN) {
            *player << "Are you kidding? " << setf(CAP) << target << " is of " << 
                        getSizeName(target->getSize()) << " size! You can't smash " << target->himHer() << "!\n";
            return(0);
        }
    }

    player->updateAttackTimer();

    i = LT(player, LT_SMASH);
    t = time(nullptr);

    if(i > t && !player->isCt()) {
        player->pleaseWait(i - t);
        return(0);
    }

    long smashInterval = 0;
    int smashSkill = (int)player->getSkillLevel("smash");

    // Time between smashes will decrease slightly with increased skill level
    if (smashSkill >= 40)
        smashInterval = 155;
    else if (smashSkill >= 30)
        smashInterval = 160;
    else if (smashSkill >= 20)
        smashInterval = 165;
    else if (smashSkill >= 10)
        smashInterval = 170;
    else
        smashInterval = 180;

    if(player->isStaff() || player->flagIsSet(P_PTESTER))
        smashInterval = 3;

    player->lasttime[LT_SMASH].ltime = t;
    player->lasttime[LT_SMASH].interval = smashInterval;

    
    i = LT(player, LT_SPELL);
    if (t >= i) 
        player->lasttime[LT_SPELL].interval = 6L;
    
    if(player->knowsSkill("slam")) {
        i = LT(player, LT_SLAM);
        if (t >= i) 
            player->lasttime[LT_SLAM].interval = 6L;
    }

    if(player->knowsSkill("bash")) {
        i = LT(player, LT_BASH);
        if (t >= i) 
            player->lasttime[LT_BASH].interval = 6L;
    }

    if(player->knowsSkill("kick")) {
        i = LT(player, LT_KICK);
        if (t >= i) 
            player->lasttime[LT_KICK].interval = 6L;
    }

    if(player->knowsSkill("gore")) {
        i = LT(player, LT_GORE);
        if (t >= i) 
            player->lasttime[LT_GORE].interval = 6L;
    }


    if(target->isMonster()) {
        target->getAsMonster()->addEnemy(player);

        if(player->flagIsSet(P_LAG_PROTECTION_SET)) // Activates Lag protection.
            player->setFlag(P_LAG_PROTECTION_ACTIVE);
    }

    // If using a weapon, allow for the weapon breaking
    if(weapon && player->breakObject(weapon, WIELD)) {
        *player << ColorOn << "^yYour SMASH missed!\n" << ColorOff;
        broadcast(player->getSock(), player->getParent(), "^y%M's SMASH missed!^x", player.get());
        player->lasttime[LT_SMASH].ltime = t;
        player->lasttime[LT_SMASH].interval = 12L;
        return(0);
    }
   
    int skillLevel=0, wpnSkill=0, modifier=0;
    
    wpnSkill = player->getWeaponSkill(weapon)/10;

    skillLevel = (smashSkill + wpnSkill)/2;

    if(player->flagIsSet(P_PTESTER) || player->flagIsSet(P_PTEST_SMASH))
        *player << ColorOn << "^DwpnSkill: " << wpnSkill << "\nsmashSkill: " << smashSkill << "\nInitial skillLevel: " << skillLevel << "\n" << ColorOff;

    //High brutality (avg of str + con) gives a bonus
    if(player->getBrutality() > 200) {
        //modifier = 300 - player->getBrutality()/10;
        modifier = (player->getBrutality() - 200)/10;
        skillLevel += modifier;
    }
    //Low brutality (avg of str + con) gives a penalty
    else if (player->getBrutality() < 100) {
         modifier = (100 - player->getBrutality()) / 10;
        skillLevel = std::max(1,skillLevel - modifier);
    }

    // Now modify by level, since we dont use computeBonus for damage.
    if (player->getLevel() > target->getLevel())
        skillLevel += player->getLevel() - target->getLevel();

    if(player->flagIsSet(P_PTESTER) || player->flagIsSet(P_PTEST_SMASH))
        *player << ColorOn << "^DBrutality: " << player->getBrutality() << "\nModifier: " << modifier << "\nModified skillLevel: " << skillLevel << "\n" << ColorOff;

    // Much harder to smash opponents more than 1 size larger than you are, effectively reducing skill level
    short sizeDiff = target->getSize() - player->getSize();
    if (sizeDiff > 1)
        skillLevel -= (skillLevel*(25*sizeDiff))/100;

    //Using a wrong-sized weapon will have consequences also
    if(weapon && abs(weaponSizeDiff) > 0)
        skillLevel -= (skillLevel*10)/100;

    AttackResult result = player->getAttackResult(target, weapon, (weapon?NO_FLAG:NO_FUMBLE), skillLevel);

    // On occasion, mobtype or size is not enough to customize, so some mobs can be flagged so they're not able to be smashed.
    // If that happens, the attack will be converted into a miss.
    if(!pTarget && target->flagIsSet(M_NO_SMASH) && result != ATTACK_MISS)
        result = ATTACK_MISS;

    if(player->isStaff() || player->flagIsSet(P_PTESTER) || player->flagIsSet(P_PTEST_SMASH)) {
        *player << ColorOn << "^DskillLevel = " << skillLevel << "\n";
        if (result == ATTACK_MISS)
            *player << "result = ATTACK_MISS\n";
        else if (result == ATTACK_HIT)
            *player << "result = ATTACK_HIT\n";
        else if (result == ATTACK_PARRY)
            *player << "result = ATTACK_PARRY\n";
        else if (result == ATTACK_BLOCK)
            *player << "result = ATTACK_BLOCK\n";
        else if (result == ATTACK_GLANCING)
            *player << "result = ATTACK_GLANCING\n";
        else 
            *player << "result = other\n";
        *player << "^x" << ColorOff;
        }

    player->smashInvis();
    player->unhide();
    player->interruptDelayedActions();

    std::string with = Statistics::damageWith(player, weapon);

    if(player->isDm() && result != ATTACK_CRITICAL)
        result = ATTACK_HIT;

    player->statistics.swing();

    // A blocked attack can turn into a hit depending on size difference between attacker and target
    if (result == ATTACK_BLOCK && target->getSize() < player->getSize()) {
        *player << ColorOn << "^y" << setf(CAP) << target << " was unable to block your smash!\n" << ColorOff;
        *target << ColorOn << "^yYou were unable to block " << player << " 's SMASH!\n" << ColorOff;
        result = ATTACK_HIT;
    }
    // An ATTACK_PARRY result will turn into a dodge if target has high enough elusiveness (dex,int average), otherwise, normal hit
    else if (result == ATTACK_PARRY) {
        *player << ColorOn << "^y" << setf(CAP) << target << " tried to parry, but " << target->heShe() << " failed!\n" << ColorOff;
        *target << ColorOn << "^yYou tried to parry " << player << " 's SMASH, but you failed!\n" << ColorOff;

        if (Random::get(1,5) == 1 && target->getElusiveness() >= 160)
            result = ATTACK_DODGE;
        else
            result = ATTACK_HIT;
    }


    if(result == ATTACK_HIT || result == ATTACK_CRITICAL || result == ATTACK_BLOCK || result == ATTACK_GLANCING) {
        
        // Smash does base damage generally 1.2x-2.0x either what attacker's unarmed attack 
        // damage is (no weapon), or damage of the wielded weapon if a weapon is being used
        smashMod = Random::get(1.5,3.0);

        int drain = 0;
        bool wasKilled = false, meKilled = false;
        // Return of 1 means the weapon was shattered or otherwise rendered unsuable
        if(player->computeDamage(target, weapon, ATTACK_SMASH, result, damage, false, drain, smashMod) == 1) {
            player->unequip(WIELD, UNEQUIP_DELETE);
            weapon = nullptr;
            player->computeAttackPower();
        }
        damage.includeBonus();


        n = damage.get();

        if(result == ATTACK_CRITICAL)
            player->statistics.critical();
        else
            player->statistics.hit();

        if(target->isPlayer())
            target->getAsPlayer()->statistics.wasHit();


        if(target->isMonster()) {
            // A successful smash hit, gives 10% of the target's max health as threat
            target->getAsMonster()->adjustThreat(player, std::max<long>((long)(target->hp.getMax()*0.1), 2));
        }


        squish = target->hp.getCur() * 2;

        *player << ColorOn << "^YYou SMASHED " << target << " for " << player->customColorize("*CC:DAMAGE*") << damage.get() << "^Y damage!^x\n" << ColorOff;
       
        log_immort(false,player, "%s SMASHED %s for %d damage.\n", player->getCName(), target->getCName(), damage.get());

        broadcast(player->getSock(), player->getParent(), "^Y%M SMASHED %M!^x", player.get(), target.get());

        *target << ColorOn << "^Y" << player << " SMASHED you" << 
                        (target->isBrittle() ? "r brittle body" : "") << " for " << 
                                            target->customColorize("*CC:DAMAGE*") << damage.get() << "^Y damage!^x\n" << ColorOff;

        broadcastGroup(false, target, "^Y%M SMASHED %N for *CC:DAMAGE*%d^Y damage!, %s%s^x\n",
            player.get(), target.get(), damage.get(), target->heShe(), target->getStatusStr(damage.get()));

        player->statistics.attackDamage(damage.get(), with);

        
        if( weapon && weapon->getMagicpower() &&
            weapon->flagIsSet(O_WEAPON_CASTS) &&
            Random::get(1,100) <= 10 && weapon->getChargesCur() > 0)
        {
             n += player->castWeapon(target, weapon, meKilled);
        }

        meKilled = player->doReflectionDamage(damage, target) || meKilled;

        player->doDamage(target, damage.get(), NO_CHECK);
        wasKilled = target->hp.getCur() < 1;

        // If killed, check for squishy squish special output!
        if(wasKilled && n > squish && Random::get(1,100) <= 50) {
            switch(Random::get(1,4)) {
            case 1:
                *player << ColorOn << "^RYou completely pulverized " << target << "!\n" << ColorOff;
                broadcast(player->getSock(), player->getParent(), "%M completely pulverized %N!",player.get(), target.get());
                if(target->isPlayer())
                    *target << ColorOn << "^R" << setf(CAP) << player << " completely pulverized you! You're dead!\n" << ColorOff;
                break;
            case 2:
                *player << ColorOn << "^RYou battered " << target << " into a pulpy unrecognizeable mess!\n" << ColorOff;
                broadcast(player->getSock(), player->getParent(), "^R%M battered %N into a pulpy unrecognizeable mess!^x",player.get(), target.get());
                if(target->isPlayer())
                    *target << ColorOn << "^R" << setf(CAP) << player << " battered you into a pulpy unrecognizeable mess! You're dead!\n" << ColorOff;

                break;
            case 3:
                if(weapon) {
                    *player << ColorOn << "^R" << setf(CAP) << weapon << " ^Rsmashed " << target << " into mush!\n" << ColorOff;
                    broadcast(player->getSock(), player->getParent(), "^R%M's %s smashed %N into mush!^x", player.get(), weapon->getCName(), target.get());

                    if(target->isPlayer())
                        *target << ColorOn << "^R" << setf(CAP) << player << "'s " << weapon << " ^Rsmashed you into mush! You're dead!\n" << ColorOff;
                }
                else
                {
                    *player << ColorOn << "^RYou smashed " << target << " into mush!\n" << ColorOff;
                    broadcast(player->getSock(), player->getParent(), "^R%M smashed %N into mush!^x",
                        player.get(), target.get());

                    if(target->isPlayer())
                        *target << ColorOn << "^R" << setf(CAP) << player << " smashed you into mush! You're dead!\n" << ColorOff;
                }

                break;
            case 4:
                *player << ColorOn << "^RYou nearly flattened " << target << " into the ground!\n" << ColorOff;
                broadcast(player->getSock(), player->getParent(), "^R%M nearly flattened %N into the ground!^x",player.get(), target.get());
                if(target->isPlayer())
                    *target << ColorOn << "^R" << setf(CAP) << player << " nearly flattened you into the ground! You're dead!\n" << ColorOff;
                break;
            }
        }

        player->checkImprove("smash", true);

        if(weapon && Random::get(0,1))
            weapon->decShotsCur();
        Creature::simultaneousDeath(player, target, false, false);

        // Stun chance here
        if(!wasKilled && !meKilled && (result == ATTACK_HIT || result == ATTACK_CRITICAL || result == ATTACK_GLANCING)) {
            int stunChance = (result == ATTACK_GLANCING?200:400); // base 20% glancing, 40% otherwise
            
            if(sizeDiff > 0)
                stunChance += (sizeDiff * 100);

            if(result == ATTACK_CRITICAL)
                stunChance += 500;

            if(target->isEffected("berserk"))
                stunChance /= 3;

            if(player->isCt())
                stunChance = 1001;

            int roll = Random::get(1,1000);
            if(isPtester(player) || player->flagIsSet(P_PTEST_SMASH))
                *player << ColorOn << "^DstunChance (1-1000): " << stunChance << "\nRoll: " << roll << "\n" << ColorOff;

            if(roll <= stunChance) {
                target->stun((result==ATTACK_GLANCING?Random::get(3,4):Random::get(6,8)));
                *player << ColorOn << "^Y" << setf(CAP) << target << " is knocked senseless!\n" << ColorOff;
                *target << ColorOn << "^YYou've been knocked senseless!\n" << ColorOff;
                broadcast(player->getSock(), player->getParent(), "^Y%M is knocked senseless!^x", target.get());
            }

        }

        // Only monsters flee, and not when their attacker was killed
        if(!wasKilled && !meKilled && target->getAsMonster()) {
            if(target->flee() == 2)
                return(0);
        }

    } else if(result == ATTACK_MISS) {
        player->statistics.miss();
        if(target->isPlayer())
            target->getAsPlayer()->statistics.wasMissed();
        *player << ColorOn << "^yYour SMASH missed!\n" << ColorOff;
        player->checkImprove("smash", false);
        broadcast(player->getSock(), player->getParent(), "^y%M tried to SMASH %M!^x", player.get(), target.get());
        player->setAttackDelay(Random::get(3,7));
    
    } else if(result == ATTACK_DODGE) {
        *player << ColorOn << "^yYour SMASH missed!\n" << ColorOff;
        broadcast(player->getSock(), player->getParent(), "^y%M tried to SMASH %M!^x", player.get(), target.get());
        target->dodge(player);
    } else if(result == ATTACK_FUMBLE) {
        player->statistics.fumble();
        *player << ColorOn << "^gYou FUMBLED " << ((weapon && !weapon->flagIsSet(O_NO_PREFIX))?"your ":" ") << (weapon?weapon->getName():"your attack") << ".\n" << ColorOff;
        broadcast(player->getSock(), player->getParent(), "^g%M fumbled %s weapon.", player.get(), player->hisHer());

        //While envenom usually requires piercing or slashing weapons, and smash is only for crushing or specifically
        //flagged smashing weapons, the below check needs to be here for completeness in case envenom is ever changed,
        //or if certain exotic or slashing/piercing weapons are flagged as able to use smash with.
        if(weapon->flagIsSet(O_ENVENOMED)) {
            if(!player->immuneToPoison() &&
                !player->chkSave(POI, player, -5) && !induel(player, target->getAsPlayer())
            ) {
                *player << ColorOn << "^G^#You poisoned yourself!!\n" << ColorOff;
                broadcast(player->getSock(), player->getParent(), "%M poisoned %sself!!",
                    player.get(), player->himHer());

                if(weapon->getEffectStrength()) {
                    dmg = (Random::get(1,3) + (weapon->getEffectStrength()/10));
                    player->hp.decrease(dmg);
                    *player << ColorOn << "^gYou take ^G" << dmg << "^g damage as the poison takes effect!\n" << ColorOff;
                }

                weapon->clearFlag(O_ENVENOMED);

                if(player->hp.getCur() < 1) {
                    n = 0;
                    player->addObj(weapon);
                    weapon = nullptr;
                    player->computeAttackPower();
                    player->die(POISON_PLAYER);
                    return(0);
                }

                dur = standardPoisonDuration(weapon->getEffectDuration(), player->constitution.getCur());
                player->poison(player, weapon->getEffectStrength(), dur);

            } else {
                *player << "You almost poisoned yourself!\n";
                broadcast(player->getSock(), player->getParent(), "%M almost poisoned %sself!", player.get(), player->himHer());
            }
        }

        n = 0;
        player->addObj(weapon);
        player->ready[WIELD-1] = nullptr;
        weapon = nullptr;
        player->computeAttackPower();
    } else {
        *player << ColorOn << "^GError: Unhandled attack result! Result: " << result << "\n" << ColorOff;
    }

    // On miss, dodge, or fumble (if weapon was used) reset timer to 9s instead of the normal smashInterval based on skill
    // Makes smash able to be used more often to help level the skill up
    if(result == ATTACK_MISS || result == ATTACK_DODGE || result == ATTACK_FUMBLE) {
        player->lasttime[LT_SMASH].ltime = t;
        player->lasttime[LT_SMASH].interval = 9L;
    }

    return(0);
}

//*********************************************************************
//                      cmdKick
//*********************************************************************
// This function allows fighters to kick their oopponents every 12
// seconds, in order to do a little extra damage. -- TC

int cmdKick(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature;
    long    i=0,t=0;
    int     chance;


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
        player->pleaseWait(i - t);
        return(0);
    }

    // Kick
    player->lasttime[LT_KICK].ltime = t;
    if(player->getClass() == CreatureClass::FIGHTER || player->getClass() == CreatureClass::MONK)
        player->lasttime[LT_KICK].interval = 9L;
    else
        player->lasttime[LT_KICK].interval = 12L;


    if(player->getClass() !=  CreatureClass::MONK) {
        player->lasttime[LT_DISARM].ltime = t;
        player->lasttime[LT_DISARM].interval = 6;
    }

    i = LT(player, LT_SPELL);
    if (t >= i) 
        player->lasttime[LT_SPELL].interval = 3L;

    if(player->knowsSkill("slam")) {
        i = LT(player, LT_SLAM);
        if (t >= i) 
            player->lasttime[LT_SLAM].interval = player->getPrimaryDelay()/10;
    }

    if(player->knowsSkill("gore")) {
        i = LT(player, LT_GORE);
        if (t >= i) 
            player->lasttime[LT_GORE].interval = player->getPrimaryDelay()/10;
    }

    if(player->knowsSkill("smash")) {
        i = LT(player, LT_SMASH);
        if (t >= i) 
            player->lasttime[LT_SMASH].interval = player->getPrimaryDelay()/10;
    }

    if(player->knowsSkill("bash")) {
        i = LT(player, LT_BASH);
        if (t >= i) 
            player->lasttime[LT_BASH].interval = player->getPrimaryDelay()/10;
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


    chance = std::max(0, std::min(95, chance));

    if(player->isBlind())
        chance = std::min(10, chance);

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
        creature->print("%M tried to kick you.\n", player.get());
        broadcast(player->getSock(),  creature->getSock(), creature->getRoomParent(), "%M tried to kick %N.", player.get(), creature.get());
    }

    return(0);
}


//*********************************************************************
//                      scentTrack
//*********************************************************************
// determines if this player finds tracks by smell or by sight

bool Player::scentTrack() {
    return(getClass() == CreatureClass::WEREWOLF || getRace() == MINOTAUR);
}

//*********************************************************************
//                      canTrack
//*********************************************************************

bool Player::canTrack() {
    if(!ableToDoCommand())
        return(false);

    if(isBlind() && !scentTrack() && !isStaff()) {
        printColor("^CYou can't see any tracks! You're blind!\n");
        return(false);
    }

    if(!knowsSkill("track")) {
        print("You lack the proper training to find tracks.\n");
        return(false);
    }

    if(checkHeavyRestrict("find tracks"))
        return(false);

    if(flagIsSet(P_SITTING))
        stand();

    return(true);
}

//*********************************************************************
//                      doTrack
//*********************************************************************

void Player::doTrack() {
    Track*  track=nullptr;
    int     chance;
    int     skLevel = (int)getSkillLevel("track");

    clearFlag(P_AFK);
    unhide();

    if(!canTrack())
        return;

    if(inAreaRoom()) {
        if(auto area = getAreaRoomParent()->area.lock()) {
            track = area->getTrack(getAreaRoomParent()->mapmarker);
        }
    }
    else if(inUniqueRoom())
        track = &getUniqueRoomParent()->track;

    chance = 25 + (bonus(dexterity.getCur()) + skLevel) * 5;

    if(!track || Random::get(1,100) > chance) {
        *this << "You fail to find any tracks.\n";
        checkImprove("track", false);
        return;
    }

    if(track->getDirection().empty()) {
        print("There are no tracks in this room.\n");
        return;
    }

    *this << ColorOn << "You " << (scentTrack() ? "smell" : "find") << " tracks leading to the " << track->getDirection() << "^x.\n" << ColorOff;

    if(skLevel >= 5 && track->getSize() != NO_SIZE)
        *this << "The tracks are " << getSizeName(track->getSize()) << ".\n";
    if(skLevel >= 10 && track->getNum())
        *this << "You find " << track->getNum() << " unique sets of tracks.\n";

    checkImprove("track", true);
}

void doTrack(const DelayedAction* action) {
    if(auto t = action->target.lock()) {
        t->getAsPlayer()->doTrack();
    }
}

//*********************************************************************
//                      cmdTrack
//*********************************************************************
// This function is the routine that allows players to search
// for tracks in a room. If successful, they will be told what
// direction the last person who was in the room went.

int cmdTrack(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    i, t;

    player->clearFlag(P_AFK);

    if(player->isMagicallyHeld(true))
        return(0);
    
    if(!player->canTrack())
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

        player->print("You begin %s for tracks.\n", (player->scentTrack() ? "sniffing" : "searching"));
    } else {
        broadcast(player->getSock(), player->getParent(), "%M %s for tracks.", player.get(), (player->scentTrack() ? "sniffs" : "searches"));

        player->doTrack();
    }

    return(0);
}

//*********************************************************************
//                      cmdHarmTouch
//*********************************************************************
// This will allow death knights to harm touch once reaching 16th level.

int cmdHarmTouch(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature;
    int     num, chance, modifier=0;
    long    t=time(nullptr), i;


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
    broadcast(player->getSock(),  creature->getSock(), creature->getRoomParent(), "%M touches %N with %s malevolent hand!", player.get(), creature.get(), player->hisHer());


    chance = 6500;

    modifier += ((player->getLevel() - creature->getLevel())*100);          //Level difference
    modifier += 20*(player->piety.getCur() - creature->piety.getCur());     //Piety difference

    chance += modifier;

    chance = std::min(9500,chance);

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
                player->doHeal(creature, std::min<int>(num,(creature->hp.getMax() - creature->hp.getCur())));
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
        broadcast(player->getSock(),  creature->getSock(), creature->getRoomParent(), "%M's harmful touch had no effect.", player.get());
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

int cmdBloodsacrifice(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    i, t;
    int     chance;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
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
    chance = std::min(85, level * 20 + bonus(player->piety.getCur()));
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

    broadcast(player->getSock(), player->getParent(), "%M sacrifices %s blood.", player.get(), player->hisHer());
    return(0);
}
