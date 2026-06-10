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

#include <fstream>
#include <string>
#include "json.hpp"
#include "paths.hpp"      // Path::Zone (REST quest index)
#include "catRef.hpp"     // must precede zone.hpp: std::less<CatRef> is specialized here
#include "zone.hpp"


void to_json(nlohmann::json &j, const Zone &zone) {
    j = json{
        {"name", zone.name},
        {"display", zone.display},
        {"flags", zone.flags},
    };

    // quest list comes from the per-zone JSON index (REST store), not the in-game XML quests
    json q = json::array();
    std::ifstream idx(Path::Zone / zone.name / "quests.index.json");
    if(idx) {
        try {
            for(const auto& e : nlohmann::json::parse(idx))
                q.push_back({ {"area", zone.name}, {"id", e.value("id", 0)}, {"name", e.value("name", std::string())} });
        } catch(...) {}
    }
    j["quests"] = q;
}

void from_json(const nlohmann::json &j, Zone &zone) {
    zone.name = j.at("name").get<std::string>();
    zone.display = j.at("display").get<std::string>();
    if (j.contains("flags")) zone.flags = j.at("flags");
}


