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

#include "config.hpp"
#include "httpServer.hpp"
#include "server.hpp"
#include "version.hpp"
#include "quests.hpp"

using json = nlohmann::json;

bool Server::initHttpServer() {
    httpServer = new HttpServer(8080);
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

    CROW_ROUTE(app, "/zones/<string>/quests/<int>").methods("GET"_method)
            ([](const crow::request& req, std::string zone, int Id){
                auto questId = CatRef(zone, (short)Id);
                auto quest = gConfig->getQuest(questId);
                if(quest == nullptr) {
                    json j;
                    j["status"] = 404;
                    j["message"] = "quest not found";
                    return to_string(j);
                }

                json questOverview = json(*quest);
                return to_string(questOverview);
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
