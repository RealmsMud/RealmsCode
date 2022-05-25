/*
 * afflictions.cpp
 *   Functions that handle poison/disease/curse/blind/vampirism/lycanthropy
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
#include <list>                      // for operator==, list<>::iterator, list
#include <set>                       // for operator==, _Rb_tree_const_iterator
#include <string>                    // for string, allocator, operator==
#include <string_view>               // for string_view

#include "cmd.hpp"                   // for cmd
#include "commands.hpp"              // for cmdCreepingDoom, cmdPoison
#include "config.hpp"                // for Config, gConfig
#include "deityData.hpp"             // for DeityData
#include "effects.hpp"               // for Effects, EffectInfo, EffectList
#include "flags.hpp"                 // for O_CURSED, P_DM_BLINDED, P_POISON...
#include "global.hpp"                // for CastType, CreatureClass, CastTyp...
#include "lasttime.hpp"              // for lasttime
#include "magic.hpp"                 // for SpellData, checkRefusingMagic
#include "monType.hpp"               // for noLivingVulnerabilities, ARACHNID
#include "mud.hpp"                   // for LT_DRAIN_LIFE, LT_SMOTHER, LT
#include "mudObjects/container.hpp"  // for Container, ObjectSet
#include "mudObjects/creatures.hpp"  // for Creature, SkillMap, CHECK_DIE
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/objects.hpp"    // for Object
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, up, bonus, broadcastG...
#include "raceData.hpp"              // for RaceData
#include "random.hpp"                // for Random
#include "server.hpp"                // for Server, gServer
#include "skills.hpp"                // for Skill
#include "statistics.hpp"            // for Statistics
#include "stats.hpp"                 // for Stat
#include "xml.hpp"                   // for loadPlayer


//*********************************************************************
//                      cmdCreepingDoom
//*********************************************************************

int cmdCreepingDoom(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    std::shared_ptr<Monster> mCreature=nullptr;
    long    i=0, t=0;
    int     chance=0, dmg=0;

    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
        return(0);

    if(!player->isCt()) {
        if(!player->knowsSkill("creeping-doom")) {
            player->print("You lack the ability to strike someone with creeping doom.\n");
            return(0);
        }
        if(!player->alignInOrder()) {
            player->print("%s requires you to be evil to do that.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
            return(0);
        }
    }

    if(cmnd->num < 2) {
        player->print("Strike whom with creeping doom?\n");
        return(0);
    }

    creature = player->getParent()->findCreature(player, cmnd->str[1], cmnd->val[1], true, true);
    if(!creature || creature == player) {
        player->print("You don't see that here.\n");
        return(0);
    }
//  pCreature = creature->getPlayer();
    mCreature = creature->getAsMonster();

    player->smashInvis();
    player->interruptDelayedActions();

    if(!player->canAttack(creature))
        return(0);

    if(mCreature && mCreature->getType() == ARACHNID &&
        !player->checkStaff("You cannot harm arachnids with creeping-doom.\n")
    )
        return(0);
    if( creature->getDeity() == ARACHNUS &&
        !player->checkStaff("You cannot harm followers of Arachnus with creeping-doom.\n")
    )
        return(0);


    double level = player->getSkillLevel("creeping-doom");
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

    dmg = Random::get((int)(level*2), (int)(level*3));

    if(mCreature)
        mCreature->addEnemy(player);

    if(!creature->isCt()) {
        if(Random::get(1, 100) > chance) {
            player->print("You failed to strike %N with creeping doom.\n", creature.get());
            broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M tried to strike %N with creeping doom!", player.get(), creature.get());
            creature->print("%M tried to strike you with creeping doom!\n", player.get());
            player->checkImprove("creeping-doom", false);
            return(0);
        }
        if(creature->chkSave(BRE, player, 0)) {
            player->printColor("^yYour creeping-doom strike was interrupted!\n");
            creature->print("You manage to partially avoid %N's creeping-doom strike.\n", player.get());
            dmg /= 2;
        }
    }

    player->printColor("You struck %N with creeping doom for %s%d^x damage.\n", creature.get(), player->customColorize("*CC:DAMAGE*").c_str(), dmg);
    player->checkImprove("creeping-doom", true);
    player->statistics.attackDamage(dmg, "creeping-doom");

    creature->printColor("%M struck you with creeping doom for %s%d^x damage!\n", player.get(), creature->customColorize("*CC:DAMAGE*").c_str(), dmg);
    broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M struck %N with creeping doom!", player.get(), creature.get());

    // if they didn't die from the damage, curse them
    if(!player->doDamage(creature, dmg, CHECK_DIE))
        creature->addEffect("creeping-doom", 200, (int)level, player, true, player);
    return(0);
}


//********************************************************************
//                      cmdPoison
//********************************************************************
// the Arachnus ability to inflict enemies with poison

int cmdPoison(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    std::shared_ptr<Monster> mCreature=nullptr;
    long    i=0, t=0;
    int     chance=0;
    unsigned int dur=0;

    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
        return(0);

    if(!player->isCt()) {
        if(!player->knowsSkill("poison")) {
            if(player->getClass() == CreatureClass::ASSASSIN || player->getSecondClass() == CreatureClass::ASSASSIN) {
                player->print("Assassins must use envenom to poison people.\n");
            } else {
                player->print("You lack the ability to poison people.\n");
            }
            return(0);
        }
        if(!player->alignInOrder()) {
            player->print("%s requires you to be evil to do that.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
            return(0);
        }
    }

    if(cmnd->num < 2) {
        player->print("Poison whom?\n");
        return(0);
    }

    creature = player->getParent()->findCreature(player, cmnd->str[1], cmnd->val[1], true, true);
    if(!creature || creature == player) {
        player->print("You don't see that here.\n");
        return(0);
    }
//  pCreature = creature->getPlayer();
    mCreature = creature->getAsMonster();

    player->smashInvis();
    player->interruptDelayedActions();

    if(!player->canAttack(creature))
        return(0);

    if(mCreature && mCreature->getType() == ARACHNID &&
        !player->checkStaff("You cannot poison arachnids with this ability.\n")
    )
        return(0);
    if( creature->getDeity() == ARACHNUS &&
        !player->checkStaff("You cannot poison followers of Arachnus with this ability.\n")
    )
        return(0);


    double level = player->getSkillLevel("poison");
    i = LT(player, LT_DRAIN_LIFE);
    t = time(nullptr);
    if(i > t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    player->lasttime[LT_DRAIN_LIFE].ltime = t;
    player->updateAttackTimer();
    player->lasttime[LT_DRAIN_LIFE].interval = 120L;


    chance = ((int)(level - creature->getLevel()) * 20) + bonus(player->piety.getCur()) * 5 + 25;
    chance = std::min(chance, 80);
    dur = standardPoisonDuration((short)level, creature->constitution.getCur());

    if(mCreature)
        mCreature->addEnemy(player);

    if(!player->isCt()) {
        if(Random::get(1, 100) > chance || creature->isPoisoned() || creature->immuneToPoison()) {
            player->printColor("^GYou fail to poison %N.\n", creature.get());
            creature->printColor("^G%M tried to poison you!\n", player.get());
            broadcastGroup(false, creature, "%M tried to poison %N.\n", player.get(), creature.get());
            broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M tried to poison %N.", player.get(), creature.get());
            player->checkImprove("poison", false);
            return(0);
        }
    }

    player->printColor("^GYou poison %N.\n", creature.get());
    creature->printColor("^G%M poisons you!\n", player.get());
    broadcastGroup(false, creature, "^G%M poisons %N.\n", player.get(), creature.get());
    broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "^G%M poisons %N.", player.get(), creature.get());

    if(creature->chkSave(POI, player, 0)) {
        player->print("%N partially resists your poison.\n", creature.get());
        creature->print("You partially resists %N's poison.\n", player.get());
        level = level * 2 / 3;
        dur = dur * 2 / 3;
    }

    player->checkImprove("poison", true);
    creature->poison(player, (unsigned int)level, dur);
    return(0);
}

//********************************************************************
//                      poison
//********************************************************************
// the calling function is responsible for announcing the poisoning

void Creature::poison(const std::shared_ptr<Creature>&enemy, unsigned int damagePerPulse, unsigned int duration) {
    if(immuneToPoison())
        return;
    setPoisonedBy("");

    if(enemy) {
        if(isPlayer()) {

            if(enemy->isPlayer() || enemy->isPet()) {
                setFlag(P_POISONED_BY_PLAYER);
                clearFlag(P_POISONED_BY_MONSTER);
            } else {
                setFlag(P_POISONED_BY_MONSTER);
                clearFlag(P_POISONED_BY_PLAYER);
            }

            if(enemy->isPet())
                setPoisonedBy(enemy->getMaster()->getName());
            else if(enemy->isMonster() && !enemy->flagIsSet(M_NO_PREFIX))
                setPoisonedBy((std::string)"a " + enemy->getName());
            else if(this == enemy.get())
                setPoisonedBy((std::string)himHer() + "self");
            else
                setPoisonedBy(enemy->getName());

        } else {

            if(enemy->isPlayer())
                setPoisonedBy(enemy->getName());
            else if(enemy->isPet())
                setPoisonedBy(enemy->getMaster()->getName());
            else if(enemy->isMonster() && !enemy->flagIsSet(M_NO_PREFIX))
                setPoisonedBy((std::string)"a " + enemy->getName());
            else
                setPoisonedBy(enemy->getName());

        }
    }

    addEffect("poison", duration, damagePerPulse, nullptr, false, Containable::downcasted_shared_from_this<Creature>());
}

//********************************************************************
//                      immuneToPoison
//********************************************************************

bool Creature::immuneToPoison() const {
    if(isUndead() || monType::noLivingVulnerabilities(type))
        return(true);

    if(deity == ARACHNUS)
        return(true);

    // check on mobs
    if(isMonster()) {
        if(flagIsSet(M_NO_POISON))
            return(true);
    // check on players
    } else {
        if(isStaff())
            return(true);
    }
    return(false);
}

//*********************************************************************
//                      isPoisoned
//*********************************************************************

bool Creature::isPoisoned() const {
    if(immuneToPoison())
        return(false);
    return(effects.hasPoison());
}

//*********************************************************************
//                      hasPoison
//*********************************************************************

bool Effects::hasPoison() const {
    EffectList::const_iterator eIt;
    for(eIt = effectList.begin() ; eIt != effectList.end() ; eIt++) {
        if((*eIt)->isPoison())
            return(true);
    }
    return(false);
}

//*********************************************************************
//                      curePoison
//*********************************************************************

bool Creature::curePoison() {
    pClearFlag(P_POISONED_BY_PLAYER);
    pClearFlag(P_POISONED_BY_MONSTER);

    removeEffect("slow-poison");
    setPoisonedBy("");
    return(effects.removePoison());
}

//*********************************************************************
//                      removePoison
//*********************************************************************

bool Effects::removePoison() {
    bool removed=false;
    // remove all effects flagged as poison
    EffectList::iterator eIt;
    EffectInfo *effect=nullptr;
    for(eIt = effectList.begin() ; eIt != effectList.end() ;) {
        effect = (*eIt);

        if(effect->isPoison()) {
            removed = true;
            effect->remove();
            delete effect;
            eIt = effectList.erase(eIt);
        } else
            eIt++;
    }
    return(removed);
}

//********************************************************************
//                      standardPoisonDuration
//********************************************************************

unsigned int standardPoisonDuration(short level, short con) {
    int dur = 60 * Random::get(1,3) - (60*bonus((int)con)) + level*10;
    if(con > 120) {
        // a spread between 400 (50%) and 120 (0%) resistance
        double percent = 1 - (con - 120.0) / (680.0 - 120.0);
        percent *= dur;
        dur = (int)percent;
    }
    return(std::max(60,dur));
}

// low con, level 40:  1-3 mins
// high con, level 40: 1 min
//

//********************************************************************
//                      disease
//********************************************************************
// the calling function is responsible for announcing the diseasing

void Creature::disease(const std::shared_ptr<Creature>& enemy, unsigned int damagePerPulse) {
    if(immuneToDisease())
        return;
    addPermEffect("disease", damagePerPulse, false);
}

//********************************************************************
//                      immuneToDisease
//********************************************************************

bool Creature::immuneToDisease() const {
    // no need to call noLivingVulnerabilities
    if(isMonster())
        return(true);

    if(isStaff())
        return(true);
    if(isUndead())
        return(true);

    return(false);
}

//*********************************************************************
//                      isDiseased
//*********************************************************************

bool Creature::isDiseased() const {
    if(immuneToDisease())
        return(false);
    return(effects.hasDisease());
}

//*********************************************************************
//                      hasDisease
//*********************************************************************

bool Effects::hasDisease() const {
    EffectList::const_iterator eIt;
    for(eIt = effectList.begin() ; eIt != effectList.end() ; eIt++) {
        if((*eIt)->isDisease())
            return(true);
    }
    return(false);
}

//*********************************************************************
//                      cureDisease
//*********************************************************************

bool Creature::cureDisease() {
    return(effects.removeDisease());
}

//*********************************************************************
//                      removeDisease
//*********************************************************************

bool Effects::removeDisease() {
    bool removed=false;
    // remove all effects flagged as disease
    EffectList::iterator eIt;
    EffectInfo *effect=nullptr;
    for(eIt = effectList.begin() ; eIt != effectList.end() ;) {
        effect = (*eIt);

        if(effect->isDisease()) {
            removed = true;
            effect->remove();
            delete effect;
            eIt = effectList.erase(eIt);
        } else
            eIt++;
    }
    return(removed);
}

//*********************************************************************
//                      removeCurse
//*********************************************************************
// remove curse effects

bool Creature::removeCurse() {
    return(effects.removeCurse());
}

bool Effects::removeCurse() {
    bool removed=false;
    EffectList::iterator eIt;
    EffectInfo *effect=nullptr;
    for(eIt = effectList.begin() ; eIt != effectList.end() ;) {
        effect = (*eIt);

        if(effect->isCurse()) {
            removed = true;
            effect->remove();
            delete effect;
            eIt = effectList.erase(eIt);
        } else
            eIt++;
    }
    return(removed);
}

//********************************************************************
//                      isBlind
//********************************************************************

bool Creature::isBlind() const {
    if(isMonster() || isStaff())
        return(false);
    return(isEffected("blindness"));
}

//***********************************************************************
//                      isNew
//***********************************************************************
// these checks distinguish legacy vampires and werewolves from new ones.
// once these legacy classes have been removed, these functions can just
// be replaced with isEffected("vampirism") and isEffected("lycanthropy")

bool Creature::isNewVampire() const {
    if(isPlayer() && cClass == CreatureClass::PUREBLOOD)
        return(false);
    return(isEffected("vampirism"));
}

bool Creature::isNewWerewolf() const {
    if(isPlayer() && cClass == CreatureClass::WEREWOLF)
        return(false);
    return(isEffected("lycanthropy"));
}

//***********************************************************************
//                      makeVampire
//***********************************************************************

void Creature::makeVampire() {
    addPermEffect("vampirism");
    if(isPlayer()) {
        setFlag(P_CHAOTIC);

        // make sure the master still exists and is still a vampire
        std::shared_ptr<Player> player = getAsPlayer();
        if(!player->getAfflictedBy().empty()) {
            bool online = true;
            std::shared_ptr<Player> master = gServer->findPlayer(player->getAfflictedBy());

            if(!master) {
                loadPlayer(player->getAfflictedBy().c_str(), master);
                online = false;
            }

            if(!master || !master->isEffected("vampirism"))
                player->setAfflictedBy("");
            else {
                master->minions.push_back(getName());
                master->save(online);
            }

            if(master && !online)
                master.reset();
        }
    }
    if(!knowsSkill("mist"))
        addSkill("mist", 1);
    if(!knowsSkill("bite"))
        addSkill("bite", 1);
    if(!knowsSkill("hypnotize"))
        addSkill("hypnotize", 1);
}

//***********************************************************************
//                      willBecomeVampire
//***********************************************************************

bool Creature::willBecomeVampire() const {
    if(!pFlagIsSet(P_PTESTER))
        return(false);

    if(level < 7 || cClass == CreatureClass::PALADIN || isEffected("lycanthropy") || isUndead())
        return(false);
    // only evil clerics become vampires
    if(cClass == CreatureClass::CLERIC && !(deity == ARAMON || deity == ARACHNUS))
        return(false);
    if(monType::noLivingVulnerabilities(type))
        return(false);
    return(true);
}

//***********************************************************************
//                      vampireCharm
//***********************************************************************
// determines if player is a spawn of the master
// if not, it does some cleanup that should have been done elsewhere
// (an extra check to make sure everything is set properly)

bool Creature::vampireCharmed(const std::shared_ptr<Player>& master) {
    if(!isPlayer() || !master)
        return(false);

    // if they're on their way to becoming a vampire, but arent one yet, don't
    // mess with minion or afflictedBy fields
    if(isEffected("porphyria"))
        return(false);

    std::shared_ptr<Player> player = getAsPlayer();
    bool charmed = true;

    // we need to make sure the minion lists match up;
    // this won't be the case if somebody suicides and remakes

    if(charmed && (!isEffected("vampirism") || !master->isEffected("vampirism")))
        charmed = false;
    if(charmed && player->getAfflictedBy() != master->getName())
        charmed = false;

    if(charmed) {
        bool found = false;
        std::list<std::string>::iterator mIt;
        for(mIt = master->minions.begin() ; mIt != master->minions.end() && !found ; mIt++) {
            if(*mIt == player->getName())
                found = true;
        }
        if(!found)
            charmed = false;
    }

    // if they're not charmed, clean up these lists
    if(!charmed) {
        if(player->getAfflictedBy() == master->getName())
            player->setAfflictedBy("");
        master->minions.remove(player->getName());
    }
    return(charmed);
}


//********************************************************************
//                      clearMinions
//********************************************************************

void Creature::clearMinions() {
    if(!isPlayer())
        return;
    std::shared_ptr<Player> player = getAsPlayer(), target=nullptr;
    bool online = true;

    if(!player->getAfflictedBy().empty()) {
        target = gServer->findPlayer(player->getAfflictedBy());

        if(!target) {
            loadPlayer(player->getAfflictedBy().c_str(), target);
            online = false;
        }

        if(target) {
            target->minions.remove(player->getName());
            target->save(online);
        }
        if(!online)
            target.reset();
    }

    std::list<std::string>::iterator mIt;
    for(mIt = player->minions.begin() ; mIt != player->minions.end() ; mIt++) {
        online = true;
        target = gServer->findPlayer(*mIt);

        if(!target) {
            loadPlayer((*mIt).c_str(), target);
            online = false;
        }

        if(target) {
            if(target->getAfflictedBy() == player->getName()) {
                target->setAfflictedBy("");
                target->save(online);
            }
            if(!online)
                target.reset();
        }
    }
}

//********************************************************************
//                      addPorphyria
//********************************************************************

bool Creature::addPorphyria(const std::shared_ptr<Creature>&killer, int chance) {

    const RaceData* rdata = gConfig->getRace(race);
    if(rdata)
        chance -= rdata->getPorphyriaResistance();

    if(Random::get(1,100) > chance)
        return(false);
    if(!willBecomeVampire())
        return(false);
    if(isEffected("porphyria") || isEffected("undead-ward") || isEffected("drain-shield"))
        return(false);
    if(!killer || !killer->isNewVampire())
        return(false);

    addEffect("porphyria", 0, 0, killer, true, Containable::downcasted_shared_from_this<Creature>());

    // if they become a vampire, they'll be permanently charmed by their master
    if(killer->isPlayer()) {
        killer->print("You have infected %N with porphyria.\n", this);
        if(isPlayer())
            getAsPlayer()->setAfflictedBy(killer->getName());
    } else if(isPlayer())
        getAsPlayer()->setAfflictedBy("");

    return(true);

}

//********************************************************************
//                      sunlightDamage
//********************************************************************
// return true if they died, false if they lived

bool Creature::sunlightDamage() {
    if(isStaff() || !isNewVampire())
        return(false);

    int dmg = hp.getMax() / 5 + Random::get(1,10);

    unmist();
    wake("Terrible nightmares disturb your sleep!");
    printColor("^Y^#The searing sunlight burns your flesh!\n");
    broadcast(getSock(), getRoomParent(), "^Y%M's flesh is burned by the sunlight!", this);

    // lots of damage every 5 seconds
    hp.decrease(dmg);

    if(hp.getCur() < 1) {
        printColor("^YYou have been disintegrated!\n");
        if(isPlayer())
            getAsPlayer()->die(SUNLIGHT);
        else
            getAsMonster()->mobDeath();
        return(true);
    }
    return(false);
}

//***********************************************************************
//                      makeWerewolf
//***********************************************************************

void Creature::makeWerewolf() {
    addPermEffect("lycanthropy");
    if(!knowsSkill("maul"))
        skills["maul"] = new Skill("maul", 1);
    if(!knowsSkill("frenzy"))
        skills["frenzy"] = new Skill("frenzy", 1);
    if(!knowsSkill("howl"))
        skills["howl"] = new Skill("howl", 1);
    if(!knowsSkill("claw"))
        skills["claw"] = new Skill("claw", level * 5);
}

//***********************************************************************
//                      willBecomeWerewolf
//***********************************************************************

bool Creature::willBecomeWerewolf() const {
    if(!pFlagIsSet(P_PTESTER))
        return(false);
    if(level < 7 || isEffected("lycanthropy") || isUndead())
        return(false);
    if(monType::noLivingVulnerabilities(type))
        return(false);
    return(true);
}

//********************************************************************
//                      addLycanthropy
//********************************************************************

bool Creature::addLycanthropy(const std::shared_ptr<Creature>&killer, int chance) {
    if(Random::get(1,100) > chance)
        return(false);
    if(!willBecomeWerewolf())
        return(false);
    if(isEffected("lycanthropy"))
        return(false);
    if(!killer || !killer->isNewWerewolf())
        return(false);

    addEffect("lycanthropy", 0, 0, killer, true, Containable::downcasted_shared_from_this<Creature>());

    if(killer->isPlayer()) {
        killer->print("You have infected %N with lycanthropy.\n", this);
        if(isPlayer())
            getAsPlayer()->setAfflictedBy(killer->getName());
    } else if(isPlayer())
        getAsPlayer()->setAfflictedBy("");

    return(true);
}


//*********************************************************************
//                      splCurePoison
//*********************************************************************
// This function allows a player to cast a curepoison spell on themself,
// another player or a monster. It will remove any poison that is in
// that player's system.

int splCurePoison(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;

    if( player->isPlayer() &&
        player->getClass() !=  CreatureClass::CLERIC &&
        player->getClass() !=  CreatureClass::PALADIN &&
        !player->isCt() &&
        spellData->how == CastType::CAST
    ) {
        player->print("Your class may not cast that spell.\n");
        return(0);
    }

    // Curepoison self
    if(cmnd->num == 2) {
        target = player;
        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Cure-poison spell cast on yourself.\n");
            player->print("You feel much better.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts cure-poison on %sself.",
                player.get(), player->himHer());
        } else if(spellData->how == CastType::POTION && player->isPoisoned())
            player->print("You feel the poison subside.\n");
        else if(spellData->how == CastType::POTION)
            player->print("Nothing happens.\n");

    // Cure a monster or player
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print("That person is not here.\n");
            return(0);
        }

        if(checkRefusingMagic(player, target))
            return(0);

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Cure-poison cast on %N.\n", target.get());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts cure-poison on %N.", player.get(), target.get());
            target->print("%M casts cure-poison on you.\nYou feel much better.\n", player.get());
        }

    }
    if(target->inCombat(false))
        player->smashInvis();

    target->curePoison();
    return(1);
}

//*********************************************************************
//                      splSlowPoison
//*********************************************************************

int splSlowPoison(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;

    // slow_poison self
    if(cmnd->num == 2) {
        target = player;
        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Slow-poison spell cast on yourself.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts slow-poison on %sself.",
                player.get(), player->himHer());
            if(!player->isPoisoned()) {
                player->print("Nothing happens.\n");
                return(0);
            }
            player->print("You feel the poison subside somewhat.\n");
        } else if(spellData->how == CastType::POTION && player->isPoisoned())
            player->print("You feel the poison subside somewhat.\n");
        else if(spellData->how == CastType::POTION)
            player->print("Nothing happens.\n");

    // Cure a monster or player
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        // monsters are now valid targets
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print("That person is not here.\n");
            return(0);
        }

        if(checkRefusingMagic(player, target))
            return(0);

        player->print("Slow-poison cast on %N.\n", target.get());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts slow-poison on %N.", player.get(), target.get());
        target->print("%M casts slow-poison on you.\n", player.get());

        if(!target->isPoisoned()) {
            player->print("Nothing happens.\n");
            if(player != target)
                target->print("Nothing happens.\n");
            return(0);
        }
        target->print("You feel a little better.\n");
    }

    if(target->inCombat(false))
        player->smashInvis();

    target->addPermEffect("slow-poison");
    return(1);
}


//*********************************************************************
//                      splCureDisease
//*********************************************************************

int splCureDisease(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;

    if(player->getClass() !=  CreatureClass::CLERIC &&
        player->getClass() !=  CreatureClass::PALADIN &&
        !player->isStaff() &&
        spellData->how == CastType::CAST)
    {
        player->print("Only clerics and paladins may cast that spell.\n");
        return(0);
    }

    if(cmnd->num == 2) {
        target = player;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Cure-disease spell cast on yourself.\n");
            player->print("Your fever subsides.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts cure-disease on %sself.", player.get(), player->himHer());
        } else if(spellData->how == CastType::POTION && player->isDiseased())
            player->print("You feel your fever subside.\n");
        else if(spellData->how == CastType::POTION)
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

        if(checkRefusingMagic(player, target))
            return(0);

        player->print("You cure-disease cast on %N.\n", target.get());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts cure-disease on %N.", player.get(), target.get());
        target->print("%M casts cure-disease on you.\nYou feel your fever subside.\n", player.get());

    }
    if(target->inCombat(false))
        player->smashInvis();

    target->cureDisease();
    return(1);
}

//*********************************************************************
//                      splCureBlindness
//*********************************************************************

int splCureBlindness(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;

    if(cmnd->num == 2) {

        target = player;
        if(!player->isStaff()) {
            if( spellData->how != CastType::POTION ||
                !player->isEffected("blindness") ||
                player->flagIsSet(P_DM_BLINDED)
            )
                player->print("Nothing happens.\n");
        }

    } else {
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
            player->print("You cast cure blindess on %N.\n", target.get());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts cure blindness on %N.",
                player.get(), target.get());
            if(target->isPlayer()) {
                target->print("%M casts cure blindness on you.\n", player.get());

                if( target->isEffected("blindness") &&
                    (player->isCt() || !target->flagIsSet(P_DM_BLINDED))
                ) {
                    //target->print("You can see again.\n");
                } else {
                    target->print("Nothing happens.\n");
                }
            }
        }


    }

    if(target->inCombat(false))
        player->smashInvis();

    if(target->isPlayer())
        if(player->isCt())
            target->clearFlag(P_DM_BLINDED);

    target->removeEffect("blindness");
    return(1);
}

//*********************************************************************
//                      isCurse
//*********************************************************************

bool EffectInfo::isCurse() const {
    return(myEffect->hasBaseEffect("curse"));
}

//*********************************************************************
//                      isDisease
//*********************************************************************

bool EffectInfo::isDisease() const {
    // lycanthropy is only a disease if it's not permanent - once permanent, it's
    // considered an afflication and can't be removed by cure-disease
    if(myEffect->getName() == "lycanthropy" && duration == -1)
        return(false);
    if(myEffect->hasBaseEffect("disease"))
        return(true);
    return(false);
}

//*********************************************************************
//                      isPoison
//*********************************************************************

bool EffectInfo::isPoison() const {
    return(myEffect->hasBaseEffect("poison"));
}

//*********************************************************************
//                      splCurse
//*********************************************************************
// This function allows a player to curse a item in their inventory

int splCurse(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Object> object=nullptr;

    if(spellData->how == CastType::CAST && player->getClass() !=  CreatureClass::MAGE && player->getClass() !=  CreatureClass::LICH && !player->isStaff()) {
        player->print("Only mages and liches can curse.\n");
        return(0);
    }
    if(cmnd->num < 3) {
        player->print("Curse what?\n");
        return(0);
    }

    object = player->findObject(player, cmnd, 2);

    if(!object) {
        player->print("You don't have that in your inventory.\n");
        return(0);
    }
    if(object->flagIsSet(O_CURSED)) {
        player->print("That object is already cursed.\n");
        return(1);
    }

    object->setFlag(O_CURSED);

    player->printColor("%O glows darkly.\n", object.get());
    broadcast(player->getSock(), player->getParent(), "%M places a curse on %1P.", player.get(), object.get());

    return(1);
}

//*********************************************************************
//                      splRemoveCurse
//*********************************************************************
// This function allows a player to remove a curse on all the items
// in their inventory or on another player's inventory

int splRemoveCurse(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;
    int     i=0;
    bool    equipment=true;

    // Cast remove-curse on self
    if(cmnd->num == 2) {

        target = player;
        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Remove-curse spell cast.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts remove-curse on %sself.", player.get(), player->himHer());
        } else if(spellData->how == CastType::POTION)
            player->print("You feel relieved of burdens.\n");

    // Cast remove-curse on another player
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findPlayer(player, cmnd, 2);

        if(!target) {
            player->print("That player is not here.\n");
            return(0);
        }

        // this purposely does not check the refuse list
        if(target->flagIsSet(P_LINKDEAD) && target->isPlayer()) {
            player->print("%M doesn't want that cast on them right now.\n", target.get());
            return(0);
        }

        player->print("Remove-curse cast on %N.\n", target.get());
        target->print("%M casts a remove-curse spell on you.\n", player.get());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts remove-curse on %N.", player.get(), target.get());
    }

    if(target->inCombat(false))
        player->smashInvis();

    // remove all effects flagged as cursed
    if(target->removeCurse())
        equipment = false;

    // if the remove-curse spell didn't remove a curse on the player,
    // it wants to remove a curse on their equipment
    if(equipment) {
        for(i=0; i<MAXWEAR; i++) {
            if(target->ready[i]) {
                target->ready[i]->clearFlag(O_CURSED);
                target->ready[i]->clearFlag(O_DARKNESS);
            }
        }

        if(target->flagIsSet(P_DARKNESS)) {
            for(const auto& obj : target->objects) {
                obj->clearFlag(O_DARKNESS);
            }
            player->print("The aura of darkness around you dissipates.\n");
            broadcast(player->getSock(), player->getParent(), "The aura of darkness around %N dissipates.", player.get());
            target->clearFlag(P_DARKNESS);
        }
    }
    return(1);
}
