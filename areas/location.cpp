/*
 * location.cpp
 *   Location file
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

#include <libxml/parser.h>  // for xmlNodePtr
#include <string>           // for string

#include "area.hpp"         // for MapMarker
#include "catRef.hpp"       // for CatRef
#include "location.hpp"     // for Location

class BaseRoom;
class Player;

//*********************************************************************
//                      Location
//*********************************************************************

Location::Location() = default;

std::string Location::str() const {
    if(room.id)
        return(room.str());
    return(mapmarker.str());
}

bool Location::operator==(const Location& l) const {
    return( (   (room.id == 0 && l.room.id == 0) || // if rooms are empty, don't care about area
                (room == l.room)                    // same room
            ) &&
            mapmarker == l.mapmarker
    );
}
bool Location::operator!=(const Location& l) const {
    return(!(*this == l));
}

short Location::getId() const {
    if(room.id)
        return(room.id);
    return(mapmarker.getArea());
}

