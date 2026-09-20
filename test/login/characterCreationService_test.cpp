// pure validators; config-dependent cases skip without REALMS_BASE_PATH.
#include <gtest/gtest.h>

#include <libxml/parser.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <string>
#include <vector>

#include "account.hpp"
#include "config.hpp"
#include "paths.hpp"
#include "global.hpp"
#include "structs.hpp"
#include "raceData.hpp"
#include "playerClass.hpp"
#include "proto.hpp" // get_class_string
#include "login/characterDraft.hpp"
#include "login/characterCreationService.hpp"
#include "mudObjects/players.hpp"
#include "apiTestSupport.hpp"

namespace {

bool gameConfigReady() {
    static const bool ready = [] {
        ensureConfig();
        ensureServer();
        const bool ok = gConfig->loadConfig() && gConfig->loadSkillGroups() && gConfig->loadSkills() && gConfig->loadDeities() && gConfig->loadClasses() && gConfig->loadRaces();
        return ok;
    }();
    return ready;
}

// GTEST_SKIP returns, so the data gate has to expand inside the test body.
#define SKIP_WITHOUT_GAME_DATA()                                                                                                                                                                                                               \
    do {                                                                                                                                                                                                                                       \
        if (!gameConfigReady())                                                                                                                                                                                                                \
            GTEST_SKIP() << "game config not available (set REALMS_BASE_PATH)";                                                                                                                                                                \
    } while (false)

using Create::parseStats;
using Create::parseStatsChoice;
using Create::resolveClassPair;
using Create::validateSex;
using Create::validateStatModifier;

// No SetUp gate: the pure validators must run with no game data on disk.
class CharacterCreationService : public ::testing::Test {};

TEST_F(CharacterCreationService, ClassPairSingleClass) {
    auto p = resolveClassPair(static_cast<int>(CreatureClass::FIGHTER));
    EXPECT_EQ(p.cClass, CreatureClass::FIGHTER);
    EXPECT_EQ(p.second, CreatureClass::NONE);
}

TEST_F(CharacterCreationService, ClassPairMulticlass) {
    auto fm = resolveClassPair(MULTI_BASE + 0);
    EXPECT_EQ(fm.cClass, CreatureClass::FIGHTER);
    EXPECT_EQ(fm.second, CreatureClass::MAGE);

    auto cf = resolveClassPair(MULTI_BASE + 5);
    EXPECT_EQ(cf.cClass, CreatureClass::CLERIC);
    EXPECT_EQ(cf.second, CreatureClass::FIGHTER);
}

TEST_F(CharacterCreationService, ClassPairInvalidCodeIsNone) {
    auto p = resolveClassPair(-1);
    EXPECT_EQ(p.cClass, CreatureClass::NONE);
    EXPECT_EQ(p.second, CreatureClass::NONE);
}

TEST_F(CharacterCreationService, SexMaleFemale) {
    EXPECT_EQ(validateSex('m', true).sex, SEX_MALE);
    EXPECT_EQ(validateSex('F', true).sex, SEX_FEMALE);
    EXPECT_TRUE(validateSex('m', true).ok);
}

TEST_F(CharacterCreationService, SexInvalidWhenGendered) { EXPECT_FALSE(validateSex('x', true).ok); }

TEST_F(CharacterCreationService, SexUngenderedIsNoneRegardlessOfInput) {
    auto r = validateSex('x', false);
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.sex, SEX_NONE);
}

TEST_F(CharacterCreationService, StatsChoiceCustomPredefinedInvalid) {
    EXPECT_EQ(parseStatsChoice("c"), Create::StatsChoiceKind::Custom);
    EXPECT_EQ(parseStatsChoice("  U  "), Create::StatsChoiceKind::Predefined);
    EXPECT_EQ(parseStatsChoice("x"), Create::StatsChoiceKind::Invalid);
    EXPECT_EQ(parseStatsChoice(""), Create::StatsChoiceKind::Invalid);
}

