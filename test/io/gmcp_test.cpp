/*
 * gmcp_test.cpp
 */
#include <gtest/gtest.h>

#include <arpa/telnet.h>   // IAC, SB, SE
#include <string>

#include "socket.hpp"      // telnet::TELOPT_GMCP, MSDP_* delimiters
#include "gmcp.hpp"
#include "gmcpEvents.hpp"  // gmcp::commTypeChannel
#include "communication.hpp" // COM_* types
#include "flags.hpp"       // P_HIDDEN, P_DM_INVIS, O_HIDDEN, O_SCENERY
#include "proto.hpp"       // roomPlayerVisible, listObjectSee
#include "mudObjects/objects.hpp"
#include "mudObjects/players.hpp"
#include "../apiTestSupport.hpp"

namespace {
    // Build MSDP byte runs with the telnet:: delimiter macros.
    std::string msdp(unsigned char b) { return std::string(1, static_cast<char>(b)); }
    std::string var(const std::string& k) { return msdp(MSDP_VAR) + k; }
    std::string val(const std::string& v) { return msdp(MSDP_VAL) + v; }
    const std::string TOPEN  = msdp(MSDP_TABLE_OPEN);
    const std::string TCLOSE = msdp(MSDP_TABLE_CLOSE);
    const std::string AOPEN  = msdp(MSDP_ARRAY_OPEN);
    const std::string ACLOSE = msdp(MSDP_ARRAY_CLOSE);
}

TEST(Gmcp, FrameWrapsAndEscapes) {
    std::string f = telnet::subnegotiate(TELOPT_GMCP, "Core.Ping");
    ASSERT_GE(f.size(), 5u);
    EXPECT_EQ(static_cast<unsigned char>(f[0]), static_cast<unsigned char>(IAC));
    EXPECT_EQ(static_cast<unsigned char>(f[1]), static_cast<unsigned char>(SB));
    EXPECT_EQ(static_cast<unsigned char>(f[2]), static_cast<unsigned char>(TELOPT_GMCP));
    EXPECT_EQ(static_cast<unsigned char>(f[f.size()-2]), static_cast<unsigned char>(IAC));
    EXPECT_EQ(static_cast<unsigned char>(f.back()), static_cast<unsigned char>(SE));
    EXPECT_EQ(f.substr(3, f.size()-5), "Core.Ping");
}

TEST(Gmcp, FrameDoublesIacInPayload) {
    std::string payload = "a";
    payload.push_back(static_cast<char>(0xFF));
    payload += "b";
    std::string f = telnet::subnegotiate(TELOPT_GMCP, payload);
    std::string mid = f.substr(3, f.size()-5);
    std::string expected = "a";
    expected.push_back(static_cast<char>(0xFF));
    expected.push_back(static_cast<char>(0xFF));
    expected += "b";
    EXPECT_EQ(mid, expected);
}

TEST(Gmcp, EscapeIacDoubles) {
    std::string in = "a";
    in.push_back(static_cast<char>(0xFF));
    in += "b";
    std::string esc = telnet::escapeIAC(in);
    std::string expected = "a";
    expected.push_back(static_cast<char>(0xFF));
    expected.push_back(static_cast<char>(0xFF));
    expected += "b";
    EXPECT_EQ(esc, expected);
    EXPECT_EQ(telnet::unescapeIAC(esc), in);
}

TEST(Gmcp, BuildCharVitals) {
    nlohmann::json body = gmcp::buildPackageBody({
        {"hp", "50", true}, {"maxhp", "100", true},
        {"mp", "20", true}, {"maxmp", "40", true},
    });
    EXPECT_TRUE(body["hp"].is_number());
    EXPECT_EQ(body["hp"], 50);
    EXPECT_EQ(body["maxhp"], 100);
    EXPECT_EQ(body["mp"], 20);
    EXPECT_EQ(body["maxmp"], 40);
}

TEST(Gmcp, BuildStringField) {
    nlohmann::json body = gmcp::buildPackageBody({{"name", "Aragorn", false}});
    EXPECT_TRUE(body["name"].is_string());
    EXPECT_EQ(body["name"], "Aragorn");
}

TEST(Gmcp, BuildNumericFallsBackToStringOnGarbage) {
    nlohmann::json body = gmcp::buildPackageBody({{"hp", "n/a", true}});
    EXPECT_TRUE(body["hp"].is_string());
    EXPECT_EQ(body["hp"], "n/a");
}

TEST(Gmcp, ParseSupportsSet) {
    auto s = gmcp::parseSupports(nlohmann::json::parse(R"(["Char 1","Room 1"])"));
    EXPECT_EQ(s, (std::set<std::string>{"Char", "Room"}));
}

TEST(Gmcp, ParseMessageSplitsPackageAndBody) {
    auto m = gmcp::parseMessage(R"(Char.Vitals {"hp":50})");
    EXPECT_EQ(m.package, "Char.Vitals");
    EXPECT_EQ(m.data["hp"], 50);

    auto p = gmcp::parseMessage("Core.Ping");
    EXPECT_EQ(p.package, "Core.Ping");
    EXPECT_TRUE(p.data.is_null());
}

TEST(Gmcp, StripSupportsVersion) {
    EXPECT_EQ(gmcp::stripSupportsVersion("Char 1"), "Char");
    EXPECT_EQ(gmcp::stripSupportsVersion("Room.Info 2"), "Room.Info");
    EXPECT_EQ(gmcp::stripSupportsVersion("Char"), "Char");
}

