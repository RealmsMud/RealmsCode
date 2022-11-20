/*
 * command2.cpp
 *   Command handling/parsing routines.
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

#include <cstdio>                      // for sprintf
#include <cstring>                     // for strcat, strlen, strncmp, strcpy
#include <set>                         // for operator==, _Rb_tree_const_ite...
#include <string>                      // for allocator, string, operator+
#include <type_traits>                 // for enable_if<>::type

#include "area.hpp"                    // for Area, MapMarker
#include "catRef.hpp"                  // for CatRef
#include "cmd.hpp"                     // for cmd
#include "commands.hpp"                // for finishDropObject, cmdPrepareFo...
#include "config.hpp"                  // for Config, gConfig
#include "damage.hpp"                  // for Damage
#include "flags.hpp"                   // for X_CLOSED, O_BROKEN_BY_CMD, O_N...
#include "global.hpp"                  // for CreatureClass, CreatureClass::...
#include "location.hpp"                // for Location
#include "money.hpp"                   // for GOLD, Money
#include "move.hpp"                    // for getRoom
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/container.hpp"    // for ObjectSet, PlayerSet, Container
#include "mudObjects/creatures.hpp"    // for Creature, CHECK_DIE, ATTACK_BLOCK
#include "mudObjects/exits.hpp"        // for Exit, getDir, getDirName, NoDi...
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/mudObject.hpp"    // for MudObject
#include "mudObjects/objects.hpp"      // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "mud.hpp"                     // for LT_TRAFFIC
#include "paths.hpp"                   // for Sign
#include "proto.hpp"                   // for broadcast, findExit, bonus
#include "random.hpp"                  // for Random
#include "socket.hpp"                  // for Socket
#include "statistics.hpp"              // for Statistics
#include "stats.hpp"                   // for Stat
#include "xml.hpp"                     // for loadRoom

#define STONE_SCROLL_INDEX      10

//*********************************************************************
//                      stoneScroll
//*********************************************************************

int stoneScroll(const std::shared_ptr<Player>& player, cmd* cmnd) {
    char    filename[80];
    sprintf(filename, "%s/stone_scroll", Path::Sign.c_str());

    if(!strncmp(cmnd->str[2], "class", strlen(cmnd->str[2])))
        strcat(filename, "_class");
    else if(!strncmp(cmnd->str[2], "race", strlen(cmnd->str[2])))
        strcat(filename, "_race");
    else if(!strncmp(cmnd->str[2], "deity", strlen(cmnd->str[2])))
        strcat(filename, "_deity");
    else if(!strncmp(cmnd->str[2], "breakdown", strlen(cmnd->str[2])))
        strcat(filename, "_breakdown");
    strcat(filename, ".txt");
    player->getSock()->viewFile(filename);
    return(0);
}

//*********************************************************************
//                      lookAtExit
//*********************************************************************

bool doScout(std::shared_ptr<Player> player, std::shared_ptr<Exit> exit);

void lookAtExit(const std::shared_ptr<Player>& player, const std::shared_ptr<Exit>& exit) {
    // blindness is already handled
    
    player->printColor("You look at the %s^x exit.\n", exit->getCName());

    if(!exit->getDescription().empty())
        player->printColor("%s\n", exit->getDescription().c_str());
    // the owner of the portal gets more information
    if(exit->flagIsSet(X_PORTAL) && exit->getPassPhrase() == player->getName())
        player->print("%s %s may pass through it.\n", intToText(exit->getKey(), true).c_str(), exit->getKey() == 1 ? "person" : "people");
    if(getDir(exit->getCName()) == NoDirection && exit->getDirection() != NoDirection)
        player->print("It leads %s.\n", getDirName(exit->getDirection()).c_str());
    
    if(exit->flagIsSet(X_CLOSED))
        player->print("It is closed.\n");
    if(exit->flagIsSet(X_LOCKED))
        player->print("It is locked.\n");
    if(exit->isWall("wall-of-fire"))
        player->print("It is blocked by a wall of fire.\n");
    if(exit->isWall("wall-of-force"))
        player->print("It is blocked by a wall of force.\n");
    if(exit->isWall("wall-of-thorns"))
        player->print("It is blocked by a wall of thorns.\n");

    const std::shared_ptr<Monster>  guard = player->getRoomParent()->getGuardingExit(exit, player);
    if(guard && !player->checkStaff("%M %s.\n", guard.get(), guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you look there" : "won't let you look there"))
        return;

    if(exit->flagIsSet(X_CAN_LOOK) || exit->flagIsSet(X_LOOK_ONLY)) {
        if( player->getRoomParent()->isEffected("dense-fog") &&
            !player->checkStaff("This room is filled with a dense fog.\nYou cannot look to the %s^x effectively.\n", exit->getCName())
        )
            return;

        doScout(player, exit);
        return;
    }
}

//*********************************************************************
//                      cmdTraffic
//*********************************************************************
// This command lets a player check for basic information about how much
// mob traffic comes in a given room.

int cmdTraffic(const std::shared_ptr<Player>& player, cmd* cmnd) {

    int     basechance=0, bonus=0, chance=0,outputChoice=0;
    long    t=0, i=0;
    bool    success=false;
    std::ostringstream oStr;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->flagIsSet(P_STUNNED)) {
        *player << "You can't check the room right now.\nYou can't move.\n";
        return(0);
    }
    if(player->inCombat()) {
        *player << "You can't check the room right now.\nYou're too busy trying not to die!\n";
        return(0);
    }
    if (player->isEffected("confusion")) {
        *player << "You find the idea of doing that entirely too confusing right now.\n";
        return(0);
    }
    if (player->isEffected("feeblemind")) {
        *player << "You're too busy drooling all over yourself to do that right now.\n";
        return(0);
    }
    if (!player->canSeeRoom(player->getRoomParent(),true))
        return(0);

    if(player->getRoomParent()->isEffected("dense-fog") &&
        !player->checkStaff("This room is filled with a dense fog.\nYou can't really make out any room traffic.\n")
    )
        return(0);

    if(player->getRoomParent()->flagIsSet(R_ETHEREAL_PLANE) &&
        !player->checkStaff("Everything in the room seems to be constantly in motion.\nIt's impossible to tell how often this place is visited.\n")
    )
        return(0);

    if (!player->isStaff() && player->getRoomParent()->flagIsSet(R_NO_CHECK_TRAFFIC)) {
        *player << "You are unable to tell how often this place is disturbed.\n";
        return(0);
    }

    i = player->lasttime[LT_TRAFFIC].ltime + player->lasttime[LT_TRAFFIC].interval;
    t = time(nullptr);

    if(!player->isStaff() && t < i) {
        player->pleaseWait(i - t);
        return(0);
    }

    // Since we won't need a skill for this, the base chance is level x 3
    basechance = player->getLevel() * 3;

    // Do bonuses now, starting with modifications for class and possible environment
    switch(player->getClass()) {
    // Rangers and druids are always better at this
    case CreatureClass::RANGER:
    case CreatureClass::DRUID: 
        bonus += (player->getLevel()*5)/2; // level * 2.5
        if (player->getRoomParent()->flagIsSet(R_FOREST_OR_JUNGLE))
            bonus += (player->getLevel()*7)/2; // level * 3.5
    break;
    default:
    break;
    }

    // Do bonuses for race and possible environment
    switch(player->getRace()) {
    case ELF:
        if (player->getRoomParent()->flagIsSet(R_FOREST_OR_JUNGLE))
            bonus += player->getLevel()*3;
         else
            bonus += player->getLevel(); // Elves are generally more observant by nature
    break;
    case HALFELF:
        if (player->getRoomParent()->flagIsSet(R_FOREST_OR_JUNGLE))
            bonus += (player->getLevel()*5)/2;
         else
            bonus += player->getLevel()/2; // Half-Elves, like elves, are also more observant by nature, but less so
    break;
    case DWARF:
    case DUERGAR:
        if (player->getRoomParent()->flagIsSet(R_UNDERGROUND))
            bonus += (player->getLevel()*7)/2; // Nobody can notice signs of traffic underground better than a dwarf
    break;
    case MINOTAUR:
    case GNOME:
    case KOBOLD:
        if (player->getRoomParent()->flagIsSet(R_UNDERGROUND))
            bonus += player->getLevel()*3;
    break;
    case DARKELF:
        if (player->getRoomParent()->flagIsSet(R_UNDERGROUND))
            bonus += player->getLevel()*3;
         else
            bonus += player->getLevel(); // Dark Elves are also quite observant by nature (paranoia)
    break;
    default:
    break;
    }

    // Modify bonus for misc other things here..i.e. intelligence, spell effects, etc..

    // Exceptional intelligence gets a bonus, sub-par intelligence gets a penalty, average intelligence gets no bonus
    if (player->intelligence.getCur() >= 150)
        bonus += player->intelligence.getCur()/10;
    if (player->intelligence.getCur() <= 80)
        bonus -= player->intelligence.getCur()/10;

    if (player->isEffected("insight"))
        bonus += 20;

    if (player->isEffected("true-sight"))
        bonus = (bonus*3)/2; // being under true-sight effect increases bonus by +50%
    
    //TODO: Whenever we have area flags, like aflag for city, swamp, etc..other environmental flags,
    //we can add more racial/class bonuses and penalties..i.e. thieves/assassins/rogues are better
    //able to figure out these things in urban areas, and trolls are better in swamps. 
    //Plenty of other possibilities here

    //Always at least a 5% chance to fail
    chance = std::max(1,(std::min(95,basechance+bonus)));

    // Drunk people suck at examining their surroundings
    if (player->isEffected("drunkenness"))
        chance = Random::get(1,10); 

    if (!player->isStaff()) {
        player->lasttime[LT_TRAFFIC].ltime = t;
        player->lasttime[LT_TRAFFIC].interval = 15L;
    }

    *player << "You examine the area for foot traffic or disturbances.\n";
    if(player->isStaff() && player->flagIsSet(P_DM_INVIS))
        broadcast(isStaff, player->getSock(), player->getRoomParent(), "%M examines the area for foot traffic or disturbances.", player.get());
    else
        broadcast(player->getSock(), player->getParent(), "%M examines the area for foot traffic or disturbances.", player.get());

    success = (Random::get(1,100) <= chance);

    if (success || player->isStaff()) {
        outputChoice = Random::get(1,2);

        WanderInfo* wander = player->getRoomParent()->getWanderInfo();
        short roomTraffic = wander->getTraffic();
        long randomCount = wander->getRandomCount();

        int traffic = (int)roomTraffic * (int)randomCount;

        oStr << "^g";

        if(player->isStaff() || player->flagIsSet(P_PTESTER)) {
            oStr << "Chance: " << chance << "%\n";
            oStr << "roomTraffic: " << roomTraffic << "%\n";
            oStr << "RandomCount = " << randomCount << "\n";
            oStr << "Traffic Weight = " << traffic << "\n";
        }
        if (traffic == 0) 
            oStr << (outputChoice == 1 ? "It looks like nothing has ever disturbed this place.":"You don't see any history of traffic whatsoever.");
        else if (traffic < 30)
            oStr << (outputChoice == 1 ? "Nobody has disturbed this place in a VERY long time.":"Someone might have disturbed this place long ago.");
        else if (traffic < 50)
            oStr << (outputChoice == 1 ? "This place is rarely disturbed.":"It looks like barely anything ever disturbs this place.");
        else if (traffic < 100)
            oStr << (outputChoice == 1 ? "Foot traffic here looks rare, but it's definitely possible.":"It's not often anyone disturbs this place, but somebody definitely has.");
        else if (traffic < 150)
            oStr << (outputChoice == 1 ? "The disturbances here look relatively uncommon, but steady.":"It looks like somebody might have been here recently.");
        else if (traffic < 200)
            oStr << (outputChoice == 1 ? "This area looks to be disturbed on a somewhat regular basis.":"It looks like this area is disturbed a lot.");
        else if (traffic < 250)
            oStr << (outputChoice == 1 ? "Traffic looks to be pretty common here.":"This place can get crowded sometimes.");
        else if (traffic < 300)
            oStr << (outputChoice == 1 ? "It looks like there's always somebody around here.":"This area looks like it's always busy.");
        else if (traffic < 400)
            oStr << (outputChoice == 1 ? "This area looks to be pretty crowded.":"The foot traffic here can be nuts.");
        else if (traffic < 500)
            oStr << (outputChoice == 1 ? "This area has a ton of foot traffic.":"It's hard not to be bumped into here.");
        else if (traffic < 600)
            oStr << (outputChoice == 1 ? "It's a wonder when somebody did NOT disturb this place.":"You can see somebody around here all the time.");
        else if (traffic < 700)
            oStr << (outputChoice == 1 ? "The foot traffic here is pretty stifling.":"You're practically in the middle of a crowd.");
        else if (traffic < 800)
            oStr << (outputChoice == 1 ? "The foot traffic here is unbelievably crazy.":"There's rarely anybody NOT here.");
        else if (traffic < 900)
            oStr << (outputChoice == 1 ? "You can hardly find anywhere to stand here without being in the way.":"It's so busy here that there's almost nowhere to stand.");
        else if (traffic < 980)
            oStr << (outputChoice == 1 ? "It's so crowded here that you can barely move.":"It's so busy that it's hard to move around.");
        else if (traffic <=1000)
            oStr << (outputChoice == 1 ? "The foot traffic here is absolutely and unbelievably insane.":"It's so busy here that you could get stampeded.");
        else 
            oStr << (outputChoice == 1 ? "Something is wrong. The room's traffic is negative.":"Well this is weird....the room traffic includes the 4th dimension.");

        *player << ColorOn << oStr.str() << "\n" << ColorOff; 
    }
    else
    {
        *player << "You were unable to determine anything concrete.\n";
        broadcast(player->getSock(), player->getParent(), "%M was unable to determine anything concrete.", player.get());
    }

    return(0);
}

//*********************************************************************
//                      cmdLook
//*********************************************************************

int cmdLook(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<MudObject> target=nullptr;
    int flags = player->displayFlags();

    if(!player->ableToDoCommand())
        return(0);
    if(player->isBlind()) {
        player->printColor("^CYou're blind!\n");
        return(0);
    }

    if(!cmnd || cmnd->num < 2) {
        display_rom(player);
        return(0);
    }

    target = player->findTarget(FIND_OBJ_INVENTORY | FIND_OBJ_EQUIPMENT | FIND_OBJ_ROOM |
        FIND_MON_ROOM | FIND_PLY_ROOM | FIND_EXIT, flags, cmnd->str[1], cmnd->val[1]);

    if(target) {
        if(target->isCreature()) {
            player->displayCreature(target->getAsCreature());
        } else if(target->isObject()) {
            auto obj = target->getAsConstObject();
            if(cmnd->num == 3 && obj->info.id == STONE_SCROLL_INDEX && obj->info.isArea("misc"))
                stoneScroll(player, cmnd);
            else
                displayObject(player, obj);
        } else if(target->isExit()) {
            lookAtExit(player, target->getAsExit());
        } else {
            player->print("You get all confused looking at it.\n");
        }
    } else {
        player->print("You don't see that here.\n");
    }

    return(0);
}

//*********************************************************************
//                      cmdThrow
//*********************************************************************

bool storageProperty(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>&  object, Property **p);

int cmdThrow(const std::shared_ptr<Creature>& creature, cmd* cmnd) {
    std::shared_ptr<Object>  object=nullptr;
    std::shared_ptr<Creature> victim=nullptr;
    std::shared_ptr<Monster>  guard=nullptr;
    std::shared_ptr<Exit>   exit=nullptr;
    std::shared_ptr<BaseRoom> room=nullptr, newRoom=nullptr;
    std::shared_ptr<Player> player = creature->getAsPlayer(), pVictim=nullptr;
    Property* p=nullptr;

    if(!creature->ableToDoCommand())
        return(0);
    if(!creature->checkAttackTimer())
        return(0);
    if(creature->isBlind()) {
        creature->printColor("^CYou can't do that. You're blind!\n");
        return(0);
    }

    object = creature->findObject(creature, cmnd, 1);
    room = creature->getRoomParent();

    if(!object) {
        creature->print("You don't have that in your inventory.\n");
        return(0);
    }


    // throw checks

    if(object->flagIsSet(O_KEEP)) {
        player->printColor("%O is currently in safe keeping.\nYou must unkeep it to throw it.\n", object.get());
        return(0);
    }

    if( object->flagIsSet(O_NO_DROP) &&
        !player->checkStaff("You cannot throw that.\n"))
    {
        return(0);
    }

    for(const auto& obj : object->objects) {
        if( obj->flagIsSet(O_NO_DROP) &&
            !player->checkStaff("You cannot throw that. It contains %P.\n", obj.get()) )
        {
            return(0);
        }
    }

    if( object->getActualWeight() > creature->strength.getCur()/3 &&
        !creature->checkStaff("%O is too heavy for you to throw!\n", object.get())
    )
        return(0);

    // are they allowed to throw objects in this storage room?
    if(!storageProperty(player, object, &p))
        return(0);
    
    // done with throw checks


    exit = findExit(creature, cmnd, 2, room);

    if(!exit)
        victim = room->findCreature(creature, cmnd, 2);

    if(exit) {
        if(creature->inCombat() && !creature->checkStaff("Not while you are in combat!\n"))
            return(0);

        creature->printColor("You throw %P to the %s^x.\n", object.get(), exit->getCName());
        broadcast(creature->getSock(), room, "%M throws %P to the %s^x.",
            creature.get(), object.get(), exit->getCName());
        creature->delObj(object);

        guard = room->getGuardingExit(exit, player);

        // don't bother getting rooms if exit is closed or guarded
        bool loadExit = (!exit->flagIsSet(X_CLOSED) && !exit->flagIsSet(X_PORTAL) && !guard);
        if(loadExit)
            Move::getRoom(creature, exit, newRoom, false, &exit->target.mapmarker);

        room->wake("Loud noises disturb your sleep.", true);

        // closed, or off map, or being guarded
        if(!loadExit || (!newRoom)) {
            if(guard)
                broadcast((std::shared_ptr<Socket> )nullptr, room, "%M knocks %P to the ground.", guard.get(), object.get());
            else
                broadcast((std::shared_ptr<Socket> )nullptr, room, "%O hits the %s^x and falls to the ground.", object.get(), exit->getCName());
            finishDropObject(object, room, creature);
        } else {
            room = newRoom;

            broadcast((std::shared_ptr<Socket> )nullptr, room, "%1O comes flying into the room.", object.get());
            room->wake("Loud noises disturb your sleep.", true);
            finishDropObject(object, room, creature, false, false, true);

            if(newRoom->isAreaRoom()) {
                auto aNewRoom = newRoom->getAsAreaRoom();
                if (aNewRoom->canDelete()) {
                    if (auto area = aNewRoom->area.lock()) {
                        area->remove(aNewRoom);
                    }
                }
            }
        }

        creature->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);

    } else if(victim) {

        pVictim = victim->getAsPlayer();
        if(!creature->canAttack(victim))
            return(0);

        if(creature->vampireCharmed(pVictim) || (victim->hasCharm(creature->getCName()) && creature->pFlagIsSet(P_CHARMED))) {
            creature->print("You like %N too much to do that.\n", victim.get());
            return(0);
        }

        creature->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
        creature->smashInvis();
        creature->unhide();

        if(player)
            player->statistics.swing();
        creature->printColor("You throw %P at %N.\n", object.get(), victim.get());
        victim->printColor("%M throws %P at you.\n", creature.get(), object.get());
        broadcast(creature->getSock(), victim->getSock(), room,
            "%M throws %P at %N.", creature.get(), object.get(), victim.get());
        creature->delObj(object);

        if(!pVictim)
            victim->getAsMonster()->addEnemy(creature);

        if(pVictim && victim->isEffected("mist")) {
            pVictim->statistics.wasMissed();
            if(player)
                player->statistics.miss();
            broadcast(victim->getSock(), room, "%O passes right through %N.", object.get(), victim.get());
            victim->printColor("%O passes right through you.\n", object.get());
        } else {
            int skillLevel = (int)creature->getSkillGained("thrown");
            AttackResult result = creature->getAttackResult(victim, nullptr, NO_FUMBLE, skillLevel);

            if(pVictim && victim->flagIsSet(P_UNCONSCIOUS))
                result = ATTACK_HIT;

            if(result == ATTACK_DODGE) {
                if(pVictim)
                    pVictim->statistics.dodge();
                if(player)
                    player->statistics.miss();
                victim->printColor("You dodge out of the way.\n");
                broadcast(victim->getSock(), room, "%M dodges out of the way.", victim.get());
                creature->checkImprove("thrown", false);
            } else if(result == ATTACK_MISS || result == ATTACK_BLOCK || result == ATTACK_PARRY) {
                if(pVictim)
                    pVictim->statistics.wasMissed();
                if(player)
                    player->statistics.miss();
                victim->printColor("You knock %P to the ground.\n", object.get());
                broadcast(victim->getSock(), room, "%M knocks %P to the ground.", victim.get(), object.get());
                creature->checkImprove("thrown", false);
            } else {

                Damage damage;
                damage.set(1);
                if(object->getActualWeight() > 1)
                    damage.set(Random::get(1,6));

                victim->modifyDamage(creature, PHYSICAL, damage);
                if(pVictim)
                    pVictim->statistics.wasHit();
                if(player) {
                    player->statistics.hit();
                    player->statistics.attackDamage(damage.get(), Statistics::damageWith(player, object));
                }

                creature->printColor("You hit %N for %s%d^x damage.\n", victim.get(), creature->customColorize("*CC:DAMAGE*").c_str(), damage.get());
                victim->printColor("%M hit you%s for %s%d^x damage!\n", creature.get(),
                    victim->isBrittle() ? "r brittle body" : "", victim->customColorize("*CC:DAMAGE*").c_str(), damage.get());
                broadcastGroup(false, victim, "^M%M^x hit ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n", creature.get(),
                    victim.get(), damage.get(), victim->heShe(), victim->getStatusStr(damage.get()));

                creature->doDamage(victim, damage.get(), CHECK_DIE);
                creature->checkImprove("thrown", true);
            }

        }

        finishDropObject(object, room, creature);
        room->wake("Loud noises disturb your sleep.", true);

    } else {
        creature->printColor("Throw %P at what?\n", object.get());
    }
    return(0);
}

//*********************************************************************
//                      cmdKnock
//*********************************************************************

int cmdKnock(const std::shared_ptr<Creature>& creature, cmd* cmnd) {
    std::shared_ptr<BaseRoom> targetRoom=nullptr;
    std::shared_ptr<Exit> exit=nullptr;

    if(cmnd->num < 2) {
        creature->print("Knock on what exit?\n");
        return(0);
    }

    lowercize(cmnd->str[1], 0);
    exit = findExit(creature, cmnd);
    if(!exit) {
        creature->print("Knock on what exit?\n");
        return(0);
    }

    if(!exit->flagIsSet(X_CLOSED)) {
        creature->print("That exit is not closed!\n");
        return(0);
    }

    creature->getParent()->wake("You awaken suddenly!", true);
    creature->printColor("You knock on the %s^x.\n", exit->getCName());
    broadcast(creature->getSock(), creature->getRoomParent(), "%M knocks on the %s^x.", creature.get(), exit->getCName());

    // change the meaning of exit
    exit = exit->getReturnExit(creature->getRoomParent(), targetRoom);
    targetRoom->wake("You awaken suddenly!", true);

    if(exit)
        broadcast((std::shared_ptr<Socket> )nullptr, targetRoom, "You hear someone knocking on the %s.", exit.get());
    else
        broadcast((std::shared_ptr<Socket> )nullptr, targetRoom, "You hear the sound of someone knocking.");
    return(0);
}

//*********************************************************************
//                      cmdPrepare
//*********************************************************************

int cmdPrepare(const std::shared_ptr<Player>& player, cmd* cmnd) {
    if(cmnd->num < 2)
        return(cmdPrepareForTraps(player, cmnd));
    return(cmdPrepareObject(player, cmnd));
}


//*********************************************************************
//                      cmdBreak
//*********************************************************************

int cmdBreak(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int     duration=0;
    CatRef  newloc;
    int     chance=0, dmg=0, xpgain=0, splvl=0;
    float   mtbf=0, shots=0, shotsmax=0;
    std::shared_ptr<UniqueRoom> new_rom=nullptr;
    char    item[80];
    std::shared_ptr<Object> object=nullptr;

    player->clearFlag(P_AFK);

    if(cmnd->num < 2) {
        player->print("Break what?\n");
        return(0);
    }


    if(!player->checkAttackTimer())
        return(0);

    object = player->findObject(player, cmnd, 1);

    if(!object) {
        player->print("You don't have that in your inventory.\n");
        return(0);
    }

    if( object->flagIsSet(O_BROKEN_BY_CMD) ||
        object->flagIsSet(O_NO_BREAK) ||
        object->getType() == ObjectType::KEY ||
        object->getSpecial() ||
        object->getType() == ObjectType::MONEY ||
        (object->getShotsCur() < 1 && object->getType() != ObjectType::CONTAINER)
    ) {
        player->print("You can't break that.\n");
        return(0);
    }



    if(object->getType() == ObjectType::CONTAINER && object->getShotsMax() < 1) {
        player->print("You can't break that.\n");
        return(0);
    }

    if((object->getType() == ObjectType::WAND || (object->getType() == ObjectType::WEAPON && object->flagIsSet(O_WEAPON_CASTS) && object->getMagicpower())) &&
            player->getRoomParent()->isPkSafe() && !player->isCt()) {
        player->print("You really shouldn't do that in here. Someone could get hurt.\n");
        return(0);
    }

    chance = player->strength.getCur()*4;
    if(player->isDm())
        player->print("Chance(str): +%d%\n", player->strength.getCur()*4);

    shots = (float)object->getShotsCur();
    shotsmax = (float)object->getShotsMax();

    mtbf = shots/shotsmax;
    mtbf *=100;
    mtbf = (100 - mtbf);
    chance += (int)mtbf;

    if(player->isDm())
        player->print("Chance(shotslost): +%f%\n", mtbf);

    chance -= object->getAdjustment()*10;
    if(player->isDm())
        player->print("Chance(adj): -%d%\n", object->getAdjustment()*10);

    chance -= object->getWeight()/10;
    if(player->isDm())
        player->print("Chance(wgt): -%d%\n",object->getWeight()/10 );

    chance = std::max(1, chance);


    if(object->getQuestnum())
        chance = 0;

    if(player->isDm()) {
        player->print("Chance: %d%\n", chance);
        //  chance = 101;
    }



    if(object->getType() == ObjectType::CONTAINER && !object->objects.empty()) {
        player->print("You have to dump its contents out first!\n");
        return(0);
    }

    if(Random::get(1,100) <= chance) {

        object->setFlag(O_BROKEN_BY_CMD);
        if(object->compass) {
            delete object->compass;
            object->compass = nullptr;
        }

        if(!(object->getType() == ObjectType::POISON && player->getClass() == CreatureClass::PALADIN)) {
            player->printColor("You manage to break %P.\n", object.get());
            broadcast(player->getSock(), player->getParent(), "%M breaks %P.", player.get(), object.get());
        } else {
            player->printColor("You dispose of the vile %P.\n", object.get());
            broadcast(player->getSock(), player->getParent(), "%M neatly disposes of the vile %P.", player.get(), object.get());
        }


        if(object->getType() == ObjectType::POISON || object->getType() == ObjectType::POTION)
            strcpy(item, "used up ");
        else
            strcpy(item, "broken ");
        object->setName(std::string(item) + object->getName());

        strncpy(object->key[2],"broken",20);


        if(object->getType() == ObjectType::CONTAINER) {
            object->setShotsMax(0);
            object->setType(ObjectType::MISC);
        }

        object->setShotsCur(0);
        object->setFlag(O_NO_FIX);
        player->updateAttackTimer(true, 60);


        if( (player->getClass() == CreatureClass::BERSERKER || player->isCt()) && object->getType() != ObjectType::WEAPON &&
            (shots > 0.0) && object->value[GOLD] > 100
        ) {
            xpgain = (int)mtbf+(object->getAdjustment()*10)+Random::get(1,50);
            if(object->getType() == ObjectType::WAND)
                xpgain *= 3;

            if(!player->halftolevel()) {
                player->print("You %s %d experience for your deed.\n", gConfig->isAprilFools() ? "lose" : "gain", xpgain);
                player->addExperience(xpgain);
            }
        }

        if( (object->getType() == ObjectType::WAND || (object->getType() == ObjectType::WEAPON && object->flagIsSet(O_WEAPON_CASTS) && object->getMagicpower())) &&
            (shots > 0.0) && (Random::get(1,100) <= 50)
        ) {
            splvl = get_spell_lvl(object->getMagicpower()-1);
            if(player->isCt())
                player->print("Spell level: %d.\n", splvl);

            if(splvl >= 4) {
                player->printColor("%O explodes in a retributive strike!\n", object.get());
                broadcast(player->getSock(), player->getParent(), "%O explodes in a retributive strike!", object.get());

                player->delObj(object, true);
                object = nullptr;
            } else {
                player->printColor("%O explodes!\n", object.get());
                broadcast(player->getSock(), player->getParent(), "%O explodes!", object.get());
                broadcast(player->getSock(), player->getParent(), "%M is engulfed by magical energy!", player.get());

                player->delObj(object, true);
                object = nullptr;
            }

            broadcast(player->getSock(), player->getParent(), "%M is engulfed by a magical vortex!", player.get());

            dmg = Random::get(1,5);
            dmg += std::max(5,(Random::get(splvl*2, splvl*6)))+((int)mtbf/2);

            if(player->chkSave(BRE, player, 0))
                dmg /= 2;

            player->printColor("You take %s%d^x damage from the release of magical energy!\n", player->customColorize("*CC:DAMAGE*").c_str(), dmg);

            if(splvl >= 4) {
                auto room = player->getRoomParent();
                auto pIt = room->players.begin();
                auto pEnd = room->players.end();

                std::shared_ptr<Player> ply=nullptr;
                while(pIt != pEnd) {
                    ply = (*pIt++).lock();
                    if(!ply) continue;
                    if(ply != player && !ply->inCombat()) {
                        dmg = Random::get(splvl*4, splvl*8);

                        if(ply->chkSave(BRE, ply, 0))
                            dmg /= 2;

                        ply->printColor("You take %s%d^x damage from the magical explosion!\n", ply->customColorize("*CC:DAMAGE*").c_str(), dmg);

                        if(player->doDamage(ply, dmg, CHECK_DIE)) {
                            continue;
                        }
                    }
                }
            }

            if(splvl >= 3)
                player->getRoomParent()->scatterObjects();

            player->hp.decrease(dmg);
            if(player->hp.getCur() < 1) {
                player->die(EXPLOSION);
                return(0);
            }

            if(splvl >= 3) {
                if(Random::get(1,100) <= 50) {
                    newloc = getEtherealTravelRoom();
                    if(player->inJail())
                        newloc = player->getUniqueRoomParent()->info;
                    else if(player->getRoomParent()->flagIsSet(R_ETHEREAL_PLANE))
                        newloc.id = 50;

                    if(!loadRoom(newloc, new_rom))
                        return(0);

                    if(!player->getRoomParent()->flagIsSet(R_ETHEREAL_PLANE)) {
                        broadcast(player->getSock(), player->getParent(), "%M was blown from this universe by the explosion!", player.get());
                        player->print("You are blown into an alternate dimension.\n");
                    }
                    player->deleteFromRoom();
                    player->addToRoom(new_rom);
                    player->doPetFollow();
                }
            }

            return(0);
        }


        if(object->getType() == ObjectType::POISON && (player->getClass() == CreatureClass::PALADIN || player->isCt())) {
            xpgain = object->getEffectStrength()*15;
            // no super fast leveling by breaking poison over and over.
            if(player->getLevel() < 9)
                xpgain = std::min(xpgain, player->getLevel()*10);

            if(!player->halftolevel()) {
                player->print("You %s %d experience for your deed.\n", gConfig->isAprilFools() ? "lose" : "gain", xpgain);
                player->addExperience(xpgain);
            }
        }

    } else {
        if(!(object->getType() == ObjectType::POISON && player->getClass() == CreatureClass::PALADIN)) {
            player->printColor("You were unable to break %P.\n", object.get());
            broadcast(player->getSock(), player->getParent(), "%M tried to break %P.", player.get(), object.get());
        } else {
            player->printColor("You were unable to properly dispose of the %P.\n", object.get());
            broadcast(player->getSock(), player->getParent(), "%M tried to dispose of the %P.", player.get(), object.get());
            chance = 30 - (bonus(player->dexterity.getCur()));

            if(player->immuneToPoison())
                chance = 101;

            if(Random::get(1,100) <= chance) {
                player->printColor("^r^#You accidentally poisoned yourself!\n");
                broadcast(player->getSock(), player->getParent(), "%M accidentally poisoned %sself!",
                    player.get(), player->himHer());

                duration = standardPoisonDuration(object->getEffectDuration(), player->constitution.getCur());

                if(player->chkSave(POI, player, -1))
                    duration = duration * 2 / 3;

                player->poison(player, object->getEffectStrength(), duration);
            }
        }
        player->updateAttackTimer(true, 40);
    }
    return(0);
}
