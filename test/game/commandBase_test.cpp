/*
 * commandBase_test.cpp
 *   Unit tests for parse() -- the command tokenizer. Regression: parse() read str[length]
 *   as a NUL terminator, an over-read that aborts under _GLIBCXX_ASSERTIONS.
 */
#include <gtest/gtest.h>

#include <cstring>
#include <random>
#include <string>
#include <string_view>

#include "cmd.hpp"

// Declared in commands.hpp; redeclared here to keep this translation unit light.
void parse(std::string_view str, cmd* cmnd);

static cmd doParse(std::string_view in) {
    cmd c;
    c.fullstr = std::string(in);
    parse(in, &c);
    return c;
}

// Inputs reaching end-of-view without a trailing space used to over-read.
TEST(Parse, ShortAndEdgeInputsDoNotAbort) {
    const char* inputs[] = {
        "", " ", "  ", "*u", "*U", "x", "a b", " lead", "trail ",
        "\"", "\"abc", "\"abc\"", "-", "12", "look", "\"a b\" c", "...",
    };
    for (const char* s : inputs) {
        cmd c = doParse(s);
        (void)c;
    }
    SUCCEED();  // reaching here means none of the above aborted
}

TEST(Parse, EmptyYieldsNoTokens) {
    cmd c = doParse("");
    EXPECT_EQ(c.num, 0);
}

TEST(Parse, SingleToken) {
    cmd c = doParse("look");
    EXPECT_EQ(c.num, 1);
    EXPECT_STREQ(c.str[0], "look");
}

TEST(Parse, TwoWords) {
    cmd c = doParse("get sword");
    EXPECT_EQ(c.num, 2);
    EXPECT_STREQ(c.str[0], "get");
    EXPECT_STREQ(c.str[1], "sword");
}

// A numeric token attaches as the value of the previous word rather than a new token.
TEST(Parse, NumericValueAttachesToPrevious) {
    cmd c = doParse("buy 3");
    EXPECT_EQ(c.num, 1);
    EXPECT_STREQ(c.str[0], "buy");
    EXPECT_EQ(c.val[0], 3);
}

TEST(Parse, LeadingAndTrailingSpaces) {
    cmd c = doParse("  kill   rat  ");
    EXPECT_EQ(c.num, 2);
    EXPECT_STREQ(c.str[0], "kill");
    EXPECT_STREQ(c.str[1], "rat");
}

TEST(Parse, LongTokenTruncatedNotOverflowed) {
    std::string longtok(200, 'a');
    cmd c = doParse(longtok);
    EXPECT_EQ(c.num, 1);
    EXPECT_LT(strlen(c.str[0]), static_cast<size_t>(MAX_TOKEN_SIZE));
}

TEST(Parse, TokenCountCappedAtCommandMax) {
    cmd c = doParse("a b c d e f g h i j k l");
    EXPECT_LE(c.num, COMMANDMAX);
}

// Fuzz: random inputs biased toward tokenizer-significant chars must not abort.
TEST(Parse, FuzzRandomInputsDoNotAbort) {
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int> lenDist(0, 80);
    const std::string alphabet = " \t\"-0123456789abcXYZ";
    std::uniform_int_distribution<size_t> chDist(0, alphabet.size() - 1);

    for (int iter = 0; iter < 5000; ++iter) {
        int len = lenDist(rng);
        std::string s;
        s.reserve(len);
        for (int k = 0; k < len; ++k)
            s += alphabet[chDist(rng)];
        cmd c = doParse(s);
        (void)c;
    }
    SUCCEED();
}
