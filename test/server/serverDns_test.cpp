/*
 * serverDns_test.cpp
 *   DNS cache add/lookup and the 15-day expiry (expireDns, the pure half of pruneDns).
 */
#include <gtest/gtest.h>
#include <ctime>
#include <string>

#include "server.hpp"
#include "apiTestSupport.hpp"

TEST(ServerDns, AddThenLookupHit) {
    ensureServer();
    gServer->addCache("203.0.113.7", "host-hit.example", time(nullptr));
    std::string ip = "203.0.113.7", host;
    EXPECT_TRUE(gServer->getDnsCache(ip, host));
    EXPECT_EQ(host, "host-hit.example");
}

TEST(ServerDns, LookupMiss) {
    ensureServer();
    std::string ip = "198.51.100.250", host = "unchanged";
    EXPECT_FALSE(gServer->getDnsCache(ip, host));
}

TEST(ServerDns, ExpireDropsOldKeepsRecent) {
    ensureServer();
    long now = time(nullptr);
    long sixteenDays = 60L*60*24*16;
    gServer->addCache("203.0.113.100", "old-a.example", now - sixteenDays);
    gServer->addCache("203.0.113.101", "old-b.example", now - sixteenDays);
    gServer->addCache("203.0.113.102", "fresh.example", now);

    size_t removed = gServer->expireDns(now);
    EXPECT_GE(removed, 2u);

    std::string ip, host;
    ip = "203.0.113.100"; EXPECT_FALSE(gServer->getDnsCache(ip, host));
    ip = "203.0.113.102"; host.clear();
    EXPECT_TRUE(gServer->getDnsCache(ip, host));
    EXPECT_EQ(host, "fresh.example");
}
