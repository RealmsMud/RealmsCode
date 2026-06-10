/*
 * serverActiveList_test.cpp
 *   Active-monster list add/remove/membership, incl. expired weak_ptr handling.
 */
#include <gtest/gtest.h>
#include <memory>

#include "server.hpp"
#include "mudObjects/monsters.hpp"
#include "apiTestSupport.hpp"

TEST(ServerActiveList, AddMakesActiveDelRemoves) {
    ensureServer();
    auto m = std::make_shared<Monster>();
    EXPECT_FALSE(gServer->isActive(m.get()));
    gServer->addActive(m);
    EXPECT_TRUE(gServer->isActive(m.get()));
    gServer->delActive(m.get());
    EXPECT_FALSE(gServer->isActive(m.get()));
}

TEST(ServerActiveList, AddIsIdempotent) {
    ensureServer();
    auto m = std::make_shared<Monster>();
    gServer->addActive(m);
    gServer->addActive(m); // second add must not create a duplicate entry
    gServer->delActive(m.get());
    EXPECT_FALSE(gServer->isActive(m.get()));
}

TEST(ServerActiveList, ExpiredEntryNotActive) {
    ensureServer();
    Monster* raw = nullptr;
    {
        auto m = std::make_shared<Monster>();
        raw = m.get();
        gServer->addActive(m);
        EXPECT_TRUE(gServer->isActive(raw));
    } // m destroyed -> the weak_ptr left in activeList expires
    EXPECT_FALSE(gServer->isActive(raw)); // must skip expired entries, not match
}
