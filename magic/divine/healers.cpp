/*
 * healers.cpp
 *   Functions that deal with healer class abilities.
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

#include <ctime>                     // for time
#include <string>                    // for allocator, string
#include <type_traits>               // for enable_if<>::type

#include "cmd.hpp"                   // for cmd
#include "config.hpp"                // for Config, gConfig
#include "deityData.hpp"             // for DeityData
#include "dice.hpp"                  // for Dice
#include "flags.hpp"                 // for P_AFK, M_SPECIAL_UNDEAD, P_LAG_P...
#include "global.hpp"                // for CreatureClass, CreatureClass::DE...
#include "lasttime.hpp"              // for lasttime
#include "magic.hpp"                 // for SpellData, splHallow, splUnhallow
#include "monType.hpp"               // for PLAYER
#include "mud.hpp"                   // for LT, LT_HOLYWORD, LT_TURN, LT_LAY...
#include "mudObjects/container.hpp"  // for Container
#include "mudObjects/creatures.hpp"  // for Creature, CHECK_DIE
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/objects.hpp"    // for Object, ObjectType, ObjectType::...
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, bonus, up
#include "random.hpp"                // for Random
#include "realm.hpp"                 // for EARTH
#include "statistics.hpp"            // for Statistics
#include "stats.hpp"                 // for Stat
#include "commands.hpp"             


//*********************************************************************
//                      cmdEnthrall
//*********************************************************************
// This ability allows a cleric of Aramon to charm undead creatures
// and players. -- TC

int cmdEnthrall(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    int     dur=0, chance=0;
    long    i=0, t=0;

    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
        return(0);

    if(!player->isCt()) {
        if(!player->knowsSkill("enthrall")) {
            player->print("You don't possess the skills to enthrall the undead!\n");
            return(0);
        }
        if(player->isBlind()) {
            player->printColor("^CYou must be able to see your enemy to do that!\n");
            return(0);
        }
    }

    i = LT(player, LT_HYPNOTIZE);
    t = time(nullptr);
    if(i > t && !player->isDm()) {
        player->pleaseWait(i-t);
        return(0);
    }

    dur = 300 + Random::get(1, 30) * 10 + bonus(player->piety.getCur()) * 30 +
          (int)(player->getSkillLevel("enthrall") * 5);


    if(!(creature = player->findVictim(cmnd, 1, true, false, "Enthrall whom?\n", "You don't see that here.\n")))
        return(0);

    if(creature->isMonster()) {
        if(creature->getAsMonster()->isEnemy(player)) {
            player->print("Not while you are already fighting %s.\n", creature->himHer());
            return(0);
        }
    }
    if(!player->canAttack(creature))
        return(0);

    if( !player->alignInOrder() &&
        !player->checkStaff("You are not wicked enough in %s's eyes to do that.\n", gConfig->getDeity(player->getDeity())->getName().c_str())
    )
        return(0);

    if( !creature->isUndead() &&
        !player->checkStaff("You may not enthrall the living with %s's power.\n", gConfig->getDeity(player->getDeity())->getName().c_str())
    )
        return(0);

    player->smashInvis();
    player->lasttime[LT_HYPNOTIZE].ltime = t;
    player->lasttime[LT_HYPNOTIZE].interval = 300L;

    chance = std::min(90, 40 + (int)((player->getSkillLevel("enthrall") - creature->getLevel()) * 10) +
            4 * bonus(player->piety.getCur()));

    if(creature->flagIsSet(M_PERMENANT_MONSTER))
        chance -= 25;

    if(player->isDm())
        chance = 101;

    if((chance < Random::get(1, 100)) && (chance != 101)) {
        player->print("You were unable to enthrall %N.\n", creature.get());
        player->checkImprove("enthrall",false);
        broadcast(player->getSock(), player->getParent(), "%M tried to enthrall %N.",player.get(), creature.get());
        if(creature->isMonster()) {
            creature->getAsMonster()->addEnemy(player);
            return(0);
        }
        creature->printColor("^r%M tried to enthrall you with the power of %s.\n", player.get(), gConfig->getDeity(player->getDeity())->getName().c_str());

        return(0);
    }


    if(!creature->isCt() &&
        creature->chkSave(MEN, player, 0))
    {
        player->printColor("^yYour enthrall failed!\n");
        creature->print("Your mind tingles as you brush off %N's enthrall.\n", player.get());
        player->checkImprove("enthrall",false);
        return(0);
    }

    if( (creature->isPlayer() && creature->isEffected("resist-magic")) ||
        (creature->isMonster() && creature->isEffected("resist-magic"))
    )
        dur /= 2;

    player->print("You enthrall %N with the power of %s.\n", creature.get(), gConfig->getDeity(player->getDeity())->getName().c_str());
    player->checkImprove("enthrall",true);

    broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M flashes %s symbol of %s to %N, enthralling %s.",
        player.get(), player->hisHer(), gConfig->getDeity(player->getDeity())->getName().c_str(), creature.get(),
        player->himHer());
    creature->print("%M's chant of %s's word enthralls you.\n", player.get(), gConfig->getDeity(player->getDeity())->getName().c_str());

    player->addCharm(creature);

    creature->stun(std::max(1,7+Random::get(1,2)+bonus(player->piety.getCur())));

    creature->lasttime[LT_CHARMED].ltime = time(nullptr);
    creature->lasttime[LT_CHARMED].interval = dur;

    creature->setFlag(creature->isPlayer() ? P_CHARMED : M_CHARMED);

    return(0);
}


//*********************************************************************
//                      cmdEarthSmother
//*********************************************************************
// This command allows a dwarven cleric/paladin of Gradius to smother enemies

int cmdEarthSmother(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    std::shared_ptr<Player> pCreature=nullptr;
    std::shared_ptr<Monster> mCreature=nullptr;
    long    i=0, t=0;
    int     chance=0, dmg=0;

    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
        return(0);


    if(!player->isCt()) {
        if(!player->knowsSkill("smother")) {
            player->print("You lack the skills to smother people with earth.\n");
            return(0);
        }
        if(!player->alignInOrder()) {
            player->print("%s requires that you are neutral or light blue to do that.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
            return(0);
        }
    }
    if(!(creature = player->findVictim(cmnd, 1, true, false, "Smother whom?\n", "You don't see that here.\n")))
        return(0);

    pCreature = creature->getAsPlayer();
    mCreature = creature->getAsMonster();

    player->smashInvis();
    player->interruptDelayedActions();

    if(!player->canAttack(creature))
        return(0);

    if(mCreature && mCreature->getBaseRealm() == EARTH &&
        !player->checkStaff("Your attack would be ineffective.\n"))
        return(0);


    double level = player->getSkillLevel("smother");
    i = LT(player, LT_SMOTHER);
    t = time(nullptr);
    if(i > t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    player->lasttime[LT_SMOTHER].ltime = t;
    player->updateAttackTimer();
    player->lasttime[LT_SMOTHER].interval = 120L;


    chance = ((int)(level - creature->getLevel()) * 20) + bonus(player->piety.getCur()) * 5 + 25;
    chance = std::min(chance, 80);
    if(creature->isEffected("resist-earth"))
        chance /= 2;
    if(!pCreature && creature->isEffected("immune-earth"))
        chance = 0;

    dmg = Random::get((int)(level*3), (int)(level*4));
    if(creature->isEffected("resist-earth"))
        dmg = Random::get(1, (int)level);
    if(player->getRoomParent()->flagIsSet(R_EARTH_BONUS) && player->getRoomParent()->flagIsSet(R_ROOM_REALM_BONUS))
        dmg = dmg*3/2;

    if(mCreature)
        mCreature->addEnemy(player);

    if(!player->isCt()) {
        if(Random::get(1, 100) > chance) {
            player->print("You failed to earth smother %N.\n", creature.get());
            broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M tried to earth smother %N.", player.get(), creature.get());
            creature->print("%M tried to earth smother you!\n", player.get());
            player->checkImprove("smother", false);
            return(0);
        }
        if(creature->chkSave(BRE, player, 0)) {
            player->printColor("^yYour earth-smother was interrupted!\n");
            creature->print("You manage to partially avoid %N's smothering earth.\n", player.get());
            dmg /= 2;
        }
    }

    player->printColor("You smothered %N for %s%d^x damage.\n", creature.get(), player->customColorize("*CC:DAMAGE*").c_str(), dmg);
    player->checkImprove("smother", true);
    player->statistics.attackDamage(dmg, "earth smother");

    if(creature->isEffected("resist-earth"))
        player->print("%M resisted your smother!\n", creature.get());

    creature->printColor("%M earth-smothered you for %s%d^x damage!\n", player.get(), creature->customColorize("*CC:DAMAGE*").c_str(), dmg);
    broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M earth-smothered %N!", player.get(), creature.get());

    player->doDamage(creature, dmg, CHECK_DIE);
    return(0);
}

//*********************************************************************
//                      cmdStarstrike
//*********************************************************************
// This command allows clerics of Mara to call a starstrike on their enemies

int cmdStarstrike(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> target=nullptr;
    long    i=0, t=0;
    int     chance=0, dmg=0, roll=0;
    bool    noHighLevelDiff=false;

    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
        return(0);

    if(!player->isCt()) {
        if(!player->knowsSkill("starstrike")) {
            *player << "You do not have the ability to call a starstrike.\n";
            return(0);
        }
        if(player->getDeity() != MARA || player->getClass() != CreatureClass::CLERIC) {
            *player << "Only clerics of Mara have the ability to call a starstrike.\n";
            return(0);
        }
        if (isDay()) {
            *player << "You may only call a starstrike at night.\n";
            return(0);
        }
        if(!player->alignInOrder()) {
            *player << gConfig->getDeity(player->getDeity())->getName() << " refuses your starstrike. You need to cleanse your soul first.\n";
            return(0);
        }
    }
    if(!(target = player->findVictim(cmnd, 1, true, false, "Starstrike whom?\n", "You don't see that here.\n")))
        return(0);

    player->smashInvis();
    player->interruptDelayedActions();

    if(!player->canAttack(target))
        return(0);

    double level = player->getSkillLevel("starstrike");

    i = LT(player, LT_STARSTRIKE);
    t = time(nullptr);
    if(i > t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    player->lasttime[LT_STARSTRIKE].ltime = t;
    player->updateAttackTimer();

    // Time between uses scales less with higher skill level
    if (level >= 40.0)
        player->lasttime[LT_STARSTRIKE].interval = 60L;
    else if (level >=31.0)
        player->lasttime[LT_STARSTRIKE].interval = 75L;
    else if (level >=25.0)
        player->lasttime[LT_STARSTRIKE].interval = 90L;
    else if (level >=19.0)
        player->lasttime[LT_STARSTRIKE].interval = 105L;
    else if (level >=13.0)
        player->lasttime[LT_STARSTRIKE].interval = 120L;
    else 
        player->lasttime[LT_STARSTRIKE].interval = 135L;

    chance = 450 + player->piety.getCur() + ((((int)level-target->getLevel())*100)/2);
    chance = std::min(chance, 950);

    noHighLevelDiff = ((int)level - target->getLevel()) < 10;

    //resist-magic effect reduces chance unless player is 10+ levels higher than target
    if(target->isEffected("resist-magic") && noHighLevelDiff)
        chance /= 2;
    
    dmg = Random::get((int)(level*4), (int)(level*5)/4) + Random::get(1,10);
    
    if(player->getRoomParent()->flagIsSet(R_MAGIC_BONUS))
        dmg = dmg*3/2;
    if(target->isUndead())
        dmg = dmg*3/2;

    //resist-magic effect overrides all above damage calculations, unless player is 10+ levels higher than target
    if(target->isEffected("resist-magic") && noHighLevelDiff)
        dmg = Random::get(1, (int)level);

    if(target->isMonster()) {
        target->getAsMonster()->addEnemy(player);
        //Mobs' innate magic resistance can make them immune to starstrike
        if(Random::get(1,100) <= target->getAsMonster()->getMagicResistance()) {
            *player << ColorOn << "^MYour starstrike had no effect on " << target << ".\n" << ColorOff;
            player->lasttime[LT_STARSTRIKE].interval = 20L;
            return(0);
        }
        
    }

    roll = Random::get(1, 1000);
   /* if (player->isCt() || player->flagIsSet(P_PTESTER)) {
        *player << "Chance: " << chance << "%\n";
        *player << "Roll: " << roll << "\n";
    }
    */

    if(!player->isCt()) {
        if( roll > chance) {
            *player << "Your starstrike missed " << target << ".\n";
            broadcast(player->getSock(), target->getSock(), player->getRoomParent(), "%M's starstrike missed %N!", player.get(), target.get());
            *target << setf(CAP) << player << " tried to starstrike you!\n";
            player->lasttime[LT_STARSTRIKE].interval = 10L;
            player->checkImprove("starstrike", false);
            return(0);
        }
        if(target->chkSave(SPL, player, 0)) {
            *player << ColorOn << "^WYour starstrike was only partially effective.\n" << ColorOff;
            *target << "You partially avoided " << player << "'s starstrike.\n";
            dmg /= 2;
        }
    }

    *player << ColorOn << "^WYour starstrike hit " << target << " for " << player->customColorize("*CC:DAMAGE*") << dmg << " ^Wdamage!\n" << ColorOff;
    player->checkImprove("starstrike", true);
    player->statistics.attackDamage(dmg, "starstrike");

    if(target->isEffected("resist-magic") && noHighLevelDiff)
        *player << setf(CAP) << target << "'s resist-magic dampened your starstrike!\n";

    *target << ColorOn << "^W" << setf(CAP) << player << "'s starstrike hit you for " << target->customColorize("*CC:DAMAGE*") << dmg << " ^Wdamage!\n" << ColorOff;
    broadcast(player->getSock(), target->getSock(), player->getRoomParent(), "%M hit %N with a starstrike!", player.get(), target.get());

    player->doDamage(target, dmg, CHECK_DIE);
    return(0);
}

