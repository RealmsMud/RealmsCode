/*
 * socketAsyncBackpressure_test.cpp
 */
#include <gtest/gtest.h>

#include <asio.hpp>
#include <chrono>
#include <ctime>
#include <memory>
#include <string>

#include "socket.hpp"
#include "login.hpp"            // CON_DISCONNECTING
#include "apiTestSupport.hpp"

using asio::ip::tcp;

namespace {

// Re-exposes the protected asio I/O seams + queue state for assertions. No new behavior.
struct TestSocket : public Socket {
    explicit TestSocket(tcp::socket s) : Socket(std::move(s)) {}
    using Socket::enqueue;
    using Socket::startRead;
    using Socket::resumeRead;
    using Socket::drainAndClose;
    using Socket::writeQueue;
    using Socket::queuedBytes;
    using Socket::writeInFlight;
    using Socket::readPaused;
    using Socket::input;
    using Socket::sock;
    using Socket::drainCloseTimeout;
};

// Restores the process-wide drain deadline the tests shorten.
struct ScopedDrainTimeout {
    std::chrono::milliseconds saved{TestSocket::drainCloseTimeout};
    explicit ScopedDrainTimeout(std::chrono::milliseconds ms) { TestSocket::drainCloseTimeout = ms; }
    ~ScopedDrainTimeout() { TestSocket::drainCloseTimeout = saved; }
};

// A loopback connection: the accepted server side wrapped in a shared TestSocket (so
// shared_from_this works for the async handlers), plus the open client side to drive.
struct Link {
    asio::io_context io;
    tcp::acceptor acc;
    tcp::socket client;
    std::shared_ptr<TestSocket> server;

    Link() : acc(io, tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), 0)), client(io) {
        ensureConfig();
        ensureServer();
        // Pre-seed DNS so the ctor doesn't fork a resolver child for 127.0.0.1.
        gServer->addCache("127.0.0.1", "localhost", time(nullptr));
        client.connect(acc.local_endpoint());
        tcp::socket peer(io);
        acc.accept(peer);
        server = std::make_shared<TestSocket>(std::move(peer));
    }

    void pump(int ms = 50) {
        if(io.stopped()) io.restart();
        io.run_for(std::chrono::milliseconds(ms));
    }
    ~Link() {
        server->drainAndClose();      // cancel any armed read so handlers release their self-ref
        io.run_for(std::chrono::milliseconds(10));
    }
};

} // namespace

TEST(SocketAsyncBackpressure, BacklogOverflowKeepsInFlightBuffer) {
    Link link;
    auto& s = *link.server;

    s.enqueue("start");
    EXPECT_TRUE(s.writeInFlight);

    const std::string chunk(64 * 1024, 'x');
    while (s.queuedBytes + chunk.size() <= (1u << 20))
        s.enqueue(chunk);
    s.enqueue(chunk);

    EXPECT_EQ(s.getState(), CON_DISCONNECTING);
    EXPECT_EQ(s.writeQueue.size(), 1u);
    EXPECT_EQ(s.queuedBytes, s.writeQueue.front().size());

    link.pump();
    EXPECT_EQ(s.queuedBytes, 0u);
    EXPECT_TRUE(s.writeQueue.empty());
}

TEST(SocketAsyncBackpressure, InputReadPausesThenResumes) {
    Link link;
    auto& s = *link.server;
    s.startRead();

    std::string flood;
    for (int i = 0; i < 4000; ++i) flood += "x\n";
    asio::write(link.client, asio::buffer(flood));

    link.pump();
    EXPECT_TRUE(s.readPaused);
    EXPECT_GE(s.input.size(), 1024u);

    const size_t pausedAt = s.input.size();
    link.pump();
    EXPECT_EQ(s.input.size(), pausedAt);

    while (!s.input.empty()) s.input.pop();
    s.resumeRead();
    EXPECT_FALSE(s.readPaused);
    link.pump();
    EXPECT_GT(s.input.size(), 0u);
}

TEST(SocketAsyncBackpressure, DrainAndCloseDoesNotBlock) {
    Link link;
    auto& s = *link.server;

    s.writeQueue.emplace_back(std::string(8u << 20, 'y'));
    s.queuedBytes = s.writeQueue.front().size();
    s.writeInFlight = false;

    s.drainAndClose();
    EXPECT_EQ(s.getFd(), -1);
    EXPECT_TRUE(s.writeQueue.empty());
    EXPECT_EQ(s.queuedBytes, 0u);
}

// disconnect() right after two writes: the first is in flight, the second queued behind it.
// Both must reach the peer before the close (the "Incorrect." before a disconnect case).
TEST(SocketAsyncBackpressure, CloseDeliversQueuedOutputFirst) {
    Link link;
    auto& s = *link.server;

    s.enqueue("first");
    s.enqueue("second");
    ASSERT_TRUE(s.writeInFlight);
    s.drainAndClose();

    for (int i = 0; i < 20 && s.sock && s.sock->is_open(); i++)
        link.pump(10);
    EXPECT_FALSE(s.sock && s.sock->is_open());

    std::string got;
    asio::error_code ec;
    char buf[64];
    while (!ec) {
        const std::size_t n = link.client.read_some(asio::buffer(buf), ec);
        got.append(buf, n);
    }
    EXPECT_EQ(got, "firstsecond");
    EXPECT_EQ(ec, asio::error::eof);
}

// A peer that stops reading stalls the in-flight write; without a deadline the deferred
// close (and the Socket its handler holds) would live forever.
TEST(SocketAsyncBackpressure, DeferredCloseIsBoundedByTimeout) {
    constexpr std::chrono::milliseconds kTimeout{50};
    Link link;
    auto& s = *link.server;
    const ScopedDrainTimeout timeout(kTimeout);

    // Shrink both windows so a sub-cap payload cannot fit in kernel buffers.
    asio::error_code optEc;
    link.client.set_option(tcp::socket::receive_buffer_size(4096), optEc);
    s.sock->set_option(tcp::socket::send_buffer_size(4096), optEc);

    const std::size_t payload = 768 * 1024;   // under the 1 MB backlog cap
    s.enqueue(std::string(payload, 'z'));
    ASSERT_TRUE(s.writeInFlight);

    s.drainAndClose();
    ASSERT_TRUE(s.sock && s.sock->is_open());   // deferred, not closed yet

    const auto started = std::chrono::steady_clock::now();
    for (int i = 0; i < 40 && s.sock && s.sock->is_open(); i++)
        link.pump(10);
    const auto elapsed = std::chrono::steady_clock::now() - started;

    ASSERT_FALSE(s.sock && s.sock->is_open());
    EXPECT_GE(elapsed, kTimeout);               // the deadline closed it, not the write completing
    EXPECT_FALSE(s.writeInFlight);
    EXPECT_TRUE(s.writeQueue.empty());

    std::string got;
    asio::error_code ec;
    char buf[4096];
    while (!ec) {
        const std::size_t n = link.client.read_some(asio::buffer(buf), ec);
        got.append(buf, n);
    }
    EXPECT_TRUE(ec == asio::error::eof || ec == asio::error::connection_reset) << ec.message();
    EXPECT_LT(got.size(), payload);             // the stalled write was aborted mid-stream
}
