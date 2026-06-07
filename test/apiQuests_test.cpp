/*
 * apiQuests_test.cpp
 *   JSON quest-store accessors, hermetic via the injectable base dir.
 */
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "apiQuests.hpp"

namespace fs = std::filesystem;

TEST(ApiQuests, ScanAndRead) {
    fs::path base = fs::temp_directory_path() / "realms_apiquests_test";
    fs::remove_all(base);
    fs::create_directories(base / "hp" / "quests");
    std::ofstream(base / "hp" / "quests" / "1.json") << R"({"id":{"area":"hp","id":1},"name":"Rat Extermination","disabled":false})";
    std::ofstream(base / "hp" / "quests" / "2.json") << R"({"id":{"area":"hp","id":2},"name":"Defend the Shop","disabled":true})";

    auto ids = apiScanQuestIds("hp", base);
    std::sort(ids.begin(), ids.end());
    ASSERT_EQ(ids.size(), 2u);
    EXPECT_EQ(ids[0], 1);
    EXPECT_EQ(ids[1], 2);

    auto j1 = apiReadQuestJson("hp", 1, base);
    ASSERT_TRUE(j1.has_value());
    EXPECT_EQ(j1->value("name", std::string()), "Rat Extermination");
    EXPECT_FALSE(j1->value("disabled", false));

    auto j2 = apiReadQuestJson("hp", 2, base);
    ASSERT_TRUE(j2.has_value());
    EXPECT_TRUE(j2->value("disabled", false));

    EXPECT_FALSE(apiReadQuestJson("hp", 99, base).has_value());   // missing file
    EXPECT_TRUE(apiScanQuestIds("nonexistent", base).empty());    // missing zone

    fs::remove_all(base);
}