//*********************************************************************
//                      cmdLayHands
//*********************************************************************
// This will allow paladins to lay on hands

int cmdLayHands(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    int     num=0;
    long    t=time(nullptr), i=0;

    player->clearFlag(P_AFK);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->knowsSkill("hands")) {
        *player << "You do not have the ability to heal with lay on hands.\n";
        return(0);
    }

    if(!player->isCt()) {
        if(player->getAdjustedAlignment() < ROYALBLUE) {
            *player << "You may only do that when your alignment is totally pure.\n";
            return(0);
        }
    }

    i = player->lasttime[LT_LAY_HANDS].ltime + player->lasttime[LT_LAY_HANDS].interval;
      
    if(i > t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }


    // Lay on self
    if(cmnd->num == 1) {

        if (player->hp.getCur() >= player->hp.getMax()) {
            *player << "You are already at full health.\n";
            return(0);
        }
        //TODO: Change heal amount calc to use hands skill level once skill trainers are put in. Until then, use player level
        num = Random::get( (int)(player->getLevel()*4), (int)(player->getLevel()*5) ) + Random::get(1,10);

        *player << "You regain " << std::min<int>(num,(player->hp.getMax() - player->hp.getCur())) << " hit points.\n";

        player->doHeal(player, num);

        broadcast(player->getSock(), player->getParent(), "%M heals %sself with the power of %s.",
            player.get(), player->himHer(), gConfig->getDeity(player->getDeity())->getName().c_str());

        *player << "You feel much better now.\n";
        player->checkImprove("hands", true);
        player->lasttime[LT_LAY_HANDS].ltime = t;
        player->lasttime[LT_LAY_HANDS].interval = 600L;

    } else {
        // Lay hands on another player or monster

        cmnd->str[1][0] = up(cmnd->str[1][0]);
        creature = player->getParent()->findCreature(player, cmnd->str[1], cmnd->val[1], false);
        if(!creature) {
            *player << "That person is not here.\n";
            return(0);
        }

        if(creature->pFlagIsSet(P_LINKDEAD) && creature->getClass() !=  CreatureClass::LICH) {
            *player << "That won't work on " << creature << " right now.\n";
            return(0);
        }

        if(!player->isCt() && creature->isUndead()) {
            *player << "That will not work on undead.\n";
            return(0);
        }

        if(creature->hp.getCur() >= creature->hp.getMax()) {
            *player << "That is not necessary, since " << creature << " is already at full health.\n";
            return(0);
        }

        if(creature->getAdjustedAlignment() < NEUTRAL) {
            *player << "Unfortunately, " << creature << " is not pure enough of heart right now.\n";
            return(0);
        }

        num = Random::get( (int)(player->getLevel()*4), (int)(player->getLevel()*5) ) + Random::get(1,10);

        *player << "You heal " << creature << " with the power of " << gConfig->getDeity(player->getDeity())->getName() << ".\n";
        *player << creature->upHeShe() <<  " gained " << (std::min<int>(num,(creature->hp.getMax() - creature->hp.getCur()))) << " hit points.\n";

        *creature << setf(CAP) << player << " lays " << player->hisHer() << " hand upon your pate.\n";
        *creature << "You regain " << (std::min<int>(num,(creature->hp.getMax() - creature->hp.getCur()))) << " hit points.\n";


        player->doHeal(creature, num);

        broadcast(player->getSock(), creature->getSock(), creature->getRoomParent(), "%M heals %N with the power of %s.",
            player.get(), creature.get(), gConfig->getDeity(player->getDeity())->getName().c_str());

        *creature << "You feel much better now.\n";

        player->lasttime[LT_LAY_HANDS].ltime = t;
        player->lasttime[LT_LAY_HANDS].interval = 600L;
        player->checkImprove("hands", true);

    }

    return(0);
}

