/*
 * viewFileReverse_test.cpp
 */
#include <gtest/gtest.h>

#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

#include "socket.hpp"
#include "login.hpp"            // CON_VIEWING_FILE_REVERSE, CON_CHOSING_WEAPONS
#include "apiTestSupport.hpp"   // makePlayer

namespace {

std::string readAvail(int fd) {
    std::string out;
    char buf[8192];
    ssize_t n;
    while ((n = ::read(fd, buf, sizeof(buf))) > 0)
        out.append(buf, static_cast<size_t>(n));
    return out;
}

// Run the first page of the reverse pager over `contents`, return the bytes sent to the client.
// `search` (may be empty) is staged in tempstr[3] exactly as the live command does.
std::string firstPage(const std::string& contents, const std::string& search) {
    const char* path = "/tmp/realms_vfr_test.txt";
    { std::ofstream f(path, std::ios::binary | std::ios::trunc); f << contents; }

    int fds[2];
    EXPECT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    nonBlock(fds[0]);
    nonBlock(fds[1]);

    auto sock = std::make_shared<Socket>(fds[0]);
    sock->setPlayer(makePlayer(CreatureClass::FIGHTER));
    snprintf(sock->tempstr[3], sizeof(sock->tempstr[3]), "%s", search.c_str());

    sock->viewFileReverse(path);            // getParam()==1: first page
    sock->setState(CON_CHOSING_WEAPONS);    // suppress the prompt in flush()
    sock->flush();
    std::string out = readAvail(fds[1]);

    sock.reset();
    ::close(fds[1]);
    ::remove(path);
    return out;
}

// 40 lines LINE000..LINE039, '\n'-separated, no trailing newline.
std::string numberedLines(const std::string& prefix = "") {
    std::string s;
    for (int i = 0; i < 40; i++) {
        if (i) s += '\n';
        char n[8];
        snprintf(n, sizeof(n), "%03d", i);
        s += prefix + "LINE" + n;
    }
    return s;
}

} // namespace

TEST(ViewFileReverse, NewestFirstAndPageBounds) {
    std::string out = firstPage(numberedLines(), "");

    // newest-first ordering
    EXPECT_NE(out.find("LINE039"), std::string::npos);
    EXPECT_LT(out.find("LINE039"), out.find("LINE038"));
    EXPECT_LT(out.find("LINE038"), out.find("LINE020"));

    // a screenful is 21 lines: LINE039..LINE019 shown, LINE018 held for the next page
    EXPECT_NE(out.find("LINE019"), std::string::npos);
    EXPECT_EQ(out.find("LINE018"), std::string::npos);
}

TEST(ViewFileReverse, OverlongSearchNoOverflow) {
    const std::string longPrefix(100, 'A');         // > the old char[80] search buffer
    std::string out = firstPage(numberedLines(longPrefix), longPrefix);

    // long lines: one chunk yields fewer than a full screen, but the newest is shown first
    // and the oldest lines are held back. The point is no overflow/crash with a >80 char search.
    EXPECT_NE(out.find("LINE039"), std::string::npos);
    EXPECT_LT(out.find("LINE039"), out.find("LINE038"));
    EXPECT_EQ(out.find("LINE000"), std::string::npos);
}
