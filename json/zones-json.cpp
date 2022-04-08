/*
 * Zones-json.cpp
 *   Zone json
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later

 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <string>
#include <nlohmann/json.hpp>

#include "zone.hpp"

void to_json(nlohmann::json &j, const Zone &zone) {
    j["name"] = zone.name;
    j["display"] = zone.display;

    std::string flags;
    to_string(zone.flags, flags);
    j["flags"] = flags;
}

void from_json(const nlohmann::json &j, Zone &zone) {
    zone.name = j.at("name").get<std::string>();
    zone.display = j.at("display").get<std::string>();
    zone.flags = boost::dynamic_bitset<>(j.at("flags").get<std::string>());
}


