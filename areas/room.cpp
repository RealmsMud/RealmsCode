/*
 * room.cpp
 *   Room routines.
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
#include <cstring>                     // for strcpy
#include <ctime>                       // for time, ctime
#include <list>                        // for list, operator==, _List_iterator
#include <map>                         // for operator==, map, _Rb_tree_cons...
#include <set>                         // for set, set<>::iterator
#include <sstream>                     // for operator<<, basic_ostream, ost...
#include <string>                      // for allocator, operator<<, char_tr...
#include <utility>                     // for pair

#include "area.hpp"                    // for Area, MapMarker
#include "catRef.hpp"                  // for CatRef
#include "config.hpp"                  // for Config, gConfig
#include "creatureStreams.hpp"         // for Streamable, operator<<, setf
#include "effects.hpp"                 // for EffectInfo
#include "flags.hpp"                   // for M_PERMENANT_MONSTER, O_JUST_BO...
#include "global.hpp"                  // for MAG, CreatureClass, CAP, Creat...
#include "hooks.hpp"                   // for Hooks
#include "lasttime.hpp"                // for crlasttime, lasttime
#include "location.hpp"                // for Location
#include "move.hpp"                    // for deletePortal
#include "mud.hpp"                     // for DL_BROAD, LT_AGGRO_ACTION
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/container.hpp"    // for MonsterSet, PlayerSet, ObjectSet
#include "mudObjects/creatures.hpp"    // for Creature, DEL_PORTAL_DESTROYED
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/objects.hpp"      // for Object, DroppedBy, ObjectType
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "property.hpp"                // for Property
#include "proto.hpp"                   // for broadcast, isCt, logn, isStaff
#include "random.hpp"                  // for Random
#include "server.hpp"                  // for Server, gServer
#include "stats.hpp"                   // for Stat
#include "structs.hpp"                 // for daily
#include "utils.hpp"                   // for MAX, MIN
#include "xml.hpp"                     // for loadMonster, loadObject, loadRoom


//*********************************************************************
//                      addToSameRoom
//*********************************************************************
// This function is called to add a player to a room. It inserts the
// player into the room's linked player list, alphabetically. Also,
// the player's room pointer is updated.

void Player::addToSameRoom(const std::shared_ptr<Creature>& target) {
    if(target->inRoom())
        addToRoom(target->getRoomParent());
}



void Player::finishAddPlayer(const std::shared_ptr<BaseRoom>& room) {

    setFleeing(false);

    wake("You awaken suddenly!");
    interruptDelayedActions();

    if(!gServer->isRebooting()) {

        if(!flagIsSet(P_DM_INVIS) && !flagIsSet(P_HIDDEN) && !isEffected("mist") ) {
            broadcast(getSock(), room, "%M just arrived.", this);
        } else if(isEffected("mist") && !flagIsSet(P_SNEAK_WHILE_MISTED)) {
            broadcast(getSock(), room, "A light mist just arrived.");
        } else {
            if(isDm())
                broadcast(::isDm, getSock(), room, "*DM* %M just arrived.", this);
            if(cClass == CreatureClass::CARETAKER)
                broadcast(::isCt, getSock(), room, "*DM* %M just arrived.", this);
            if(!isCt())
                broadcast(::isStaff, getSock(), room, "*DM* %M just arrived.", this);
        }

        if(!isStaff()) {
            if((isEffected("darkness") || flagIsSet(P_DARKNESS)) && !room->flagIsSet(R_MAGIC_DARKNESS))
                broadcast(getSock(), room, "^DA globe of darkness just arrived.");
        }
    }

    if(flagIsSet(P_SNEAK_WHILE_MISTED))
        clearFlag(P_SNEAK_WHILE_MISTED);
    setLastPawn(nullptr);


    for(const auto& obj : objects) {
        if(obj->getType() == ObjectType::CONTAINER) {
            for(const auto& subObj : obj->objects) {
                if(subObj->flagIsSet(O_JUST_BOUGHT))
                    subObj->clearFlag(O_JUST_BOUGHT);
            }
        } else {
            if(obj->flagIsSet(O_JUST_BOUGHT))
                obj->clearFlag(O_JUST_BOUGHT);
        }
    }

    clearFlag(P_JUST_REFUNDED); // Player didn't just refund something (can haggle again)
    clearFlag(P_JUST_TRAINED); // Player can train again.
    clearFlag(P_CAUGHT_SHOPLIFTING); // No longer in the act of shoplifting.
    clearFlag(P_LAG_PROTECTION_OPERATING); // Lag detection routines deactivated.

    if(flagIsSet(P_SITTING))
        stand();


    // Clear the enemy list of pets when leaving the room
    for(const auto& pet : pets) {
        if(pet->isPet()) {
            pet->clearEnemyList();
        }
    }


    // don't close exits if we're rebooting
    if(!isCt() && !gServer->isRebooting())
        room->checkExits();


    if(room->players.empty()) {
        for(const auto& mons : room->monsters) {
            gServer->addActive(mons);
        }
    }

    addTo(room);
    display_rom(Containable::downcasted_shared_from_this<Player>());

    Hooks::run(room, "afterAddCreature", Containable::downcasted_shared_from_this<Player>(), "afterAddToRoom");
}

void Player::addToRoom(const std::shared_ptr<BaseRoom>& room) {
    std::shared_ptr<AreaRoom> aRoom = room->getAsAreaRoom();

    if(aRoom)
        addToRoom(aRoom);
    else
        addToRoom(room->getAsUniqueRoom());
}

void Player::addToRoom(const std::shared_ptr<AreaRoom>& aRoom) {
    Hooks::run(aRoom, "afterAddCreature", Containable::downcasted_shared_from_this<Player>(), "afterAddToRoom");
    currentLocation.room.clear();
    finishAddPlayer(aRoom);
}

void Player::addToRoom(const std::shared_ptr<UniqueRoom>& uRoom) {
    bool    builderInRoom=false;

    Hooks::run(uRoom, "beforeAddCreature", Containable::downcasted_shared_from_this<Player>(), "beforeAddToRoom");
    currentLocation.room = uRoom->info;
    currentLocation.mapmarker.reset();

    // So we can see an accurate this count.
    if(!isStaff())
        uRoom->incBeenHere();

    if(uRoom->getRoomExperience()) {
        bool beenHere=false;
        std::list<CatRef>::iterator it;

        for(it = roomExp.begin() ; it != roomExp.end() ; it++) {
            if(uRoom->info == (*it)) {
                beenHere = true;
                break;
            }
        }

        if(!beenHere) {
            if(!halftolevel()) {
                print("You %s experience for entering this place for the first time.\n", gConfig->isAprilFools() ? "lose" : "gain");
                addExperience(uRoom->getRoomExperience());
            } else {
                print("You have entered this place for the first time.\n");
            }
            roomExp.push_back(uRoom->info);
        }
    }


    // Builders cannot leave their areas unless escorted by a DM.
    // Any other time they leave their areas it will be logged.
    if( cClass== CreatureClass::BUILDER &&
        (   !getGroupLeader() ||
            (getGroupLeader() && getGroupLeader()->isDm()) ) &&
        checkRangeRestrict(uRoom->info))
    {
        // only log if another builder is not in the room
        for(const auto& ply: uRoom->players) {
            if( ply.get() != this &&
                ply->getClass() == CreatureClass::BUILDER)
            {
                builderInRoom = true;
            }
        }
        if(!builderInRoom) {
            checkBuilder(uRoom);
            printColor("^yYou are illegally out of your assigned area. This has been logged.\n");
            broadcast(::isCt, "^y### %s is illegally out of %s assigned area. (%s)",
                getCName(), hisHer(), uRoom->info.displayStr().c_str());
            logn("log.builders", "%s illegally entered room %s - (%s).\n", getCName(),
                 uRoom->info.displayStr().c_str(), uRoom->getCName());
        }
    }


    uRoom->addPermCrt();
    uRoom->addPermObj();
    uRoom->validatePerms();

    finishAddPlayer(uRoom);
    updateRooms(uRoom);
}

//*********************************************************************
//                      setPreviousRoom
//*********************************************************************
// This function records what room the player was last in

void Creature::setPreviousRoom() {
    if(inUniqueRoom()) {
        if(!getUniqueRoomParent()->flagIsSet(R_NO_PREVIOUS)) {
            previousRoom.room = getUniqueRoomParent()->info;
            previousRoom.mapmarker.reset();
        }
    } else if(inAreaRoom()) {
        if(!getAreaRoomParent()->flagIsSet(R_NO_PREVIOUS)) {
            previousRoom.room.clear();
            previousRoom.mapmarker = getAreaRoomParent()->mapmarker;
        }
    }
}

//*********************************************************************
//                      deleteFromRoom
//*********************************************************************
// This function removes a player from a room's linked list of players.

int Creature::deleteFromRoom(bool delPortal) {
    Hooks::run(getRoomParent(), "beforeRemoveCreature", Containable::downcasted_shared_from_this<Creature>(), "beforeRemoveFromRoom");

    setPreviousRoom();

    if(inUniqueRoom()) {
        return(doDeleteFromRoom(getUniqueRoomParent(), delPortal));
    } else if(inAreaRoom()) {
        std::shared_ptr<AreaRoom> room = getAreaRoomParent();
        int i = doDeleteFromRoom(room, delPortal);
        if(room->canDelete()) {
            if(auto roomArea = room->area.lock()) {
                roomArea->remove(room);
                i |= DEL_ROOM_DESTROYED;
            }
        }
        return(i);
    }
    return(0);
}

int Player::doDeleteFromRoom(std::shared_ptr<BaseRoom> room, bool delPortal) {
    long    t=0;
    int     i=0;

    t = time(nullptr);
    if(inUniqueRoom() && !isStaff()) {
        strcpy(getUniqueRoomParent()->lastPly, getCName());
        strcpy(getUniqueRoomParent()->lastPlyTime, ctime(&t));
    }

    currentLocation.mapmarker.reset();
    currentLocation.room.clear();

    if(delPortal && flagIsSet(P_PORTAL) && Move::deletePortal(room, getName()))
        i |= DEL_PORTAL_DESTROYED;

    // when we're removing them from the room - AreaRoom, usually -
    // but they were never added to the player list. this happens
    // in dmMove for offline players.
    if(room->players.empty()) {
        Hooks::run(room, "afterRemoveCreature", Containable::downcasted_shared_from_this<Player>(), "afterRemoveFromRoom");
        return(i);
    }

    removeFrom();

    if(room->players.empty()) {
        for(const auto& mons : room->monsters) {
            /*
             * pets and fast wanderers are always active
             *
            */
            if( !mons->flagIsSet(M_FAST_WANDER) &&
                !mons->isPet() &&
                !mons->isPoisoned() &&
                !mons->flagIsSet(M_FAST_TICK) &&
                !mons->flagIsSet(M_ALWAYS_ACTIVE) &&
                !mons->flagIsSet(M_REGENERATES) &&
                !mons->isEffected("slow") &&
                !mons->flagIsSet(M_PERMENANT_MONSTER) &&
                !mons->flagIsSet(M_AGGRESSIVE)
            )
                gServer->delActive(mons.get());
        }
    }

    Hooks::run(room, "afterRemoveCreature", Containable::downcasted_shared_from_this<Player>(), "afterRemoveFromRoom");
    return(i);
}

