/*
 * md5_test.cpp
 */
#include <string>

#include <gtest/gtest.h>

#include "md5.hpp"

TEST(Md5, Rfc1321Vectors) {
    md5wrapper md5;
    EXPECT_EQ(md5.getHashFromString(""), "d41d8cd98f00b204e9800998ecf8427e");
    EXPECT_EQ(md5.getHashFromString("abc"), "900150983cd24fb0d6963f7d28e17f72");
    EXPECT_EQ(md5.getHashFromString("message digest"), "f96b697d7cb7938d525a2f31aaf161d0");
    EXPECT_EQ(md5.getHashFromString("abcdefghijklmnopqrstuvwxyz"), "c3fcd3d76192e4007dfb496cca67e13b");
}

TEST(Md5, AlwaysThirtyTwoHexChars) {
    md5wrapper md5;
    const std::string h = md5.getHashFromString("the quick brown fox");
    EXPECT_EQ(h.size(), 32u);
    EXPECT_EQ(h.find_first_not_of("0123456789abcdef"), std::string::npos);
}
