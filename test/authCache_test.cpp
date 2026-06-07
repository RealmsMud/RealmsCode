/*
 * authCache_test.cpp
 *   ApiAuthCache behavior + a drift guard proving the AuthContext snapshot's authz
 *   stays identical to the live Player authz it mirrors.
 */
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "apiAuthCache.hpp"
#include "httpServer.hpp"       // apiAuthorizeStaff/apiAuthorizeAccess decls
#include "apiTestSupport.hpp"

// Cache mechanics

TEST(AuthCache, HitReturnsStored) {
    ApiAuthCache c;
    AuthContext in;
    in.id = "42"; in.name = "Bane"; in.cClass = CreatureClass::DUNGEONMASTER;
    c.store("42", in);

    AuthContext out;
    ASSERT_TRUE(c.lookup("42", out));
    EXPECT_EQ(out.name, "Bane");
    EXPECT_EQ(out.cClass, CreatureClass::DUNGEONMASTER);
}

TEST(AuthCache, MissOnUnknownKey) {
    ApiAuthCache c;
    AuthContext out;
    EXPECT_FALSE(c.lookup("nope", out));
}

TEST(AuthCache, ExpiresAfterTtl) {
    ApiAuthCache c(std::chrono::seconds(0));   // anything older than 0s is stale
    AuthContext in; in.name = "X";
    c.store("k", in);
    std::this_thread::sleep_for(std::chrono::milliseconds(3));

    AuthContext out;
    EXPECT_FALSE(c.lookup("k", out));
}

TEST(AuthCache, InvalidateRemoves) {
    ApiAuthCache c;
    AuthContext in; in.name = "X";
    c.store("k", in);
    c.invalidate("k");

    AuthContext out;
    EXPECT_FALSE(c.lookup("k", out));
}

TEST(AuthCache, ClearEmptiesAll) {
    ApiAuthCache c;
    AuthContext in; in.name = "X";
    c.store("a", in);
    c.store("b", in);
    c.clear();

    AuthContext out;
    EXPECT_FALSE(c.lookup("a", out));
    EXPECT_FALSE(c.lookup("b", out));
}

// Exercises the shared_mutex paths under contention; meaningful under the
// sanitizer build (LEAK=1), a no-crash smoke test otherwise.
TEST(AuthCache, ConcurrentAccessNoRace) {
    ApiAuthCache c;
    std::vector<std::thread> ts;
    for(int t = 0; t < 8; t++)
        ts.emplace_back([&c, t] {
            for(int i = 0; i < 5000; i++) {
                std::string k = "k" + std::to_string((t * 7 + i) % 16);
                AuthContext in; in.name = k;
                c.store(k, in);
                AuthContext out;
                (void)c.lookup(k, out);
                if(i % 4 == 0) c.invalidate(k);
                if(i % 1000 == 0) c.clear();
            }
        });
    for(auto& th : ts)
        th.join();
    SUCCEED();
}

// AuthContext snapshot authz must answer identically to the live Player.

TEST(AuthCacheDrift, MatchesPlayerAuthz) {
    struct PCase { CreatureClass cls; std::vector<Range> ranges; };
    std::vector<PCase> players = {
        {CreatureClass::FIGHTER, {}},
        {CreatureClass::CARETAKER, {}},
        {CreatureClass::DUNGEONMASTER, {}},
        {CreatureClass::BUILDER, {mkRange("misc", 1, 100)}},
        {CreatureClass::BUILDER, {}},
    };
    std::vector<CatRef> crs = {
        mkCr("misc", 50), mkCr("misc", 500), mkCr("misc", 0),
        mkCr("scroll", 1), mkCr("test", 5), mkCr("song", 2),
    };

    for(const auto& pc : players) {
        auto p = makePlayer(pc.cls, pc.ranges);
        AuthContext ctx = toAuthContext(p);

        EXPECT_EQ(p->isStaff(), ctx.isStaff()) << "class=" << static_cast<int>(pc.cls);
        EXPECT_EQ(p->isDm(), ctx.isDm()) << "class=" << static_cast<int>(pc.cls);
        EXPECT_EQ(apiAuthorizeStaff(p), apiAuthorizeStaff(ctx)) << "class=" << static_cast<int>(pc.cls);

        for(const auto& cr : crs) {
            for(bool reading : {true, false}) {
                EXPECT_EQ(p->checkRangeRestrict(cr, reading), ctx.checkRangeRestrict(cr, reading))
                    << "class=" << static_cast<int>(pc.cls) << " area=" << cr.area
                    << " id=" << cr.id << " reading=" << reading;
                EXPECT_EQ(apiAuthorizeAccess(p, cr, reading), apiAuthorizeAccess(ctx, cr, reading))
                    << "class=" << static_cast<int>(pc.cls) << " area=" << cr.area
                    << " id=" << cr.id << " reading=" << reading;
            }
        }
    }
}
