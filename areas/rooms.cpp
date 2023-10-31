/*
 * rooms.cpp
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

#include <cstdarg>                    // for va_list, va_end, va_start
#include <unistd.h>                    // for unlink
#include <algorithm>                   // for find
#include <cstring>                     // for memset, strcpy
#include <ctime>                       // for time
#include <list>                        // for operator==, list<>::iterator
#include <map>                         // for operator==, _Rb_tree_const_ite...
#include <set>                         // for set, set<>::iterator
#include <sstream>                     // for char_traits, operator<<, basic...
#include <string>                      // for string, allocator, operator<<
#include <string_view>                 // for string_view, operator==, basic...
#include <utility>

#include "area.hpp"                    // for MapMarker, Area, AreaZone, Til...
#include "catRef.hpp"                  // for CatRef
#include "catRefInfo.hpp"              // for CatRefInfo, CatRefInfo::limbo
#include "config.hpp"                  // for Config, gConfig
#include "effects.hpp"                 // for Effects
#include "enums/loadType.hpp"          // for LoadType, LoadType::LS_BACKUP
#include "flags.hpp"                   // for R_LIMBO, R_VAMPIRE_COVEN, R_DA...
#include "global.hpp"                  // for HELD, ARACHNUS, ARAMON, ARES
#include "hooks.hpp"                   // for Hooks
#include "lasttime.hpp"                // for lasttime, CRLastTime
#include "location.hpp"                // for Location
#include "move.hpp"                    // for deletePortal
#include "mud.hpp"                     // for MAX_MOBS_IN_ROOM
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/container.hpp"    // for ObjectSet, MonsterSet, PlayerSet
#include "mudObjects/creatures.hpp"    // for Creature
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/objects.hpp"      // for Object
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "proto.hpp"                   // for link_rom, isDay, broadcast
#include "random.hpp"                  // for Random
#include "realm.hpp"                   // for Realm
#include "server.hpp"                  // for Server, gServer, RoomCache
#include "size.hpp"                    // for Size, NO_SIZE, SIZE_COLOSSAL
#include "socket.hpp"                  // for Socket
#include "wanderInfo.hpp"              // for WanderInfo
#include "xml.hpp"                     // for loadRoom
#include "paths.hpp"


BaseRoom::BaseRoom() {
    tempNoKillDarkmetal = false;
    memset(misc, 0, sizeof(misc));
}


std::string BaseRoom::getVersion() const { return(version); }
void BaseRoom::setVersion(std::string_view v) { version = v; }


std::string BaseRoom::fullName() const {
    std::ostringstream oStr;
    if(!this) {
        oStr << "Invalid Room";
        return(oStr.str());
    }

    const std::shared_ptr<const UniqueRoom> uRoom = getAsConstUniqueRoom();
    const std::shared_ptr<const AreaRoom> aRoom = getAsConstAreaRoom();

    if(uRoom) {
        oStr << uRoom->info.displayStr() << "(" << uRoom->getName() << ")";
    } else if(aRoom) {
        oStr << aRoom->mapmarker.str();
    } else
        oStr << "invalid room";
    return(oStr.str());
}


//*********************************************************************
//                      Room
//*********************************************************************

UniqueRoom::UniqueRoom() {
    short_desc = "";
    long_desc = "";
    lowLevel = highLevel = maxmobs = trap = trapweight = trapstrength = 0;
    flags.reset();
    roomExp = 0;
    size = NO_SIZE;
    beenhere = 0;
    memset(last_mod, 0, sizeof(last_mod));

    memset(lastModTime, 0, sizeof(lastModTime));

    memset(lastPly, 0, sizeof(lastPly));
    memset(lastPlyTime, 0, sizeof(lastPlyTime));
}

bool UniqueRoom::operator< (const UniqueRoom& t) const {
    if(this->info.area[0] == t.info.area[0]) {
        return(this->info.id < t.info.id);
    } else {
        return(this->info.area < t.info.area);
    }
}

std::string UniqueRoom::getShortDescription() const { return(short_desc); }
std::string UniqueRoom::getLongDescription() const { return(long_desc); }
short UniqueRoom::getLowLevel() const { return(lowLevel); }
short UniqueRoom::getHighLevel() const { return(highLevel); }
int UniqueRoom::getMaxMobs() const {
    return(maxmobs ? maxmobs : MAX_MOBS_IN_ROOM);
}
short UniqueRoom::getTrap() const { return(trap); }
CatRef UniqueRoom::getTrapExit() const { return(trapexit); }
short UniqueRoom::getTrapWeight() const { return(trapweight); }
short UniqueRoom::getTrapStrength() const { return(trapstrength); }
std::string UniqueRoom::getFaction() const { return(faction); }
long UniqueRoom::getBeenHere() const { return(beenhere); }
int UniqueRoom::getRoomExperience() const { return(roomExp); }
Size UniqueRoom::getSize() const { return(size); }

void UniqueRoom::setShortDescription(std::string_view desc) { short_desc = desc; }
void UniqueRoom::setLongDescription(std::string_view desc) { long_desc = desc; }
void UniqueRoom::appendShortDescription(std::string_view desc) { short_desc += desc; }
void UniqueRoom::appendLongDescription(std::string_view desc) { long_desc += desc; }
void UniqueRoom::setLowLevel(short lvl) { lowLevel = std::max<short>(0, lvl); }
void UniqueRoom::setHighLevel(short lvl) { highLevel = std::max<short>(0, lvl); }
void UniqueRoom::setMaxMobs(short m) { maxmobs = std::max<short>(0, m); }
void UniqueRoom::setTrap(short t) { trap = t; }
void UniqueRoom::setTrapExit(const CatRef& t) { trapexit = t; }
void UniqueRoom::setTrapWeight(short weight) { trapweight = std::max<short>(0, weight); }
void UniqueRoom::setTrapStrength(short strength) { trapstrength = std::max<short>(0, strength); }
void UniqueRoom::setFaction(std::string_view f) { faction = f; }
void UniqueRoom::incBeenHere() { beenhere++; }
void UniqueRoom::setRoomExperience(int exp) { roomExp = std::max(0, exp); }
void UniqueRoom::setSize(Size s) { size = s; }

//*********************************************************************
//                      deconstructors
//*********************************************************************

void BaseRoom::BaseDestroy() {
    clearExits();

    objects.clear();
    monsters.clear();
    players.clear();

    effects.removeAll();
    removeEffectsIndex();
    gServer->removeDelayedActions(this);
}

UniqueRoom::~UniqueRoom() {
    BaseDestroy();
}

AreaRoom::~AreaRoom() {
    BaseDestroy();
    decCompass = needsCompass = stayInMemory = false;
    exits.clear();
}
bool AreaRoom::operator< (const AreaRoom& t) const {
    if(this->mapmarker.getX() == t.mapmarker.getX()) {
        if(this->mapmarker.getY() == t.mapmarker.getY()) {
            return(this->mapmarker.getZ() < t.mapmarker.getZ());
        } else {
            return(this->mapmarker.getY() < t.mapmarker.getY());
        }
    } else {
        return(this->mapmarker.getX() < t.mapmarker.getX());
    }
}

//*********************************************************************
//                      reset
//*********************************************************************

void AreaRoom::reset() {
    if (!area.expired())
        area.reset();
    decCompass = needsCompass = stayInMemory = false;
    exits.clear();

}

//*********************************************************************
//                      AreaRoom
//*********************************************************************

AreaRoom::AreaRoom(const std::shared_ptr<Area>& a) {
    reset();
    area = a;
}

Size AreaRoom::getSize() const { return(SIZE_COLOSSAL); }
bool AreaRoom::getNeedsCompass() const { return(needsCompass); }
bool AreaRoom::getDecCompass() const { return(decCompass); }
bool AreaRoom::getStayInMemory() const { return(stayInMemory); }

void AreaRoom::setNeedsCompass(bool need) { needsCompass = need; }
void AreaRoom::setDecCompass(bool dec) { decCompass = dec; }
void AreaRoom::setStayInMemory(bool stay) { stayInMemory = stay; }

//*********************************************************************
//                      recycle
//*********************************************************************

void AreaRoom::recycle() {
    objects.clear();

    updateExits();
}

//*********************************************************************
//                      setMapMarker
//*********************************************************************

void AreaRoom::setMapMarker(const MapMarker &m) {
    // The caller is responsible for registering/unregistering the room
    mapmarker = m;
    setId(std::string("R") + m.str());
}

//*********************************************************************
//                      updateExits
//*********************************************************************

bool AreaRoom::updateExit(std::string_view dir) {
    if(dir == "north") {
        mapmarker.add(0, 1, 0);
        link_rom(Container::downcasted_shared_from_this<AreaRoom>(), mapmarker, dir);
        mapmarker.add(0, -1, 0);

    } else if(dir == "east") {
        mapmarker.add(1, 0, 0);
        link_rom(Container::downcasted_shared_from_this<AreaRoom>(), mapmarker, dir);
        mapmarker.add(-1, 0, 0);

    } else if(dir == "south") {
        mapmarker.add(0, -1, 0);
        link_rom(Container::downcasted_shared_from_this<AreaRoom>(), mapmarker, dir);
        mapmarker.add(0, 1, 0);

    } else if(dir == "west") {
        mapmarker.add(-1, 0, 0);
        link_rom(Container::downcasted_shared_from_this<AreaRoom>(), mapmarker, dir);
        mapmarker.add(1, 0, 0);

    } else if(dir == "northeast") {
        mapmarker.add(1, 1, 0);
        link_rom(Container::downcasted_shared_from_this<AreaRoom>(), mapmarker, dir);
        mapmarker.add(-1, -1, 0);

    } else if(dir == "northwest") {
        mapmarker.add(-1, 1, 0);
        link_rom(Container::downcasted_shared_from_this<AreaRoom>(), mapmarker, dir);
        mapmarker.add(1, -1, 0);

    } else if(dir == "southeast") {
        mapmarker.add(1, -1, 0);
        link_rom(Container::downcasted_shared_from_this<AreaRoom>(), mapmarker, dir);
        mapmarker.add(-1, 1, 0);

    } else if(dir == "southwest") {
        mapmarker.add(-1, -1, 0);
        link_rom(Container::downcasted_shared_from_this<AreaRoom>(), mapmarker, dir);
        mapmarker.add(1, 1, 0);

    } else
        return(false);
    return(true);
}

void AreaRoom::updateExits() {
    updateExit("north");
    updateExit("east");
    updateExit("south");
    updateExit("west");
    updateExit("northeast");
    updateExit("northwest");
    updateExit("southeast");
    updateExit("southwest");
}


//*********************************************************************
//                      exitNameByOrder
//*********************************************************************

const char *exitNameByOrder(int i) {
    if(i==0) return("north");
    if(i==1) return("east");
    if(i==2) return("south");
    if(i==3) return("west");
    if(i==4) return("northeast");
    if(i==5) return("northwest");
    if(i==6) return("southeast");
    if(i==7) return("southwest");
    return("");
}

//*********************************************************************
//                      canSave
//*********************************************************************

bool AreaRoom::canSave() const {
    int     i=0;

    if(unique.id)
        return(true);

    if(needsEffectsIndex())
        return(true);

    if(!exits.empty()) {
        for(const auto& ext : exits) {
            if(ext->getName() != exitNameByOrder(i++))
                return(true);
            // doesnt check rooms leading to other area rooms
            if(ext->target.room.id)
                return(true);
        }
        if(i != 8)
            return(true);
    }

    return(false);
}


//*********************************************************************
//                      getRandomWanderInfo
//*********************************************************************

WanderInfo* AreaRoom::getRandomWanderInfo() const {
    std::list<std::shared_ptr<AreaZone> >::iterator it;
    std::shared_ptr<AreaZone> zone;
    std::shared_ptr<TileInfo>  tile;
    std::map<int, WanderInfo*> w;
    int i=0;

    // we want to randomly pick one of the zones
    if(auto myArea = area.lock()) {
        for (it = myArea->areaZones.begin(); it != myArea->areaZones.end(); it++) {
            zone = (*it);
            if (zone->wander.getTraffic() && zone->inside(myArea, mapmarker)) {
                w[i++] = &zone->wander;
            }
        }

        tile = myArea->getTile(myArea->getTerrain(nullptr, mapmarker, 0, 0, 0, true), myArea->getSeasonFlags(mapmarker));
        if (tile && tile->wander.getTraffic())
            w[i++] = &tile->wander;
    }

    if(!i)
        return(nullptr);
    return(w[Random::get(0,i-1)]);
}


//*********************************************************************
//                      getWanderInfo
//*********************************************************************

WanderInfo* BaseRoom::getWanderInfo() {
    std::shared_ptr<UniqueRoom> uRoom = getAsUniqueRoom();
    if(uRoom)
        return(&uRoom->wander);
    std::shared_ptr<AreaRoom> aRoom = getAsAreaRoom();
    if(aRoom)
        return(aRoom->getRandomWanderInfo());
    return(nullptr);
}

//*********************************************************************
//                      delExit
//*********************************************************************

bool BaseRoom::delExit(const std::shared_ptr<Exit>& exit) {
    if(exit) {
        auto xit = std::find(exits.begin(), exits.end(), exit);
        if(xit != exits.end()) {
            exits.erase(xit);
            return(true);
        }
    }
    return(false);
}
bool BaseRoom::delExit( std::string_view dir) {
    for(const auto& ext : exits) {
        if(ext->getName() == dir) {
            exits.remove(ext);
            return(true);
        }
    }

    return(false);
}


void BaseRoom::clearExits() {
    ExitList::iterator xit;
    for(xit = exits.begin() ; xit != exits.end(); ) {
        std::shared_ptr<Exit> exit = (*xit++);
        if(exit->flagIsSet(X_PORTAL)) {
            std::shared_ptr<BaseRoom> target = exit->target.loadRoom();
            Move::deletePortal(target, exit->getPassPhrase());
        }
    }
    exits.clear();
}

//*********************************************************************
//                      canDelete
//*********************************************************************

bool AreaRoom::canDelete() {
    // don't delete unique rooms
    if(stayInMemory || unique.id)
        return(false);
    if(!monsters.empty())
        return(false);
    if(!players.empty())
        return(false);
    for(const auto& obj : objects) {
        if(!obj->flagIsSet(O_DISPOSABLE))
            return(false);
    }
    // any room effects?
    if(needsEffectsIndex())
        return(false);
    if(!exits.empty()) {
        int     i=0;
        for(const auto& ext : exits) {
            if(ext->getName() != exitNameByOrder(i++))
                return(false);
            // doesn't check rooms leading to other area rooms
            if(ext->target.room.id)
                return(false);
        }
        if(i != 8)
            return(false);
    }
    return(true);
}

//*********************************************************************
//                  isInteresting
//*********************************************************************
// This will determine whether the looker will see anything of
// interest in the room. This is used by cmdScout.

bool AreaRoom::isInteresting(const std::shared_ptr<const Player> &viewer) const {
    int     i;

    if(unique.id)
        return(true);

    for(const auto& pIt: players) {
        if(auto ply = pIt.lock()) {
            if (!ply->flagIsSet(P_HIDDEN) && viewer->canSee(ply))
                return (true);
        }
    }
    for(const auto& mons : monsters) {
        if(!mons->flagIsSet(M_HIDDEN) && viewer->canSee(mons))
            return(true);
    }
    i = 0;
    for(const auto& ext : exits) {
        if(i < 7 && ext->getName() != exitNameByOrder(i))
            return(true);
        if(i >= 8) {
            if(viewer->showExit(ext))
                return(true);
        }
        i++;
    }
    // Fewer than 8 exits
    if( i < 7 )
        return(true);

    return(false);
}


bool AreaRoom::flagIsSet(int flag) const {
    MapMarker m = mapmarker;
    auto myArea = area.lock();
    return(myArea && myArea->flagIsSet(flag, m));
}

void AreaRoom::setFlag(int flag) {
    std::clog << "Trying to set a flag on an area room!" << std::endl;
}
bool UniqueRoom::flagIsSet(int flag) const {
    return flags.test(flag);
}
void UniqueRoom::setFlag(int flag) {
    flags.set(flag);
}
void UniqueRoom::clearFlag(int flag) {
    flags.reset(flag);
}
bool UniqueRoom::toggleFlag(int flag) {
flags.flip(flag);
    return(flagIsSet(flag));
}


//*********************************************************************
//                      isMagicDark
//*********************************************************************

bool BaseRoom::isMagicDark() const {
    if(flagIsSet(R_MAGIC_DARKNESS))
        return(true);

    // check for darkness spell
    for(const auto& pIt: players) {
        if(auto ply = pIt.lock()) {
            // darkness spell on staff does nothing
            if (ply->isEffected("darkness") && !ply->isStaff())
                return (true);
            if (ply->flagIsSet(P_DARKNESS) && !ply->flagIsSet(P_DM_INVIS))
                return (true);
        }
    }
    for(const auto& mons : monsters) {
        if(mons->isEffected("darkness"))
            return(true);
        if(mons->flagIsSet(M_DARKNESS))
            return(true);
    }
    for(const auto& obj : objects) {
        if(obj->flagIsSet(O_DARKNESS))
            return(true);
    }
    return(false);
}


//*********************************************************************
//                      isNormalDark
//*********************************************************************

bool BaseRoom::isNormalDark() const {
    if(flagIsSet(R_DARK_ALWAYS))
        return(true);

    if(!isDay() && flagIsSet(R_DARK_AT_NIGHT))
        return(true);

    return(false);
}


void BaseRoom::addExit(const std::shared_ptr<Exit>& ext) {
    ext->setRoom(Container::downcasted_shared_from_this<BaseRoom>());

    exits.push_back(ext);
}

//
// This function checks the status of the exits in a room.  If any of
// the exits are closable or lockable, and the correct time interval
// has occurred since the last opening/unlocking, then the doors are
// re-shut/re-closed.
//
void BaseRoom::checkExits() {
    long    t = time(nullptr);

    for(const auto& ext : exits) {
        if( ext->flagIsSet(X_LOCKABLE) && (ext->ltime.ltime + ext->ltime.interval) < t)
        {
            ext->setFlag(X_LOCKED);
            ext->setFlag(X_CLOSED);
        } else if(  ext->flagIsSet(X_CLOSABLE) && (ext->ltime.ltime + ext->ltime.interval) < t)
            ext->setFlag(X_CLOSED);
    }
}

//*********************************************************************
//                      deityRestrict
//*********************************************************************
// if our particular flag is set, fail = 0
// else if ANY OTHER flags are set, fail = 1

bool BaseRoom::deityRestrict(const std::shared_ptr<const Creature> & creature) const {

    // if no flags are set
    if( !flagIsSet(R_ARAMON) &&
        !flagIsSet(R_CERIS) &&
        !flagIsSet(R_ENOCH) &&
        !flagIsSet(R_GRADIUS) &&
        !flagIsSet(R_KAMIRA) &&
        !flagIsSet(R_ARES) &&
        !flagIsSet(R_ARACHNUS) &&
        !flagIsSet(R_LINOTHAN) &&
        !flagIsSet(R_MARA) &&
        !flagIsSet(R_JAKAR)
    )
        return(false);

    // if the deity flag is set and they match, they pass
    if( (flagIsSet(R_ARAMON) && creature->getDeity() == ARAMON) ||
        (flagIsSet(R_CERIS) && creature->getDeity() == CERIS) ||
        (flagIsSet(R_ENOCH) && creature->getDeity() == ENOCH) ||
        (flagIsSet(R_GRADIUS) && creature->getDeity() == GRADIUS) ||
        (flagIsSet(R_KAMIRA) && creature->getDeity() == KAMIRA) ||
        (flagIsSet(R_ARES) && creature->getDeity() == ARES) ||
        (flagIsSet(R_ARACHNUS) && creature->getDeity() == ARACHNUS) ||
        (flagIsSet(R_LINOTHAN) && creature->getDeity() == LINOTHAN) ||
        (flagIsSet(R_MARA) && creature->getDeity() == MARA) ||
        (flagIsSet(R_JAKAR) && creature->getDeity() == JAKAR)
    )
        return(false);

    return(true);
}



//*********************************************************************
//                          maxCapacity
//*********************************************************************
// 0 means no limit

int BaseRoom::maxCapacity() const {
    if(flagIsSet(R_ONE_PERSON_ONLY))
        return(1);
    if(flagIsSet(R_TWO_PEOPLE_ONLY))
        return(2);
    if(flagIsSet(R_THREE_PEOPLE_ONLY))
        return(3);
    return(0);
}

//*********************************************************************
//                          isFull
//*********************************************************************
// Can the player fit in the room?

bool BaseRoom::isFull() const {
    return(maxCapacity() && countVisPly() >= maxCapacity());
}

//*********************************************************************
//                      countVisPly
//*********************************************************************
// This function counts the number of (non-DM-invisible) players in a
// room and returns that number.

int BaseRoom::countVisPly() const {
    int     num = 0;

    for(const auto& pIt: players) {
        if(auto ply = pIt.lock()) {
            if (!ply->flagIsSet(P_DM_INVIS))
                num++;
        }
    }

    return(num);
}


//*********************************************************************
//                      countCrt
//*********************************************************************
// This function counts the number of creatures in a
// room and returns that number.

int BaseRoom::countCrt() const {
    int num = 0;

    for(const auto& mons : monsters) {
        if(!mons->isPet())
            num++;
    }

    return(num);
}


//*********************************************************************
//                      getMaxMobs
//*********************************************************************

int BaseRoom::getMaxMobs() const {
    return(MAX_MOBS_IN_ROOM);
}


//*********************************************************************
//                      vampCanSleep
//*********************************************************************

bool BaseRoom::vampCanSleep(std::shared_ptr<Socket> sock) const {
    // not at night
    if(!isDay()) {
        sock->print("Your thirst for blood keeps you from sleeping.\n");
        return(false);
    }
    // during the day, under certain circumstances
    if(isMagicDark())
        return(true);
    if( flagIsSet(R_DARK_ALWAYS) ||
        flagIsSet(R_INDOORS) ||
        flagIsSet(R_LIMBO) ||
        flagIsSet(R_VAMPIRE_COVEN) ||
        flagIsSet(R_UNDERGROUND) )
        return(true);
    sock->print("The sunlight prevents you from sleeping.\n");
    return(false);
}


//*********************************************************************
//                      isCombat
//*********************************************************************
// checks to see if there is any time of combat going on in this room

bool BaseRoom::isCombat() const {

    for(const auto& pIt: players) {
        if(auto ply = pIt.lock()) {
            if (ply->inCombat(true))
                return (true);
        }
    }
    for(const auto& mons : monsters) {
        if(mons->inCombat(true))
            return(true);
    }

    return(false);
}


//*********************************************************************
//                      isUnderwater
//*********************************************************************

bool BaseRoom::isUnderwater() const {
    return(flagIsSet(R_UNDER_WATER_BONUS));
}


//*********************************************************************
//                      isOutdoors
//*********************************************************************

bool BaseRoom::isOutdoors() const {
    return(!flagIsSet(R_INDOORS) && !flagIsSet(R_VAMPIRE_COVEN) && !flagIsSet(R_SHOP_STORAGE) && !flagIsSet(R_LIMBO));
}


//*********************************************************************
//                      magicBonus
//*********************************************************************

bool BaseRoom::magicBonus() const {
    return(flagIsSet(R_MAGIC_BONUS));
}


//*********************************************************************
//                      isForest
//*********************************************************************

bool BaseRoom::isForest() const {
    return(flagIsSet(R_FOREST_OR_JUNGLE));
}

//*********************************************************************
//                      isWater
//*********************************************************************
// drowning in area rooms is different than drowning in unique water rooms,
// therefore we need a different isWater function

bool AreaRoom::isWater() const {
    auto myArea = area.lock();
    return(myArea && myArea->isWater(mapmarker.getX(), mapmarker.getY(), mapmarker.getZ(), true));
}

//*********************************************************************
//                      isRoad
//*********************************************************************

bool AreaRoom::isRoad() const {
    auto myArea = area.lock();
    return(myArea && myArea->isRoad(mapmarker.getX(), mapmarker.getY(), mapmarker.getZ(), true));
}

//*********************************************************************
//                      getUnique
//*********************************************************************
// creature is allowed to be null

CatRef AreaRoom::getUnique(const std::shared_ptr<Creature>&creature, bool skipDec) {
    if(!needsCompass)
        return(unique);
    CatRef  cr;

    if(!creature)
        return(cr);

    bool pet = creature->isPet();
    std::shared_ptr<Player> player = creature->getPlayerMaster();

    if( !player ||
        !player->ready[HELD-1] ||
        player->ready[HELD-1]->getShotsCur() < 1 ||
        !player->ready[HELD-1]->compass ||
        *player->ready[HELD-1]->compass != *&mapmarker
    )
        return(cr);

    // no need to decrement twice if their pet is going through
    if(!skipDec && decCompass && !pet) {
        player->ready[HELD-1]->decShotsCur();
        player->breakObject(player->ready[HELD-1], HELD);
    }
    return(unique);
}

bool BaseRoom::isDropDestroy() const {
    if(!flagIsSet(R_DESTROYS_ITEMS))
        return(false);
    const auto aRoom = getAsConstAreaRoom();
    if(aRoom) {
        if(auto myArea = aRoom->area.lock()) {
            return (!myArea->isRoad(aRoom->mapmarker.getX(), aRoom->mapmarker.getY(), aRoom->mapmarker.getZ(), true));
        }
    }
    return false;
}
bool BaseRoom::hasRealmBonus(Realm realm) const {
    return(flagIsSet(R_ROOM_REALM_BONUS) && flagIsSet(getRealmRoomBonusFlag(realm)));
}
bool BaseRoom::hasOppositeRealmBonus(Realm realm) const {
    return(flagIsSet(R_OPPOSITE_REALM_BONUS) && flagIsSet(getRealmRoomBonusFlag(getOppositeRealm(realm ))));
}


std::shared_ptr<Monster>  BaseRoom::getTollkeeper() const {
    for(const auto& mons : monsters) {
        if(!mons->isPet() && mons->flagIsSet(M_TOLLKEEPER))
            return(mons->getAsMonster());
    }
    return(nullptr);
}

//*********************************************************************
//                      isSunlight
//*********************************************************************
// the first version is isSunlight is only called when an AreaRoom
// does not exist - when rogues scout, for example

bool Area::isSunlight(const MapMarker& mapmarker) const {
    if(!isDay())
        return(false);

    // if their room has any of the following flags, no kill
    if( flagIsSet(R_DARK_ALWAYS, mapmarker) ||
        flagIsSet(R_UNDERGROUND, mapmarker) ||
        flagIsSet(R_INDOORS, mapmarker) ||
        flagIsSet(R_LIMBO, mapmarker) ||
        flagIsSet(R_VAMPIRE_COVEN, mapmarker) ||
        flagIsSet(R_ETHEREAL_PLANE, mapmarker) ||
        flagIsSet(R_IS_STORAGE_ROOM, mapmarker) ||
        flagIsSet(R_MAGIC_DARKNESS, mapmarker)
    )
        return(false);

    return(true);
}

// this version of isSunlight is called when a room exists

bool BaseRoom::isSunlight() const {

    // !this - for offline players. make it not sunny so darkmetal isnt destroyed
    if(!this)
        return(false);

    if(!isDay() || !isOutdoors() || isMagicDark())
        return(false);

    const std::shared_ptr<const UniqueRoom> uRoom = getAsConstUniqueRoom();
    const CatRefInfo* cri=nullptr;
    if(uRoom)
        cri = gConfig->getCatRefInfo(uRoom->info.area);

    // be nice - no killing darkmetal in special rooms
    if(cri && (
        !uRoom->info.id ||
        uRoom->info.id == cri->getRecall() ||
        uRoom->info.id == cri->getLimbo()
    ) )
        return(false);

    // if their room has any of the following flags, no kill
    if( flagIsSet(R_DARK_ALWAYS) ||
        flagIsSet(R_UNDERGROUND) ||
        flagIsSet(R_ETHEREAL_PLANE) ||
        flagIsSet(R_IS_STORAGE_ROOM)
    )
        return(false);

    return(true);
}

//*********************************************************************
//                      destroy
//*********************************************************************

void UniqueRoom::destroy() {
    saveToFile(0, LoadType::LS_BACKUP);
    auto path = Path::roomPath(info);
    expelPlayers(true, true, true);
    gServer->roomCache.remove(info);
    fs::remove(path);
}

//*********************************************************************
//                      expelPlayers
//*********************************************************************

void BaseRoom::expelPlayers(bool useTrapExit, bool expulsionMessage, bool expelStaff) {
    std::shared_ptr<Player> target;
    std::shared_ptr<BaseRoom> newRoom=nullptr;
    std::shared_ptr<UniqueRoom> uRoom=nullptr;

    // allow them to be sent to the room's trap exit
    if(useTrapExit) {
        uRoom = getAsUniqueRoom();
        if(uRoom) {
            CatRef cr = uRoom->getTrapExit();
            uRoom = nullptr;
            if(cr.id && loadRoom(cr, uRoom))
                newRoom = uRoom;
        }
    }

    auto pIt = players.begin();
    auto pEnd = players.end();
    while(pIt != pEnd) {
        target = pIt->lock();
        if(!target) {
            pIt = players.erase(pIt);
            continue;
        }
        pIt++;

        if(!expelStaff && target->isStaff())
            continue;

        if(expulsionMessage) {
            target->printColor("^rYou are expelled from this room as it collapses in on itself!\n");
            broadcast(target->getSock(), shared_from_this(), "^r%M is expelled from the room as it collapses in on itself!", target.get());
        } else {
            target->printColor("^CShifting dimensional forces displace you from this room.\n");
            broadcast(target->getSock(), shared_from_this(), "^CShifting dimensional forces displace %N from this room.", target.get());
        }

        if(!useTrapExit || !newRoom) {
            newRoom = target->bound.loadRoom(target);
            if(!newRoom || newRoom.get() == this)
                newRoom = abortFindRoom(target, "expelPlayers");
        }

        target->deleteFromRoom();
        target->addToRoom(newRoom);
        target->doPetFollow();
    }
}


//*********************************************************************
//                      getSpecialArea
//*********************************************************************

Location getSpecialArea(int (CatRefInfo::*toCheck), const CatRef& cr) {
    return(getSpecialArea(toCheck, nullptr, cr.area, cr.id));
}

Location getSpecialArea(int (CatRefInfo::*toCheck), const std::shared_ptr<const Creature> & creature, std::string_view area, short id) {
    Location l;

    if(creature) {
        if(creature->inAreaRoom()) {
            l.room.setArea("area");
            if(auto roomArea = creature->getConstAreaRoomParent()->area.lock()){
                l.room.id = roomArea->id;
            }
        } else {
            l.room.setArea(creature->currentLocation.room.area);
        }
    }
    if(!area.empty())
        l.room.setArea(std::string(area));
    if(id)
        l.room.id = id;


    const CatRefInfo* cri = gConfig->getCatRefInfo(l.room.area, l.room.id, true);

    if(!cri)
        cri = gConfig->getCatRefInfo(gConfig->getDefaultArea());
    if(cri) {
        l.room.setArea(cri->getArea());
        l.room.id = static_cast<short>(cri->*toCheck);
    } else {
        // failure!
        l.room.setArea(gConfig->getDefaultArea());
        l.room.id = 0;
    }

    return(l);
}

//*********************************************************************
//                      getLimboRoom
//*********************************************************************

Location Creature::getLimboRoom() const {
    return(getSpecialArea(&CatRefInfo::limbo, Containable::downcasted_shared_from_this<Creature>(), "", 0));
}

//*********************************************************************
//                      getRecallRoom
//*********************************************************************

Location Creature::getRecallRoom() const {
    const std::shared_ptr<const Player> player = getAsConstPlayer();
    if(player && (player->flagIsSet(P_T_TO_BOUND) || player->getClass() == CreatureClass::BUILDER))
        return(player->bound);
    return(getSpecialArea(&CatRefInfo::recall, Containable::downcasted_shared_from_this<Creature>(), "", 0));
}


//*********************************************************************
//                      print
//*********************************************************************

bool hearBroadcast(std::shared_ptr<Creature> target, std::shared_ptr<Socket> ignore1, std::shared_ptr<Socket> ignore2, bool showTo(std::shared_ptr<Socket>));

void BaseRoom::print(std::shared_ptr<Socket> ignore, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    doPrint(nullptr, ignore, nullptr, fmt, ap);
    va_end(ap);
}
void BaseRoom::print(std::shared_ptr<Socket> ignore1, std::shared_ptr<Socket> ignore2, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    doPrint(nullptr, ignore1, ignore2, fmt, ap);
    va_end(ap);
}

//*********************************************************************
//                      doPrint
//*********************************************************************

void BaseRoom::doPrint(bool showTo(std::shared_ptr<Socket>), std::shared_ptr<Socket> ignore1, std::shared_ptr<Socket> ignore2, const char *fmt, va_list ap) {
    for(const auto& pIt: players) {
        if(auto ply = pIt.lock()) {
            if (!hearBroadcast(ply, ignore1, ignore2, showTo))
                continue;
            if (ply->flagIsSet(P_UNCONSCIOUS))
                continue;

            ply->vprint(ply->customColorize(fmt).c_str(), ap);
        }
    }
}


//*********************************************************************
//                      isOutlawSafe
//*********************************************************************

bool BaseRoom::isOutlawSafe() const {
    return( flagIsSet(R_OUTLAW_SAFE) ||
            flagIsSet(R_VAMPIRE_COVEN) ||
            flagIsSet(R_LIMBO) ||
            hasTraining()
    );
}

//*********************************************************************
//                      isOutlawSafe
//*********************************************************************

bool BaseRoom::isPkSafe() const {
    return( flagIsSet(R_SAFE_ROOM) ||
            flagIsSet(R_VAMPIRE_COVEN) ||
            flagIsSet(R_LIMBO)
    );
}

//*********************************************************************
//                      isOutlawSafe
//*********************************************************************

bool BaseRoom::isFastTick() const {
    return( flagIsSet(R_FAST_HEAL) ||
            flagIsSet(R_LIMBO)
    );
}

//*********************************************************************
//                      canPortHere
//*********************************************************************

bool UniqueRoom::canPortHere(const std::shared_ptr<Creature> & creature) const {
    // check creature-specific settings
    if(creature) {
        if(size && creature->getSize() && creature->getSize() > size)
            return(false);
        if(deityRestrict(creature))
            return(false);
        if(getLowLevel() > creature->getLevel())
            return(false);
        if(getHighLevel() && creature->getLevel() > getHighLevel())
            return(false);
    }

    // check room-specific settings
    if( flagIsSet(R_NO_TELEPORT) ||
        flagIsSet(R_LIMBO) ||
        flagIsSet(R_VAMPIRE_COVEN) ||
        flagIsSet(R_SHOP_STORAGE) ||
        flagIsSet(R_JAIL) ||
        flagIsSet(R_IS_STORAGE_ROOM) ||
        flagIsSet(R_ETHEREAL_PLANE) ||
        isConstruction() ||
        hasTraining()
            )
        return(false);
    if(isFull())
        return(false);
    // artificial limits for the misc area
    if(info.isArea("misc") && info.id <= 1000)
        return(false);
    if(exits.empty())
        return(false);

    return(true);
}