TEST(Gmcp, PackageForVar) {
    EXPECT_EQ(gmcp::packageForVar("HEALTH"), "Char.Vitals");
    EXPECT_EQ(gmcp::packageForVar("ROOM"), "Room.Info");
    EXPECT_TRUE(gmcp::packageForVar("SERVER_ID").empty());
}

TEST(Gmcp, MsdpTableToJson) {
    std::string t = TOPEN + var("NUM") + val("5") + var("NAME") + val("The Square") + TCLOSE;
    nlohmann::json j = gmcp::msdpTableToJson(t);
    EXPECT_EQ(j["NUM"], "5");
    EXPECT_EQ(j["NAME"], "The Square");
}

TEST(Gmcp, MsdpTableNested) {
    std::string exits = var("EXITS") + val("") + TOPEN
                        + var("north") + val("") + TOPEN + var("NUM") + val("6") + TCLOSE
                        + TCLOSE;
    std::string t = TOPEN + var("NUM") + val("5") + exits + TCLOSE;
    nlohmann::json j = gmcp::msdpTableToJson(t);
    EXPECT_EQ(j["NUM"], "5");
    EXPECT_EQ(j["EXITS"]["north"]["NUM"], "6");
}

TEST(Gmcp, MsdpArrayOfScalars) {
    std::string t = TOPEN + var("EFFECTS") + val("") + AOPEN
                    + val("Blind") + val("Poisoned") + ACLOSE + TCLOSE;
    nlohmann::json j = gmcp::msdpTableToJson(t);
    EXPECT_EQ(j["EFFECTS"], (nlohmann::json{"Blind", "Poisoned"}));
}

TEST(Gmcp, MsdpNumericKeyedMembers) {
    std::string members = var("MEMBERS") + val("") + TOPEN
                          + var("1") + val("") + TOPEN + var("NAME") + val("Bob") + TCLOSE
                          + var("2") + val("") + TOPEN + var("NAME") + val("Sue") + TCLOSE
                          + TCLOSE;
    std::string t = TOPEN + members + TCLOSE;
    nlohmann::json j = gmcp::msdpTableToJson(t);
    ASSERT_TRUE(j["MEMBERS"].is_object());
    EXPECT_EQ(j["MEMBERS"]["1"]["NAME"], "Bob");
    EXPECT_EQ(j["MEMBERS"]["2"]["NAME"], "Sue");
}

TEST(Gmcp, CharStatusVars) {
    nlohmann::json v = gmcp::charStatusVars();
    ASSERT_TRUE(v.is_object());
    EXPECT_EQ(v["wimpy"], "Wimpy");
    EXPECT_EQ(v["gold"], "Gold");
}

TEST(Gmcp, MsdpValueScalarVsTable) {
    EXPECT_EQ(gmcp::msdpValueToJson("50"), nlohmann::json("50"));
    std::string t = TOPEN + var("NUM") + val("5") + TCLOSE;
    nlohmann::json j = gmcp::msdpValueToJson(t);
    ASSERT_TRUE(j.is_object());
    EXPECT_EQ(j["NUM"], "5");
}

TEST(Gmcp, CommTypeChannelNames) {
    EXPECT_EQ(gmcp::commTypeChannel(COM_SAY), "say");
    EXPECT_EQ(gmcp::commTypeChannel(COM_TELL), "tell");
    EXPECT_EQ(gmcp::commTypeChannel(COM_YELL), "yell");
    EXPECT_EQ(gmcp::commTypeChannel(COM_GT), "group");
    EXPECT_EQ(gmcp::commTypeChannel(COM_EMOTE), "emote");
}

TEST(Gmcp, RoomPlayerVisibleHidesHiddenFromMortal) {
    auto viewer = makePlayer(CreatureClass::FIGHTER);
    auto plain  = makePlayer(CreatureClass::FIGHTER);
    auto hidden = makePlayer(CreatureClass::FIGHTER);
    hidden->setFlag(P_HIDDEN);

    EXPECT_TRUE(roomPlayerVisible(viewer, plain));
    EXPECT_FALSE(roomPlayerVisible(viewer, hidden));
    EXPECT_TRUE(roomPlayerVisible(viewer, hidden, 100));  // magical hidden-sight reveals
}

TEST(Gmcp, RoomPlayerVisibleHidesDmInvisFromMortal) {
    auto mortal = makePlayer(CreatureClass::FIGHTER);
    auto staff  = makePlayer(CreatureClass::CARETAKER);
    staff->setFlag(P_DM_INVIS);

    EXPECT_FALSE(roomPlayerVisible(mortal, staff));
    EXPECT_TRUE(roomPlayerVisible(staff, mortal));  // staff sees everyone
}

TEST(Gmcp, ListObjectSeeHidesHiddenAndScenery) {
    auto viewer  = makePlayer(CreatureClass::FIGHTER);
    auto plain   = std::make_shared<Object>();
    auto hidden  = std::make_shared<Object>();
    auto scenery = std::make_shared<Object>();
    hidden->setFlag(O_HIDDEN);
    scenery->setFlag(O_SCENERY);

    EXPECT_TRUE(listObjectSee(viewer, plain, false));
    EXPECT_FALSE(listObjectSee(viewer, hidden, false));
    EXPECT_TRUE(listObjectSee(viewer, hidden, true));  // showAll reveals hidden
    EXPECT_FALSE(listObjectSee(viewer, scenery, false));
}
