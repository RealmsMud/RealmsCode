/*
 * protocol_test.cpp
 *   Telnet/OOB protocol unit tests: telnet-strip OOB safety (skipTelnetSeq),
 *   parseForOutput 0xFF doubling + trailing-caret guard, escapeIAC, and the
 *   pure MSSP payload builder.
 */
#include <gtest/gtest.h>

#include <arpa/telnet.h>   // IAC, SB, SE, WILL
#include <string>

#include "socket.hpp"

// A buffer ending on a bare IAC must not read inStr[size()] (string_view OOB).
TEST(StripTelnet, TrailingBareIacDoesNotOverread) {
    std::string buf = "hi";
    buf.push_back(static_cast<char>(IAC));      // dangling IAC, no following byte
    EXPECT_EQ(Socket::stripTelnet(buf), "hi");  // partial negotiation dropped, text kept
}

TEST(StripTelnet, UnterminatedSbDoesNotOverread) {
    std::string buf = "x";
    buf.push_back(static_cast<char>(IAC));
    buf.push_back(static_cast<char>(SB));
    buf.push_back(static_cast<char>(TELOPT_MSDP));  // SB opened, never closed
    EXPECT_EQ(Socket::stripTelnet(buf), "x");
}

TEST(StripTelnet, CompleteNegotiationStripped) {
    std::string buf = "a";
    buf.push_back(static_cast<char>(IAC));
    buf.push_back(static_cast<char>(WILL));
    buf.push_back(static_cast<char>(TELOPT_MSDP));
    buf += "b";
    EXPECT_EQ(Socket::stripTelnet(buf), "ab");
}

TEST(NeedsPrompt, TrailingBareIacDoesNotOverread) {
    std::string buf;
    buf.push_back(static_cast<char>(IAC));       // only a dangling IAC
    EXPECT_FALSE(Socket::needsPrompt(buf));      // nothing but (incomplete) telnet
}

TEST(NeedsPrompt, PlainTextNeedsPrompt) {
    EXPECT_TRUE(Socket::needsPrompt(std::string("hello")));
}

// parseForOutput is a member; a default-reset Socket(-1) is global-free.
TEST(ParseForOutput, LiteralFFIsDoubled) {
    Socket sock(-1);
    std::string in = "a";
    in.push_back(static_cast<char>(0xFF));       // literal IAC byte in output
    in += "b";
    std::string expected = "a";
    expected.push_back(static_cast<char>(0xFF));
    expected.push_back(static_cast<char>(0xFF)); // doubled
    expected += "b";
    EXPECT_EQ(sock.parseForOutput(in), expected);
}

TEST(ParseForOutput, TrailingCaretDoesNotOverread) {
    Socket sock(-1);
    EXPECT_EQ(sock.parseForOutput(std::string("hi^")), "hi");  // dangling caret dropped, no over-read
}

TEST(ParseForOutput, NewlineBecomesCrlf) {
    Socket sock(-1);
    EXPECT_EQ(sock.parseForOutput(std::string("a\nb")), "a\r\nb");
}

// promptGoAhead selects the raw marker (single IAC, never doubled).
TEST(PromptGoAhead, GaForNegotiatedClient) {
    std::string out = telnet::promptGoAhead(/*eor*/false, /*dumb*/false);
    std::string expected;
    expected.push_back(static_cast<char>(IAC));
    expected.push_back(static_cast<char>(GA));
    EXPECT_EQ(out, expected);
}

TEST(PromptGoAhead, EorWhenEnabled) {
    std::string out = telnet::promptGoAhead(/*eor*/true, /*dumb*/false);
    std::string expected;
    expected.push_back(static_cast<char>(IAC));
    expected.push_back(static_cast<char>(EOR));
    EXPECT_EQ(out, expected);
}

TEST(PromptGoAhead, EmptyForDumbClient) {
    EXPECT_EQ(telnet::promptGoAhead(/*eor*/false, /*dumb*/true), "");
}

// parseForOutput recognizes the sentinel and (default reset state: dumb) drops it.
TEST(ParseForOutput, GoAheadDumbEmitsNothing) {
    Socket sock(-1);                                 // reset(): dumb=true, eor=false
    EXPECT_EQ(sock.parseForOutput(std::string("hi") + GO_AHEAD), "hi");
}