//*********************************************************************
//                      cmdPray
//*********************************************************************
// This function allows clerics, paladins, and druids to pray every 10
// minutes to increase their piety for a duration of 5 minutes.

int cmdPray(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    i=0, t=0;
    int     chance=0;

    std::ostringstream failStr, succStr;

    player->clearFlag(P_AFK);


    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->knowsSkill("pray")) {
        *player << "You say a quick prayer.\n";
        return(0);
    }

    if(player->isEffected("pray") || player->isEffected("dkpray")) {
        *player << gConfig->getDeity(player->getDeity())->getName() << " has already answered your prayers.\n";
        return(0);
    }

    if(player->getClass() == CreatureClass::DEATHKNIGHT) {
        if(player->isEffected("strength")) {
            *player << "Your magically enhanced strength prevents you from praying.\n";
            return(0);
        }
        if(player->isEffected("enfeeblement")) {
            *player << "Your magically reduced strength prevents you from praying.\n";
            return(0);
        }
    } else {
        if(player->isEffected("prayer")) {
            *player << "Your current spiritual blessing prevents you from praying.\n";
            return(0);
        }
        if(player->isEffected("damnation")) {
            *player << "Your current spiritual damnation prevents you from praying.\n";
            return(0);
        }
    }

    i = player->lasttime[LT_PRAY].ltime;
    t = time(nullptr);

    if(t - i < 600L && !player->isStaff()) {
        player->pleaseWait(600L-t+i);
        return(0);
    }

    if(player->getClass()==CreatureClass::DEATHKNIGHT)
        chance = std::min<int>(8500, (player->getSkillLevel("pray") * 750) + player->strength.getCur() * 5);
    else
        chance = std::min<int>(8500, (player->getSkillLevel("pray") * 1500) + player->piety.getCur() );

   if (player->isCt()) {
        *player << "Pray chance (adjusted): " << chance << "\n";
        *player << "DKNIGHT (unadjusted): " << (player->getSkillLevel("pray") * 750) + player->strength.getCur() * 5 << ".\n";
        *player << "OTHERS (unadjusted): " << (player->getSkillLevel("pray") * 1500) + player->piety.getCur()  << ".\n";

   }

    switch(player->getDeity()) {
            case ARAMON:
                *player << "You pray for the " << (player->getClass() == CreatureClass::CLERIC ? "wrath":"dark power") << " of " << gConfig->getDeity(player->getDeity())->getName() << ".\n";
                broadcast(player->getSock(),player->getParent(), "%M prays for the %s of %s.", player.get(), player->getClass() == CreatureClass::CLERIC ? "wrath":"dark power", gConfig->getDeity(player->getDeity())->getName().c_str());
                failStr << gConfig->getDeity(player->getDeity())->getName() << " scoffed at your prayer.\n";
                succStr << gConfig->getDeity(player->getDeity())->getName() << " has reluctantly granted his " << (player->getClass() == CreatureClass::CLERIC ? "wrath":"dark power") << ". It'd be unwise of you to fail him.\n";
            break;
            case CERIS:
                *player << "You expose yourself and dance, doing a fertility ritual for " << gConfig->getDeity(player->getDeity())->getName() << ".\n";
                broadcast(player->getSock(),player->getParent(), "%M exposes %sself and dances, doing a fertility ritual for %s.", player.get(), player->himHer(), gConfig->getDeity(player->getDeity())->getName().c_str());
                failStr << gConfig->getDeity(player->getDeity())->getName() << " found your exposition dance inadequate.\n";
                succStr << gConfig->getDeity(player->getDeity())->getName() << " found your exposition dance worthy and granted your prayer.\n";
            break;
            case ENOCH:
                *player << "You pray for the rightenousness of " << gConfig->getDeity(player->getDeity())->getName() << ".\n";
                broadcast(player->getSock(),player->getParent(), "%M prays for the righteousness of %s.", player.get(), gConfig->getDeity(player->getDeity())->getName().c_str());
                failStr << gConfig->getDeity(player->getDeity())->getName() << " found your worthiness lacking.\n";
                succStr << gConfig->getDeity(player->getDeity())->getName() << " has granted your prayer. Go forth and smite injustice in his name!\n";
            break;
            case GRADIUS:
                *player << "You rub some dirt between your hands, praying for wisdom from " << gConfig->getDeity(player->getDeity())->getName() << ".\n";
                broadcast(player->getSock(),player->getParent(), "%M rubs dirt between %s hands, praying for wisdom from %s.", player.get(), player->hisHer(), gConfig->getDeity(player->getDeity())->getName().c_str());
                failStr << gConfig->getDeity(player->getDeity())->getName() << " is subbornly silent to your prayer.\n";
                succStr << gConfig->getDeity(player->getDeity())->getName() << " has proudly granted your prayer.\n";
            break;
            case ARES:
                *player << "You smear blood across your face, praying to " << gConfig->getDeity(player->getDeity())->getName() << " for good fortune in battle.\n";
                 broadcast(player->getSock(),player->getParent(), "%M smears blood across %s face, praying to %s for good fortune in battle.", player.get(), player->hisHer(), gConfig->getDeity(player->getDeity())->getName().c_str());
                failStr << gConfig->getDeity(player->getDeity())->getName() << " found your blood ritual lacking.\n";
                succStr << "Rage and violence flow through you as " << gConfig->getDeity(player->getDeity())->getName() << " grants your prayer.\n";
            break;
            case KAMIRA:
                *player << "You slam a shot of whiskey and toss your holy dice, praying for the luck of " << gConfig->getDeity(player->getDeity())->getName() << ".\n";
                broadcast(player->getSock(),player->getParent(), "%M prays to %s, slamming a shot of whiskey and throwing %s holy dice.", player.get(), gConfig->getDeity(player->getDeity())->getName().c_str(), player->hisHer());
                failStr << "Unfortunately, " << gConfig->getDeity(player->getDeity())->getName() << " didn't like your roll and whimsically refused your prayer.\n";
                succStr << gConfig->getDeity(player->getDeity())->getName() << " decided your roll was adequate and happily granted your prayer.\n";
            break;
            case LINOTHAN:
                *player << "You begin an intricate sword dancing ritual, praying to " << gConfig->getDeity(player->getDeity())->getName() << ".\n";
                broadcast(player->getSock(),player->getParent(), "%M does an intricate sword dancing ritual, praying to %s.", player.get(), gConfig->getDeity(player->getDeity())->getName().c_str());
                failStr << gConfig->getDeity(player->getDeity())->getName() << " found your sword dancing ritual lacking.\n";
                succStr << gConfig->getDeity(player->getDeity())->getName() << " was impressed by your sword dancing ritual and has granted your prayer.\n";
            break;
            case ARACHNUS:
                *player << "You prostrate yourself and beg to " << gConfig->getDeity(player->getDeity())->getName() << " for favor.\n";
                broadcast(player->getSock(),player->getParent(), "%M prostrates %sself and begs for the favor of %s.", player.get(), player->himHer(), gConfig->getDeity(player->getDeity())->getName().c_str());
                failStr << gConfig->getDeity(player->getDeity())->getName() << " blatantly ignored your pathetic prostration.\n";
                succStr << gConfig->getDeity(player->getDeity())->getName() << " has whimsically granted you favor.\n";
            break;
            case MARA:
                *player << "You fall to your knees and pray for the guidance of " << gConfig->getDeity(player->getDeity())->getName() << ".\n";
                broadcast(player->getSock(),player->getParent(),"%M falls to %s knees and prays for guidance from %s.", player.get(), player->hisHer(), gConfig->getDeity(player->getDeity())->getName().c_str());
                failStr << gConfig->getDeity(player->getDeity())->getName() << " refused to answer your prayer.\n";
                succStr << gConfig->getDeity(player->getDeity())->getName() << " has responded and warmly granted your prayer.\n";
            break;
            case JAKAR:
                *player << "You ritually count your coins, praying for wisdom from " << gConfig->getDeity(player->getDeity())->getName() << ".\n";
                broadcast(player->getSock(),player->getParent(),"%M ritually counts %s coins, praying for wisdom from %s.", player.get(), player->hisHer(), gConfig->getDeity(player->getDeity())->getName().c_str());
                failStr << gConfig->getDeity(player->getDeity())->getName() << " ignored your counting ritual and didn't answer your prayer.\n";
                succStr << gConfig->getDeity(player->getDeity())->getName() << " has answered your prayer. Go forth in the name of profits!\n";
            break;
            default:
                *player << "You rant and rave over the fallacy of the worlds' religions, mockingly pretending to pray.\n";
                broadcast(player->getSock(), player->getParent(), "%M rants and raves over the fallacy of all the worlds' religions, mockingly pretending to pray.", player.get());
                failStr << "Fortunately, the universe refused to listen to you, just as it should be!\n";
                succStr << "WTF! The universe...or something...answered your prayer!\n";
            break;
        }


 

    if(Random::get(1, 10000) <= chance) {
        player->lasttime[LT_PRAY].ltime = t;

        *player << succStr.str();

        if(player->getClass() !=  CreatureClass::DEATHKNIGHT) {
            player->addEffect("pray", 450L, 50);
            player->lasttime[LT_PRAY].interval = 600L;
        } else {
            player->addEffect("dkpray", 240L, 30);
            player->computeAC();
            player->computeAttackPower();
            player->lasttime[LT_PRAY].interval = 600L;
        }
        player->checkImprove("pray", true);
    } else {
        
        *player << failStr.str();
        player->checkImprove("pray", false);
        player->lasttime[LT_PRAY].ltime = t - 590L;
    }

    return(0);
}


