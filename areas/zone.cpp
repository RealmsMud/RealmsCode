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

#include <filesystem>
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
    namespace fs = std::filesystem;
    fs::path zoneRoot = Path::Zone;
    zones.clear();

    std::error_code ec;
    if(!fs::is_directory(zoneRoot, ec))
        return true;

    for(const auto& entry : fs::directory_iterator(zoneRoot)) {
        if(!entry.is_directory())
            continue;
        fs::path zfile = entry.path() / "zone.json";
        if(!fs::exists(zfile))
            continue;
        try {
            std::ifstream ifs(zfile);
            json j = json::parse(ifs);
            Zone zone;
            from_json(j, zone);
            zones.emplace(entry.path().filename().string(), std::move(zone));
        } catch(const std::exception& e) {
            std::clog << "Zone load failed for " << zfile << ": " << e.what() << std::endl;
        }
    }
    return true;
}

std::ostream& operator<<(std::ostream& strm, const Zone& zone) {
    return strm << "Zone<" << zone.name << ", " << zone.display << ">";
}