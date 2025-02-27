/*
 * weaponless.cpp
 *   Functions that deal with weaponless abilities
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

#include <cstdlib>                   // for abs
#include <ctime>                     // for time
#include <list>                      // for operator==, _List_iterator, list
#include <map>                       // for operator==, _Rb_tree_const_iterator
#include <string>                    // for allocator, string
#include <type_traits>               // for enable_if<>::type

#include "commands.hpp"              // for cmdFocus, cmdFrenzy, cmdHowl
#include "damage.hpp"                // for Damage
#include "flags.hpp"                 // for M_PERMANENT_MONSTER, P_AFK, P_FO...
#include "global.hpp"                // for CreatureClass, CreatureClass::CA...
#include "group.hpp"                 // for CreatureList, Group
#include "lasttime.hpp"              // for lasttime
#include "mud.hpp"                   // for LT_TOUCH_OF_DEATH, LT_MEDITATE
#include "mudObjects/container.hpp"  // for MonsterSet
#include "mudObjects/creatures.hpp"  // for Creature, ATTACK_MAUL, CHECK_DIE
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/objects.hpp"    // for Object
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, bonus, log_immort
#include "random.hpp"                // for Random
#include "stats.hpp"                 // for Stat
#include "structs.hpp"               // for saves

//*********************************************************************
//                      cmdMeditate
//*********************************************************************

int cmdMeditate(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int     chance=0,inCombatModifier=0;
    long    i=0, t = time(nullptr);

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("meditate") || player->isUndead()) {
        player->print("You meditate.\n");
        broadcast(player->getSock(), player->getParent(), "%M meditates.", player.get());
        return(0);
    }

    i = player->lasttime[LT_MEDITATE].ltime + player->lasttime[LT_MEDITATE].interval;
    if(i>t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    chance = std::min(85, (int)(player->getSkillLevel("meditate")*10)+bonus(player->piety.getCur()));

    if(Random::get(1, 100) > player->getLuck() + (int)(player->getSkillLevel("meditate") * 2))
        chance = 10;

    int level = std::max(1,(int)player->getSkillLevel("meditate"));

    broadcast(player->getSock(), player->getParent(), "%M meditates.", player.get());

    if(Random::get(1,100) <= chance) {

        if(player->inCombat()) {
            player->print("Your stillness of mind adds to your endurance.\n");
            inCombatModifier=3;
        }
        else {
            player->print("You feel rejuvenated and at one with the universe.\n");
            inCombatModifier=2;
        }

        player->checkImprove("meditate", true);

        int heal = (std::max(0,(level / inCombatModifier // heals for a little less when in combat with mobs
            + Random::get(1, level)
                           + bonus(player->constitution.getCur()) * Random::get(1, 4))));

        player->doHeal(player, heal,1.0);
        
        player->lasttime[LT_MEDITATE].ltime = t;
        if(inCombatModifier < 3) {
            player->lasttime[LT_MEDITATE].interval = 90L;
            player->mp.increase(heal / 2); // MP increases also when not in combat with mobs.
        }
        else
            player->lasttime[LT_MEDITATE].interval = 40L; // timer faster when done during combat with mobs
        
    } else {
        player->print("You were unable to find inner stillness.\n");
        player->checkImprove("meditate", false);
        player->lasttime[LT_MEDITATE].ltime = t;
        player->lasttime[LT_MEDITATE].interval = 5L;
    }

    return(0);

}


//*********************************************************************
//                      cmdTouchOfDeath
//*********************************************************************
// This function allows monks to kill nonundead creatures.
// If they succeed then the creature is either killed or harmed
// for approximately half of its hit points.

int cmdTouchOfDeath(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    std::shared_ptr<Player> pCreature=nullptr;
    long    i=0, t=time(nullptr);
    int     chance=0;
    Damage damage;

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("touch")) {
        player->print("You touch yourself.\n");
        broadcast(player->getSock(), player->getParent(), "%M touches %sself", player.get(), player->himHer());
        return(0);
    }

    if(player->ready[WIELD-1] && !player->isDm() && !player->ready[WIELD-1]->flagIsSet(O_ALLOW_TOUCH_OF_DEATH)) {
        player->print("How can you do that with your hands full?\n");
        return(0);
    }


    if(!(creature = player->findVictim(cmnd, 1, true, false, "Use the touch of death on whom?\n", "You don't see that here.\n")))
        return(0);

    if(creature)
        pCreature = creature->getAsPlayer();

    if(!player->canAttack(creature))
        return(0);

    if(player->isBlind()) {
        player->printColor("^CYou're blind!\n");
        return(0);
    }


    if( creature->isUndead() &&
        !player->checkStaff("That won't work on the undead.\n") )
    {
        if(!pCreature)
            creature->getAsMonster()->addEnemy(player);
        return(0);
    }



    i = player->lasttime[LT_TOUCH_OF_DEATH].ltime + player->lasttime[LT_TOUCH_OF_DEATH].interval;
    if(i>t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    player->smashInvis();
    player->unhide();

    if(pCreature) {

    } else {
        creature->getAsMonster()->addEnemy(player);
        if(player->flagIsSet(P_LAG_PROTECTION_SET)) {    // Activates lag protection.
            player->setFlag(P_LAG_PROTECTION_ACTIVE);
        }
    }

    player->updateAttackTimer(false);
    player->lasttime[LT_TOUCH_OF_DEATH].ltime = t;
  

    if(player->isDm())
        player->lasttime[LT_TOUCH_OF_DEATH].interval = 0;


    chance = (int)((player->getSkillLevel("touch") - creature->getLevel())*20)+ bonus(player->constitution.getCur()) * 10;
    chance = std::min(chance, 85);

    if(player->isCt())
        chance = 101;

    if(creature->mFlagIsSet(M_RESIST_TOUCH) && !player->isCt())
        chance = 0;

    if(creature->isPlayer() && creature->isEffected("berserk"))
        chance /= 2;

    if(Random::get(1,100) > chance) {
        player->print("You failed to harm %N.\n", creature.get());
        player->checkImprove("touch", false);
        broadcast(player->getSock(), player->getParent(), "%M failed the touch of death on %N.\n",
            player.get(), creature.get());
        player->lasttime[LT_TOUCH_OF_DEATH].interval = 25L;
        return(0);
    }


    if(!player->isCt()) {
        if(creature->chkSave(DEA, player, 0)) {
            player->printColor("^y%M avoided your touch of death!\n", creature.get());
            player->checkImprove("touch", false);
            creature->print("You avoided %N's touch of death.\n", player.get());
            player->lasttime[LT_TOUCH_OF_DEATH].interval = 25L;
            return(0);
        }
    }

    if( ((Random::get(1,100) > 80 - bonus(player->constitution.getCur())) &&
         (   (creature->isMonster() &&
                !creature->flagIsSet(M_PERMANENT_MONSTER)) ||
                creature->isPlayer() )) ||
        player->isDm())
    {
        player->print("You fatally wound %N.\n", creature.get());
        player->checkImprove("touch", true);
        if(!player->isDm())
            log_immort(false,player, "%s fatally wounds %s.\n", player->getCName(), creature->getCName());

        broadcast(player->getSock(), player->getParent(), "%M fatally wounds %N.", player.get(), creature.get());
        if(creature->isMonster())
            creature->getAsMonster()->adjustThreat(player, creature->hp.getCur());

        player->lasttime[LT_TOUCH_OF_DEATH].interval = 600L;
        
        creature->die(player);

    } else {

        damage.set(std::max<int>(1, creature->hp.getCur() / 2));

        creature->modifyDamage(player, ABILITY, damage);
        //player->statistics.attackDamage(dmg, "touch-of-death");

        player->printColor("You touched %N for %s%d^x damage.\n", creature.get(), player->customColorize("*CC:DAMAGE*").c_str(), damage.get());
        player->checkImprove("touch", true);
        broadcast(player->getSock(), player->getParent(), "%M uses the touch of death on %N.", player.get(), creature.get());

        player->lasttime[LT_TOUCH_OF_DEATH].interval = 600L;

        if(player->getClass() == CreatureClass::CARETAKER)
            log_immort(false,player, "%s uses the touch of death on %s.\n", player->getCName(), creature->getCName());
        if(player->doDamage(creature, damage.get(), CHECK_DIE)) {
            if(player->getClass() == CreatureClass::CARETAKER)
                log_immort(false,player, "%s killed %s with touch of death.\n", player->getCName(), creature->getCName());

        }
    }

    return(0);
}

//*********************************************************************
//                      cmdFocus
//*********************************************************************

int cmdFocus(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    i=0, t=0;
    int     chance=0;

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("focus")) {
        player->print("You try to focus on inner peace but fail.\n");
        return(0);
    }

    if(player->flagIsSet(P_FOCUSED)) {
        player->print("You've already focused your energy.\n");
        return(0);
    }

    i = player->lasttime[LT_FOCUS].ltime;
    t = time(nullptr);

    if(t - i < 600L) {
        player->pleaseWait(600L-t+i);
        return(0);
    }
    chance = std::min(80, (int)(player->getSkillLevel("focus") * 20) + bonus(player->piety.getCur()));

    double level = player->getSkillLevel("focus");
    if(Random::get(1, 100) > player->getLuck() + (int)(level * 2))
        chance = 10;

    if(player->isEffected("pray"))
        chance += 10;

    if(Random::get(1, 100) <= chance) {
        player->print("You begin to focus your energy.\n");
        player->checkImprove("focus", true);
        broadcast(player->getSock(), player->getParent(), "%M focuses %s energy.", player.get(), player->hisHer());
        player->setFlag(P_FOCUSED);
        player->lasttime[LT_FOCUS].ltime = t;
        player->lasttime[LT_FOCUS].interval = 210L;

        player->computeAttackPower();
    } else {
        player->print("You failed to focus your energy.\n");
        player->checkImprove("focus", false);
        broadcast(player->getSock(), player->getParent(), "%M tried to focus %s energy.",
            player.get(), player->hisHer());
        player->lasttime[LT_FOCUS].ltime = t - 590L;
    }

    return(0);

}


//*********************************************************************
//                      cmdFrenzy
//*********************************************************************
// This command allows a werewolf to attack in a frenzy (every 2 seconds
// instead of 3) and get an extra 5 dex.

int cmdFrenzy(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    i=0, t=0;
    int     chance=0;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->knowsSkill("frenzy")) {
        player->print("You try to work yourself up into a frenzy but fail.\n");
        return(0);
    }
    if(!player->isEffected("lycanthropy") && !player->isStaff()) {
        player->print("Only werewolves may go into a frenzy.\n");
        return(0);
    }

    if(player->isEffected("frenzy")) {
        player->print("You're already in a frenzy.\n");
        return(0);
    }
    if(player->flagIsSet(P_SITTING)) 
        player->stand();

    if(player->isEffected("haste")) {
        player->print("Your magically enhanced speed prevents you from going into a frenzy.\n");
        return(0);
    }
    if(player->isEffected("slow")) {
        player->print("Your magically reduced speed prevents you from going into a frenzy.\n");
        return(0);
    }

    i = player->lasttime[LT_FRENZY].ltime;
    t = time(nullptr);

    if(t - i < 600L && !player->isStaff()) {
        player->pleaseWait(600L-t+i);
        return(0);
    }

    chance = std::min(85, (int)(player->getSkillLevel("frenzy") * 20) + bonus(player->dexterity.getCur()));

    if(Random::get(1, 100) > player->getLuck() + (int)(player->getSkillLevel("frenzy") * 2))
        chance = 10;

    if(Random::get(1, 100) <= chance) {
        player->print("You begin to attack in a frenzy.\n");
        player->checkImprove("frenzy", true);
        broadcast(player->getSock(), player->getParent(), "%M attacks in a frenzy.", player.get());
        player->addEffect("frenzy", 210L, 50);
        player->lasttime[LT_FRENZY].ltime = t;
        player->lasttime[LT_FRENZY].interval = 600 + 210L;
    } else {
        player->print("Your attempt to frenzy failed.\n");
        player->checkImprove("frenzy", false);
        broadcast(player->getSock(), player->getParent(), "%M tried to attack in a frenzy.", player.get());
        player->lasttime[LT_FRENZY].ltime = t - 590L;
    }

    return(0);
}

//*********************************************************************
//                      cmdMaul
//*********************************************************************
// This function allows werewolves to "maul" an opponent,
// doing less damage than a normal attack, but knocking the opponent
// over for a few seconds, leaving them unable to attack back.

int cmdMaul(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    std::shared_ptr<Player> pCreature=nullptr;
    long    i=0, t=0;
    int     chance=0, not_initial=0;

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("maul")) {
        player->print("You simply don't know how to maul anyone!\n");
        return(0);
    }
    if(!player->isEffected("lycanthropy") && !player->isStaff()) {
        player->print("Only werewolves may maul people.\n");
        return(0);
    }

    if(!(creature = player->findVictim(cmnd, 1, true, false, "Maul whom?\n", "You don't see that here.\n")))
        return(0);

    if(creature)
        pCreature = creature->getAsPlayer();

    if(!player->canAttack(creature))
        return(0);

    
    if(!player->isCt()) {
       


        i = LT(player, LT_MAUL);
        t = time(nullptr);
        if(i > t) {
            player->pleaseWait(i - t);
            return(0);
        }
    }


    if(!player->isCt() && pCreature) {
        if(pCreature->isEffected("mist")) {
            player->print("You cannot physically hit a misted creature.\n");
            return(0);
        }

        if(player->vampireCharmed(pCreature) || (pCreature->hasCharm(player->getName()) && player->flagIsSet(P_CHARMED))) {
            player->print("You like %N too much to do that.\n", pCreature.get());
            return(0);
        }
    }



    player->unhide();
    player->smashInvis();

    //Wwolves can only wield claw weapons now...ok to maul with them. - TC
/*      if(player->ready[WIELD - 1]) {
        player->print("How can you do that with your claws full?\n");
        return(0);
    }
*/

    player->lasttime[LT_MAUL].ltime = t;
    if(creature->isMonster())
        player->lasttime[LT_MAUL].interval = 15L;
    else
        player->lasttime[LT_MAUL].interval = 3L;

    player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
    player->lasttime[LT_SPELL].ltime = t;
    player->lasttime[LT_SPELL].interval = 3;
    player->lasttime[LT_READ_SCROLL].ltime = t;
    player->lasttime[LT_READ_SCROLL].interval = 3;

    if(!pCreature) {

        creature->getAsMonster()->addEnemy(player);

        // Activates lag protection.
        if(player->flagIsSet(P_LAG_PROTECTION_SET))
            player->setFlag(P_LAG_PROTECTION_ACTIVE);

        if(!player->isCt() && creature->flagIsSet(M_ONLY_HARMED_BY_MAGIC)) {
            player->print("Your maul has no effect on %N.\n", creature.get());
            return(0);
        }

    }
    double level = player->getSkillLevel("maul");
    chance = 45 + (int)((level - creature->getLevel()) * 5) +
             bonus(player->strength.getCur()) * 3 + (bonus(player->dexterity.getCur())
                                                     - bonus(creature->dexterity.getCur())) * 2;

    if(player->isBlind())
        chance = std::min(20, chance);

    if((creature->flagIsSet(M_ENCHANTED_WEAPONS_ONLY) || creature->flagIsSet(M_PLUS_TWO) || creature->flagIsSet(M_PLUS_THREE)) && (not_initial==1)) {
        chance /= 2;
        chance = std::min(chance, 50);
    }

    if(player->isDm())
        chance = 101;

    if(Random::get(1, 100) > player->getLuck() + (int)(level * 2))
        chance = 5;

    if(Random::get(1, 100) <= chance) {
        player->attackCreature(creature, ATTACK_MAUL);
        if(!induel(player, pCreature)) {
            // 5% chance to get lycanthropy when mauled by a werewolf
            creature->addLycanthropy(player, 5);
        }
    } else {
        player->print("You failed to maul %N.\n", creature.get());
        player->checkImprove("maul", false);
        creature->print("%M tried to maul you.\n", player.get());
        broadcast(player->getSock(), creature->getSock(), creature->getRoomParent(),
            "%M tried to maul %N.", player.get(), creature.get());
    }

    return(0);
}