bool Creature::kamiraLuck(const std::shared_ptr<Creature>&attacker) {
    int chance=0;

    if(!(cClass == CreatureClass::CLERIC && deity == KAMIRA) && !isCt())
        return(false);

    chance = (level / 10)+3;
    if(Random::get(1,100) <= chance) {
        broadcast(getSock(), getRoomParent(), "^YA feeling of deja vous comes over you.");
        printColor("^YThe luck of %s was with you!\n", gConfig->getDeity(KAMIRA)->getName().c_str());
        printColor("^C%M missed you.\n", attacker.get());
        attacker->print("You thought you hit, but you missed!\n");
        return(true);
    }
    return(false);
}



int Creature::getTurnChance(const std::shared_ptr<Creature>& target) {
    double  adjLevel = 0.0;
    int     chance=0, bns=0;


    adjLevel = getSkillLevel("turn");

    switch (getDeity()) {
    case CERIS:
    case LINOTHAN:
        if(getAdjustedAlignment() <= (getDeity()==LINOTHAN ? RED:REDDISH))
            adjLevel += (getDeity()==LINOTHAN && isDay() ? 1:2);
        else
            adjLevel -= (getDeity()==LINOTHAN ? 1:2);
        break;
    case MARA:
        if(getAdjustedAlignment() <= RED && !isDay())
            adjLevel += 1;
        else
            adjLevel -= 1;
        break;
    case ENOCH:
        if(getAdjustedAlignment() <= REDDISH)
            adjLevel -=4;
        break;
    default:
        break;
    }

    adjLevel = std::max<double>(1, adjLevel);


    bns = bonus(piety.getCur());

    chance = (int)((adjLevel - target->getLevel()) * 20) +
            bns*5 + (getClass() == CreatureClass::PALADIN ? 15:35);
    chance = std::min(chance, 85);

    if(target->isPlayer()) {
        if(isDm())
            chance = 101;
    } else {
        if(target->flagIsSet(M_SPECIAL_UNDEAD))
            chance = std::min(chance, 15);
    }


    return(chance);
}


//*********************************************************************
//                      cmdTurn
//*********************************************************************