TEST(EscapeIAC, DoublesFF) {
    std::string in = "a";
    in.push_back(static_cast<char>(0xFF));
    in += "b";
    std::string out = "a";
    out.push_back(static_cast<char>(0xFF));
    out.push_back(static_cast<char>(0xFF));
    out += "b";
    EXPECT_EQ(telnet::escapeIAC(in), out);
}

TEST(EscapeIAC, NoFFUnchanged) {
    EXPECT_EQ(telnet::escapeIAC(std::string("plain")), "plain");
}

namespace {
    // Assert exact "var"/"val" byte runs inside the MSSP payload.
    std::string msspVar(const std::string& name) {
        return std::string(1, static_cast<char>(MSSP_VAR)) + name;
    }
    std::string msspVal(const std::string& v) {
        return std::string(1, static_cast<char>(MSSP_VAL)) + v;
    }
}

TEST(Mssp, PayloadReportsConfiguredPortOnly) {
    std::string p = telnet::buildMsspPayload(/*players*/3, /*uptime*/1700000000L,
                                             /*port*/3333, /*classes*/10,
                                             /*races*/12, /*skills*/50);
    EXPECT_NE(p.find(msspVar("PORT") + msspVal("3333")), std::string::npos);
    // The old hardcoded telnet port 23 is gone. ("3333" starts with '3', so it
    // cannot match MSSP_VAL + "23".)
    EXPECT_EQ(p.find(msspVal("23")), std::string::npos);
}

TEST(Mssp, PayloadRetainsMspAdvertisement) {
    std::string p = telnet::buildMsspPayload(0, 1L, 3333, 1, 1, 1);
    EXPECT_NE(p.find(msspVar("MSP") + msspVal("1")), std::string::npos);
    EXPECT_EQ(p.find(msspVar("MSP") + msspVal("0")), std::string::npos);
}

TEST(Mssp, PayloadCarriesRuntimeValues) {
    std::string p = telnet::buildMsspPayload(7, 1700000000L, 3333, 9, 14, 88);
    EXPECT_NE(p.find(msspVar("NAME") + msspVal("The Realms of Hell")), std::string::npos);
    EXPECT_NE(p.find(msspVar("PLAYERS") + msspVal("7")), std::string::npos);
    EXPECT_NE(p.find(msspVar("UPTIME") + msspVal("1700000000")), std::string::npos);
    EXPECT_NE(p.find(msspVar("CLASSES") + msspVal("9")), std::string::npos);
    EXPECT_NE(p.find(msspVar("RACES") + msspVal("14")), std::string::npos);
    EXPECT_NE(p.find(msspVar("SKILLS") + msspVal("88")), std::string::npos);
}

TEST(Mssp, PayloadIsWrappedInSubnegotiation) {
    std::string p = telnet::buildMsspPayload(0, 1L, 3333, 1, 1, 1);
    ASSERT_GE(p.size(), 5u);
    EXPECT_EQ(static_cast<unsigned char>(p.front()), static_cast<unsigned char>(IAC));
    EXPECT_EQ(static_cast<unsigned char>(p[1]),      static_cast<unsigned char>(SB));
    EXPECT_EQ(static_cast<unsigned char>(p[p.size()-2]), static_cast<unsigned char>(IAC));
    EXPECT_EQ(static_cast<unsigned char>(p.back()),      static_cast<unsigned char>(SE));
}

TEST(ParseMtts, ExtractsBits) {
    EXPECT_EQ(telnet::parseMtts("MTTS 13"), 13);
    EXPECT_EQ(telnet::parseMtts("MTTS 4"), MTTS_UTF8);
}

TEST(ParseMtts, BitTests) {
    long b = telnet::parseMtts("MTTS 13");
    EXPECT_TRUE(b & MTTS_ANSI);
    EXPECT_TRUE(b & MTTS_UTF8);
    EXPECT_TRUE(b & MTTS_256COLOR);
    EXPECT_FALSE(b & MTTS_TRUECOLOR);
}

