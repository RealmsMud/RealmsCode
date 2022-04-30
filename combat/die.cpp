/*
 * die.cpp
 *   New death code
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

#include <boost/algorithm/string/case_conv.hpp>  // for to_lower
#include <cmath>                                 // for abs
#include <cstdio>                                // for sprintf
#include <cstdlib>                               // for abs
#include <cstring>                               // for strcpy, strncpy
#include <ctime>                                 // for time
#include <list>                                  // for operator==, _List_it...
#include <map>                                   // for allocator, operator==
#include <ostream>                               // for operator<<, basic_os...
#include <set>                                   // for set<>::iterator, set
#include <string>                                // for string, operator==
#include <type_traits>                           // for enable_if<>::type
#include <utility>                               // for pair

#include "bank.hpp"                              // for guildLog
#include "catRef.hpp"                            // for CatRef
#include "commands.hpp"                          // for finishDropObject
#include "config.hpp"                            // for Config, gConfig
#include "enums/loadType.hpp"                    // for LoadType, LoadType::...
#include "factions.hpp"                          // for Faction
#include "flags.hpp"                             // for P_OUTLAW, O_CURSED
#include "global.hpp"                            // for WIELD, CreatureClass
#include "group.hpp"                             // for Group, CreatureList
#include "guilds.hpp"                            // for Guild
#include "hooks.hpp"                             // for Hooks
#include "lasttime.hpp"                          // for lasttime
#include "location.hpp"                          // for Location
#include "money.hpp"                             // for Money, GOLD
#include "mud.hpp"                               // for LT_MOBDEATH, LT_KILL...
#include "mudObjects/container.hpp"              // for ObjectSet, MonsterSet
#include "mudObjects/creatures.hpp"              // for Creature, PetList
#include "mudObjects/monsters.hpp"               // for Monster
#include "mudObjects/objects.hpp"                // for Object
#include "mudObjects/players.hpp"                // for Player
#include "mudObjects/rooms.hpp"                  // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"            // for UniqueRoom
#include "os.hpp"                                // for ASSERTLOG
#include "paths.hpp"                             // for BugLog
#include "proto.hpp"                             // for logn, broadcast, induel
#include "random.hpp"                            // for Random
#include "realm.hpp"                             // for COLD, EARTH, ELEC, FIRE
#include "server.hpp"                            // for Server, gServer, GOL...
#include "socket.hpp"                            // for Socket
#include "statistics.hpp"                        // for Statistics
#include "stats.hpp"                             // for Stat
#include "structs.hpp"                           // for saves
#include "threat.hpp"                            // for ThreatSet, ThreatTable
#include "unique.hpp"                            // for Lore
#include "utils.hpp"                             // for MAX, MIN
#include "web.hpp"                               // for updateRecentActivity
#include "xml.hpp"                               // for loadObject


class Property;

//********************************************************************
//                      isHardcore
//********************************************************************
// Return: true if player is flagged as Hardcore (death = permanent)

bool Player::isHardcore() const {
    return(flagIsSet(P_HARDCORE) && !isStaff());
}

//********************************************************************
//                      hardcoreDeath
//********************************************************************
bool canDrop(const Player* player, const Object* object, const Property* p);
bool delete_drop_obj(const BaseRoom* room, const Object* object, bool factionCanRecycle);

void hardcoreDeath(Player* player) {
    if(!player->isHardcore())
        return;
    bool factionCanRecycle = !player->inUniqueRoom() || Faction::willDoBusinessWith(player, player->getUniqueRoomParent()->getFaction());
    player->hooks.execute("preHardcoreDeath");

    for(int i=0; i<MAXWEAR; i++) {
        if(player->ready[i] && (!(player->ready[i]->flagIsSet(O_CURSED) && player->ready[i]->getShotsCur() > 0))) {
            player->ready[i]->clearFlag(O_WORN);
            player->doRemove(i);
        }
    }
    player->computeAC();
    player->computeAttackPower();


    BaseRoom* room = player->getRoomParent();
    Object* object=nullptr;
    ObjectSet::iterator it;
    for( it = player->objects.begin() ; it != player->objects.end() ; ) {
        object = (*it++);
        if(delete_drop_obj(room, object, factionCanRecycle) || !canDrop(player, object, nullptr) || object->flagIsSet(O_STARTING)) {
            delete object;
            continue;
        }

        player->delObj(object, false, true, true, false);
        object->addToRoom(room);
    }
    player->checkDarkness();

    if(player->coins[GOLD]) {
        object=nullptr;
        loadObject(MONEY_OBJ, &object);
        object->nameCoin("gold", player->coins[GOLD]);
        object->setDroppedBy(player, "HardcoreDeath");
        object->value.set(player->coins[GOLD], GOLD);
        player->coins.sub(player->coins[GOLD], GOLD);
        Server::logGold(GOLD_OUT, player, object->value, nullptr, "HardcoreDeath");

        object->addToRoom(room);
    }

    player->printColor("\n\n                 ^YYou have died.\n\n");
    player->printColor("^RAs a hardcore character, this death is permanent.\n");
    player->print("\n\n");
    player->statistics.display(player, true);
    player->print("\n");
    broadcast("^#^R### %s's soul is lost forever.", player->getCName());
    player->hooks.execute("postHardcoreDeath");
    deletePlayer(player);
}

//********************************************************************
//                      isHoliday
//********************************************************************
// if a string is returned, player will get bonus exp

std::string isHoliday() {
    std::string str = gConfig->getMonthDay();

    // see if today is a holiday
    if(str == "Oct 31")
        return("Happy Halloween!");

    if(str == "Dec 24" || str == "Dec 25")
        return("Merry Christmas!");

    if(str == "Dec 31" || str == "Jan  1")
        return("Happy New Year!");

    return("");
}


//********************************************************************
//                      dropCorpse
//********************************************************************
// Parameters:  <killer> The creature attacking
// Handles the dropping of items from creatures

void Monster::dropCorpse(Creature *killer) {
    BaseRoom* room = getRoomParent();
    std::string str = "", carry = "";
    Object      *object=nullptr;
    Player*     player=nullptr;
    Player*     pMaster = isPet() ? getPlayerMaster() : nullptr;
    bool        destroy = (room->isDropDestroy() && !flagIsSet(M_IGNORE_ROOM_DESTROY));

    if(killer)
        player = killer->getPlayerMaster();

    killDarkmetal();
    killUniques();

    // Drop weapons also
    if(ready[WIELD-1]) {
        Lore::remove(pMaster, ready[WIELD-1], true);
        if(destroy) {
            unequip(WIELD, UNEQUIP_DELETE);
        } else {
            if(player)
                player->printColor("%M dropped their weapon: %1P.\n", this, ready[WIELD - 1]);
            Object* drop = unequip(WIELD, UNEQUIP_NOTHING, false);
            if(drop) {
                if(drop->flagIsSet(O_JUST_LOADED))
                    drop->setDroppedBy(this, "MobDeath");
                drop->addToRoom(room);
            }
        }
    }


    // list all on death
    // check for player: for mob killing pet, player is null, and then we don't
    // care about listing items dropped
    if(!destroy && player)
        str = this->listObjects(player, true);

    ObjectSet::iterator it;
    for( it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);
        if(object->flagIsSet(O_JUST_LOADED))
            object->setDroppedBy(this, "MobDeath");

        Lore::remove(pMaster, object, true);
        delObj(object, false, false, true, false);

        if(destroy)
            delete object;
        else if(!flagIsSet(M_TRADES) || !object->info.id) {
            object->clearFlag(O_BODYPART);
            object->addToRoom(room);
        }
    }
    checkDarkness();

    if(!destroy) {
        if(!coins.isZero()) {
            loadObject(MONEY_OBJ, &object);
            object->value.set(coins);

            object->setDroppedBy(this, "MobDeath");

            object->nameCoin("gold", object->value[GOLD]);
            object->addToRoom(room);

            if(player) {
                if(!str.empty())
                    str += ", ";
                str += object->getName();
            }
        }

        if(player && !str.empty()) {
            carry = getCrtStr(player, CAP | INV, 0);
            carry += " was carrying: ";
            carry += str;
            carry += ".\n";

            player->printColor("%s", carry.c_str());
            if(!player->flagIsSet(P_DM_INVIS))
                broadcastGroup(true, killer, "%s", carry.c_str());
        }
    }
}

//********************************************************************
//                      die
//********************************************************************
// wrapper for die

void Creature::die(Creature *killer) {
    bool freeTarget=true;
    die(killer, freeTarget);
}

//********************************************************************
//                      die
//********************************************************************
// Parameters:  <killer> The creature attacking
// Handles the death of people

void Creature::die(Creature *killer, bool &freeTarget) {
    Player*  pVictim = getAsPlayer();
    Monster* mVictim = getAsMonster();

    Player*  pKiller = killer->getAsPlayer();
    Monster* mKiller = killer->getAsMonster();
    bool duel = induel(pVictim, pKiller);

    if(pKiller) {
        pKiller->statistics.kill();
        if(mVictim)
            pKiller->statistics.monster(mVictim);
        if(pKiller->hasCharm(getName()))
            pKiller->delCharm(this);
    }

    if(pVictim)
        freeTarget = false;

    // Any time a level 7+ player dies, they are auto backed up.
    if(pVictim && pVictim->getLevel() >= 7 && !duel)
        pVictim->save(true, LoadType::LS_BACKUP);

    Hooks::run(killer, "preKill", this, "preDeath", std::to_string(duel));

    if(mKiller && mKiller->isPet() && pVictim) {
        pVictim->dieToPet(mKiller);

    } else if(mKiller && mKiller->isPet() && mVictim) {
        mVictim->dieToPet(mKiller, freeTarget);

    } else if(mKiller && mVictim) {
        mVictim->dieToMonster(mKiller, freeTarget);

    } else if(mKiller && pVictim) {
        pVictim->dieToMonster(mKiller);

    } else if(pKiller && mVictim) {
        mVictim->dieToPlayer(pKiller, freeTarget);

    } else if(pKiller && pVictim) {
        pVictim->dieToPlayer(pKiller);

    }
    if(pVictim) {
        pVictim->clearFocus();
    }

    // 10% chance to get porphyria/lycanthropy when killed by a vampire/werewolf
    if(pVictim && !duel) {
        pVictim->addPorphyria(killer, 10);
        pVictim->addLycanthropy(killer, 10);
    }
}

//********************************************************************
//                      dieToMonster
//********************************************************************
// Parameters:  <killer> The creature attacking
// Handles monsters killing players

void Player::dieToMonster(Monster *killer) {
    // Some sanity checks here -- Should never fail these
    ASSERTLOG( !killer->isPet() );

    // no real penalties for staff dying
    if(isStaff()) {
        printColor("^r*** You just died ***\n");
        broadcast(::isStaff, "^G### Sadly, %s just died.", getCName());

        clearAsEnemy();

        hp.restore();
        mp.restore();

        return;
    }

    print("%M killed you!\n", killer);

    if(flagIsSet(P_OUTLAW) && killer->getLevel() > 10)
        clearFlag(P_OUTLAW);

    setFlag(P_KILLED_BY_MOB);

    // Stop level 13+ players from suiciding right after dieing
    if(level >= 13) {
        lasttime[LT_MOBDEATH].ltime = time(nullptr);
        lasttime[LT_MOBDEATH].interval = 60*60*24L;
        setFlag(P_NO_SUICIDE);
    }

    unsigned long oldxp = experience;
    unsigned short oldlvl = level;

    broadcast("### Sadly, %s was killed by %1N.", getCName(), killer);
    killer->clearEnemy(this);
    clearTarget();
    clearAsEnemy();
    gServer->clearAsEnemy(this);
    loseExperience(killer);
    dropEquipment(false);
    logDeath(killer);

    // more info for staff
    broadcast(::isCt, "^rOld Exp: %u Lost Exp: %d, Old Lvl: %u, Lvl: %u, Room: %s",
        oldxp, (int)(oldxp - experience), oldlvl, level, getRoomParent()->fullName().c_str());

    resetPlayer(killer);
}

//********************************************************************
//                      dieToPet
//********************************************************************
// Parameters:  <killer> The creature attacking
// Handles pets killing players

void Player::dieToPet(Monster *killer) {
    Player  *master=nullptr;

    if(killer->getMaster())
        master = killer->getMaster()->getAsPlayer();
    else {
        broadcast(::isCt, "^y*** Pet %s has no master and is trying to kill a player. Room %s",
            killer->getCName(), killer->currentLocation.room.displayStr().c_str());
        return;
    }

    bool dueling = induel(this, master);

    if(dueling)
        broadcast("^R### Sadly, %s was killed by %s's %s in a duel.", getCName(), master->getCName(), killer->getCName());
    else {
        if(level > 2)
            broadcast("### Sadly, %s was killed by %s's %s.", getCName(), master->getCName(), killer->getCName());
        else {
            broadcast(::isCt, "^y### Sadly, %s was killed by %s's %s.", getCName(), master->getCName(), killer->getCName());
            print("### Sadly, %s was killed by %s's %s.", getCName(), master->getCName(), killer->getCName());
        }
    }

    killer->clearEnemy(this);
    getPkilled(master, dueling);
}

//********************************************************************
//                      dieToPet
//********************************************************************
// Parameters:  <killer> The creature attacking
// Handles pets killing monsters

void Monster::dieToPet(Monster *killer, bool &freeTarget) {
    Creature* petKiller=nullptr;
    Player* pKiller=nullptr;
    logDeath(killer);

    if(killer->getMaster()) {
        petKiller = killer;
        pKiller = killer->getMaster()->getAsPlayer();
    } else {
        broadcast(::isCt, "^y*** Pet %s has no master and is trying to kill a mob. Room %s",
            killer->getCName(), killer->currentLocation.room.displayStr().c_str());
        return;
    }


    broadcast(nullptr, pKiller->getRoomParent(), "%M's %s killed %N.", pKiller, petKiller->getCName(), this);

    mobDeath(pKiller, freeTarget);
}


//********************************************************************
//                      dieToMonster
//********************************************************************
// Parameters:  <this> The creature being attacked
//              <killer> The creature attacking

// Handles monsters killing monsters
void Monster::dieToMonster(Monster *killer, bool &freeTarget) {
    if(this != killer)
        broadcast(nullptr, killer->getRoomParent(), "%M killed %N.", killer, this);
    mobDeath(killer, freeTarget);
}



//********************************************************************
//                      dieToPlayer
//********************************************************************
// Parameters:  <killer> The creature attacking
// Handles players killing monsters

void Monster::dieToPlayer(Player *killer, bool &freeTarget) {
    logDeath(killer);
    if(getMaster() != killer) {
        killer->print("You killed %N.\n", this);
        broadcast(killer->getSock(), killer->getRoomParent(), "%M killed %N.", killer, this);
    }
    mobDeath(killer, freeTarget);
}

//********************************************************************
//                      mobDeath
//********************************************************************
// Common routine for all mob's dying.

void Monster::mobDeath(Creature *killer) {
    bool freeTarget=true;
    mobDeath(killer, freeTarget);
}
void Monster::mobDeath(Creature *killer, bool &freeTarget) {
    if(killer)
        killer->checkDoctorKill(this);

    distributeExperience(killer);
    dropCorpse(killer);
    cleanFollow(killer);
    clearAsEnemy();

    // does the calling function want us to free the target?
    if(freeTarget)
        finishMobDeath(killer);

    // freeTarget now means: do I (calling function) need to free the target?
    freeTarget = !freeTarget;
}

//********************************************************************
//                      finishMobDeath
//********************************************************************

void Monster::finishMobDeath(Creature *killer) {
    //if(killer)
    // a null killer is valid, so the called function should handle that properly
    Hooks::run(killer, "postKill", this, "postDeath", "0");

    deleteFromRoom();
    gServer->delActive(this);
    delete this;
}


//********************************************************************
//                      dieToPlayer
//********************************************************************
// Parameters:  <killer> The creature attacking
// Handles players killing players

void Player::dieToPlayer(Player *killer) {
    char    deathstring[80];

    bool dueling = induel(this, killer);

    killer->print("You killed %N.\n", this);
    print("%M killed you.\n", killer);
    broadcast(killer->getSock(), getSock(), killer->getRoomParent(), "%M killed %N.", killer, this);

    if(killer->isEffected("lycanthropy") && killer->getLevel() >= 13)
        strcpy(deathstring, "eaten");
    else if(killer->getClass() == CreatureClass::ASSASSIN && killer->getLevel() >= 13)
        strcpy(deathstring, "assassinated");
    else
        strcpy(deathstring, "killed");

    if(dueling)
        broadcast("^R### Sadly, %s was %s by %s in a duel.", getCName(), deathstring, killer->getCName());
    else// if(level > 2)
        broadcast("### Sadly, %s was %s by %1N.", getCName(), deathstring, killer);
    //else {
    //  broadcast(::isWatcher, "^C### Sadly, %s was %s by %1N.", name, deathstring, killer);
    //  print("### Sadly, %s was %s by %1N.\n", name, deathstring, killer);
    //}

    getPkilled(killer, dueling);
}

//********************************************************************
//                      getPkilled
//********************************************************************

void Player::getPkilled(Player *killer, bool dueling, bool reset) {
    unsigned long oldxp = experience;
    unsigned short oldlvl = level;

    if(isGuildKill(killer))
        guildKill(killer);

    if(isGodKill(killer))
        godKill(killer);
    else if(isClanKill(killer))
        clanKill(killer);

    // make the pets stop attacking the players
    clearAsPetEnemy();
    killer->clearAsPetEnemy();

    if(!dueling) {
        updatePkill(killer);

        dropBodyPart(killer);

        if(!killer->isStaff())
            dropEquipment(true, killer->getSock());

        logDeath(killer);

        // more info for staff
        broadcast(::isCt, "^rOld Exp: %u Lost Exp: %d, Old Lvl: %u, Lvl: %u, Room: %s",
            oldxp, (int)(oldxp - experience), oldlvl, level, getRoomParent()->fullName().c_str());

    }

    if(reset)
        resetPlayer(killer);
}

//********************************************************************
//                      isGodKill
//********************************************************************

bool Player::isGodKill(const Player *killer) const {
    if(induel(killer, this))
        return(false);

    if(killer->getLevel() < 7 || level < 7)
        return(false);

    if(!killer->getDeity() || !deity)
        return(false);

    if(killer->isStaff())
        return(false);

    return(true);
}

//********************************************************************
//                      isClanKill
//********************************************************************

bool Player::isClanKill(const Player *killer) const {

    if(flagIsSet(P_OUTLAW_WILL_LOSE_XP))
        return(true);

    if(induel(killer, this))
        return(false);

    if(killer->getLevel() < 7 || level < 7)
        return(false);

    if(clan && killer->getClan())
        return(true);


    // Dk vs Paly, Paly vs Dk, or Dk vs Dk
    if( (killer->getClass() == CreatureClass::DEATHKNIGHT && cClass == CreatureClass::PALADIN) ||
        (killer->getClass() == CreatureClass::DEATHKNIGHT && (cClass == CreatureClass::PALADIN || cClass == CreatureClass::DEATHKNIGHT))
    )
        return(true);

    if( ((killer->getClass() == CreatureClass::CLERIC && killer->flagIsSet(P_CHAOTIC) && killer->getLevel() >= 7) && flagIsSet(P_PLEDGED)) ||
        ((cClass == CreatureClass::CLERIC && flagIsSet(P_CHAOTIC) && level >= 7) && killer->flagIsSet(P_PLEDGED))
    )
        return(true);

    return(false);
}

//********************************************************************
//                      guildKill
//********************************************************************

int Player::guildKill(Player *killer) {
    int     bns=0, penalty=0, levelDiff=0;
    Guild *killerGuild, *thisGuild;
    int     same=0, base=0;
    long    total=0;

    killerGuild = gConfig->getGuild(killer->getGuild());
    thisGuild = gConfig->getGuild(guild);

    base = level * 50;

    levelDiff = level - killer->getLevel();

    if(levelDiff < 0)
        bns += -5 * level * levelDiff;
    else
        bns += levelDiff * 50;

    if(guildRank == GUILD_MASTER)
        bns *= 3;
    else if(guildRank == GUILD_BANKER)
        bns *= 2;
    else if(guildRank == GUILD_OFFICER)
        bns = (bns*3)/2;

    total = MAX(1, Random::get((base + bns)/2, base + bns));

    if(killer->halftolevel())
        total = 0;

    if(killer->hasSecondClass())
        total = total * 3 / 4;

    penalty = MIN(Random::get(1000,1500), (bns*3)/2);

    if(killer->getLevel() > level + 6)
        penalty = 100;

    if(killerGuild == thisGuild) {
        total = -1000;
        penalty = 1000;
        same = 1;
    } else {
        killerGuild->incPkillsIn();
        killerGuild->incPkillsWon();
        thisGuild->incPkillsIn();

        killerGuild->bank.add(coins);
        Bank::guildLog(killer->getGuild(), "Guild PKILL: %s by %s [Balance: %s]\n",
            coins.str().c_str(), killer->getCName(), killerGuild->bank.str().c_str());
        coins.zero();

        gConfig->saveGuilds();
    }


    if(!same) {
        killer->printColor("^gYou have defeated a member of a rival guild!\n");
        killer->print("Your guild honors you with %d experience.\n", std::abs(total));
        if(coins[GOLD])
            killer->printColor("You grab %N's coins and put them in your guild bank.^x\n", this);

        printColor("^gYou have been defeated by a member of a rival guild!\n");
        print("Your guild shames you by taking %d experience.\n", std::abs(penalty));
        if(coins[GOLD])
            printColor("%M grabs all of your coins!\n", killer);
    } else {
        killer->printColor("^gYou have shamelessly defeated a member of your own guild!\n");
        killer->printColor("Your guild penalizes you for %d experience.\n", std::abs(total));

        printColor("^gYou have been defeated during senseless guild infighting!\n");
        printColor("Your guild penalizes you for %d experience.\n", std::abs(penalty));
    }

    killer->addExperience(total);
    subExperience(penalty);

    return(1);
}

//********************************************************************
//                      godKill
//********************************************************************

int Player::godKill(Player *killer) {
    int     bns=0, penalty=0, levelDiff=0;
    int     same=0, base=0;
    Guild *killerGuild, *thisGuild;
    long    total=0;

    killerGuild = gConfig->getGuild(killer->getGuild());
    thisGuild = gConfig->getGuild(guild);

    base = level * 50;
    levelDiff = level - killer->getLevel();

    if(levelDiff < 0)
        bns += 5 * level * levelDiff;
    else
        bns += levelDiff * 50;


    total = MAX(1, Random::get((base + bns)/2, base + bns));

    if(killer->halftolevel())
        total = 0;

    if(killer->hasSecondClass())
        total = total * 3 / 4;

    penalty = MIN(Random::get(1000,1500), (bns*3)/2);

    if(killer->getLevel() > level + 6)
        penalty = 100;

    if(killer->getDeity() == deity) {
        total = -50;
        penalty = 50;
        same = 1;
    }
    if(killerGuild != nullptr && thisGuild != nullptr) {
        if(killerGuild != thisGuild) {
            killerGuild->incPkillsIn();
            killerGuild->incPkillsWon();
            thisGuild->incPkillsIn();
        }
    }


    if(!same) {
        killer->printColor("^cYou have defeated a member of an enemy religion!\n");
        killer->printColor("Your order honors you with %d experience.\n", std::abs(total));

        printColor("^cYou have been defeated by a member of a rival religion!\n");
        printColor("Your order shames you by taking %d experience.\n", std::abs(penalty));
    } else {
        killer->printColor("^cYou have shamelessly defeated a member of your own religion!\n");
        killer->printColor("Your order penalizes you for %d experience.\n", std::abs(total));

        printColor("^cYou have been defeated during senseless religious infighting!\n");
        printColor("Your order penalizes you for %d experience.\n", std::abs(penalty));
    }

    killer->addExperience(total);
    subExperience(penalty);

    return(1);
}

//********************************************************************
//                      clanKill
//********************************************************************

int Player::clanKill(Player *killer) {
    bool penalty = false; // Do we give a penalty for killing this person?
    int expGain = 0, expLoss = 0;

    // Paly's shouldn't be killing each other
    if(cClass == CreatureClass::PALADIN && killer->getClass() == CreatureClass::PALADIN && deity == killer->getDeity())
        penalty = true;
    // Clan members should not be killing members of their own clan
    if(clan == killer->getClan())
        penalty = true;
    // Never a penalty for killing an outlaw!
    if(flagIsSet(P_OUTLAW_WILL_LOSE_XP))
        penalty = false;

    // No penalty
    if(!penalty) {
        expGain = (level * killer->getLevel()) * 10;
        expLoss = expGain * 2;

        if(killer->halftolevel())
            expGain = 0;
        if(killer->hasSecondClass())
            expGain = expGain * 3 / 4;

        print("You have been bested!\nYou lose %d experience.\n", expLoss);
        subExperience(expLoss);

        if(!killer->flagIsSet(P_OUTLAW)) {
            killer->print("You have vanquished %s.\n", getCName());
            killer->print("You %s %d experience for your heroic deed.\n", gConfig->isAprilFools() ? "lose" : "gain", expGain);
            killer->addExperience(expGain);
        }
    }
    return(1);
}

//********************************************************************
//                      checkDoctorKill
//********************************************************************
// Parameters:  <player>
//              <killer> The creature attacking
// Checks for doctor killers

void Creature::checkDoctorKill(Creature *victim) {
    if(victim->getName() == "doctor") {
        if( (isPlayer() && !isStaff()) ||
            (isMonster() && isPet() && !getMaster()->isStaff()))
        {
            Creature* target = isPlayer() ? this : getMaster();

            target->setFlag(P_DOCTOR_KILLER);
            if(!target->flagIsSet(P_DOCTOR_KILLER))
                broadcast("### %s is a doctor this!", target->getCName());
            target->lasttime[LT_KILL_DOCTOR].ltime = time(nullptr);
            target->lasttime[LT_KILL_DOCTOR].interval = 72000L;
        }
    }
}


//********************************************************************
//                      checkLevel
//********************************************************************
// Parameters:  <player>
// Checks if a player has deleveled or releveled
//void doTrain(Player* player) {
//
//
//}

int Player::checkLevel() {
    // Staff doesn't delevel or relevel
    if(isCt())
        return(0);

    int n = exp_to_lev(experience);
    // De-Level!
    if(level > n) {
        print("You have deleveled to level %s!\n", int_to_text(n));
        while(level > n)
            downLevel();

        negativeLevels = 0;

        return(-1); // Delevel
    }

    // Re-Level!
    if(level < n && level < actual_level && n <= actual_level && !negativeLevels) {
        if(inUniqueRoom() && getUniqueRoomParent()->getHighLevel() && level == getUniqueRoomParent()->getHighLevel()) {
            print("You have enough experience to relevel, but cannot do so in this room.\n");
            return(0);
        }
        print("You have releveled to level %s!\n", int_to_text(n));
        logn("log.relevel", "%s just releveled to level %d from level %d in room %s.\n",
                getCName(), n, level, getRoomParent()->fullName().c_str());
        if(!isStaff())
            broadcast("### %s just releveled to %s!", getCName(), int_to_text(n));
        while(level < n)
            upLevel();
        return(1); // Relevel
    }

    // OCEANCREST: Dom: HC
    //if(isHardcore() && level < n && !negativeLevels) {
    //  doTrain(this);
    //  return(2); // Level
    //}

    return(0); // No change
}

//********************************************************************
//                      updatePkill
//********************************************************************

void Player::updatePkill(Player *killer) {
    // No pkill changes if either party is staff
    if(isStaff() || killer->isStaff())
        return;

    statistics.losePk();
    killer->statistics.winPk();
}

//********************************************************************
//                      dropEquipment
//********************************************************************
// Parameters:  <killer> The creature attacking
// Function to make players drop the equipment they are currently wearing
// via a pkill

void Player::dropEquipment(bool dropAll, Socket* killerSock) {
    std::string dropString = "";
    int     i=0;

    // dropping weapons is handled separately from equipment, but we still need their
    // names for dropping later
    Object* main = ready[WIELD-1];
    Object* held = ready[HELD-1];

    dropWeapons();

    // be nice to lowbies
    if(dropAll && level > 3) {


        // don't make them lose all their inventory in a drop-destroy room
        if(!getRoomParent()->isDropDestroy()) {

            // we reassign these for the purposes of having them printed in the list;
            // they will be reset afterwards
            Object* rMain = ready[WIELD-1];
            Object* rHeld = ready[HELD-1];
            ready[WIELD-1] = main;
            ready[HELD-1] = held;

            for(i=0; i<MAXWEAR; i++) {
                if( !ready[i] ||
                    ready[i]->flagIsSet(O_CURSED) ||
                    ready[i]->flagIsSet(O_NO_DROP) ||
                    ready[i]->flagIsSet(O_STARTING)
                )
                    continue;
                // 20% chance each item will be dropped (ignore this chance for wielded items
                // as we want them added to the list of stuff dropped)
                if((i != WIELD-1 && i != HELD-1) && Random::get(1,5) > 1)
                    continue;

                if(!dropString.empty())
                    dropString += ", ";
                dropString += ready[i]->getName();

                if(i != WIELD-1 && i != HELD-1) {
                    // I is wearloc-1, so add one to it
                    Object* temp = unequip(i+1, UNEQUIP_NOTHING, false);
                    if(temp)
                        finishDropObject(temp, getRoomParent(), this);
                }
            }

            // reset these wear locations
            ready[WIELD-1] = rMain;
            ready[HELD-1] = rHeld;

            if(!dropString.empty()) {
                if(killerSock)
                    killerSock->printColor("%s dropped: %s.\n", getCName(), dropString.c_str());

                print("You dropped your inventory where you died!\n");
            }
        }
    }

    checkDarkness();
    computeAC();
    computeAttackPower();
}


//********************************************************************
//                      dropBodyPart
//********************************************************************
// Parameters:  <killer> The creature attacking
// Makes a player drop a body part

void Player::dropBodyPart(Player *killer) {
    ASSERTLOG( killer->isPlayer());

    if(getRoomParent()->isDropDestroy())
        return;

    bool nopart = false;
    int dueling = 0, num = 0;
    Object *body_part;
    char part[12], partName[25];

    dueling = induel(this,killer);

    if(Random::get<bool>(0.95) && !killer->isDm() && !flagIsSet(P_OUTLAW))
        nopart = true;

    if(level <= 4 && !killer->isDm() && !flagIsSet(P_OUTLAW))
        nopart = true;

    if( isStaff() || (
            !killer->isPet() &&
            !killer->isDm() &&
            (killer->getLevel() > level + 5) &&
            !flagIsSet(P_OUTLAW))
        || (
            killer->isPet() &&
            !killer->getMaster()->isDm() &&
            (killer->getMaster()->getLevel() > level + 5) &&
            !flagIsSet(P_OUTLAW)
        ) )
    {
        nopart = true;
    }

    if(!nopart && !dueling) {

        if(loadObject(BODYPART_OBJ, &body_part)) {
            num = Random::get(1,14);

            switch(num) {
            case 1:
                strcpy(part, "skull");
                break;
            case 2:
                strcpy(part, "head");
                break;
            case 3:
                strcpy(part, "ear");
                break;
            case 4:
                strcpy(part, "scalp");
                break;
            case 5:
                strcpy(part, "hand");
                break;
            case 6:
                strcpy(part, "spleen");
                break;
            case 7:
                strcpy(part, "nose");
                break;
            case 8:
                strcpy(part, "arm");
                break;
            case 9:
                strcpy(part, "foot");
                break;
            case 10:
                strcpy(part, "spine");
                break;
            case 11:
                strcpy(part, "heart");
                break;
            case 12:
                strcpy(part, "brain");
                break;
            case 13:
                strcpy(part, "eyeball");
                break;
            case 14:
                if(isEffected("vampirism"))
                    strcpy(part, "fangs");
                else
                    strcpy(part, "teeth");
                break;
            default:
                strcpy(part, "skull");
                break;
            }

            std::string newPartName = getName() + "'s " + part;
            boost::algorithm::to_lower(newPartName);
            body_part->setName(newPartName);

            lowercize(partName, 0);
            strncpy(body_part->key[0], partName, 20);
            strncpy(body_part->key[1], part, 20);
            strncpy(body_part->key[2], part, 20);

            body_part->setAdjustment(Random::get(1,2));
            if(Random::get(1,100) == 1)
                body_part->setAdjustment(3);

            body_part->addToRoom(getRoomParent());
        }
    }

}

//********************************************************************
//                      logDeath
//********************************************************************

void Player::logDeath(Creature *killer) {
    char    file[16], killerName[80];
    Player* pKiller = killer->getAsPlayer();

    if(!killer->isStaff())
        statistics.die();
    strcpy(killerName, "");

    if(killer->isPet())
        sprintf(killerName, "%s's %s.", killer->getMaster()->getCName(), killer->getCName());
    else
        strcpy(killerName, killer->getCName());

    if(pKiller)
        strcpy(file, "log.pkill");
    else
        strcpy(file, "log.death");

    if(!(!pKiller && killer->flagIsSet(M_NO_EXP_LOSS))) {
        logn(file, "%s(%d) was killed by %s(%d) in room %s.\n", getCName(), level,
             killer->getCName(), killer->getLevel(), killer->getRoomParent()->fullName().c_str());

    }

    if(pKiller && pKiller->isStaff()) {
        log_immort(false, pKiller, "%s(%d) was killed by %s(%d) in room %s.\n", getCName(), level,
            pKiller->getCName(), pKiller->getLevel(), pKiller->getRoomParent()->fullName().c_str());
    }

    updateRecentActivity();
}

//********************************************************************
//                      resetPlayer
//********************************************************************
// Parameters:  <killer> The creature attacking
// Reset a player after death -- Move them to limbo, broadcast
// they were killed, clear deterimental effects, etc

void Player::resetPlayer(Creature *killer) {
    int     duel=0;
    bool    same=false;
    Player* pKiller = killer->getAsPlayer();
    BaseRoom *newRoom = getLimboRoom().loadRoom(this);

    duel = induel(this, pKiller);

    if( inJail() ||
        duel ||
        !newRoom
    )
        same = true;

    if(cClass == CreatureClass::BUILDER) {
        same = true;
        print("*** You died ***\n");
        broadcast(::isStaff, "^G### Sadly, %s was killed by %s.", getCName(), killer->getCName());
    }

    // a hardcore player about to die doesnt need to worry about room movement
    if(isHardcore() && !killer->isStaff())
        same = true;

    if(!same) {
        Hooks::run(killer, "postKillPreLimbo", this, "postDeathPreLimbo");

        deleteFromRoom();
        addToRoom(newRoom);
        doPetFollow();
    }
    //doClearPetEnemy(this, killer->getCName());

    curePoison();
    cureDisease();
    tickDmg = 0;
    removeCurse();
    removeEffect("hold-person");
    removeEffect("petrification");
    removeEffect("confusion");
    removeEffect("drunkenness");
    unhide();

    if(killer->isPlayer() || duel) {
        hp.setCur( MAX<int>(1, MAX(hp.getMax()/2, hp.getCur())));
        mp.setCur(MAX(mp.getCur(),(mp.getMax())/10));
    } else {
        hp.restore();
        mp.restore();
    }

    if(duel) {
        setFlag(P_DIED_IN_DUEL);
        knockUnconscious(10);
        broadcast(getSock(), getRoomParent(), "%s is knocked unconscious!", getCName());

        updateAttackTimer(true, 300);
        pKiller->delDueling(getName());
        delDueling(killer->getName());
    } else if(isHardcore() && !killer->isStaff()) {
        hardcoreDeath(this);
        // the player is invalid after this
        return;
    } else {
        courageous();
    }

    killer->hooks.execute("postKill", this, std::to_string(duel));
    hooks.execute("postDeath", killer, std::to_string(duel), std::to_string(same));
}

//********************************************************************
//                      hearMobDeath
//********************************************************************

bool hearMobDeath(Socket* sock) {
    if(!sock->getPlayer() || !isCt(sock))
        return(false);
    return(!sock->getPlayer()->flagIsSet(P_NO_DEATH_MSG));
}

//********************************************************************
//                      logDeath
//********************************************************************

void Monster::logDeath(Creature *killer) {
    int         logType=0;
    Creature *leader=nullptr, *pet=nullptr;
    BaseRoom* room = killer->getRoomParent();
    char        file[80], killerString[1024];
    char        logStr[2096];

    bool        solo = true;


    strcpy(file, "");
    strcpy(killerString, "");

    if(flagIsSet(M_WILL_BE_LOGGED)) {
        strcpy(file, "log.mdeath");
        logType = 1;
    } else if(flagIsSet(M_PERMENANT_MONSTER) && (killer->isPlayer() || killer->isPet())) {
        strcpy(file, "log.perm");
        logType = 2;
    } else if(killer->pFlagIsSet(P_BUGGED) ) {
        sprintf(file, "%s/%s", Path::BugLog.c_str(), killer->getCName());
        logType = 3;
    } else if(killer->pFlagIsSet(P_KILLS_LOGGED) ) {
        sprintf(file, "%s/%s.kills", Path::BugLog.c_str(), killer->getCName());
        logType = 4;
    } else
        return; //Mob's death not logged

    // There are 4 cases for perm broadcast, otherwise we do single broadcast
    // (which includes the pet message).

    // group broadcast (aka solo = 1)
    //      killer is pet
    //          pet's leader has a player follower
    //          pet's leader is following
    //      killer is a player
    //          killer has a player follower
    //          killer is following

    if(killer->isPlayer() || killer->isPet()) {
        // we have the leader of the pet
        if(killer->isPet())
            leader = killer->getMaster();
        else
            leader = killer;

        // see if they are in a group
        if(leader->getGroup())
            solo = false;
    }

    if(killer->isPet() || leader->hasPet()) {
        if(leader->pets.size() == 1) {
            pet = leader->pets.front();
            sprintf(killerString, "%s and %s %s", leader->getCName(), leader->hisHer(), pet->getCName());
        } else {
            sprintf(killerString, "%s and %s pets", leader->getCName(), leader->hisHer());
        }
    } else {
        sprintf(killerString, "%s", killer->getCName());
    }


    switch(logType) {
    case 1: // unsaved mob
        sprintf(logStr, "%s(L%d) was killed by %s(L%d) in room %s for %lu experience.",
                getCName(), level, killerString, killer->getLevel(), room->fullName().c_str(), experience);
        broadcast(hearMobDeath, "^g*** %s(L%d) was killed by %s(L%d) in room %s for %lu experience.",
                getCName(), level, killerString, killer->getLevel(), room->fullName().c_str(), experience);
        break;
    case 2: // Mob is a perm
        {
            Group* group = leader->getGroup();

            // if the killer was a pet, leader is pointing to the pet's leader
            // make it point to the leader of the pet's leader
            if(group)
                leader = group->getLeader();

            if(!solo)
                sprintf(logStr, "%s(L%d) was killed by %s(L%d) in %s(L%d)'s group in room %s.",
                        getCName(), level, killerString, killer->getLevel(),
                     leader->getCName(), leader->getLevel(), room->fullName().c_str());
            else
                sprintf(logStr, "%s(L%d) was killed by %s(L%d) in room %s [SOLO].",
                        getCName(), level, killerString, killer->getLevel(), room->fullName().c_str());


            if(!killer->isStaff() && flagIsSet(M_NO_PREFIX)) {
                if(!solo)
                    broadcast(wantsPermDeaths, "^m### Sadly, %s was killed by %s and %s followers.", getCName(), leader->getCName(), leader->hisHer());
                else
                    broadcast(wantsPermDeaths, "^m### Sadly, %s was killed by %s.", getCName(), killerString);
            }
        }
        break;
    case 3: // bugged player
    case 4: // all player's kills logged
        sprintf(logStr, "%s(L%d) was killed by %s(L%d) in room %s for %lu experience%s.",
                getCName(), level, killerString, killer->getLevel(),
             room->fullName().c_str(), experience, solo == 1 ? "[SOLO]" : "");
        break;
    }

    logn(file, "%s\n", logStr);
    // I don't want to see bugged kill logs...too much spam
    if((killer->isPlayer() || killer->isPet()) && !killer->isCt() && logType != 4)
        broadcast(hearMobDeath, "^y*** %s", logStr);
}

//********************************************************************
//                      loseExperience
//********************************************************************
// Parameters:  <killer> The creature attacking
// Handles experience/realms loss & death effects

void Player::loseExperience(Monster *killer) {
    float   xploss=0.0;
    long    n=0;
    int     count=0;


    // No exp loss from possesed monsters
    if(killer->flagIsSet(M_DM_FOLLOW))
        return;

    if(killer->flagIsSet(M_NO_EXP_LOSS) || (killer->isPet() && killer->getMaster()->isStaff()))
        return;

    if(level >= 7)
        addEffect("death-sickness");

    if(level < 10) {
        // Under level 10, 10% exp loss
        xploss = ((float)experience / 10.0);
        statistics.experienceLost((long)xploss);
        experience -= (long)xploss;

    } else {
        // Level 10 and over, 2% exp loss with a minimum of 10k
        xploss = MAX<long>((long)( (float)experience * 0.02), 10000);
        statistics.experienceLost((long)xploss);
        experience -= (long)xploss;
    }
    print("You have lost %ld experience.\n", (long)xploss);
    n = level - exp_to_lev(experience);

    if(n > 1) {
        if(level < (MAXALVL+2))
            experience = Config::expNeeded(level-2);
        else
            experience = (long)((Config::expNeeded(MAXALVL)*(level-2)));
    }

    checkLevel();
//  n = exp_to_lev(experience);
//  while(level > n)
//      down_level(this);

    for(count=0; count < 6; count++) {      // Random loss to saving throws due to death.
        if(Random::get(1,100) <= 25) {
            saves[count].chance -= 1;
            saves[count].gained -= 1;
            saves[count].chance = MAX<short>(1, saves[count].chance);
            saves[count].gained = MAX<short>(1, saves[count].gained);
        }
    }
}

//********************************************************************
//                      giveExperience
//********************************************************************
// Parameters:  <killer> The creature attacking
// Handles experience gain on monster death

void Monster::distributeExperience(Creature *killer) {
    long    expGain = 0;

    Player* player=nullptr;

    if(isPet())
        return;

    if(flagIsSet(M_PERMENANT_MONSTER))
        diePermCrt();

    if(killer) {
        Group* group = nullptr;
        if(killer->isPet())
            player = killer->getMaster()->getAsPlayer();
        else
            player = killer->getAsPlayer();

        if(player) {
            group = player->getGroup();
        }

        std::map<Player*, int> expList;
        // See if the group has experience split turned on
        if(group && group->flagIsSet(GROUP_SPLIT_EXPERIENCE) && group->getNumPlyInSameRoom(player) >= 1) {

            // Split exp evenly amongst the group
            int numGroupMembers=0, totalGroupLevel=0, totalGroupDamage=0;
            long n = 0;

            // Calculate how many people are in the group, see how much damage they have done
            // and remove them from the enemy list

            Player* groupMember;
            for(Creature* crt : group->members )
            {
                if(isEnemy(crt) && inSameRoom(crt)) {
                    if(crt->getsGroupExperience(this)) {
                        // Group member
                        groupMember = crt->getAsPlayer();
                        numGroupMembers++;
                        totalGroupLevel += groupMember->getLevel();
                        n = clearEnemy(groupMember);
                        totalGroupDamage += n;
                        expList[groupMember] += n;
                    } else if(crt->isPet() && (crt->getMaster()->getsGroupExperience(this) || expList.find(crt->getPlayerMaster()) != expList.end() )) {
                        // If the master gets group experience, or was calculated earlier to get group experience (They're in expList already)
                        // then count the pet as the same
                        n = clearEnemy(crt);
                        totalGroupDamage += n;
                        expList[crt->getPlayerMaster()] += n;
                    }
                }
            }
            float xpPercent = (float)MIN<int>(totalGroupDamage, hp.getMax())/(float)hp.getMax();
            long adjustedExp = (long)((xpPercent*experience) * (1.0 + (.25*numGroupMembers)));

            // Exp is split amoungst the group based on their level,
            // since we split it evenly, in this case pets do NOT give their master
            // any extra experience.

            int averageEffort = totalGroupDamage / MAX<int>(expList.size(), 1);
            std::clog << "GROUP EXP: TGD:" << totalGroupDamage << " Num:" << expList.size() << " AVG EFF:" << averageEffort << std::endl;
            for(std::pair<Player*, int> p : expList) {
                Player* ply = p.first;
                int effort = p.second;

                if(ply) {
                    expGain = (long)(((float)ply->getLevel()/(float)totalGroupLevel) * adjustedExp);
//                  ply->printColor("You contributed ^m%d^x effort.  Average effort was ^m%d^x.\n", effort, averageEffort);
                    // Adjust based on reduced effort
                    if(effort < (averageEffort/2)) {
                        ply->printColor("You receive reduced experience because you contributed less than half of the average effort.\n");
                        expGain *= (((float)effort)/totalGroupDamage);
                    }
                    expGain = MAX<long>(1, expGain);
                    ply->gainExperience(this, killer, expGain, true);
                }
            }
        }
    }

    // Now handle everyone else on the list
    std::map<Player*, int> expList;

    auto tIt = threatTable->threatSet.begin();
    ThreatEntry* threat = nullptr;
    Creature* crt = nullptr;
    while(tIt != threatTable->threatSet.end()) {
    // Iterate it because we will be invaliding this iterator
        threat = (*tIt++);
        crt = gServer->lookupCrtId(threat->getUid());
        if(!crt) continue;

        if(crt->isPet())
            expList[crt->getPlayerMaster()] += clearEnemy(crt);
        else if(crt->isPlayer())
            expList[crt->getAsPlayer()] += clearEnemy(crt);
    }

    for(std::pair<Player*, int> p : expList) {
        Player* ply = p.first;
        if(!ply) {
            std::clog << "Distribute Experience: null Player found" << std::endl;
            continue;
        }
        int effort = p.second;

        expGain = (experience * effort) / MAX<int>(hp.getMax(), 1);
        expGain = MIN<long>(MAX<long>(0,expGain), experience);

        ply->gainExperience(this, killer, expGain);


        // TODO: Why is this here?
        // Adjust monk/wolf ac and thac0 for extreme aligment
        // **************************************************
        ply->alignAdjustAcThaco();
        // **************************************************
    }
}

//********************************************************************
//                      adjustExperience
//********************************************************************

void Creature::adjustExperience(Monster* victim, int& expAmount, int& holidayExp) {
    Player* player;
    if(isPet())
        player = getMaster()->getAsPlayer();
    else
        player = getAsPlayer();

    if(player->halftolevel()) {
        expAmount = 0;
        holidayExp = 0;
        return;
    }

    if(player->getRace() == HUMAN && expAmount)
        expAmount += MAX(Random::get(4,6),expAmount/3/10);

    if(player->hasSecondClass()) {
        // Penalty is 12.5% at level 30 and above
        if(player->level >= 30)
            expAmount = (expAmount*7)/8;
        else // and 25% below 30
            expAmount = (expAmount*3)/4;
    }
//  // All experience is multiplied by 3/4 for a multi-classed player
//  if(player->hasSecondClass())
//      expAmount = (expAmount*3)/4;

    int levelDiff = abs((int)player->getLevel() - (int)victim->getLevel());
    float multiplier=1.0;

    if(levelDiff <= 5)
        multiplier = 1.0;
    // 6 - 8 levels = 90%
    else if(levelDiff >= 6 && levelDiff <= 8) {
        multiplier = 0.9;
    } // 8 - 10 = 70%
    else if(levelDiff >= 8 && levelDiff <= 10 ) {
        multiplier = 0.70;
    } // 10 - 15 = 50%
    else if(levelDiff >= 10 && levelDiff <= 15) {
        multiplier = 0.50;
    } // 15 - 25 = 25%
    else if(levelDiff >= 15 && levelDiff <= 25) {
        multiplier = 0.25;
    } // 26+ = 10%
    else
        multiplier = 0.10;
    if(multiplier < 1.0) {
//      player->printColor("^YExp Adjustment: %d%% (%d level difference) %d -> %d\n", (int)(multiplier*100), levelDiff, expAmount, (int)(expAmount*multiplier));
        expAmount = (int)(expAmount * multiplier);
        expAmount = MAX(1, expAmount);
    }

    // Add in holiday experience
    std::string holidayStr = isHoliday();

    if(!holidayStr.empty())
        holidayExp = MAX(1, (int)(expAmount * 0.5));

    if(victim->flagIsSet(M_PERMENANT_MONSTER))
        holidayExp = 0;
}

//********************************************************************
//                      gainExperience
//********************************************************************

void Player::gainExperience(Monster* victim, Creature* killer, int expAmount, bool groupExp) {
    int holidayExp = 0;
    adjustExperience(victim, expAmount, holidayExp);
    bool notlocal = false, af = gConfig->isAprilFools();

    if(!inSameRoom(victim))
        notlocal = true;

    std::string holidayStr = isHoliday();
    if( groupExp ||
        this == killer ||
        !killer || !(notlocal &&
                    !(killer->isPlayer() && killer->flagIsSet(P_DM_INVIS)) &&
                    !(killer->isPet() && killer->getMaster()->flagIsSet(P_DM_INVIS))))
    {
        printColor("You %s ^y%d^x %sexperience for the death of %N.\n", af ? "lose" : "gain", expAmount, groupExp ? "group " : "", victim);
    } else {
        if(killer->isPet())
            print("%M's %s %s you %d experience for the death of %N.\n", killer->getMaster(), killer->getCName(), af ? "cost" : "gained", expAmount, victim);
        else
            print("%M %s you %d experience for the death of %N.\n", killer, af ? "cost" : "gained", expAmount, victim);
    }
    if(holidayExp > 0)
        printColor("%s You %s an extra ^y%d^x experience!\n", holidayStr.c_str(), af ? "lost" : "gained", holidayExp);

    adjustFactionStanding(victim->factions);
    adjustAlignment(victim);
    updateMobKills(victim);
    statistics.experience(expAmount, victim->getName());

    addExperience(expAmount + holidayExp);

    if(victim->flagIsSet(M_WILL_BE_LOGGED))
        logn("log.mdeath", "%s was killed by %s, for %d experience.\n", victim->getCName(), getCName(), expAmount);

    for(Monster* pet : pets) {
        pet->clearEnemy(victim);
    }
}

//********************************************************************
//                      gainExperience
//********************************************************************

void Monster::gainExperience(Monster* victim, Creature* killer, int expAmount, bool groupExp) {
    Creature* master = getMaster();
    if(!master || !isPet())
        return;
    bool af = gConfig->isAprilFools();

    int holidayExp = 0;
    adjustExperience(victim, expAmount, holidayExp);

    master->printColor("Your %s %s you ^y%d^x experience for the death of %N.\n", this->getCName(), af ? "cost" : "earned", expAmount, victim);

    std::string holidayStr = isHoliday();
    if(holidayExp > 0)
        master->printColor("%s You %s an extra ^y%d^x experience!\n", holidayStr.c_str(), af ? "lost" : "gained", holidayExp);

    master->addExperience(expAmount + holidayExp);
    clearEnemy(victim);
}


//********************************************************************
//                      checkDie
//********************************************************************
// wrapper for checkDie
// Return: true if they died, false if they didn't

bool Creature::checkDie(Creature *killer) {
    bool freeTarget=true;
    return(checkDie(killer, freeTarget));
}

//********************************************************************
//                      checkDie
//********************************************************************
// Parameters:  <killer> The creature attacking
// If the victim has 0 hp or less, kill them off
// Return: true if they died, false if they didn't

bool Creature::checkDie(Creature *killer, bool &freeTarget) {
    if(hp.getCur() < 1) {
        die(killer, freeTarget);
        return(true);
    }
    return(false);
}

//********************************************************************
//                      checkDieRobJail
//********************************************************************
// wrapper for checkDieRobJail

int Creature::checkDieRobJail(Monster *killer) {
    bool freeTarget=true;
    return(checkDieRobJail(killer, freeTarget));
}

//********************************************************************
//                      checkDieRobJail
//********************************************************************
// Parameters:  <killer> The creature attacking
// Similar to checkDie, but this function will see if the attacker
// is a mugger, or a jailer and handle appropriately.
// Return Value:  1 <Death>
//                2 <Unconscious/Jail/Mug>
//                0 <Nothing>

int Creature::checkDieRobJail(Monster *killer, bool &freeTarget) {
    BaseRoom* room = getRoomParent();
    Player  *pVictim = getAsPlayer();
    ASSERTLOG(killer->isMonster());

    // Less than 1 hp & not police/greedy monster or it's monster, then die
    if(hp.getCur() < 1
        && ((!killer->flagIsSet(M_POLICE) && ! killer->flagIsSet(M_GREEDY)) || isMonster() )
    ) {
        die(killer, freeTarget);
        return(1);
    }
    // Police/Greedy & less than 1/10th hp & pVictim
    else if((killer->flagIsSet(M_GREEDY) || killer->flagIsSet(M_POLICE)) &&
        pVictim && pVictim->hp.getCur() < pVictim->hp.getMax()/10)
    {
        freeTarget = false;
        pVictim->printColor("^r%M knocks you unconscious.\n",killer);
        pVictim->knockUnconscious(39);
        broadcast(pVictim->getSock(), room, "%M knocked %N unconscious.", killer, pVictim);
        pVictim->hp.setCur( pVictim->hp.getMax()/20);
        pVictim->clearAsEnemy();
        killer->clearEnemy(pVictim);
        if(killer->flagIsSet(M_POLICE)) {
            broadcast(pVictim->getSock(), room, "%M picks %N up and hauls %s off.",killer, pVictim, pVictim->himHer());
            if(!killer->jail.id && !pVictim->isStaff())
                broadcast("### Unfortunately, %N was hauled off to jail by %N.",pVictim, killer);
            killer->toJail(pVictim);
            return(2);
        }
        if(killer->flagIsSet(M_GREEDY)) {
            broadcast(pVictim->getSock(), room, "%M rummages through %N's inventory.", killer, pVictim);
            broadcast("### Unfortunately, %N was mugged by %N.", pVictim, killer);
            killer->grabCoins(pVictim);
            return(2);
        }
        return(2);
    }

    freeTarget = false;
    return(0);
}


//********************************************************************
//                      die
//********************************************************************
// Parameters:  <killer> The creature attacking
// This function is called when a player dies to something other then a
// player or a monster. -- TC

void Player::die(DeathType dt) {
    std::string death = "";
    Player* killer=nullptr;
    Object* statue=nullptr;
    char    deathStr[2048];
    unsigned long oldxp=0;
    int     n=0;
    unsigned short oldlvl=0;
    bool killedByPlayer = dt == POISON_PLAYER || dt == CREEPING_DOOM;

    deathtype = DT_NONE;
    statistics.die();

    if(isStaff()) {
        printColor("^r*** You just died ***\n");
        broadcast(::isStaff, "^G### Sadly, %s just died.", getCName());

        clearAsEnemy();

        hp.restore();
        mp.restore();
        return;
    }

    // Any time a level 7+ this dies, they are backed up.
    if(level >= 7)
        save(true, LoadType::LS_BACKUP);
    if(!killedByPlayer) {
        setFlag(P_KILLED_BY_MOB);
        if(level >= 13) {
            lasttime[LT_MOBDEATH].ltime = time(nullptr);
            lasttime[LT_MOBDEATH].interval = 60*60*24L;
            setFlag(P_NO_SUICIDE);
        }
    }

    switch(dt) {
    case POISON_PLAYER:
    case CREEPING_DOOM:
        // these are player-caused death types
        switch(dt) {
        case POISON_PLAYER:
            sprintf(deathStr, "### Sadly, %s was poisoned to death.", getCName());
            logn("log.death", "%s was poisoned to death by %s.\n", getCName(), poisonedBy.c_str());
            break;
        case CREEPING_DOOM:
            removeEffect("creeping-doom", false);
            sprintf(deathStr, "### Sadly, %s was killed by cursed spiders.", getCName());
            logn("log.death", "%s was poisoned to death by %s.\n", getCName(), poisonedBy.c_str());
            break;
        default:
            break;
        }

        if(!poisonedBy.empty()) {
            switch(dt) {
            case POISON_PLAYER:
                sprintf(deathStr, "### Sadly, %s was poisoned to death by %s.", getCName(), poisonedBy.c_str());
                break;
            case CREEPING_DOOM:
                sprintf(deathStr, "### Sadly, %s was killed by %s's cursed spiders.", getCName(), poisonedBy.c_str());
                break;
            default:
                break;
            }

            killer = gServer->findPlayer(poisonedBy.c_str());
            if(killer)
                getPkilled(killer, false, false);
        }

        break;
    case POISON_MONSTER:
        sprintf(deathStr, "### Sadly, %s was poisoned to death by %s.", getCName(), poisonedBy.c_str());
        logn("log.death", "%s was poisoned to death by %s.\n", getCName(), poisonedBy.c_str());
        death = "poison";
        break;
    case POISON_GENERAL:
        sprintf(deathStr, "### Sadly, %s was poisoned to death.", getCName());
        logn("log.death", "%s was poisoned to death.\n", getCName());
        death = "poison";
        break;
    case FALL:
        sprintf(deathStr, "### Sadly, %s fell to %s death.", getCName(), hisHer());
        logn("log.death", "%s was killed by a fall.\n", getCName());
        death = "a fall";
        break;
    case PETRIFIED:
        sprintf(deathStr, "### Sadly, %s was turned to stone.\n### %s statue crumbles and breaks.",
                getCName(), upHisHer());

        // all inventory, equipment, and gold are destroyed
        lose_all(this, true, "petrification");
        coins.set(0, GOLD);
        printColor("^rAll your possessions were lost!\n");

        if(level >= 10) {
            if(loadObject(STATUE_OBJ, &statue)) {
                statue->setName("broken statue of " + getName());
                statue->description = getName();
                statue->description += " is forever frozen in stone.";
                strncpy(statue->key[0], "broken", 20);
                strncpy(statue->key[1], "statue", 20);
                strncpy(statue->key[2], getCName(), 20);

                statue->setWeight(100 + getWeight());
                statue->setBulk(50);
                statue->setFlag(O_NO_BREAK);

                statue->addToRoom(getRoomParent());
            }
        }

        logn("log.death", "%s died from petrification.\n", getCName());
        death = "petrification";

        removeEffect("petrification", false);

        break;
    case DISEASE:
        sprintf(deathStr, "### Sadly, %s died from disease.", getCName());
        logn("log.death", "%s was killed by disease.\n", getCName());
        death = "disease";
        break;
    case WOUNDED:
        logn("log.death", "%s was killed by festering wounds.\n", getCName());
        sprintf(deathStr, "### Sadly, %s has bled to death.", getCName());
        death = "festering wounds";
        break;
    case ELVEN_ARCHERS:
        logn("log.death", "%s was shot to death by elven archers.\n", getCName());
        sprintf(deathStr, "### Sadly, %s was shot to death by elven archers.", getCName());
        death = "elven archers";
        break;
    case DEADLY_MOSS:
        logn("log.death", "%s was choked to death by deadly underdark moss.\n", getCName());
        sprintf(deathStr, "### Sadly, %s was choked to death by deadly underdark moss.", getCName());
        death = "deadly underdark moss";
    case PIERCER:
        sprintf(deathStr, "### Sadly, %s was impaled to death by a piercer.", getCName());
        logn("log.death", "%s was killed by a piercer.\n", getCName());
        death = "a piercer";
        break;
    case SMOTHER:
        sprintf(deathStr, "### Sadly, %s was engulfed by the earth.", getCName());
        logn("log.death", "%s was killed by an earth damage room.\n", getCName());
        death = "engulfing earth";
        break;
    case FROZE:
        sprintf(deathStr, "### Sadly, %s froze to death.", getCName());
        logn("log.death", "%s was killed by an air damage room.\n", getCName());
        death = "hypothermia";
        break;
    case LIGHTNING:
        sprintf(deathStr, "### Sadly, %s was blasted to bits by electricity.", getCName());
        logn("log.death", "%s was killed by an electricity damage room.\n", getCName());
        death = "electricity";
        break;
    case WINDBATTERED:
        sprintf(deathStr, "### Sadly, %s was ripped apart by the wind.", getCName());
        logn("log.death", "%s was killed by an air damage room.\n", getCName());
        death = "battering winds";
        break;
    case BURNED:
        sprintf(deathStr, "### Sadly, %s was burned alive.", getCName());
        logn("log.death", "%s was killed by a fire damage room or effect.\n", getCName());
        death = "a raging fire";
        break;
    case THORNS:
        sprintf(deathStr, "### Sadly, %s was killed by a wall of thorns.", getCName());
        logn("log.death", "%s was killed by a wall of thorns.\n", getCName());
        death = "a wall of thorns";
        break;
    case DROWNED:
        sprintf(deathStr, "### Sadly, %s drowned.", getCName());
        logn("log.death", "%s was killed by drowning.\n", getCName());
        death = "drowning";
        break;
    case DRAINED:
        sprintf(deathStr, "### Sadly, %s's life force was completely drained.", getCName());
        logn("log.death", "%s was killed by a pharm room.\n", getCName());
        death = "life-drain";
        break;
    case ZAPPED:
        sprintf(deathStr, "### Sadly, %s was zapped to death.", getCName());
        logn("log.death", "%s was killed by a combo lock.\n", getCName());
        death = "a fatal shock";
        break;
    case SHOCKED:
        sprintf(deathStr, "### Sadly, %s was shocked to death.", getCName());
        logn("log.death", "%s was killed by shocking.\n", getCName());
        death = "a fatal shock";
        break;
    case PIT:
        sprintf(deathStr, "### Sadly, %s fell into a pit and died.", getCName());
        logn("log.death", "%s was killed by a pit trap.\n", getCName());
        death = "a fall";
        break;
    case BLOCK:
        sprintf(deathStr, "### Sadly, %s was crushed to death by a giant stone block.", getCName());
        logn("log.death", "%s was killed by a stone block trap.\n", getCName());
        death = "a giant stone block";
        break;
    case DART:
        sprintf(deathStr, "### Sadly, %s was killed by a poisoned dart.", getCName());
        logn("log.death", "%s was killed by a poison dart trap.\n", getCName());
        death = "a poisoned dart";
        break;
    case ARROW:
        sprintf(deathStr, "### Sadly, %s was killed by a flight of arrows.", getCName());
        logn("log.death", "%s was killed by an arrow trap.\n", getCName());
        death = "a flight of arrows";
        break;
    case SPIKED_PIT:
        sprintf(deathStr, "### Sadly, %s fell into a pit and was impaled by spikes.", getCName());
        logn("log.death", "%s was killed by a spiked pit trap.\n", getCName());
        death = "a fall";
        break;
    case FIRE_TRAP:
        sprintf(deathStr, "### Sadly, %s was engulfed by flames and died.", getCName());
        logn("log.death", "%s was killed by a fire trap.\n", getCName());
        death = "a raging fire";
        break;
    case FROST:
        sprintf(deathStr, "### Sadly, %s was frozen alive, and died.", getCName());
        logn("log.death", "%s was killed by a frost trap.\n", getCName());
        death = "hypothermia";
        break;
    case ELECTRICITY:
        sprintf(deathStr, "### Sadly, %s was killed by electrocution.", getCName());
        logn("log.death", "%s was killed by an electricity trap.\n", getCName());
        death = "electricity";
        break;
    case ACID:
        sprintf(deathStr, "### Sadly, %s was dissolved by acid.", getCName());
        logn("log.death", "%s was killed by an acid trap.\n", getCName());
        death = "acid";
        break;
    case ROCKS:
        sprintf(deathStr, "### Sadly, %s was crushed to death in a rockslide.", getCName());
        logn("log.death", "%s was killed by a rockslide trap.\n", getCName());
        death = "a rockslide";
        break;
    case ICICLE_TRAP:
        sprintf(deathStr, "### Sadly, %s was impaled by a giant icicle.", getCName());
        logn("log.death", "%s was killed by a falling icicle trap.\n", getCName());
        death = "a giant icicle";
        break;
    case SPEAR:
        sprintf(deathStr, "### Sadly, %s was killed by a giant spear.", getCName());
        logn("log.death", "%s was killed by a spear trap.\n", getCName());
        death = "a spear";
        break;
    case CROSSBOW_TRAP:
        sprintf(deathStr, "### Sadly, %s was killed by a crossbow trap.", getCName());
        logn("log.death", "%s was killed by a crossbow trap.\n", getCName());
        death = "a crossbow bolt";
        break;
    case VINES:
        sprintf(deathStr, "### Sadly, %s was ripped apart by crawling vines.", getCName());
        logn("log.death", "%s was killed by deadly vines.\n", getCName());
        death = "deadly vines";
        break;
    case COLDWATER:
        sprintf(deathStr, "### Sadly, %s died from hypothermia.", getCName());
        logn("log.death", "%s was killed from hypothermia.\n", getCName());
        death = "hypothermia";
        break;
    case EXPLODED:
        sprintf(deathStr, "### Sadly, %s exploded.", getCName());
        logn("log.death", "%s was killed by exploding.\n", getCName());
        death = "self-combustion";
        break;
    case SPLAT:
        sprintf(deathStr, "### Sadly, %s tumbled to %s death. (SPLAT!)", getCName(), hisHer());
        logn("log.death", "%s tumbled to %s death.\n", getCName(), hisHer());
        death = "a fall";
        break;
    case BOLTS:
        sprintf(deathStr, "### Sadly, %s was blasted to death by energy bolts.", getCName());
        logn("log.death", "%s was killed by energy bolts.\n", getCName());
        death = "energy bolts";
        break;
    case BONES:
        sprintf(deathStr, "### Sadly, %s was crushed to death under an avalanche of bones.", getCName());
        logn("log.death", "%s was killed by a bone avalanche.\n", getCName());
        death = "an avalanche of bones";
        break;
    case EXPLOSION:
        sprintf(deathStr, "### Sadly, %s was vaporized in a magical explosion.", getCName());
        logn("log.death", "%s was killed by a magical explosion.\n", getCName());
        death = "a magical explosion";
        break;
    case SUNLIGHT:
        sprintf(deathStr, "### Sadly, %s was disintegrated by sunlight.", getCName());
        logn("log.death", "%s was disintegrated by sunlight.\n", getCName());
        death = "sunlight";
        break;
    default:
        sprintf(deathStr, "### Sadly, %s died.", getCName());
        logn("log.death", "%s was killed by %s.\n", getCName(), getCName());
        death = "misfortune";
        break;
    }
    //if(level > 2)
    //  broadcast(deathStr);
    //else {
    //  broadcast(::isWatcher, "^C%s", deathStr);
    //  print(deathStr);
    //}
    broadcast(deathStr);


    oldxp = experience;
    oldlvl = level;

    if(!killedByPlayer) {
        cureDisease();
        removeCurse();
    }
    if(!killedByPlayer || dt == POISON_PLAYER)
        curePoison();

    // only drop all if killed by player
    dropEquipment(killedByPlayer && (!killer || !killer->isStaff()), killer ? killer->getSock() : nullptr);

    /*
    if(ready[WIELD - 1] && !ready[WIELD - 1]->flagIsSet(O_CURSED))
        unequip(WIELD);

    if( ready[HELD - 1] &&
        ready[HELD - 1]->getWearflag() == WIELD &&
        !ready[HELD - 1]->flagIsSet(O_CURSED)
    ) {
        Object* held = unequip(HELD, UNEQUIP_NOTHING);
        if(held) {
            held->addToRoom(getRoom());
            held->tempPerm();
        }
    }

    Object* temp=0;
    for(i=0; i<MAXWEAR; i++) {
        if(ready[i] && !ready[i]->flagIsSet(O_CURSED)) {
            if(!killedByPlayer) {
                // i is wearloc-1 so add 1
                unequip(i+1);
            }
            else {
                temp = unequip(i+1, UNEQUIP_NOTHING);
                if(temp)
                    temp->addToRoom(getRoom());
            }
        }
    }
    */
    checkDarkness();
    computeAC();
    computeAttackPower();
    courageous();


    int xploss = 0;
    if(!killedByPlayer) {
        if(level >= 5)
            addEffect("death-sickness");

        if(level < 10) {
            // Under level 10, 10% exp loss
            xploss = (int)((float)experience / 10.0);
        } else {
            // Level 10 and over, 2% exp loss with a minimum of 10k
            xploss = MAX<long>((long)( (float)experience * 0.02), 10000);
            print("You have lost %ld experience.\n", (long)xploss);
        }
        subExperience((long)xploss);

    }


    n = level - exp_to_lev(experience);

    if(n > 1) {
        if(level < (MAXALVL+2))
            experience = Config::expNeeded(level-2);
        else
            experience = (long)((Config::expNeeded(MAXALVL)*(level-2)));
    }


    if(level >= 7)
        logn("log.death", "L:%dXP:%dE:%luA:%luF:%luW:%luE:%luC:%lu\n",
             oldlvl, oldxp,
             getRealm(EARTH), getRealm(WIND), getRealm(FIRE), getRealm(WATER),
             getRealm(ELEC), getRealm(COLD));



    if(!killedByPlayer) {
        n = exp_to_lev(experience);
        while(level > n)
            downLevel();

        negativeLevels = 0;
    }

    hp.restore();
    mp.restore();

    unhide();

    // more info for staff
    broadcast(::isCt, "^rOld Exp: %u Lost Exp: %d, Old Lvl: %u, Lvl: %u, Room: %s",
        oldxp, xploss, oldlvl, level, getRoomParent()->fullName().c_str());

    if(isHardcore()) {
        hardcoreDeath(this);
    // if you die in jail, you stay in jail
    } else if(!inJail()) {
        BaseRoom *newRoom = getLimboRoom().loadRoom(this);
        if(newRoom) {
            deleteFromRoom();
            addToRoom(newRoom);
            doPetFollow();
        }
    }
}

