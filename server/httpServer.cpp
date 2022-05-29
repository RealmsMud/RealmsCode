/*
 * httpServer.cpp
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

#include <thread>

#include <crow.h>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <jwt-cpp/jwt.h>

#include "config.hpp"
#include "httpServer.hpp"
#include "server.hpp"
#include "version.hpp"
#include "quests.hpp"
#include "proto.hpp"                // for lowercize
#include "mudObjects/players.hpp"   // for Player
#include "xml.hpp"                  // for loadPlayer

using json = nlohmann::json;

bool Server::initHttpServer() {
    httpServer = new HttpServer(8080);
    if(!httpServer)
        throw std::runtime_error("WTF httpserver!");
    return true;
}

void Server::cleanupHttpServer() {
    delete httpServer;
}


HttpServer::HttpServer(int pPort) {
    app.signal_clear();
    port = pPort;

    CROW_ROUTE(app, "/version").methods("GET"_method)
        ([](const crow::request& req){
            json j;
            j["status"] = 200;  
            j["version"] = VERSION;
            j["lastCompiled"] = fmt::format("{} {}", __DATE__, __TIME__);
            return to_string(j);
        });

    CROW_ROUTE(app, "/authtest").methods("GET"_method)
    .CROW_MIDDLEWARES(app, AuthMiddleware)
        ([this](const crow::request& req){
            json j;
            auto ctx = app.get_context<AuthMiddleware>(req);
            j["userId"] = ctx.userId;
            return crow::response(to_string(j));
        });

    CROW_ROUTE(app, "/login").methods("POST"_method)
        ([](const crow::request& req){
            json body = json::parse(req.body);

            std::string name = body["name"];
            std::string pw = body["pw"];
            lowercize(name, 1);
            bool valid=false;

            // TODO: Thread Saftey
            std::shared_ptr<Player> player = gServer->findPlayer(name);
            if(!player) {
                if (!loadPlayer(name, player)) {
                    return crow::response(crow::status::NOT_FOUND);
                }
            }

            json j;
            if (player->isPassword(pw)) {
                valid = true;
                j["token"] = jwt::create()
                        .set_issuer("realms")
                        .set_payload_claim("userId", jwt::claim(player->getId()))
                        .sign(jwt::algorithm::hs256{"not a real secret, replace me"});
                j["name"] = player->getName();
            }


            if(!valid)
                return crow::response(crow::status::UNAUTHORIZED);

            return crow::response(to_string(j));
        });

    CROW_ROUTE(app, "/zones/<string>/quests/<int>").methods("GET"_method)
            ([](const crow::request& req, std::string zone, int Id){
                auto questId = CatRef(zone, (short)Id);
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

    CROW_ROUTE(app, "/zones/<string>/quests").methods("GET"_method)
            ([](const crow::request& req, std::string zone){
                return "not implemented";
            });
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::run() {
    std::clog << "Starting HTTP Server" << std::endl;
    appFuture = app.port(port).multithreaded().run_async();
    std::clog << "Started HTTP Server" << std::endl;
    appRunning = true;
}

void HttpServer::stop() {
    if(!appRunning)
        return;

    std::clog << "Stopping HTTP Server" << std::endl;
    app.stop();
    appFuture.wait();
}
