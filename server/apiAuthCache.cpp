/*
 * apiAuthCache.cpp
 *   Implementation of the REST API authorization snapshot cache.
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

#include "apiAuthCache.hpp"

#include "catRef.hpp"
#include "mudObjects/players.hpp"

AuthContext AuthContext::fromPlayer(const std::shared_ptr<Player>& p) {
    AuthContext ctx;
    ctx.id = p->getId();
    ctx.name = p->getName();
    ctx.cClass = p->getClass();
    for(int i = 0; i < MAX_BUILDER_RANGE; i++)
        ctx.bRange[i] = p->bRange[i];
    return ctx;
}

bool AuthContext::checkRangeRestrict(const CatRef& cr, bool reading) const {
    return builderRangeRestricted(cClass, bRange, cr, reading);
}

bool ApiAuthCache::lookup(const std::string& key, AuthContext& out) const {
    std::shared_lock<std::shared_mutex> lock(mtx);
    auto it = entries.find(key);
    if(it == entries.end()) return false;
    if(std::chrono::steady_clock::now() - it->second.at > ttl) return false;
    out = it->second.ctx;
    return true;
}

void ApiAuthCache::store(const std::string& key, const AuthContext& ctx) {
    std::unique_lock<std::shared_mutex> lock(mtx);
    entries[key] = Entry{ctx, std::chrono::steady_clock::now()};
}

void ApiAuthCache::invalidate(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mtx);
    entries.erase(key);
}

void ApiAuthCache::clear() {
    std::unique_lock<std::shared_mutex> lock(mtx);
    entries.clear();
}
