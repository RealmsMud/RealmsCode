/*
 * area.cpp
 *   Everything related to the overland map
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
#include <sys/stat.h>                // for stat
#include <cmath>                     // for abs, atan, pow
#include <algorithm>                 // for replace
#include <cctype>                    // for islower, isupper, tolower, toupper
#include <cstdio>                    // for sprintf
#include <cstdlib>                   // for abs, atoi
#include <cstring>                   // for strcmp, strlen, strcpy, strncpy
#include <fstream>                   // for operator<<, basic_ostream, ostri...
#include <list>                      // for list, operator==, _List_iterator
#include <map>                       // for operator==, map, _Rb_tree_iterator
#include <memory>                    // for allocator_traits<>::value_type
#include <set>                       // for set
#include <stdexcept>                 // for out_of_range
#include <string>                    // for string, basic_string, allocator
#include <string_view>               // for string_view
#include <utility>                   // for pair
#include <vector>                    // for vector

#include "area.hpp"                  // for Area, MapMarker, TileInfo, AreaZone
#include "calendar.hpp"              // for Calendar
#include "catRef.hpp"                // for CatRef
#include "cmd.hpp"                   // for cmd
#include "commands.hpp"              // for getFullstrText
#include "config.hpp"                // for Config, gConfig
#include "effects.hpp"               // for EffectInfo
#include "flags.hpp"                 // for M_HIDDEN, P_HIDDEN, P_INVERT_ARE...
#include "global.hpp"                // for DARKELF, MAXWEAR
#include "location.hpp"              // for Location
#include "mudObjects/areaRooms.hpp"  // for AreaRoom
#include "mudObjects/container.hpp"  // for MonsterSet, PlayerSet, ObjectSet
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/objects.hpp"    // for Object
#include "mudObjects/players.hpp"    // for Player
#include "paths.hpp"                 // for AreaData
#include "proto.hpp"                 // for zero, file_exists, showRoomFlags
#include "season.hpp"                // for Season, NO_SEASON
#include "server.hpp"                // for Server, gServer
#include "socket.hpp"                // for Socket
#include "track.hpp"                 // for Track
#include "utils.hpp"                 // for MAX

class BaseRoom;


class WanderInfo;

//*********************************************************************
//                      AreaTrack
//*********************************************************************

AreaTrack::AreaTrack() {
    duration = 0;
}

int AreaTrack::getDuration() const { return(duration); }
void AreaTrack::setDuration(int dur) { duration = dur; }

//*********************************************************************
//                      MapMarker
//*********************************************************************

MapMarker::MapMarker() {
    reset();
}

short MapMarker::getArea() const { return(area); }
short MapMarker::getX() const { return(x); }
short MapMarker::getY() const { return(y); }
short MapMarker::getZ() const { return(z); }


void MapMarker::setArea(short n) { area = n; }
void MapMarker::setX(short n) { x = n; }
void MapMarker::setY(short n) {y  = n; }
void MapMarker::setZ(short n) { z = n; }
void MapMarker::set(short _area, short _x, short _y, short _z) {
    area = _area;
    x = _x;
    y = _y;
    z = _z;
}
void MapMarker::add(short _x, short _y, short _z) {
    x += _x;
    y += _y;
    z += _z;
}


void MapMarker::reset() {
    area = x = y = z = 0;
}

MapMarker& MapMarker::operator=(const MapMarker& m) {
    area = m.getArea();
    x = m.getX();
    y = m.getY();
    z = m.getZ();
    return(*this);
}
bool MapMarker::operator==(const MapMarker& m) const {
    return(area == m.getArea() &&
        x == m.getX() &&
        y == m.getY() &&
        z == m.getZ());
}
bool MapMarker::operator!=(const MapMarker& m) const {
    return(!(*this == m));
}

//*********************************************************************
//                      load
//*********************************************************************

void MapMarker::load(std::string str) {
    // trim
    str = str.substr(3, str.length()-1);

    area = atoi(str.c_str());
    x = atoi(getFullstrText(str, 1, ':').c_str());
    y = atoi(getFullstrText(str, 2, ':').c_str());
    z = atoi(getFullstrText(str, 3, ':').c_str());
}


//*********************************************************************
//                      str
//*********************************************************************

std::string MapMarker::str(bool color) const {
    std::ostringstream oStr;
    std::string sep = ":";
    if(color) {
        sep = "^b:^w";
        oStr << "^b";
    } else
        oStr << "(";

    oStr  << "A" << sep << (int)area << sep << (int)x << sep << (int)y << sep << (int)z;

    if(!color)
        oStr << ")";
    return(oStr.str());
}


//*********************************************************************
//                      direction
//*********************************************************************
// judges the direction to the target

std::string MapMarker::direction(const MapMarker *mapmarker) const {
    if(area != mapmarker->area || *this == *mapmarker)
        return("nowhere");

    // the 360 degrees have been divided into 24 sections, 15 degrees each
    std::string dir = "";
    double dY = y - mapmarker->y;
    double dX = x - mapmarker->x;
    double dZ = z - mapmarker->z;
    double angle = 0.0;

    // do the standard plane
    if(x == mapmarker->getX()) {
        if(y > mapmarker->getY()) {
            dir = "south";
        } else if(y < mapmarker->getY()) {
            dir = "north";
        }
    } else if(y == mapmarker->getY()) {
        if(x > mapmarker->getX()) {
            dir = "west";
        } else if(x < mapmarker->getX()) {
            dir = "east";
        }
    } else {

        angle = atan(dY / dX) * RADIAN;

        if(y > mapmarker->getY()) {

            if(angle < -75)
                dir = "south";
            else if(angle < -60)
                dir = "south southeast";
            else if(angle < -30)
                dir = "southeast";
            else if(angle < -15)
                dir = "east southeast";
            else if(angle < 0)
                dir = "east";
            else if(angle < 15)
                dir = "west";
            else if(angle < 30)
                dir = "west southwest";
            else if(angle < 60)
                dir = "southwest";
            else if(angle < 75)
                dir = "south southwest";
            else
                dir = "south";

        } else if(y < mapmarker->getY()) {

            if(angle < -75)
                dir = "north";
            else if(angle < -60)
                dir = "north northwest";
            else if(angle < -30)
                dir = "northwest";
            else if(angle < -15)
                dir = "west northwest";
            else if(angle < 0)
                dir = "west";
            else if(angle < 15)
                dir = "east";
            else if(angle < 30)
                dir = "east northeast";
            else if(angle < 60)
                dir = "northeast";
            else if(angle < 75)
                dir = "north northeast";
            else
                dir = "north";

        }
    }

    // now do elevation
    if(z != mapmarker->getZ()) {

        if(y == mapmarker->getY() && x == mapmarker->getX()) {
            if(z > mapmarker->getZ())
                dir = "straight down";
            else
                dir = "straight up";
        } else {

            dir += ", ";
            // Pythagorean theorem
            angle = abs((int)(atan(dZ / pow(pow(dX, 2) + pow(dY, 2), 0.5)) * RADIAN));

            if(angle < 15)
                dir += "slight";
            else if(angle < 30)
                dir += "gradual";
            else if(angle < 60)
                dir += "moderate";
            else
                dir += "steep";

            dir += " ";

            if(z > mapmarker->getZ())
                dir += "descent";
            else
                dir += "ascent";

        }
    }

    return(dir);
}

//*********************************************************************
//                      distance
//*********************************************************************
// estimates the distance between two points

std::string MapMarker::distance(const MapMarker *mapmarker) const {
    // we don't understand distance in these scenarios
    if(area != mapmarker->getArea() || *this == *mapmarker)
        return("");

    int distance = (MAX(abs(z - mapmarker->getZ()), MAX(abs(x - mapmarker->getX()), abs(y - mapmarker->getY()))));

    // The target is...
    if(distance <= 3)
        return("very close by");
    else if(distance <= 9)
        return("close by");
    else if(distance <= 15)
        return("in the vicinity");
    else if(distance <= 25)
        return("a ways away");
    else if(distance <= 45)
        return("a good distance away");
    else if(distance <= 90)
        return("far away");
    else if(distance <= 120)
        return("very far away");
    return("far, far away");
}

//*********************************************************************
//                      filename
//*********************************************************************
// returns the filename this mapmarker would use

std::string MapMarker::filename() const {
    std::ostringstream oStr;
    std::string str;
    oStr << "m" << (int)x << "." << (int)y << "." << (int)z << ".xml";
    str = oStr.str();
    // svn doesnt like -, so we'll just use underscore
    std::replace(str.begin(), str.end(), '-', '_');
    return(str);
}

//*********************************************************************
//                      AreaZone
//*********************************************************************

AreaZone::AreaZone() {
    name = display = fishing = "";
    zero(terRestrict, sizeof(terRestrict));
    zero(mapRestrict, sizeof(mapRestrict));

    min.setX(1000);
    min.setY(1000);
    min.setZ(1000);
    max.setX(-1000);
    max.setY(-1000);
    max.setZ(-1000);

    zero(flags, sizeof(flags));
}

AreaZone::~AreaZone() {
    for (auto const& [mapId, mapmarker] : coords) {
        delete mapmarker;
    }
    coords.clear();
}



//*********************************************************************
//                      inside
//*********************************************************************
// Point in polygon code adapted for Realms

// Copyright 2001, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose providing that this
// copyright notice is included with it. SoftSurfer makes no warranty for this code,
// and cannot be held liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.

// cn_PnPoly(): crossing number test for a point in a polygon
//  Input:  P = a point,
//              V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//  Return: 0 = outside, 1 = inside
// This code is patterned after [Franklin, 2000]

bool AreaZone::inside(const Area *area, const MapMarker *mapmarker) const {
    MapMarker *vi=nullptr, *vn=nullptr;
    // the crossing number counter
    int     cn = 0;
    int     i=0, n = coords.size();
    float   vt = 0.0;

    // if they aren't even within the established region, don't do the work
    if( mapmarker->getX() > max.getX() || mapmarker->getY() > max.getY() || mapmarker->getZ() > max.getZ() ||
        mapmarker->getX() < min.getX() || mapmarker->getY() < min.getY() || mapmarker->getZ() < min.getZ()
    ) {
        return(false);
    }

    if( !inRestrict(area->getTerrain(nullptr, mapmarker, 0, 0, 0, true), terRestrict) ||
        !inRestrict(area->getTerrain(nullptr, mapmarker, 0, 0, 0, false), mapRestrict)
    ) {
        return(false);
    }


    // loop through all edges of the polygon
    // edge from V[i] to V[i+1]
    for(i=0; i<n; i++) {
        vi = (*coords.find(i)).second;
        vn = (*coords.find(i+1 < n ? i+1 : 0)).second;
        if( (vi->getY() <= mapmarker->getY() && vn->getY() > mapmarker->getY()) ||  // an upward crossing
            (vi->getY() > mapmarker->getY() && vn->getY() <= mapmarker->getY()) // a downward crossing
        ) {
            // compute the actual edge-ray intersect x-coordinate
            vt = (float)(mapmarker->getY() - vi->getY());
            vt /= (float)(vn->getY() - vi->getY());
            vt *= (float)(vn->getX() - vi->getX());
            vt += (float)vi->getX();
            // P.x < intersect
            if(mapmarker->getX() <= vt) {
                // a valid crossing of y=P.y right of P.x
                ++cn;
            }
        }
    }
    // 0 if even (out), and 1 if odd (in)
    return(cn&1);
}

//*********************************************************************
//                      inRestrict
//*********************************************************************

bool AreaZone::inRestrict(char tile, const char *list) const {
    int i=strlen(list);
    if(!i)
        return(true);
    for(;i>=0;i--)
        if(tile == list[i])
            return(true);
    return(false);
}


bool AreaZone::flagIsSet(int flag) const {
    return(flags[flag/8] & 1<<(flag%8));
}

//*********************************************************************
//                      TileInfo
//*********************************************************************

TileInfo::TileInfo() {
    display = '#';
    id = style = 0;
    name = description = fishing = "";
    trackDur = cost = fly = 0;
    vision = 0.0;
    zero(flags, sizeof(flags));
    water = road = false;
    herbs.clear();
}


char TileInfo::getId() const { return(id); }
std::string TileInfo::getName() const { return(name); }
std::string TileInfo::getDescription() const { return(description); }
short TileInfo::getCost() const { return(cost); }
float TileInfo::getVision() const { return(vision); }
char TileInfo::getDisplay() const { return(display); }
short TileInfo::getTrackDur() const { return(trackDur); }
bool TileInfo::flagIsSet(int flag) const {
    return(flags[flag/8] & 1<<(flag%8));
}
bool TileInfo::isWater() const { return(water); }
bool TileInfo::isRoad() const { return(road); }
short TileInfo::getFly() const { return(fly); }

//*********************************************************************
//                      getStyle
//*********************************************************************

char TileInfo::getStyle(const Player* player) const {
    if(player && player->flagIsSet(P_INVERT_AREA_COLOR)) {
        // toggle case
        if(isupper(style))
            return(tolower(style));
        else if(islower(style))
            return(toupper(style));
    }
    return(style);
}


//*********************************************************************
//                      AreaData
//*********************************************************************

AreaData::AreaData() {
    area = nullptr;
    isTerrain = true;
}

char AreaData::get(short x, short y, short z) const {
    try {
        return (data.at(z).at(y).at(x));
    } catch(std::out_of_range &e) {
        return (area->errorTerrain);
    }
}

void AreaData::setArea(Area* a) { area = a; }
void AreaData::setTerrain(bool t) { isTerrain = t; }

AreaData::~AreaData() {
    data.clear();
}

//*********************************************************************
//                      Area
//*********************************************************************

Area::Area() {
    id = 0;
    width = height = depth = minDepth = 0;
    name = "";
    defaultTerrain = errorTerrain = 0;
    critical_x = critical_y = critical_z = 0;
    zero_offset_x = zero_offset_y = zero_offset_z = 0;
    flightPower = 0;

    aTerrain.setArea(this);
    aMap.setArea(this);
    aMap.setTerrain(false);
    aSeason.setArea(this);
    aSeason.setTerrain(false);
}

Area::~Area() {
    for (auto const& [roomId, room] : rooms) {
        delete room;
    }
    rooms.clear();

    for(auto const& zone : zones) {
        delete zone;
    }
    zones.clear();

    for (auto const& [tileId, tile] : ter_tiles) {
        delete tile;
    }
    ter_tiles.clear();

    for (auto const& [tileId, tile] : map_tiles) {
        delete tile;
    }
    map_tiles.clear();

    for( auto const& aTrack : tracks) {
        delete aTrack;
    }
    tracks.clear();
}


//*********************************************************************
//                      getUnique
//*********************************************************************

CatRef Area::getUnique(const MapMarker *mapmarker) const {
    for (auto const& zone : zones) {
        if(zone->unique.id && zone->inside(this, mapmarker))
            return(zone->unique);
    }

    return(CatRef{});
}


//*********************************************************************
//                      move
//*********************************************************************

// this function does not check rules for moving
bool Area::move(Player* player, MapMarker *mapmarker) {
    bool    mem=false;
    // does it already exist?
    AreaRoom* room = getRoom(mapmarker);

    if(!room) {
        room = player->getAreaRoomParent();

        if(room && room->canDelete()) {
            room->setMapMarker(mapmarker);
            room->recycle();
        } else {
            room = new AreaRoom(this, mapmarker);
        }
    }

    // the room we are going to (could be the same room!) should not be
    // deleted after everyone leaves
    mem = room->getStayInMemory();
    room->setStayInMemory(true);

    // everyone leaves room
    BaseRoom* old_room = player->getRoomParent();
    player->deleteFromRoom();

    room->killMortalObjects();

    // everyone enters room
    player->addToRoom(room);
    player->doFollow(old_room);

    // back to normal
    room->setStayInMemory(mem);
    return(true);
}

//*********************************************************************
//                      remove
//*********************************************************************

void Area::remove(AreaRoom* room) {
    if(!room)
        return;
    rooms.erase(room->mapmarker.str());
    delete room;
}


//*********************************************************************
//                      loadRoom
//*********************************************************************
// loads the room, including options for recycling, if needed.
// creature is allowed to be null.

AreaRoom *Area::loadRoom(Creature* creature, const MapMarker* mapmarker, bool recycle, bool p) {

    // we only care about this if we have a creature
    if(creature) {
        // impassable terrain
        if(!canPass(creature, mapmarker, true)) {
            if(!(creature->isPet() && creature->getConstMaster()->isStaff())) {
                if(p) creature->checkStaff("You can't go there!\n");
                if(!creature->isStaff()) return(nullptr);
            }
        }
    }

    // does it already exist?
    AreaRoom* room = getRoom(mapmarker);

    // because an AreaRoom can only be a candidate for recycling once
    // it is completely empty, we'll just use this room for now
    if(!room && creature && recycle)
        room = creature->getAreaRoomParent();
    if(!room)
        room = new AreaRoom(this, mapmarker);

    return(room);
}


//*********************************************************************
//                      checkCycle
//*********************************************************************
// this set of functions forces us to uses a bounded map that cycles
// when we get too far away from the center

void Area::checkCycle(MapMarker *mapmarker) const {
    mapmarker->setX(checkCycle(mapmarker->getX(), critical_x));
    mapmarker->setY(checkCycle(mapmarker->getY(), critical_y));
    mapmarker->setZ(checkCycle(mapmarker->getZ(), critical_z));
}

short Area::checkCycle(short vector, short critical) const {
    if(critical) {
        if(vector > critical)
            vector = vector % critical - critical - 1;
        else if(vector < (critical * -1))
            vector = vector % critical + critical + 1;
    }
    return(vector);
}

//*********************************************************************
//                      canPass
//*********************************************************************
// Tells if the creature can pass through this terrain. Send a null
// creature to simulate a normal person

bool Area::canPass(const Creature* creature, const MapMarker *mapmarker, bool adjust) const {
    int fly = 0;
    TileInfo* tile=nullptr;

    short x = mapmarker->getX();
    short y = mapmarker->getY();
    short z = mapmarker->getZ();

    if(adjust)
        adjustCoords(&x, &y, &z);

    if(outOfBounds(x, y, z))
        tile = getTile(defaultTerrain, 0);
    else if(isRoad(x, y, z))
        return(true);
    else
        tile = getTile(aTerrain.get(x,y,z), aSeason.get(x,y,z));

    if(!tile)
        return(false);

    if(creature && creature->isEffected("fly"))
        fly = creature->getEffect("fly")->getStrength();

    // code swimming rules later
    if(tile->isWater() && !fly)
        return(false);

    // can they fly over it?
    if(tile->getFly() == -1)
        return(false);

    return(fly >= tile->getFly());
}


//*********************************************************************
//                      isRoad
//*********************************************************************

bool Area::isRoad(short x, short y, short z, bool adjust) const {
    if(adjust)
        adjustCoords(&x, &y, &z);
    if(outOfBounds(x, y, z))
        return(false);
    TileInfo* tile = getTile(aMap.get(x,y,z), 0);
    return(tile && tile->isRoad());
}


//*********************************************************************
//                      isWater
//*********************************************************************

bool Area::isWater(short x, short y, short z, bool adjust) const {
    if(adjust)
        adjustCoords(&x, &y, &z);
    TileInfo* tile = nullptr;
    if(outOfBounds(x, y, z))
        tile = getTile(defaultTerrain, 0);
    else
        tile = getTile(aTerrain.get(x,y,z), aSeason.get(x,y,z));
    return(tile && tile->isWater());
}

//*********************************************************************
//                      getTile
//*********************************************************************
// when getting a tile stored in memory, we may get a different tile
// during a particular season. this is how rivers freeze during the winter

TileInfo *Area::getTile(char grid, char seasonFlags, Season season, bool checkSeason) const {
    // figure out rules for season
    if(checkSeason)
        season = gConfig->getCalendar()->whatSeason();

    // pow() turns season (1-4) to binary (1,2,4,8)
    // if the season's flag isnt set, then we don't look for alternate season info
    if(season != NO_SEASON && !(seasonFlags & (int)pow(2, (double)season-1)))
        season = NO_SEASON;

    auto it = ter_tiles.find(grid);
    if(it != ter_tiles.end()) {
        TileInfo* tile = (*it).second;
        if(season != NO_SEASON) {
            auto st = tile->season.find(season);
            if(st != tile->season.end()) {
                it = ter_tiles.find((*st).second);
                // only switch if we can find the new tile
                if(it != ter_tiles.end())
                    tile = (*it).second;
            }
        }
        return(tile);
    }
    it = map_tiles.find(grid);
    if(it != map_tiles.end())
        return((*it).second);
    return(nullptr);
}

//*********************************************************************
//                      getTerrain
//*********************************************************************

char Area::getTerrain(const Player* player, const MapMarker *mapmarker, short y, short x, short z, bool terOnly) const {
    AreaRoom* room=nullptr;
    bool    staff = player != nullptr && player->isStaff();
    bool    found=false;

    // If given a player, vision may be distorted based on properties on the player
    if(player) {
        MapMarker m = *mapmarker;

        // adjust our mapmarker
        m.add(x, y, z);
        auto it = rooms.find(m.str());
        if(it != rooms.end()) {
            room = it->second;

            if(!room->players.empty() || !room->monsters.empty()) {
                if(!staff && room->isMagicDark()) {
                    return ('*');
                }

                // can they see anybody in the room?
                for(Player* ply : room->players) {
                    if(player == ply || (player->canSee(ply) && (staff || !ply->flagIsSet(P_HIDDEN)))) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    for(Monster* mons : room->monsters) {
                        if( player->canSee(mons) && (staff || !mons->flagIsSet(M_HIDDEN))) {
                            found = true;
                            break;
                        }
                    }
                }

                // if so, show them the creature symbol
                if(found)
                    return('@');
            }
        }
    }

    // adjust our coordinates
    x += mapmarker->getX();
    y += mapmarker->getY();
    z += mapmarker->getZ();

    adjustCoords(&x, &y, &z);

    if(outOfBounds(x, y, z))
        return(defaultTerrain);

    if(!terOnly && aMap.get(x,y,z) != ' ')
        return(aMap.get(x,y,z));
    return(aTerrain.get(x,y,z));
}

//*********************************************************************
//                      getSeasonFlags
//*********************************************************************

char Area::getSeasonFlags(const MapMarker *mapmarker, short y, short x, short z) const {
    // adjust our coordinates
    x += mapmarker->getX();
    y += mapmarker->getY();
    z += mapmarker->getZ();

    adjustCoords(&x, &y, &z);

    if(outOfBounds(x, y, z))
        return(0);
    return(aSeason.get(x,y,z));
}


//*********************************************************************
//                      getLosPower
//*********************************************************************

float Area::getLosPower(const Player* player, int xVision, int yVision) const {
    // default
    float power = xVision - 0.5;
    // elves have great vision
    if(player->getRace())
        power += 0.5;
    // drow and vampires can see well at night
    if( !isDay() &&
        (player->getRace() == DARKELF || player->isEffected("vampirism"))
    )
        power += 0.5;
    return(power);
}


//*********************************************************************
//                      getGridText
//*********************************************************************

void Area::getGridText(char grid[][80], int height, const MapMarker *mapmarker, int maxWidth) const {
   TileInfo *tile = getTile(getTerrain(nullptr, mapmarker, 0, 0, 0, true), getSeasonFlags(mapmarker));
    std::string desc = tile ? tile->getDescription() : "";
    if(maxWidth < 80)
        desc = wrapText(desc, maxWidth);
    if(desc == "\n")
        desc = "";

    int     i=0, n=0, k=0, lines=1, offset=0;
    bool    displayed = (desc.empty());

    for(i=0; i<height; i++)
        strcpy(grid[i], "");

    // see if there is any zone text to add
    for (auto const& zone : zones) {
        if(!zone->display.empty() && zone->inside(this, mapmarker)) {
            if(!displayed) {
                desc += "\n";
                displayed = true;
            }
            desc += "\n";
            if(maxWidth < 80)
                desc += wrapText(zone->display, maxWidth);
            else
                desc += zone->display;
        }
    }

    // if we know how many lines the text will take up, we can adjust its
    // starting point based on how much room we have
    for (char const &c: desc)
        if(c == '\n')
            lines++;

    // time to move the description into the array
    offset = MAX(0, (height - lines) / 2);
    k = desc.size()-1;
    n=0;

    for(i=0; i<=k && offset<height; i++) {
        if(desc.at(i) == '\n' || i==k) {
            if(i==k)
                i++;
            strncpy(grid[offset++], desc.substr(n, i-n).c_str(), 79);
            n = i+1;
        }
    }
}

//*********************************************************************
//                      showGrid
//*********************************************************************

std::string Area::showGrid(const Player* player, const MapMarker *mapmarker, bool compass) const {
    std::list<AreaZone*>::const_iterator it;
    std::string border = "";
    std::ostringstream grid;
    int     xVision = player->getVision();
    int     yVision = xVision * 2 / 3;
    bool    staff = player->isStaff();
    int     my = yVision*2+1, mx = xVision*2+1, y=0, x=0, i=0;
    int     zx=0, zy=0, zi=0;
    char    gridText[my][80];
    char    seasonFlags;
    Season  season = gConfig->getCalendar()->whatSeason();
    TileInfo *tile=nullptr;
    MapMarker m = *mapmarker;

    zero(gridText, sizeof(gridText));
    getGridText(gridText, my, &m, player->getSock()->getTermCols()-mx-5);

    // line of sight
    float   losGrid[my][mx];
    float   losPower = getLosPower(player, xVision, yVision);
    makeLosGrid(*losGrid, player, my, mx, mapmarker);

    // these are only for staff who want to see zones
    bool    zInside=false;
    int     zZone = 0, zHigh = 0;
    char    zColor[] = {'r', 'w', 'm', 'M', 'W', 'R'};
    int     zNum = sizeof(zColor) / sizeof(char);
    bool    resetBlink=false;

    border += ".";
    i=0;
    for(y = yVision; y >= yVision * -1; y--) {
        grid << "|";
        for(x = xVision * -1; x <= xVision; x++) {
            if(!y)
                border += "-";

            // This lets them see the edges of mountains, even though they
            // can't see over the mountains themselves.
            zy = y;
            zx = x;
            if(!staff && losPower < losGrid[zy+yVision][zx+xVision]) {
                zi++;
                losCloser(&zy, &zx, 0, 0, 0);
            }

            if(!staff && losPower < losGrid[zy+yVision][zx+xVision])
                grid << " ";
            else {
                seasonFlags = getSeasonFlags(mapmarker, y, x);
                tile = getTile(getTerrain(player, mapmarker, y, x, 0, false), seasonFlags, season, false);

                if(!tile)
                    grid << " " ;
                else {
                    grid << "^";

                    // if they want to view zones, we're going to have to look
                    // at every zone for every square of the map
                    if(player->isCt() && player->flagIsSet(P_VIEW_ZONES)) {
                        m.add(x, y, 0);
                        zInside = false;
                        zZone = 0;
                        for(it = zones.begin() ; it != zones.end() ; it++) {
                            zZone++;
                            if((*it)->inside(this, mapmarker)) {
                                zHigh = zZone;
                                zInside = true;
                            }
                        }
                        if(!zInside)
                            grid << tile->getStyle(player);
                        else
                            grid << zColor[(zHigh-1)%zNum];
                        m.add(x*-1, y*-1, 0);
                    } else {
                        if(tile->getDisplay() == '@' && !x && !y) {
                            std::string self = player->customColorize("*CC:SELF*", false);
                            if(self == "#") {
                                grid << "x^";
                                resetBlink = true;
                            }
                            grid << self;
                        } else {
                            if(resetBlink) {
                                grid << "x^";
                                resetBlink = false;
                            }
                            grid << tile->getStyle(player);
                        }
                    }


                    grid << tile->getDisplay();

                    // escape character
                    if(tile->getDisplay() == '^')
                        grid << "^";
                }
            }
        }
        grid << "^w|   ";
        grid << gridText[i++];
        grid << "\n";
    }
    border += ".\n";

    grid << border;
    //player->printColor("%s%s%s", border.c_str(), grid.str().c_str(), border.c_str());

    // if they have a compass equipped
    if(compass)
        for(i=0; i<MAXWEAR; i++)
            if(player->ready[i] && player->ready[i]->compass)
                grid << player->ready[i]->getCompass(player, true);

    border += grid.str();
    return(border);
}

//*********************************************************************
//                      outOfBounds
//*********************************************************************
// don't access outside of the array

bool Area::outOfBounds(short x, short y, short z) const {
    return(y < 0 || x < 0 || z < 0 || y >= height || x >= width || z >= depth);
}

//*********************************************************************
//                      adjustCoords
//*********************************************************************
// adjust the points from map to container

void Area::adjustCoords(short* x, short* y, short* z) const {
    (*x) += zero_offset_x;
    (*y) *= -1;
    (*y) += zero_offset_y;
    (*z) += zero_offset_z;
}

//*********************************************************************
//                      getTrack
//*********************************************************************
// search, return if found

Track* Area::getTrack(MapMarker* mapmarker) const {
    std::list<AreaTrack*>::const_iterator it;
    AreaTrack *aTrack=nullptr;

    for(it = tracks.begin() ; it != tracks.end() ; it++) {
        aTrack = (*it);
        if(*&aTrack->mapmarker == *mapmarker)
            return(&aTrack->track);
    }
    return(nullptr);
}


//*********************************************************************
//                      addTrack
//*********************************************************************
// add, pop from end if too many

void Area::addTrack(AreaTrack *aTrack) {
    tracks.push_front(aTrack);
    while(tracks.size() > MAX_AREA_TRACK)
        tracks.pop_back();
}


//*********************************************************************
//                      getTrackDuration
//*********************************************************************
// how long will the tracks last?

int Area::getTrackDuration(const MapMarker* mapmarker) const {
    TileInfo *tile = getTile(getTerrain(nullptr, mapmarker, 0, 0, 0, true), getSeasonFlags(mapmarker));
    return(tile ? tile->getTrackDur() : 0);
}


//*********************************************************************
//                      updateTrack
//*********************************************************************
// make sure tracks don't stay around for too long

void Area::updateTrack(int t) {
    auto it = tracks.begin();
    AreaTrack *aTrack=nullptr;

    while(it != tracks.end()) {
        auto next = it;
        ++next;
        aTrack = (*it);
        aTrack->setDuration(aTrack->getDuration() - t);
        if(aTrack->getDuration() <= 0) {
            delete aTrack;
            tracks.erase(it);
        }
        it = next;
    }
}


//*********************************************************************
//                      checkFileSize
//*********************************************************************
// if the terrain, map, and season files are different sizes, that
// is a bad thing.

void Area::checkFileSize(int& size, const char* filename) const {
    struct stat f_stat{};
    if(stat(filename, &f_stat))
        return;
    if(!size) {
        size = f_stat.st_size;
        return;
    }
    if(size != f_stat.st_size) {
        std::clog << "Area file sizes are not the same. Aborting.\n";
        crash(-1);
    }
}

//*********************************************************************
//                      loadTerrain
//*********************************************************************

void Area::loadTerrain(int minDepth) {
    bool    gotOffset=false;
    int     i=0, n=0, k=minDepth, len=0, size=0;
    char    filename[256];
    char    storage[MAX(height, width)+1];

    while(k < depth) {
        sprintf(filename, "%s/%s.%d.ter", Path::AreaData, dataFile, k);
        if(!file_exists(filename))
            return;

        checkFileSize(size, filename);
        std::fstream t(filename, std::ios::in);
        i=0;
        std::vector< std::vector<char> > vTer;
        while(!t.eof() && i < height) {
            std::vector<char> v;

            t.getline(storage, width+1);
            len = strlen(storage);
            v.reserve(len);

            for(n=0; n<len; n++)
                v.push_back(storage[n]);
            vTer.push_back(v);
            i++;
        }
        t.close();
        aTerrain.data.push_back(vTer);

        sprintf(filename, "%s/%s.%d.map", Path::AreaData, dataFile, k);
        std::vector< std::vector<char> > vMap;
        if(file_exists(filename)) {
            checkFileSize(size, filename);
            std::fstream m(filename, std::ios::in);
            i=0;
            while(!m.eof() && i < height) {
                std::vector<char> v;

                m.getline(storage, width+1);
                len = strlen(storage);
                v.reserve(len);

                for(n=0; n<len; n++) {
                    if(storage[n] == '*') {
                        v.push_back(' ');
                        if(!gotOffset) {
                            zero_offset_x = n;
                            zero_offset_y = i;
                            zero_offset_z = k - minDepth;
                            gotOffset = true;
                        }
                    } else {
                        v.push_back(storage[n]);
                    }
                }
                vMap.push_back(v);
                i++;
            }
            m.close();
        } else {
            // if we don't get a file, fill with emptyness
            for(i=0; i<height; i++) {
                std::vector<char> v;
                v.reserve(width);

                for(n=0; n<width; n++)
                    v.push_back(' ');
                vMap.push_back(v);
            }
        }
        aMap.data.push_back(vMap);

        sprintf(filename, "%s/%s.%d.sn", Path::AreaData, dataFile, k);
        std::vector< std::vector<char> > vSn;
        if(file_exists(filename)) {
            checkFileSize(size, filename);
            std::fstream s(filename, std::ios::in);
            i=0;
            while(!s.eof() && i < height) {
                std::vector<char> v;

                s.getline(storage, width+1);
                len = strlen(storage);
                v.reserve(len);

                // convert from ascii numbers to actual numbers
                for(n=0; n<len; n++)
                    v.push_back(storage[n]-48);
                vSn.push_back(v);
                i++;
            }
            s.close();
        } else {
            // if we don't get a file, fill with emptyness
            for(i=0; i<height; i++) {
                std::vector<char> v;
                v.reserve(width);

                for(n=0; n<width; n++)
                    v.push_back(0);
                vSn.push_back(v);
            }
        }
        aSeason.data.push_back(vSn);

        k++;
    }
}



//*********************************************************************
//                      flagIsSet
//*********************************************************************

bool Area::flagIsSet(int flag, const MapMarker* mapmarker) const {
    TileInfo *tile = getTile(getTerrain(nullptr, mapmarker, 0, 0, 0, true), getSeasonFlags(mapmarker));
    if(!tile)
        return(false);
    if(tile->flagIsSet(flag))
        return(true);

    std::list<AreaZone*>::const_iterator it;
    AreaZone *zone=nullptr;

    for(it = zones.begin() ; it != zones.end() ; it++) {
        zone = (*it);
        if(zone->flagIsSet(flag) && zone->inside(this, mapmarker))
            return(true);
    }
    return(false);
}


//*********************************************************************
//                      dmListArea
//*********************************************************************

// a prototype needed for only this function: from dmroom.cpp
void showMobList(Player* player, WanderInfo *wander, std::string_view type);

int dmListArea(Player* player, cmd* cmnd) {
    int     a=0;
    std::string str = getFullstrText(cmnd->fullstr, 1);

    if(!str.empty())
        a = atoi(str.c_str());
    if(!a)
        player->printColor("You may type ^y*arealist [num]^x to display only a specific area.\n\n");

    bool empty = cmnd->num >= 2 && !strcmp(cmnd->str[1], "all");
    bool tiles = cmnd->num >= 2 && !strcmp(cmnd->str[1], "tile");
    bool zones = cmnd->num >= 2 && !strcmp(cmnd->str[1], "zones");
    bool coords = cmnd->num >= 3 && !strcmp(cmnd->str[1], "zones") && !strcmp(cmnd->str[2], "coords");
    bool wander = cmnd->num >= 3 && (!strcmp(cmnd->str[1], "zones") || !strcmp(cmnd->str[1], "tile")) && !strcmp(cmnd->str[2], "wander");
    bool track = cmnd->num >= 2 && !strcmp(cmnd->str[1], "track");

    for(auto area : gServer->areas) {
        if(a && a != area->id)
            continue;
        player->printColor("Area: ^C%s\n", area->name.c_str());
        player->printColor("  ID: ^c%d^x, DataFile: ^c%s\n", area->id, area->dataFile);
        player->printColor("  Critical Cycle X: ^c%d^x, Y: ^c%d^x, Z: ^c%d\n", area->critical_x, area->critical_y, area->critical_z);
        player->printColor("  Zero Coord Offset X: ^c%d^x, Y: ^c%d^x, Z: ^c%d\n", area->zero_offset_x, area->zero_offset_y, area->zero_offset_z);
        player->printColor("  Height: ^c%d^x, Width: ^c%d^x, Depth: ^c%d\n", area->height, area->width, area->depth);
        player->printColor("  DefaultTerrain: %c  ErrorTerrain: %c  FlightPower: ^c%d\n", area->defaultTerrain, area->errorTerrain, area->flightPower);

        player->printColor("  Rooms Available: ^c%d\n", area->height * area->width);
        player->printColor("  Rooms In Memory: ^c%d\n", area->rooms.size());
        if(!empty) {
            if(a)
                player->printColor("     ^yTo show empty rooms in memory, type \"*arealist %d all\".\n", a);
            else
                player->printColor("     ^yTo show empty rooms in memory, type \"*arealist all\".\n");
        }
        for(const auto& [roomId, room] : area->rooms) {
            if(!room->players.empty() || !room->monsters.empty() || !room->objects.empty() || empty) {
                player->printColor("     %-16s Ply: %s^x  Mon: %s^x  Obj: %s\n",
                    room->fullName().c_str(), !room->players.empty() ? "^gy" : "^rn",
                    !room->monsters.empty() ? "^gy" : "^rn", !room->objects.empty() ? "^gy" : "^rn");
            }
        }
        player->print("\n");

        if(tiles) {
            if(!wander) {
                if(a)
                    player->printColor("^yTo show tile wander info, type \"*arealist %d tile wander\".\n", a);
                else
                    player->printColor("^yTo show tile wander info, type \"*arealist tile wander\".\n");
            }

            for(const auto& [tId, tile] : area->ter_tiles) {
                player->print("Ter Tile: %-17s ID: %c  ", tile->getName().c_str(), tile->getId());
                if(wander) {
                    player->print("\n");
                    showMobList(player, &tile->wander, "tile");
                } else {
                    player->printColor("^%cStyle: %c^x   ", tile->getStyle(), tile->getStyle());
                    player->printColor("Display: %c  Cost: %d  Vision: %-4.1f  TrackDur: %-4d Fly: %2d",
                        tile->getDisplay(), tile->getCost(), tile->getVision(), tile->getTrackDur(),
                        tile->getFly());
                    if(tile->isWater())
                        player->printColor("  ^BWater: ^x(^c%s^x)", tile->getFishing().c_str());
                    player->print("\n");
                    showRoomFlags(player, nullptr, tile, nullptr);
                }
            }

            if(!wander) {
                player->print("\n");
                for(const auto& [tId, tile] : area->map_tiles) {
                    player->print("Map Tile: %-17s ID: %c   ", tile->getName().c_str(), tile->getId());
                    player->printColor("^%cStyle: %c^x  ", tile->getStyle(), tile->getStyle());
                    player->printColor("Display: %c%s\n", tile->getDisplay(), tile->isRoad() ? "  ^WRoad" : "");
                }
            }

        } else {
            if(a)
                player->printColor("^yTo show tile information, type \"*arealist %d tile\".\n", a);
            else
                player->printColor("^yTo show tile information, type \"*arealist tile\".\n");
        }

        if(zones) {
            if(!coords) {
                if(a)
                    player->printColor("^yTo show zone coordinates, type \"*arealist %d zones coords\".\n", a);
                else
                    player->printColor("^yTo show zone coordinates, type \"*arealist zones coords\".\n");
            }
            if(!wander) {
                if(a)
                    player->printColor("^yTo show zone wander info, type \"*arealist %d zones wander\".\n", a);
                else
                    player->printColor("^yTo show zone wander info, type \"*arealist zones wander\".\n");
            }
            for(const auto& zone : area->zones) {
                player->printColor("Name: ^c%s\n", zone->name.c_str());
                if(wander) {
                    showMobList(player, &zone->wander, "zone");
                } else {
                    player->printColor("   Display: ^c%s\n", zone->display.c_str());
                    player->printColor("   TerRestrict: ^c%s^x  MapRestrict: ^c%s\n",
                        zone->terRestrict, zone->mapRestrict);
                    player->printColor("   Unique: ^c%s\n", zone->unique.displayStr().c_str());
                    player->printColor("   Min: (X:^c%d^x Y:^c%d^x Z:^c%d^x) Max: (X:^c%d^x Y:^c%d^x Z:^c%d^x)\n",
                        zone->min.getX(), zone->min.getY(), zone->min.getZ(),
                        zone->max.getX(), zone->max.getY(), zone->max.getZ());
                    if(!zone->getFishing().empty())
                        player->printColor("   Fishing: ^c%s\n", zone->getFishing().c_str());
                    player->print("   ");
                    showRoomFlags(player, nullptr, nullptr, zone);

                    if(coords) {
                        player->print("   Coords:\n");
                        for(const auto& [c, mapmarker] : zone->coords) {
                            player->printColor("      (X:^c%d^x Y:^c%d^x Z:^c%d^x)\n",
                                mapmarker->getX(), mapmarker->getY(), mapmarker->getZ());
                        }
                    }
                }
            }
            player->print("\n");

        } else {
            if(a)
                player->printColor("^yTo show zone information, type \"*arealist %d zones\".\n", a);
            else
                player->printColor("^yTo show zone information, type \"*arealist zones\".\n");
        }

        if(track) {
            player->printColor("Track Objects: ^c%d\n", area->tracks.size());

            for(const auto& aTrack : area->tracks)  {
                player->print("   MapMarker: %s  Dur: %d   Dir: %s \n",
                    aTrack->mapmarker.str().c_str(), aTrack->getDuration(), aTrack->track.getDirection().c_str());
            }

        } else {
            if(a)
                player->printColor("^yTo show track information, type \"*arealist %d track\".\n", a);
            else
                player->printColor("^yTo show track information, type \"*arealist track\".\n");
        }
    }
    player->print("\n");
    return(0);
}

void Server::areaInit(Creature* player, MapMarker mapmarker) {
    Area *area = getArea(mapmarker.getArea());
    if(area)
        player->currentLocation.mapmarker = mapmarker;
//  AreaRoom* aRoom=0;
//  if(area)
//      aRoom = area->loadRoom(0, &mapmarker, false);
//  player->area_room = aRoom;
}

//*********************************************************************
//                      losCloser
//*********************************************************************
// navigating our grid, move closer to the goal

void Area::losCloser(int *x, int *y, int me_x, int me_y, int i) const {
    if( (abs(me_y - (*y)) == abs(me_x - (*x))) || (
            i%2 &&
            !((*y)==me_y || (*x)==me_x)
        )
    ) {
        // do some diagonal movement!
        (*y) += (me_y<(*y) ? -1 : 1);
        (*x) += (me_x<(*x) ? -1 : 1);
    } else {
        // do some straight movement
        if(abs(me_y - (*y)) > abs(me_x - (*x)))
            (*y) += (me_y<(*y) ? -1 : 1);
        else
            (*x) += (me_x<(*x) ? -1 : 1);
    }
}

//*********************************************************************
//                      lineOfSight
//*********************************************************************
// recursive function to place values in our grid

float Area::lineOfSight(float *grid, const Player* player, int width, int *y, int *x, int me_y, int me_x, int *i, const MapMarker *mapmarker) const {
    int     og_y = (*y), og_x = (*x);
    float   *g, *h, cost=0.0;
    TileInfo *tile=nullptr;

    // Calculate the position in the array you're going after
    g = grid + (*y) * width + (*x);
    h = grid + og_y * width + og_x;

    // check to see if float is empty
    if((int)(*h))
        return(*h);

    if((*y)==me_y && (*x)==me_x) {
        (*g) = 0;
    } else {

        // see how much this piece of terrain costs
        tile = getTile(getTerrain(nullptr, mapmarker, og_y - me_y, og_x - me_x, 0, true), getSeasonFlags(mapmarker, og_y - me_y, og_x - me_x));
        cost = tile ? tile->getVision() : 0;
        if(isRoad(og_x - me_x + mapmarker->getX(), og_y - me_y + mapmarker->getY(), mapmarker->getZ(), true))
            cost /= 4 ;

        // Profiling: Check flightpower before checking effect. Flightpower is less expensive
        // flying people can see further
        if(flightPower && player->isEffected("fly"))
            cost /= flightPower;

        // base cost for distance
        cost += 1;

        // move closer to the goal
        (*i)++;
        losCloser(y, x, me_y, me_x, *i);

        // diagonal
        if((*y)!=og_y && (*x)!=og_x)
            cost += 0.5;
        // we need to make an oval because our display grid is not square
        if((*y)!=og_y && (*x)==og_x)
            cost += 0.5;

        // have we already calculated this location?
        // check to see if float is empty
        if((int)(*g))
            (*h) = cost + (*g);
        else
            (*h) = cost + lineOfSight(grid, player, width, y, x, me_y, me_x, i, mapmarker);
    }

    return(*h);
}

//*********************************************************************
//                      makeLosGrid
//*********************************************************************
// generates line of sight values in supplied grid

void Area::makeLosGrid(float *grid, const Player* player, int height, int width, const MapMarker *mapmarker) const {
    int     x=0, y=0, me_x = (width-1)/2, me_y = (height-1)/2;
    int     zx=0, zy=0, zi=0;
    float   *g;
    MapMarker m = *mapmarker;

    zero(grid, height*width*sizeof(float));

    for(y=0; y<height; y++) {
        zi = 1;
        zx = 0;
        zy = y;
        lineOfSight(grid, player, width, &zy, &zx, me_y, me_x, &zi, &m);
        zi = 1;
        zx = width-1;
        zy = y;
        lineOfSight(grid, player, width, &zy, &zx, me_y, me_x, &zi, &m);

        if(y == 0 || y == height-1) {
            for(x=0; x<width-1; x++) {
                zi = 1;
                zx = x;
                zy = y;
                lineOfSight(grid, player, width, &zy, &zx, me_y, me_x, &zi, &m);
            }
        }

    }
    // verification
    for(y=0; y<height; y++) {
        for(x=0; x<width-1; x++) {
            g = grid + y * width + x;
            if((int)(*g))
                continue;
            zi = 1;
            zx = x;
            zy = y;
            lineOfSight(grid, player, width, &zy, &zx, me_y, me_x, &zi, &m);
        }
    }
}

//*********************************************************************
//                      clearAreas
//*********************************************************************

void Server::clearAreas() {
    Area    *area=nullptr;

    while(!areas.empty()) {
        area = areas.front();
        delete area;
        areas.pop_front();
    }
    areas.clear();
}

//*********************************************************************
//                      getArea
//*********************************************************************

Area *Server::getArea(int id) {
    for(auto area: areas) {
        if(area->id == id)
            return(area);
    }
    return(nullptr);
}

//*********************************************************************
//                      cleanUpRooms
//*********************************************************************

void Server::cleanUpAreas() {
    for(auto area : areas) {
        area->cleanUpRooms();
    }
}

//*********************************************************************
//                      cleanUpRooms
//*********************************************************************

void Area::cleanUpRooms() {
    std::map<std::string, AreaRoom*>::iterator it;
    AreaRoom* room=nullptr;

    for(it = rooms.begin() ; it != rooms.end() ; ) {
        room = (*it).second;
        it++;

        if(room->canDelete()) {
            rooms.erase(room->mapmarker.str());
            delete room;
        }
    }
}
