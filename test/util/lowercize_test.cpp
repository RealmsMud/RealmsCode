/*
 * lowercize_test.cpp
 *   lowercize() casing rules, including the empty-input guard the REST handlers rely on.
 */
#include <string>

#include <gtest/gtest.h>

#include "proto.hpp"

TEST(Lowercize, EmptyStringIsNoOp) {
    std::string s;
    lowercize(s, 1);
    EXPECT_TRUE(s.empty());
}

TEST(Lowercize, LowersAllAndCapitalizesFirst) {
    std::string s = "ABCdef";
    lowercize(s, 1);
    EXPECT_EQ(s, "Abcdef");
}

TEST(Lowercize, ZeroFlagLowersEverything) {
    std::string s = "ABCdef";
    lowercize(s, 0);
    EXPECT_EQ(s, "abcdef");
}

TEST(Lowercize, NullCStringIsNoOp) {
    char *s = nullptr;
    lowercize(s, 1);
    EXPECT_EQ(s, nullptr);
}
