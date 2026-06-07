/*
 * xmlReader_test.cpp
 *   xml::readRootChildText - streaming root-child read (depth-1 only, early exit).
 */
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include "xml.hpp"

namespace fs = std::filesystem;

namespace {
    // Writes content to a uniquely-named temp file; removes it on destruction.
    struct TempXml {
        fs::path path;
        TempXml(const std::string& tag, const std::string& content) {
            path = fs::temp_directory_path() / ("realms_xmlreader_" + tag + ".xml");
            std::ofstream(path) << content;
        }
        ~TempXml() { std::error_code ec; fs::remove(path, ec); }
    };
}

TEST(XmlReadRootChildText, ReturnsRootNameEarlyExit) {
    TempXml f("nested",
        "<Object><Name>griffon key</Name>"
        "<SubItems><Object><Name>nested junk</Name></Object></SubItems></Object>");
    EXPECT_EQ(xml::readRootChildText(f.path, "Object", "Name"), "griffon key");
}

// A <Name> nested below depth 1 must NOT be picked up (the depth-1 guard).
TEST(XmlReadRootChildText, IgnoresNestedNameWhenNoDirectChild) {
    TempXml f("onlynested",
        "<Object><Flags>3</Flags>"
        "<SubItems><Object><Name>nested junk</Name></Object></SubItems></Object>");
    EXPECT_EQ(xml::readRootChildText(f.path, "Object", "Name"), "");
}

TEST(XmlReadRootChildText, CreatureRoot) {
    TempXml f("creature", "<Creature><Name>Captain Ennsok</Name></Creature>");
    EXPECT_EQ(xml::readRootChildText(f.path, "Creature", "Name"), "Captain Ennsok");
}

TEST(XmlReadRootChildText, RoomRoot) {
    TempXml f("room", "<Room><Name>Dangling Ladder</Name></Room>");
    EXPECT_EQ(xml::readRootChildText(f.path, "Room", "Name"), "Dangling Ladder");
}

TEST(XmlReadRootChildText, WrongRootReturnsEmpty) {
    TempXml f("wrongroot", "<Room><Name>Dangling Ladder</Name></Room>");
    EXPECT_EQ(xml::readRootChildText(f.path, "Object", "Name"), "");
}

TEST(XmlReadRootChildText, MissingChildReturnsEmpty) {
    TempXml f("nochild", "<Object><Flags>3</Flags></Object>");
    EXPECT_EQ(xml::readRootChildText(f.path, "Object", "Name"), "");
}

TEST(XmlReadRootChildText, EmptyChildReturnsEmpty) {
    TempXml f("emptychild", "<Object><Name/></Object>");
    EXPECT_EQ(xml::readRootChildText(f.path, "Object", "Name"), "");
}

TEST(XmlReadRootChildText, NonexistentFileReturnsEmpty) {
    fs::path missing = fs::temp_directory_path() / "realms_xmlreader_does_not_exist_zzz.xml";
    std::error_code ec; fs::remove(missing, ec);
    EXPECT_EQ(xml::readRootChildText(missing, "Object", "Name"), "");
}
