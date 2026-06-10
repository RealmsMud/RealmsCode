/*
 * roomsJson_test.cpp
 *   Shape/contract guards for the UniqueRoom JSON read-path serializer.
 */
#include <gtest/gtest.h>

#include "json.hpp"
#include "enums/loadType.hpp"
#include "location.hpp"
#include "area.hpp"
#include "mudObjects/exits.hpp"
#include "mudObjects/uniqueRooms.hpp"
#include "apiTestSupport.hpp"

using json = nlohmann::json;

TEST(RoomJson, BlankRoomHasExpectedShape) {
    ensureConfig();
    ensureServer();
    auto room = std::make_shared<UniqueRoom>();
    room->setName("Test Room");

    json j;
    to_json(j, *room);

    ASSERT_TRUE(j.is_object());
    EXPECT_EQ(j.at("name").get<std::string>(), "Test Room");
    EXPECT_TRUE(j.contains("info"));
    EXPECT_FALSE(j.contains("area"));            // dropped; info already carries area
    EXPECT_TRUE(j.at("exits").is_array());
    EXPECT_TRUE(j.contains("flags"));
}

// REGRESSION: exit "target" is now a full Location object (room CatRef + overland
// mapmarker), not a bare room CatRef. Guards against silently dropping the mapmarker.
TEST(RoomJson, ExitTargetCarriesRoomAndMapmarker) {
    ensureConfig();
    ensureServer();
    auto exit = std::make_shared<Exit>();
    exit->setName("north");
    exit->target.room = mkCr("test", 42);
    exit->target.mapmarker.set(1, 5, 6, 7);     // nonzero overland coords

    json j;
    to_json(j, *exit, LoadType::LS_FULL);

    ASSERT_TRUE(j.contains("target"));
    ASSERT_TRUE(j.at("target").is_object());
    EXPECT_TRUE(j.at("target").contains("room"));
    EXPECT_TRUE(j.at("target").contains("mapmarker"));
    // MapMarker NLOHMANN_DEFINE keys: area, x, y, z
    const json& mm = j.at("target").at("mapmarker");
    EXPECT_EQ(mm.at("area").get<int>(), 1);
    EXPECT_EQ(mm.at("x").get<int>(), 5);
    EXPECT_EQ(mm.at("y").get<int>(), 6);
    EXPECT_EQ(mm.at("z").get<int>(), 7);
}

// Authored metadata is present regardless of load mode.
TEST(RoomJson, AuthoredKeysPresentInBothModes) {
    ensureConfig();
    ensureServer();
    auto room = std::make_shared<UniqueRoom>();

    for(LoadType mode : {LoadType::LS_FULL, LoadType::LS_PROTOTYPE}) {
        json j;
        to_json(j, *room, mode);
        EXPECT_TRUE(j.contains("version"))     << "mode=" << static_cast<int>(mode);
        EXPECT_TRUE(j.contains("lastModTime")) << "mode=" << static_cast<int>(mode);
    }
}

// Runtime state keys are full-only: present for LS_FULL, absent for LS_PROTOTYPE.
TEST(RoomJson, RuntimeKeysAreFullOnly) {
    ensureConfig();
    ensureServer();
    auto room = std::make_shared<UniqueRoom>();

    json full;
    to_json(full, *room, LoadType::LS_FULL);
    ASSERT_TRUE(full.contains("lasttime"));
    EXPECT_TRUE(full.at("lasttime").is_array());
    EXPECT_EQ(full.at("lasttime").size(), 16u);
    EXPECT_TRUE(full.contains("beenHere"));
    EXPECT_TRUE(full.contains("lastPly"));
    EXPECT_TRUE(full.contains("lastPlyTime"));

    json proto;
    to_json(proto, *room, LoadType::LS_PROTOTYPE);
    EXPECT_FALSE(proto.contains("lasttime"));
    EXPECT_FALSE(proto.contains("beenHere"));
    EXPECT_FALSE(proto.contains("lastPly"));
    EXPECT_FALSE(proto.contains("lastPlyTime"));
}
