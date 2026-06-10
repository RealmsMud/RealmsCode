/*
 * serverIds_test.cpp
 *   Server unique-id generation, registration, and typed lookup.
 */
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "server.hpp"
#include "mudObjects/players.hpp"
#include "apiTestSupport.hpp"

namespace {
long idNum(const std::string& id) { return std::stol(id.substr(1)); }
}

TEST(ServerIds, MonsterIdFormatAndIncrement) {
    ensureServer();
    std::string a = gServer->getNextMonsterId();
    std::string b = gServer->getNextMonsterId();
    EXPECT_EQ(a.front(), 'M');
    EXPECT_EQ(b.front(), 'M');
    EXPECT_EQ(idNum(b), idNum(a) + 1);
    EXPECT_EQ(gServer->getMaxMonsterId(), idNum(b));
}

TEST(ServerIds, ObjectAndPlayerPrefixes) {
    ensureServer();
    EXPECT_EQ(gServer->getNextObjectId().front(), 'O');
    EXPECT_EQ(gServer->getNextPlayerId().front(), 'P');
}

TEST(ServerIds, LookupRejectsWrongPrefixAndEmpty) {
    ensureServer();
    EXPECT_EQ(gServer->lookupObjId(""), nullptr);
    EXPECT_EQ(gServer->lookupObjId("P5"), nullptr);   // player id, not an object
    EXPECT_EQ(gServer->lookupPlyId("O5"), nullptr);
    EXPECT_EQ(gServer->lookupCrtId("O5"), nullptr);
    EXPECT_EQ(gServer->lookupObjId("O999999999"), nullptr); // unregistered
}

TEST(ServerIds, RegisterLookupUnregisterRoundTrip) {
    auto p = makePlayer(CreatureClass::FIGHTER);
    p->setId("Ptest_roundtrip");
    ASSERT_TRUE(gServer->registerMudObject(p));
    EXPECT_EQ(gServer->lookupPlyId("Ptest_roundtrip"), p);
    EXPECT_EQ(gServer->lookupCrtId("Ptest_roundtrip"), p); // a player is a creature
    EXPECT_TRUE(gServer->unRegisterMudObject(p.get()));
    EXPECT_EQ(gServer->lookupPlyId("Ptest_roundtrip"), nullptr);
}