TEST_F(CharacterCreationService, StatsValidFiftySix) {
    auto r = parseStats("12 12 12 10 10");
    EXPECT_EQ(r.error, Create::StatsError::Ok);
    EXPECT_EQ(r.sum, 56);
    EXPECT_EQ(r.stats, (std::array<int, 5>{12, 12, 12, 10, 10}));
}

TEST_F(CharacterCreationService, StatsTooFew) { EXPECT_EQ(parseStats("12 12 12").error, Create::StatsError::TooFew); }

TEST_F(CharacterCreationService, StatsNonNumericTokenIsTooFew) {
    EXPECT_EQ(parseStats("12 ab 12 10 10").error, Create::StatsError::TooFew);
    EXPECT_EQ(parseStats("12 12 12 10 10x").error, Create::StatsError::TooFew);
    EXPECT_EQ(parseStats("").error, Create::StatsError::TooFew);
}

TEST_F(CharacterCreationService, StatsSixNumbersIsTooFew) { EXPECT_EQ(parseStats("12 12 12 10 10 3").error, Create::StatsError::TooFew); }

TEST_F(CharacterCreationService, StatsToleratesExtraWhitespace) {
    const auto r = parseStats("  12\t12   12\t10 10  ");
    EXPECT_EQ(r.error, Create::StatsError::Ok);
    EXPECT_EQ(r.sum, Create::kStatTotal);
    EXPECT_EQ(r.stats, (std::array<int, Create::kStatCount>{12, 12, 12, 10, 10}));
}

TEST_F(CharacterCreationService, StatsOnePointShortIsBadSum) {
    const auto r = parseStats("12 12 12 10 9");
    EXPECT_EQ(r.error, Create::StatsError::BadSum);
    EXPECT_EQ(r.sum, Create::kStatTotal - 1);
}

TEST_F(CharacterCreationService, StatsOutOfRange) {
    EXPECT_EQ(parseStats("19 10 10 10 7").error, Create::StatsError::OutOfRange);
    EXPECT_EQ(parseStats("2 18 18 8 10").error, Create::StatsError::OutOfRange);
}

TEST_F(CharacterCreationService, StatsBadSum) {
    auto r = parseStats("18 18 18 18 18"); // 90, all in range
    EXPECT_EQ(r.error, Create::StatsError::BadSum);
    EXPECT_EQ(r.sum, 90);
}

TEST_F(CharacterCreationService, StatModifierRange) {
    EXPECT_TRUE(validateStatModifier('a').ok);
    EXPECT_EQ(validateStatModifier('a').statId, STR);
    EXPECT_EQ(validateStatModifier('e').statId, STR + 4); // PTY
    EXPECT_FALSE(validateStatModifier('f').ok);
}

TEST_F(CharacterCreationService, SingleAndMultiClassKeys) {
    CharacterDraft d;
    d.cClass = CreatureClass::FIGHTER;
    const std::string fighter = get_class_string(static_cast<int>(CreatureClass::FIGHTER));
    EXPECT_EQ(Create::draftClassKey(d), fighter);

    d.secondClass = CreatureClass::MAGE;
    const std::string mage = get_class_string(static_cast<int>(CreatureClass::MAGE));
    EXPECT_EQ(Create::draftClassKey(d), fighter + "/" + mage);
}

TEST_F(CharacterCreationService, SanitizeDeityKeepsWorshippableElseZero) {
    EXPECT_EQ(Create::sanitizeDeity(ARAMON), ARAMON);
    EXPECT_EQ(Create::sanitizeDeity(MARA), MARA);
    EXPECT_EQ(Create::sanitizeDeity(0), 0);
    EXPECT_EQ(Create::sanitizeDeity(999), 0);
}

