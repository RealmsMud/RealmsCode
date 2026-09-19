/*
 * apiLoginThrottle.hpp
 *   Per-peer failed-login counter and lockout for the REST API.
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
#include <mutex>
#include <string>
#include <unordered_map>

// One instance shared by every credential route, keyed on the client address, so
// hopping between routes does not multiply the budget. In-memory: a restart forgives.
class ApiLoginThrottle {
public:
    struct Settings {
        int maxFailures = 10;                   // <= 0 disables the throttle
        std::chrono::seconds window{300};       // failures older than this no longer count
        std::chrono::seconds lockout{600};
        std::size_t maxEntries = 10000;
    };

    ApiLoginThrottle() = default;
    explicit ApiLoginThrottle(Settings pSettings) : cfg(pSettings) {}

    // Seconds remaining (rounded up), 0 when allowed. Drops an entry whose lockout expired.
    [[nodiscard]] int lockedFor(const std::string& key);
    void recordFailure(const std::string& key);
    void recordSuccess(const std::string& key);
    void clear();
    [[nodiscard]] std::size_t size() const;

private:
    struct Entry {
        int failures = 0;
        std::chrono::steady_clock::time_point windowStart;
        std::chrono::steady_clock::time_point lockedUntil;
    };

    void evict(std::chrono::steady_clock::time_point now);   // caller holds mtx

    mutable std::mutex mtx;   // every public call mutates, so a shared_mutex would not pay
    std::unordered_map<std::string, Entry> entries;
    Settings cfg;
};
