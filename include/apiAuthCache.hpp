/*
 * apiAuthCache.hpp
 *   Caches the authorization-relevant snapshot of a staff Player.
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
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include <memory>

#include "global.hpp"   // CreatureClass, MAX_BUILDER_RANGE
#include "range.hpp"    // Range

class CatRef;
class Player;

// Thread-safe authz snapshot of a staff Player for use off the game thread.
struct AuthContext {
    std::string id;
    std::string name;
    CreatureClass cClass = CreatureClass::NONE;
    Range bRange[MAX_BUILDER_RANGE];

    [[nodiscard]] static AuthContext fromPlayer(const std::shared_ptr<Player>& p);

    [[nodiscard]] bool isStaff() const { return isStaffClass(cClass); }
    [[nodiscard]] bool isDm() const { return isDmClass(cClass); }
    [[nodiscard]] bool checkRangeRestrict(const CatRef& cr, bool reading) const;  // true == RESTRICTED
};

class ApiAuthCache {
public:
    explicit ApiAuthCache(std::chrono::seconds ttl = std::chrono::seconds(60)) : ttl(ttl) {}

    [[nodiscard]] bool lookup(const std::string& key, AuthContext& out) const;
    void store(const std::string& key, const AuthContext& ctx);
    void invalidate(const std::string& key);
    void clear();

private:
    struct Entry {
        AuthContext ctx;
        std::chrono::steady_clock::time_point at;
    };

    mutable std::shared_mutex mtx;
    std::unordered_map<std::string, Entry> entries;
    std::chrono::seconds ttl;
};
