/*
 * endpoint_test.cpp
 *   Route + auth-verifier tests (no game state). The middleware 401 path needs the
 *   per-connection pipeline that handle_full() lacks, so it's smoke-tested; here we
 *   unit-test apiVerifyJwt directly.
 */
#include <chrono>
#include <string>

#include <crow.h>
#include <gtest/gtest.h>
#include <jwt-cpp/jwt.h>

#include "config.hpp"
#include "flags.hpp"
#include "httpServer.hpp"
#include "apiTestSupport.hpp"

TEST(Endpoint, VersionIsPublic) {
    HttpServer server(0);
    server.prepareForTesting();

    crow::request req;
    req.url = "/api/version";
    req.method = crow::HTTPMethod::Get;
    crow::response res;
    server.handle(req, res);

    EXPECT_EQ(res.code, 200);
    EXPECT_NE(res.body.find("version"), std::string::npos);
}

TEST(Auth, VerifyRejectsMissingHeader) {
    std::string userId, name;
    EXPECT_FALSE(apiVerifyJwt("", userId, name));
}

TEST(Auth, VerifyRejectsTooShortHeader) {
    std::string userId, name;
    EXPECT_FALSE(apiVerifyJwt("Bearer", userId, name));
}

TEST(Auth, VerifyRejectsMalformedToken) {
    std::string userId, name;
    EXPECT_FALSE(apiVerifyJwt("Bearer not.a.real.jwt", userId, name));
}

static std::string signToken(const std::string& secret, const std::string& issuer, const std::string& userId, const std::string& name, std::chrono::system_clock::time_point exp) {
    return jwt::create()
            .set_type("JWT")
            .set_issuer(issuer)
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(exp)
            .set_payload_claim("userId", jwt::claim(userId))
            .set_payload_claim("name", jwt::claim(name))
            .sign(jwt::algorithm::hs256{secret});
}

TEST(Auth, VerifyAcceptsValidToken) {
    ensureConfig();
    auto exp = std::chrono::system_clock::now() + std::chrono::hours(1);
    std::string tok = signToken(gConfig->getJwtSecret(), gConfig->getJwtIssuer(), "42", "Tester", exp);
    std::string userId, name;
    EXPECT_TRUE(apiVerifyJwt("Bearer " + tok, userId, name));
    EXPECT_EQ(userId, "42");
    EXPECT_EQ(name, "Tester");
}

TEST(Auth, VerifyRejectsExpiredToken) {
    ensureConfig();
    auto exp = std::chrono::system_clock::now() - std::chrono::hours(1);
    std::string tok = signToken(gConfig->getJwtSecret(), gConfig->getJwtIssuer(), "42", "Tester", exp);
    std::string userId, name;
    EXPECT_FALSE(apiVerifyJwt("Bearer " + tok, userId, name));
}

TEST(Auth, VerifyRejectsWrongIssuer) {
    ensureConfig();
    auto exp = std::chrono::system_clock::now() + std::chrono::hours(1);
    std::string tok = signToken(gConfig->getJwtSecret(), "evil", "42", "Tester", exp);
    std::string userId, name;
    EXPECT_FALSE(apiVerifyJwt("Bearer " + tok, userId, name));
}

TEST(Auth, VerifyRejectsWrongSecret) {
    ensureConfig();
    auto exp = std::chrono::system_clock::now() + std::chrono::hours(1);
    std::string tok = signToken("wrongkey", gConfig->getJwtIssuer(), "42", "Tester", exp);
    std::string userId, name;
    EXPECT_FALSE(apiVerifyJwt("Bearer " + tok, userId, name));
}

// Authorization gate (apiAuthorizeStaff / apiAuthorizeAccess).
// These drive the real helpers against in-memory Players (no gServer/disk). The
// predicates they call (isStaff/isDm/checkRangeRestrict/canBuild*) read only
// Player fields, so this exercises the actual gate every endpoint reuses.

TEST(Authz, NullPlayerIs401) {
    ensureConfig();
    EXPECT_EQ(apiAuthorizeStaff(nullptr), 401);
}

TEST(Authz, NonStaffIs403) {
    auto p = makePlayer(CreatureClass::FIGHTER);
    EXPECT_EQ(apiAuthorizeStaff(p), 403);
    EXPECT_EQ(apiAuthorizeAccess(p, mkCr("misc", 500), false), 403);
}

TEST(Authz, DmBypassesRanges) {
    auto p = makePlayer(CreatureClass::DUNGEONMASTER);
    EXPECT_EQ(apiAuthorizeStaff(p), 0);
    EXPECT_EQ(apiAuthorizeAccess(p, mkCr("misc", 5000), false), 0);
}

TEST(Authz, CaretakerNotRangeRestricted) {
    auto p = makePlayer(CreatureClass::CARETAKER);  // no bRange set
    EXPECT_EQ(apiAuthorizeStaff(p), 0);
    // Only BUILDER is range-checked, so a CT passes regardless of ranges.
    EXPECT_EQ(apiAuthorizeAccess(p, mkCr("misc", 500), false), 0);
}

TEST(Authz, BuilderInRangeAllowed) {
    auto p = makePlayer(CreatureClass::BUILDER, { mkRange("misc", 1, 1000) });
    EXPECT_EQ(apiAuthorizeAccess(p, mkCr("misc", 500), false), 0);
}

TEST(Authz, BuilderOutOfRangeForbidden) {
    auto p = makePlayer(CreatureClass::BUILDER, { mkRange("misc", 1, 100) });
    EXPECT_EQ(apiAuthorizeAccess(p, mkCr("misc", 500), false), 403);
}

TEST(Authz, BuilderReadOnlyAreasReadableNotWritable) {
    auto p = makePlayer(CreatureClass::BUILDER);  // no ranges
    EXPECT_EQ(apiAuthorizeAccess(p, mkCr("scroll", 1), true), 0);     // read allowed
    EXPECT_EQ(apiAuthorizeAccess(p, mkCr("scroll", 1), false), 403);  // write denied
}

TEST(Authz, BuilderZeroIdForbidden) {
    auto p = makePlayer(CreatureClass::BUILDER, { mkRange("misc", 1, 1000) });
    EXPECT_EQ(apiAuthorizeAccess(p, mkCr("misc", 0), false), 403);
}

TEST(Authz, CanBuildObjectsRespectsFlag) {
    EXPECT_FALSE(makePlayer(CreatureClass::BUILDER)->canBuildObjects());
    EXPECT_TRUE(makePlayer(CreatureClass::BUILDER, {}, { P_BUILDER_OBJS })->canBuildObjects());
    EXPECT_TRUE(makePlayer(CreatureClass::CARETAKER)->canBuildObjects());
    EXPECT_FALSE(makePlayer(CreatureClass::FIGHTER)->canBuildObjects());
}

TEST(Authz, CanBuildMonstersRespectsFlag) {
    EXPECT_FALSE(makePlayer(CreatureClass::BUILDER)->canBuildMonsters());
    EXPECT_TRUE(makePlayer(CreatureClass::BUILDER, {}, { P_BUILDER_MOBS })->canBuildMonsters());
    EXPECT_TRUE(makePlayer(CreatureClass::CARETAKER)->canBuildMonsters());
    EXPECT_FALSE(makePlayer(CreatureClass::FIGHTER)->canBuildMonsters());
}
