/*
 * colorWidth_test.cpp
 *   lengthNoColor / padColor: ^X color skipping + gated UTF-8 codepoint counting.
 */
#include <gtest/gtest.h>
#include <string>
#include "color.hpp"

static const std::string CAFE = "caf\xC3\xA9"; // 5 bytes, 4 glyphs

TEST(LengthNoColor, PlainAscii) {
    EXPECT_EQ(lengthNoColor("hello"), 5u);
    EXPECT_EQ(lengthNoColor("hello", true), 5u);
}

TEST(LengthNoColor, ColorCodesSkipped) {
    EXPECT_EQ(lengthNoColor("^Rred^x"), 3u);
    EXPECT_EQ(lengthNoColor("^Rred^x", true), 3u);
}

TEST(LengthNoColor, Utf8GatedCodepoints) {
    EXPECT_EQ(lengthNoColor(CAFE), 5u);
    EXPECT_EQ(lengthNoColor(CAFE, true), 4u);
}

TEST(LengthNoColor, MixedColorAndUtf8) {
    EXPECT_EQ(lengthNoColor("^R" + CAFE + "^x", true), 4u);
    EXPECT_EQ(lengthNoColor("^R" + CAFE + "^x"), 5u);
}

TEST(LengthNoColor, TruncatedMultibyte) {
    EXPECT_EQ(lengthNoColor("caf\xC3", true), 4u);
    EXPECT_EQ(lengthNoColor("\x80\x80", true), 0u);
}

TEST(PadColor, PadsToDisplayWidth) {
    EXPECT_EQ(padColor("ab", 4), "ab  ");
    EXPECT_EQ(padColor(CAFE, 6, true), CAFE + "  ");
    EXPECT_EQ(padColor(CAFE, 6, false), CAFE + " ");
    EXPECT_EQ(padColor("toolong", 3), "toolong");
}