int cmdTurn(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> target=nullptr;
    long    i=0, t=0;
    int     m=0, dmg=0, dis=5, chance=0;
    int     roll=0, disroll=0, bns=0;


    player->clearFlag(P_AFK);

    bns = bonus(player->piety.getCur());

    if(!player->ableToDoCommand())
        return(0);

    if(!player->isCt() && !player->knowsSkill("turn")) {
        if(player->getClass() == CreatureClass::CLERIC)
            *player << gConfig->getDeity(player->getDeity())->getName() << " does not grant you the power to turn undead.\n";
        else
            *player << "You do not have the ability to turn undead.\n";
        return(0);
    }

    if(!(target = player->findVictim(cmnd, 1, true, false, "Turn whom?\n", "You don't see that here.\n")))
        return(0);

    
    if(!player->canAttack(target))
        return(0);
    if(target->isMonster()) {
        if(player->flagIsSet(P_LAG_PROTECTION_SET)) {    // Activates lag protection.
            player->setFlag(P_LAG_PROTECTION_ACTIVE);
        }

        if( !target->isUndead() &&
            !player->checkStaff("You may only turn undead monsters.\n"))
            return(0);

    } else {

        if( !target->isUndead() &&
            !player->checkStaff("%M is not undead.\n", target.get()))
            return(0);

    }

    player->smashInvis();
    player->interruptDelayedActions();

    i = LT(player, LT_TURN);
    t = time(nullptr);

    if(i > t && !player->isDm()) {
        player->pleaseWait(i-t);
        return(0);
    }


    if(target->isMonster())
        target->getAsMonster()->addEnemy(player);

    player->lasttime[LT_TURN].ltime = t;
    player->updateAttackTimer(false);

    if( (player->getDeity() == ENOCH || player->getDeity() == LINOTHAN) &&
        player->getLevel() >= 10
    ) {
        if(LT(player, LT_HOLYWORD) <= t) {
            player->lasttime[LT_HOLYWORD].ltime = t;
            player->lasttime[LT_HOLYWORD].interval = 3L;
        }
    }

    player->lasttime[LT_TURN].interval = 30L;


    // determine damage turn will do
    dmg = std::max<int>(1, target->hp.getCur() / 2);


    switch(player->getDeity()) {
    case CERIS:
    case LINOTHAN:
        if(!player->alignInOrder()) {
            dis = -1;
            dmg /= 2;
        }
        else
        {
            dis = (player->getDeity()==CERIS ? 10:7);
            *player << "The power of " << gConfig->getDeity(player->getDeity())->getName() << " flows through you.\n";
        }
        break;
    case MARA:
    case ENOCH:
        if(!player->alignInOrder()) {
            dis = -1;
            dmg /= 2;
        }
        break;

    default:
        break;
    }

    chance = player->getTurnChance(target);

    roll = Random::get(1,100);

    if(roll > chance && !player->isStaff()) {
        *player << "You failed to turn " << target << "\n", target.get();
        player->checkImprove("turn", false);
        if(target->mFlagIsSet(M_SPECIAL_UNDEAD))
            *player << ColorOn << "^y" << setf(CAP) << " greatly resisted your efforts to turn " << target->himHer() << ColorOff;
        broadcast(player->getSock(), player->getParent(), "%M failed to turn %N.", player.get(), target.get());
        return(0);
    }

    disroll = Random::get(1,100);

    if((disroll < (dis + bns) && !target->flagIsSet(M_SPECIAL_UNDEAD)) || player->isDm()) {
        *player << ColorOn << "^BYou disintegrated " << target << "!\n" << ColorOff;
        broadcast(player->getSock(), player->getParent(), "^B%M disintegrated %N.", player.get(), target.get());
        // TODO: SKILLS: add a bonus to this
        player->checkImprove("turn", true);
        if(target->isMonster())
            target->getAsMonster()->adjustThreat(player, target->hp.getCur());
        //player->statistics.attackDamage(target->hp.getCur(), "turn undead");

        target->die(player);
    } else {

        m = std::min<int>(target->hp.getCur(), dmg);
        //player->statistics.attackDamage(dmg, "turn undead");

        if(target->isMonster())
            target->getAsMonster()->adjustThreat(player, m);

        *player << ColorOn << "^YYou turned " << target << " for ^W" << dmg << "^Y damage.\n" << ColorOff;
        player->checkImprove("turn", true);

        broadcast(player->getSock(), player->getParent(), "^Y%M turned %N.", player.get(), target.get());
        player->doDamage(target, dmg, CHECK_DIE);

    }

    return(0);

}

//*********************************************************************
//                      cmdRenounce
//*********************************************************************
// This function allows paladins and death knights to renounce one
// another with the power of their respective gods, doing damage or
// destroying one another utterly.  --- TC

int cmdRenounce(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> target=nullptr;
    long    i=0, t=0;
    int     chance=0, dmg=0;

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("renounce")) {
        player->print("You lack the understanding to renounce.\n");
        return(0);
    }
    
    if(!(target = player->findVictim(cmnd, 1, true, false, "Renounce whom?\n", "You don't see that here.\n")))
        return(0);

    double level = player->getSkillLevel("renounce");
    if(target->isMonster()) {
        std::shared_ptr<Monster>  mTarget = target->getAsMonster();
        if( (!player->isCt()) &&
            (((player->getClass() == CreatureClass::PALADIN) && (target->getClass() !=  CreatureClass::DEATHKNIGHT)) ||
            ((player->getClass() == CreatureClass::DEATHKNIGHT) && (target->getClass() !=  CreatureClass::PALADIN)))
        ) {
            player->print("%s is not of your opposing class.\n", target->upHeShe());
            return(0);
        }

        player->smashInvis();
        player->interruptDelayedActions();

        if(player->flagIsSet(P_LAG_PROTECTION_SET)) {    // Activates lag protection.
            player->setFlag(P_LAG_PROTECTION_ACTIVE);
        }

        i = LT(player, LT_RENOUNCE);
        t = time(nullptr);

        if(i > t && !player->isCt()) {
            player->pleaseWait(i-t);
            return(0);
        }

        if(!player->checkAttackTimer(true))
            return(0);

        if(!player->canAttack(target))
            return(0);

        if(mTarget)
            mTarget->addEnemy(player);

        player->updateAttackTimer(false);
        player->lasttime[LT_RENOUNCE].ltime = t;
        player->lasttime[LT_RENOUNCE].interval = 60L;

        chance = ((int)(level - target->getLevel())*20) + bonus(player->piety.getCur()) * 5 + 25;
        if(target->flagIsSet(M_PERMENANT_MONSTER))
            chance -= 15;
        chance = std::min(chance, 90);
        if(player->isDm())
            chance = 101;

        if(Random::get(1,100) > chance) {
            player->print("Your god refuses to renounce %N.\n", target.get());
            player->checkImprove("renounce", false);
            broadcast(player->getSock(), player->getParent(), "%M tried to renounce %N.", player.get(), target.get());
            return(0);
        }


        if(!player->isCt()) {
            if(target->chkSave(DEA, player, 0)) {
                player->printColor("^yYour renounce failed!\n");
                player->checkImprove("renounce", false);
                target->print("%M failed renounce you.\n", player.get());
                return(0);
            }
        }

        if(Random::get(1,100) > 90 - bonus(player->piety.getCur()) || player->isDm()) {
            player->print("You destroy %N with your faith.\n", target.get());
            player->checkImprove("renounce", true);
            broadcast(player->getSock(), player->getParent(), "The power of %N's faith destroys %N.", player.get(), target.get());
            mTarget->adjustThreat(player, target->hp.getCur());

            //player->statistics.attackDamage(target->hp.getCur(), "renounce");
            target->die(player);
        }
        else {
            dmg = std::max<int>(1, target->hp.getCur() / 2);
            //player->statistics.attackDamage(dmg, "renounce");

            player->printColor("You renounced %N for %s%d^x damage.\n", target.get(), player->customColorize("*CC:DAMAGE*").c_str(), dmg);
            player->checkImprove("renounce", true);
            //target->print("%M renounced you for %d damage!\n", player.get(), dmg);
            broadcast(player->getSock(), player->getParent(), "%M renounced %N.", player.get(), target.get());
            player->doDamage(target, dmg, CHECK_DIE);
        }

        return(0);
    } else {
        if( (!player->isCt()) &&
            (((player->getClass() == CreatureClass::PALADIN) && (target->getClass() !=  CreatureClass::DEATHKNIGHT)) ||
            ((player->getClass() == CreatureClass::DEATHKNIGHT) && (target->getClass() !=  CreatureClass::PALADIN)))
        ) {
            player->print("%s is not of your opposing class.\n", target->upHeShe());
            return(0);
        }
        if(!player->canAttack(target))
            return(0);


        player->smashInvis();
        player->interruptDelayedActions();

        i = LT(player, LT_RENOUNCE);
        t = time(nullptr);

        if(i > t && !player->isCt()) {
            player->pleaseWait(i-t);
            return(0);
        }

        if(!player->checkAttackTimer())
            return(0);

        player->updateAttackTimer(false);
        player->lasttime[LT_RENOUNCE].ltime = t;
        player->lasttime[LT_RENOUNCE].interval = 60L;

        chance = ((int)(level - target->getLevel())*20) +
                 bonus(player->piety.getCur()) * 5 + 25;
        chance = std::min(chance, 90);
        if(player->isDm())
            chance = 101;
        if(Random::get(1,100) > chance) {
            player->print("Your god refuses to renounce %N.\n", target.get());
            player->checkImprove("renounce", false);
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to renounce %N.", player.get(), target.get());
            target->print("%M tried to renounce you!\n", player.get());
            return(0);
        }


        if(!target->isCt()) {
            if(target->chkSave(DEA, player, 0)) {
                player->printColor("^yYour renounce failed!\n");
                player->checkImprove("renounce", false);
                target->print("%M failed renounce you.\n", player.get());
                return(0);
            }
        }

        if(Random::get(1,100) > 90 - bonus(player->piety.getCur()) || player->isDm()) {
            player->print("You destroyed %N with your faith.\n", target.get());
            player->checkImprove("renounce", true);
            target->print("The power of %N's faith destroys you!\n", player.get());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M destroys %N with %s faith!", player.get(), target.get(), target->hisHer());
            //player->statistics.attackDamage(target->hp.getCur(), "renounce");
            target->die(player);

        } else {
            dmg = std::max<int>(1, target->hp.getCur() / 2);
            //player->statistics.attackDamage(dmg, "renounce");
            player->printColor("You renounced %N for %s%d^x damage.\n", target.get(), player->customColorize("*CC:DAMAGE*").c_str(), dmg);
            player->checkImprove("renounce", true);
            target->printColor("%M renounced you for %s%d^x damage.\n", player.get(), target->customColorize("*CC:DAMAGE*").c_str(), dmg);
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M renounced %N!", player.get(), target.get());
            player->doDamage(target, dmg, CHECK_DIE);
        }

        return(0);
    }

}

