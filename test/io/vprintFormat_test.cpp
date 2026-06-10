/*
 * vprintFormat_test.cpp
 *   realmsFormat - portable replacement for glibc register_printf_specifier + vasprintf.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include <gtest/gtest.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

std::string realmsFormat(const char *fmt, va_list ap);

static std::string rf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string s = realmsFormat(fmt, ap);
    va_end(ap);
    return s;
}

static std::string libc(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *buf = nullptr;
    int n = vasprintf(&buf, fmt, ap);
    va_end(ap);
    std::string s = (n >= 0 && buf) ? std::string(buf, n) : std::string();
    free(buf);
    return s;
}

#define SAME(...) EXPECT_EQ(rf(__VA_ARGS__), libc(__VA_ARGS__))

TEST(RealmsFormat, Integers) {
    SAME("%d", 0);
    SAME("%d", -5);
    SAME("%d", 2147483647);
    SAME("%i", 42);
    SAME("%u", 4000000000u);
    SAME("count=%d done", 7);
}

TEST(RealmsFormat, WidthFlagsPrecision) {
    SAME("%5d", 42);
    SAME("%-5d|", 42);
    SAME("%05d", 42);
    SAME("%+d", 42);
    SAME("% d", 42);
    SAME("%8.3f", 3.14159);
    SAME("%.0f", 2.7);
    SAME("%-10s|", "hi");
    SAME("%10s|", "hi");
    SAME("%.3s", "hello");
}

TEST(RealmsFormat, LengthModifiers) {
    SAME("%ld", 1234567890123L);
    SAME("%02ld", 5L);
    SAME("%lu", 12345678901UL);
    SAME("%lld", -9000000000000LL);
    SAME("%zu", (size_t) 4096);
    SAME("%jd", (intmax_t) -123);
}

TEST(RealmsFormat, HexOctalCharFloat) {
    SAME("%x", 255);
    SAME("%#x", 255);
    SAME("%X", 0xABCDEF);
    SAME("%o", 64);
    SAME("%c", 'A');
    SAME("%g", 0.0001);
    SAME("%e", 123456.789);
}

TEST(RealmsFormat, DynamicWidthPrecision) {
    SAME("%*d", 6, 42);
    SAME("%.*f", 2, 3.14159);
    SAME("%-*d|", 5, 7);
}

TEST(RealmsFormat, PercentLiteralAndPlainText) {
    SAME("100%% complete");
    SAME("no specifiers here");
    EXPECT_EQ(rf(""), "");
}

TEST(RealmsFormat, CustomOStringStream) {
    std::ostringstream oss;
    oss << "WORLD";
    EXPECT_EQ(rf("%T", &oss), "WORLD");
}

TEST(RealmsFormat, LockstepMixedArgs) {
    std::ostringstream oss;
    oss << "WORLD";
    EXPECT_EQ(rf("%s %T %d", "HELLO", &oss, 42), "HELLO WORLD 42");
    EXPECT_EQ(rf("[%d] %T!", 7, &oss), "[7] WORLD!");
}
