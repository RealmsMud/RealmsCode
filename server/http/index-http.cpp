/*
 * index-http.cpp
 *   Per-zone list endpoints for the REST API (served from the ZoneIndex).
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

#include <chrono>
#include <vector>

#include <crow.h>

#include "apiQuests.hpp"
#include "catRef.hpp"
#include "config.hpp"
#include "httpServer.hpp"
#include "server.hpp"
#include "json.hpp"
#include "quests.hpp"

using json = nlohmann::json;

static crow::response zoneListResponse(ZoneIndex::Type type, const std::string& zone) {
    std::vector<ZoneSummary> out;
    (void)gServer->zoneIndex.list(type, zone, out);
    json j = json::array();
    for(const auto& s : out)
        j.push_back({{"id", s.id}, {"name", s.name}});
    return crow::response(to_string(j));
}

crow::response HttpServer::zoneListEndpoint(const crow::request& req, const std::string& zone, int type) {
    auto t = static_cast<ZoneIndex::Type>(type);

    crow::response authRes = submitAuthorized(req,
        [zone](const AuthContext& actx) { return apiAuthorizeZone(actx, zone); },
        [](const AuthContext&) { return crow::response(200); });
    if(authRes.code != 200)
        return authRes;

    if(!gServer->zoneIndex.isBuilt(t, zone)) {
        gServer->zoneIndexBuilder.request(t, zone);
        if(!gServer->zoneIndex.waitUntilBuilt(t, zone, std::chrono::seconds(30)))
            return crow::response(503);
    }
    return zoneListResponse(t, zone);
}

void HttpServer::registerIndex() {
    CROW_ROUTE(app, "/api/zones/<string>/rooms").methods("GET"_method)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
        ([this](const crow::request& req, const std::string& zone) {
            return zoneListEndpoint(req, zone, static_cast<int>(ZoneIndex::Type::Room)); });
    CROW_ROUTE(app, "/api/zones/<string>/objects").methods("GET"_method)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
        ([this](const crow::request& req, const std::string& zone) {
            return zoneListEndpoint(req, zone, static_cast<int>(ZoneIndex::Type::Object)); });
    CROW_ROUTE(app, "/api/zones/<string>/monsters").methods("GET"_method)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
        ([this](const crow::request& req, const std::string& zone) {
            return zoneListEndpoint(req, zone, static_cast<int>(ZoneIndex::Type::Monster)); });

    CROW_ROUTE(app, "/api/zones/<string>/quests").methods("GET"_method)
        .CROW_MIDDLEWARES(app, AuthMiddleware)
        ([this](const crow::request& req, const std::string& zone) {
            bool includeDisabled = req.url_params.get("includeDisabled") != nullptr;

            crow::response authRes = submitAuthorized(req,
                [zone](const AuthContext& actx) { return apiAuthorizeZone(actx, zone); },
                [](const AuthContext&) { return crow::response(200); });
            if(authRes.code != 200)
                return authRes;

            auto t = ZoneIndex::Type::Quest;
            if(!gServer->zoneIndex.isBuilt(t, zone)) {
                gServer->zoneIndexBuilder.request(t, zone);
                if(!gServer->zoneIndex.waitUntilBuilt(t, zone, std::chrono::seconds(30)))
                    return crow::response(503);
            }
            std::vector<ZoneSummary> out;
            (void)gServer->zoneIndex.list(t, zone, out);

            // index carries id+name; disabled comes from the JSON quest file (REST store)
            json arr = json::array();
            for(const auto& s : out) {
                bool disabled = false;
                if(auto j = apiReadQuestJson(zone, s.id))
                    disabled = j->value("disabled", false);
                if(disabled && !includeDisabled)
                    continue;
                arr.push_back({{"id", s.id}, {"name", s.name}, {"disabled", disabled}});
            }
            return crow::response(to_string(arr));
        });
}
