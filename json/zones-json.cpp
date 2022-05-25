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

namespace boost {
    void to_json(nlohmann::json& j, const boost::dynamic_bitset<>& b) {
        std::string str;
        to_string(b, str);
        j = str;
    }

    void from_json(const nlohmann::json& j, boost::dynamic_bitset<>& b) {
        b = boost::dynamic_bitset<>(j.get<std::string>());
    }
}

void to_json(nlohmann::json &j, const Zone &zone) {
    j["name"] = zone.name;
    j["display"] = zone.display;

    j["flags"] = zone.flags;
}

void from_json(const nlohmann::json &j, Zone &zone) {
    zone.name = j.at("name").get<std::string>();
    zone.display = j.at("display").get<std::string>();
    if(j.contains("flags")) zone.flags = j.at("flags");
}


