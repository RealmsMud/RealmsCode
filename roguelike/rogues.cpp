/*
 * rogues.cpp
 *   Functions that deal with rouge like abilities
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

#include <cstdlib>                     // for strtoul
#include <ctime>                       // for time
#include <list>                        // for list, operator==, list<>::cons...
#include <set>                         // for operator==, _Rb_tree_const_ite...
#include <string>                      // for allocator, string, operator==

#include "area.hpp"                    // for Area, TileInfo, MapMarker
#include "catRef.hpp"                  // for CatRef
#include "cmd.hpp"                     // for cmd
#include "commands.hpp"                // for cmdNoAuth, cmdAmbush, cmdBackstab
#include "damage.hpp"                  // for Damage
#include "delayedAction.hpp"           // for ActionSearch, DelayedAction
#include "dm.hpp"                      // for dmMobInventory
#include "factions.hpp"                // for Faction
#include "flags.hpp"                   // for P_AFK, M_PERMENANT_MONSTER
#include "global.hpp"                  // for CreatureClass, CreatureClass::...
#include "hooks.hpp"                   // for Hooks
#include "lasttime.hpp"                // for lasttime
#include "location.hpp"                // for Location
#include "money.hpp"                   // for Money, GOLD
#include "move.hpp"                    // for formatFindExit, getRoom
#include "mud.hpp"                     // for LT_HIDE, LT_SEARCH, LT_SCOUT, LT
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/container.hpp"    // for MonsterSet, Container, ObjectSet
#include "mudObjects/creatures.hpp"    // for Creature, ATTACK_CRITICAL, ATT...
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/mudObject.hpp"    // for MudObject
#include "mudObjects/objects.hpp"      // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "proto.hpp"                   // for broadcast, bonus, log_immort
#include "random.hpp"                  // for Random
#include "server.hpp"                  // for Server, gServer, GOLD_OUT
#include "size.hpp"                    // for searchMod, SIZE_GARGANTUAN
#include "statistics.hpp"              // for Statistics
#include "stats.hpp"                   // for Stat
#include "xml.hpp"                     // for loadObject, loadRoom


//*********************************************************************
//                      cmdPrepareForTraps
//*********************************************************************
// This function allows players to prepare themselves for traps that
// might be in the next room they enter.  This way, they can hope to
// avoid a trap that they already know is there.

int cmdPrepareForTraps(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    i=0, t = time(nullptr);


    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);


    if(player->flagIsSet(P_PREPARED)) {
        *player << "You've already prepared.\n";
        return(0);
    }


    if(player->inCombat()) {
        *player << "You are too busy trying to keep from dying.\n";
        return(0);
    }

    i = LT(player, LT_PREPARE);

    if(t < i) {
        player->pleaseWait(i-t);
        return(0);
    }

    player->lasttime[LT_PREPARE].ltime = t;
    player->lasttime[LT_PREPARE].interval = player->isDm() ? 0:15;

    *player << "You prepare yourself for traps.\n";
    broadcast(player->getSock(), player->getParent(), "%M prepares for traps.", player.get());
    player->setFlag(P_PREPARED);
    if(player->isBlind())
        player->clearFlag(P_PREPARED);

    return(0);
}

//*********************************************************************
//                      cmdBribe
//*********************************************************************
// This function allows players to bribe monsters. If they give the
// monster enough money, it will leave the room. If not, the monster
// keeps the money and stays.

int cmdBribe(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster> creature=nullptr;
    unsigned long amount=0;
    Money cost;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(player->getClass() == CreatureClass::BUILDER) {
        *player << "You cannot do that.\n";
        return(0);
    }
    if(cmnd->num < 2) {
        *player << "Bribe whom?\n";
        return(0);
    }
    if(cmnd->num < 3 || cmnd->str[2][0] != '$') {
        *player << "Syntax: bribe <monster> $<amount>\n";
        return(0);
    }

    creature = player->getParent()->findMonster(player, cmnd);
    if(!creature) {
        *player << "That is not here.\n";
        return(0);
    }

    if(!Faction::willDoBusinessWith(player, creature->getPrimeFaction())) {
        *player << setf(CAP) << creature << " refuses to do business with you.\n";
        return(0);
    }

    if(creature->isPet()) {
        *player << setf(CAP) << creature << " is too loyal to " << creature->getMaster() << " to be bribed.\n";
        creature->getMaster()->print("%M tried to bribe %N.\n", player.get(), creature.get());
        return(0);
    }

    amount = strtoul(&cmnd->str[2][1], nullptr, 0);
    if(amount < 1 || amount > player->coins[GOLD]) {
        *player << "Please enter the amount of the bribe.\n";
        return(0);
    }

    cost.set(creature->getExperience()*50, GOLD);
    cost = Faction::adjustPrice(player, creature->getPrimeFaction(), cost, true);

    player->coins.sub(amount, GOLD);

    Server::logGold(GOLD_OUT, player, Money(amount, GOLD), creature, "Bribe");

    if(amount < cost[GOLD] || creature->flagIsSet(M_PERMENANT_MONSTER)) {
        *player << setf(CAP) << creature << " takes your money, but stays.\n";
        broadcast(player->getSock(), player->getParent(), "%M tried to bribe %N.", player.get(), creature.get());
        creature->coins.add(amount, GOLD);
    } else {
        *player << setf(CAP) << creature << " takes your money and leaves.\n";
        broadcast(player->getSock(), player->getParent(), "%M bribed %N.", player.get(), creature.get());

        log_immort(true, player, "%s bribed %s.\n", player->getCName(), creature->getCName());

        creature->deleteFromRoom();
        gServer->delActive(creature.get());
    }

    return(0);
}


//*********************************************************************
//                      cmdGamble
//*********************************************************************
// Code for people to gamble money

int cmdGamble(const std::shared_ptr<Player>& player, cmd* cmnd) {
    if(!player->getRoomParent()->flagIsSet(R_CASINO) && !player->isCt()) {
        player->print("You can't gamble here.\n");
        return(0);
    }

    player->unhide();

    *player << "Not implemented yet.\n";
    return(0);
}

//*********************************************************************
//                      canSearch
//*********************************************************************

bool canSearch(const std::shared_ptr<Player> player) {
    if(!player->ableToDoCommand())
        return(false);

    if(player->isMagicallyHeld(true))
        return(false);

    if(player->isBlind()) {
        *player << "You're blind! How can you do that?\n";
        return(false);
    }

    if(player->flagIsSet(P_SITTING)) {
        *player << "You must stand up first!\n";
        return(false);
    }

    return(true);
}

//*********************************************************************
//                      doSearch
//*********************************************************************

void doSearch(std::shared_ptr<Player> player, bool immediate) {
    std::shared_ptr<BaseRoom> room = player->getRoomParent();
    // TODO: Why is the player sticking around after they've logged off, but not in a room
    if(!room) {
        std::clog << "DoSearch for " << player->getName() << " without a parent room.";
        return;
    }
    int     chance=0;
    bool    found=false, detectMagic = player->isEffected("detect-magic");

    player->clearFlag(P_AFK);

    if(!canSearch(player))
        return;

    player->unhide();


    int level = (int)(player->getSkillLevel("search"));

    // TODO: SKILLS: tweak the formula
    chance = 15 + 5*bonus(player->piety.getCur()) + level * 2;
    chance = std::min(chance, 90);
    if(player->getClass() == CreatureClass::RANGER)
        chance += level*8;
    if(player->getClass() == CreatureClass::WEREWOLF)
        chance += level*7;
    if(player->getClass() == CreatureClass::DRUID)
        chance += level*6;

    if(player->getClass() == CreatureClass::CLERIC && (
        (player->getDeity() == KAMIRA && player->getAdjustedAlignment() >= NEUTRAL) ||
        (player->getDeity() == ARACHNUS && player->alignInOrder())
    ) )
        chance += level*3;

    if(player->isStaff())
        chance = 100;

    
    for(const auto& ext : room->exits) {
        if( (ext->flagIsSet(X_SECRET) && Random::get(1,100) <= (chance + searchMod(ext->getSize()))) ||
            (ext->isConcealed(player) && Random::get(1,100) <= 5))
        {
            // canSee doesnt handle DescOnly
            if(player->canSee(ext) && !ext->flagIsSet(X_DESCRIPTION_ONLY) && !ext->hasBeenUsedBy(player)) {
                found = true;
                *player << "You found an exit: " << ext->getCName() << "\n";

                if(ext->isWall("wall-of-fire"))
                    player->printColor("%s", ext->blockedByStr('R', "wall-of-fire", "wall-of-fire", detectMagic, true).c_str());
                if(ext->isWall("wall-of-force"))
                    player->printColor("%s", ext->blockedByStr('M', "wall-of-force", "wall-of-force", detectMagic, true).c_str());
                if(ext->isWall("wall-of-thorns"))
                    player->printColor("%s", ext->blockedByStr('y', "wall-of-thorns", "wall-of-thorns", detectMagic, true).c_str());
                if(ext->isWall("wall-of-lightning"))
                    player->printColor("%s", ext->blockedByStr('c', "wall-of-lightning", "wall-of-lightning", detectMagic, true).c_str());
                if(ext->isWall("wall-of-sleet"))
                    player->printColor("%s", ext->blockedByStr('C', "wall-of-sleet", "wall-of-sleet", detectMagic, true).c_str());
            }
        }
    }

    for(const auto& obj : room->objects) {
        if( obj->flagIsSet(O_HIDDEN) &&
            player->canSee(obj) &&
            Random::get(1,100) <= (chance + searchMod(obj->getSize())) )
        {
            found = true;
            *player << ColorOn << "You found " << obj << "\n" << ColorOff;
        }
    }

    for(const auto& pIt: room->players) {
        if(auto ply = pIt.lock()) {
            if (ply->flagIsSet(P_HIDDEN) && player->canSee(ply) && Random::get(1, 100) <= (chance + searchMod(ply->getSize()))) {
                found = true;
                *player << "You found " << ply << " hiding.\n";
            }
        }
    }

    for(const auto& mons : room->monsters) {
        if( mons->flagIsSet(M_HIDDEN) &&
            player->canSee(mons) &&
            Random::get(1,100) <= (chance + searchMod(mons->getSize())))
        {
            found = true;
            *player << ColorOn << "You found " << mons << " hiding.\n" << ColorOff;
        }
    }

    // reuse chance as chance to find herbs
    // make it a 15-85% scale, range from 1-MAXALVL*10
    chance = level * (MAXALVL*10 / (85-15)) + 15;

    if(player->isStaff())
        chance = 100;

    if(chance >= Random::get(1,100)) {
        if(player->inAreaRoom() && player->getAreaRoomParent()->spawnHerbs()) {
            *player << "You found some herbs!\n";
            found = true;
        }
    }


    if(found) {
        if(immediate)
            broadcast(player->getSock(), room, "%s found something!", player->upHeShe());
        else
            broadcast(player->getSock(), room, "%M found something!", player.get());
        player->checkImprove("search", true);
    } else {
        *player << "You didn't find anything.\n";
        player->checkImprove("search", false);
    }
}

void doSearch(const DelayedAction* action) {
    auto target = action->target.lock();
    if(!target) return;

    doSearch(target->getAsPlayer(), false);
}

//*********************************************************************
//                      cmdSearch
//*********************************************************************
// This function allows a player to search a room for hidden objects,
// exits, monsters and players.

int cmdSearch(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    i=0, t = time(nullptr);

    player->clearFlag(P_AFK);

    if(!canSearch(player))
        return(0);
    if(gServer->hasAction(player, ActionSearch)) {
        *player << "You are already searching!\n";
        return(0);
    }

    i = LT(player, LT_SEARCH);

    if(t < i) {
        player->pleaseWait(i-t);
        return(0);
    }

    player->lasttime[LT_SEARCH].ltime = t;
    if( player->isDm() )
        player->lasttime[LT_SEARCH].interval = 0;
    else if(player->getClass() == CreatureClass::RANGER || player->getClass() == CreatureClass::WEREWOLF)
        player->lasttime[LT_SEARCH].interval = 3;
    else if((player->getClass() == CreatureClass::CLERIC && player->getDeity() == KAMIRA && player->getAdjustedAlignment() >= NEUTRAL) || player->getClass() == CreatureClass::THIEF || player->getClass() == CreatureClass::ROGUE)
        player->lasttime[LT_SEARCH].interval = 5;
    else
        player->lasttime[LT_SEARCH].interval = 7;
    
    player->interruptDelayedActions();

    broadcast(player->getSock(), player->getParent(), "%M searches the room.", player.get());

    // big rooms take longer to search
    if(player->getRoomParent()->getSize() >= SIZE_GARGANTUAN) {
        // doSearch calls unhide, no need to do it twice
        player->unhide();

        *player << "You begin searching.\n";
        gServer->addDelayedAction(doSearch, player, nullptr, ActionSearch, player->lasttime[LT_SEARCH].interval);
    } else {
        doSearch(player, true);
    }
    return(0);
}

//*********************************************************************
//                      spawnHerbs
//*********************************************************************

bool AreaRoom::spawnHerbs() {
    auto myArea = area.lock();
    if(!myArea)
        return(false);
    const std::shared_ptr<TileInfo>  tile = myArea->getTile(myArea->getTerrain(nullptr, mapmarker, 0, 0, 0, true), myArea->getSeasonFlags(mapmarker));
    if(!tile)
        return(false);
    return(tile->spawnHerbs(Container::downcasted_shared_from_this<AreaRoom>()));
}

//*********************************************************************
//                      spawnHerbs
//*********************************************************************

bool TileInfo::spawnHerbs(std::shared_ptr<BaseRoom> room) const {
    std::shared_ptr<Object>  object=nullptr;
    int     max = herbs.size();
    short   num = Random::get(1,std::max(1,max)), i=0, n=0, k=0;
    std::list<CatRef>::const_iterator it;
    bool    found=false;

    if(!max)
        return(false);

    // don't spawn if there's already herbs here
    for(const auto& obj : room->objects) {
        if(obj->flagIsSet(O_DISPOSABLE))
            return(false);
    }

    for(; i<num; i++) {
        k = 1;
        n = Random::get(1, max);

        for(it = herbs.begin() ; it != herbs.end() ; it++) {
            if(k++==n)
                break;
        }

        if(loadObject(*it, object)) {
            found = true;
            object->setFlag(O_DISPOSABLE);
            object->setDroppedBy(room, "SpawnHerb");
            object->addToRoom(room);
        }
    }

    return(found);
}

//*********************************************************************
//                      cmdHide
//*********************************************************************
// This command allows a player to try and hide themself in the shadows
// or it can be used to hide an object in a room.

int cmdHide(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> object=nullptr;
    long    i = LT(player, LT_HIDE), t = time(nullptr);
    int     chance=0;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->knowsSkill("hide")) {
        *player << "You don't really know how to hide effectively.\n";
        return(0);
    }
    if(player->flagIsSet(P_SITTING)) {
        player->print("You cannot do that while sitting down.\n");
        return(0);
    }

    if(player->isEffected("mist")) {
        *player << "You are already hidden as a mist.\n";
        return(0);
    }

    if(i > t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    player->lasttime[LT_HIDE].ltime = t;
    if( player->getClass() == CreatureClass::THIEF ||
        player->getClass() == CreatureClass::ASSASSIN ||
        player->getClass() == CreatureClass::ROGUE ||
        player->getClass() == CreatureClass::RANGER
    )
        player->lasttime[LT_HIDE].interval = 5;
    else
        player->lasttime[LT_HIDE].interval = 15;


    if(player->getSecondClass() == CreatureClass::THIEF || player->getSecondClass() == CreatureClass::ASSASSIN || (player->getClass() == CreatureClass::CLERIC && player->getDeity() == KAMIRA))
        player->lasttime[LT_HIDE].interval = 6L;

    int level = (int)player->getSkillLevel("hide");
    if(cmnd->num == 1) {
        switch(player->getClass()) {
        case CreatureClass::THIEF:
            if(player->getSecondClass() == CreatureClass::MAGE) {
                chance = std::min(90, 5 + 4*level + 3*bonus(player->dexterity.getCur()));
                player->lasttime[LT_HIDE].interval = 8;
            } else
                chance = std::min(90, 5 + 6*level + 3*bonus(player->dexterity.getCur()));
            break;
        case CreatureClass::ASSASSIN:
            chance = std::min(90, 5 + 6*level + 3*bonus(player->dexterity.getCur()));
            break;
        case CreatureClass::FIGHTER:
            if(player->getSecondClass() == CreatureClass::THIEF)
                chance = std::min(90, 5 + 4*level + 3*bonus(player->dexterity.getCur()));
            else
                chance = std::min(90, 5 + 2*level + 3*bonus(player->dexterity.getCur()));
            break;
        case CreatureClass::MAGE:
            if(player->getSecondClass() == CreatureClass::ASSASSIN || player->getSecondClass() == CreatureClass::THIEF) {
                chance = std::min(90, 5 + 4*level + 3*bonus(player->dexterity.getCur()));
                player->lasttime[LT_HIDE].interval = 8;
            } else
                chance = std::min(90, 5 + 2*level +
                        3*bonus(player->dexterity.getCur()));
            break;
        case CreatureClass::CLERIC:
            if(player->getSecondClass() == CreatureClass::ASSASSIN) {
                chance = std::min(90, 5 + 5*level + 3*bonus(player->dexterity.getCur()));
            } else if( (player->getDeity() == KAMIRA && player->getAdjustedAlignment() >= NEUTRAL) ||
                (player->getDeity() == ARACHNUS && player->alignInOrder())
            ) {
                chance = std::min(90, 5 + 4*level + 3*bonus(player->piety.getCur()));
            } else
                chance = std::min(90, 5 + 2*level + 3*bonus(player->dexterity.getCur()));

            break;
        case CreatureClass::RANGER:
        case CreatureClass::DRUID:
            chance = 5 + 10*level + 3*bonus(player->dexterity.getCur());
            break;
        case CreatureClass::ROGUE:
            chance = std::min(90, 5 + 5*level + 3*bonus(player->dexterity.getCur()));
            break;
        default:
            chance = std::min(90, 5 + 2*level + 3*bonus(player->dexterity.getCur()));
            break;
        }


        if(player->isStaff())
            chance = 101;


        if(player->isEffected("camouflage") && player->getRoomParent()->isOutdoors())
            chance += 20;

        if( (player->getRoomParent()->flagIsSet(R_DARK_AT_NIGHT) && !isDay()) ||
            player->getRoomParent()->flagIsSet(R_DARK_ALWAYS) ||
            player->getRoomParent()->isEffected("dense-fog")
        )
            chance += 10;

        if(player->dexterity.getCur()/10 < 9 && !(player->getClass() == CreatureClass::CLERIC && player->getDeity() == KAMIRA))
            chance -= 10*(9 - player->dexterity.getCur()/10); // Having less then average dex

        *player << "You attempt to hide in the shadows.\n";

        if((player->getClass() == CreatureClass::RANGER || player->getClass() == CreatureClass::DRUID) && !player->getRoomParent()->isOutdoors()) {
            chance /= 2;
            chance = std::max(25, chance);
            *player << "You have trouble hiding while inside.\n";
        }

        if(player->inCombat())
            chance = 0;

        if(player->isBlind())
            chance = std::min(chance, 20);

        if(Random::get(1,100) <= chance || player->isEffected("mist")) {
            player->setFlag(P_HIDDEN);
            *player << "You slip into the shadows unnoticed.\n";
            player->checkImprove("hide", true);
        } else {
            player->unhide();
            broadcast(player->getSock(), player->getParent(), "%M tries to hide in the shadows.", player.get());
            player->checkImprove("hide", false);
        }

        return(0);
    }

    object = player->getRoomParent()->findObject(player, cmnd, 1);

    if(!object) {
        *player << "You don't see that here.\n";
        return(0);
    }

    player->unhide();

    if( object->flagIsSet(O_NO_TAKE) &&
        !player->checkStaff("You cannot hide that.\n")
    )
        return(0);

    if(isGuardLoot(player->getRoomParent(), player, "%M will not let you hide that.\n"))
        return(0);

    if(player->isDm())
        chance = 100;
    else if(player->getClass() == CreatureClass::THIEF || player->getClass() == CreatureClass::ASSASSIN || player->getClass() == CreatureClass::ROGUE)
        chance = std::min(90, 10 + 5*level + 5*bonus(player->dexterity.getCur()));
    else if(player->getClass() == CreatureClass::RANGER || player->getClass() == CreatureClass::DRUID)
        chance = 5 + 9*level + 3*bonus(player->dexterity.getCur());
    else
        chance = std::min(90, 5 + 3*level + 3*bonus(player->dexterity.getCur()));

    *player << "You attempt to hide it.\n";
    broadcast(player->getSock(), player->getParent(), "%M attempts to hide %1P.", player.get(), object.get());

    if(Random::get(1, 100) <= chance) {
        object->setFlag(O_HIDDEN);
        *player << ColorOn << "You tuck " << object << " into a corner.\n" << ColorOff;
        player->checkImprove("hide", true);
    } else {
        object->clearFlag(O_HIDDEN);
        player->checkImprove("hide", false);
    }

    return(0);
}

//*********************************************************************
//                      doScout
//*********************************************************************

bool doScout(std::shared_ptr<Player> player, const std::shared_ptr<Exit> exit) {
    std::shared_ptr<BaseRoom> room=nullptr;


    if(exit->flagIsSet(X_STAFF_ONLY) && !player->isStaff()) {
        *player << "A magical force prevents you from seeing beyond that exit.\n";
        return(false);
    }


    Move::getRoom(player, exit, room, true);

    if(player->getClass() == CreatureClass::BUILDER && room && !room->isConstruction()) {
        *player << "A magical force prevents you from seeing beyond that exit.\n";
        return(false);
    }

    if( room &&
        room->isEffected("dense-fog")&&
        !player->checkStaff("That room is filled with a dense fog.\n"))
        return(false);

    if(!room && exit->target.mapmarker.getArea()) {
        std::shared_ptr<Area>  area = gServer->getArea(exit->target.mapmarker.getArea());
        if(!area) {
            *player << "Off the map in that direction.\n";
            if(player->isStaff())
                *player << ColorOn << "^yArea does not exist.\n" << ColorOff;
            return(false);
        }
        // There is no room, but we're going to pretend there is.
        *player << "\n";
        if(!area->name.empty())
          //  player->printColor("%s%s^x\n\n",
            //    (!player->flagIsSet(P_NO_EXTRA_COLOR) && area->isSunlight(exit->target.mapmarker) ? "^C" : "^c"),
              //  area->name.c_str());
            *player << ColorOn << (!player->flagIsSet(P_NO_EXTRA_COLOR) && area->isSunlight(exit->target.mapmarker) ? "^C" : "^c") << area->name.c_str() << "^x\n\n" << ColorOff; 

        //player->printColor("%s", area->showGrid(player, exit->target.mapmarker, false).c_str());

        *player << ColorOn << area->showGrid(player, exit->target.mapmarker, false).c_str() << ColorOff;

       // player->printColor("^g%s exits: north, east, south, west, northeast, northwest, southeast, southwest.^w\n\n",
         //   player->isStaff() ? "All" : "Obvious");

        *player << ColorOn << "^g" << (player->isStaff() ? "All" : "Obvious") << " exits: north, east, south, west, northeast, northwest, southeast, southwest.^w\n\n" << ColorOff;

    } else if(room) {
        display_rom(player, room);
    } else {
        *player << "Off the map in that direction.\n";
        if(player->isStaff())
            *player << ColorOn << "^yRoom does not exist.\n" << ColorOff;
        return(false);
    }

    return(true);
}

//*********************************************************************
//                      cmdScout
//*********************************************************************

int cmdScout(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Exit> exit=nullptr;
    int     chance=0;
    long    t=0, i=0;
    bool    alwaysSucceed=false;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->isStaff() && !player->knowsSkill("scout")) {
        *player << "You lack the training to properly scout exits.\n";
        return(0);
    }

    if(!player->isStaff() && player->isBlind()) {
        *player << ColorOn << "^CYou're blind!\n" << ColorOff;
        return(0);
    }

    if (!player->isStaff() && player->inCombat() ) {
        *player << "You are fighting right now. How can you do that?\n";
        return(0);
    }

    if( player->getRoomParent()->isEffected("dense-fog") &&
        !player->checkStaff("This room is filled with a dense fog.\nYou cannot scout effectively.\n")
    )
        return(0);

    if(cmnd->num < 2) {
        *player << "Scout where?\n";
        return(0);
    }

    i = player->lasttime[LT_SCOUT].ltime + player->lasttime[LT_SCOUT].interval;
    t = time(nullptr);

    if(!player->isStaff() && t < i) {
        player->pleaseWait(i - t);
        return(0);
    }

    exit = findExit(player, Move::formatFindExit(cmnd), cmnd->val[1], player->getRoomParent());


    if(!exit) {
        *player << "You don't see that exit.\n";
        player->lasttime[LT_SCOUT].ltime = t;
        player->lasttime[LT_SCOUT].interval = 10L;
        return(0);
    }


    alwaysSucceed = (exit->flagIsSet(X_CAN_LOOK) || exit->flagIsSet(X_LOOK_ONLY));

    if( !alwaysSucceed &&
        exit->flagIsSet(X_NO_SCOUT) &&
        !player->checkStaff("You were unable to scout it.\n"))
        return(0);

    const std::shared_ptr<Monster>  guard = player->getRoomParent()->getGuardingExit(exit, player);
    if(guard && !player->checkStaff("%M %s.\n", guard.get(), guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you scout there" : "won't let you scout there"))
        return(0);

    if(!alwaysSucceed) {
        player->lasttime[LT_SCOUT].ltime = t;
        player->lasttime[LT_SCOUT].interval = 20L;

        chance = 40 + (int)player->getSkillLevel("scout") * 3;
        if (player->getClass() == CreatureClass::CLERIC && player->getDeity() == MARA)
            chance += ((player->piety.getCur()+player->dexterity.getCur())/20);
        else
            chance += ((player->intelligence.getCur()+player->dexterity.getCur())/20);

        if(exit->isEffected("wall-of-fire"))
            chance -= 15;
        if(exit->isEffected("wall-of-thorns"))
            chance -= 15;
        if(exit->isEffected("wall-of-lightning"))
            chance -= 15;
        if(exit->isEffected("wall-of-sleet"))
            chance -= 15;

        chance = std::min(85, chance);

        if(!player->isStaff() && Random::get(1, 100) > chance) {
            player->print("You fail scout in that direction.\n");
            player->checkImprove("scout", false);
            return(0);
        }

        if( (exit->flagIsSet(X_LOCKED) || exit->flagIsSet(X_CLOSED) ) &&
            !player->checkStaff("You cannot scout through a closed exit.\n")
        )
            return(0);

        if( exit->flagIsSet(X_NEEDS_FLY) &&
            !player->isEffected("fly") &&
            !player->checkStaff("You must fly to scout there.\n")
        )
            return(0);
    }

    // can't scout where you can't go
    if(exit->target.mapmarker.getArea()) {
        std::shared_ptr<Area> area = gServer->getArea(exit->target.mapmarker.getArea());
        if(!area) {
            player->print("Off the map in that direction.\n");
            if(player->isStaff())
                player->printColor("^eArea does not exist.\n");
            return(0);
        }
        if( !area->canPass(player, exit->target.mapmarker, true) &&
            !player->checkStaff("You cannot scout that way.\n") )
            return(0);
    }

    if(!alwaysSucceed)
        player->checkImprove("scout", true);
    player->printColor("You scout the %s^x exit.\n", exit->getCName());

    if(player->isStaff() && player->flagIsSet(P_DM_INVIS))
        broadcast(isStaff, player->getSock(), player->getRoomParent(), "%M scouts the %s^x exit.", player.get(), exit->getCName());
    else if(exit->flagIsSet(X_SECRET) || exit->isConcealed() || exit->flagIsSet(X_DESCRIPTION_ONLY))
        broadcast(player->getSock(), player->getParent(), "%M scouts the area.", player.get());
    else
        broadcast(player->getSock(), player->getParent(), "%M scouts the %s^x exit.", player.get(), exit->getCName());

    doScout(player, exit);
    return(0);
}

//*********************************************************************
//                      cmdEnvenom
//*********************************************************************
// This will allow an assassin to put poison on a weapon and use it to
// poison other players and monsters. -- TC

int cmdEnvenom(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> weapon=nullptr, object=nullptr;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->knowsSkill("envenom")) {
        *player << "You lack the training to envenom your weapons.\n";
        return(0);
    }

    if(player->isBlind()) {
        *player << ColorOn << "^CYou can't do that! You're blind!\n" << ColorOff;
        return(0);
    }

    if(cmnd->num < 3) {
        *player << "Syntax: envenom (weapon) (poison)\n";
        return(0);
    }

    weapon = player->findObject(player, cmnd, 1);

    if(!weapon) {
        *player << "You do not have that weapon in your inventory.\n";
        return(0);
    }

    if(weapon->flagIsSet(O_ENVENOMED)) {
        *player << ColorOn << weapon << " is already envenomed.\n" << ColorOff;
        return(0);
    }

    if(weapon->getShotsCur() < 1) {
        *player << "Envenoming a broken weapon is kind of pointless.\n";
        return(0);
    }

    std::string category = weapon->getWeaponCategory();
    if(category != "slashing" && category != "piercing" &&
        !player->checkStaff("You can only envenom slashing and piercing weapons.\n"))
        return(0);

    object = player->findObject(player, cmnd, 2);

    if(!object) {
        *player << "You do not have that poison in your inventory.\n";
        return(0);
    }

    if(object->getType() != ObjectType::POISON) {
        *player << "That is not poison.\n";
        return(0);
    }

    if(object->getShotsCur() < 1) {
        *player << ColorOn << object << " is all used up.\n" << ColorOff;
        return(0);
    }

    *player << ColorOn << "You envenom "<< weapon << " with " << object << "\n." << ColorOff;
    player->checkImprove("envenom", true);
    broadcast(player->getSock(), player->getParent(), "%M envenoms %P with %P.", player.get(), weapon.get(), object.get());


    // TODO: make poison more powerful with better envenom skill
    object->decShotsCur();
    weapon->setFlag(O_ENVENOMED);
    if(!object->getEffect().empty())
        weapon->setEffect(object->getEffect());
    else
        weapon->setEffect("poison");
    weapon->setEffectDuration(object->getEffectDuration()); // Sets poison duration.
    weapon->setEffectStrength(object->getEffectStrength()); // Sets poison tick damage.

    // Did this because some poisons are better then others, and assassins
    // will be buying the poison. The more expensive, the better it is. Having
    // them have to buy it makes it more of a pain for some anarchist to just run around
    // poisoning everyone with a small knife just for fun.

    weapon->lasttime[LT_ENVEN].ltime = time(nullptr);
    weapon->lasttime[LT_ENVEN].interval = 60*(Random::get(3,5)+weapon->getAdjustment());

    return(0);
}


//*********************************************************************
//                      cmdShoplift
//*********************************************************************
// This function will allow thieves to steal from shops.

bool isValidShop(const std::shared_ptr<UniqueRoom>& shop, const std::shared_ptr<UniqueRoom>& storage);

int cmdShoplift(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<UniqueRoom> room=nullptr, storage=nullptr;
    std::shared_ptr<Object> object=nullptr, object2=nullptr;
    int     chance=0, guarded=0;


    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!needUniqueRoom(player))
        return(0);

    if(player->flagIsSet(P_SITTING)) {
        player->print("You have to stand up first.\n");
        return(0);
    }


    if(player->getClass() == CreatureClass::BUILDER)
        return(0);

    room = player->getUniqueRoomParent();

    if(!player->isCt() && player->getLevel() < 10 && player->getClass() !=  CreatureClass::THIEF) {
        *player << "You couldn't possibly succeed. Wait until level 7.\n";
        return(0);
    }

    if(player->getClass() == CreatureClass::THIEF && player->getLevel() < 4) {
        *player << "You couldn't possibly succeed. Wait until level 4.\n";
        return(0);
    }

    if(player->getClass() == CreatureClass::PALADIN) {
        *player << "Paladins do not allow themselves to fall to the level of petty theft.\n";
        return(0);
    }

    if(player->isEffected("berserk")) {
        *player << "You can't shoplift while your going berserk! Go break something!\n";
        return(0);
    }

    if(!room->flagIsSet(R_SHOP)) {
        *player << "This is not a shop. There's nothing to shoplift here.\n";
        return(0);
    }

    if(!room->flagIsSet(R_CAN_SHOPLIFT)) {
        *player << "Someone surely would see you if you tried that here.\n";
        return(0);
    }

    if(player->inCombat()) {
        *player << "You can't shoplift in the middle of combat!\n";
        return(0);
    }

    if(player->isEffected("mist")) {
        *player << "You can't shoplift while you are a mist.\n";
        return(0);
    }



    loadRoom(shopStorageRoom(room), storage);

    if(!isValidShop(room, storage)) {
        *player << "This is not a shop; there's nothing to shoplift.\n";
        return(0);
    }


    
    storage->addPermCrt();
    for(const auto& mons : storage->monsters) {
        if(mons->getLevel() >= 10 && mons->flagIsSet(M_PERMENANT_MONSTER))
            guarded++;
    }

    // This is done in order that a catastrophic building mistake cannot be made.
    // If someone forgot to perm mobs in the storeroom, and there were no perms
    // in the shop itself to guard the items, then players would be able to steal
    // all they wanted from the shop all day long whenever they wanted to.

    if(guarded < 2) {
        *player <<"Someone surely would see you if you tried that here.\n";
        return(0);
    }

    // This has to be done to prevent baiting.
    // The mobs will wander after a time.
    for(const auto& mons : room->monsters) {
        if(mons->flagIsSet(M_ATTACKING_SHOPLIFTER) && !player->isDm()) {
            *player << "The shopkeep or shop's guards are too alert right now.\n";
            return(0);
        }
    }


    if(cmnd->num < 2) {
        *player << "Shoplift what?\n";
        return(0);
    }

    object = storage->findObject(player, cmnd, 1);

    if(!object) {
        *player << "That item isn't on display.\n";
        return(0);
    }
    

    if(object->getName() == "storage room") {
        *player << "You can't shoplift that!\n";
        return(0);
    }

    if(object->flagIsSet(O_NO_SHOPLIFT)) {
        *player << "That item is too well guarded for you to shoplift.\n";
        return(0);
    }

    if(object->getActualWeight() > 50) {
        *player << "That object is too heavy and bulky to shoplift.\n";
        return(0);
    }

    if(player->getWeight() + object->getActualWeight() > player->maxWeight()) {
        *player << "That would be too much for you to carry.\n";
        return(0);
    }

    player->unhide();

    switch (player->getClass()) {
    case CreatureClass::THIEF:
    case CreatureClass::CARETAKER:
    case CreatureClass::DUNGEONMASTER:
        chance = ((player->getLevel()+35 - (2*object->getActualWeight())) - (object->value[GOLD]/1500))
                + (bonus(player->dexterity.getCur()) * 2);
        break;

    case CreatureClass::ASSASSIN:
    case CreatureClass::BARD:
    case CreatureClass::ROGUE:
        chance = ((player->getLevel()+20 - (2*object->getActualWeight())) - (object->value[GOLD]/1500))
                + (bonus(player->dexterity.getCur()) * 2);
        break;

    // Casting classes just aren't cut out for it.
    case CreatureClass::MAGE:
    case CreatureClass::LICH:
    case CreatureClass::DRUID:
    case CreatureClass::CLERIC:
        chance = (((int)player->getLevel() - (2*object->getActualWeight())) - (object->value[GOLD]/1500))
                + (bonus(player->dexterity.getCur()) * 2);
    default:
        chance = (((int)player->getLevel()+10 - (2*object->getActualWeight())) - (object->value[GOLD]/1500))
                + (bonus(player->dexterity.getCur()) * 2);
        break;
    }

    // Armor makes a lot of noise
    if(object->getType() == ObjectType::ARMOR && object->getWearflag() != FINGER)
        chance -= 10;

    // An invisible person would have it easier.
    if(player->isInvisible())
        chance += 5;


    chance = std::min(70, std::max(2,chance));
    if(player->isCt())
        player->print("Chance: %d%\n", chance);

    if(player->isDm())
        chance = 101;

    player->statistics.attemptSteal();
    player->interruptDelayedActions();

    if(Random::get(1,100) > chance) {
        player->setFlag(P_CAUGHT_SHOPLIFTING);
        *player << "You were caught!\n";
        broadcast(player->getSock(), room, "%M tried to shoplift %1P.\n%M was seen!", player.get(), object.get(), player.get());

        player->smashInvis();

        // Activates lag protection.
        if(player->flagIsSet(P_LAG_PROTECTION_SET))
            player->setFlag(P_LAG_PROTECTION_ACTIVE);

        for(const auto& mons : room->monsters) {
            if(mons->flagIsSet(M_PERMENANT_MONSTER)) {

                gServer->addActive(mons);
                mons->clearFlag(M_DEATH_SCENE);
                mons->clearFlag(M_FAST_WANDER);
                mons->addPermEffect("detect-invisible");
                mons->addPermEffect("true-sight");
                mons->setFlag(M_BLOCK_EXIT);
                mons->setFlag(M_ATTACKING_SHOPLIFTER);

                mons->addEnemy(player, true);
                mons->diePermCrt();
            }
        }
        auto mIt = storage->monsters.begin();

        while(mIt != storage->monsters.end()) {

            std::shared_ptr<Monster> mons = (*mIt++);
            if(mons->flagIsSet(M_PERMENANT_MONSTER)) {
                if(!storage->players.empty())
                    broadcast(mons->getSock(), mons->getRoomParent(),
                        "%M runs out after a shoplifter.", mons.get());
                else
                    gServer->addActive(mons);
                mons->clearFlag(M_PERMENANT_MONSTER);  // This is all done to prevent people from getting
                mons->clearFlag(M_FAST_WANDER);  // killed mostly by others baiting these mobs in.
                mons->clearFlag(M_DEATH_SCENE);
                mons->addPermEffect("detect-invisible");
                mons->addPermEffect("true-sight");
                mons->setFlag(M_BLOCK_EXIT);
                mons->setFlag(M_WILL_ASSIST);
                mons->setFlag(M_WILL_BE_ASSISTED);
                mons->clearFlag(M_AGGRESSIVE_GOOD);
                mons->clearFlag(M_AGGRESSIVE_EVIL);
                mons->clearFlag(M_AGGRESSIVE);

                mons->setFlag(M_OUTLAW_AGGRO);
                mons->setFlag(M_ATTACKING_SHOPLIFTER);

                mons->setExperience(10);
                mons->coins.zero();

                mons->addEnemy(player, true);
                mons->diePermCrt();
                mons->deleteFromRoom();
                mons->addToRoom(room);
            }
        }

        // prevents shoptlift ; flee
        player->stun(Random::get(3,5));

    } else {

        object2 = std::make_shared<Object>();

        *object2 = *object;
        object2->clearFlag(O_PERM_INV_ITEM);
        object2->clearFlag(O_PERM_ITEM);
        object2->clearFlag(O_TEMP_PERM);

        object2->setFlag(O_WAS_SHOPLIFTED);
        object2->setDroppedBy(room, "Shoplift");

        player->addObj(object2);
        *player << ColorOn << "You manage to conceal " << object2 << " on your person.\n" << ColorOff;
        broadcast(isCt, player->getSock(), player->getRoomParent(), "*DM* %M just shoplifted %1P.", player.get(), object2.get());
        player->statistics.steal();
    }

    return(0);
}

//*********************************************************************
//                      cmdBackstab
//*********************************************************************
// This function allows thieves and assassins to backstab a monster.
// If successful, a damage multiplier is given to the player. The
// player must be successfully hidden for the backstab to work. If
// the backstab fails, then the player is forced to wait double the
// normal amount of time for their next attack.

int cmdBackstab(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> pTarget=nullptr;
    std::shared_ptr<Creature> target=nullptr;
    int      n=0;
    int     disembowel=0, dur=0, dmg=0;
    float   stabMod=0.0, cap=0.0;
    Damage damage;

    if(!player->ableToDoCommand())
        return(0);


    if(!player->knowsSkill("backstab")) {
        *player << "You don't know how to backstab.\n";
        return(0);
    }


    if(player->isBlind()) {
        *player << "Backstab what?\n";
        return(0);
    }

    std::shared_ptr<Object>  weapon = player->ready[WIELD - 1];

    if(!weapon && !player->checkStaff("Backstabing requires a weapon.\n"))
        return(0);

    std::string category = player->getPrimaryWeaponCategory();

    if( category != "slashing" && category != "piercing" &&
        !player->checkStaff("Backstabing requires a slashing or piercing weapon.\n")
    )
        return(0);

    if( weapon && weapon->flagIsSet(O_NO_BACKSTAB) &&
        !player->checkStaff("%O cannot be used to backstab.\n", weapon.get())
    )
        return(0);


    if(!player->checkAttackTimer())
        return(0);

    if(player->checkHeavyRestrict("backstab"))
        return(0);

    if(!(target = player->findVictim(cmnd, 1, true, false, "Backstab what?\n", "You don't see that here.\n")))
        return(0);

    if(target)
        pTarget = target->getAsPlayer();

    if(!pTarget && target->getAsMonster()->isEnemy(player)) {
        *player << "Not while you're already fighting " << target->himHer() << ".\n";
        return(0);
    }
    if(!player->canAttack(target))
        return(0);

    player->updateAttackTimer();

    if(player->dexterity.getCur() > 180)
        player->modifyAttackDelay(-10);

    *player << ColorOn << "You attempt to backstab " << target << ".\n" << ColorOff;
    *target << setf(CAP) << player << " attempts to backstab you!\n";
    broadcast(player->getSock(), target->getSock(), player->getParent(), "%M attempts to backstab %N.", player.get(), target.get());

    if(target->isMonster())
        target->getAsMonster()->addEnemy(player);

    if(player->breakObject(player->ready[WIELD-1], WIELD)) {
        broadcast(player->getSock(), player->getParent(), "%s backstab failed.", player->upHisHer());
        player->setAttackDelay(player->getAttackDelay()*2);
        return(0);
    }

    int skillLevel = (int)((player->getWeaponSkill(weapon) + (player->getSkillGained("backstab")*2)) / 3);

    AttackResult result = player->getAttackResult(target, weapon, DOUBLE_MISS, skillLevel);

    if(result == ATTACK_HIT || result == ATTACK_CRITICAL || result == ATTACK_BLOCK || result == ATTACK_GLANCING) {
        if(!player->isHidden() && !player->isEffected("mist"))
            result = ATTACK_MISS;

        if(!pTarget && target->flagIsSet(M_NO_BACKSTAB))
            result = ATTACK_MISS;
    }

    player->smashInvis();
    player->unhide();

    int level = (int)player->getSkillLevel("backstab");
    std::string with = Statistics::damageWith(player, player->ready[WIELD-1]);

    if(player->isDm() && result != ATTACK_CRITICAL)
        result = ATTACK_HIT;

    player->statistics.swing();

    if(result == ATTACK_HIT || result == ATTACK_CRITICAL || result == ATTACK_BLOCK || result == ATTACK_GLANCING) {
//      int dmg = 0;

        // Progressive backstab code for thieves and assassins....
        if(player->getClass() == CreatureClass::THIEF || player->getSecondClass() == CreatureClass::THIEF) {
            if(level < 10)
                stabMod = 2.75;
            else if(level < 16)
                stabMod = 3.25;
            else if(level < 22)
                stabMod = 3.75;
            else if(level < 28)
                stabMod = 4.5;
            else if(level < 36)
                stabMod = 5.0;
            else if(level < 40)
                stabMod = 5.5;
            else
                stabMod = 5.75;
        } else if(player->getClass() == CreatureClass::ASSASSIN || player->getSecondClass() == CreatureClass::ASSASSIN) {
            if(level < 10)
                stabMod = 3.25;
            else if(level < 13)
                stabMod = 3.75;
            else if(level < 16)
                stabMod = 4;
            else if(level < 19)
                stabMod = 4.25;
            else if(level < 22)
                stabMod = 4.5;
            else if(level < 25)
                stabMod = 5.0;
            else if(level < 28)
                stabMod = 5.25;
            else if(level < 30)
                stabMod = 5.5;
            else if(level < 34)
                stabMod = 6.0;
            else if (level < 38)
                stabMod = 6.25;
            else
                stabMod = 6.5;
        } else
            stabMod = 5.25;



        if( player->getSecondClass() == CreatureClass::THIEF &&
            (player->getClass() == CreatureClass::FIGHTER || player->getClass() == CreatureClass::MAGE)
        ) {
            cap = 3.0;
        }

        if( player->getSecondClass() == CreatureClass::ASSASSIN ||
            (player->getClass() == CreatureClass::MAGE && player->getSecondClass() == CreatureClass::THIEF)
        ) {
            cap = 4.0;
        }

        if(cap > 0)
            stabMod = std::min<float>(cap, stabMod-1);

        // Version 2.43c Update -- I'm multiplying the stabMod by 2.0 to compensate for the increase
        // in armor absorb, and the removal of multiplier for attackPower
        stabMod *= 2.0;

        if (target->isPlayer() && target->isEffected("stoneskin")) {
            stabMod /= 3.0;
            *player << ColorOn << "^y" << setf(CAP) << target << "'s stoneskin spell reduced your backstab's effectiveness!\n" << ColorOff;
            *target << ColorOn << "^yYour stoneskin spell reduced the effectiveness of " << player << "'s backstab.\n" << ColorOff;
        }

        // Return of 1 means the weapon was shattered or otherwise rendered unsuable
        int drain = 0;
        bool wasKilled = false, meKilled = false;
        if(player->computeDamage(target, weapon, ATTACK_BACKSTAB, result, damage, true, drain, stabMod) == 1) {
            player->unequip(WIELD, UNEQUIP_DELETE);
            weapon = nullptr;
            player->computeAttackPower();
        }
        damage.includeBonus();
        n = damage.get();

        if(result == ATTACK_BLOCK) {
            *player << ColorOn << "^C" << setf(CAP) << target << "partially blocked your attack!\n" << ColorOff;
            *target << ColorOn << "^CYou manage to partially block " << player << "'s attack!\n" << ColorOff;
        }


        if(result == ATTACK_CRITICAL)
            player->statistics.critical();
        else
            player->statistics.hit();
        if(target->isPlayer())
            target->getAsPlayer()->statistics.wasHit();


        disembowel = target->hp.getCur() * 2;

        player->printColor("You backstabbed %N for %s%d^x damage.\n", target.get(), player->customColorize("*CC:DAMAGE*").c_str(), damage.get());
        player->checkImprove("backstab", true);
        log_immort(false,player, "%s backstabbed %s for %d damage.\n", player->getCName(), target->getCName(), damage.get());


        target->printColor( "^M%M^x backstabbed you%s for %s%d^x damage.\n", player.get(),
            target->isBrittle() ? "r brittle body" : "", target->customColorize("*CC:DAMAGE*").c_str(), damage.get());

        broadcastGroup(false, target, "^M%M^x backstabbed ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n",
            player.get(), target.get(), damage.get(), target->heShe(), target->getStatusStr(damage.get()));

        player->statistics.attackDamage(damage.get(), with);

        if( weapon && weapon->getMagicpower() &&
            weapon->flagIsSet(O_WEAPON_CASTS) &&
            Random::get(1,100) <= 50 && weapon->getChargesCur() > 0)
        {
            n += player->castWeapon(target, weapon, meKilled);
        }

        if( weapon &&
            (   (weapon->flagIsSet(O_ENVENOMED) && player->getClass() == CreatureClass::ASSASSIN && level >=7) ||
                (weapon->flagIsSet(O_ENVENOMED) && player->isCt())
            )
        )
            n += player->checkPoison(target, weapon);

        meKilled = player->doReflectionDamage(damage, target) || meKilled;

        player->doDamage(target, damage.get(), NO_CHECK);
        wasKilled = target->hp.getCur() < 1;

        // Check for disembowel
        if(wasKilled && n > disembowel && Random::get(1,100) < 50) {
            switch(Random::get(1,4)) {
            case 1:
                *player << ColorOn << "^cYou completely disemboweled " << target << "! " << target->upHeShe() << "'s dead!\n" << ColorOff;
                broadcast(player->getSock(), player->getParent(), "%M completely disembowels %N! %s's dead!",
                    player.get(), target.get(), target->upHeShe());
                if(target->isPlayer())
                    *target << ColorOn << "^c" << setf(CAP) << player << " completely disemboweled you! You're dead!\n" << ColorOff;

                break;
            case 2:
                *player << ColorOn << "^cYou impaled " << target << " through " << target->hisHer() << " back! " << target->upHeShe() << "'s dead!\n";
                broadcast(player->getSock(), player->getParent(), "%M impales %N through %s back! %s's dead!",
                    player.get(), target.get(), target->hisHer(), target->upHeShe());
                if(target->isPlayer())
                    *target << ColorOn << "^c" << setf(CAP) << player << " impaled you through the back! You're dead!\n" << ColorOff;

                break;
            case 3:
                if(weapon) {
                    *player << ColorOn << "^cThe " << weapon << " went completely through " << target << "! " << target->upHeShe() << "'s dead!\n" << ColorOff;
                    broadcast(player->getSock(), player->getParent(), "%M's %s goes completely through %N! %s's dead!",
                        player.get(), weapon->getCName(), target.get(), target->upHeShe());
                    if(target->isPlayer())
                        *target << ColorOn << "^c" << setf(CAP) << player << "'s " << weapon << " sticks out of your chest! You're dead!\n" << ColorOff;
                }
                break;
            case 4:
                *player << ColorOn << "^cYou cut %N in half from behind! " << target->upHeShe() << "'s dead!\n" << ColorOff;
                broadcast(player->getSock(), player->getParent(), "%M cut %N in half from behind! %s's dead!",
                    player.get(), target.get(), target->upHeShe());
                if(target->isPlayer())
                    *target << ColorOn << "^c" << setf(CAP) << player << " cut you in half from behind! You're dead!\n" << ColorOff;
                break;
            }
        }

        if(weapon)
            weapon->decShotsCur();
        Creature::simultaneousDeath(player, target, false, false);

        // only monsters flee, and not when their attacker was killed
        if(!wasKilled && !meKilled && target->getAsMonster()) {
            if(target->flee() == 2)
                return(0);
        }

    } else if(result == ATTACK_MISS) {
        player->statistics.miss();
        if(target->isPlayer())
            target->getAsPlayer()->statistics.wasMissed();
        *player << "You missed.\n";
        player->checkImprove("backstab", false);
        broadcast(player->getSock(), player->getParent(), "%s backstab failed.", player->upHisHer());
        player->setAttackDelay(Random::get(30,90));
    } else if(result == ATTACK_DODGE) {
        target->dodge(player);
    } else if(result == ATTACK_PARRY) {
        int parryResult = target->parry(player);
        if(parryResult == 2) {
            // oh no, we died from a riposte...
            return(0);
        }
    } else if(result == ATTACK_FUMBLE) {
        player->statistics.fumble();
        *player << ColorOn << "^gYou FUMBLED your weapon.\n" << ColorOff;
        broadcast(player->getSock(), player->getParent(), "^g%M fumbled %s weapon.", player.get(), player->hisHer());

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
        player->computeAttackPower();
    } else {
        *player << ColorOn << "^RError!!! Unhandled attack result: " << result << "\n" << ColorOff;
    }

    return(0);
}

//*********************************************************************
//                      checkPoison
//*********************************************************************
// This will check for envenom poisoning against the target with the given weapon

int Player::checkPoison(std::shared_ptr<Creature> target, std::shared_ptr<Object>  weapon) {
    if(!weapon)
        return(0);

    int dmg = 0;
    int level = (int)getSkillLevel("backstab");
    int poisonchance = 50+((level - (target->getLevel()-1))*10);
    poisonchance -= (2*bonus(target->constitution.getCur()));

    if(target->mFlagIsSet(M_WILL_POISON))
        poisonchance /= 2;

    poisonchance = std::max(0,std::min(poisonchance, 80));

    // Can't poison them if they're immune to poison
    if(target->immuneToPoison())
        poisonchance = 0;

    // No poisoning people if they're in a duel
    if(induel(Containable::downcasted_shared_from_this<Player>(), target->getAsPlayer()))
        poisonchance = 0;

    // If they're already poisoned, don't poison them again
    if(target && target->isPoisoned())
        poisonchance = 0;

    // Less of a chance to poison a perm
    if(!target && target->flagIsSet(M_PERMENANT_MONSTER))
        poisonchance = std::min(poisonchance, 10);

    // We can always poison someone if we're a ct
    if(isCt())
        poisonchance = 101;

    if(Random::get(1,100) <= poisonchance && !target->chkSave(POI, Containable::downcasted_shared_from_this<Player>(), -1)) {
        // 75% chance to use up the poison
        if(Random::get<bool>(0.75))
            weapon->clearFlag(O_ENVENOMED);

        *this << ColorOn << "^gYou poisoned " << target << "!\n" << ColorOff;
        *target << ColorOn << "^g^#" << setf(CAP) << this << " poisoned you!!\n" << ColorOff;
        broadcast(getSock(), target->getSock(), target->getRoomParent(), "%M poisoned %N!", this, target.get());

        int dur = standardPoisonDuration(weapon->getEffectDuration(), target->constitution.getCur());

        if(weapon->getEffectStrength()) {
            dmg = (Random::get(1,3) + (weapon->getEffectStrength()/10));
            *this << ColorOn << "^gThe poison did ^G" << dmg << "^g onset damage!\n" << ColorOff;
            *target << ColorOn << "^gYou take ^G" << dmg << "^g damage as the poison takes effect!\n" << ColorOff;
        }

        target->poison(Containable::downcasted_shared_from_this<Player>(), weapon->getEffectStrength(), dur);
    }
    return(dmg);
}

//*********************************************************************
//                      cmdAmbush
//*********************************************************************
// This function allows rogues/rangers to ambush their opponents from
// hiding. It is similar to backstab, but somewhat weakened and
// more limited. -- TC

int cmdAmbush(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> pCreature=nullptr;
    std::shared_ptr<Creature> creature=nullptr;


    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("ambush")) {
        *player << "You are not trained in proper ambush techniques.\n";
        return(0);
    }

    if(player->isBlind()) {
        *player << "Ambush whom?\n";
        return(0);
    }

    if(!player->isCt()) {
        if( !player->ready[WIELD-1] ||
            !player->ready[HELD-1] ||
            player->ready[HELD-1]->getType() != ObjectType::WEAPON
        ) {
            *player << "You must have two weapons wielded to properly ambush.\n";
            return(0);
        }

        if(!player->flagIsSet(P_HIDDEN) && !player->isEffected("mist") && !player->isCt()) {
            *player << "How do you expect to ambush anything when you aren't hiding?\n";
            return(0);
        }

        if(!player->checkAttackTimer())
            return(0);

        if(player->checkHeavyRestrict("ambush"))
            return(0);
    }


    if(!(creature = player->findVictim(cmnd, 1, true, false, "Ambush whom?\n", "You don't see that here.\n")))
        return(0);

    if(creature)
        pCreature = creature->getAsPlayer();

    if(!pCreature && creature->getAsMonster()->isEnemy(player) && !player->isCt()) {
        *player << "Not while you're already fighting " << creature->himHer() << ".\n";
        return(0);
    }

    if(!player->canAttack(creature))
        return(0);

    player->smashInvis();

    if(creature->isMonster()) {
        creature->getAsMonster()->addEnemy(player);
    }

    player->updateAttackTimer();

    if(player->dexterity.getCur() > 180)
        player->modifyAttackDelay(-10);

    *player << ColorOn << "You ambush " << creature << "!\n" << ColorOff;
    broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M ambushes %N!", player.get(), creature.get());
    *creature << ColorOn << setf(CAP) << player << " ambushes you!\n" << ColorOff;

    player->unhide();

    player->attackCreature(creature, ATTACK_AMBUSH);

    return(0);
}



//*********************************************************************
//                      cmdPickLock
//*********************************************************************
// This function is called when a thief or assassin attempts to pick a
// lock.  If the lock is pickable, there is a chance (depending on the
// player's level) that the lock will be picked.

int cmdPickLock(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Exit> exit=nullptr;
    long    i=0, t=0;
    int     chance=0;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->knowsSkill("pick")) {
        *player << "You don't know how to pick locks.\n";
        return(0);
    }

    if(!player->isCt()) {
        if(player->flagIsSet(P_SITTING)) {
            player->print("You have to stand up first.\n");
            return(0);
        }
        if(player->isBlind()) {
            *player << ColorOn << "^CHow can you pick a lock when you're blind?\n" << ColorOff;
            return(0);
        }
    }


    if(cmnd->num < 2) {
        *player << "Pick what?\n";
        return(0);
    }
    exit = findExit(player, cmnd);

    if(!exit) {
        *player << "You don't see that here.\n";
        return(0);
    }


    const std::shared_ptr<Monster>  guard = player->getRoomParent()->getGuardingExit(exit, player);
    if(guard && !player->checkStaff("%M %s.\n", guard.get(), guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you pick it" : "won't let you pick it"))
        return(0);

    if(!exit->flagIsSet(X_LOCKED)) {
        *player << "It's not locked.\n";
        return(0);
    }

    if(exit->isWall("wall-of-force")) {
        *player << ColorOn << "The " << exit->getCName() << " is blocked by a ^Mwall-of-force.\n" << ColorOff;
        return(0);
    }
    // were they killed by exit effect damage?
    if(exit->doEffectDamage(player))
        return(0);
    
    player->unhide();

    if(!player->isCt()) {

        i = LT(player, LT_PICKLOCK);
        t = time(nullptr);

        if(t < i) {
            player->pleaseWait(i-t);
            return(0);
        }

        player->lasttime[LT_PICKLOCK].ltime = t;
        player->lasttime[LT_PICKLOCK].interval = (player->getClass() == CreatureClass::ROGUE ? 20 : 10);

    }
    int level = (int)player->getSkillLevel("pick");
    if(player->getClass() == CreatureClass::THIEF)
        chance = 10*(level - exit->getLevel()) + (2*bonus(player->dexterity.getCur()));
    else if(player->getSecondClass() == CreatureClass::THIEF && (player->getClass() == CreatureClass::MAGE || player->getClass() == CreatureClass::FIGHTER) )
        chance = 5*(level - exit->getLevel()) + (2*bonus(player->dexterity.getCur()));
    else if(player->getClass() == CreatureClass::ROGUE || (player->getClass() == CreatureClass::THIEF && player->getSecondClass() == CreatureClass::MAGE))
        chance = 7*(level - exit->getLevel()) + (2*bonus(player->dexterity.getCur()));
    else
        chance = 2*(level - exit->getLevel()) + (2*bonus(player->dexterity.getCur()));


    if(exit->flagIsSet(X_UNPICKABLE))
        chance = 0;

    if((exit->getLevel() > level) && !player->isCt()) {
        *player << "The lock's mechanism is currently beyond your experience.\n";
        chance = 0;
    }

    chance = std::max(0, chance);

    if(player->isCt())
        chance = 101;

    broadcast(player->getSock(), player->getParent(), "%M attempts to pick the %s^x.", player.get(), exit->getCName());

    if(Random::get(1,100) <= chance) {
        log_immort(false, player, "%s picked the %s in room %s.\n", player->getCName(), exit->getCName(),
            player->getRoomParent()->fullName().c_str());

        *player << "You successfully picked the lock.\n";
        player->checkImprove("pick", true);
        exit->clearFlag(X_LOCKED);
        broadcast(player->getSock(), player->getParent(), "%s succeeded.", player->upHeShe());

        Hooks::run(player, "succeedPickExit", exit, "succeedPickByCreature");

    } else {
        *player << "You failed.\n";
        player->checkImprove("pick", false);

        Hooks::run(player, "failPickExit", exit, "failPickByCreature");
    }

    return(0);
}


//*********************************************************************
//                      cmdPeek
//*********************************************************************
// This function allows a thief to peek at the inventory of
// another player.  If successful, they will be able to see it and
// another roll is made to see if they get caught.

int cmdPeek(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    std::shared_ptr<Player> pCreature=nullptr;
    std::shared_ptr<Monster>  mCreature=nullptr;
    std::string str = "";
    long    i=0, t=0;
    int     chance=0, goldchance=0, ok=0;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->isStaff() && !player->knowsSkill("peek")) {
        *player << "You don't know how to peek at other people's inventories.\n";
        return(0);
    }

    if(cmnd->num < 2) {
        *player << "Peek at who?\n";
        return(0);
    }

    int level = (int)player->getSkillLevel("peek");

    if(player->isBlind()) {
        *player << ColorOn << "^CYou can't do that! You're blind!\n" << ColorOff;
        return(0);
    }

    creature = player->getParent()->findCreature(player, cmnd);
    if(!creature || creature == player) {
        *player << "That person is not here.\n";
        return(0);
    }
    mCreature = creature->getAsMonster();
    pCreature = creature->getAsPlayer();

    if(player->getClass() == CreatureClass::BUILDER) {
        if(pCreature) {
            *player << "You cannot peek players.\n";
            return(0);
        }
    if(!player->canBuildMonsters() && !player->canBuildObjects())
            return(cmdNoAuth(player));
        if(!player->checkBuilder(player->getUniqueRoomParent())) {
            *player << ColorOn << "^yError: Room number not inside any of your alotted ranges.\n" << ColorOff;
            return(0);
        }
        if(mCreature && mCreature->info.id && !player->checkBuilder(mCreature->info)) {
            *player << ColorOn << "^yError: Monster not inside any of your alotted ranges.\n" << ColorOff;
            return(0);
        }
    }

    if(!pCreature && player->isStaff())
        return(dmMobInventory(player, cmnd));


    if(pCreature && pCreature->isEffected("mist")) {
        *player << "You cannot peek at the inventory of a mist.\n";
        return(0);
    }

    if(player->checkHeavyRestrict("peek"))
        return(0);

    i = LT(player, LT_PEEK);
    t = time(nullptr);

    if(i > t && !player->isStaff()) {
        player->pleaseWait(i-t);
        return(0);
    }

    player->lasttime[LT_PEEK].ltime = t;
    player->lasttime[LT_PEEK].interval = 5;

    if(!player->isCt() && creature->isStaff()) {
        *player << "You failed.\n";
        return(0);
    }

    if(!pCreature && (creature->flagIsSet(M_CANT_BE_STOLEN_FROM) || creature->flagIsSet(M_TRADES) || creature->flagIsSet(M_CAN_PURCHASE_FROM)) && !player->isDm()) {
        *player << setf(CAP) << creature << " manages to keep " << creature->hisHer() << " inventory hidden from you.\n";
        return(0);
    }

    if(cmnd->num > 2 && pCreature && (player->getClass() == CreatureClass::THIEF || player->isCt())) {
        peek_bag(player, pCreature, cmnd, 0);
        return(0);
    }

    if(!ok && (player->getClass() == CreatureClass::THIEF || player->isStaff()))
        chance = (25 + level*10)-(creature->getLevel()*5);
    else
        chance = (level*10)-(creature->getLevel()*5);

    if(chance<0)
        chance=0;

    if(creature->getLevel() >= level + 6)
        chance = 0;

    if(player->isStaff())
        chance=100;
    if(Random::get(1,100) > chance) {
        player->print("You failed.\n");
        if(Random::get(1,50) <= 50)
            player->unhide();
        player->checkImprove("peek", false);
        return(0);
    }

    chance = std::min(90, (player->getClass() == CreatureClass::ASSASSIN ? (level*4):(15 + level*5)));

    if(Random::get(1,100) > chance && !player->isStaff()) {
        *creature << setf(CAP) << player << " peeked at your inventory.\n";
        broadcast(player->getSock(), creature->getSock(), player->getRoomParent(),
            "%M peeked at %N's inventory.", player.get(), creature.get());
        *player << creature->upHeShe() << " noticed!\n";
    } else {
        *player << creature->upHeShe() << "'s oblivious to your rummaging.\n";
    }
    player->checkImprove("peek", true);

    str = creature->listObjects(player, player->isStaff());
    if(!str.empty())
        *player << ColorOn << creature->upHeShe() << " is carrying: " << str << ".\n" << ColorOff;
    else
        *player << creature->upHeShe() << " isn't holding anything.\n";


    goldchance = (5+(level*5)) - creature->getLevel();
    goldchance = std::min(std::max(1,goldchance),90);

    if(player->isCt())
        goldchance = 101;

    if(Random::get(1,100) <= goldchance && !pCreature && !creature->flagIsSet(M_PERMENANT_MONSTER))
        *player << ColorOn << creature->upHeShe() << " has ^y" << creature->coins[GOLD] << "^x gold coins.\n" << ColorOff;

    return(0);
}


//*********************************************************************
//                      peek_bag
//*********************************************************************

int peek_bag(std::shared_ptr<Player> player, std::shared_ptr<Player> target, cmd* cmnd, int inv) {
    std::shared_ptr<Object> container=nullptr;
    std::string str = "";
    int     chance=0;

    if(!player->isStaff()) {
        int level = (int)player->getSkillLevel("peek");

        if(level < 10) {
            *player << "You are are not experienced enough at peeking to do that.\n";
            return(0);
        }

        chance = (5 + level*10)-(target->getLevel()*5);
        if(chance<0)
            chance=0;

        if(target->getLevel() >= level + 6)
            chance = 0;

        if(Random::get(1,100) > chance) {
            *player << "You failed.\n";
            player->checkImprove("peek", false);
            return(0);
        }
    }

    container = target->findObject(player, cmnd, 2);

    if(!container) {
        *player << target->upHeShe() << " doesn't have that.\n";
        return(0);
    }

    if(container->getType() != ObjectType::CONTAINER) {
        *player << "That isn't a container.\n";
        return(0);
    }

    if(!inv) {
        if(Random::get(1,100) > chance && !player->isStaff()) {

            *player << ColorOn << "You manage to peek inside " << target << "'s " << container->getCName() << ".\n" << ColorOff;
            *target << ColorOn << setf(CAP) << player << " managed to peek inside your " << container->getCName() << "!\n" << ColorOff;
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M peeked at %N's inventory.",
                player.get(), target.get());
            *player << ColorOn << target->upHeShe() << " noticed!\n" << ColorOff;

        } else {
            *player << ColorOn << "You manage to peek inside " << target << "'s " << container->getCName() << ".\n" << ColorOff;
            *player << target->upHeShe() << "'s oblivious to your rummaging.\n";
        }
        player->checkImprove("peek", true);
    }

    if(container->getType() == ObjectType::CONTAINER) {
        str = container->listObjects(player, false);
        if(!str.empty())
            *player << ColorOn << "It contains: " << str << ".\n" << ColorOff;
        else
            *player << "It is empty.\n";
    }

    return(0);
}

