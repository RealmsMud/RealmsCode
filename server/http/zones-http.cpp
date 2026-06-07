/*
 * Zones-http.cpp
 *   Zone HTTP endpoints
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
#include <climits>
#include <fstream>
#include <memory>
#include <sstream>

#include <crow.h>
#include <fmt/format.h>

#include "apiQuests.hpp"
#include "config.hpp"
#include "httpServer.hpp"
#include "server.hpp"
#include "json.hpp"
#include "catRef.hpp"
#include "quests.hpp"
#include "zone.hpp"                 // for Zone, to_json(Zone)
#include "mudObjects/players.hpp"   // for Player
#include "mudObjects/objects.hpp"   // for Object
#include "mudObjects/uniqueRooms.hpp" // for UniqueRoom
#include "mudObjects/monsters.hpp"  // for Monster
#include "xml.hpp"                  // for loadObject/loadRoom/loadMonster, LoadType

using json = nlohmann::json;

static LoadType apiReadMode(const crow::request& req) {
    const char* m = req.url_params.get("mode");
    return (m && std::string(m) == "full") ? LoadType::LS_FULL : LoadType::LS_PROTOTYPE;
}

void HttpServer::registerZones() {
    CROW_BP_ROUTE(zoneBlueprint, "/").methods("GET"_method)
            .CROW_MIDDLEWARES(app, AuthMiddleware)
            ([this](const crow::request& req) {
                return submitAuthorized(req,
                    [](const AuthContext& actx) { return apiAuthorizeStaff(actx); },
                    [this](const AuthContext&) -> crow::response {
                        json j = gConfig->zones;
                        return crow::response(to_string(j));
                    });
            });

    CROW_BP_ROUTE(zoneBlueprint, "/<string>").methods("GET"_method)
            .CROW_MIDDLEWARES(app, AuthMiddleware)
            ([this](const crow::request& req, const std::string& zone) {
                return submitAuthorized(req,
                    [](const AuthContext& actx) { return apiAuthorizeStaff(actx); },
                    [this, zone](const AuthContext&) -> crow::response {
                        auto it = gConfig->zones.find(zone);
                        if(it == gConfig->zones.end())
                            return crow::response(crow::status::NOT_FOUND);
                        json j = it->second;
                        return crow::response(to_string(j));
                    });
            });

    CROW_BP_ROUTE(zoneBlueprint, "/<string>/objects/<int>").methods("GET"_method)
            .CROW_MIDDLEWARES(app, AuthMiddleware)
            ([this](const crow::request& req, std::string zone, int id) {
                LoadType mode = apiReadMode(req);
                return submitAuthorized(req,
                    [zone, id](const AuthContext& actx) -> int {
                        if(id < 1 || id > SHRT_MAX) return 404;
                        CatRef cr(zone, (short)id);
                        return apiAuthorizeAccess(actx, cr, true);
                    },
                    [this, zone, id, mode](const AuthContext&) -> crow::response {
                        CatRef cr(zone, (short)id);
                        std::shared_ptr<Object> obj = nullptr;
                        if(!loadObject(cr, obj))
                            return crow::response(crow::status::NOT_FOUND);
                        json objectJson;
                        to_json(objectJson, *obj, false, mode, 1, false, nullptr);
                        return crow::response(to_string(objectJson));
                    });
            });

    CROW_BP_ROUTE(zoneBlueprint, "/<string>/rooms/<int>").methods("GET"_method)
            .CROW_MIDDLEWARES(app, AuthMiddleware)
            ([this](const crow::request& req, std::string zone, int id) {
                LoadType mode = apiReadMode(req);
                return submitAuthorized(req,
                    [zone, id](const AuthContext& actx) -> int {
                        if(id < 1 || id > SHRT_MAX) return 404;
                        CatRef cr(zone, (short)id);
                        return apiAuthorizeAccess(actx, cr, true);
                    },
                    [this, zone, id, mode](const AuthContext&) -> crow::response {
                        CatRef cr(zone, (short)id);
                        std::shared_ptr<UniqueRoom> room = nullptr;
                        if(!loadRoom(cr, room) || !room)
                            return crow::response(crow::status::NOT_FOUND);
                        json roomJson;
                        to_json(roomJson, *room, mode);
                        return crow::response(to_string(roomJson));
                    });
            });

    CROW_BP_ROUTE(zoneBlueprint, "/<string>/monsters/<int>").methods("GET"_method)
            .CROW_MIDDLEWARES(app, AuthMiddleware)
            ([this](const crow::request& req, std::string zone, int id) {
                LoadType mode = apiReadMode(req);
                return submitAuthorized(req,
                    [zone, id](const AuthContext& actx) -> int {
                        if(id < 1 || id > SHRT_MAX) return 404;
                        CatRef cr(zone, (short)id);
                        return apiAuthorizeAccess(actx, cr, true);
                    },
                    [this, zone, id, mode](const AuthContext&) -> crow::response {
                        CatRef cr(zone, (short)id);
                        std::shared_ptr<Monster> mon = nullptr;
                        if(!loadMonster(cr, mon) || !mon)
                            return crow::response(crow::status::NOT_FOUND);
                        json monsterJson;
                        to_json(monsterJson, *mon, mode);
                        return crow::response(to_string(monsterJson));
                    });
            });

    CROW_BP_ROUTE(zoneBlueprint, "/<string>/quests/<int>").methods("GET"_method)
            .CROW_MIDDLEWARES(app, AuthMiddleware)
            ([this](const crow::request& req, std::string zone, int id) {
                return submitAuthorized(req,
                    [zone, id](const AuthContext& actx) -> int {
                        if(id < 1 || id > SHRT_MAX) return 404;
                        CatRef cr(zone, (short)id);
                        return apiAuthorizeAccess(actx, cr, true);
                    },
                    [zone, id](const AuthContext&) -> crow::response {
                        std::ifstream ifs(apiQuestJsonPath(zone, id));
                        if(!ifs)
                            return crow::response(crow::status::NOT_FOUND);
                        std::stringstream ss;
                        ss << ifs.rdbuf();
                        crow::response res(ss.str());
                        res.set_header("Content-Type", "application/json");
                        return res;
                    });
            });

    app.register_blueprint(zoneBlueprint);
}
