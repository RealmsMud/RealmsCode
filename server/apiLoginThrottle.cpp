/*
 * apiLoginThrottle.cpp
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

#include "apiLoginThrottle.hpp"

namespace {
    using Clock = std::chrono::steady_clock;

    constexpr Clock::time_point kNeverLocked{};
}

int ApiLoginThrottle::lockedFor(const std::string& key) {
    if(cfg.maxFailures <= 0)
        return 0;

    const std::lock_guard<std::mutex> guard(mtx);
    const auto it = entries.find(key);
    if(it == entries.end())
        return 0;

    const auto now = Clock::now();
    if(it->second.lockedUntil > now)
        return static_cast<int>(std::chrono::ceil<std::chrono::seconds>(it->second.lockedUntil - now).count());
    if(it->second.lockedUntil != kNeverLocked)
        entries.erase(it);
    return 0;
}

void ApiLoginThrottle::recordFailure(const std::string& key) {
    if(cfg.maxFailures <= 0)
        return;

    const std::lock_guard<std::mutex> guard(mtx);
    const auto now = Clock::now();

    auto it = entries.find(key);
    if(it == entries.end()) {
        if(entries.size() >= cfg.maxEntries)
            evict(now);
        it = entries.emplace(key, Entry{0, now, kNeverLocked}).first;
    }

    Entry& entry = it->second;
    if(now - entry.windowStart > cfg.window) {
        entry.failures = 0;
        entry.windowStart = now;
        entry.lockedUntil = kNeverLocked;
    }
    if(++entry.failures >= cfg.maxFailures)
        entry.lockedUntil = now + cfg.lockout;
}

void ApiLoginThrottle::recordSuccess(const std::string& key) {
    const std::lock_guard<std::mutex> guard(mtx);
    entries.erase(key);
}

void ApiLoginThrottle::clear() {
    const std::lock_guard<std::mutex> guard(mtx);
    entries.clear();
}

std::size_t ApiLoginThrottle::size() const {
    const std::lock_guard<std::mutex> guard(mtx);
    return entries.size();
}

void ApiLoginThrottle::evict(Clock::time_point now) {
    for(auto it = entries.begin(); it != entries.end(); ) {
        const bool unlocked = it->second.lockedUntil == kNeverLocked || it->second.lockedUntil <= now;
        if(unlocked && now - it->second.windowStart > cfg.window)
            it = entries.erase(it);
        else
            ++it;
    }
    if(entries.empty() || entries.size() < cfg.maxEntries)
        return;

    // Still full of live entries: the oldest window is the least useful one to keep.
    auto oldest = entries.begin();
    for(auto it = entries.begin(); it != entries.end(); ++it)
        if(it->second.windowStart < oldest->second.windowStart)
            oldest = it;
    entries.erase(oldest);
}