int cmdHolyword(const std::shared_ptr<Player>& player, cmd* cmnd) {

    if(!player->knowsSkill("holyword") && !player->isCt()) {
            *player << "You haven't learned how to channel " << gConfig->getDeity(player->getDeity())->getName() << "'s divine holy word.\n";
            return(0);
        }

    return(doDivineWords(player, cmnd, "holy-word"));
}

int cmdUnholyword(const std::shared_ptr<Player>& player, cmd* cmnd) {

    if(!player->knowsSkill("unholyword") && !player->isCt()) {
            *player << "You haven't learned how to channel " << gConfig->getDeity(player->getDeity())->getName() << "'s divine unholy word.\n";
            return(0);
        }

    return(doDivineWords(player, cmnd, "unholy-word"));

}

//*********************************************************************
//                      doDivineWords
//*********************************************************************

int doDivineWords(const std::shared_ptr<Player>& player, cmd* cmnd, const std::string wordType) {
    std::shared_ptr<Creature> target=nullptr;
    long    i=0, t=0;
    int     chance=0, dmg=0;
    bool    holy=false, badPlayerAlignment=false, badTargetAlignment=false;

    if(!player->ableToDoCommand())
        return(0);

    if (wordType == "holy-word")
        holy = true;

    if(!player->isCt()) {
        //Safeguard. Must have a deity set for divine word spells to work.
        if(!player->getDeity()) {
            *player << "You have no faith! Divine words have no power if you do not have a deity.\n";
            return(0);
        }

        badPlayerAlignment = ((holy && player->getAdjustedAlignment() < BLUE) ||
                              (!holy && player->getAdjustedAlignment() > RED));
       
        if (badPlayerAlignment) {
            *player << ColorOn << "^yYou are not " << (holy?"pure":"wicked") << " enough at heart right now to channel " 
                                << gConfig->getDeity(player->getDeity())->getName() << "'s " << (holy?"holy":"unholy") << " word.\n" << ColorOff;
            return(0);
        }
    }

    target = player->getParent()->findCreature(player, cmnd->str[1], cmnd->val[1], false);

    if (!target) {
        *player << "You don't see that target here.\n";
        return(0);
    }

    double level = player->getSkillLevel(holy?"holyword":"unholyword");

    if(!player->isCt()) {

        if(!player->canAttack(target))
            return(0);

        if(player->getDeity() == target->getDeity()) {
            if (!holy) {
                *player << "While " << gConfig->getDeity(player->getDeity())->getName() << 
                        " could sometimes find it entertaining, he won't grant an unholy word on another of his faith. Perhaps torture " << target->himHer() << " instead?\n";
            }
            else
                *player << gConfig->getDeity(player->getDeity())->getName() << " will not grant a holy word on another of his faith. You should pray for " << target->hisHer() << " redemption instead.\n";
            return(0);
        }

        badTargetAlignment = ((holy && target->getAdjustedAlignment() >= NEUTRAL) || 
                              (!holy && target->getAdjustedAlignment() <= NEUTRAL));

        if(badTargetAlignment) {
            *player << ColorOn << "^y" << setf(CAP) << target << " is not " << (holy?"wicked":"pure") << " enough at heart for your " << 
                                                                                    (holy?"holy":"unholy") << " word to be effective.\n" << ColorOff;
            return(0);
        }

        if(target->isMonster()) {
            if (holy && target->getType() == DEVA) {
                *player << ColorOn << "^y" << gConfig->getDeity(player->getDeity())->getName() << " will not allow the use of a holy word on his celestial servants.\n" << ColorOff;
                return(0);
            }

            if(!holy && (target->getType() == DEMON || target->getType() == DEVIL || target->getType() == DAEMON)) {
                *player << ColorOn << "^yYou would be insane to use an unholy word on a denizen of the lower realms.\n";
                *player << gConfig->getDeity(player->getDeity())->getName() << "'s wrath would be near immediate if you did such a thing!\n" << ColorOff;
                return(0);
            }
        }

    }

    i = LT(player, holy?LT_HOLYWORD:LT_UNHOLYWORD);
    t = time(nullptr);

    if(i > t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    player->smashInvis();
    player->interruptDelayedActions();

    player->updateAttackTimer(false);
    player->lasttime[holy?LT_HOLYWORD:LT_UNHOLYWORD].ltime = t;
    if(LT(player, LT_TURN) <= t) {
        player->lasttime[LT_TURN].ltime = t;
        player->lasttime[LT_TURN].interval = 3L;
    }
    player->lasttime[LT_SPELL].ltime = t;
    player->lasttime[holy?LT_HOLYWORD:LT_UNHOLYWORD].interval = 150L;

    chance = (((int)level - target->getLevel())*50) + (player->piety.getCur() - target->piety.getCur()) + 550;
    chance = std::min(chance, 950);

    if(player->isDm() && !player->flagIsSet(P_PTESTER))
        chance = 1001;

    if(target->isEffected(holy?"malediction":"benediction") && !(player->isEffected(holy?"benediction":"malediction") && ((int)level >= target->getLevel())) ) {

        *player << ColorOn << "^yThe " << (holy?"^D":"^W") << "divine " << (holy?"darkness":"purity") << " ^yinfused into " << target << "'s soul thwarted " 
                                        << gConfig->getDeity(player->getDeity())->getName() << "'s " << (holy?"holy":"unholy") << " word.\n" << ColorOff;

        *target << ColorOn << (holy?"^W":"^D") << setf(CAP) << player << " tried to pronounce " << gConfig->getDeity(player->getDeity())->getName() << "'s " << (holy?"a holy":"an unholy") 
                                                                << " word upon you. Your " << (holy?"malediction":"benediction") << " absorbed it.\n" << ColorOff;

        broadcast(player->getSock(), target->getSock(), player->getParent(), "^y%N's %s absorbed %M's %s word.^x", 
                                    target.get(), (holy?"benediction":"malediction"), player.get(), (holy?"holy":"unholy"));

        player->lasttime[holy?LT_HOLYWORD:LT_UNHOLYWORD].interval = 10L;
        if(target->isMonster()) {
            if (player->flagIsSet(P_LAG_PROTECTION_SET))
                player->setFlag(P_LAG_PROTECTION_ACTIVE);
            target->getAsMonster()->addEnemy(player, true);
        }
        return(0);
    }

    short chanceRoll = Random::get(1,1000);
    if(player->isCt() || player->flagIsSet(P_PTESTER))
        *player << ColorOn << "^D*P-Test*\nHit chance: " << chance << "\nRoll(d1000): " << (chanceRoll<=chance?"^G":"^R") << chanceRoll << "\n" << ColorOff;

    if(chanceRoll > chance) {

        *player << ColorOn << (holy?"^W":"^D") << "Your pronunciation failed!\n" << ColorOff;

        *target << ColorOn << (holy?"^W":"^D") << setf(CAP) << player << " tried to pronounce " << gConfig->getDeity(player->getDeity())->getName() << "'s " << (holy?"holy":"unholy") 
                                        << " word upon you! " << player->upHisHer() << " articulation failed!\n" << ColorOff;

        broadcast(player->getSock(), target->getSock(), player->getParent(), "%s%M tried to pronounce %s's %s word on %N. %s articulation failed!^x", 
                                            (holy?"^W":"^D"), player.get(), gConfig->getDeity(player->getDeity())->getName().c_str(), (holy?"holy":"unholy"), target.get(), player->upHisHer());

        player->checkImprove((holy?"holyword":"unholyword"), false);
        player->lasttime[holy?LT_HOLYWORD:LT_UNHOLYWORD].interval = 10L;
        if(target->isMonster()) {
                if (player->flagIsSet(P_LAG_PROTECTION_SET))
                    player->setFlag(P_LAG_PROTECTION_ACTIVE);
                target->getAsMonster()->addEnemy(player, true);
            }
        return(0);
    }

    *target << ColorOn << (holy?"^W":"^D") << setf(CAP) << player << " pronounces " << gConfig->getDeity(player->getDeity())->getName() << "'s " << (holy?"holy":"unholy") << " word upon you!\n" << ColorOff;

    short smiteChance = 50 + player->piety.getCur()*2;
    //If under both pray and benediction/malediction effects, bonus of 3.5% to smite chance
    if(player->isEffected("pray") && player->isEffected(holy?"benediction":"malediction"))
        smiteChance += 35;

    if(player->isDm() && !player->flagIsSet(P_PTESTER))
        smiteChance = 10001;

    short smiteChanceRoll = Random::get(1,10000);
    if(player->isCt() || player->flagIsSet(P_PTESTER))
        *player << ColorOn << "^DSmite chance: " << smiteChance << "\nRoll(d10000): " << (smiteChanceRoll<=smiteChance?"^G":"^R") << smiteChanceRoll << "\n\n" << ColorOff;

    if(((int)level >= target->getLevel()) && (smiteChanceRoll <= smiteChance)) {
        *player << ColorOn << (holy?"^W":"^D") << "The divine power of " << gConfig->getDeity(player->getDeity())->getName() << "'s " << (holy?"holy":"unholy") 
                                                                                                        << " word smited " << target << " to death!\n" << ColorOff;

        *target << ColorOn << (holy?"^W":"^D") << "The divine power of " << gConfig->getDeity(player->getDeity())->getName() << " has smited you to death!\n" << ColorOff;


        player->checkImprove((holy?"holyword":"unholyword"), true);
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%s%M's %s word smited %N to death!^x", (holy?"^W":"^D"), player.get(), (holy?"holy":"unholy"), target.get());
        
        player->statistics.attackDamage(target->hp.getCur(), (holy?"holyword":"unholyword"));
        target->die(player);
    } else {

        if(target->isMonster()) {
            if (player->flagIsSet(P_LAG_PROTECTION_SET))
                player->setFlag(P_LAG_PROTECTION_ACTIVE);
            target->getAsMonster()->addEnemy(player, false);
        }
        
        dmg = (int)level + abs(player->getAlignment()) / 100;
        dmg += Random::get(((int)level*2), (((int)level*7)/2)) + player->piety.getCur()/(player->getDeity() == ARAMON?25:15); // Aramon unholy word is a little weaker because they have pets
        
        if(player->isEffected(holy?"benediction":"malediction")) {
            *player << ColorOn << (holy?"^B":"^R")<< "Your channeled power is increased by the divine " << (holy?"purity":"foulness") << " infusing your soul.\n";
            dmg += (dmg/20);
        }

        //Seraphs and Cambions get damage bonuses of +10%
        if((holy && player->getRace() == SERAPH) || (!holy && player->getRace() == CAMBION))
            dmg += (dmg/10);

        if (target->isMonster()) {
            if (holy && (target->getType() == DEMON || target->getType() == DEVIL || target->getType() == DAEMON)) {
                *player << ColorOn << "^B" << gConfig->getDeity(player->getDeity())->getName() << "'s distaste for foul denizens of the lower realms such as " 
                                                                                                                    << target << " increases the damage!\n" << ColorOff;
                dmg += (dmg/10);
            }
            else if (!holy && target->getType() == DEVA) {
                *player << ColorOn << "^R" << gConfig->getDeity(player->getDeity())->getName() << "'s disgust for sickening celestial swine such as " 
                                                                                                                  << target << " increases the damage!\n" << ColorOff;
                dmg += (dmg/10);
            }
        }

        int alignDifference = abs(target->getAlignment()) - abs(player->getAlignment());
        bool saved=false;

        // Target gets a luck save for 1/2 damage, bonus to save if target closer to opposing alignment max than player, otherwise no bonus
        // The bonus will rarely work out to be more than +3-4% due to alignment restrictions on using holy/unholy word. The way it works out,
        // is that for holyword it's a good idea to be royal blue when fighting a blood red mob, and bloodred if unholyword and royalblue mob.
        if(!player->isCt()) {
            if(target->chkSave(LCK, player, (alignDifference > 0 ? (alignDifference/200) : 0)) ) {
                *player << ColorOn << "^yYour intonation was imperfect! The " << (holy?"holy":"unholy") << " word was only partially effective!\n" << ColorOff;
                *target << ColorOn << "^y" << setf(CAP) << player << "'s intonation was imperfect! " << "The " << (holy?"holy":"unholy") << " word was only partially effective!\n" << ColorOff;
                saved=true;
                dmg /= 2;
            }
        }

        *player << ColorOn << (holy?"^W":"^D") << gConfig->getDeity(player->getDeity())->getName() << "'s " << (saved?"enervated":"") << " " << (holy?"holy":"unholy") 
                                                         << " word does " << (holy?"^Y":"^R") << dmg << (holy?"^W":"^D") << " divine damage to " << target << ".\n" << ColorOff;

        *target << ColorOn << (holy?"^W":"^D") << "The divine power of " << gConfig->getDeity(player->getDeity())->getName() << " " << (saved?"tears at":"savages") << " your soul for " 
                                                                        << (holy?"^Y":"^R") << dmg << (holy?"^W":"^D") << " damage! You scream in " << (saved?"pain":"agony") << "!\n" << ColorOff;

        broadcast(player->getSock(), target->getSock(), player->getParent(), 
                        "%s%M pronounced %s's %s word on %N. %s screams in %s!", (holy?"^W":"^D"), player.get(), gConfig->getDeity(player->getDeity())->getName().c_str(), 
                                                                                                     (holy?"holy":"unholy"), target.get(), target->upHeShe(), (saved?"pain":"agony"));
        
        player->checkImprove((holy?"holyword":"unholyword"), true);
        
        if(!saved)
            target->stun((player->piety.getCur()/50) + Random::get(2, 5));

        player->doDamage(target, dmg, CHECK_DIE);
    }

    return(0);
}


//*********************************************************************
//                      cmdBandage
//*********************************************************************

int cmdBandage(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    std::shared_ptr<Object> object=nullptr;
    int     heal=0;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(player->inCombat()) {
        player->print("Not while you are fighting.\n");
        return(0);
    }

    if(player->getClass() == CreatureClass::BUILDER) {
        if(!player->canBuildObjects()) {
            player->print("You are not allowed to use items.\n");
            return(0);
        }
        if(!player->checkBuilder(player->getUniqueRoomParent())) {
            player->print("Error: Room number not inside any of your alotted ranges.\n");
            return(0);
        }
    }

    if(!player->checkAttackTimer())
        return(0);

    if(cmnd->num < 2) {
        player->print("Syntax: bandage (bandagetype)\n");
        player->print("        bandage (target) (bandagetype)\n");
        return(0);
    }

    if(cmnd->num == 2) {

        object = player->findObject(player, cmnd, 1);
        if(!object) {
            player->print("Use what bandages?\n");
            return(0);
        }

        if(player->getClass() == CreatureClass::LICH) {
            player->print("Liches don't bleed. Stop wasting your time and regenerate!\n");
            object->decShotsCur();
            return(0);
        }

        if(object->getType() != ObjectType::BANDAGE) {
            player->print("You cannot bandage yourself with that.\n");
            return(0);
        }

        if(object->getShotsCur() < 1) {
            player->print("You can't bandage yourself with that. It's used up.\n");
            return(0);
        }

        if(player->hp.getCur() >= player->hp.getMax()) {
            player->print("You are at full health! You don't need to bandage yourself!\n");
            return(0);
        }


        if(player->isEffected("stoneskin")) {
            player->print("Your stoneskin spell prevents bandaging efforts.\n");
            return(0);
        }

        object->decShotsCur();

        heal = object->damage.roll();

        heal = player->hp.increase(heal);
        player->print("You regain %d hit points.\n", heal);


        if(object->getShotsCur() < 1)
            player->printColor("Your %s %s all used up.\n", object->getCName(), (object->flagIsSet(O_SOME_PREFIX) ? "are":"is"));

        broadcast(player->getSock(), player->getParent(), "%M bandages %sself.", player.get(), player->himHer());

        player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);

    } else {

        if(!cmnd->str[2][0]) {
            player->print("Which bandages will you use?\n");
            player->print("Syntax: bandage (target)[#] (bandages)[#]\n");
            return(0);
        }


        cmnd->str[2][0] = up(cmnd->str[2][0]);
        creature = player->getParent()->findCreature(player, cmnd->str[1], cmnd->val[1], false);
        if(!creature) {
            player->print("That person is not here.\n");
            return(0);
        }



        object = player->findObject(player, cmnd, 2);
        if(!object) {
            player->print("You don't have that in your inventory.\n");
            return(0);
        }

        if(object->getType() != ObjectType::BANDAGE) {
            player->print("You cannot bandage %N with that.\n", creature.get());
            return(0);
        }

        if(object->getShotsCur() < 1) {
            player->print("You can't bandage %N with that. It's used up.\n", creature.get());
            return(0);
        }


        if(creature->hp.getCur() == creature->hp.getMax()) {
            player->print("%M doesn't need to be bandaged right now.\n", creature.get());
            return(0);
        }

        if(creature->getClass() == CreatureClass::LICH) {
            player->print("The aura of death around %N causes bandaging to fail.\n",creature.get());
            object->decShotsCur();
            return(0);
        }

        if(creature->isPlayer() && creature->inCombat()) {
            player->print("You cannot bandage %N while %s's fighting a monster.\n", creature.get(), creature->heShe());
            return(0);
        }

        if(creature->getType() !=PLAYER && !creature->isPet() && creature->getAsMonster()->nearEnemy() && !creature->getAsMonster()->isEnemy(player)) {
            player->print("Not while %N is in combat with someone.\n", creature.get());
            return(0);
        }


        if(creature->isPlayer() && creature->isEffected("stoneskin")) {
            player->print("%M's stoneskin spell resists your bandaging efforts.\n", creature.get());
            return(0);
        }

        if(creature->pFlagIsSet(P_LINKDEAD)) {
            player->print("%M doesn't want to be bandaged right now.\n", creature.get());
            return(0);
        }


        object->decShotsCur();

        heal = object->damage.roll();

        heal = creature->hp.increase(heal);

        player->print("%M regains %d hit points.\n", creature.get(), heal);


        if(object->getShotsCur() < 1)
            player->printColor("Your %s %s all used up.\n", object->getCName(),
                  (object->flagIsSet(O_SOME_PREFIX) ? "are":"is"));

        broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M bandages %N.", player.get(), creature.get());
        player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
    }

    return(0);
}

