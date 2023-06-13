/*
 * Zone.h
 *   Code for zones .
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

#include <iostream>
#include <fstream>
#include <string>

#include "catRef.hpp"   // for CatRef
#include "config.hpp"
#include "json.hpp"
#include "paths.hpp"
#include "quests.hpp"   // for QuestInfo
#include "zone.hpp"

using json = nlohmann::json;

Zone::Zone() {
//    std::clog << "Zone ctor" << std::endl;
}

Zone::~Zone() {
//    std::clog << "~Zone" << std::endl;
}


bool Config::loadZones() {
    auto zoneIndex = Path::Zone / "zones.json";
    std::ifstream ifs(zoneIndex);
    json j = json::parse(ifs);

    for (const auto& [key, zoneJson] : j.items()) {
        zones.emplace(key, zoneJson);
    }

    for (const auto& [name, zone] : zones) {
        std::clog << "Zone: <" << name << ">: " << zone << std::endl;
    }

    json blah = zones;
    std::clog << blah["hp"] << std::endl;
    return true;
}

std::ostream& operator<<(std::ostream& strm, const Zone& zone) {
    return strm << "Zone<" << zone.name << ", " << zone.display << ">";
}