//********************************************************************
//                      clearAsPetEnemy
//********************************************************************

void Creature::clearAsPetEnemy() {
    for(Monster* mons : getRoomParent()->monsters) {
        if(mons->isPet())
            mons->clearEnemy(this);
    }
}

//********************************************************************
//                      cleanFollow
//********************************************************************

void Monster::cleanFollow(Creature *killer) {
    if(isMonster() && getMaster()) {
        Player* player = getMaster()->getAsPlayer();

        // This should fix the bug with having a dm's pet killed while possessing a mob
        if(flagIsSet(M_DM_FOLLOW)) {
            player->setAlias(nullptr);
            player->clearFlag(P_ALIASING);
        }
        if(getMaster() != killer) {
            player->printColor("^r%M's body has been destroyed.\n", this);
            if(killer)
                broadcast(killer->getSock(), getRoomParent(), "%M was killed by %N.", this, killer);
        }
        removeFromGroup(false);
        player->delPet(this);
    }
}

//*********************************************************************
//                      dropWeapons
//*********************************************************************
// handle dropping of weapons when you flee or die
// Return: true if any weapons were dropped, false if none were dropped

bool Player::dropWeapons() {
    BaseRoom* room = getRoomParent();
    Object* weapon;
    bool    dropMain=false, dropSec=false, fumbleMain=false, fumbleSec=false;
    bool    dropDestroy = room->isDropDestroy();
    bool    jump=false;

    if(isStaff())
        return(false);

    // check main weapons
    weapon = ready[WIELD-1];
    if(weapon && !weapon->flagIsSet(O_CURSED)) {
        if(weapon->flagIsSet(O_NO_DROP) || weapon->flagIsSet(O_STARTING) || dropDestroy) {
            // Add to inventory
            unequip(WIELD);
            fumbleMain = true;
        } else {
            dropMain = true;
            // Unequip, and add to the room
            weapon = unequip(WIELD, UNEQUIP_NOTHING, false);
            finishDropObject(weapon, room, this);
        }
    }

    // check secondary weapons
    weapon = ready[HELD-1];
    if(weapon && weapon->getWearflag() == WIELD) {
        if(weapon->flagIsSet(O_CURSED)) {
            if(!ready[WIELD-1]) {
                ready[WIELD-1] = ready[HELD-1];
                ready[HELD-1] = nullptr;
                jump = true;
            }
        } else {
            if(weapon->flagIsSet(O_NO_DROP) || weapon->flagIsSet(O_STARTING) || dropDestroy) {
                fumbleSec = true;
                // Add to inventory
                unequip(HELD);
            } else {
                dropSec = true;
                // Unequip and add to the room
                weapon = unequip(HELD, UNEQUIP_NOTHING, false);
                finishDropObject(weapon, room, this);
            }
        }
    }

    if(dropMain || dropSec || fumbleMain || fumbleSec) {
        // only fumble
        if(!dropMain && !dropSec)
            if(fumbleMain && fumbleSec)
                print("You fumble your weapons.\n");
            else
                print("You fumble your %sweapon.\n", fumbleSec ? "secondary " : "");

        // only drop
        else if(!fumbleMain && !fumbleSec)
            if(dropMain && dropSec)
                print("You drop your weapons.\n");
            else
                print("You drop your %sweapon.\n", dropSec ? "secondary " : "");

        // one of each
        else
            print("You drop your %s weapon and fumble your %s weapon.\n",
                dropMain ? "main" : "secondary", fumbleMain ? "main" : "secondary");

        if(jump)
            print("%s%s jumped to your primary hand! It's cursed!\n",
                !ready[WIELD-1]->flagIsSet(O_NO_PREFIX) ? "The " : "", ready[WIELD-1]->getCName());

        // checkDarkness(), computeAttackPower(), computeAC() should be handled outside this function
        return(true);
    }

    return(false);
}

//********************************************************************
