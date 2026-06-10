/*
 * questRemap_test.cpp
 *   Dense per-zone renumbering used to relocate the JSON quest store out of misc.
 */
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "catRef.hpp"
#include "quests.hpp"

TEST(QuestRemap, DensePerZoneFromOneByOldId) {
    std::vector<std::pair<CatRef, std::string>> in = {
        {CatRef("misc", 2), "hp"},
        {CatRef("misc", 5), "druid"},
        {CatRef("misc", 1), "misc"},
        {CatRef("misc", 9), "hp"},
        {CatRef("misc", 4), "misc"},
    };
    auto m = buildQuestRemap(in);
    ASSERT_EQ(m.size(), 5u);

    // hp: old 2,9 -> hp.1, hp.2 (ascending old id)
    EXPECT_EQ(m.at(CatRef("misc", 2)).area, "hp");
    EXPECT_EQ(m.at(CatRef("misc", 2)).id, 1);
    EXPECT_EQ(m.at(CatRef("misc", 9)).area, "hp");
    EXPECT_EQ(m.at(CatRef("misc", 9)).id, 2);

    // druid: single -> druid.1
    EXPECT_EQ(m.at(CatRef("misc", 5)).area, "druid");
    EXPECT_EQ(m.at(CatRef("misc", 5)).id, 1);

    // misc is renumbered like any other zone -> misc.1, misc.2
    EXPECT_EQ(m.at(CatRef("misc", 1)).area, "misc");
    EXPECT_EQ(m.at(CatRef("misc", 1)).id, 1);
    EXPECT_EQ(m.at(CatRef("misc", 4)).area, "misc");
    EXPECT_EQ(m.at(CatRef("misc", 4)).id, 2);
}

TEST(QuestRemap, ZonesNumberIndependentlyFromOne) {
    std::vector<std::pair<CatRef, std::string>> in = {
        {CatRef("misc", 100), "a"},
        {CatRef("misc", 1), "b"},
    };
    auto m = buildQuestRemap(in);
    EXPECT_EQ(m.at(CatRef("misc", 100)).area, "a");
    EXPECT_EQ(m.at(CatRef("misc", 100)).id, 1);
    EXPECT_EQ(m.at(CatRef("misc", 1)).area, "b");
    EXPECT_EQ(m.at(CatRef("misc", 1)).id, 1);
}