//*********************************************************************
//                      splHallow
//*********************************************************************

int splHallow(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength = 10;
    long duration = 600;

    if(player->noPotion( spellData))
        return(0);

    if(spellData->how == CastType::CAST) {
        if(player->getClass() !=  CreatureClass::CLERIC && !player->isCt()) {
            player->print("Only clerics may cast that spell.\n");
            return(0);
        }
        player->print("You cast a hallow spell.\n");
        broadcast(player->getSock(), player->getParent(), "%M casts a hallow spell.", player.get());
    }

    if(player->getRoomParent()->hasPermEffect("hallow")) {
        player->print("The spell didn't take hold.\n");
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus())
            player->print("The room's magical properties increase the power of your spell.\n");
    }

    player->getRoomParent()->addEffect("hallow", duration, strength, player, true, player);
    return(1);
}

//*********************************************************************
//                      splUnhallow
//*********************************************************************

int splUnhallow(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength = 10;
    long duration = 600;

    if(player->noPotion( spellData))
        return(0);

    if(spellData->how == CastType::CAST) {
        if(player->getClass() !=  CreatureClass::CLERIC && !player->isCt()) {
            player->print("Only clerics may cast that spell.\n");
            return(0);
        }
    }
    player->print("You cast an unhallow spell.\n");
    broadcast(player->getSock(), player->getParent(), "%M casts an unhallow spell.", player.get());

    if(player->getRoomParent()->hasPermEffect("unhallow")) {
        player->print("The spell didn't take hold.\n");
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus())
            player->print("The room's magical properties increase the power of your spell.\n");
    }

    player->getRoomParent()->addEffect("unhallow", duration, strength, player, true, player);
    return(1);
}

