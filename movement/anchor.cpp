/*
 * anchor.cpp
 *   Magical anchors
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <libxml/parser.h>  // for xmlNodePtr, xmlNode

#include "anchor.hpp"       // for Anchor
#include "area.hpp"         // for MapMarker, Area, TileInfo
#include "bstring.hpp"      // for bstring
#include "catRef.hpp"       // for CatRef
#include "creatures.hpp"    // for Player
#include "rooms.hpp"        // for AreaRoom, BaseRoom, UniqueRoom
#include "xml.hpp"          // for newStringChild, copyToBString, NODE_NAME

//*********************************************************************
//                      Anchor
//*********************************************************************

Anchor::Anchor() {
    reset();
}

Anchor::Anchor(std::string_view a, const Player* player) {
    reset();
    alias = a;
    bind(player);
}

Anchor::~Anchor() {
        delete mapmarker;
}

//*********************************************************************
//                      getRoom
//*********************************************************************

CatRef Anchor::getRoom() const { return(room); }

//*********************************************************************
//                      setRoom
//*********************************************************************

void Anchor::setRoom(const CatRef& r) { room = r; }

//*********************************************************************
//                      getMapMarker
//*********************************************************************

const MapMarker* Anchor::getMapMarker() const { return(mapmarker); }

//*********************************************************************
//                      getAlias
//*********************************************************************

bstring Anchor::getAlias() const { return(alias); }

//*********************************************************************
//                      getRoomName
//*********************************************************************

bstring Anchor::getRoomName() const { return(roomName); }

//*********************************************************************
//                      reset
//*********************************************************************

void Anchor::reset() {
    alias = roomName = "";
    mapmarker = nullptr;
}

//*********************************************************************
//                      bind
//*********************************************************************

void Anchor::bind(const Player* player) {

    if(player->inUniqueRoom())
        bind(player->getConstUniqueRoomParent());
    else
        bind(player->getConstAreaRoomParent());
}
void Anchor::bind(const UniqueRoom* uRoom) {
    roomName = uRoom->getName();
    room = uRoom->info;
}
void Anchor::bind(const AreaRoom* aRoom) {
    roomName = aRoom->area->getTile(
            aRoom->area->getTerrain(nullptr, &aRoom->mapmarker, 0, 0, 0, true), false
        )->getName().c_str();
        delete mapmarker;
    mapmarker = new MapMarker;
    *mapmarker = *&aRoom->mapmarker;
}

//*********************************************************************
//                      is
//*********************************************************************

bool Anchor::is(const BaseRoom* room) const {
    const UniqueRoom* uRoom = room->getAsConstUniqueRoom();
    if(uRoom)
        return(is(uRoom));
    else
        return(is(room->getAsConstAreaRoom()));
}
bool Anchor::is(const Player* player) const {
    if(player->inUniqueRoom())
        return(is(player->getConstUniqueRoomParent()));
    else
        return(is(player->getConstAreaRoomParent()));
}
bool Anchor::is(const UniqueRoom* uRoom) const {
    return(room.id && room == uRoom->info);
}
bool Anchor::is(const AreaRoom* aRoom) const {
    return(mapmarker && *mapmarker == *&aRoom->mapmarker);
}

//*********************************************************************
//                      operator=
//*********************************************************************

Anchor& Anchor::operator=(const Anchor& a) {
    if (this == &a)
        return(*this);

    alias = a.alias;
    roomName = a.roomName;
    room = a.room;
    if(a.mapmarker) {
        delete mapmarker;
        mapmarker = new MapMarker;
        *mapmarker = *a.mapmarker;
    }
    return(*this);
}

//*********************************************************************
//                      load
//*********************************************************************

void Anchor::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Alias")) xml::copyToBString(alias, childNode);
        else if(NODE_NAME(childNode, "RoomName")) xml::copyToBString(roomName, childNode);
        else if(NODE_NAME(childNode, "Room")) room.load(childNode);
        else if(NODE_NAME(childNode, "MapMarker")) {
            mapmarker = new MapMarker;
            mapmarker->load(childNode);
        }

        childNode = childNode->next;
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void Anchor::save(xmlNodePtr curNode) const {
    xml::newStringChild(curNode, "Alias", alias);
    xml::newStringChild(curNode, "RoomName", roomName);
    room.save(curNode, "Room", false);
    if(mapmarker) {
        xmlNodePtr childNode = xml::newStringChild(curNode, "MapMarker");
        mapmarker->save(childNode);
    }
}
