/*
 * zoneIndex.cpp
 *   Implementation of the per-zone entity summary index.
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

#include "zoneIndex.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <utility>

#include "json.hpp"
#include "paths.hpp"

using json = nlohmann::json;

const char* ZoneIndex::typeName(Type t) {
    switch(t) {
        case Type::Room:    return "rooms";
        case Type::Object:  return "objects";
        case Type::Monster: return "monsters";
        case Type::Quest:   return "quests";
    }
    return "unknown";
}

static std::filesystem::path indexFilePath(ZoneIndex::Type type, const std::string& zone) {
    return Path::Zone / zone / (std::string(ZoneIndex::typeName(type)) + ".index.json");
}

void ZoneIndex::upsert(Type type, const std::string& zone, int id, const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(mtx);
    Key key{type, zone};
    entries[key][id] = ZoneSummary{id, name};
    dirty.insert(key);
    // does not mark built: a single save is not a full enumeration
}

void ZoneIndex::remove(Type type, const std::string& zone, int id) {
    std::unique_lock<std::shared_mutex> lock(mtx);
    Key key{type, zone};
    auto it = entries.find(key);
    if(it == entries.end())
        return;
    it->second.erase(id);
    dirty.insert(key);
}

bool ZoneIndex::isBuilt(Type type, const std::string& zone) const {
    std::shared_lock<std::shared_mutex> lock(mtx);
    return built.count(Key{type, zone}) != 0;
}

bool ZoneIndex::list(Type type, const std::string& zone, std::vector<ZoneSummary>& out) const {
    std::shared_lock<std::shared_mutex> lock(mtx);
    Key key{type, zone};
    if(built.count(key) == 0)
        return false;
    auto it = entries.find(key);
    if(it != entries.end())
        for(const auto& [id, s] : it->second)
            out.push_back(s);
    return true;
}

void ZoneIndex::markBuilt(Type type, const std::string& zone) {
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        built.insert(Key{type, zone});
    }
    buildCv.notify_all();
}

bool ZoneIndex::waitUntilBuilt(Type type, const std::string& zone, std::chrono::milliseconds timeout) {
    Key key{type, zone};
    std::unique_lock<std::shared_mutex> lock(mtx);   // exclusive; waiters are rare
    return buildCv.wait_for(lock, timeout, [&] { return built.count(key) != 0; });
}

bool ZoneIndex::loadFile(Type type, const std::string& zone) {
    std::filesystem::path f = indexFilePath(type, zone);
    std::error_code ec;
    if(!std::filesystem::exists(f, ec))
        return false;
    try {
        std::ifstream ifs(f);
        json j = json::parse(ifs);
        {
            std::unique_lock<std::shared_mutex> lock(mtx);
            Key key{type, zone};
            auto& m = entries[key];
            for(const auto& e : j) {
                int id = e.at("id").get<int>();
                m[id] = ZoneSummary{id, e.value("name", std::string())};
            }
            built.insert(key);
        }
        buildCv.notify_all();
    } catch(...) {
        return false;
    }
    return true;
}

void ZoneIndex::flushDirty() {
    std::vector<std::pair<Key, std::vector<ZoneSummary>>> toWrite;
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        if(dirty.empty())
            return;
        for(const auto& key : dirty) {
            std::vector<ZoneSummary> v;
            auto it = entries.find(key);
            if(it != entries.end())
                for(const auto& [id, s] : it->second)
                    v.push_back(s);
            toWrite.emplace_back(key, std::move(v));
        }
        dirty.clear();
    }

    for(const auto& [key, v] : toWrite) {
        std::filesystem::path f = indexFilePath(key.type, key.zone);
        std::error_code ec;
        std::filesystem::create_directories(f.parent_path(), ec);
        json j = json::array();
        for(const auto& s : v)
            j.push_back({{"id", s.id}, {"name", s.name}});
        std::ofstream out(f);
        if(out)
            out << std::setw(2) << j << std::endl;
    }
}