//*********************************************************************
//                      add_obj_rom
//*********************************************************************
// This function adds the object pointed to by the first parameter to
// the object list of the room pointed to by the second parameter.
// The object is added alphabetically to the room.

void Object::addToRoom(const std::shared_ptr<BaseRoom>& room) {
    validateId();

    Hooks::run(room, "beforeAddObject", Containable::downcasted_shared_from_this<Object>(), "beforeAddToRoom");
    clearFlag(O_KEEP);
    room->add(Containable::downcasted_shared_from_this<Object>());
    Hooks::run(room, "afterAddObject", Containable::downcasted_shared_from_this<Object>(), "afterAddToRoom");
    room->killMortalObjects();
}

//*********************************************************************
//                      deleteFromRoom
//*********************************************************************
// This function removes the object pointer to by the first parameter
// from the room pointed to by the second.

void Object::deleteFromRoom() {
    if(!inRoom())
        return;
    std::shared_ptr<BaseRoom> room = this->getRoomParent();

    Hooks::run(room, "beforeRemoveObject", Containable::downcasted_shared_from_this<Object>(), "beforeRemoveFromRoom");
    removeFrom();
    Hooks::run(room, "afterRemoveObject", Containable::downcasted_shared_from_this<Object>(), "afterRemoveFromRoom");
}

