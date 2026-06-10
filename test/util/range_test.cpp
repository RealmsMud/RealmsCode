/*
 * range_test.cpp
 *   Unit tests for builder-range membership (the core of API authz range checks).
 */
#include <gtest/gtest.h>

#include "catRef.hpp"
#include "range.hpp"

static CatRef makeCr(const char* area, short id) {
    std::string a = area;
    return CatRef(a, id);
}

static Range makeRange(const char* area, short low, short high) {
    Range r;
    r.low.setArea(area);
    r.low.id = low;
    r.high = high;
    return r;
}

TEST(Range, InsideInclusiveBounds) {
    Range r = makeRange("misc", 100, 200);
    EXPECT_TRUE(r.belongs(makeCr("misc", 100)));   // low edge
    EXPECT_TRUE(r.belongs(makeCr("misc", 150)));
    EXPECT_TRUE(r.belongs(makeCr("misc", 200)));   // high edge
}

TEST(Range, OutsideBounds) {
    Range r = makeRange("misc", 100, 200);
    EXPECT_FALSE(r.belongs(makeCr("misc", 99)));
    EXPECT_FALSE(r.belongs(makeCr("misc", 201)));
}

TEST(Range, WrongArea) {
    Range r = makeRange("misc", 100, 200);
    EXPECT_FALSE(r.belongs(makeCr("other", 150)));
}

TEST(Range, WildcardWholeArea) {
    Range r = makeRange("misc", -1, -1);
    EXPECT_TRUE(r.belongs(makeCr("misc", 1)));
    EXPECT_TRUE(r.belongs(makeCr("misc", 32000)));
    EXPECT_FALSE(r.belongs(makeCr("other", 1)));
}
