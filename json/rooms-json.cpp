/*
 * Rooms-json.cpp
 *   UniqueRoom (+ Exit/Wander/Track) JSON for the REST API read path.
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

#include <string>

#include "json.hpp"
#include "catRef.hpp"
#include "lasttime.hpp"
#include "location.hpp"
#include "mudObjects/mudObject.hpp"
#include "mudObjects/rooms.hpp"
#include "mudObjects/uniqueRooms.hpp"
#include "mudObjects/exits.hpp"
#include "track.hpp"
#include "wanderInfo.hpp"

using json = nlohmann::json;

void to_json(nlohmann::json &j, const WanderInfo &wander) {
    j = json{
        {"traffic", wander.traffic},
        {"random", wander.random},          // map<int, CatRef>
    };
}

void to_json(nlohmann::json &j, const Track &track) {
    j = json{
        {"num", track.num},
        {"size", static_cast<int>(track.size)},
        {"direction", track.direction},
    };
}

void to_json(nlohmann::json &j, const Location &loc) {
    j = json{
        {"room", loc.room},
        {"mapmarker", loc.mapmarker},
    };
}

void to_json(nlohmann::json &j, const Exit &exit) {
    to_json(j, exit, LoadType::LS_FULL);
}

void to_json(nlohmann::json &j, const Exit &exit, LoadType mode) {
    to_json(j, static_cast<const MudObject&>(exit), false);     // name (+ hooks/effects)

    json keys = json::array();
    for(int i = 0; i < 3; i++)
        keys.push_back(std::string(exit.desc_key[i]));

    j.update({
        {"keys", keys},
        {"target", exit.target},        // full Location: room CatRef + overland mapmarker
        {"toll", exit.toll},
        {"level", exit.level},
        {"trap", exit.trap},
        {"key", exit.key},
        {"keyArea", exit.keyArea},
        {"size", static_cast<int>(exit.size)},
        {"direction", static_cast<int>(exit.direction)},
        {"passPhrase", exit.passphrase},
        {"passLang", exit.passlang},
        {"description", exit.description},
        {"enter", exit.enter},
        {"open", exit.open},
        {"flags", exit.flags},
    });

    if(mode == LoadType::LS_FULL) {
        j["ltime"] = exit.ltime;
        j["usedBy"] = exit.usedBy;
    }
}

void to_json(nlohmann::json &j, const UniqueRoom &room) {
    to_json(j, room, LoadType::LS_FULL);
}

void to_json(nlohmann::json &j, const UniqueRoom &room, LoadType mode) {
    const bool full = (mode == LoadType::LS_FULL);
    to_json(j, static_cast<const MudObject&>(room), true);      // name, id, hooks, effects

    j.update({
        {"info", room.info},
        {"shortDescription", room.short_desc},
        {"longDescription", room.long_desc},
        {"fishing", room.fishing},
        {"faction", room.faction},
        {"lastModBy", std::string(room.last_mod)},
        {"lastModTime", std::string(room.lastModTime)},
        {"version", room.getVersion()},
        {"lowLevel", room.lowLevel},
        {"highLevel", room.highLevel},
        {"maxMobs", room.maxmobs},
        {"trap", room.trap},
        {"trapExit", room.trapexit},
        {"trapWeight", room.trapweight},
        {"trapStrength", room.trapstrength},
        {"roomExp", room.roomExp},
        {"size", static_cast<int>(room.getSize())},
        {"flags", room.flags},
        {"wander", room.wander},
        {"track", room.track},
        {"permMonsters", room.permMonsters},    // map<int, CRLastTime>
        {"permObjects", room.permObjects},
    });

    json exits = json::array();
    for(const auto& ex : room.exits)
        if(ex) {
            json je;
            to_json(je, *ex, mode);
            exits.push_back(je);
        }
    j["exits"] = exits;

    if(full) {
        json lts = json::array();
        for(int i = 0; i < 16; i++) lts.push_back(room.lasttime[i]);
        j["lasttime"] = lts;
        j["beenHere"] = room.beenhere;
        j["lastPly"] = std::string(room.lastPly);
        j["lastPlyTime"] = std::string(room.lastPlyTime);
    }
}