//*********************************************************************
//                      addToRoom
//*********************************************************************
// This function adds the monster pointed to by the first parameter to
// the room pointed to by the second. The third parameter determines
// how the people in the room should be notified. If it is equal to
// zero, then no one will be notified that a monster entered the room.
// If it is non-zero, then the room will be told that "num" monsters
// of that name entered the room.

void Monster::addToRoom(const std::shared_ptr<BaseRoom>& room, int num) {
    Hooks::run(room, "beforeAddCreature", Containable::downcasted_shared_from_this<Monster>(), "beforeAddToRoom");

    char    str[160];
    validateId();

    lasttime[LT_AGGRO_ACTION].ltime = time(nullptr);
    killDarkmetal();

    // Only show if num != 0 and it isn't a perm, otherwise we'll either
    // show to staff or players
    if(num != 0 && !flagIsSet(M_PERMENANT_MONSTER)) {
        if(!flagIsSet(M_NO_SHOW_ARRIVE) && !isInvisible()
            && !flagIsSet(M_WAS_PORTED) )
        {
            sprintf(str, "%%%dM just arrived.", num);
            broadcast(getSock(), room, str, this);
        } else {
            sprintf(str, "*DM* %%%dM just arrived.", num);
            broadcast(::isStaff, getSock(), room, str, this);
        }
    }
    // Handle sneaking mobs
    if(flagIsSet(M_SNEAKING)) {
        setFlag(M_NO_SHOW_ARRIVE);
        clearFlag(M_SNEAKING);
    } else
        clearFlag(M_NO_SHOW_ARRIVE);

    // Handle random aggressive monsters
    if(!flagIsSet(M_AGGRESSIVE)) {
        if(loadAggro && (Random::get(1,100) <= MAX<unsigned short>(1, loadAggro)))
            setFlag(M_WILL_BE_AGGRESSIVE);
    }

    addTo(room);
//  room->monsters.insert(this);
    Hooks::run(room, "afterAddCreature", Containable::downcasted_shared_from_this<Monster>(), "afterAddToRoom");
}

