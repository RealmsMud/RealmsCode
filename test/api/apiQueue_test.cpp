/*
 * apiQueue_test.cpp
 *   Unit tests for the REST API -> game-loop request bridge.
 */
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <crow.h>
#include <gtest/gtest.h>

#include "apiQueue.hpp"

TEST(ApiQueue, InlineRunsImmediately) {
    ApiRequestQueue q;
    q.setInlineMode(true);
    auto r = q.submit([]() { return crow::response(200, "hi"); });
    EXPECT_EQ(r.code, 200);
    EXPECT_EQ(r.body, "hi");
}

TEST(ApiQueue, QueuedDeliveredOnDrain) {
    ApiRequestQueue q;
    crow::response captured;
    std::thread worker([&] {
        captured = q.submit([]() { return crow::response(201, "made"); }, std::chrono::seconds(5));
    });

    // Simulate the main loop draining the queue once per tick.
    for(int i = 0; i < 200; i++) {
        q.drain();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    worker.join();

    EXPECT_EQ(captured.code, 201);
    EXPECT_EQ(captured.body, "made");
}

TEST(ApiQueue, TimeoutReturns503WhenNeverDrained) {
    ApiRequestQueue q;
    auto r = q.submit([]() { return crow::response(200); }, std::chrono::milliseconds(50));
    EXPECT_EQ(r.code, 503);
}

TEST(ApiQueue, InlineJobThrowReturns500) {
    ApiRequestQueue q;
    q.setInlineMode(true);
    auto r = q.submit([]() -> crow::response { throw std::runtime_error("boom"); });
    EXPECT_EQ(r.code, 500);
}

TEST(ApiQueue, QueuedJobThrowReturns500) {
    ApiRequestQueue q;
    crow::response captured;
    std::thread worker([&] {
        captured = q.submit([]() -> crow::response { throw std::runtime_error("boom"); },
                            std::chrono::seconds(5));
    });
    for(int i = 0; i < 200; i++) {
        q.drain();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    worker.join();
    EXPECT_EQ(captured.code, 500);
}

TEST(ApiQueue, ConcurrentSubmittersAllComplete) {
    ApiRequestQueue q;
    std::atomic<bool> stop{false};
    std::thread drainer([&] {
        while(!stop.load()) {
            q.drain();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        q.drain();
    });

    constexpr int N = 32;
    std::vector<std::thread> workers;
    std::vector<crow::response> results(N);
    for(int i = 0; i < N; i++)
        workers.emplace_back([&, i] {
            results[i] = q.submit([i]() { return crow::response(200, std::to_string(i)); },
                                  std::chrono::seconds(10));
        });
    for(auto& w : workers)
        w.join();
    stop.store(true);
    drainer.join();

    for(int i = 0; i < N; i++) {
        EXPECT_EQ(results[i].code, 200);
        EXPECT_EQ(results[i].body, std::to_string(i));
    }
}
