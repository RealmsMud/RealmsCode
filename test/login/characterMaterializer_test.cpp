// Create::materialize: CharacterDraft in, a fully-built level-1 Player out.
// needs REALMS_BASE_PATH; skipped when absent.
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <memory>
#include <string>

#include "account.hpp"
#include "config.hpp"
#include "flags.hpp"
#include "paths.hpp"
#include "global.hpp"
#include "mudObjects/players.hpp"
#include "raceData.hpp"
#include "structs.hpp"
#include "login/characterCreationService.hpp"
#include "login/characterDraft.hpp"
#include "login/characterMaterializer.hpp"
#include "apiTestSupport.hpp"

namespace {

bool gameConfigReady() {
    static const bool ready = [] {
        ensureConfig();
        ensureServer();
        const bool ok = gConfig->loadConfig() && gConfig->loadStartLoc() && gConfig->loadSkillGroups() && gConfig->loadSkills() && gConfig->loadDeities() && gConfig->loadClasses() && gConfig->loadRaces();
        gConfig->loadCalendar(); // setBirthday() reads the calendar
        return ok;
    }();
    return ready;
}

CharacterDraft humanFighter() {
    CharacterDraft d;
    d.name = "Testchar";
    d.race = HUMAN;
    d.sex = SEX_MALE;
    d.cClass = CreatureClass::FIGHTER;
    d.baseStats = {12, 12, 12, 10, 10}; // sums to 56
    return d;
}

class CharacterMaterializer : public ::testing::Test {
protected:
    void SetUp() override {
        if (!gameConfigReady())
            GTEST_SKIP() << "game config not available (set REALMS_BASE_PATH)";
    }
    Account account{"testacct"};
};

TEST_F(CharacterMaterializer, BuildsCoreIdentity) {
    auto p = Create::materialize(humanFighter(), account);
    ASSERT_TRUE(p);
    EXPECT_EQ(p->getClass(), CreatureClass::FIGHTER);
    EXPECT_EQ(p->getRace(), HUMAN);
    EXPECT_EQ(p->getName(), "Testchar");
    EXPECT_EQ(p->getAccountName(), "testacct");
    EXPECT_EQ(p->getLevel(), 1);
    EXPECT_FALSE(p->hasSecondClass());
}

TEST_F(CharacterMaterializer, GrantsFighterDefensiveAndArmorSkills) {
    auto p = Create::materialize(humanFighter(), account);
    // Every new character gets defense; a fighter gets block + parry.
    EXPECT_TRUE(p->knowsSkill("defense"));
    EXPECT_TRUE(p->knowsSkill("block"));
    EXPECT_TRUE(p->knowsSkill("parry"));
    // Fighter armor proficiencies (the fall-through armor block).
    EXPECT_TRUE(p->knowsSkill("plate"));
    EXPECT_TRUE(p->knowsSkill("chain"));
    EXPECT_TRUE(p->knowsSkill("leather"));
    EXPECT_TRUE(p->knowsSkill("cloth"));
}

TEST_F(CharacterMaterializer, GrantsArcaneSchoolsForNonDivineNonBerserker) {
    auto p = Create::materialize(humanFighter(), account);
    EXPECT_TRUE(p->knowsSkill("abjuration"));
    EXPECT_TRUE(p->knowsSkill("evocation"));
    EXPECT_TRUE(p->knowsSkill("fire"));
    EXPECT_FALSE(p->knowsSkill("healing"));
}

TEST_F(CharacterMaterializer, AppliesStatsWithFloor) {
    auto p = Create::materialize(humanFighter(), account);
    // adjustStats restores cur from initial; every stat ends up at least the
    // floor (10 initial -> 100 internal scale) and strictly positive.
    EXPECT_GT(p->strength.getInitial(), 0);
    EXPECT_GE(p->constitution.getInitial(), 100);
}

// Only Server::addPlayer mints an id on the telnet path; the headless path has to do it
// itself or starting items and the saved file record an empty owner.
TEST_F(CharacterMaterializer, AssignsPlayerId) {
    auto p = Create::materialize(humanFighter(), account);
    const std::string id = p->getId();
    ASSERT_GT(id.size(), 1u);
    EXPECT_EQ(id[0], 'P');
    EXPECT_TRUE(std::all_of(id.begin() + 1, id.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) << id;
}

TEST_F(CharacterMaterializer, StatsMatchComputeFinalStats) {
    CharacterDraft d = humanFighter();
    d.bonusStat = STR;
    d.penaltyStat = INT;

    auto p = Create::materialize(d, account);
    const auto expected = Create::computeFinalStats(d, gConfig->getRace(d.race));
    EXPECT_EQ(p->strength.getInitial(), expected[0]);
    EXPECT_EQ(p->dexterity.getInitial(), expected[1]);
    EXPECT_EQ(p->constitution.getInitial(), expected[2]);
    EXPECT_EQ(p->intelligence.getInitial(), expected[3]);
    EXPECT_EQ(p->piety.getInitial(), expected[4]);
}

TEST_F(CharacterMaterializer, ClearsPasswordAndMarksAccountAuth) {
    auto p = Create::materialize(humanFighter(), account);
    EXPECT_TRUE(p->getPassword().empty());
    EXPECT_TRUE(p->flagIsSet(P_PASSWORD_CURRENT));
}

TEST_F(CharacterMaterializer, PersistWritesPlayerAndLinksAccount) {
    CharacterDraft d = humanFighter();
    d.name = "Zzpersistchar";
    Account acct{"zzpersistacct"};
    const fs::path playerFile = (Path::Player / d.name).replace_extension("xml");
    const fs::path acctFile = (Path::Account / acct.getName()).replace_extension("json");
    fs::remove(playerFile);
    fs::remove(acctFile);

    auto p = Create::materialize(d, acct);
    const Create::PersistResult r = Create::persistNewCharacter(p, acct);
    EXPECT_TRUE(r.ok) << r.error;
    EXPECT_TRUE(fs::exists(playerFile));
    EXPECT_TRUE(acct.hasCharacter(d.name));

    fs::remove(playerFile);
    fs::remove(acctFile);
}

TEST_F(CharacterMaterializer, PersistRollsBackWhenAccountFull) {
    CharacterDraft d = humanFighter();
    d.name = "Zzrollbackchar";
    Account acct{"zzrollbackacct"};
    for (int i = 0; i < acct.getCharacterLimit(); i++)
        ASSERT_TRUE(acct.addCharacter("Filler" + std::to_string(i)));
    const fs::path playerFile = (Path::Player / d.name).replace_extension("xml");
    fs::remove(playerFile);

    auto p = Create::materialize(d, acct);
    const Create::PersistResult r = Create::persistNewCharacter(p, acct);
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(acct.hasCharacter(d.name)); // membership rolled back / never added
    EXPECT_FALSE(fs::exists(playerFile)); // the saved player file was removed
}

TEST_F(CharacterMaterializer, CommitRejectsTakenName) {
    CharacterDraft d = humanFighter();
    d.name = "Zztakenchar";
    Account acct{"zztakenacct"};
    const fs::path playerFile = (Path::Player / d.name).replace_extension("xml");
    {
        std::ofstream stub(playerFile);
        stub << "<Player/>";
    }

    const Create::CommitResult r = Create::commitCharacter(d, acct);
    EXPECT_FALSE(r.ok);
    EXPECT_TRUE(r.nameTaken);
    EXPECT_FALSE(acct.hasCharacter(d.name));

    fs::remove(playerFile);
}

} // namespace
