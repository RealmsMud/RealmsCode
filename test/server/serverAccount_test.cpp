/*
 * serverAccount_test.cpp
 *   Account connection tracking, character listing, and cache eviction.
 */
#include <gtest/gtest.h>
#include <algorithm>
#include <string>
#include <vector>

#include "server.hpp"
#include "apiTestSupport.hpp"

namespace {
bool has(const std::vector<std::string>& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}
}

TEST(ServerAccount, TrackAndListCharacters) {
    ensureServer();
    gServer->trackAccountConnection("acct_track", "Alpha");
    gServer->trackAccountConnection("acct_track", "Beta");
    auto chars = gServer->getAccountCharacters("acct_track");
    EXPECT_EQ(chars.size(), 2u);
    EXPECT_TRUE(has(chars, "Alpha"));
    EXPECT_TRUE(has(chars, "Beta"));
}

TEST(ServerAccount, UntrackRemovesLastCharacterAndEvicts) {
    ensureServer();
    gServer->trackAccountConnection("acct_untrack", "Solo");
    gServer->untrackAccountConnection("acct_untrack", "Solo");
    EXPECT_TRUE(gServer->getAccountCharacters("acct_untrack").empty());
}

TEST(ServerAccount, ReleaseEvictsWhenNoConnections) {
    ensureServer();
    gServer->trackAccountConnection("acct_release", "One");
    gServer->releaseAccount("acct_release", "One"); // untracks the only character
    EXPECT_TRUE(gServer->getAccountCharacters("acct_release").empty());
}

TEST(ServerAccount, UnknownAccountHasNoCharacters) {
    ensureServer();
    EXPECT_TRUE(gServer->getAccountCharacters("nope_account").empty());
}

TEST(ServerAccount, SaveAllCachedAccountsNoCrashWhenEmpty) {
    ensureServer();
    gServer->saveAllCachedAccounts(); // no cached accounts -> no-op, must not crash
    SUCCEED();
}
