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
#include <string>

#include <crow.h>
#include <fmt/format.h>
#include <jwt-cpp/jwt.h>
#include <boost/algorithm/string/trim.hpp>

#include "config.hpp"
#include "httpServer.hpp"
#include "server.hpp"
#include "json.hpp"
#include "catRef.hpp"
#include "proto.hpp"                // for lowercize
#include "mudObjects/players.hpp"   // for Player
#include "xml.hpp"                  // for loadPlayer
#include "account.hpp"              // for Account
#include "login/accountAuthService.hpp"  // for AccountAuth

using json = nlohmann::json;

bool apiVerifyJwt(const std::string& authHeader, std::string& outUserId, std::string& outName) {
    try {
        if(authHeader.size() < 8)               // strlen("Bearer ") + 1
            return false;
        const std::string token = authHeader.substr(7);

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

bool apiVerifyAccountJwt(const std::string& authHeader, std::string& outAccountName) {
    try {
        if(authHeader.size() < 8)               // strlen("Bearer ") + 1
            return false;
        const std::string token = authHeader.substr(7);

        auto decoded = jwt::decode(token);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{gConfig->getJwtSecret()})
            .with_issuer(gConfig->getJwtIssuer())
            .verify(decoded);

        if(!decoded.has_payload_claim("type") ||
           decoded.get_payload_claim("type").as_string() != "account")
            return false;
        outAccountName = decoded.get_subject();
        return !outAccountName.empty();
    } catch(...) {
        return false;
    }
}

namespace {
    auto jwtBuilder() {
        auto now = std::chrono::system_clock::now();
        return jwt::create()
                .set_type("JWT")
                .set_issuer(gConfig->getJwtIssuer())
                .set_issued_at(now)
                .set_expires_at(now + std::chrono::hours(12));
    }
}

std::string apiMintAccountToken(const std::string& accountName) {
    return jwtBuilder()
            .set_subject(accountName)
            .set_payload_claim("type", jwt::claim(std::string("account")))
            .sign(jwt::algorithm::hs256{gConfig->getJwtSecret()});
}

std::string apiMintPlayerToken(const std::string& userId, const std::string& name) {
    return jwtBuilder()
            .set_payload_claim("userId", jwt::claim(userId))
            .set_payload_claim("name", jwt::claim(name))
            .sign(jwt::algorithm::hs256{gConfig->getJwtSecret()});
}

std::optional<nlohmann::json> apiParseJsonBody(const std::string& body,
                                               std::initializer_list<ApiField> requiredStrings) {
    nlohmann::json in;
    try {
        in = nlohmann::json::parse(body);
    } catch(...) {
        return std::nullopt;
    }
    for(const auto& field : requiredStrings) {
        const auto it = in.find(std::string(field.name));
        if(it == in.end() || !it->is_string())
            return std::nullopt;
        const std::string& value = it->get_ref<const std::string&>();
        if(value.empty() || (field.maxLen && value.size() > field.maxLen))
            return std::nullopt;
    }
    return in;
}

std::string apiClientIp(const crow::request& req) {
    if(!gConfig || !gConfig->getTrustXForwardedFor())
        return req.remote_ip_address;

    const std::string xff = req.get_header_value("X-Forwarded-For");
    const auto comma = xff.rfind(',');
    std::string hop = comma == std::string::npos ? xff : xff.substr(comma + 1);
    boost::trim(hop);
    return hop.empty() ? req.remote_ip_address : hop;
}

int apiGateAccount(const std::string& accountName, std::shared_ptr<Account>& out) {
    auto resolved = AccountAuth::resolveAccount(accountName);
    if(!resolved.found)
        return crow::status::UNAUTHORIZED;
    if(resolved.banned)
        return crow::status::FORBIDDEN;
    out = resolved.account;
    return 0;
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
    const std::string key = !id.empty() ? id : name;
    if(!key.empty() && cache.lookup(key, out))
        return true;

    const std::shared_ptr<Player> player = apiResolvePlayer(id, name);
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
        if(const int s = authorize(actx))
            return crow::response(s);
        return work(actx);
    });
}

