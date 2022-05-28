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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _SONGS_H
#define _SONGS_H

#include <set>
#include <libxml/parser.h>  // for xmlNodePtr

#include "global.hpp"
#include "structs.hpp"

class MudObject;
class SongBuilder;

// ******************
//   Song
// ******************

class Song: public MysticMethod {
public:
    static inline const std::set<std::string> validTypes = {"effect", "script"};
    static inline const std::set<std::string> validTargetTypes = {"room", "self", "group", "target", "room-beneficial", "room-aggro"};

public:
    friend class SongBuilder;  // The builder can access the internals
    Song(std::string_view pCmdStr) {
        name = pCmdStr;
    }
    ~Song() = default;

    bool runScript(std::shared_ptr<MudObject> singer, std::shared_ptr<MudObject> target = nullptr) const;

    [[nodiscard]] const std::string& getEffect() const;
    [[nodiscard]] const std::string& getType() const;
    [[nodiscard]] const std::string& getTargetType() const;

    [[nodiscard]] int getDelay() const;
    [[nodiscard]] int getDuration() const;
    [[nodiscard]] int getPriority() const;

    Song(const Song&) = delete; // No Copies
    Song(Song&&) = default;     // Only Moves
private:
    Song() = default;;
    std::string effect;

    // TODO: These should be classes that have an interface for being used/called
    std::string type; // script, effect, etc
    std::string targetType; // Valid Targets: Room, Self, Group, Target, RoomBene, RoomAggro

    int delay{};
    int duration{};

};

#endif  /* _SONGS_H */