//*********************************************************************
//                      deleteFromRoom
//*********************************************************************
// This function removes the monster pointed to by the first parameter
// from the room pointed to by the second.
// Return Value: True if the area room was purged
//               False otherwise

int Monster::doDeleteFromRoom(std::shared_ptr<BaseRoom> room, bool delPortal) {
    if(!room)
        return(0);
    if(room->monsters.empty())
        return(0);

    removeFrom();
    Hooks::run(room, "afterRemoveCreature", Containable::downcasted_shared_from_this<Monster>(), "afterRemoveFromRoom");
    return(0);
}

//********************************************************************
//              addPermCrt
//********************************************************************
// This function checks a room to see if any permanent monsters need to
// be loaded. If so, the monsters are loaded to the room, and their
// permanent flag is set.

void UniqueRoom::addPermCrt() {
    std::map<int, crlasttime>::iterator it, nt;
    crlasttime* crtm;
    std::map<int, bool> checklist;
    std::shared_ptr<Monster> monster=nullptr;
    long    t = time(nullptr);
    int     j=0, m=0, n=0;

    for(it = permMonsters.begin(); it != permMonsters.end() ; it++) {
        crtm = &(*it).second;

        if(checklist[(*it).first])
            continue;
        if(!crtm->cr.id)
            continue;
        if(crtm->ltime + crtm->interval > t)
            continue;

        n = 1;
        m = 0;
        nt = it;
        nt++;
        for(; nt != permMonsters.end() ; nt++) {
            if( crtm->cr == (*nt).second.cr &&
                ((*nt).second.ltime + (*nt).second.interval) < t
            ) {
                n++;
                checklist[(*nt).first] = true;
            }
        }

        if(!loadMonster(crtm->cr, monster))
            continue;

        for(const auto& mons : monsters) {
            if( mons->flagIsSet(M_PERMENANT_MONSTER) && mons->getName() == monster->getName() )
                m++;
        }

        monster.reset();

        for(j=0; j<n-m; j++) {

            if(!loadMonster(crtm->cr, monster))
                continue;

            monster->initMonster();
            monster->setFlag(M_PERMENANT_MONSTER);
            monster->daily[DL_BROAD].cur = 20;
            monster->daily[DL_BROAD].max = 20;

            monster->validateAc();
            monster->addToRoom(BaseRoom::downcasted_shared_from_this<UniqueRoom>(), 0);

            if(!players.empty())
                gServer->addActive(monster);
        }
    }
}


//*********************************************************************
//                      addPermObj
//*********************************************************************
// This function checks a room to see if any permanent objects need to
// be loaded.  If so, the objects are loaded to the room, and their
// permanent flag is set.

void UniqueRoom::addPermObj() {
    std::map<int, crlasttime>::iterator it, nt;
    crlasttime* crtm=nullptr;
    std::map<int, bool> checklist;
    std::shared_ptr<Object> object=nullptr;
    long    t = time(nullptr);
    int     j=0, m=0, n=0;

    for(it = permObjects.begin(); it != permObjects.end() ; it++) {
        crtm = &(*it).second;

        if(checklist[(*it).first])
            continue;
        if(!crtm->cr.id)
            continue;
        if(crtm->ltime + crtm->interval > t)
            continue;

        n = 1;
        m = 0;
        nt = it;
        nt++;
        for(; nt != permObjects.end() ; nt++) {
            if( crtm->cr == (*nt).second.cr &&
                ((*nt).second.ltime + (*nt).second.interval) < t )
            {
                n++;
                checklist[(*nt).first] = true;
            }
        }

        if(!loadObject(crtm->cr, object))
            continue;

        for(const auto& obj : objects) {
            if(obj->flagIsSet(O_PERM_ITEM)) {
                if(obj->getName() == object->getName() && obj->info == object->info)
                    m++;
                else if( object->getName() == obj->droppedBy.getName() &&
                        object->info.str() == obj->droppedBy.getIndex())
                    m++;
                }
        }

        object.reset();

        for(j=0; j<n-m; j++) {
            if(!loadObject(crtm->cr, object))
                continue;
        if (!object->randomObjects.empty())
             object->init();
        else
             object->setDroppedBy(shared_from_this(), "PermObject");


            if(object->flagIsSet(O_RANDOM_ENCHANT))
                object->randomEnchant();

            object->setFlag(O_PERM_ITEM);


            object->addToRoom(BaseRoom::downcasted_shared_from_this<UniqueRoom>());
        }
    }
}

//*********************************************************************
//                      roomEffStr
//*********************************************************************

