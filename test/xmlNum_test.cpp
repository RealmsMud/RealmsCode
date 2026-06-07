/*
 * xmlNum_test.cpp
 *   xml::toNum - exception-free std::from_chars parse + gated stacktrace diagnostic.
 */
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include <gtest/gtest.h>

#include "xml.hpp"

namespace {
    // toNum frees its argument, so every call needs a fresh heap copy (never a string literal).
    char* dup(const char* s) { return s ? strdup(s) : nullptr; }
}

TEST(XmlToNum, ValidIntegrals) {
    EXPECT_EQ(xml::toNum<int>(dup("35")), 35);
    EXPECT_EQ(xml::toNum<int>(dup("-12")), -12);
    EXPECT_EQ(xml::toNum<short>(dup("17")), static_cast<short>(17));
    EXPECT_EQ(xml::toNum<unsigned short>(dup("65000")), static_cast<unsigned short>(65000));
    EXPECT_EQ(xml::toNum<long>(dup("1211421712")), 1211421712L);   // real Made timestamp
    EXPECT_EQ(xml::toNum<unsigned long>(dup("0")), 0UL);
}

TEST(XmlToNum, ValidDoubles) {
    EXPECT_DOUBLE_EQ(xml::toNum<double>(dup("14.8785")), 14.8785);
    EXPECT_DOUBLE_EQ(xml::toNum<double>(dup("3083.67")), 3083.67);
    EXPECT_DOUBLE_EQ(xml::toNum<double>(dup("0")), 0.0);
    EXPECT_DOUBLE_EQ(xml::toNum<double>(dup("-2.5")), -2.5);
}

// The real failure mode: every observed parse failure was an empty string. Core guarantee: no throw.
TEST(XmlToNum, EmptyAndNullNeverThrow) {
    EXPECT_NO_THROW({ EXPECT_EQ(xml::toNum<int>(dup("")), 0); });
    EXPECT_NO_THROW({ EXPECT_EQ(xml::toNum<short>(static_cast<char*>(nullptr)), static_cast<short>(0)); });
}

TEST(XmlToNum, EdgesAndLeniency) {
    EXPECT_EQ(xml::toNum<int>(dup(" 5")), 5);                      // leading ws trimmed
    EXPECT_EQ(xml::toNum<int>(dup(" ")), 0);                      // ws-only -> 0
    EXPECT_EQ(xml::toNum<int>(dup("abc")), 0);                    // malformed -> 0
    EXPECT_EQ(xml::toNum<short>(dup("99999")), static_cast<short>(0)); // out of short range -> 0
    EXPECT_EQ(xml::toNum<int>(dup("5x")), 5);                     // trailing garbage ignored (from_chars)
}

TEST(XmlToNum, GatedDiagnostic) {
    std::ostringstream cap;
    std::streambuf* old = std::clog.rdbuf(cap.rdbuf());
    struct Restore {
        std::streambuf* o;
        ~Restore() { std::clog.rdbuf(o); xml::logParseErrors = false; }
    } restore{old};

    xml::logParseErrors = false;
    (void)xml::toNum<int>(dup("notanumber"));
    EXPECT_TRUE(cap.str().empty());                              // silent when off

    xml::logParseErrors = true;
    (void)xml::toNum<int>(dup("notanumber"));
    const std::string out = cap.str();

    EXPECT_NE(out.find("bad numeric"), std::string::npos);       // the message
    EXPECT_NE(out.find("notanumber"), std::string::npos);        // includes the offending value
    // ...plus a stacktrace: more output than the message line alone.
    EXPECT_GT(out.size(), std::string("xml parse: bad numeric 'notanumber'\n").size());
}