TEST_F(CharacterCreationService, ComputeFinalStatsScalesAndAppliesBonusPenalty) {
    SKIP_WITHOUT_GAME_DATA();
    const RaceData *human = gConfig->getRace(HUMAN);
    ASSERT_NE(human, nullptr);

    CharacterDraft base;
    base.race = HUMAN;
    base.baseStats = {12, 12, 12, 10, 10};
    const auto plain = Create::computeFinalStats(base, human);

    // base*10 + race adj, before any bonus/penalty.
    EXPECT_EQ(plain[0], 12 * 10 + human->getStatAdj(STR));

    CharacterDraft bonused = base;
    bonused.bonusStat = STR;
    bonused.penaltyStat = DEX;
    const auto adj = Create::computeFinalStats(bonused, human);
    EXPECT_EQ(adj[0], plain[0] + 10); // +10 to STR
    EXPECT_EQ(adj[1], plain[1] - 10); // -10 to DEX
}

TEST_F(CharacterCreationService, ValidateNameAcceptsAndRejects) {
    SKIP_WITHOUT_GAME_DATA();
    EXPECT_EQ(Create::validateNewCharacterName("Tordek", nullptr).error, Create::CharNameError::Ok);
    EXPECT_EQ(Create::validateNewCharacterName("Tordek", nullptr).normalized, "Tordek");
    EXPECT_EQ(Create::validateNewCharacterName("  ", nullptr).error, Create::CharNameError::Empty);
    EXPECT_EQ(Create::validateNewCharacterName("Abcdefghijklmnopqrstuvwxyz", nullptr).error, Create::CharNameError::TooLong);
    EXPECT_EQ(Create::validateNewCharacterName("Bo", nullptr).error, Create::CharNameError::BadName);
}

// Taken/InAccount are probed before the reserved-name flag, so an existing DM character
// reports Taken and only a still-free DM name reaches the DM-password gate.
TEST_F(CharacterCreationService, ValidateNameChecksTakenBeforeReserved) {
    SKIP_WITHOUT_GAME_DATA();
    const auto skipped = Create::validateNewCharacterName("bane", nullptr, /*checkExists*/ false);
    EXPECT_EQ(skipped.error, Create::CharNameError::Reserved);
    EXPECT_EQ(skipped.normalized, "Bane");

    const auto probed = Create::validateNewCharacterName("bane", nullptr);
    EXPECT_EQ(probed.error, Player::exists("Bane") ? Create::CharNameError::Taken : Create::CharNameError::Reserved);
}

TEST_F(CharacterCreationService, ValidateNameRejectsAccountDuplicate) {
    SKIP_WITHOUT_GAME_DATA();
    Account account{"creacct"};
    account.addCharacter("Tordek");
    // lowercize("tordek",1) -> "Tordek", which the account already holds.
    EXPECT_EQ(Create::validateNewCharacterName("tordek", &account).error, Create::CharNameError::InAccount);
}

TEST_F(CharacterCreationService, ClassForDraftFindsKnownRejectsUnknown) {
    SKIP_WITHOUT_GAME_DATA();
    CharacterDraft d;
    d.cClass = CreatureClass::FIGHTER;
    EXPECT_NE(Create::classForDraft(d), nullptr);

    d.secondClass = CreatureClass::LICH; // no fighter/lich multiclass definition
    EXPECT_EQ(Create::classForDraft(d), nullptr);

    CharacterDraft unset; // class never chosen -> unmapped key
    EXPECT_EQ(Create::classForDraft(unset), nullptr);

    // The lookup must not pollute gConfig->classes with null entries (operator[] would).
    const auto count = gConfig->classes.size();
    EXPECT_EQ(Create::classForDraft(d), nullptr);
    EXPECT_EQ(gConfig->classes.size(), count);
}

TEST_F(CharacterCreationService, ValidateNameExistsProbeIsSkippable) {
    SKIP_WITHOUT_GAME_DATA();
    const std::string name = "Zzexistsprobe";
    const fs::path playerFile = (Path::Player / name).replace_extension("xml");
    {
        std::ofstream stub(playerFile);
        stub << "<Player/>";
    }

    EXPECT_EQ(Create::validateNewCharacterName(name, nullptr).error, Create::CharNameError::Taken);
    EXPECT_EQ(Create::validateNewCharacterName(name, nullptr, false).error, Create::CharNameError::Ok);

    fs::remove(playerFile);
}