std::string roomEffStr(const std::string& effect, std::string str, const std::shared_ptr<BaseRoom>& room, bool detectMagic) {
    if(!room->isEffected(effect))
        return("");
    if(detectMagic) {
        EffectInfo* eff = room->getEffect(effect);
        if(!eff->getOwner().empty()) {
            str += " (cast by ";
            str += eff->getOwner();
            str += ")";
        }
    }
    str += ".\n";
    return(str);
}

//*********************************************************************
//                      displayRoom
//*********************************************************************
// This function displays the descriptions of a room, all the players
// in a room, all the monsters in a room, all the objects in a room,
// and all the exits in a room.  That is, unless they are not visible
// or the room is dark.

void displayRoom(const std::shared_ptr<Player>& player, const std::shared_ptr<BaseRoom>& room, int magicShowHidden) {
    std::shared_ptr<UniqueRoom> target=nullptr;
    char    name[256];
    int     n, m,  staff=0;
    unsigned int flags = (player->displayFlags() | QUEST);
    std::ostringstream oStr;
    std::string str;
    bool    wallOfFire=false, wallOfThorns=false, canSee=false;

    const std::shared_ptr<const UniqueRoom> uRoom = room->getAsConstUniqueRoom();
    const std::shared_ptr<const AreaRoom> aRoom = room->getAsConstAreaRoom();
    strcpy(name, "");

    staff = player->isStaff();

    player->print("\n");

    if( player->isEffected("petrification") &&
        !player->checkStaff("You can't see anything. You're petrified!")
    )
        return;

    if(!player->canSeeRoom(room, true))
        return;

    oStr << (!player->flagIsSet(P_NO_EXTRA_COLOR) && room->isSunlight() ? "^C" : "^c");

    if(uRoom) {

        if(staff)
            oStr << uRoom->info.displayStr() << " - ";
        oStr << uRoom->getName() << "^x\n\n";

        if(!uRoom->getShortDescription().empty())
            oStr << uRoom->getShortDescription() << "\n";

        if(!uRoom->getLongDescription().empty())
            oStr << uRoom->getLongDescription() << "\n";

    } else if(auto roomArea = aRoom->area.lock()) {

        if(!roomArea->name.empty()) {
            oStr << roomArea->name;
            if(player->isCt())
                oStr << " " << aRoom->fullName();
            oStr << "^x\n\n";
        }

        oStr << roomArea->showGrid(player, aRoom->mapmarker, player->getAreaRoomParent() == aRoom);
    }

    oStr << "^g" << (staff ? "All" : "Obvious") << " exits: ";
    n=0;

    str = "";
    for(const auto& ext : room->exits) {
        wallOfFire = ext->isWall("wall-of-fire");
        wallOfThorns = ext->isWall("wall-of-thorns");

        canSee = player->showExit(ext, magicShowHidden);
        if(canSee) {
            if(n)
                oStr << "^g, ";

            // first, check P_NO_EXTRA_COLOR
            // putting loadRoom in the boolean statement means that, if
            // canEnter returns false, it will save us on running the
            // function every once in a while
            oStr << "^";
            if(wallOfFire) {
                oStr << "R";
            } else if(wallOfThorns) {
                oStr << "o";
            } else if(  !player->flagIsSet(P_NO_EXTRA_COLOR) &&
                (   !player->canEnter(ext) || (
                        ext->target.room.id && (
                            !loadRoom(ext->target.room, target) ||
                            !target ||
                            !player->canEnter(target)
                        )
                    )
                )
            ) {
                oStr << "G";
            } else {
                oStr << "g";
            }
            oStr << ext->getName();

            if(ext->flagIsSet(X_CLOSED) || ext->flagIsSet(X_LOCKED)) {
                oStr << "[";
                if(ext->flagIsSet(X_CLOSED))
                    oStr << "c";
                if(ext->flagIsSet(X_LOCKED))
                    oStr << "l";
                oStr << "]";
            }

            if(staff) {
                if(ext->flagIsSet(X_SECRET))
                    oStr << "(h)";
                if(ext->flagIsSet(X_CAN_LOOK) || ext->flagIsSet(X_LOOK_ONLY))
                    oStr << "(look)";
                if(ext->flagIsSet(X_NO_WANDER))
                    oStr << "(nw)";
                if(ext->flagIsSet(X_NO_SEE))
                    oStr << "(dm)";
                if(ext->isEffected("invisibility"))
                    oStr << "(*)";
                if(ext->isConcealed(player))
                    oStr << "(c)";
                if(ext->flagIsSet(X_DESCRIPTION_ONLY))
                    oStr << "(desc)";
                if(ext->flagIsSet(X_NEEDS_FLY))
                    oStr << "(fly)";
            } else {
                // if player can see the exit and these flags are set, then the player has discovered and used this exit before.
                if(ext->flagIsSet(X_SECRET) || ext->flagIsSet(X_DESCRIPTION_ONLY) || ext->isConcealed(player))
                    oStr << "(h)";
                if(ext->isEffected("invisibility"))
                    oStr << "(*)";
                if(ext->flagIsSet(X_NEEDS_FLY))
                    oStr << "(fly)";
            }

            n++;
        }

        if(wallOfFire)
            str += ext->blockedByStr('R', "wall of fire", "wall-of-fire", flags & MAG, canSee);
        if(ext->isWall("wall-of-force"))
            str += ext->blockedByStr('s', "wall of force", "wall-of-force", flags & MAG, canSee);
        if(wallOfThorns)
            str += ext->blockedByStr('o', "wall of thorns", "wall-of-thorns", flags & MAG, canSee);

    }

    if(!n)
        oStr << "none";
    // change colors
    oStr << "^g.\n";


    // room auras
    if(player->isEffected("know-aura")) {
        if(!room->isEffected("unhallow"))
            oStr << roomEffStr("hallow", "^YA holy aura fills the room", room, flags & MAG);
        if(!room->isEffected("hallow"))
            oStr << roomEffStr("unhallow", "^DAn unholy aura fills the room", room, flags & MAG);
    }

    oStr << roomEffStr("dense-fog", "^WA dense fog fills the room", room, flags & MAG);
    oStr << roomEffStr("toxic-cloud", "^GA toxic cloud fills the room", room, flags & MAG);
    oStr << roomEffStr("globe-of-silence", "^DAn unnatural silence hangs in the air", room, flags & MAG);


    oStr << str << "^c";
    str = "";
    n = 0;
    for(const auto& ply : room->players) {
        if(ply != player && player->canSee(ply)) {

            // other non-vis rules
            if(!staff) {
                if(ply->flagIsSet(P_HIDDEN)) {
                    // if we're using magic to see hidden creatures
                    if(!magicShowHidden)
                        continue;
                    if(ply->isEffected("resist-magic")) {
                        // if resisting magic, we use the strength of each spell to
                        // determine if they are seen
                        EffectInfo* effect = ply->getEffect("resist-magic");
                        if(effect->getStrength() >= magicShowHidden)
                            continue;
                    }
                }
            }


            if(n)
                oStr << ", ";
            else
                oStr << "You see ";

            oStr << ply->fullName();

            if(ply->isStaff())
                oStr << " the " << ply->getTitle();

            if(ply->flagIsSet(P_SLEEPING))
                oStr << "(sleeping)";
            else if(ply->flagIsSet(P_UNCONSCIOUS))
                oStr << "(unconscious)";
            else if(ply->flagIsSet(P_SITTING))
                oStr << "(sitting)";

            if(ply->flagIsSet(P_AFK))
                oStr << "(afk)";

            if(ply->isEffected("petrification"))
                oStr << "(statue)";

            if(staff) {
                if(ply->flagIsSet(P_HIDDEN))
                    oStr << "(h)";
                if(ply->isInvisible())
                    oStr << "(*)";
                if(ply->isEffected("mist"))
                    oStr << "(m)";
                if(ply->flagIsSet(P_OUTLAW))
                    oStr << "(o)";
            }
            n++;
        }
    }

    if(n)
        oStr << ".\n";
    oStr << "^m";

    n=0;

    auto mIt = room->monsters.begin();
    while(mIt != room->monsters.end()) {
        auto creature = (*mIt++);

        if(staff || (player->canSee(creature) && (!creature->flagIsSet(M_HIDDEN) || magicShowHidden))) {
            m=1;
            while(mIt != room->monsters.end()) {
                if( (*mIt)->getName() == creature->getName() &&
                    (staff || (player->canSee(*mIt) && (!(*mIt)->flagIsSet(M_HIDDEN) || magicShowHidden))) &&
                    creature->isInvisible() == (*mIt)->isInvisible() )
                {
                    m++;
                    mIt++;
                } else
                    break;
            }

            oStr << (n ? ", " : "You see ") << creature->getCrtStr(player, flags, m);

            if(staff) {
                if(creature->flagIsSet(M_HIDDEN))oStr << "(h)";
                if(creature->isInvisible())      oStr << "(*)";
            }
            n++;
        }
    }

    if(n) oStr << ".\n";

    str = room->listObjects(player, false, 'y');
    if(!str.empty())
        oStr << "^yYou see " << str << ".^w\n";

    *player << ColorOn << oStr.str();

    for(const auto& mons : room->monsters) {
        if(mons && mons->hasEnemy()) {
            auto creature = mons->getTarget();


            if(creature == player)
                *player << "^r" << setf(CAP) << mons << " is attacking you.\n";
            else if(creature)
                *player << "^r" << setf(CAP) << mons << " is attacking " << setf(CAP) << creature << ".\n";
        }
    }

    player->print("^x\n");
}

