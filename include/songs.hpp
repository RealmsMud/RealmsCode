/*
 * Songs.h
 *   Additional song-casting routines.
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

#ifndef _SONGS_H
#define _SONGS_H

#include <list>
#include <libxml/parser.h>  // for xmlNodePtr

#include "global.hpp"
#include "structs.hpp"

class MudObject;

// ******************
//   Song
// ******************

class Song: public MysticMethod {
public:
    explicit Song(xmlNodePtr rootNode);
    Song(std::string_view pCmdStr) {
        name = pCmdStr;
    }
    ~Song() {};

    void save(xmlNodePtr rootNode) const override;

    [[nodiscard]] const bstring& getEffect() const;
    [[nodiscard]] const bstring& getType() const;
    [[nodiscard]] const bstring& getTargetType() const;
    bool runScript(MudObject* singer, MudObject* target = nullptr) const;

    [[nodiscard]] int getDelay() const;
    [[nodiscard]] int getDuration() const;
private:
    Song() {};
    bstring effect;
    bstring type; // script, effect, etc
    bstring targetType; // Valid Targets: Room, Self, Group, Target, RoomBene, RoomAggro

    int delay{};
    int duration{};

};

#endif  /* _SONGS_H */

