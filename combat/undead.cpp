/*
 * undead.cpp
 *   Functions for undead players.
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

#include <ctime>           // for time

#include "creatures.hpp"   // for Player, Creature, Monster, CHECK_DIE
#include "damage.hpp"      // for Damage
#include "flags.hpp"       // for P_OUTLAW, P_LAG_PROTECTION_ACTIVE, P_LAG_P...
#include "global.hpp"      // for CreatureClass, CreatureClass::CARETAKER
#include "mud.hpp"         // for LT, LT_REGENERATE, LT_MIST, LT_DRAIN_LIFE
#include "proto.hpp"       // for broadcast, bonus, log_immort, isDay, broad...
#include "random.hpp"      // for Random
#include "rooms.hpp"       // for BaseRoom
#include "utils.hpp"       // for MIN

class cmd;

//*********************************************************************
//                      cmdBite
//*********************************************************************

int cmdBite(Player* player, cmd* cmnd) {
    Creature* target=nullptr;
    Player  *pTarget=nullptr;
    long    i=0, t=0;
    int     chance=0, dmgnum=0;
    Damage damage;


    if(!player->ableToDoCommand() || !player->checkAttackTimer())
        return(0);

    if((!player->knowsSkill("bite") || !player->isEffected("vampirism")) && !player->isStaff()) {
        player->print("You lack the fangs!\n");
        return(0);
    }

    if(isDay() && !player->isCt())  {
        player->print("You may only bite people during the night.\n");
        return(0);
    }

    if(player->inCombat() && !player->checkStaff("Not while you're in combat.\n"))
        return(0);

    if(!(target = player->findVictim(cmnd, 1, true, false, "Bite what?\n", "You don't see that here.\n")))
        return(0);

    pTarget = target->getAsPlayer();

    if(!player->canAttack(target))
        return(0);

    if(player->isBlind()) {
        player->printColor("^CYou're blind!\n");
        return(0);
    }

    if(target->isEffected("drain-shield") && target->isMonster()) {
        player->print("Your bite would be ineffective.\n");
        return(0);
    }

    if( target->isUndead() &&
        !player->checkStaff("That won't work on the undead.\n")
    ) {
        if(target->isMonster())
            target->getAsMonster()->addEnemy(player);
        return(0);
    }

    i = player->lasttime[LT_PLAYER_BITE].ltime;
    t = time(nullptr);

    if((t - i < 30L) && !player->isDm()) {
        player->pleaseWait(30L-t+i);
        return(0);
    }

    player->smashInvis();
    player->interruptDelayedActions();

    if(pTarget) {

    } else {
        target->getAsMonster()->addEnemy(player);
        // Activates lag protection.
        if(player->flagIsSet(P_LAG_PROTECTION_SET))
            player->setFlag(P_LAG_PROTECTION_ACTIVE);
    }

    player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
    player->lasttime[LT_PLAYER_BITE].ltime = t;
    player->lasttime[LT_READ_SCROLL].interval = 3L;
    player->lasttime[LT_READ_SCROLL].ltime = t;
    player->lasttime[LT_PLAYER_BITE].interval = 30L;


    chance = 35 + ((int)((player->getSkillLevel("bite") - target->getLevel()) * 20) + bonus(player->strength.getCur()) * 5);
    chance = MIN(chance, 85);

    if(target->isEffected("drain-shield"))
        chance /= 2;

    if(player->isDm())
        chance = 100;

    if(Random::get(1, 100) > chance) {
        player->print("%s eludes your bite.\n", target->upHeShe());
        player->checkImprove("bite", false);
        target->print("%M tried to bite you!\n",player);
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to bite %N.", player, target);
        return(0);
    }


    damage.set(Random::get((int)(player->getSkillLevel("bite")*3), (int)(player->getSkillLevel("bite")*4)));

    target->modifyDamage(player, NEGATIVE_ENERGY, damage);

    dmgnum = damage.get();

    damage.set(MIN<int>(damage.get(), target->hp.getCur() + 1));
    if(damage.get() < 1)
        damage.set(1);

    player->printColor("You bite %N for %s%d^x damage.\n", target, player->customColorize("*CC:DAMAGE*").c_str(), dmgnum);
    player->checkImprove("bite", true);

    if(player->hp.getCur() < player->hp.getMax() && damage.getDrain()) {
        player->print("The blood of %N revitalizes your strength.\n", target);
        player->hp.increase(damage.getDrain());
    }

    if(player->getClass() == CreatureClass::CARETAKER)
        log_immort(false,player, "%s bites %s.\n", player->getCName(), target->getCName());

    target->printColor("%M bites you for %s%d^x damage.\n", player, target->customColorize("*CC:DAMAGE*").c_str(), dmgnum);
    target->stun((Random::get(5, 8) + bonus(player->strength.getCur())));
    broadcast(player->getSock(), target->getSock(), player->getParent(), "%M bites %N!", player, target);
    broadcastGroup(false, target, "^M%M^x bites ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n",
        player, target, dmgnum, target->heShe(), target->getStatusStr(dmgnum));

    player->statistics.attackDamage(dmgnum, "bite");

    if(player->doDamage(target, damage.get(), CHECK_DIE)) {
        if(player->getClass() == CreatureClass::CARETAKER)
            log_immort(true, player, "%s killed %s with a bite.\n", player->getCName(), target->getCName());
    } else {
        if(!induel(player, pTarget)) {
            // 5% chance to get porphyria when bitten by a vampire
            target->addPorphyria(player, 5);
        }
    }

    return(0);
}

//*********************************************************************
//                      `Mist
//*********************************************************************

int cmdMist(Player* player, cmd* cmnd) {
    long    i=0, t=0;
    t = time(nullptr);
    i = player->getLTAttack() > LT(player, LT_SPELL) ? player->getLTAttack() : LT(player, LT_SPELL);

    if(LT(player, LT_MIST) > i)
        i = LT(player, LT_MIST);



    if(!player->ableToDoCommand())
        return(0);

    if((!player->knowsSkill("mist") || !player->isEffected("vampirism")) && !player->isStaff()) {
        player->print("You are unable to turn to mist.\n");
        return(0);
    }
    if(player->getRoomParent()->flagIsSet(R_ETHEREAL_PLANE)) {
        player->print("That is not possible here.\n");
        return(0);
    }

    if(t < i) {
        player->pleaseWait(i-t);
        return(0);
    }

    if(!player->canMistNow()) {
        player->print("You may only turn to mist during the night.\n");
        return(0);
    }

    if(player->flagIsSet(P_OUTLAW)) {
        player->print("You are unable to mist right now.\n");
        return(0);
    }

    if(player->getRoomParent()->flagIsSet(R_DISPERSE_MIST)) {
        player->print("Swirling vapors prevent you from entering mist form.\n");
        return(0);
    }

    if(player->getGroup()) {
        player->print("You can't do that while in a group.\n");
        return(0);
    }
    if(player->isEffected("mist")) {
        player->print("You are already a mist.\n");
        return(0);
    }

    if(player->inCombat()) {
        player->print("Not in the middle of combat.\n");
        return(0);
    }

    broadcast(player->getSock(), player->getParent(), "%M turns to mist.", player);
    player->addEffect("mist", -1);
    player->print("You turn to mist.\n");
    //player->checkImprove("mist", true);
    player->clearFlag(P_SITTING);
    player->interruptDelayedActions();

    player->lasttime[LT_MIST].ltime = time(nullptr);
    player->lasttime[LT_MIST].interval = 3L;

    return(0);
}

//*********************************************************************
//                      canMistNow
//*********************************************************************
// whether or not the player can currently turn to mist

bool Player::canMistNow() const {
    if(isStaff())
        return(true);
    // only people who know how to do it can mist
    if((!knowsSkill("mist") || !isEffected("vampirism")))
        return(false);
    // does the room let them mist anyway?
    // BUG: callin flagIsSet() with possibly invalid getRoom()
    if(getConstRoomParent() && (getConstRoomParent()->flagIsSet(R_VAMPIRE_COVEN) || getConstRoomParent()->flagIsSet(R_CAN_ALWAYS_MIST)))
        return(true);
    // otherwise, not during the day
    if(isDay())
        return(false);
    return(true);
}

//*********************************************************************
//                      cmdUnmist
//*********************************************************************

int cmdUnmist(Player* player, cmd* cmnd) {
    if(player->isEffected("mist")) {
        player->unmist();
    } else {
        if(!player->isEffected("vampirism") && !player->isStaff())
            return(0);

        player->print("You are not in mist form.\n");
    }
    return(0);
}

//*********************************************************************
//                      cmdHypnotize
//*********************************************************************

int cmdHypnotize(Player* player, cmd* cmnd) {
    Creature* target=nullptr;
    int     dur=0, chance=0;
    long    i=0, t=0;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("hypnotize")) {
        player->print("You lack the training to hypnotize anyone!\n");
        return(0);
    }
    if(!player->isEffected("vampirism") && !player->isStaff()) {
        player->print("Only vampires may hypnotize people.\n");
        return(0);
    }

    i = LT(player, LT_HYPNOTIZE);
    t = time(nullptr);
    if(i > t && !player->isDm()) {
        player->pleaseWait(i-t);
        return(0);
    }

    dur = 300 + Random::get(1, 30) * 10 + bonus(player->constitution.getCur()) * 30 +
          (int)(player->getSkillLevel("hypnotize") * 5);

    if(!(target = player->findVictim(cmnd, 1, true, false, "Hypnotize whom?\n",
            "You don't see that here.\n")))
        return(0);

    if(!target->canSee(player) && !player->checkStaff("Your victim must see you to be hypnotized.\n"))
        return(0);

    if(!player->canAttack(target))
        return(0);

    if(target->isMonster()) {
        if(target->getAsMonster()->isEnemy(player)) {
            player->print("Not while you are already fighting %s.\n", target->himHer());
            return(0);
        }
    }

    if( target->isPlayer() &&
        (   player->vampireCharmed(target->getAsPlayer()) ||
            (target->hasCharm(player->getName()) && player->flagIsSet(P_CHARMED))
        ))
    {
        player->print("But they are already your good friend!\n");
        return(0);
    }

    if(target->isPlayer() && target->getClass() == CreatureClass::RANGER && target->getLevel() > player->getLevel()) {
        player->print("%M is immune to your hypnotizing gaze.\n", target);
        return(0);
    }

    if(target->isMonster() && (target->flagIsSet(M_NO_CHARM) || (target->intelligence.getCur() < 3)) ) {
        player->print("Your hypnotism fails.\n");
        return(0);
    }

    if( target->isUndead() &&
        !player->checkStaff("You cannot hypnotize undead beings.\n") )
        return(0);

    player->smashInvis();


    player->lasttime[LT_HYPNOTIZE].ltime = t;
    player->lasttime[LT_HYPNOTIZE].interval = 600L;
    chance = MIN(90, 40 + (int)(player->getSkillLevel("hypnotize") - target->getLevel()) * 20 + 4 *
            bonus(player->intelligence.getCur()));
    if(target->flagIsSet(M_PERMENANT_MONSTER))
        chance-=25;
    if(player->isDm())
        chance = 101;
    if(Random::get(1, 100) > chance && !player->isCt()) {
        player->print("You fail to hypnotize %N.\n", target);
        player->checkImprove("hypnotize", false);
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M attempts to hypnotize %N.",player, target);
        if(target->isMonster()) {
            target->getAsMonster()->addEnemy(player);
            if(player->flagIsSet(P_LAG_PROTECTION_SET)) {    // Activates lag protection.
                player->setFlag(P_LAG_PROTECTION_ACTIVE);
            }
            return(0);
        }
        target->printColor("^m%M tried to hypnotize you.\n", player);
        return(0);
    }

    if( (target->isPlayer() && target->isEffected("resist-magic")) ||
        (target->isMonster() && target->isEffected("resist-magic"))
    )
        dur /= 2;


    if(!player->isCt()) {
        if(target->chkSave(MEN, player,0)) {
            player->print("%M avoided your hypnotizing gaze.\n", target);
            player->checkImprove("hypnotize", false);
            target->print("You avoided %s's hypnotizing gaze.\n", player->getCName());
            return(0);
        }
    }

    log_immort(false,player, "%s hypnotizes %s.\n", player->getCName(), target->getCName());

    player->print("You hypnotize %N.\n", target);
    player->checkImprove("hypnotize", true);
    broadcast(player->getSock(), target->getSock(), player->getParent(), "%M hypnotizes %N!", player, target);
    target->print("%M hypnotizes you.\n", player);


    player->addCharm(target);

    target->lasttime[LT_CHARMED].ltime = time(nullptr);
    target->lasttime[LT_CHARMED].interval = dur;

    if(target->isPlayer())
        target->setFlag(P_CHARMED);
    else
        target->setFlag(M_CHARMED);


    return(0);
}


//*********************************************************************
//                      cmdRegenerate
//*********************************************************************
// This command is for liches.

int cmdRegenerate(Player* player, cmd* cmnd) {
    int     chance=0, xtra=0, pasthalf=0,inCombat=0;
    long    i=0, t=0;

    if(!player->ableToDoCommand())
        return(0);


    player->clearFlag(P_AFK);
    if(!player->knowsSkill("regenerate")) {
        player->print("You lack the training to regenerate.\n");
        return(0);
    }


    inCombat = player->inCombat();

    if(!player->isCt()) {
        if(player->hp.getCur() > player->hp.getMax() * 3 / 4) {
            player->printColor("^cYou cannot regenerate when past 3/4 of your life force.\n");
            return(0);
        }
        i = player->lasttime[LT_REGENERATE].ltime + player->lasttime[LT_REGENERATE].interval;
        t = time(nullptr);
        if(i > t) {
            player->pleaseWait(i-t);
            return(0);
        }
    }

    if( player->getAlignment() > 100 &&
        !player->checkStaff("Your soul is not corrupt enough to draw energy.\n")
    )
        return(0);

    int level = (int)player->getSkillLevel("regenerate");
    // get a base number:
    //   10 = -36.66
    //  120 = 0
    //  400 = 93.3
    chance = (player->constitution.getCur() - 120) / 3;
    chance = MIN(85, level * 4 + chance);
    if(player->isCt())
        chance = 101;

    if(Random::get(1, 100) <= chance) {
        player->print("Your dark essence draws from the positive energy around you.\n");
        player->checkImprove("regenerate", true);
        broadcast(player->getSock(), player->getParent(), "%M regenerates.", player);

        for(Player* ply : player->getRoomParent()->players) {
            if(!ply->isUndead())
                ply->print("You shiver from a sudden deathly coldness.\n");
        }

        if(inCombat)
            player->hp.increase(Random::get(level+6,level*(3)+7));
        else
            player->hp.increase(Random::get(level+8,level*(4)+9));

        // Prevents players from avoiding the 75% hp limit to allow regenerate
        // by casting on themselves to get below half. A lich may never regenerate
        // past 75% their max HP. This potentially evens out their ticktime with mages
        if(player->hp.getCur() > player->hp.getMax() * 3 / 4) {
            player->hp.setCur((player->hp.getMax() * 3 / 4) + 1);
            pasthalf = 1;
        }

        // Extra regeneration from being extremely evil.
        if(player->getAlignment() <= -250 && !pasthalf) {
            if(player->getAlignment() <= -400)
                xtra = Random::get(6,9);
            else
                xtra = Random::get(2,5);
            player->print("Your dark essence regenerated %d more due to your soul's corruption.\n",xtra);
            player->hp.increase(xtra);
        }


        player->lasttime[LT_REGENERATE].ltime = t;
        player->lasttime[LT_REGENERATE].interval = inCombat ? 45L : 75L;


    } else {
        player->print("You failed to regenerate.\n");
        player->checkImprove("regenerate", false);
        broadcast(player->getSock(), player->getParent(), "%M tried to regenerate.", player);
        player->lasttime[LT_REGENERATE].ltime = t;
        player->lasttime[LT_REGENERATE].interval = 6L;
    }

    return(0);
}

//*********************************************************************
//                      cmdDrainLife
//*********************************************************************
// This allows a lich to drain hp from someone else and add half the
// damage to their own

int cmdDrainLife(Player* player, cmd* cmnd) {
    Creature* target=nullptr;
    Player  *pTarget=nullptr;
    long    i=0, t=0;
    int     chance=0;
    Damage damage;

    if(!player->ableToDoCommand())
        return(0);


    if(!player->knowsSkill("drain")) {
        player->print("You haven't learned how to drain someone's life.\n");
        return(0);
    }

    if(!(target = player->findVictim(cmnd, 1, true, false, "Drain whom?\n", "You don't see that here.\n")))
        return(0);

    pTarget = target->getAsPlayer();

    player->smashInvis();
    player->interruptDelayedActions();

    if(!player->canAttack(target))
        return(0);

    if( target->isUndead() &&
        !player->checkStaff("You may only drain the life of the living.\n") )
        return(0);

    if(!pTarget) {
        // Activates lag protection.
        if(player->flagIsSet(P_LAG_PROTECTION_SET))
            player->setFlag(P_LAG_PROTECTION_ACTIVE);

        if(target->isPet()) {
            if(!player->flagIsSet(P_CHAOTIC) && !player->isCt() && !target->getPlayerMaster()->flagIsSet(P_OUTLAW)) {
                player->print("Sorry, you're lawful.\n");
                return(0);
            }
            if(!target->getPlayerMaster()->flagIsSet(P_CHAOTIC) && !player->isCt() && !target->getPlayerMaster()->flagIsSet(P_OUTLAW)) {
                player->print("Sorry, that creature is lawful.\n");
                return(0);
            }
            if(player->getRoomParent()->isPkSafe() && !target->getPlayerMaster()->flagIsSet(P_OUTLAW)) {
                player->print("No killing allowed in this room.\n");
                return(0);
            }
        }
    }

    i = LT(player, LT_DRAIN_LIFE);
    t = time(nullptr);
    if(i > t && !player->isDm()) {
        player->pleaseWait(i-t);
        return(0);
    }

    if(target->isMonster())
        target->getAsMonster()->addEnemy(player);

    player->lasttime[LT_DRAIN_LIFE].ltime = t;
    player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
    player->lasttime[LT_DRAIN_LIFE].interval = 45L;

    int level = (int)player->getSkillLevel("drain");

    chance = (level - target->getLevel()) * 20 + bonus(player->constitution.getCur()) * 5 + 25;
    chance = MIN(chance, 80);

    if(target->isEffected("drain-shield"))
        chance /= 2;

    if(Random::get(1, 100) > chance) {
        player->print("You failed to drain %N's life.\n", target);
        player->checkImprove("drain", false);

        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to drain %N's life.", player, target);
        target->print("%M tried drain your life!\n", player);
        return(0);
    }


    damage.set(Random::get(level * 3, level * 5));
    if(target->isEffected("drain-shield"))
        damage.set(Random::get(1, level));

    // Berserked barbarians have more energy to drain
    if(pTarget && pTarget->isEffected("berserk"))
        damage.set(damage.get() + (damage.get() / 5));

    damage.set(MIN<int>(damage.get(), target->hp.getCur() + 1));
    if(damage.get() < 1)
        damage.set(1);

    if(!target->isCt()) {
        if(target->chkSave(DEA, player, 0)) {
            player->printColor("^yYour life-drain failed!\n");
            target->print("%M failed to drain your life.\n", player);
            return(0);
        }
    }

    target->modifyDamage(player, NEGATIVE_ENERGY, damage);

    // drain heals us
    if(!target->isEffected("drain-shield"))
        player->hp.increase(damage.get() / 3);

    player->printColor("You drained %N for %s%d^x damage.\n", target, player->customColorize("*CC:DAMAGE*").c_str(), damage.get());
    player->checkImprove("drain", true);

    player->statistics.attackDamage(damage.get(), "drain-life");

    if(target->isEffected("drain-shield"))
        player->print("%M resisted your drain!\n", target);

    target->printColor("%M drained you for %s%d^x damage.\n", player, target->customColorize("*CC:DAMAGE*").c_str(), damage.get());
    broadcast(player->getSock(), target->getSock(), player->getParent(), "%M drained %N's life.", player, target);

    if(player->doDamage(target, damage.get(), CHECK_DIE)) {
        if(player->getClass() == CreatureClass::CARETAKER && pTarget)
            log_immort(true, player, "%s killed %s with the drain.\n", player->getCName(), target->getCName());
    }

    return(0);
}
