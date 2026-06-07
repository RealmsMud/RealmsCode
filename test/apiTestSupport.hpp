/*
 * apiTestSupport.hpp
 *   Shared bootstrap for REST API unit tests (bare gConfig/gServer, no game data).
 */
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "config.hpp"
#include "server.hpp"
#include "catRef.hpp"
#include "range.hpp"
#include "apiAuthCache.hpp"
#include "mudObjects/players.hpp"

inline void ensureConfig() {
    if(!gConfig)
        gConfig = Config::getInstance();
}

// ~Creature touches gServer; the bare instance is a cheap, leaked no-op
inline void ensureServer() {
    if(!gServer)
        gServer = Server::getInstance();
}

inline Range mkRange(const std::string& area, short low, short high) {
    Range r;
    r.low.setArea(area);
    r.low.id = low;
    r.high = high;
    return r;
}

inline CatRef mkCr(const std::string& area, short id) {
    CatRef cr;
    cr.setArea(area);
    cr.id = id;
    return cr;
}

// make_shared: setClass/getAsPlayer use shared_from_this
inline std::shared_ptr<Player> makePlayer(CreatureClass cls, const std::vector<Range>& ranges = {}, const std::vector<int>& flags = {}) {
    ensureConfig();
    ensureServer();
    auto player = std::make_shared<Player>();
    player->setClass(cls);
    for(size_t i = 0; i < ranges.size() && i < MAX_BUILDER_RANGE; ++i)
        player->bRange[i] = ranges[i];
    for(int f : flags)
        player->setFlag(f);
    return player;
}

inline AuthContext toAuthContext(const std::shared_ptr<Player>& p) {
    return AuthContext::fromPlayer(p);
}