TEST(ParseMtts, RejectsNonMtts) {
    EXPECT_EQ(telnet::parseMtts("xterm-256color"), 0);
    EXPECT_EQ(telnet::parseMtts("MTTS"), 0);
    EXPECT_EQ(telnet::parseMtts("MTTS abc"), 0);
    EXPECT_EQ(telnet::parseMtts(""), 0);
}

TEST(MttsCaps, DecodesSetBits) {
    EXPECT_EQ(telnet::mttsCaps(MTTS_ANSI | MTTS_UTF8 | MTTS_256COLOR), "ANSI UTF8 256COLOR");
    EXPECT_EQ(telnet::mttsCaps(0), "");
    EXPECT_EQ(telnet::mttsCaps(MTTS_UTF8), "UTF8");
}

TEST(CharsetFrame, RequestBytesExact) {
    const unsigned char expected[] = {
        IAC, SB, TELOPT_CHARSET, 1, ' ', 'U', 'T', 'F', '-', '8', IAC, SE, '\0'
    };
    for (size_t i = 0; i < sizeof(expected); i++)
        EXPECT_EQ(telnet::charset_utf8[i], expected[i]) << "byte " << i;
}

TEST(NewEnvironFrame, SendAllBytesExact) {
    const unsigned char expected[] = { IAC, SB, TELOPT_NEW_ENVIRON, TELQUAL_SEND, IAC, SE, '\0' };
    for (size_t i = 0; i < sizeof(expected); i++)
        EXPECT_EQ(telnet::sb_new_environ_send[i], expected[i]) << "byte " << i;
}

TEST(SkipTelnetSeq, DanglingIacConsumesToEnd) {
    std::string buf = "x";
    buf.push_back(static_cast<char>(IAC));           // IAC at index 1, nothing after
    EXPECT_EQ(Socket::skipTelnetSeq(buf, 1), buf.size());
}

TEST(SkipTelnetSeq, OptionCommandIsThreeBytes) {
    std::string buf;
    buf.push_back(static_cast<char>(IAC));
    buf.push_back(static_cast<char>(WILL));
    buf.push_back(static_cast<char>(TELOPT_MSDP));
    buf += "tail";
    EXPECT_EQ(Socket::skipTelnetSeq(buf, 0), 3u);     // past IAC WILL <opt>
}

TEST(SkipTelnetSeq, SubnegotiationRunsPastSe) {
    std::string buf;
    buf.push_back(static_cast<char>(IAC));
    buf.push_back(static_cast<char>(SB));
    buf.push_back(static_cast<char>(TELOPT_MSDP));
    buf += "data";
    buf.push_back(static_cast<char>(IAC));
    buf.push_back(static_cast<char>(SE));
    buf += "rest";
    EXPECT_EQ(Socket::skipTelnetSeq(buf, 0), buf.size() - 4);  // index of 'r' in "rest"
}

TEST(SkipTelnetSeq, UnterminatedSubnegotiationConsumesToEnd) {
    std::string buf;
    buf.push_back(static_cast<char>(IAC));
    buf.push_back(static_cast<char>(SB));
    buf.push_back(static_cast<char>(TELOPT_MSDP));
    buf += "never closed";
    EXPECT_EQ(Socket::skipTelnetSeq(buf, 0), buf.size());
}

TEST(NewEnviron, DecodesMnesUservars) {
    std::vector<unsigned char> sb = { TELQUAL_IS };
    auto put = [&](unsigned char tag, const std::string& s) {
        sb.push_back(tag);
        for (char c : s) sb.push_back(static_cast<unsigned char>(c));
    };
    put(ENV_USERVAR, "CLIENT_NAME"); put(NEW_ENV_VALUE, "MUDLET");
    put(ENV_USERVAR, "MTTS");        put(NEW_ENV_VALUE, "271");
    auto m = telnet::decodeNewEnviron(sb);
    EXPECT_EQ(m["CLIENT_NAME"], "MUDLET");
    EXPECT_EQ(m["MTTS"], "271");
    EXPECT_EQ(telnet::parseMtts("MTTS " + m["MTTS"]), 271);
}
