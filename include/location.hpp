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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

#include <libxml/parser.h>
#include <string>

#include "area.hpp"
#include "catRef.hpp"

class Player;
class BaseRoom;

class Location {
public:
    Location();
    void save(xmlNodePtr rootNode, const std::string &name) const;
    void load(xmlNodePtr curNode);
    [[nodiscard]] std::string str() const;
    bool    operator==(const Location& l) const;
    bool    operator!=(const Location& l) const;
    std::shared_ptr<BaseRoom> loadRoom(const std::shared_ptr<Player>& player=nullptr) const;
    [[nodiscard]] short getId() const;

    CatRef room;
    MapMarker mapmarker;
};