crow::response HttpServer::submitThrottled(const crow::request& req, ApiRequestQueue::Job job,
                                           bool (*isFailure)(int code)) {
    const std::string ip = apiClientIp(req);
    if(const int wait = loginThrottle.lockedFor(ip); wait > 0) {
        crow::response res(crow::status::TOO_MANY_REQUESTS);
        res.add_header("Retry-After", std::to_string(wait));
        return res;
    }

    crow::response res = apiQueue.submit(std::move(job));
    if(isFailure(res.code))
        loginThrottle.recordFailure(ip);
    else if(res.code == static_cast<int>(crow::status::OK))
        loginThrottle.recordSuccess(ip);
    return res;
}

namespace {
    // 400/500/503 stay neutral: malformed and overloaded are not credential guesses.
    bool credentialFailure(int code) {
        return code == crow::status::UNAUTHORIZED || code == crow::status::FORBIDDEN ||
               code == crow::status::NOT_FOUND;
    }
}

void HttpServer::registerAuth() {

    CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)
            ([this](const crow::request &req) {
                return submitThrottled(req, [body = req.body]() -> crow::response {
                    const auto in = apiParseJsonBody(body, {{"name", AccountAuth::kMaxNameLen},
                                                            {"pw", AccountAuth::kMaxPasswordLen}});
                    if(!in)
                        return crow::response(crow::status::BAD_REQUEST);

                    std::string name = (*in)["name"].get<std::string>();
                    const std::string pw = (*in)["pw"].get<std::string>();
                    lowercize(name, 1);

                    std::shared_ptr<Player> player = gServer->findPlayer(name);
                    if(!player && !loadPlayer(name, player))
                        return crow::response(crow::status::NOT_FOUND);

                    if(!player->isPassword(pw))
                        return crow::response(crow::status::UNAUTHORIZED);
                    if(!player->isStaff())          // editor is staff-only
                        return crow::response(crow::status::FORBIDDEN);

                    json j;
                    j["token"] = apiMintPlayerToken(player->getId(), player->getName());
                    j["name"] = player->getName();
                    return crow::response(to_string(j));
                }, credentialFailure);
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
                            if(const int s = apiAuthorizeStaff(actx))
                                return crow::response(s);
                            json j;
                            j["userId"] = actx.id;
                            j["name"] = actx.name;
                            j["isDm"] = actx.isDm();
                            return crow::response(to_string(j));
                        });
                    });
}

void HttpServer::registerAccountAuth() {

    CROW_ROUTE(app, "/api/account/login").methods("POST"_method)
            ([this](const crow::request &req) {
                return submitThrottled(req, [body = req.body]() -> crow::response {
                    const auto in = apiParseJsonBody(body, {{"name", AccountAuth::kMaxNameLen},
                                                            {"pw", AccountAuth::kMaxPasswordLen}});
                    if(!in)
                        return crow::response(crow::status::BAD_REQUEST);

                    std::string name = (*in)["name"].get<std::string>();
                    const std::string pw = (*in)["pw"].get<std::string>();
                    lowercize(name, 1);

                    auto resolved = AccountAuth::resolveAccount(name);
                    if(!resolved.found)
                        return crow::response(crow::status::NOT_FOUND);
                    if(resolved.banned)
                        return crow::response(crow::status::FORBIDDEN);
                    if(!AccountAuth::verifyPassword(*resolved.account, pw))
                        return crow::response(crow::status::UNAUTHORIZED);

                    AccountAuth::recordLogin(*resolved.account);

                    json j;
                    // Distinguished from the player editor token by claim shape (type=account, no userId); no aud claim.
                    j["token"] = apiMintAccountToken(resolved.account->getName());
                    j["account"] = resolved.account->getName();
                    return crow::response(to_string(j));
                }, credentialFailure);
            });
}