//*********************************************************************
//                      packBonus
//*********************************************************************

int Player::packBonus() {
    int     bns=0;

    Group* group = getGroup();
    if(group) {
        auto it = group->members.begin();
        while(it != group->members.end()) {
            if(auto crt = (*it).lock()) {
                it++;
                // pack isn't pure
                if(!crt->isEffected("lycanthropy") || crt->isMonster())
                    return(0);

                if(!crt->inSameRoom(Containable::downcasted_shared_from_this<Creature>())) continue;

                // bonus for all within 3 levels.
                if(crt->isEffected("lycanthropy") && (abs((int)getLevel() - (int)getLevel()) < 4))
                    bns += Random::get(1,2);
            } else {
                it = group->members.erase(it);
            }
        }
    }

    return(std::min(10, bns));
}


//*********************************************************************
//                      cmdHowl
//*********************************************************************

int cmdHowl(const std::shared_ptr<Creature>& player, cmd* cmnd) {
    std::shared_ptr<BaseRoom> room = player->getRoomParent();
    std::shared_ptr<Monster> monster=nullptr;
    long    i=0, t=0, stunTime=0;
    int     maxEffected=0, numEffected=0, bns=0;

    if(!player->canSpeak()) {
        player->print("You try to howl, but no sound comes forth!\n");
        return(0);
    }

    if((!player->knowsSkill("howl") || !player->isEffected("lycanthropy")) && !player->isStaff()) {
        player->print("You howl at the moon!\n");
        broadcast(player->getSock(), player->getParent(), "%M howls at the moon.", player.get());
        return(0);
    }

    i = LT(player, LT_HOWLS);
    t = time(nullptr);
    if(i > t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    if(player->numEnemyMonInRoom() < 1) {
        player->print("There are no enemies here for your howl to terrify.\n");
        return(0);
    }

    int level = (int)player->getSkillLevel("howl");
    room->wake("You awaken suddenly!", true);
    player->print("You let out a blood-curdling supernatural howl!\n");
    player->checkImprove("howl", true);
    broadcast(player->getSock(), room, "^Y%M lets out a blood-curdling supernatural howl!", player.get());
    maxEffected = level / 2;

    player->lasttime[LT_HOWLS].ltime = t;
    player->lasttime[LT_HOWLS].interval = 240L; // every 4 minutes

    auto mIt = room->monsters.begin();
    while(mIt != room->monsters.end() && numEffected < maxEffected) {
        monster = (*mIt++);

        if(!monster || !monster->isEnemy(player))
            continue;
        if(level - monster->getLevel() < 6)
            continue;
        if(monster->isPet() || monster->getClass() == CreatureClass::PALADIN)
            continue;

        numEffected++;


        bns = -10 * ((level - 6) - monster->getLevel());
        bns += monster->saves[MEN].chance*2;

        if(monster->isEffected("resist-magic"))
            bns += 50;

        // nothing happens if mob saves
        if(monster->chkSave(MEN, player, bns))
            continue;


        if( !monster->flagIsSet(M_DM_FOLLOW) &&
            !monster->flagIsSet(M_PERMANENT_MONSTER) &&
            !monster->flagIsSet(M_PASSIVE_EXIT_GUARD) &&
            Random::get(1,100) <= 50
        ) {
            if(monster->flee(true) == 2)
                continue;
            monster->stun(Random::get(5,8));
            continue;
        }

        stunTime = Random::get(5, std::max(6, player->getLevel()/2));
        monster->stun(stunTime);
        broadcast((std::shared_ptr<Socket>)nullptr, room, "^b%M is frozen in terror!", monster.get());
    }

    return(0);
}

