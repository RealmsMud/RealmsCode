/*
 * mccp_test.cpp
 */
#include <gtest/gtest.h>

#include <arpa/telnet.h>   // IAC, DO, TELOPT_COMPRESS2
#include <sys/socket.h>
#include <unistd.h>
#include <zlib.h>

#include <cstring>
#include <memory>
#include <random>
#include <string>

#include "socket.hpp"
#include "apiTestSupport.hpp"   // ensureServer

namespace {

// Read everything currently available from a non-blocking fd.
std::string readAvail(int fd) {
    std::string out;
    char buf[8192];
    ssize_t n;
    while ((n = ::read(fd, buf, sizeof(buf))) > 0)
        out.append(buf, static_cast<size_t>(n));
    return out;
}

// Inflate a whole MCCP2 stream (deflate w/ Z_SYNC_FLUSH, default window).
std::string inflateAll(const std::string& in) {
    z_stream zs{};
    EXPECT_EQ(inflateInit(&zs), Z_OK);
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(in.data()));
    zs.avail_in = static_cast<uInt>(in.size());
    std::string out;
    char buf[16384];
    int ret = Z_OK;
    do {
        zs.next_out = reinterpret_cast<Bytef*>(buf);
        zs.avail_out = sizeof(buf);
        ret = inflate(&zs, Z_NO_FLUSH);
        out.append(buf, sizeof(buf) - zs.avail_out);
    } while (ret == Z_OK && zs.avail_out == 0);   // refill only while output stays saturated
    inflateEnd(&zs);
    return out;
}

// Strip the IAC SB COMPRESS2 IAC SE start marker that precedes the deflate stream.
std::string stripMccpStart(std::string s) {
    const unsigned char marker[] = { IAC, SB, TELOPT_COMPRESS2, IAC, SE };
    if (s.size() >= sizeof(marker) &&
        std::memcmp(s.data(), marker, sizeof(marker)) == 0)
        s.erase(0, sizeof(marker));
    return s;
}

// Turn on MCCP2 by feeding the server "IAC DO COMPRESS2" through its own fd.
std::shared_ptr<Socket> makeCompressingSocket(int serverFd, int peerFd) {
    auto sock = std::make_shared<Socket>(serverFd);
    nonBlock(serverFd);
    nonBlock(peerFd);
    unsigned char neg[] = { IAC, DO, TELOPT_COMPRESS2 };
    EXPECT_EQ(::write(peerFd, neg, sizeof(neg)), static_cast<ssize_t>(sizeof(neg)));
    sock->processInput();       // -> negotiate() -> startCompress()
    return sock;
}

std::string randomBytes(size_t n, unsigned seed) {
    std::mt19937 rng(seed);
    std::string s(n, '\0');
    for (auto& c : s) c = static_cast<char>(rng() & 0xff);
    return s;
}

} // namespace

TEST(Mccp, RoundTripCompressedWrite) {
    ensureServer();
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    auto sock = makeCompressingSocket(fds[0], fds[1]);
    ASSERT_TRUE(sock->mccpEnabled());

    std::string payload;
    for (int i = 0; i < 200; i++)
        payload += "the quick brown fox jumps over the lazy dog\n";

    sock->writeRaw(payload);    // deflate -> fd, process=false (byte-exact)
    sock->flush();

    std::string raw = stripMccpStart(readAvail(fds[1]));
    EXPECT_EQ(inflateAll(raw), payload);

    sock.reset();
    ::close(fds[1]);
}

// Force EWOULDBLOCK with tiny socket buffers + incompressible data; confirm no loss.
TEST(Mccp, BackpressureBuffersAndDelivers) {
    ensureServer();
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    int sz = 2048;
    setsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, &sz, sizeof(sz));
    setsockopt(fds[1], SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz));

    auto sock = makeCompressingSocket(fds[0], fds[1]);
    ASSERT_TRUE(sock->mccpEnabled());

    std::string raw = readAvail(fds[1]);   // capture the start marker before it clogs the tiny buffer

    const std::string payload = randomBytes(256 * 1024, 12345u);   // ~incompressible -> exceeds buffers
    sock->writeRaw(payload);

    // The socket is full, so most compressed output must still be buffered, not dropped.
    EXPECT_TRUE(sock->hasOutput());

    int guard = 0;
    while (sock->hasOutput() && guard++ < 1000000) {
        raw += readAvail(fds[1]);
        sock->flush();
    }
    raw += readAvail(fds[1]);

    std::string got = inflateAll(stripMccpStart(raw));
    EXPECT_EQ(got.size(), payload.size());
    EXPECT_EQ(got, payload);

    sock.reset();
    ::close(fds[1]);
}
