/*
 * apiQuests.cpp
 *   REST-side accessors for the on-disk per-zone JSON quest store
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

#include "apiQuests.hpp"

#include <cstdlib>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

std::filesystem::path apiQuestJsonPath(const std::string& zone, int id, const fs::path& base) {
    return base / zone / "quests" / (std::to_string(id) + ".json");
}

std::vector<int> apiScanQuestIds(const std::string& zone, const fs::path& base) {
    std::vector<int> ids;
    fs::path dir = base / zone / "quests";
    std::error_code ec;
    if(!fs::is_directory(dir, ec))
        return ids;
    for(const auto& e : fs::directory_iterator(dir)) {
        if(e.path().extension() != ".json")
            continue;
        int id = std::atoi(e.path().stem().string().c_str());
        if(id > 0)
            ids.push_back(id);
    }
    return ids;
}

std::optional<nlohmann::json> apiReadQuestJson(const std::string& zone, int id, const fs::path& base) {
    std::ifstream ifs(apiQuestJsonPath(zone, id, base));
    if(!ifs)
        return std::nullopt;
    try {
        return nlohmann::json::parse(ifs);
    } catch(...) {
        return std::nullopt;
    }
}
