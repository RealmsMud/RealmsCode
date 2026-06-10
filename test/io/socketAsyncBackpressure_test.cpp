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
