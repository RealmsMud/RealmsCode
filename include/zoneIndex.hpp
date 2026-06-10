/*
 * zoneIndex.hpp
 *   Thread-safe per-zone index of entity summaries (id + name) for REST API lists.
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

#pragma once

#include <chrono>
#include <condition_variable>
#include <map>
#include <set>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <vector>

struct ZoneSummary {
    int id = 0;
    std::string name;
};

class ZoneIndex {
public:
    enum class Type { Room, Object, Monster, Quest };
    static const char* typeName(Type t);

    void upsert(Type type, const std::string& zone, int id, const std::string& name);
    void remove(Type type, const std::string& zone, int id);

    [[nodiscard]] bool isBuilt(Type type, const std::string& zone) const;
    [[nodiscard]] bool list(Type type, const std::string& zone, std::vector<ZoneSummary>& out) const;
    void markBuilt(Type type, const std::string& zone);
    bool waitUntilBuilt(Type type, const std::string& zone, std::chrono::milliseconds timeout);
    bool loadFile(Type type, const std::string& zone);
    void flushDirty();

private:
    struct Key {
        Type type;
        std::string zone;
        bool operator<(const Key& o) const { return std::tie(type, zone) < std::tie(o.type, o.zone); }
    };

    mutable std::shared_mutex mtx;
    mutable std::condition_variable_any buildCv;   // signaled by markBuilt
    std::map<Key, std::map<int, ZoneSummary>> entries;
    std::set<Key> built;
    std::set<Key> dirty;
};
