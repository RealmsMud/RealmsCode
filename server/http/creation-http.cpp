/*
 * creation-http.cpp
 *   commit replays the whole path before materializing.
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

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <crow.h>

#include "account.hpp"
#include "config.hpp"
#include "httpServer.hpp"
#include "json.hpp"
#include "proto.hpp" // lowercize, get_class_string
#include "raceData.hpp"
#include "server.hpp"
#include "xml.hpp" // loadPlayer
#include "mudObjects/players.hpp"
#include "login/accountAuthService.hpp"
#include "login/characterDraft.hpp"
#include "login/characterMaterializer.hpp"
#include "login/creationFlow.hpp"

using json = nlohmann::json;
namespace Flow = Create::Flow;

namespace {

// 0 = OK sentinel
int entryStatus(Flow::EntryReason reason) {
    switch (reason) {
        case Flow::EntryReason::LockedOut:
            return crow::status::FORBIDDEN;
        case Flow::EntryReason::LimitReached:
            return crow::status::CONFLICT;
        case Flow::EntryReason::ConfigUnavailable:
            return crow::status::SERVICE_UNAVAILABLE;
        case Flow::EntryReason::CreationDisabled:
            return crow::status::SERVICE_UNAVAILABLE;
        case Flow::EntryReason::Ok:
            return 0;
    }
    return crow::status::FORBIDDEN;
}

// 0 = may proceed; every creation route gates on the same account state.
int gateEntry(const Account &account, const std::string &ip) {
    Flow::EntryContext ec;
    ec.ip = ip;
    const Flow::EntryResult er = Flow::checkEntry(account, ec);
    return er.ok ? 0 : entryStatus(er.reason);
}

bool legacyFailure(int code) { return code == crow::status::UNAUTHORIZED || code == crow::status::NOT_FOUND; }

// A 409 here is a name-exists probe, the only account enumeration this route offers.
bool createFailure(int code) { return code == crow::status::CONFLICT; }

json grantedJson(const Create::InitialSkills &g) { return json{{"raceSkills", g.raceSkills}, {"raceCount", g.raceCount}, {"classSkills", g.classSkills}, {"classCount", g.classCount}}; }

} // namespace

std::optional<std::vector<Flow::Choice>> apiParseCreationPath(const json &in) {
    if (!in.contains("path"))
        return std::vector<Flow::Choice>{};
    const json &path = in.at("path");
    if (!path.is_array() || path.size() > kMaxPathLen)
        return std::nullopt;
    try {
        return path.get<std::vector<Flow::Choice>>();
    } catch (...) {
        return std::nullopt;
    }
}

void HttpServer::registerCreation() {

    CROW_ROUTE(app, "/api/account/create").methods("POST"_method)([this](const crow::request &req) {
        return submitThrottled(
                req,
                [body = req.body]() -> crow::response {
                    const auto in = apiParseJsonBody(body, {{"name", AccountAuth::kMaxNameLen}, {"pw", AccountAuth::kMaxPasswordLen}});
                    if (!in)
                        return crow::response(crow::status::BAD_REQUEST);

                    std::string name = (*in)["name"].get<std::string>();
                    const std::string pw = (*in)["pw"].get<std::string>();
                    lowercize(name, 1);

                    if (AccountAuth::checkName(name) != AccountAuth::NameError::Ok)
                        return crow::response(crow::status::BAD_REQUEST);
                    if (isdm(name)) // DM-named accounts need the DM password: telnet only
                        return crow::response(crow::status::FORBIDDEN);
                    if (!AccountAuth::isValidPassword(pw))
                        return crow::response(crow::status::BAD_REQUEST);
                    if (AccountAuth::resolveAccount(name).found)
                        return crow::response(crow::status::CONFLICT);

                    auto account = AccountAuth::createAccount(name, pw);
                    if (!account)
                        return crow::response(crow::status::INTERNAL_SERVER_ERROR);

                    json j;
                    j["token"] = apiMintAccountToken(account->getName());
                    j["account"] = account->getName();
                    return crow::response(to_string(j));
                },
                createFailure);
    });

    CROW_ROUTE(app, "/api/account/legacy-login").methods("POST"_method)([this](const crow::request &req) {
        return submitThrottled(
                req,
                [body = req.body]() -> crow::response {
                    const auto in = apiParseJsonBody(body, {{"name", AccountAuth::kMaxNameLen}, {"pw", AccountAuth::kMaxPasswordLen}});
                    if (!in)
                        return crow::response(crow::status::BAD_REQUEST);

                    std::string name = (*in)["name"].get<std::string>();
                    const std::string pw = (*in)["pw"].get<std::string>();
                    lowercize(name, 1);

                    auto lookup = AccountAuth::lookupLegacyCharacter(name);
                    if (!lookup.found)
                        return crow::response(crow::status::NOT_FOUND);
                    if (lookup.alreadyClaimed) // claimed chars must use account login
                        return crow::response(crow::status::CONFLICT);
                    if (!AccountAuth::verifyCharacterPassword(*lookup.player, pw))
                        return crow::response(crow::status::UNAUTHORIZED);

                    json j;
                    j["token"] = apiMintPlayerToken(lookup.player->getId(), lookup.player->getName());
                    j["name"] = lookup.player->getName();
                    return crow::response(to_string(j));
                },
                legacyFailure);
    });

    CROW_ROUTE(app, "/api/creation/start").methods("GET"_method).CROW_MIDDLEWARES(app, AccountAuthMiddleware)([this](const crow::request &req) {
        auto &ctx = app.get_context<AccountAuthMiddleware>(req);
        return apiQueue.submit([acct = ctx.accountName, ip = apiClientIp(req)]() -> crow::response {
            std::shared_ptr<Account> account;
            if (const int gate = apiGateAccount(acct, account))
                return crow::response(gate);
            if (const int gate = gateEntry(*account, ip))
                return crow::response(gate);

            const CharacterDraft draft;
            json j;
            j["path"] = json::array();
            j["view"] = Flow::describe(draft, Flow::firstStep());
            return crow::response(to_string(j));
        });
    });

    CROW_ROUTE(app, "/api/creation/step").methods("POST"_method).CROW_MIDDLEWARES(app, AccountAuthMiddleware)([this](const crow::request &req) {
        auto &ctx = app.get_context<AccountAuthMiddleware>(req);
        return apiQueue.submit([acct = ctx.accountName, body = req.body, ip = apiClientIp(req)]() -> crow::response {
            std::shared_ptr<Account> account;
            if (const int gate = apiGateAccount(acct, account))
                return crow::response(gate);
            if (const int gate = gateEntry(*account, ip))
                return crow::response(gate);

            const auto in = apiParseJsonBody(body, {{"value", kMaxStepValueLen}});
            if (!in)
                return crow::response(crow::status::BAD_REQUEST);
            auto parsed = apiParseCreationPath(*in);
            if (!parsed)
                return crow::response(crow::status::BAD_REQUEST);
            std::vector<Flow::Choice> path = std::move(*parsed);
            const std::string value = (*in)["value"].get<std::string>();

            CharacterDraft draft;
            Flow::Step current = Flow::firstStep();
            // The path was validated by earlier requests; skip its on-disk name probe.
            // The live advance() below and commit's replay stay strict.
            const Flow::ReplayResult wr = Flow::walk(path, draft, current, account.get(),
                                                     /*checkNameExists*/ false);
            if (!wr.ok) {
                const json j{{"failedStep", Flow::stepToken(wr.failedStep)}, {"code", wr.code}, {"message", wr.message}};
                return crow::response(crow::status::BAD_REQUEST, to_string(j));
            }
            if (current == Flow::Step::Done)
                return crow::response(crow::status::CONFLICT); // already complete; commit instead

            const Flow::AdvanceResult ar = Flow::advance(draft, current, value, {.account = account.get()});
            if (!ar.ok) {
                const json j{{"ok", false}, {"error", {{"code", ar.error.code}, {"message", ar.error.message}}}, {"view", Flow::describe(draft, current)}};
                return crow::response(to_string(j));
            }

            path.push_back({current, value});
            json j;
            j["ok"] = true;
            j["path"] = path;
            j["done"] = ar.done;
            j["view"] = Flow::describe(draft, ar.next);
            if (ar.granted.raceCount || ar.granted.classCount)
                j["granted"] = grantedJson(ar.granted);
            return crow::response(to_string(j));
        });
    });

    CROW_ROUTE(app, "/api/creation/view").methods("POST"_method).CROW_MIDDLEWARES(app, AccountAuthMiddleware)([this](const crow::request &req) {
        auto &ctx = app.get_context<AccountAuthMiddleware>(req);
        return apiQueue.submit([acct = ctx.accountName, body = req.body, ip = apiClientIp(req)]() -> crow::response {
            std::shared_ptr<Account> account;
            if (const int gate = apiGateAccount(acct, account))
                return crow::response(gate);
            if (const int gate = gateEntry(*account, ip))
                return crow::response(gate);

            const auto in = apiParseJsonBody(body);
            if (!in)
                return crow::response(crow::status::BAD_REQUEST);
            const auto path = apiParseCreationPath(*in);
            if (!path)
                return crow::response(crow::status::BAD_REQUEST);

            CharacterDraft draft;
            Flow::Step current = Flow::firstStep();
            const Flow::ReplayResult wr = Flow::walk(*path, draft, current, account.get(),
                                                     /*checkNameExists*/ false);
            if (!wr.ok) {
                const json j{{"failedStep", Flow::stepToken(wr.failedStep)}, {"code", wr.code}, {"message", wr.message}};
                return crow::response(crow::status::BAD_REQUEST, to_string(j));
            }

            json j;
            j["path"] = *path;
            j["done"] = (current == Flow::Step::Done);
            j["view"] = Flow::describe(draft, current);
            return crow::response(to_string(j));
        });
    });

    CROW_ROUTE(app, "/api/creation/commit").methods("POST"_method).CROW_MIDDLEWARES(app, AccountAuthMiddleware)([this](const crow::request &req) {
        auto &ctx = app.get_context<AccountAuthMiddleware>(req);
        return apiQueue.submit([acct = ctx.accountName, body = req.body, ip = apiClientIp(req)]() -> crow::response {
            std::shared_ptr<Account> account;
            if (const int gate = apiGateAccount(acct, account))
                return crow::response(gate);
            if (const int gate = gateEntry(*account, ip))
                return crow::response(gate);

            const auto in = apiParseJsonBody(body);
            if (!in)
                return crow::response(crow::status::BAD_REQUEST);
            const auto path = apiParseCreationPath(*in);
            if (!path)
                return crow::response(crow::status::BAD_REQUEST);

            CharacterDraft draft;
            const Flow::ReplayResult rr = Flow::replay(*path, draft, account.get());
            if (!rr.ok) {
                const json j{{"failedStep", Flow::stepToken(rr.failedStep)}, {"code", rr.code}, {"message", rr.message}};
                // strict replay re-probes the name on disk: a taken name is a conflict, not a bad path
                const auto status = rr.code == "nameTaken" ? crow::status::CONFLICT : crow::status::BAD_REQUEST;
                return crow::response(status, to_string(j));
            }
            const Create::CommitResult cr = Create::commitCharacter(draft, *account);
            if (cr.nameTaken) // lost the race since the Name step
                return crow::response(crow::status::CONFLICT);
            if (!cr.ok) {
                std::clog << "commitCharacter failed: " << cr.error << std::endl;
                return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }

            json j;
            j["name"] = cr.player->getName();
            j["level"] = cr.player->getLevel();
            j["class"] = get_class_string(static_cast<int>(cr.player->getClass()));
            if (const RaceData *race = gConfig->getRace(cr.player->getRace()))
                j["race"] = race->getName();
            return crow::response(to_string(j));
        });
    });

    CROW_ROUTE(app, "/api/account/claim").methods("POST"_method).CROW_MIDDLEWARES(app, AccountAuthMiddleware)([this](const crow::request &req) {
        auto &ctx = app.get_context<AccountAuthMiddleware>(req);
        return submitThrottled(
                req,
                [acct = ctx.accountName, body = req.body]() -> crow::response {
                    std::shared_ptr<Account> account;
                    if (const int gate = apiGateAccount(acct, account))
                        return crow::response(gate);

                    const auto in = apiParseJsonBody(body, {{"charName", AccountAuth::kMaxNameLen}, {"charPassword", AccountAuth::kMaxPasswordLen}});
                    if (!in)
                        return crow::response(crow::status::BAD_REQUEST);

                    std::string charName = (*in)["charName"].get<std::string>();
                    const std::string charPassword = (*in)["charPassword"].get<std::string>();
                    lowercize(charName, 1);

                    auto lookup = AccountAuth::lookupLegacyCharacter(charName);
                    if (!lookup.found)
                        return crow::response(crow::status::NOT_FOUND);
                    if (lookup.alreadyClaimed || account->hasCharacter(charName))
                        return crow::response(crow::status::CONFLICT);
                    if (!account->canCreateCharacter())
                        return crow::response(crow::status::CONFLICT);
                    if (!AccountAuth::verifyCharacterPassword(*lookup.player, charPassword))
                        return crow::response(crow::status::UNAUTHORIZED);
                    if (!AccountAuth::claimCharacter(*account, *lookup.player))
                        return crow::response(crow::status::INTERNAL_SERVER_ERROR);

                    lookup.player->save();
                    account->save();

                    json j;
                    j["claimed"] = lookup.player->getName();
                    return crow::response(to_string(j));
                },
                legacyFailure);
    });
}
