/*
 * startlocs_test.cpp
 *   startingChoices() writes start-location names into a caller-supplied char* using a
 *   threaded buffer size (the sprintf->snprintf fix for its char* parameter). These guard
 *   that an undersized buffer truncates + NUL-terminates and never overruns.
 *
 *   A DARKELF FIGHTER (no deity) resolves to two options (oakspire, highport), which takes the
 *   !choose branch -- it formats names into `location` and returns false with no bind/file
 *   side effects, so no room/world fixture is needed.
 */
#include <cstring>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "global.hpp"
#include "mudObjects/players.hpp"
#include "apiTestSupport.hpp"

// from areas/startlocs.cpp -- no public header (mirrors the forward decl in server/login.cpp)
bool startingChoices(std::shared_ptr<Player> player, std::string str, char *location, size_t locationSize, bool choose);

namespace {
    std::shared_ptr<Player> darkelfFighter() {
        auto p = makePlayer(CreatureClass::FIGHTER);
        p->setRace(DARKELF);
        return p;
    }
}

TEST(StartingChoices, ListsChoicesIntoBuffer) {
    auto p = darkelfFighter();
    char loc[256];
    EXPECT_FALSE(startingChoices(p, "", loc, sizeof(loc), false));  // >1 option -> must choose
    const std::string out = loc;
    EXPECT_NE(out.find("Oakspire"), std::string::npos);
    EXPECT_NE(out.find("Highport"), std::string::npos);
}

TEST(StartingChoices, HonorsBufferSizeNoOverflow) {
    auto p = darkelfFighter();
    char loc[64];
    memset(loc, 0xAB, sizeof(loc));
    const size_t cap = 8;                                        // pretend buffer is only 8 bytes
    startingChoices(p, "", loc, cap, false);
    EXPECT_EQ(static_cast<unsigned char>(loc[cap]), 0xAB);       // nothing written at/after cap
    EXPECT_LT(strnlen(loc, cap), cap);                           // NUL-terminated within cap
}
