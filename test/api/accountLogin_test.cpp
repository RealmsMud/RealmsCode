// route tests for POST /api/account/login; drives the real Crow handler via handle_full(), accounts seeded in-memory.
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include <crow.h>
#include <gtest/gtest.h>

#include "account.hpp"
#include "httpServer.hpp"
#include "paths.hpp"
#include "login/accountAuthService.hpp"
#include "apiTestSupport.hpp"

using namespace std::chrono_literals;

namespace {

constexpr int kTestMaxFailures = 3;

crow::response postLogin(HttpServer& server, const std::string& body,
                         const std::string& remoteIp = "10.0.0.1", const std::string& xff = "") {
    crow::request req;
    req.url = "/api/account/login";
    req.method = crow::HTTPMethod::Post;
    req.body = body;
    req.remote_ip_address = remoteIp;
    if(!xff.empty())
        req.headers.emplace("X-Forwarded-For", xff);
    crow::response res;
    server.handle(req, res);
    return res;
}

std::shared_ptr<Account> seedAccount(const std::string& properName, const std::string& pw, bool banned = false) {
    auto acct = std::make_shared<Account>(properName);
    acct->setPassword(pw);
    acct->setBanned(banned);
    gServer->cacheAccount(acct);
    return acct;
}

struct AccountLogin : ::testing::Test {
    // A small threshold keeps the throttle tests short; production defaults come from config.
    HttpServer server{0, {.maxFailures = kTestMaxFailures, .window = 60s, .lockout = 60s}};
    void SetUp() override {
        ensureConfig();
        ensureServer();
        server.prepareForTesting();
    }
    void TearDown() override {
        gConfig->setTrustXForwardedFor(false);
    }
};

TEST_F(AccountLogin, RejectsMalformedJson) {
    EXPECT_EQ(postLogin(server, "not json").code, 400);
}

TEST_F(AccountLogin, RejectsMissingFields) {
    EXPECT_EQ(postLogin(server, R"({"name":"someacct"})").code, 400);
}

TEST_F(AccountLogin, MissingAccountIs404) {
    EXPECT_EQ(postLogin(server, R"({"name":"nosuchacctxyz","pw":"whatever"})").code, 404);
}

TEST_F(AccountLogin, WrongPasswordIs401) {
    // Account names are stored proper-cased (lowercize(name,1)); the endpoint
    // applies the same normalization to the request name before lookup.
    seedAccount("Apiloginacct", "rightpass");
    EXPECT_EQ(postLogin(server, R"({"name":"apiloginacct","pw":"wrongpass"})").code, 401);
}

TEST_F(AccountLogin, BannedAccountIs403) {
    seedAccount("Apiloginbanned", "rightpass", /*banned*/ true);
    EXPECT_EQ(postLogin(server, R"({"name":"apiloginbanned","pw":"rightpass"})").code, 403);
}

TEST_F(AccountLogin, ValidLoginReturnsToken) {
    if(getenv("REALMS_BASE_PATH") == nullptr)
        GTEST_SKIP() << "recordLogin persists the account, which needs a writable data path";

    const std::string name = "Apiloginok";  // proper-cased; request sends "apiloginok"
    seedAccount(name, "goodpass");
    const auto file = (Path::Account / name).replace_extension("json");
    std::filesystem::remove(file);

    auto res = postLogin(server, R"({"name":"apiloginok","pw":"goodpass"})");
    EXPECT_EQ(res.code, 200);
    EXPECT_NE(res.body.find("token"), std::string::npos);
    EXPECT_NE(res.body.find(name), std::string::npos);

    std::filesystem::remove(file);
}

// Body validation: wrong types and out-of-bound lengths never reach the account lookup.

TEST_F(AccountLogin, NonStringNameIs400) {
    EXPECT_EQ(postLogin(server, R"({"name":123,"pw":"whatever"})").code, 400);
}

TEST_F(AccountLogin, NonStringPasswordIs400) {
    EXPECT_EQ(postLogin(server, R"({"name":"someacct","pw":false})").code, 400);
}

TEST_F(AccountLogin, EmptyNameIs400) {
    EXPECT_EQ(postLogin(server, R"({"name":"","pw":"whatever"})").code, 400);
}

TEST_F(AccountLogin, OverlongNameIs400) {
    const std::string name(AccountAuth::kMaxNameLen + 1, 'a');
    EXPECT_EQ(postLogin(server, R"({"name":")" + name + R"(","pw":"whatever"})").code, 400);
}

TEST_F(AccountLogin, OverlongPasswordIs400) {
    const std::string pw(AccountAuth::kMaxPasswordLen + 1, 'a');
    EXPECT_EQ(postLogin(server, R"({"name":"someacct","pw":")" + pw + R"("})").code, 400);
}

// Throttle wiring.

TEST_F(AccountLogin, ThrottlesAfterRepeated401) {
    seedAccount("Apithrottleacct", "rightpass");
    const std::string body = R"({"name":"apithrottleacct","pw":"wrongpass"})";
    for(int i = 0; i < kTestMaxFailures; i++)
        EXPECT_EQ(postLogin(server, body).code, 401) << "attempt " << i;

    auto res = postLogin(server, body);
    EXPECT_EQ(res.code, 429);
    EXPECT_FALSE(res.get_header_value("Retry-After").empty());
}

TEST_F(AccountLogin, NotFoundCountsTowardThrottle) {
    const std::string body = R"({"name":"nosuchacctxyz","pw":"whatever"})";
    for(int i = 0; i < kTestMaxFailures; i++)
        EXPECT_EQ(postLogin(server, body).code, 404) << "attempt " << i;

    EXPECT_EQ(postLogin(server, body).code, 429);
}

TEST_F(AccountLogin, BadRequestDoesNotCount) {
    for(int i = 0; i < kTestMaxFailures * 2; i++)
        EXPECT_EQ(postLogin(server, "not json").code, 400) << "attempt " << i;

    EXPECT_EQ(postLogin(server, R"({"name":"nosuchacctxyz","pw":"whatever"})").code, 404);
}

TEST_F(AccountLogin, SuccessClearsFailures) {
    if(getenv("REALMS_BASE_PATH") == nullptr)
        GTEST_SKIP() << "recordLogin persists the account, which needs a writable data path";

    const std::string name = "Apithrottleclear";
    seedAccount(name, "goodpass");
    const auto file = (Path::Account / name).replace_extension("json");
    std::filesystem::remove(file);

    for(int i = 0; i < kTestMaxFailures - 1; i++)
        EXPECT_EQ(postLogin(server, R"({"name":"apithrottleclear","pw":"wrongpass"})").code, 401);
    EXPECT_EQ(postLogin(server, R"({"name":"apithrottleclear","pw":"goodpass"})").code, 200);
    EXPECT_EQ(postLogin(server, R"({"name":"apithrottleclear","pw":"wrongpass"})").code, 401);

    std::filesystem::remove(file);
}

TEST_F(AccountLogin, KeyedByRemoteIp) {
    const std::string body = R"({"name":"nosuchacctxyz","pw":"whatever"})";
    for(int i = 0; i < kTestMaxFailures; i++)
        postLogin(server, body, "10.0.0.5");

    EXPECT_EQ(postLogin(server, body, "10.0.0.5").code, 429);
    EXPECT_EQ(postLogin(server, body, "10.0.0.6").code, 404);
}

TEST_F(AccountLogin, IgnoresXffByDefault) {
    const std::string body = R"({"name":"nosuchacctxyz","pw":"whatever"})";
    for(int i = 0; i < kTestMaxFailures; i++)
        postLogin(server, body, "10.0.0.7", "203.0.113." + std::to_string(i));

    // Untrusted: the forged header cannot spread the budget across keys.
    EXPECT_EQ(postLogin(server, body, "10.0.0.7", "203.0.113.99").code, 429);
}

TEST_F(AccountLogin, HonorsLastXffHopWhenTrusted) {
    gConfig->setTrustXForwardedFor(true);
    const std::string body = R"({"name":"nosuchacctxyz","pw":"whatever"})";
    for(int i = 0; i < kTestMaxFailures; i++)
        postLogin(server, body, "10.0.0.8", "198.51.100.1, 203.0.113.7");

    EXPECT_EQ(postLogin(server, body, "10.0.0.8", "198.51.100.2, 203.0.113.7").code, 429);
    // Same proxy chain prefix, different last hop: a different client, its own budget.
    EXPECT_EQ(postLogin(server, body, "10.0.0.8", "198.51.100.1, 203.0.113.8").code, 404);
}

} // namespace
