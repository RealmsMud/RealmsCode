/*
 * processInput_test.cpp
 */
#include <gtest/gtest.h>

#include <arpa/telnet.h>   // IAC, SB, SE, WILL, TELQUAL_IS
#include <span>
#include <string>
#include <vector>

#include "socket.hpp"
#include "apiTestSupport.hpp"

namespace {

using TS = Socket::TelnetState;

// Exposes the protected FSM seams + state for assertions. No fd, no I/O.
struct TestSocket : public Socket {
    TestSocket() : Socket(-1) {}
    using Socket::decodeBytes;
    using Socket::extractCommands;
    using Socket::tState;
    using Socket::inBuf;
    using Socket::input;
    using Socket::opts;

    std::string decode(const std::vector<unsigned char>& v) {
        std::string out;
        decodeBytes(std::span<const unsigned char>(v.data(), v.size()), out);
        return out;
    }
    // Full pipeline: decode then queue completed lines.
    void feed(const std::vector<unsigned char>& v) {
        extractCommands(decode(v));
    }
    std::vector<std::string> drain() {
        std::vector<std::string> r;
        while (!input.empty()) { r.push_back(input.front()); input.pop(); }
        return r;
    }
};

std::vector<unsigned char> B(std::initializer_list<int> bytes) {
    std::vector<unsigned char> v;
    v.reserve(bytes.size());
    for (int b : bytes) v.push_back(static_cast<unsigned char>(b));
    return v;
}

std::vector<unsigned char> B(std::string_view s) {
    return {s.begin(), s.end()};
}

class ProcessInput : public ::testing::Test {
protected:
    void SetUp() override { ensureConfig(); ensureServer(); }
    TestSocket sock;
};

TEST_F(ProcessInput, PlainTextQueuesCommand) {
    sock.feed(B("say hello\n"));
    EXPECT_EQ(sock.drain(), (std::vector<std::string>{"say hello"}));
}

TEST_F(ProcessInput, PlainTextWithoutNewlineStaysBuffered) {
    sock.feed(B("partial"));
    EXPECT_TRUE(sock.drain().empty());
    EXPECT_EQ(sock.inBuf, "partial");
}

TEST_F(ProcessInput, CrLfCollapsesToOneCommand) {
    sock.feed(B("hi\r\n"));
    EXPECT_EQ(sock.drain(), (std::vector<std::string>{"hi"}));
}

TEST_F(ProcessInput, LoneCrBecomesNewline) {
    sock.feed(B("hi\r"));
    EXPECT_EQ(sock.drain(), (std::vector<std::string>{"hi"}));
}

TEST_F(ProcessInput, LoneLf) {
    sock.feed(B("hi\n"));
    EXPECT_EQ(sock.drain(), (std::vector<std::string>{"hi"}));
}

TEST_F(ProcessInput, BackspaceErasesPrecedingChar) {
    sock.feed(B("abc\b\n"));
    EXPECT_EQ(sock.drain(), (std::vector<std::string>{"ab"}));
}

TEST_F(ProcessInput, DelActsAsBackspace) {
    sock.feed(B({'a', 'b', 'c', 127, '\n'}));
    EXPECT_EQ(sock.drain(), (std::vector<std::string>{"ab"}));
}

// Regression: a line starting with a backspace used to read past the end of the
// buffer (stale length bound) and throw std::out_of_range out of processInput.
TEST_F(ProcessInput, LeadingBackspaceDoesNotOverread) {
    sock.feed(B("\bab\n"));
    EXPECT_EQ(sock.drain(), (std::vector<std::string>{"ab"}));
}

TEST_F(ProcessInput, LeadingBackspaceOnlyYieldsEmptyCommand) {
    sock.feed(B("\b\n"));
    EXPECT_EQ(sock.drain(), (std::vector<std::string>{""}));
}

// Backspace at the start of a read erases the tail of a previously buffered read.
TEST_F(ProcessInput, BackspaceReachesIntoPriorRead) {
    sock.feed(B("ab"));            // buffered, no newline yet
    ASSERT_TRUE(sock.drain().empty());
    sock.feed(B("\b\n"));          // backspace deletes the buffered 'b'
    EXPECT_EQ(sock.drain(), (std::vector<std::string>{"a"}));
}

TEST_F(ProcessInput, DoubledIacEmitsSingleByte) {
    const std::string out = sock.decode(B({IAC, IAC}));
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(static_cast<unsigned char>(out[0]), 255u);
    EXPECT_EQ(sock.tState, TS::NEG_NONE);
}

TEST_F(ProcessInput, ClientGoAheadSentinelDropped) {
    EXPECT_EQ(sock.decode(B({CH_GO_AHEAD, 'h', 'i'})), "hi");
}

// The fix: these used to emit the byte's DECIMAL value (fmt maps unsigned char to
// unsigned), e.g. ESC [ A -> "\033[65". They must now pass the raw byte through.
TEST_F(ProcessInput, MxpEscapeBareEscPassthrough) {
    EXPECT_EQ(sock.decode(B({0x1b, 'X'})), std::string("\033X"));
}

TEST_F(ProcessInput, MxpEscapeCsiPassthrough) {
    EXPECT_EQ(sock.decode(B({0x1b, '[', 'A'})), std::string("\033[A"));
}

TEST_F(ProcessInput, MxpEscapeCsiOnePassthrough) {
    EXPECT_EQ(sock.decode(B({0x1b, '[', '1', 'X'})), std::string("\033[1X"));
}

TEST_F(ProcessInput, MxpSecureHandshakeEnablesMode) {
    // Stop before the terminating newline so parseMXPSecure() is not invoked.
    sock.decode(B({0x1b, '[', '1', 'z'}));
    EXPECT_TRUE(sock.getMxpClientSecure());
    EXPECT_EQ(sock.tState, TS::NEG_MXP_SECURE_CONSUME);
}

TEST_F(ProcessInput, NawsSetsTerminalSize) {
    sock.feed(B({IAC, SB, NAWS, 0, 80, 0, 24, IAC, SE}));
    EXPECT_EQ(sock.getTermCols(), 80);
    EXPECT_EQ(sock.getTermRows(), 24);
    EXPECT_TRUE(sock.drain().empty());
}

TEST_F(ProcessInput, Naws255RowsHandlesStraySe) {
    // rows == 255 arms the broken-client stray-SE guard; must not produce input.
    sock.feed(B({IAC, SB, NAWS, 0, 80, 0, 255, IAC, SE}));
    EXPECT_EQ(sock.getTermRows(), 255);
    EXPECT_TRUE(sock.drain().empty());
}

TEST_F(ProcessInput, TtypeMttsAppliesCapabilities) {
    std::vector<unsigned char> seq = B({IAC, SB, TTYPE, TELQUAL_IS});
    for (unsigned char c : B("MTTS 13")) seq.push_back(c);   // ANSI|UTF8|256COLOR
    for (unsigned char c : B({IAC, SE})) seq.push_back(c);
    sock.feed(seq);
    EXPECT_EQ(sock.getMtts(), 13);
    EXPECT_TRUE(sock.utf8Enabled());
    EXPECT_TRUE(sock.opts.xterm256);
    EXPECT_EQ(sock.getTermType(), "MTTS 13");
}

TEST_F(ProcessInput, Ttype256ColorTermSetsXterm256) {
    std::vector<unsigned char> seq = B({IAC, SB, TTYPE, TELQUAL_IS});
    for (unsigned char c : B("xterm-256color")) seq.push_back(c);
    for (unsigned char c : B({IAC, SE})) seq.push_back(c);
    sock.feed(seq);
    EXPECT_TRUE(sock.opts.xterm256);
}

TEST_F(ProcessInput, DoubledIacSplitAcrossReads) {
    EXPECT_EQ(sock.decode(B({IAC})), "");
    EXPECT_EQ(sock.tState, TS::NEG_IAC);
    const std::string out = sock.decode(B({IAC}));   // completes the doubled IAC
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(static_cast<unsigned char>(out[0]), 255u);
    EXPECT_EQ(sock.tState, TS::NEG_NONE);
}

TEST_F(ProcessInput, TextSplitAcrossReads) {
    sock.feed(B("ab"));
    sock.feed(B("c\n"));
    EXPECT_EQ(sock.drain(), (std::vector<std::string>{"abc"}));
}

TEST_F(ProcessInput, PureNegotiationProducesNoText) {
    sock.feed(B({IAC, WILL, NAWS}));
    EXPECT_TRUE(sock.drain().empty());
    EXPECT_TRUE(sock.inBuf.empty());
    EXPECT_TRUE(sock.nawsEnabled());
}

} // namespace
