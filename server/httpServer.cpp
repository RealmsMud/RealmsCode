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

#include <chrono>
#include <cstddef>
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

namespace {
    constexpr int kMaxPort = 65535;
    constexpr std::size_t kMinSecretLen = 32;   // HS256 wants >= 256 bits of key

    ApiLoginThrottle::Settings throttleSettingsFromConfig() {
        return ApiLoginThrottle::Settings{
            .maxFailures = gConfig->getMaxLoginFailures(),
            .window = std::chrono::seconds(gConfig->getLoginFailureWindow()),
            .lockout = std::chrono::seconds(gConfig->getLoginLockoutTime())
        };
    }
}

bool apiHttpConfigUsable(int port, const std::string& secret, std::string& why) {
    if(port <= 0) {
        why = "HttpPort=0 (disabled)";
        return false;
    }
    if(port > kMaxPort) {
        why = fmt::format("HttpPort={} is out of range", port);
        return false;
    }
    if(secret.empty()) {
        why = "JwtSecret is empty";
        return false;
    }
    if(secret.size() < kMinSecretLen)
        why = fmt::format("JwtSecret is only {} bytes; use at least {}", secret.size(), kMinSecretLen);
    return true;
}

bool Server::initHttpServer() {
    std::string why;
    if(!apiHttpConfigUsable(gConfig->getHttpPort(), gConfig->getJwtSecret(), why)) {
        std::clog << "HTTP Server NOT started: " << why << std::endl;
        return false;
    }
    if(!why.empty())
        std::clog << "HTTP Server warning: " << why << std::endl;
    if(!gConfig->getTrustXForwardedFor())
        std::clog << "TrustXForwardedFor is off: throttle keys on the direct peer" << std::endl;

    httpServer = new HttpServer(gConfig->getHttpPort(), throttleSettingsFromConfig());
    return true;
}

void Server::cleanupHttpServer() {
    delete httpServer;
}

void Server::invalidateApiAuth(const std::string& id, const std::string& name) {
    if(httpServer)
        httpServer->invalidateAuth(id, name);
}


HttpServer::HttpServer(int pPort, ApiLoginThrottle::Settings throttleSettings) : loginThrottle(throttleSettings) {
    app.signal_clear();
    port = pPort;

    CROW_ROUTE(app, "/api/version").methods("GET"_method)
        ([](const crow::request& req){
            json j;
            j["status"] = 200;  
            j["version"] = VERSION;
            j["lastCompiled"] = fmt::format("{} {}", __DATE__, __TIME__);
            return to_string(j);
        });


    registerAuth();
    registerAccountAuth();
    registerCreation();
    registerZones();
    registerIndex();

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
