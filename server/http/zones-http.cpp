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
#include <thread>

#include <crow.h>
#include <fmt/format.h>
#include <jwt-cpp/jwt.h>

#include "config.hpp"
#include "httpServer.hpp"
#include "server.hpp"
#include "json.hpp"
#include "version.hpp"
#include "quests.hpp"
#include "proto.hpp"                // for lowercize
#include "mudObjects/players.hpp"   // for Player
#include "mudObjects/objects.hpp"   // for Object
#include "xml.hpp"                  // for loadPlayer
#include "zone.hpp"

using json = nlohmann::json;

void HttpServer::registerZones() {
    CROW_BP_ROUTE(zoneBlueprint, "/").methods("GET"_method)(
            [](const crow::request& req) {
                json j = gConfig->zones;
                return crow::response(to_string(j));;
            });

    CROW_BP_ROUTE(zoneBlueprint, "/<string>").methods("GET"_method)(
            [](const crow::request& req, const std::string& zone) {
                json j = gConfig->zones[zone];
                return crow::response(to_string(j));;
            });

    CROW_BP_ROUTE(zoneBlueprint, "/<string>/objects/<int>").methods("GET"_method)
            ([](const crow::request& req, std::string zone, int id){
                auto objId = CatRef(zone, (short)id);
                std::shared_ptr<Object> obj=nullptr;

                if(!loadObject(objId, obj)) {
                    json j;
                    j["status"] = 404;
                    j["message"] = "obj not found";
                    return crow::response(to_string(j));
                }
                json objectJson;
                to_json(objectJson, *obj, false, LoadType::LS_FULL, 1, false, nullptr);

                return crow::response(to_string(objectJson));
            });

    CROW_BP_ROUTE(zoneBlueprint, "/<string>/quests/<int>").methods("GET"_method)
            ([](const crow::request& req, std::string zone, int id){
                auto questId = CatRef(zone, (short)id);
                auto quest = gConfig->getQuest(questId);
                if(quest == nullptr) {
                    json j;
                    j["status"] = 404;
                    j["message"] = "quest not found";
                    return crow::response(to_string(j));
                }

                json questOverview = json(*quest);
                return crow::response(to_string(questOverview));
            });

    CROW_ROUTE(app, "/<string>/quests").methods("GET"_method)
            ([](const crow::request& req, const std::string& zone){
                return "not implemented";
            });
    app.register_blueprint(zoneBlueprint);
}