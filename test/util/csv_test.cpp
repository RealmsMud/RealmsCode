/*
 * csv_test.cpp
 *   util::parseCsv - quoting, CRLF, blanks, empty fields.
 */
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "csv.hpp"

TEST(Csv, SimpleRows) {
    std::istringstream in("a,b,c\n1,2,3\n");
    auto r = util::parseCsv(in);
    ASSERT_EQ(r.size(), 2u);
    EXPECT_EQ(r[0], (std::vector<std::string>{"a", "b", "c"}));
    EXPECT_EQ(r[1][2], "3");
}

TEST(Csv, QuotedFieldWithComma) {
    std::istringstream in("88,\"Cut Away, Cut Away\",dcarnival\n");
    auto r = util::parseCsv(in);
    ASSERT_EQ(r.size(), 1u);
    ASSERT_EQ(r[0].size(), 3u);
    EXPECT_EQ(r[0][0], "88");
    EXPECT_EQ(r[0][1], "Cut Away, Cut Away");   // comma stays inside the quoted field
    EXPECT_EQ(r[0][2], "dcarnival");
}

TEST(Csv, SkipsBlankLinesAndToleratesCrlf) {
    std::istringstream in("a,b\r\n\r\n1,2\r\n");
    auto r = util::parseCsv(in);
    ASSERT_EQ(r.size(), 2u);
    EXPECT_EQ(r[0][1], "b");                     // trailing CR stripped
    EXPECT_EQ(r[1][0], "1");
}

TEST(Csv, EmptyLeadingFieldPreserved) {
    std::istringstream in(",Defeat the Shadow Lurker Lord,durgas\n");
    auto r = util::parseCsv(in);
    ASSERT_EQ(r.size(), 1u);
    ASSERT_EQ(r[0].size(), 3u);
    EXPECT_TRUE(r[0][0].empty());
    EXPECT_EQ(r[0][2], "durgas");
}
