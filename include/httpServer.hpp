/*
 * httpServer.hpp
 *   Code to handle the REST API
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

#include <functional>
#include <memory>
#include <string>

#include <crow.h>

#include "apiQueue.hpp"
#include "apiAuthCache.hpp"

class Player;
class CatRef;

bool apiVerifyJwt(const std::string& authHeader, std::string& outUserId, std::string& outName);
std::shared_ptr<Player> apiResolvePlayer(const std::string& id, const std::string& name);
int apiAuthorizeStaff(const std::shared_ptr<Player>& player);
int apiAuthorizeAccess(const std::shared_ptr<Player>& player, const CatRef& cr, bool reading);

bool apiResolveAuth(ApiAuthCache& cache, const std::string& id, const std::string& name, AuthContext& out);
int apiAuthorizeStaff(const AuthContext& ctx);
int apiAuthorizeAccess(const AuthContext& ctx, const CatRef& cr, bool reading);
int apiAuthorizeZone(const AuthContext& ctx, const std::string& zone);

class HttpServer {
public:
    explicit HttpServer(int pPort);
    virtual ~HttpServer();
    void run();
    void stop();

    void processApiQueue() { apiQueue.drain(); }

    void setInlineMode(bool v) { apiQueue.setInlineMode(v); }
    ApiRequestQueue& queue() { return apiQueue; }
    void prepareForTesting() { apiQueue.setInlineMode(true); app.validate(); }
    void handle(crow::request& req, crow::response& res) { app.handle_full(req, res); }

    void invalidateAuth(const std::string& id, const std::string& name) {
        if(!id.empty())   apiAuthCache.invalidate(id);
        if(!name.empty()) apiAuthCache.invalidate(name);
    }

private:
    struct AuthMiddleware : crow::ILocalMiddleware {
        struct context {
            std::string userId;
            std::string name;
        };

        void before_handle(crow::request& req, crow::response& res, context& ctx) {
            if(!apiVerifyJwt(req.get_header_value("Authorization"), ctx.userId, ctx.name)) {
                res.code = 401;
                res.end();
            }
        }

        void after_handle(crow::request& req, crow::response& res, context& ctx) {}
    };

    struct CORSMiddleware {
        struct context { };

        void before_handle(crow::request& req, crow::response& res, context& ctx) { }

        void after_handle(crow::request& req, crow::response& res, context& ctx) {
            res.add_header("Access-Control-Allow-Origin", "*");
            res.add_header("Access-Control-Allow-Headers", "*");
            res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        }
    };

    crow::App<CORSMiddleware, AuthMiddleware> app;
    ApiRequestQueue apiQueue;
    ApiAuthCache apiAuthCache;
    int port;
    bool appRunning = false;
    std::future<void> appFuture;
    crow::Blueprint zoneBlueprint = crow::Blueprint("api/zones");

    void registerAuth();
    void registerZones();
    void registerIndex();
    crow::response zoneListEndpoint(const crow::request& req, const std::string& zone, int type);

    // capture by value: a timed-out job still runs after submitAuthorized returns
    crow::response submitAuthorized(const crow::request& req,
                                    std::function<int(const AuthContext&)> authorize,
                                    std::function<crow::response(const AuthContext&)> work);
};