void display_rom(const std::shared_ptr<Player>& player, std::shared_ptr<Player> looker, int magicShowHidden) {
    if(!looker)
        looker = player;
    displayRoom(looker, player->getRoomParent(), magicShowHidden);
}

void display_rom(const std::shared_ptr<Player> &player, std::shared_ptr<BaseRoom> &room) {
    displayRoom(player, room, 0);
}


//*********************************************************************
//                      createStorage
//*********************************************************************
// This function creates a storage room. Setting all the flags, and
// putting in a generic description

void storageName(const std::shared_ptr<UniqueRoom>& room, const std::shared_ptr<Player>& player) {
    room->setName(player->getName() + "'s Personal Storage Room");
}

int createStorage(CatRef cr, const std::shared_ptr<Player>& player) {
    std::shared_ptr<UniqueRoom> newRoom = std::make_shared<UniqueRoom>();
    std::string desc;

    newRoom->info = cr;
    storageName(newRoom, player);

    newRoom->setShortDescription("You are in your personal storage room.");
    newRoom->setLongDescription("The realtor's office has granted you this space in order to store excess\nequipment. Since you have paid quite a bit for this room, you may keep it\nindefinitely, and you may access it from any storage room shop in the\ngame. However, you may occasionally have to buy a new key, which you may\npurchase in the main Office. The chest on the floor is for your use. It\nwill hold 100 items, including those of the same type. For now you may\nonly have one chest. The realtor's office will possibly offer more then\none chest at a future time. You will be informed when this occurs.");

    CatRef sr;
    sr.id = 2633;
    link_rom(newRoom, sr, "out");
    newRoom->exits.front()->setFlag(X_TO_PREVIOUS);

    // reuse catref
    cr.id = 1;
    newRoom->permObjects[0].cr =
    newRoom->permObjects[1].cr =
    newRoom->permObjects[2].cr =
    newRoom->permObjects[3].cr =
    newRoom->permObjects[4].cr =
    newRoom->permObjects[5].cr =
    newRoom->permObjects[6].cr =
    newRoom->permObjects[7].cr =
    newRoom->permObjects[8].cr =
    newRoom->permObjects[9].cr = cr;

    newRoom->permObjects[0].interval =
    newRoom->permObjects[1].interval =
    newRoom->permObjects[2].interval =
    newRoom->permObjects[3].interval =
    newRoom->permObjects[4].interval =
    newRoom->permObjects[5].interval =
    newRoom->permObjects[6].interval =
    newRoom->permObjects[7].interval =
    newRoom->permObjects[8].interval =
    newRoom->permObjects[9].interval = (long)10;

    newRoom->setFlag(R_IS_STORAGE_ROOM);
    newRoom->setFlag(R_FAST_HEAL);
    newRoom->setFlag(R_NO_SUMMON_OUT);
    newRoom->setFlag(R_NO_TELEPORT);
    newRoom->setFlag(R_NO_TRACK_TO);
    newRoom->setFlag(R_NO_SUMMON_TO);
    newRoom->setFlag(R_OUTLAW_SAFE);
    newRoom->setFlag(R_INDOORS);

    auto *p = new Property;
    p->found(player, PROP_STORAGE, "any realty office", false);

    p->setName(newRoom->getName());
    p->addRange(newRoom->info);

    gConfig->addProperty(p);

    if(newRoom->saveToFile(0) < 0)
        return(0);

    newRoom.reset();
    return(0);
}

