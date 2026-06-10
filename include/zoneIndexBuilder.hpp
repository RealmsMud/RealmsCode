/*
 * zoneIndexBuilder.hpp
 *   Incremental, time-budgeted per-zone index builder, pumped on the game loop.
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
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "zoneIndex.hpp"

class ZoneIndexBuilder {
public:
    using Loader  = std::function<std::string(ZoneIndex::Type, const std::string&, int)>;
    using Scanner = std::function<std::vector<int>(ZoneIndex::Type, const std::string&)>;

    explicit ZoneIndexBuilder(ZoneIndex& index, Loader loader = {}, Scanner scanner = {});

    void request(ZoneIndex::Type type, const std::string& zone);
    void pump(std::chrono::microseconds budget, std::size_t minCount);

    [[nodiscard]] bool hasWork() const;

private:
    struct Build {
        ZoneIndex::Type type;
        std::string zone;
        std::vector<int> remaining;
        std::size_t pos = 0;
    };
    struct Key {
        ZoneIndex::Type type;
        std::string zone;
        bool operator<(const Key& o) const { return std::tie(type, zone) < std::tie(o.type, o.zone); }
    };

    ZoneIndex& index;
    Loader loader;
    Scanner scanner;
    mutable std::mutex mtx;
    std::deque<Build> queue;
    std::set<Key> enqueued;
};
