/*
 * monstersJson_test.cpp
 *   Shape/contract guards for the Monster JSON read-path serializer.
 */
#include <gtest/gtest.h>

#include "json.hpp"
#include "enums/loadType.hpp"
#include "mudObjects/monsters.hpp"
#include "apiTestSupport.hpp"

using json = nlohmann::json;

TEST(MonsterJson, BlankMonsterHasExpectedShape) {
    ensureConfig();
    ensureServer();
    auto mon = std::make_shared<Monster>();
    mon->setName("Test Mob");

    json j;
    to_json(j, *mon);

    ASSERT_TRUE(j.is_object());
    EXPECT_EQ(j.at("name").get<std::string>(), "Test Mob");
    EXPECT_TRUE(j.contains("info"));
    EXPECT_FALSE(j.contains("area"));            // dropped; info already carries area

    EXPECT_EQ(j.at("keys").size(), 3u);
    EXPECT_EQ(j.at("moveTypes").size(), 3u);
    EXPECT_EQ(j.at("proficiencies").size(), 6u);
    EXPECT_EQ(j.at("savingThrows").size(), 6u);
    EXPECT_EQ(j.at("attacks").size(), 3u);
    EXPECT_TRUE(j.at("talkResponses").is_array());
    EXPECT_TRUE(j.at("stats").contains("hp"));
}

// Authored monster keys present regardless of load mode.
static void expectAuthoredMonsterKeys(const json& j) {
    EXPECT_TRUE(j.contains("version"));

    ASSERT_TRUE(j.contains("jail"));
    EXPECT_TRUE(j.at("jail").is_object());
    EXPECT_TRUE(j.at("jail").contains("area"));
    EXPECT_TRUE(j.at("jail").contains("id"));

    EXPECT_TRUE(j.at("assistMobs").is_array());
    EXPECT_TRUE(j.at("enemyMobs").is_array());
    EXPECT_TRUE(j.at("rescue").is_array());
    EXPECT_TRUE(j.at("carry").is_array());
    EXPECT_TRUE(j.at("specialAttacks").is_array());
}

// Build a mob with one entry in each authored array, plus a version stamp.
static std::shared_ptr<Monster> makeAuthoredMob() {
    ensureConfig();
    ensureServer();
    auto mon = std::make_shared<Monster>();
    mon->setName("Authored Mob");
    mon->setVersion("9.9.9");

    mon->jail = mkCr("misc", 10);
    mon->assist_mob[0] = mkCr("misc", 11);
    mon->enemy_mob[0] = mkCr("misc", 12);
    mon->rescue[0] = mkCr("misc", 13);
    mon->carry[0].info = mkCr("misc", 14);
    mon->carry[0].numTrade = 3;
    mon->addSpecial("bash");
    return mon;
}

TEST(MonsterJson, AuthoredArraysPopulate) {
    auto mon = makeAuthoredMob();

    json j;
    to_json(j, *mon, LoadType::LS_FULL);

    expectAuthoredMonsterKeys(j);
    EXPECT_EQ(j.at("version").get<std::string>(), "9.9.9");

    EXPECT_EQ(j.at("jail").at("id").get<short>(), 10);

    ASSERT_EQ(j.at("assistMobs").size(), 1u);
    EXPECT_EQ(j.at("assistMobs").at(0).at("id").get<short>(), 11);

    ASSERT_EQ(j.at("enemyMobs").size(), 1u);
    EXPECT_EQ(j.at("enemyMobs").at(0).at("id").get<short>(), 12);

    ASSERT_EQ(j.at("rescue").size(), 1u);
    EXPECT_EQ(j.at("rescue").at(0).at("id").get<short>(), 13);

    ASSERT_EQ(j.at("carry").size(), 1u);
    EXPECT_TRUE(j.at("carry").at(0).contains("info"));
    EXPECT_TRUE(j.at("carry").at(0).contains("numTrade"));
    EXPECT_EQ(j.at("carry").at(0).at("info").at("id").get<short>(), 14);
    EXPECT_EQ(j.at("carry").at(0).at("numTrade").get<int>(), 3);

    ASSERT_EQ(j.at("specialAttacks").size(), 1u);
    EXPECT_TRUE(j.at("specialAttacks").at(0).contains("name"));
    // addSpecial() stores the canonical (capitalized) name; just confirm it serialized non-empty.
    EXPECT_FALSE(j.at("specialAttacks").at(0).at("name").get<std::string>().empty());
}

// Authored keys appear in prototype mode too; full-only keys must not.
TEST(MonsterJson, PrototypeModeHasAuthoredKeysButNoInstanceData) {
    auto mon = makeAuthoredMob();

    json j;
    to_json(j, *mon, LoadType::LS_PROTOTYPE);

    expectAuthoredMonsterKeys(j);

    EXPECT_FALSE(j.contains("id"));          // instance id omitted in prototype mode
    EXPECT_FALSE(j.contains("daily"));
    EXPECT_FALSE(j.contains("lasttime"));
    EXPECT_FALSE(j.contains("inventory"));
    EXPECT_FALSE(j.contains("equipment"));

    // authored arrays still carry their entries
    EXPECT_EQ(j.at("assistMobs").size(), 1u);
    EXPECT_EQ(j.at("specialAttacks").size(), 1u);
}

// Full mode carries the instance id plus per-instance arrays at fixed sizes.
TEST(MonsterJson, FullModeIncludesIdAndInstanceData) {
    auto mon = makeAuthoredMob();

    json j;
    to_json(j, *mon, LoadType::LS_FULL);

    EXPECT_TRUE(j.contains("id"));           // instance id present in full mode

    ASSERT_TRUE(j.contains("daily"));
    EXPECT_EQ(j.at("daily").size(), 20u);
    ASSERT_TRUE(j.contains("lasttime"));
    EXPECT_EQ(j.at("lasttime").size(), 128u);

    EXPECT_TRUE(j.at("inventory").is_array());
    EXPECT_TRUE(j.at("equipment").is_array());

    // skills/minions only emitted when non-empty; a blank mob omits them
    EXPECT_FALSE(j.contains("skills"));
    EXPECT_FALSE(j.contains("minions"));
}

// The default (no-mode) overload must match explicit LS_FULL.
TEST(MonsterJson, DefaultOverloadMatchesFull) {
    auto mon = makeAuthoredMob();

    json def, full;
    to_json(def, *mon);
    to_json(full, *mon, LoadType::LS_FULL);

    EXPECT_EQ(def, full);
}
