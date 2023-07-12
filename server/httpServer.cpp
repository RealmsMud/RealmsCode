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
#include <fmt/format.h>
#include <jwt-cpp/jwt.h>

#include "config.hpp"
#include "httpServer.hpp"
#include "json.hpp"
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


    registerAuth();
    registerZones();

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