//*********************************************************************
//                      validatePerms
//*********************************************************************

void UniqueRoom::validatePerms() {
    std::map<int, crlasttime>::iterator it;
    crlasttime* crtm=nullptr;
    long    t = time(nullptr);

    for(it = permMonsters.begin(); it != permMonsters.end() ; it++) {
        crtm = &(*it).second;
        if(crtm->ltime > t) {
            crtm->ltime = t;
            logn("log.validate", "Perm #%d(%s) in Room %s (%s): Time has been revalidated.\n",
                (*it).first+1, crtm->cr.displayStr().c_str(), info.displayStr().c_str(), getCName());
            broadcast(isCt, "^yPerm Mob #%d(%s) in Room %s (%s) has been revalidated",
                (*it).first+1, crtm->cr.displayStr().c_str(), info.displayStr().c_str(), getCName());
        }
    }
    for(it = permObjects.begin(); it != permObjects.end() ; it++) {
        crtm = &(*it).second;
        if(crtm->ltime > t) {
            crtm->ltime = t;
            logn("log.validate", "Perm Obj #%d(%s) in Room %s (%s): Time has been revalidated.\n",
                (*it).first+1, crtm->cr.displayStr().c_str(), info.displayStr().c_str(), getCName());

            broadcast(isCt, "^yPerm Obj #%d(%s) in Room %s (%s) has been revalidated.",
                (*it).first+1, crtm->cr.displayStr().c_str(), info.displayStr().c_str(), getCName());
        }
    }
}

//*********************************************************************
//                      doRoomHarms
//*********************************************************************

