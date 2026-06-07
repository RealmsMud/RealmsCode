/*
 * zoneIndex_test.cpp
 *   In-memory behavior of the per-zone summary index (no file I/O).
 */
#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "zoneIndex.hpp"

TEST(ZoneIndex, UnbuiltListReturnsFalse) {
    ZoneIndex idx;
    EXPECT_FALSE(idx.isBuilt(ZoneIndex::Type::Room, "misc"));
    std::vector<ZoneSummary> out;
    EXPECT_FALSE(idx.list(ZoneIndex::Type::Room, "misc", out));
}

TEST(ZoneIndex, UpsertAloneDoesNotBuild) {
    ZoneIndex idx;
    idx.upsert(ZoneIndex::Type::Room, "misc", 5, "Fifth");
    EXPECT_FALSE(idx.isBuilt(ZoneIndex::Type::Room, "misc"));
    std::vector<ZoneSummary> out;
    EXPECT_FALSE(idx.list(ZoneIndex::Type::Room, "misc", out));
}

TEST(ZoneIndex, BuiltListOrdersById) {
    ZoneIndex idx;
    idx.upsert(ZoneIndex::Type::Room, "misc", 5, "Fifth");
    idx.upsert(ZoneIndex::Type::Room, "misc", 2, "Second");
    idx.markBuilt(ZoneIndex::Type::Room, "misc");

    std::vector<ZoneSummary> out;
    ASSERT_TRUE(idx.list(ZoneIndex::Type::Room, "misc", out));
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].id, 2);            // std::map orders by id
    EXPECT_EQ(out[0].name, "Second");
    EXPECT_EQ(out[1].id, 5);
}

TEST(ZoneIndex, UpsertOverwrites) {
    ZoneIndex idx;
    idx.upsert(ZoneIndex::Type::Object, "shop", 1, "old");
    idx.upsert(ZoneIndex::Type::Object, "shop", 1, "new");
    idx.markBuilt(ZoneIndex::Type::Object, "shop");
    std::vector<ZoneSummary> out;
    ASSERT_TRUE(idx.list(ZoneIndex::Type::Object, "shop", out));
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].name, "new");
}

TEST(ZoneIndex, RemoveKeepsZoneBuilt) {
    ZoneIndex idx;
    idx.upsert(ZoneIndex::Type::Object, "shop", 1, "x");
    idx.markBuilt(ZoneIndex::Type::Object, "shop");
    idx.remove(ZoneIndex::Type::Object, "shop", 1);
    std::vector<ZoneSummary> out;
    EXPECT_TRUE(idx.list(ZoneIndex::Type::Object, "shop", out));   // still built
    EXPECT_TRUE(out.empty());
}

TEST(ZoneIndex, MarkBuiltEmptyZone) {
    ZoneIndex idx;
    idx.markBuilt(ZoneIndex::Type::Monster, "empty");
    std::vector<ZoneSummary> out;
    EXPECT_TRUE(idx.list(ZoneIndex::Type::Monster, "empty", out)); // built, no entries
    EXPECT_TRUE(out.empty());
}

TEST(ZoneIndex, TypesAndZonesAreIndependent) {
    ZoneIndex idx;
    idx.markBuilt(ZoneIndex::Type::Room, "misc");
    EXPECT_TRUE(idx.isBuilt(ZoneIndex::Type::Room, "misc"));
    EXPECT_FALSE(idx.isBuilt(ZoneIndex::Type::Object, "misc"));
    EXPECT_FALSE(idx.isBuilt(ZoneIndex::Type::Room, "shop"));
}

TEST(ZoneIndex, WaitUntilBuiltImmediateAndTimeout) {
    ZoneIndex idx;
    idx.markBuilt(ZoneIndex::Type::Room, "a");
    EXPECT_TRUE(idx.waitUntilBuilt(ZoneIndex::Type::Room, "a", std::chrono::milliseconds(0)));
    EXPECT_FALSE(idx.waitUntilBuilt(ZoneIndex::Type::Room, "b", std::chrono::milliseconds(20)));
}

TEST(ZoneIndex, WaitUntilBuiltWakesOnMarkBuilt) {
    ZoneIndex idx;
    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        idx.markBuilt(ZoneIndex::Type::Room, "c");
    });
    EXPECT_TRUE(idx.waitUntilBuilt(ZoneIndex::Type::Room, "c", std::chrono::seconds(2)));
    t.join();
}

TEST(ZoneIndex, QuestTypeNameAndRoundTrip) {
    EXPECT_STREQ(ZoneIndex::typeName(ZoneIndex::Type::Quest), "quests");

    ZoneIndex idx;
    idx.upsert(ZoneIndex::Type::Quest, "misc", 7, "Taking out the trash");
    idx.markBuilt(ZoneIndex::Type::Quest, "misc");
    std::vector<ZoneSummary> out;
    ASSERT_TRUE(idx.list(ZoneIndex::Type::Quest, "misc", out));
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].id, 7);
    EXPECT_EQ(out[0].name, "Taking out the trash");
}
