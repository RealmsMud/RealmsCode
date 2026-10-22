// route tests for the account/creation REST endpoints via handle_full(); flow tests skip without REALMS_BASE_PATH.
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <crow.h>
#include <gtest/gtest.h>
#include <jwt-cpp/jwt.h>

#include "account.hpp"
#include "config.hpp"
#include "httpServer.hpp"
#include "paths.hpp"
#include "server.hpp"
#include "login/accountAuthService.hpp"
#include "apiTestSupport.hpp"

using namespace std::chrono_literals;

namespace {

bool gameConfigReady() {
    static const bool ready = [] {
        ensureConfig();
        ensureServer();
        const bool ok = gConfig->loadConfig() && gConfig->loadStartLoc() && gConfig->loadSkillGroups() && gConfig->loadSkills() && gConfig->loadDeities() && gConfig->loadClasses() && gConfig->loadRaces();
        gConfig->loadCalendar();
        return ok;
    }();
    return ready;
}

crow::response post(HttpServer &server, const std::string &url, const std::string &body, const std::string &token = "", const std::string &remoteIp = "10.0.0.1") {
    crow::request req;
    req.url = url;
    req.method = crow::HTTPMethod::Post;
    req.body = body;
    req.remote_ip_address = remoteIp;
    if (!token.empty())
        req.headers.emplace("Authorization", "Bearer " + token);
    crow::response res;
    server.handle(req, res);
    return res;
}

std::shared_ptr<Account> seedAccount(const std::string &properName, const std::string &pw, bool banned = false) {
    ensureConfig();
    ensureServer();
    auto acct = std::make_shared<Account>(properName);
    acct->setPassword(pw);
    acct->setBanned(banned);
    gServer->cacheAccount(acct);
    return acct;
}

void removeAccountFile(const std::string &properName) { std::filesystem::remove((Path::Account / properName).replace_extension("json")); }

std::string makeAccountToken(const std::string &subject) {
    auto now = std::chrono::system_clock::now();
    return jwt::create()
            .set_type("JWT")
            .set_issuer(gConfig->getJwtIssuer())
            .set_issued_at(now)
            .set_expires_at(now + std::chrono::hours(1))
            .set_subject(subject)
            .set_payload_claim("type", jwt::claim(std::string("account")))
            .sign(jwt::algorithm::hs256{gConfig->getJwtSecret()});
}

std::string makeStaffToken() {
    auto now = std::chrono::system_clock::now();
    return jwt::create()
            .set_type("JWT")
            .set_issuer(gConfig->getJwtIssuer())
            .set_issued_at(now)
            .set_expires_at(now + std::chrono::hours(1))
            .set_payload_claim("userId", jwt::claim(std::string("1")))
            .sign(jwt::algorithm::hs256{gConfig->getJwtSecret()});
}

constexpr int kTestMaxFailures = 3;

struct CreationHttp : ::testing::Test {
    HttpServer server{0, {.maxFailures = kTestMaxFailures, .window = 60s, .lockout = 60s}};
    void SetUp() override {
        ensureConfig();
        ensureServer();
        server.prepareForTesting();
    }
    void TearDown() override { gConfig->setTrustXForwardedFor(false); }
};

TEST_F(CreationHttp, AccountCreateRejectsMalformedJson) { EXPECT_EQ(post(server, "/api/account/create", "not json").code, 400); }

TEST_F(CreationHttp, AccountCreateRejectsWeakPassword) { EXPECT_EQ(post(server, "/api/account/create", R"({"name":"weakpwacct","pw":"x"})").code, 400); }

TEST_F(CreationHttp, AccountCreateRefusesDmName) { EXPECT_EQ(post(server, "/api/account/create", R"({"name":"bane","pw":"goodpass"})").code, 403); }

// The middleware 401 path needs the per-connection pipeline that handle_full() lacks
// (see endpoint_test.cpp), so the account-token verifier is unit-tested directly.
TEST_F(CreationHttp, JwtRejectsMissingHeader) {
    ensureConfig();
    std::string acct;
    EXPECT_FALSE(apiVerifyAccountJwt("", acct));
}

TEST_F(CreationHttp, JwtRejectsGarbageToken) {
    ensureConfig();
    std::string acct;
    EXPECT_FALSE(apiVerifyAccountJwt("Bearer not.a.real.jwt", acct));
}

TEST_F(CreationHttp, JwtRejectsStaffToken) {
    ensureConfig();
    std::string acct;
    EXPECT_FALSE(apiVerifyAccountJwt("Bearer " + makeStaffToken(), acct)); // no type=account
}

TEST_F(CreationHttp, JwtAcceptsAccountToken) {
    ensureConfig();
    std::string acct;
    EXPECT_TRUE(apiVerifyAccountJwt("Bearer " + makeAccountToken("Someacct"), acct));
    EXPECT_EQ(acct, "Someacct");
}

TEST_F(CreationHttp, AccountCreateAndDuplicate) {
    if (!gameConfigReady())
        GTEST_SKIP() << "createAccount persists + checkName needs data (set REALMS_BASE_PATH)";

    const std::string name = "Httpcreateacct";
    removeAccountFile(name);

    auto first = post(server, "/api/account/create", R"({"name":"httpcreateacct","pw":"goodpass"})");
    EXPECT_EQ(first.code, 200);
    EXPECT_NE(first.body.find("token"), std::string::npos);

    auto dup = post(server, "/api/account/create", R"({"name":"httpcreateacct","pw":"goodpass"})");
    EXPECT_EQ(dup.code, 409);

    removeAccountFile(name);
}

TEST_F(CreationHttp, AccountCreateNonStringFieldsAre400) {
    EXPECT_EQ(post(server, "/api/account/create", R"({"name":123,"pw":"goodpass"})").code, 400);
    EXPECT_EQ(post(server, "/api/account/create", R"({"name":"someacct","pw":null})").code, 400);
}

TEST_F(CreationHttp, AccountCreateThrottlesOn409) {
    seedAccount("Existsacct", "goodpass");
    const std::string body = R"({"name":"existsacct","pw":"goodpass"})";
    for (int i = 0; i < kTestMaxFailures; i++)
        EXPECT_EQ(post(server, "/api/account/create", body, "", "10.9.9.1").code, 409) << "attempt " << i;

    auto res = post(server, "/api/account/create", body, "", "10.9.9.1");
    EXPECT_EQ(res.code, 429);
    EXPECT_FALSE(res.get_header_value("Retry-After").empty());
}

TEST_F(CreationHttp, LegacyLoginRejectsNonStringName) { EXPECT_EQ(post(server, "/api/account/legacy-login", R"({"name":[],"pw":"goodpass"})").code, 400); }

TEST_F(CreationHttp, LegacyLoginThrottlesOn404) {
    const std::string body = R"({"name":"nosuchlegacychar","pw":"whatever"})";
    for (int i = 0; i < kTestMaxFailures; i++)
        EXPECT_EQ(post(server, "/api/account/legacy-login", body, "", "10.9.9.2").code, 404) << "attempt " << i;

    EXPECT_EQ(post(server, "/api/account/legacy-login", body, "", "10.9.9.2").code, 429);
}

// The authed creation routes need the per-connection middleware handle_full() lacks,
// so their shared gates are unit-tested directly.

TEST_F(CreationHttp, GateAccountMissingIs401) {
    ensureConfig();
    ensureServer();
    std::shared_ptr<Account> out;
    EXPECT_EQ(apiGateAccount("Nosuchgateacct", out), 401);
    EXPECT_EQ(out, nullptr);
}

TEST_F(CreationHttp, GateAccountBannedIs403) {
    seedAccount("Gatebannedacct", "goodpass", /*banned*/ true);
    std::shared_ptr<Account> out;
    EXPECT_EQ(apiGateAccount("Gatebannedacct", out), 403);
}

TEST_F(CreationHttp, GateAccountResolvedIsOk) {
    auto seeded = seedAccount("Gateokacct", "goodpass");
    std::shared_ptr<Account> out;
    EXPECT_EQ(apiGateAccount("Gateokacct", out), 0);
    EXPECT_EQ(out, seeded);
}

TEST_F(CreationHttp, ClientIpDefaultsToRemoteAddress) {
    ensureConfig();
    gConfig->setTrustXForwardedFor(false);
    crow::request req;
    req.remote_ip_address = "10.0.0.1";
    req.headers.emplace("X-Forwarded-For", "203.0.113.7");
    EXPECT_EQ(apiClientIp(req), "10.0.0.1");
}

TEST_F(CreationHttp, ClientIpTrustedUsesLastHop) {
    ensureConfig();
    gConfig->setTrustXForwardedFor(true);
    crow::request req;
    req.remote_ip_address = "10.0.0.1";
    req.headers.emplace("X-Forwarded-For", "198.51.100.1, 203.0.113.7");
    EXPECT_EQ(apiClientIp(req), "203.0.113.7");
    gConfig->setTrustXForwardedFor(false);
}

TEST_F(CreationHttp, ClientIpTrustedFallsBackWhenHeaderAbsent) {
    ensureConfig();
    gConfig->setTrustXForwardedFor(true);
    crow::request req;
    req.remote_ip_address = "10.0.0.1";
    EXPECT_EQ(apiClientIp(req), "10.0.0.1");
    gConfig->setTrustXForwardedFor(false);
}

TEST_F(CreationHttp, ParsePathMissingIsEmpty) {
    const auto path = apiParseCreationPath(nlohmann::json::parse(R"({"value":"a"})"));
    ASSERT_TRUE(path);
    EXPECT_TRUE(path->empty());
}

TEST_F(CreationHttp, ParsePathRejectsNonArray) { EXPECT_FALSE(apiParseCreationPath(nlohmann::json::parse(R"({"path":"race"})"))); }

TEST_F(CreationHttp, ParsePathRejectsOverlongPath) {
    nlohmann::json entry{{"step", "race"}, {"value", "1"}};
    nlohmann::json in;
    in["path"] = nlohmann::json::array();
    for (std::size_t i = 0; i <= kMaxPathLen; i++)
        in["path"].push_back(entry);

    EXPECT_FALSE(apiParseCreationPath(in));
}

TEST_F(CreationHttp, ParsePathRejectsUnknownStepToken) { EXPECT_FALSE(apiParseCreationPath(nlohmann::json::parse(R"({"path":[{"step":"nosuchstep","value":"1"}]})"))); }

} // namespace
