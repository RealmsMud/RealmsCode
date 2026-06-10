/*
 * zoneIndexBuilder.cpp
 *   Incremental, time-budgeted per-zone index builder.
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

#include "zoneIndexBuilder.hpp"

#include <cstdlib>
#include <filesystem>
#include <memory>

#include "apiQuests.hpp"             // JSON quest store accessors (REST quest index)
#include "catRef.hpp"
#include "paths.hpp"
#include "xml.hpp"                   // xml::readRootChildText (streaming name-only parse)

namespace fs = std::filesystem;

static fs::path entityDir(ZoneIndex::Type type, const std::string& zone) {
    switch(type) {
        case ZoneIndex::Type::Room:    return Path::UniqueRoom / zone;
        case ZoneIndex::Type::Object:  return Path::Object / zone;
        case ZoneIndex::Type::Monster: return Path::Monster / zone;
        case ZoneIndex::Type::Quest:   return {};   // quests live in gConfig, not a category dir
    }
    return {};
}

// [orm]NNNNN.xml -> id N; quests come from the in-memory store, not files.
static std::vector<int> realScan(ZoneIndex::Type type, const std::string& zone) {
    std::vector<int> ids;
    if(type == ZoneIndex::Type::Quest)
        return apiScanQuestIds(zone);
    fs::path dir = entityDir(type, zone);
    std::error_code ec;
    if(!fs::is_directory(dir, ec))
        return ids;
    for(const auto& e : fs::directory_iterator(dir)) {
        if(e.path().extension() != ".xml")
            continue;
        std::string stem = e.path().stem().string();
        if(stem.size() < 2)
            continue;
        int id = std::atoi(stem.c_str() + 1);
        if(id > 0)
            ids.push_back(id);
    }
    return ids;
}

// name-only read
static std::string realLoad(ZoneIndex::Type type, const std::string& zone, int id) {
    CatRef cr(zone, (short)id);
    if(type == ZoneIndex::Type::Quest) {
        auto j = apiReadQuestJson(zone, id);
        return j ? j->value("name", std::string()) : std::string();
    }
    switch(type) {
        case ZoneIndex::Type::Object:  return xml::readRootChildText(Path::objectPath(cr),  "Object",   "Name");
        case ZoneIndex::Type::Monster: return xml::readRootChildText(Path::monsterPath(cr), "Creature", "Name");
        case ZoneIndex::Type::Room:    return xml::readRootChildText(Path::roomPath(cr),    "Room",     "Name");
        case ZoneIndex::Type::Quest:   return "";
    }
    return "";
}

ZoneIndexBuilder::ZoneIndexBuilder(ZoneIndex& index, Loader loader, Scanner scanner)
    : index(index),
      loader(loader ? std::move(loader) : Loader(realLoad)),
      scanner(scanner ? std::move(scanner) : Scanner(realScan)) {}

void ZoneIndexBuilder::request(ZoneIndex::Type type, const std::string& zone) {
    if(index.isBuilt(type, zone)) return;
    if(index.loadFile(type, zone)) return;

    Key key{type, zone};
    {
        std::lock_guard<std::mutex> lk(mtx);
        if(enqueued.count(key))
            return;
        enqueued.insert(key);
    }

    std::vector<int> ids;
    try {
        ids = scanner(type, zone);
    } catch(...) {
        std::lock_guard<std::mutex> lk(mtx);
        enqueued.erase(key);
        return;
    }

    std::lock_guard<std::mutex> lk(mtx);
    if(ids.empty()) {
        enqueued.erase(key);
        index.markBuilt(type, zone);
        return;
    }
    queue.push_back(Build{type, zone, std::move(ids), 0});
}

void ZoneIndexBuilder::pump(std::chrono::microseconds budget, std::size_t minCount) {
    std::lock_guard<std::mutex> lk(mtx);
    if(queue.empty())
        return;

    Build& b = queue.front();
    auto startT = std::chrono::steady_clock::now();
    std::size_t loaded = 0;
    while(b.pos < b.remaining.size()) {
        int id = b.remaining[b.pos++];
        try {
            index.upsert(b.type, b.zone, id, loader(b.type, b.zone, id));
        } catch(...) {
            // one unreadable entity must not abort the build or stall the tick
        }
        if(++loaded >= minCount && (std::chrono::steady_clock::now() - startT) >= budget)
            break;
    }

    if(b.pos >= b.remaining.size()) {
        ZoneIndex::Type t = b.type;
        std::string z = b.zone;
        enqueued.erase(Key{t, z});
        queue.pop_front();
        index.markBuilt(t, z);
    }
}

bool ZoneIndexBuilder::hasWork() const {
    std::lock_guard<std::mutex> lk(mtx);
    return !queue.empty();
}
