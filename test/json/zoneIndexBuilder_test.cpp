/*
 * zoneIndexBuilder_test.cpp
 *   The incremental builder with injected loader/scanner (no gServer/disk).
 */
#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "zoneIndex.hpp"
#include "zoneIndexBuilder.hpp"

using Type = ZoneIndex::Type;

TEST(ZoneIndexBuilder, RequestThenPumpBuilds) {
    ZoneIndex idx;
    auto scan = [](Type, const std::string&) { return std::vector<int>{1, 2, 3}; };
    auto load = [](Type, const std::string&, int id) { return "name" + std::to_string(id); };
    ZoneIndexBuilder b(idx, load, scan);

    b.request(Type::Room, "zzz");
    EXPECT_TRUE(b.hasWork());
    EXPECT_FALSE(idx.isBuilt(Type::Room, "zzz"));   // not built until pumped to completion

    b.pump(std::chrono::microseconds(1'000'000), 100);
    EXPECT_FALSE(b.hasWork());
    ASSERT_TRUE(idx.isBuilt(Type::Room, "zzz"));

    std::vector<ZoneSummary> out;
    ASSERT_TRUE(idx.list(Type::Room, "zzz", out));
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0].name, "name1");
}

TEST(ZoneIndexBuilder, RequestIsIdempotent) {
    ZoneIndex idx;
    int scans = 0;
    auto scan = [&](Type, const std::string&) { scans++; return std::vector<int>{1}; };
    auto load = [](Type, const std::string&, int) { return std::string("n"); };
    ZoneIndexBuilder b(idx, load, scan);

    b.request(Type::Object, "zzz");
    b.request(Type::Object, "zzz");                 // already queued -> no re-scan
    EXPECT_EQ(scans, 1);
}

TEST(ZoneIndexBuilder, MinCountFloorThenBudgetStop) {
    ZoneIndex idx;
    auto scan = [](Type, const std::string&) {
        std::vector<int> v;
        for(int i = 1; i <= 10; i++) v.push_back(i);
        return v;
    };
    int loads = 0;
    auto load = [&](Type, const std::string&, int) { loads++; return std::string("n"); };
    ZoneIndexBuilder b(idx, load, scan);

    b.request(Type::Monster, "zzz");
    b.pump(std::chrono::microseconds(0), 3);        // budget 0 -> exactly the minCount floor
    EXPECT_EQ(loads, 3);
    EXPECT_FALSE(idx.isBuilt(Type::Monster, "zzz"));
    EXPECT_TRUE(b.hasWork());

    b.pump(std::chrono::microseconds(1'000'000), 100);
    EXPECT_EQ(loads, 10);
    EXPECT_TRUE(idx.isBuilt(Type::Monster, "zzz"));
}

TEST(ZoneIndexBuilder, LoaderThrowDoesNotAbortBuild) {
    ZoneIndex idx;
    auto scan = [](Type, const std::string&) { return std::vector<int>{1, 2, 3}; };
    auto load = [](Type, const std::string&, int id) -> std::string {
        if(id == 2) throw std::runtime_error("bad entity");
        return "name" + std::to_string(id);
    };
    ZoneIndexBuilder b(idx, load, scan);

    b.request(Type::Room, "zzz");
    EXPECT_NO_THROW(b.pump(std::chrono::microseconds(1'000'000), 100));
    ASSERT_TRUE(idx.isBuilt(Type::Room, "zzz"));

    std::vector<ZoneSummary> out;
    ASSERT_TRUE(idx.list(Type::Room, "zzz", out));
    EXPECT_EQ(out.size(), 2u);                       // id 2 threw and was skipped
}

TEST(ZoneIndexBuilder, ScannerThrowLeavesNoWorkAndRetryable) {
    ZoneIndex idx;
    int scans = 0;
    auto scan = [&](Type, const std::string&) -> std::vector<int> {
        if(++scans == 1) throw std::runtime_error("io");
        return std::vector<int>{1};
    };
    auto load = [](Type, const std::string&, int) { return std::string("n"); };
    ZoneIndexBuilder b(idx, load, scan);

    EXPECT_NO_THROW(b.request(Type::Room, "zzz"));    // first scan throws
    EXPECT_FALSE(b.hasWork());
    EXPECT_FALSE(idx.isBuilt(Type::Room, "zzz"));

    b.request(Type::Room, "zzz");                     // reservation cleared -> rescans
    EXPECT_EQ(scans, 2);
    EXPECT_TRUE(b.hasWork());
}

TEST(ZoneIndexBuilder, EmptyZoneBuiltImmediately) {
    ZoneIndex idx;
    auto scan = [](Type, const std::string&) { return std::vector<int>{}; };
    auto load = [](Type, const std::string&, int) { return std::string(); };
    ZoneIndexBuilder b(idx, load, scan);

    b.request(Type::Room, "zzz_empty");
    EXPECT_FALSE(b.hasWork());                       // empty -> marked built, nothing queued
    EXPECT_TRUE(idx.isBuilt(Type::Room, "zzz_empty"));
}

TEST(ZoneIndexBuilder, QuestTypeBuilds) {
    ZoneIndex idx;
    auto scan = [](Type t, const std::string&) {
        return t == Type::Quest ? std::vector<int>{1, 5} : std::vector<int>{};
    };
    auto load = [](Type, const std::string&, int id) { return "quest" + std::to_string(id); };
    ZoneIndexBuilder b(idx, load, scan);

    // throwaway zone with no on-disk index file, so request() uses the injected scanner
    b.request(Type::Quest, "zzz");
    b.pump(std::chrono::microseconds(1'000'000), 100);
    ASSERT_TRUE(idx.isBuilt(Type::Quest, "zzz"));

    std::vector<ZoneSummary> out;
    ASSERT_TRUE(idx.list(Type::Quest, "zzz", out));
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].name, "quest1");
}
