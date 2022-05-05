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

#include <crow.h>
#include <jwt-cpp/jwt.h>

class HttpServer {
public:
    explicit HttpServer(int pPort);
    virtual ~HttpServer();
    void run();
    void stop();

private:
    struct AuthMiddleware : crow::ILocalMiddleware {
        struct context {
            std::string userId;
        };

        void before_handle(crow::request& req, crow::response& res, context& ctx) {
            try {
                std::string authHeader = req.get_header_value("Authorization");
                std::string token = authHeader.substr(7);

                auto verifier = jwt::verify()
                    .allow_algorithm(jwt::algorithm::hs256{"not a real secret, replace me"});
                auto decoded = jwt::decode(token);

                verifier.verify(decoded);

                ctx.userId = decoded.get_payload_claim("userId").as_string();
            } catch(std::system_error& e) {
                res.code = 403;
                res.end();
            }
        }

        void after_handle(crow::request& req, crow::response& res, context& ctx) {} 
    };

    crow::App<AuthMiddleware> app;
    int port;
    bool appRunning = false;
    std::future<void> appFuture;
};
