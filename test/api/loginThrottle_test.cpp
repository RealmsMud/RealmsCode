/*
 * loginThrottle_test.cpp
 *   ApiLoginThrottle counting, lockout, expiry and eviction.
 */
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "apiLoginThrottle.hpp"

using namespace std::chrono_literals;

namespace {

ApiLoginThrottle::Settings settings(int maxFailures, std::chrono::seconds window, std::chrono::seconds lockout, std::size_t maxEntries = 100) { return ApiLoginThrottle::Settings{maxFailures, window, lockout, maxEntries}; }

TEST(LoginThrottle, AllowsBelowThreshold) {
    ApiLoginThrottle t(settings(3, 60s, 60s));
    t.recordFailure("ip");
    t.recordFailure("ip");
    EXPECT_EQ(t.lockedFor("ip"), 0);
}

TEST(LoginThrottle, LocksAtThreshold) {
    ApiLoginThrottle t(settings(3, 60s, 30s));
    for (int i = 0; i < 3; i++)
        t.recordFailure("ip");

    const int wait = t.lockedFor("ip");
    EXPECT_GT(wait, 0);
    EXPECT_LE(wait, 30);
}

TEST(LoginThrottle, RetryAfterDecreases) {
    ApiLoginThrottle t(settings(1, 60s, 2s));
    t.recordFailure("ip");
    const int first = t.lockedFor("ip");
    std::this_thread::sleep_for(1100ms);
    const int second = t.lockedFor("ip");

    EXPECT_GT(first, 0);
    EXPECT_LT(second, first);
}

TEST(LoginThrottle, WindowExpiryResetsCount) {
    ApiLoginThrottle t(settings(2, 0s, 60s)); // anything older than 0s falls out of the window
    t.recordFailure("ip");
    std::this_thread::sleep_for(3ms);
    t.recordFailure("ip");
    EXPECT_EQ(t.lockedFor("ip"), 0);
}

TEST(LoginThrottle, LockoutExpiresAndDropsEntry) {
    ApiLoginThrottle t(settings(1, 60s, 0s));
    t.recordFailure("ip");
    std::this_thread::sleep_for(3ms);

    EXPECT_EQ(t.lockedFor("ip"), 0);
    EXPECT_EQ(t.size(), 0u);
}

TEST(LoginThrottle, SuccessClearsFailures) {
    ApiLoginThrottle t(settings(2, 60s, 60s));
    t.recordFailure("ip");
    t.recordSuccess("ip");
    t.recordFailure("ip");

    EXPECT_EQ(t.lockedFor("ip"), 0);
    EXPECT_EQ(t.size(), 1u);
}

TEST(LoginThrottle, KeysAreIsolated) {
    ApiLoginThrottle t(settings(2, 60s, 60s));
    t.recordFailure("a");
    t.recordFailure("a");

    EXPECT_GT(t.lockedFor("a"), 0);
    EXPECT_EQ(t.lockedFor("b"), 0);
}

TEST(LoginThrottle, DisabledWhenMaxFailuresZero) {
    ApiLoginThrottle t(settings(0, 60s, 60s));
    for (int i = 0; i < 50; i++)
        t.recordFailure("ip");

    EXPECT_EQ(t.lockedFor("ip"), 0);
    EXPECT_EQ(t.size(), 0u);
}

TEST(LoginThrottle, EvictsWhenFull) {
    ApiLoginThrottle t(settings(5, 60s, 60s, 2));
    t.recordFailure("a");
    t.recordFailure("b");
    t.recordFailure("c");

    EXPECT_LE(t.size(), 2u);
}

TEST(LoginThrottle, ClearEmptiesAll) {
    ApiLoginThrottle t(settings(5, 60s, 60s));
    t.recordFailure("a");
    t.recordFailure("b");
    t.clear();

    EXPECT_EQ(t.size(), 0u);
}

// Exercises the mutex paths under contention; meaningful under the sanitizer build
// (LEAK=1), a no-crash smoke test otherwise.
TEST(LoginThrottle, ConcurrentAccessNoRace) {
    ApiLoginThrottle t(settings(3, 60s, 60s, 8));
    std::vector<std::thread> ts;
    for (int n = 0; n < 8; n++)
        ts.emplace_back([&t, n] {
            for (int i = 0; i < 5000; i++) {
                const std::string k = "k" + std::to_string((n * 7 + i) % 16);
                t.recordFailure(k);
                (void) t.lockedFor(k);
                if (i % 4 == 0)
                    t.recordSuccess(k);
                if (i % 1000 == 0)
                    t.clear();
            }
        });
    for (auto &th: ts)
        th.join();
    SUCCEED();
}

} // namespace