void doRoomHarms(const std::shared_ptr<BaseRoom>& inRoom, const std::shared_ptr<Player>& target) {
    int     roll=0, toHit=0, dmg=0;

    if(!inRoom || !target)
        return;

    // elven archers
    if(inRoom->flagIsSet(R_ARCHERS)) {
        if( target->flagIsSet(P_HIDDEN) ||
            target->isInvisible() ||
            target->isEffected("mist") ||
            target->flagIsSet(P_DM_INVIS)
        ) {
            return;
        }

        roll = Random::get(1,20);
        toHit = 10 - (int)target->getArmor()/10;
        toHit = MAX(MIN(toHit,20), 1);


        if(roll >= toHit) {
            dmg = Random::get(1,8) + Random::get(1,2);
            target->printColor("A deadly arrow strikes you from above for %s%d^x damage.\n", target->customColorize("*CC:DAMAGE*").c_str(), dmg);
            broadcast(target->getSock(), inRoom, "An arrow strikes %s from the trees above!", target->getCName());

            target->hp.decrease(dmg);
            if(target->hp.getCur() < 1) {
                target->print("The arrow killed you.\n");
                broadcast(target->getSock(), inRoom, "The arrow killed %s!", target->getCName());
                target->die(ELVEN_ARCHERS);
                return;
            }
        } else {
            target->print("An arrow whizzes past you from above!\n");
            broadcast(target->getSock(), inRoom, "An arrow whizzes past %s from above!", target->getCName());
        }
    }


    // deadly moss
    if(inRoom->flagIsSet(R_DEADLY_MOSS)) {
        if(target->flagIsSet(P_DM_INVIS) || target->getClass() == CreatureClass::LICH)
            return;

        dmg = 15 - MIN(bonus((int)target->constitution.getCur()),2) + Random::get(1,3);
        target->printColor("Deadly underdark moss spores envelope you for %s%d^x damage!\n", target->customColorize("*CC:DAMAGE*").c_str(), dmg);
        broadcast(target->getSock(), inRoom, "Spores from deadly underdark moss envelope %s!", target->getCName());

        target->hp.decrease(dmg);
        if(target->hp.getCur() < 1) {
            target->print("The spores killed you.\n");
            broadcast(target->getSock(), inRoom, "The spores killed %s!", target->getCName());
            target->die(DEADLY_MOSS);
        }
    }
}

//*********************************************************************
//                      abortFindRoom
//*********************************************************************
// When this function is called, we're in trouble. Perhaps teleport searched
// too long, or some vital room (like The Origin) couldn't be loaded. We need
// to hurry up and find a room to take the player - if we can't, we need to
// crash the mud.

std::shared_ptr<BaseRoom> abortFindRoom(const std::shared_ptr<Creature>& player, const char from[15]) {
    std::shared_ptr<BaseRoom> room;
    std::shared_ptr<UniqueRoom> newRoom;

    player->print("Shifting dimensional forces direct your travel!\n");
    loge("Error: abortFindRoom called by %s in %s().\n", player->getCName(), from);
    broadcast(isCt, "^yError: abortFindRoom called by %s in %s().", player->getCName(), from);

    room = player->getRecallRoom().loadRoom(player->getAsPlayer());
    if(room)
        return(room);
    broadcast(isCt, "^yError: could not load Recall: %s.", player->getRecallRoom().str().c_str());

    room = player->getLimboRoom().loadRoom(player->getAsPlayer());
    if(room)
        return(room);
    broadcast(isCt, "^yError: could not load Limbo: %s.", player->getLimboRoom().str().c_str());

    std::shared_ptr<Player> target=nullptr;
    if(player)
        target = player->getAsPlayer();
    if(target) {
        room = target->bound.loadRoom(target);
        if(room)
            return(room);
        broadcast(isCt, "^yError: could not load player's bound room: %s.",
            target->bound.str().c_str());
    }

    if(loadRoom(0, newRoom))
        return(newRoom);
    broadcast(isCt, "^yError: could not load The Void. Aborting.");
    crash(-1);

    // since we "technically" have to return something to make compiler happy
    return(room);
}

//*********************************************************************
//                      getWeight
//*********************************************************************

int UniqueRoom::getWeight() {
    int     i=0;

    // count weight of all players in this
    for(const auto& ply: players) {
        if(ply->countForWeightTrap())
            i += ply->getWeight();
    }

    // count weight of all objects in this
    for(const auto& obj : objects ) {
        i += obj->getActualWeight();
    }

    // count weight of all monsters in this
    for(const auto& mons : monsters) {
        if(mons->countForWeightTrap()) {
            i+= mons->getWeight();
        }
    }

    return(i);
}

//*********************************************************************
//                      needUniqueRoom
//*********************************************************************

bool needUniqueRoom(const std::shared_ptr<Creature> & player) {
    if(!player->inUniqueRoom()) {
        player->print("You can't do that here.\n");
        return(false);
    }
    return(true);
}
