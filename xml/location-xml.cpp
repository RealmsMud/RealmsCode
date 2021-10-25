/*
 * location-xml.cpp
 *   Location xml
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

#include "area.hpp"         // for MapMarker, Area
#include "catRef.hpp"       // for CatRef
#include "creatures.hpp"    // for Player
#include "location.hpp"     // for Location
#include "rooms.hpp"        // for AreaRoom, BaseRoom (ptr only), UniqueRoom
#include "server.hpp"       // for Server, gServer
#include "xml.hpp"          // for newStringChild, loadRoom, NODE_NAME

void Location::save(xmlNodePtr rootNode, const std::string &name) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, name);
    room.save(curNode, "Room", false);
    if(mapmarker.getArea()) {
        xmlNodePtr childNode = xml::newStringChild(curNode, "MapMarker");
        mapmarker.save(childNode);
    }
}

void Location::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Room")) room.load(childNode);
        else if(NODE_NAME(childNode, "MapMarker")) mapmarker.load(childNode);
        childNode = childNode->next;
    }
}

BaseRoom* Location::loadRoom(Player* player) const {
    UniqueRoom* uRoom=nullptr;
    if(room.id && ::loadRoom(room, &uRoom))
        return(uRoom);

    Area* area = gServer->getArea(mapmarker.getArea());
    if(area) {
        AreaRoom* aRoom = area->loadRoom(player, &mapmarker, false);
        return(aRoom);
    }

    return(nullptr);
}
