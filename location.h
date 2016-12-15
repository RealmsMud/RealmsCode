/*
 * location.h
 *   Location file
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___z
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _LOCATION_H
#define _LOCATION_H

#include "common.h"
#include "area.h"

class Player;
class BaseRoom;

class Location {
public:
    Location();
    void save(xmlNodePtr rootNode, bstring name) const;
    void load(xmlNodePtr curNode);
    bstring str() const;
    bool    operator==(const Location& l) const;
    bool    operator!=(const Location& l) const;
    BaseRoom* loadRoom(Player* player=0) const;
    short getId() const;

    CatRef room;
    MapMarker mapmarker;
};


#endif  /* _LOCATION_H */

