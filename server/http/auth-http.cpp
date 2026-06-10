/*
 * auth-http.cpp
 *   Auth HTTP endpoints + shared authorization helpers for the REST API.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later

 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include <chrono>
#include <memory>

#include <crow.h>
#include <fmt/format.h>
#include <jwt-cpp/jwt.h>

#include "config.hpp"
#include "httpServer.hpp"
#include "server.hpp"
#include "json.hpp"
#include "catRef.hpp"
#include "proto.hpp"                // for lowercize
#include "mudObjects/players.hpp"   // for Player
#include "xml.hpp"                  // for loadPlayer

using json = nlohmann::json;

bool apiVerifyJwt(const std::string& authHeader, std::string& outUserId, std::string& outName) {
    try {
        if(authHeader.size() < 8)               // strlen("Bearer ") + 1
            return false;
        std::string token = authHeader.substr(7);

        auto decoded = jwt::decode(token);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{gConfig->getJwtSecret()})
            .with_issuer(gConfig->getJwtIssuer())
            .verify(decoded);                   // also validates exp/nbf/iat when present

        outUserId = decoded.get_payload_claim("userId").as_string();
        if(decoded.has_payload_claim("name"))
            outName = decoded.get_payload_claim("name").as_string();
        return true;
    } catch(...) {
        return false;
    }
}

std::shared_ptr<Player> apiResolvePlayer(const std::string& id, const std::string& name) {
    std::shared_ptr<Player> player = gServer->lookupPlyId(id);
    if(player)
        return player;
    if(!name.empty() && loadPlayer(name, player))
        return player;
    return nullptr;
}

// checkRangeRestrict returns true == RESTRICTED, so the isStaff gate must precede it.
template<class P>
static int authorizeResolvedAccess(const P& principal, const CatRef& cr, bool reading) {
    if(!principal.isStaff())
        return 403;
    if(principal.isDm())
        return 0;
    return principal.checkRangeRestrict(cr, reading) ? 403 : 0;
}

int apiAuthorizeStaff(const std::shared_ptr<Player>& player) {
    if(!player)
        return 401;
    if(!player->isStaff())
        return 403;
    return 0;
}

int apiAuthorizeAccess(const std::shared_ptr<Player>& player, const CatRef& cr, bool reading) {
    if(!player)
        return 401;
    return authorizeResolvedAccess(*player, cr, reading);
}

bool apiResolveAuth(ApiAuthCache& cache, const std::string& id, const std::string& name, AuthContext& out) {
    std::string key = !id.empty() ? id : name;
    if(!key.empty() && cache.lookup(key, out))
        return true;

    std::shared_ptr<Player> player = apiResolvePlayer(id, name);
    if(!player)
        return false;

    out = AuthContext::fromPlayer(player);

    if(!key.empty())
        cache.store(key, out);
    return true;
}

int apiAuthorizeStaff(const AuthContext& ctx) {
    return ctx.isStaff() ? 0 : 403;
}

int apiAuthorizeAccess(const AuthContext& ctx, const CatRef& cr, bool reading) {
    return authorizeResolvedAccess(ctx, cr, reading);
}

// lists have no id, so checkRangeRestrict (which restricts id<=0) is wrong here
int apiAuthorizeZone(const AuthContext& ctx, const std::string& zone) {
    if(!ctx.isStaff())
        return 403;
    if(ctx.cClass != CreatureClass::BUILDER)    // DM/CT bypass ranges
        return 0;
    for(int i = 0; i < MAX_BUILDER_RANGE; i++)
        if(ctx.bRange[i].low.isArea(zone))
            return 0;
    return 403;
}

crow::response HttpServer::submitAuthorized(const crow::request& req,
                                            std::function<int(const AuthContext&)> authorize,
                                            std::function<crow::response(const AuthContext&)> work) {
    auto& ctx = app.get_context<AuthMiddleware>(req);
    return apiQueue.submit([this, uid = ctx.userId, uname = ctx.name,
                            authorize = std::move(authorize), work = std::move(work)]() -> crow::response {
        AuthContext actx;
        if(!apiResolveAuth(apiAuthCache, uid, uname, actx))
            return crow::response(401);
        if(int s = authorize(actx))
            return crow::response(s);
        return work(actx);
    });
}

void HttpServer::registerAuth() {

    CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)
            ([this](const crow::request &req) {
                return apiQueue.submit([body = req.body]() -> crow::response {
                    json in;
                    try {
                        in = json::parse(body);
                    } catch(...) {
                        return crow::response(crow::status::BAD_REQUEST);
                    }
                    if(!in.contains("name") || !in.contains("pw"))
                        return crow::response(crow::status::BAD_REQUEST);

                    std::string name = in["name"];
                    std::string pw = in["pw"];
                    lowercize(name, 1);

                    std::shared_ptr<Player> player = gServer->findPlayer(name);
                    if(!player && !loadPlayer(name, player))
                        return crow::response(crow::status::NOT_FOUND);

                    if(!player->isPassword(pw))
                        return crow::response(crow::status::UNAUTHORIZED);
                    if(!player->isStaff())          // editor is staff-only
                        return crow::response(crow::status::FORBIDDEN);

                    auto now = std::chrono::system_clock::now();
                    std::string token = jwt::create()
                            .set_type("JWT")
                            .set_issuer(gConfig->getJwtIssuer())
                            .set_issued_at(now)
                            .set_expires_at(now + std::chrono::hours(12))
                            .set_payload_claim("userId", jwt::claim(player->getId()))
                            .set_payload_claim("name", jwt::claim(player->getName()))
                            .sign(jwt::algorithm::hs256{gConfig->getJwtSecret()});

                    json j;
                    j["token"] = token;
                    j["name"] = player->getName();
                    return crow::response(to_string(j));
                });
            });

    // Smoke-test route: echoes the resolved staff identity.
    CROW_ROUTE(app, "/api/authtest").methods("GET"_method)
            .CROW_MIDDLEWARES(app, AuthMiddleware)
                    ([this](const crow::request &req) {
                        auto& ctx = app.get_context<AuthMiddleware>(req);
                        return apiQueue.submit([this, id = ctx.userId, name = ctx.name]() -> crow::response {
                            AuthContext actx;
                            if(!apiResolveAuth(apiAuthCache, id, name, actx))
                                return crow::response(401);
                            if(int s = apiAuthorizeStaff(actx))
                                return crow::response(s);
                            json j;
                            j["userId"] = actx.id;
                            j["name"] = actx.name;
                            j["isDm"] = actx.isDm();
                            return crow::response(to_string(j));
                        });
                    });
}
