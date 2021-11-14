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
#include "paths.hpp"                   // for Sign
#include "proto.hpp"                   // for broadcast, findExit, bonus
#include "random.hpp"                  // for Random
#include "socket.hpp"                  // for Socket
#include "statistics.hpp"              // for Statistics
#include "stats.hpp"                   // for Stat
#include "utils.hpp"                   // for MAX, MIN
#include "xml.hpp"                     // for loadRoom

#define STONE_SCROLL_INDEX      10

//*********************************************************************
//                      stoneScroll
//*********************************************************************

int stoneScroll(Player* player, cmd* cmnd) {
    char    filename[80];
    sprintf(filename, "%s/stone_scroll", Path::Sign);

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

bool doScout(Player* player, const Exit* exit);

void lookAtExit(Player* player, Exit* exit) {
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

    const Monster* guard = player->getRoomParent()->getGuardingExit(exit, player);
    if(guard && !player->checkStaff("%M %s.\n", guard, guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you look there" : "won't let you look there"))
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
//                      cmdLook
//*********************************************************************

int cmdLook(Player* player, cmd* cmnd) {
    MudObject* target=nullptr;
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
            auto* obj = dynamic_cast<Object*>(target);
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

bool storageProperty(const Player* player, const Object* object, Property **p);

int cmdThrow(Creature* creature, cmd* cmnd) {
    Object* object=nullptr;
    Creature* victim=nullptr;
    const Monster* guard=nullptr;
    Exit*   exit=nullptr;
    BaseRoom *room=nullptr, *newRoom=nullptr;
    Property* p=nullptr;
    Player* player = creature->getAsPlayer(), *pVictim=nullptr;

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
        player->printColor("%O is currently in safe keeping.\nYou must unkeep it to throw it.\n", object);
        return(0);
    }

    if( object->flagIsSet(O_NO_DROP) &&
        !player->checkStaff("You cannot throw that.\n"))
    {
        return(0);
    }

    for(Object *obj : object->objects) {
        if( obj->flagIsSet(O_NO_DROP) &&
            !player->checkStaff("You cannot throw that. It contains %P.\n", obj) )
        {
            return(0);
        }
    }

    if( object->getActualWeight() > creature->strength.getCur()/3 &&
        !creature->checkStaff("%O is too heavy for you to throw!\n", object)
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

        creature->printColor("You throw %P to the %s^x.\n", object, exit->getCName());
        broadcast(creature->getSock(), room, "%M throws %P to the %s^x.",
            creature, object, exit->getCName());
        creature->delObj(object);

        guard = room->getGuardingExit(exit, player);

        // don't bother getting rooms if exit is closed or guarded
        bool loadExit = (!exit->flagIsSet(X_CLOSED) && !exit->flagIsSet(X_PORTAL) && !guard);
        if(loadExit)
            Move::getRoom(creature, exit, &newRoom, false, &exit->target.mapmarker);

        room->wake("Loud noises disturb your sleep.", true);

        // closed, or off map, or being guarded
        if(!loadExit || (!newRoom)) {
            if(guard)
                broadcast(nullptr, room, "%M knocks %P to the ground.", guard, object);
            else
                broadcast(nullptr, room, "%O hits the %s^x and falls to the ground.",
                    object, exit->getCName());
            finishDropObject(object, room, creature);
        } else {
            room = newRoom;

            broadcast(nullptr, room, "%1O comes flying into the room.", object);
            room->wake("Loud noises disturb your sleep.", true);
            finishDropObject(object, room, creature, false, false, true);

            if(newRoom->isAreaRoom() && newRoom->getAsAreaRoom()->canDelete())
                newRoom->getAsAreaRoom()->area->remove(newRoom->getAsAreaRoom());
        }

        creature->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);

    } else if(victim) {

        pVictim = victim->getAsPlayer();
        if(!creature->canAttack(victim))
            return(0);

        if(creature->vampireCharmed(pVictim) || (victim->hasCharm(creature->getCName()) && creature->pFlagIsSet(P_CHARMED))) {
            creature->print("You like %N too much to do that.\n", victim);
            return(0);
        }

        creature->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
        creature->smashInvis();
        creature->unhide();

        if(player)
            player->statistics.swing();
        creature->printColor("You throw %P at %N.\n", object, victim);
        victim->printColor("%M throws %P at you.\n", creature, object);
        broadcast(creature->getSock(), victim->getSock(), room,
            "%M throws %P at %N.", creature, object, victim);
        creature->delObj(object);

        if(!pVictim)
            victim->getAsMonster()->addEnemy(creature);

        if(pVictim && victim->isEffected("mist")) {
            pVictim->statistics.wasMissed();
            if(player)
                player->statistics.miss();
            broadcast(victim->getSock(), room, "%O passes right through %N.", object, victim);
            victim->printColor("%O passes right through you.\n", object);
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
                broadcast(victim->getSock(), room, "%M dodges out of the way.", victim);
                creature->checkImprove("thrown", false);
            } else if(result == ATTACK_MISS || result == ATTACK_BLOCK || result == ATTACK_PARRY) {
                if(pVictim)
                    pVictim->statistics.wasMissed();
                if(player)
                    player->statistics.miss();
                victim->printColor("You knock %P to the ground.\n", object);
                broadcast(victim->getSock(), room, "%M knocks %P to the ground.", victim, object);
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

                creature->printColor("You hit %N for %s%d^x damage.\n", victim, creature->customColorize("*CC:DAMAGE*").c_str(), damage.get());
                victim->printColor("%M hit you%s for %s%d^x damage!\n", creature,
                    victim->isBrittle() ? "r brittle body" : "", victim->customColorize("*CC:DAMAGE*").c_str(), damage.get());
                broadcastGroup(false, victim, "^M%M^x hit ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n", creature,
                    victim, damage.get(), victim->heShe(), victim->getStatusStr(damage.get()));

                creature->doDamage(victim, damage.get(), CHECK_DIE);
                creature->checkImprove("thrown", true);
            }

        }

        finishDropObject(object, room, creature);
        room->wake("Loud noises disturb your sleep.", true);

    } else {
        creature->printColor("Throw %P at what?\n", object);
    }
    return(0);
}

//*********************************************************************
//                      cmdKnock
//*********************************************************************

int cmdKnock(Creature* creature, cmd* cmnd) {
    BaseRoom* targetRoom=nullptr;
    Exit    *exit=nullptr;

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
    broadcast(creature->getSock(), creature->getRoomParent(), "%M knocks on the %s^x.", creature, exit->getCName());

    // change the meaning of exit
    exit = exit->getReturnExit(creature->getRoomParent(), &targetRoom);
    targetRoom->wake("You awaken suddenly!", true);

    if(exit)
        broadcast(nullptr, targetRoom, "You hear someone knocking on the %s.", exit);
    else
        broadcast(nullptr, targetRoom, "You hear the sound of someone knocking.");
    return(0);
}

//*********************************************************************
//                      cmdPrepare
//*********************************************************************

int cmdPrepare(Player* player, cmd* cmnd) {
    if(cmnd->num < 2)
        return(cmdPrepareForTraps(player, cmnd));
    return(cmdPrepareObject(player, cmnd));
}


//*********************************************************************
//                      cmdBreak
//*********************************************************************

int cmdBreak(Player* player, cmd* cmnd) {
    int     duration=0;
    CatRef  newloc;
    int     chance=0, dmg=0, xpgain=0, splvl=0;
    float   mtbf=0, shots=0, shotsmax=0;
    UniqueRoom *new_rom=nullptr;
    char    item[80];
    Object  *object=nullptr;

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

    chance = MAX(1, chance);


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
            player->printColor("You manage to break %P.\n", object);
            broadcast(player->getSock(), player->getParent(), "%M breaks %P.", player, object);
        } else {
            player->printColor("You dispose of the vile %P.\n", object);
            broadcast(player->getSock(), player->getParent(), "%M neatly disposes of the vile %P.", player, object);
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
                player->printColor("%O explodes in a retributive strike!\n", object);
                broadcast(player->getSock(), player->getParent(), "%O explodes in a retributive strike!", object);

                player->delObj(object, true);
                delete object;
            } else {
                player->printColor("%O explodes!\n", object);
                broadcast(player->getSock(), player->getParent(), "%O explodes!", object);
                broadcast(player->getSock(), player->getParent(), "%M is engulfed by magical energy!", player);

                player->delObj(object, true);
                delete object;
            }

            broadcast(player->getSock(), player->getParent(), "%M is engulfed by a magical vortex!", player);

            dmg = Random::get(1,5);
            dmg += MAX(5,(Random::get(splvl*2, splvl*6)))+((int)mtbf/2);

            if(player->chkSave(BRE, player, 0))
                dmg /= 2;

            player->printColor("You take %s%d^x damage from the release of magical energy!\n", player->customColorize("*CC:DAMAGE*").c_str(), dmg);

            if(splvl >= 4) {
                auto pIt = player->getRoomParent()->players.begin();
                auto pEnd = player->getRoomParent()->players.end();

                Player* ply=nullptr;
                while(pIt != pEnd) {
                    ply = (*pIt++);
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

                    if(!loadRoom(newloc, &new_rom))
                        return(0);

                    if(!player->getRoomParent()->flagIsSet(R_ETHEREAL_PLANE)) {
                        broadcast(player->getSock(), player->getParent(), "%M was blown from this universe by the explosion!", player);
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
                xpgain = MIN(xpgain, player->getLevel()*10);

            if(!player->halftolevel()) {
                player->print("You %s %d experience for your deed.\n", gConfig->isAprilFools() ? "lose" : "gain", xpgain);
                player->addExperience(xpgain);
            }
        }

    } else {
        if(!(object->getType() == ObjectType::POISON && player->getClass() == CreatureClass::PALADIN)) {
            player->printColor("You were unable to break %P.\n", object);
            broadcast(player->getSock(), player->getParent(), "%M tried to break %P.", player, object);
        } else {
            player->printColor("You were unable to properly dispose of the %P.\n", object);
            broadcast(player->getSock(), player->getParent(), "%M tried to dispose of the %P.", player, object);
            chance = 30 - (bonus(player->dexterity.getCur()));

            if(player->immuneToPoison())
                chance = 101;

            if(Random::get(1,100) <= chance) {
                player->printColor("^r^#You accidentally poisoned yourself!\n");
                broadcast(player->getSock(), player->getParent(), "%M accidentally poisoned %sself!",
                    player, player->himHer());

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
