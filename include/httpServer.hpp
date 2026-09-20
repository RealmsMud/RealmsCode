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

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <crow.h>
#include <nlohmann/json.hpp>

#include "apiQueue.hpp"
#include "apiAuthCache.hpp"
#include "apiLoginThrottle.hpp"
#include "login/creationFlow.hpp"

class Account;
class Player;
class CatRef;

bool apiVerifyJwt(const std::string& authHeader, std::string& outUserId, std::string& outName);
// Verify an account token (subject = account name, type=account, no userId claim).
bool apiVerifyAccountJwt(const std::string& authHeader, std::string& outAccountName);

// Mint the canonical tokens apiVerifyAccountJwt / apiVerifyJwt accept (12h expiry, HS256).
std::string apiMintAccountToken(const std::string& accountName);                     // sub + type=account
std::string apiMintPlayerToken(const std::string& userId, const std::string& name);  // userId + name claims

// A step value carries a letter, a name, or the five-number stat string.
inline constexpr std::size_t kMaxStepValueLen = 64;
// A creation path may repeat a step (re-answers), so twice the step count is the honest cap.
inline constexpr std::size_t kMaxPathLen = 2 * (static_cast<std::size_t>(Create::Flow::Step::Done) + 1);

struct ApiField {
    std::string_view name;
    std::size_t maxLen = 0;     // 0 = unbounded

    constexpr ApiField(const char* pName, std::size_t pMaxLen = 0) : name(pName), maxLen(pMaxLen) {}
};

// Parse a JSON request body; nullopt (caller: 400) on a parse error, or when a required
// field is missing, not a string, empty, or longer than maxLen.
std::optional<nlohmann::json> apiParseJsonBody(const std::string& body,
                                               std::initializer_list<ApiField> requiredStrings = {});

// nullopt = malformed path (bad step token, wrong shape, or over kMaxPathLen);
// empty vector = no path sent.
std::optional<std::vector<Create::Flow::Choice>> apiParseCreationPath(const nlohmann::json& in);

// Peer address for throttling. X-Forwarded-For is honored only when <TrustXForwardedFor>
// is set, and then only its last element: the hop the trusted proxy appended.
std::string apiClientIp(const crow::request& req);

// Game thread. Loads an account by name: 0 = ok (out set), 401 = gone, 403 = banned.
int apiGateAccount(const std::string& accountName, std::shared_ptr<Account>& out);

// Startup policy for the REST listener; why carries the log line (and any warning).
bool apiHttpConfigUsable(int port, const std::string& secret, std::string& why);

std::shared_ptr<Player> apiResolvePlayer(const std::string& id, const std::string& name);
int apiAuthorizeStaff(const std::shared_ptr<Player>& player);
int apiAuthorizeAccess(const std::shared_ptr<Player>& player, const CatRef& cr, bool reading);

bool apiResolveAuth(ApiAuthCache& cache, const std::string& id, const std::string& name, AuthContext& out);
int apiAuthorizeStaff(const AuthContext& ctx);
int apiAuthorizeAccess(const AuthContext& ctx, const CatRef& cr, bool reading);
int apiAuthorizeZone(const AuthContext& ctx, const std::string& zone);

class HttpServer {
public:
    explicit HttpServer(int pPort, ApiLoginThrottle::Settings throttleSettings = {});
    virtual ~HttpServer();
    void run();
    void stop();

    void processApiQueue() { apiQueue.drain(); }

    void setInlineMode(bool v) { apiQueue.setInlineMode(v); }
    ApiRequestQueue& queue() { return apiQueue; }
    ApiLoginThrottle& throttle() { return loginThrottle; }
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

    // Gate for account-scoped routes (character creation, claim). Resolves the account
    // token to its name; the handler loads the Account on the game thread.
    struct AccountAuthMiddleware : crow::ILocalMiddleware {
        struct context {
            std::string accountName;
        };

        void before_handle(crow::request& req, crow::response& res, context& ctx) {
            if(!apiVerifyAccountJwt(req.get_header_value("Authorization"), ctx.accountName)) {
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

    crow::App<CORSMiddleware, AuthMiddleware, AccountAuthMiddleware> app;
    ApiRequestQueue apiQueue;
    ApiAuthCache apiAuthCache;
    ApiLoginThrottle loginThrottle;
    int port;
    bool appRunning = false;
    std::future<void> appFuture;
    crow::Blueprint zoneBlueprint = crow::Blueprint("api/zones");

    void registerAuth();
    void registerAccountAuth();
    void registerCreation();
    void registerZones();
    void registerIndex();
    crow::response zoneListEndpoint(const crow::request& req, const std::string& zone, int type);

    // capture by value: a timed-out job still runs after submitAuthorized returns
    crow::response submitAuthorized(const crow::request& req, std::function<int(const AuthContext&)> authorize, std::function<crow::response(const AuthContext&)> work);

    // Crow thread. Rejects locked-out peers before the job takes a queue slot, then records
    // the outcome; isFailure decides which status codes count against the peer.
    crow::response submitThrottled(const crow::request& req, ApiRequestQueue::Job job, bool (*isFailure)(int code));
};