TEST_F(CharacterCreationService, PlayableRacesIncludeHumanAllNonSubrace) {
    SKIP_WITHOUT_GAME_DATA();
    const std::vector<short> races = Create::playableRaceChoices();
    ASSERT_FALSE(races.empty());
    EXPECT_NE(std::find(races.begin(), races.end(), short{HUMAN}), races.end());
    for (short id: races) {
        const RaceData *r = gConfig->getRace(id);
        ASSERT_NE(r, nullptr);
        EXPECT_TRUE(r->isPlayable());
        EXPECT_EQ(r->getParentRace(), 0); // no subraces in the top-level menu
    }
}

TEST_F(CharacterCreationService, HumanAllowsFighter) {
    SKIP_WITHOUT_GAME_DATA();
    const std::vector<int> codes = Create::allowedClassCodes(HUMAN);
    EXPECT_NE(std::find(codes.begin(), codes.end(), static_cast<int>(CreatureClass::FIGHTER)), codes.end());
}

TEST_F(CharacterCreationService, MaleClericDeitiesExcludeMara) {
    SKIP_WITHOUT_GAME_DATA();
    // Mara clerics must be female; a male cleric should never be offered Mara.
    const std::vector<int> deities = Create::allowedDeityChoices(HUMAN, CreatureClass::CLERIC, CreatureClass::NONE, SEX_MALE);
    EXPECT_EQ(std::find(deities.begin(), deities.end(), int{MARA}), deities.end());
}

TEST_F(CharacterCreationService, GrantInitialWeaponSkillsIsDeterministicAndConsistent) {
    SKIP_WITHOUT_GAME_DATA();
    const RaceData *human = gConfig->getRace(HUMAN);
    PlayerClass *fighter = gConfig->classes[get_class_string(static_cast<int>(CreatureClass::FIGHTER))];
    ASSERT_NE(human, nullptr);
    ASSERT_NE(fighter, nullptr);

    CharacterDraft a;
    a.race = HUMAN;
    a.cClass = CreatureClass::FIGHTER;
    CharacterDraft b = a;

    const auto ra = Create::grantInitialWeaponSkills(a, human, fighter);
    const auto rb = Create::grantInitialWeaponSkills(b, human, fighter);

    EXPECT_EQ(a.weaponSkills, b.weaponSkills); // deterministic
    EXPECT_LE(a.weaponSkills.size(), static_cast<size_t>(ra.raceCount + ra.classCount)); // dedup
    EXPECT_EQ(ra.raceCount, rb.raceCount);
    for (const auto &s: a.weaponSkills)
        EXPECT_FALSE(s.empty());
}

// A base skill that is not in skills.xml must be skipped, not dereferenced.
TEST_F(CharacterCreationService, GrantInitialWeaponSkillsSkipsUnknownSkill) {
    SKIP_WITHOUT_GAME_DATA();
    const std::string xml = R"(<Race id="97" name="Testmissingskill"><Data><Skills>)"
                            R"(<Skill><Name>zzz-no-such-skill</Name><Gained>1</Gained></Skill>)"
                            R"(</Skills></Data></Race>)";
    xmlDocPtr doc = xmlReadMemory(xml.c_str(), static_cast<int>(xml.size()), nullptr, nullptr, 0);
    ASSERT_NE(doc, nullptr);
    RaceData race(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);

    PlayerClass *fighter = gConfig->classes[get_class_string(static_cast<int>(CreatureClass::FIGHTER))];
    ASSERT_NE(fighter, nullptr);

    CharacterDraft d;
    d.cClass = CreatureClass::FIGHTER;
    const auto granted = Create::grantInitialWeaponSkills(d, &race, fighter);
    EXPECT_EQ(granted.raceCount, 0);
    EXPECT_EQ(std::count(d.weaponSkills.begin(), d.weaponSkills.end(), "zzz-no-such-skill"), 0);
}

} // namespace