int splBenediction(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    if( spellData->how == CastType::CAST &&
        player->getClass() !=  CreatureClass::CLERIC &&
        player->getClass() !=  CreatureClass::PALADIN &&
        player->getClass() !=  CreatureClass::DRUID &&
        !player->isCt()) 
    {
        *player << "Only druids, paladins, and clerics of Ares, Ceris, Enoch, Gradius, Kamira, Linothan, or Mara may cast that spell.\n"; 
        return(0);
    }

    if (!player->isCt() &&
        player->getClass() != CreatureClass::DRUID &&
        player->getDeity() != ARES && 
        player->getDeity() != ENOCH &&
        player->getDeity() != LINOTHAN &&
        player->getDeity() != CERIS &&
        player->getDeity() != MARA &&
        player->getDeity() != GRADIUS &&
        player->getDeity() != KAMIRA)
    {
        *player << gConfig->getDeity(player->getDeity())->getName().c_str() << 
            ((player->getDeityAlignment() == BLOODRED) ? " is furious that you tried to cast that spell.\nYou should prostrate yourself immediately and beg for mercy.":" does not grant that spell to you.") << "\n";
        return(0);
    }

    if (!player->isCt() && spellData->how == CastType::CAST && (player->getAdjustedAlignment() < NEUTRAL)) {
        *player << "Your alignment must be neutral or good in order to cast a benediction spell.\n";
        return(0);
    }

    return(splGeneric(player, cmnd, spellData, "a", "benediction", "benediction"));
}

int splMalediction(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    if( spellData->how == CastType::CAST &&
        player->getClass() !=  CreatureClass::CLERIC &&
        player->getClass() !=  CreatureClass::DEATHKNIGHT &&
        player->getClass() !=  CreatureClass::DRUID &&
        !player->isCt()) 
    {
        *player << "Only death knights, druids, and clerics of Arachnus, Aramon, or Ares may cast that spell.\n";
        return(0);
    }

    if (spellData->how == CastType::CAST &&
        !player->isCt() &&
        player->getClass() != CreatureClass::DRUID && 
        player->getDeity() != ARACHNUS &&
        player->getDeity() != ARES &&
        player->getDeity() != ARAMON)
    {
        *player << gConfig->getDeity(player->getDeity())->getName().c_str() << 
            ((player->getDeityAlignment() == ROYALBLUE) ? " is disappointed that you tried to cast that spell.":" does not grant that spell to you.") << "\n";
        return(0);
    }

    if (!player->isCt() && spellData->how == CastType::CAST && (player->getAdjustedAlignment() > NEUTRAL)) {
        *player << "Your alignment must be neutral or evil in order to cast a malediction spell.\n";
        return(0);
    }

    return(splGeneric(player, cmnd, spellData, "a", "malediction", "malediction"));
}

