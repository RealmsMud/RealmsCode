/*
 * startlocs_test.cpp
 *   startingLocationChoices() resolves the legal start locations for a race/class/deity,
 *   with no bind/file side effects, so no room/world fixture is needed.
 */
#include <gtest/gtest.h>

#include "global.hpp"
#include "login/characterCreationService.hpp"
#include "apiTestSupport.hpp"

// A DARKELF FIGHTER (no deity) resolves to two options (oakspire, highport).
constexpr short kRace = DARKELF;
constexpr CreatureClass kClass = CreatureClass::FIGHTER;
constexpr int kNoDeity = 0;

TEST(StartingChoices, ListsOptionsWithCapitalizedLabels) {
    ensureConfig();
    const auto options = Create::startingLocationChoices(kRace, kClass, kNoDeity);
    ASSERT_EQ(options.size(), 2u);
    EXPECT_EQ(options[0].key, "oakspire");
    EXPECT_EQ(options[0].label, "Oakspire");
    EXPECT_EQ(options[1].key, "highport");
    EXPECT_EQ(options[1].label, "Highport");
}
