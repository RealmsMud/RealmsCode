/*
 * area-xml.cpp
 *   Overland map - xml
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

#include <dirent.h>                                 // for closedir, opendir
#include <libxml/parser.h>                          // for xmlFreeDoc, xmlCl...
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <cstdio>                                   // for sprintf
#include <list>                                     // for list, list<>::con...
#include <map>                                      // for map, operator==
#include <ostream>                                  // for basic_ostream::op...
#include <string>                                   // for string, allocator
#include <utility>                                  // for pair

#include "area.hpp"                                 // for Area, TileInfo
#include "catRef.hpp"                               // for CatRef
#include "flags.hpp"                                // for MAX_ROOM_FLAGS
#include "global.hpp"                               // for FATAL
#include <libxml/xmlstring.h>                       // for BAD_CAST
#include "mudObjects/areaRooms.hpp"                 // for AreaRoom
#include "os.hpp"                                   // for merror
#include "paths.hpp"                                // for AreaRoom, AreaData
#include "season.hpp"                               // for Season
#include "server.hpp"                               // for Server
#include "wanderInfo.hpp"                           // for WanderInfo
#include "xml.hpp"                                  // for saveNonZeroNum
#include "proto.hpp"                                // for merror

class Creature;

//*********************************************************************
//                      load
//*********************************************************************

void MapMarker::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    reset();

    while(childNode) {
        if(NODE_NAME(childNode, "Area")) xml::copyToNum(area, childNode);
        else if(NODE_NAME(childNode, "X")) xml::copyToNum(x, childNode);
        else if(NODE_NAME(childNode, "Y")) xml::copyToNum(y, childNode);
        else if(NODE_NAME(childNode, "Z")) xml::copyToNum(z, childNode);

        childNode = childNode->next;
    }
}



//*********************************************************************
//                      save
//*********************************************************************

void MapMarker::save(xmlNodePtr curNode) const {
    xml::saveNonZeroNum(curNode, "Area", area);
    xml::saveNonZeroNum(curNode, "X", x);
    xml::saveNonZeroNum(curNode, "Y", y);
    xml::saveNonZeroNum(curNode, "Z", z);
}

void AreaZone::save(xmlNodePtr curNode) const {
    xmlNodePtr subNode, childNode;

    xml::saveNonNullString(curNode, "Name", name);
    xml::saveNonNullString(curNode, "Display", display);
    xml::saveNonNullString(curNode, "Fishing", fishing);
    xml::saveNonNullString(curNode, "TerRestrict", terRestrict);
    xml::saveNonNullString(curNode, "MapRestrict", mapRestrict);
    unique.save(curNode, "Unique", false);
    saveBitset(curNode, "Flags", MAX_ROOM_FLAGS, flags);
    wander.save(curNode);

    childNode = xml::newStringChild(curNode, "Coords");
    std::map<int, MapMarker*>::const_iterator it;
    for(it = coords.begin() ; it != coords.end() ; it++) {
        subNode = xml::newStringChild(childNode, "MapMarker");
        (*it).second->save(subNode);
    }
}


//*********************************************************************
//                      load
//*********************************************************************

void AreaZone::load(xmlNodePtr curNode) {
    xmlNodePtr mNode, childNode = curNode->children;
    MapMarker *mapmarker=nullptr;
    int     i=0;

    while(childNode) {
        if(NODE_NAME(childNode, "Name")) { xml::copyToString(name, childNode); }
        else if(NODE_NAME(childNode, "Fishing")) { xml::copyToString(fishing, childNode); }
        else if(NODE_NAME(childNode, "Display")) { xml::copyToString(display, childNode); }
        else if(NODE_NAME(childNode, "TerRestrict")) xml::copyToCString(terRestrict, childNode);
        else if(NODE_NAME(childNode, "MapRestrict")) xml::copyToCString(mapRestrict, childNode);
        else if(NODE_NAME(childNode, "Unique")) unique.load(childNode);
        else if(NODE_NAME(childNode, "Flags")) loadBitset(childNode, flags);
        else if(NODE_NAME(childNode, "Wander")) wander.load(childNode);
        else if(NODE_NAME(childNode, "Coords")) {
            mNode = childNode->children;
            while(mNode) {
                if(NODE_NAME(mNode, "MapMarker")) {
                    mapmarker = new MapMarker;
                    mapmarker->load(mNode);

                    // load min/max while running through the coords
                    if(mapmarker->getX() > max.getX())
                        max.setX(mapmarker->getX());
                    if(mapmarker->getX() < min.getX())
                        min.setX(mapmarker->getX());

                    if(mapmarker->getY() > max.getY())
                        max.setY(mapmarker->getY());
                    if(mapmarker->getY() < min.getY())
                        min.setY(mapmarker->getY());

                    if(mapmarker->getZ() > max.getZ())
                        max.setZ(mapmarker->getZ());
                    if(mapmarker->getZ() < min.getZ())
                        min.setZ(mapmarker->getZ());

                    coords[i++] = mapmarker;
                }
                mNode = mNode->next;
            }
        }

        childNode = childNode->next;
    }
}

//*********************************************************************
//                      TileInfo
//*********************************************************************


void TileInfo::load(xmlNodePtr curNode) {
    xmlNodePtr  childNode = curNode->children, subNode;
    Season  s;
    char    temp[2];

    while(childNode) {
        if(NODE_NAME(childNode, "Name")) xml::copyToString(name, childNode);
        else if(NODE_NAME(childNode, "Fishing")) xml::copyToString(fishing, childNode);
        else if(NODE_NAME(childNode, "Description")) xml::copyToString(description, childNode);
        else if(NODE_NAME(childNode, "Id")) {
            xml::copyToCString(temp, childNode);
            id = temp[0];
        }
        else if(NODE_NAME(childNode, "Cost")) xml::copyToNum(cost, childNode);
        else if(NODE_NAME(childNode, "Vision")) xml::copyToNum(vision, childNode);
        else if(NODE_NAME(childNode, "TrackDur")) xml::copyToNum(trackDur, childNode);
        else if(NODE_NAME(childNode, "Wander")) wander.load(childNode);
        else if(NODE_NAME(childNode, "Style")) {
            xml::copyToCString(temp, childNode);
            style = temp[0];
        }
        else if(NODE_NAME(childNode, "Display")) {
            xml::copyToCString(temp, childNode);
            display = temp[0];
        }
        else if(NODE_NAME(childNode, "Flags")) loadBitset(childNode, flags);
        else if(NODE_NAME(childNode, "Fly")) xml::copyToNum(fly, childNode);
        else if(NODE_NAME(childNode, "Water")) water = true;
        else if(NODE_NAME(childNode, "Road")) road = true;
        else if(NODE_NAME(childNode, "Herbs")) {
            subNode = childNode->children;
            while(subNode) {
                if(NODE_NAME(subNode, "Herb")) {
                    CatRef cr;
                    cr.load(subNode);
                    herbs.push_back(cr);
                }
                subNode = subNode->next;
            }
        }
        else if(NODE_NAME(childNode, "Seasons")) {
            subNode = childNode->children;
            while(subNode) {
                if(NODE_NAME(subNode, "Season")) {
                    s = (Season)xml::getIntProp(subNode, "Id");
                    xml::copyToCString(temp, subNode);
                    season[s] = temp[0];
                }
                subNode = subNode->next;
            }
        }

        childNode = childNode->next;
    }
}

void TileInfo::save(xmlNodePtr curNode) const {
    xmlNodePtr childNode, subNode;

    xml::saveNonNullString(curNode, "Name", name);
    xml::saveNonNullString(curNode, "Description", description);
    xml::saveNonNullString(curNode, "Fishing", fishing);
    xml::saveNonNullString(curNode, "Id", std::to_string(id));

    xml::saveNonZeroNum(curNode, "Cost", cost);
    xml::saveNonZeroNum(curNode, "Vision", vision);
    xml::saveNonZeroNum(curNode, "TrackDur", trackDur);
    wander.save(curNode);

    xml::saveNonNullString(curNode, "Style", std::to_string(style));
    xml::saveNonNullString(curNode, "Display", std::to_string(display));

    saveBitset(curNode, "Flags", MAX_ROOM_FLAGS, flags);

    xml::saveNonZeroNum(curNode, "Fly", fly);
    xml::saveNonZeroNum(curNode, "Road", road);
    xml::saveNonZeroNum(curNode, "Water", water);

    std::list<CatRef>::const_iterator it;
    if(!herbs.empty()) {
        childNode = xml::newStringChild(curNode, "Herbs");
        for(it = herbs.begin() ; it != herbs.end() ; it++) {
            (*it).save(childNode, "Herb", true);
        }
    }

    std::map<Season,char>::const_iterator st;
    if(!season.empty()) {
        childNode = xml::newStringChild(curNode, "Seasons");
        for(st = season.begin() ; st != season.end() ; st++) {
            subNode = xml::newStringChild(childNode, "Season", std::to_string((*st).second));
            xml::newNumProp(subNode, "Id", (int)(*st).first);
        }
    }
}

//*********************************************************************
//                      getRoom
//*********************************************************************
// only checks to see if the room is in memory or on disk

AreaRoom *Area::getRoom(const MapMarker *mapmarker) {
    AreaRoom* room=nullptr;
    MapMarker m = *mapmarker;

    // this will modify the mapmarker, but if it's pointing to invalid rooms,
    // this is desired behavior
    checkCycle(&m);

    if(rooms.find(m.str()) != rooms.end())
        return(rooms[m.str()]);

    char            filename[256];
    sprintf(filename, "%s/%d/%s", Path::AreaRoom.c_str(), id, m.filename().c_str());

    if(fs::exists(filename)) {
        xmlDocPtr   xmlDoc;
        xmlNodePtr  rootNode;

        if((xmlDoc = xml::loadFile(filename, "AreaRoom")) == nullptr)
            merror("Unable to read arearoom file", FATAL);

        rootNode = xmlDocGetRootElement(xmlDoc);

        room = new AreaRoom(this);
        room->load(rootNode);

        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();

        return(room);
    }

    return(nullptr);
}


void Area::save(xmlNodePtr curNode, bool saveRooms) const {
    xmlNodePtr childNode, subNode;

    xml::newNumProp(curNode, "id", id);
    xml::saveNonNullString(curNode, "Name", name);
    xml::saveNonNullString(curNode, "DataFile", dataFile);

    xml::saveNonZeroNum(curNode, "Width", width);
    xml::saveNonZeroNum(curNode, "Height", height);
    xml::saveNonZeroNum(curNode, "Depth", depth);
    xml::saveNonZeroNum(curNode, "MinDepth", minDepth);

    xml::saveNonZeroNum(curNode, "CriticalX", critical_x);
    xml::saveNonZeroNum(curNode, "CriticalY", critical_y);
    xml::saveNonZeroNum(curNode, "CriticalZ", critical_z);

    xml::saveNonZeroNum(curNode, "FlightPower", flightPower);
    xml::saveNonNullString(curNode, "DefaultTerrain", &defaultTerrain);
    xml::saveNonNullString(curNode, "ErrorTerrain", &errorTerrain);

    std::list<AreaZone*>::const_iterator zIt;
    childNode = xml::newStringChild(curNode, "Zones");
    for(zIt = areaZones.begin() ; zIt != areaZones.end() ; zIt++) {
        subNode = xml::newStringChild(childNode, "Zone");
        (*zIt)->save(subNode);
    }


    std::map<char, TileInfo*>::const_iterator tIt;
    childNode = xml::newStringChild(curNode, "TerrainInfo");
    for(tIt = ter_tiles.begin() ; tIt != ter_tiles.end() ; tIt++) {
        subNode = xml::newStringChild(childNode, "Terrain");
        (*tIt).second->save(subNode);
    }

    childNode = xml::newStringChild(curNode, "MapInfo");
    for(tIt = map_tiles.begin() ; tIt != map_tiles.end() ; tIt++) {
        subNode = xml::newStringChild(childNode, "Map");
        (*tIt).second->save(subNode);
    }

    if(saveRooms) {
        std::map<std::string, AreaRoom*>::const_iterator rIt;
        for(rIt = rooms.begin() ; rIt != rooms.end() ; rIt++) {
            (*rIt).second->save();
        }
    }
}

//*********************************************************************
//                      load
//*********************************************************************

void Area::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    char    temp[2];

    id = xml::getIntProp(curNode, "id");

    while(childNode) {
        if(NODE_NAME(childNode, "Name")) xml::copyToString(name, childNode);
        else if(NODE_NAME(childNode, "DataFile")) xml::copyToCString(dataFile, childNode);

        else if(NODE_NAME(childNode, "Width")) xml::copyToNum(width, childNode);
        else if(NODE_NAME(childNode, "Height")) xml::copyToNum(height, childNode);
        else if(NODE_NAME(childNode, "Depth")) xml::copyToNum(depth, childNode);
        else if(NODE_NAME(childNode, "MinDepth")) xml::copyToNum(minDepth, childNode);

        else if(NODE_NAME(childNode, "Id")) xml::copyToNum(id, childNode);
        else if(NODE_NAME(childNode, "CriticalX")) xml::copyToNum(critical_x, childNode);
        else if(NODE_NAME(childNode, "CriticalY")) xml::copyToNum(critical_y, childNode);
        else if(NODE_NAME(childNode, "CriticalZ")) xml::copyToNum(critical_z, childNode);

        else if(NODE_NAME(childNode, "FlightPower")) xml::copyToNum(flightPower, childNode);

        else if(NODE_NAME(childNode, "DefaultTerrain")) {
            xml::copyToCString(temp, childNode);
            defaultTerrain = temp[0];
        }
        else if(NODE_NAME(childNode, "ErrorTerrain")) {
            xml::copyToCString(temp, childNode);
            errorTerrain = temp[0];
        }
        else if(NODE_NAME(childNode, "Zones")) loadZones(childNode);

        else if(NODE_NAME(childNode, "TerrainInfo")) loadTiles(childNode, true);
        else if(NODE_NAME(childNode, "MapInfo")) loadTiles(childNode, false);
        childNode = childNode->next;
    }

    loadTerrain(minDepth);
    loadRooms();
}

//*********************************************************************
//                      loadZones
//*********************************************************************

void Area::loadZones(xmlNodePtr curNode) {
    xmlNodePtr  childNode = curNode->children;
    AreaZone    *zone=nullptr;

    while(childNode) {
        if(NODE_NAME(childNode, "Zone")) {
            zone = new AreaZone();
            zone->load(childNode);
            areaZones.push_back(zone);
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      loadRooms
//*********************************************************************

void Area::loadRooms() {
    struct dirent *dirp=nullptr;
    DIR         *dir=nullptr;
    AreaRoom    *room=nullptr;
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char        filename[256];

    sprintf(filename, "%s/%d", Path::AreaRoom.c_str(), id);

    if((dir = opendir(filename)) == nullptr)
        return;

    while((dirp = readdir(dir)) != nullptr){
        // is this a player file?
        if(dirp->d_name[0] == '.')
            continue;

        sprintf(filename, "%s/%d/%s", Path::AreaRoom.c_str(), id, dirp->d_name);

        if((xmlDoc = xml::loadFile(filename, "AreaRoom")) == nullptr)
            continue;

        rootNode = xmlDocGetRootElement(xmlDoc);

        room = new AreaRoom(this);
        room->setVersion(xml::getProp(rootNode, "Version"));
        room->load(rootNode);

        xmlFreeDoc(xmlDoc);
    }

    xmlCleanupParser();
    closedir(dir);
}


//*********************************************************************
//                      loadTiles
//*********************************************************************

void Area::loadTiles(xmlNodePtr curNode, bool ter) {
    xmlNodePtr  childNode = curNode->children;
    TileInfo    *tile=nullptr;

    while(childNode) {
        tile = new TileInfo();
        tile->load(childNode);

        // TODO: Dom: why does this happen?
        if(tile->getName().empty())
            delete tile;
        else {
            if(ter)
                ter_tiles[tile->getId()] = tile;
            else
                map_tiles[tile->getId()] = tile;
        }
        childNode = childNode->next;
    }
}


//*********************************************************************
//                      areaInit
//*********************************************************************

void Server::areaInit(Creature* player, xmlNodePtr curNode) {
    MapMarker mapmarker;
    mapmarker.load(curNode);
    areaInit(player, mapmarker);
}



//*********************************************************************
//                      loadAreas
//*********************************************************************

bool Server::loadAreas() {
    char filename[80];
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr  curNode;
    Area    *area=nullptr;

    sprintf(filename, "%s/areas.xml", Path::AreaData.c_str());

    if(!fs::exists(filename))
        return(false);

    if((xmlDoc = xml::loadFile(filename, "Areas")) == nullptr)
        return(false);

    rootNode = xmlDocGetRootElement(xmlDoc);
    curNode = rootNode->children;

    clearAreas();
    while(curNode) {
        if(NODE_NAME(curNode, "Area")) {
            area = new Area;
            area->load(curNode);
            areas.push_back(area);
        }
        curNode = curNode->next;
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}


//*********************************************************************
//                      saveAreas
//*********************************************************************

void Server::saveAreas(bool saveRooms) const {
    std::list<Area*>::const_iterator it;
    xmlDocPtr   xmlDoc;
    xmlNodePtr      rootNode, curNode;
    char            filename[80];

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Areas", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    for(it = areas.begin() ; it != areas.end() ; it++) {
        curNode = xml::newStringChild(rootNode, "Area");
        (*it)->save(curNode, saveRooms);
    }

    sprintf(filename, "%s/areas2.xml", Path::Config.c_str());
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
}
