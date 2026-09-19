// needs REALMS_BASE_PATH; skipped when absent.
#include <gtest/gtest.h>

#include <sys/socket.h>
#include <unistd.h>

#include <libxml/parser.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "account.hpp"
#include "config.hpp"
#include "global.hpp"
#include "login.hpp"
#include "login/characterDraft.hpp"
#include "login/characterMaterializer.hpp"
#include "login/creationFlow.hpp"
#include "login/creationSession.hpp"
#include "mudObjects/players.hpp"
#include "paths.hpp"
#include "raceData.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "structs.hpp"
#include "apiTestSupport.hpp"

extern const char* dmname[];

namespace {

using Step = CreationSession::Step;
namespace Flow = Create::Flow;

bool gameConfigReady() {
    static const bool ready = [] {
        ensureConfig();
        ensureServer();
        const bool ok = gConfig->loadConfig()
            && gConfig->loadStartLoc()
            && gConfig->loadSkillGroups() && gConfig->loadSkills()
            && gConfig->loadDeities()
            && gConfig->loadClasses()
            && gConfig->loadRaces();
        gConfig->loadCalendar();
        return ok;
    }();
    return ready;
}

void drainFd(int fd) {
    char buf[8192];
    while (::read(fd, buf, sizeof(buf)) > 0) {}
}

// The positional telnet letter for a stable Flow value at the given step.
char letterFor(const CharacterDraft& draft, Step step, const std::string& value) {
    const auto options = Flow::options(draft, step);
    for (std::size_t i = 0; i < options.size(); i++)
        if (options[i].value == value)
            return static_cast<char>('a' + i);
    ADD_FAILURE() << "no option '" << value << "' at step " << Flow::stepToken(step);
    return '?';
}

// Drives createPlayer over a socketpair; the caller inspects the draft/session.
struct Wizard {
    int fds[2]{-1, -1};
    std::shared_ptr<Socket> sock;

    explicit Wizard(const std::string& accountName = "wizacct") {
        EXPECT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
        nonBlock(fds[0]);
        nonBlock(fds[1]);
        sock = std::make_shared<Socket>(fds[0]);
        auto acct = std::make_shared<Account>(accountName);
        gServer->cacheAccount(acct);
        sock->setAccount(acct);
        startCharacterCreation(sock);   // renders the first prompt (race menu)
        pump();
    }
    ~Wizard() {
        if (fds[1] != -1)
            ::close(fds[1]);
    }

    void send(const std::string& in) {
        createPlayer(sock, in);
        pump();
    }
    void pick(const std::string& value) { send(std::string(1, letterFor(draft(), step(), value))); }

    [[nodiscard]] const CharacterDraft& draft() const { return sock->getCreationDraft(); }
    [[nodiscard]] Step step() const { return sock->getCreationSession().step; }

    // The first letter past the end of the current menu.
    [[nodiscard]] std::string pastEndLetter() const {
        const auto n = Flow::options(draft(), step()).size();
        EXPECT_LT(n, 26u);
        return std::string(1, static_cast<char>('a' + std::min<std::size_t>(n, 25)));
    }

    void pump() {
        sock->flush();
        drainFd(fds[1]);
    }
};

std::shared_ptr<Socket> driveWizard(const std::vector<std::string>& inputs,
                                    const std::string& accountName = "wizacct") {
    Wizard wizard(accountName);
    for (const auto& in : inputs)
        wizard.send(in);
    return wizard.sock;
}

// First reserved DM name whose player file does / does not exist; "" when none qualifies.
std::string reservedName(bool onDisk) {
    for (int i = 0; dmname[i]; i++)
        if (Player::exists(dmname[i]) == onDisk)
            return dmname[i];
    return "";
}

// Installs a synthetic race for the duration of a test. No production race is ungendered
// and no production subrace is playable, so both paths need injected data.
struct InjectedRace {
    int id;
    explicit InjectedRace(int raceId, const std::string& xml) : id(raceId) {
        xmlDocPtr doc = xmlReadMemory(xml.c_str(), static_cast<int>(xml.size()), nullptr, nullptr, 0);
        EXPECT_NE(doc, nullptr);
        gConfig->races[id] = new RaceData(xmlDocGetRootElement(doc));
        xmlFreeDoc(doc);
    }
    ~InjectedRace() {
        // The parent's makeParent() flag stays set; harmless, since advance only routes to
        // SubRace when a playable non-self subrace exists.
        auto it = gConfig->races.find(id);
        if (it != gConfig->races.end()) {
            delete it->second;
            gConfig->races.erase(it);
        }
    }
};

class CreationWizard : public ::testing::Test {
protected:
    void SetUp() override {
        if (!gameConfigReady())
            GTEST_SKIP() << "game config not available (set REALMS_BASE_PATH)";
    }
};

// Reaches the Prof step as a Human Fighter with self-chosen stats.
void walkHumanFighter(Wizard& wizard) {
    wizard.pick(std::to_string(HUMAN));
    wizard.send("m");
    wizard.pick(std::to_string(static_cast<int>(CreatureClass::FIGHTER)));
    wizard.pick("highport");
    wizard.send("c");
    wizard.send("12 12 12 10 10");
    wizard.send("a");
    wizard.send("b");
}

TEST_F(CreationWizard, CollectsHumanFighterChoices) {
    auto sock = driveWizard({
        "e",                // Human
        "m",                // male
        "d",                // Fighter
        "a",                // start location: first option (Highport)
        "c",                // choose own stats
        "12 12 12 10 10",   // stats (sum 56)
        "a",                // bonus stat: STR (Human is a bonus-stat race)
        "b",                // penalty stat: DEX
    });

    const CharacterDraft& d = sock->getCreationDraft();
    EXPECT_EQ(d.race, HUMAN);
    EXPECT_EQ(d.sex, SEX_MALE);
    EXPECT_EQ(d.cClass, CreatureClass::FIGHTER);
    EXPECT_EQ(d.secondClass, CreatureClass::NONE);
    EXPECT_EQ(d.startLocation, "highport");
    EXPECT_FALSE(d.usePredefinedStats);
    EXPECT_EQ(d.baseStats, (std::array<int, 5>{12, 12, 12, 10, 10}));
    EXPECT_EQ(d.bonusStat, STR);
    EXPECT_EQ(d.penaltyStat, DEX);

    // reaching Prof auto-grants bare-hand
    EXPECT_NE(std::find(d.weaponSkills.begin(), d.weaponSkills.end(), "bare-hand"),
              d.weaponSkills.end());
}

TEST_F(CreationWizard, CollectsWeaponProficiency) {
    auto sock = driveWizard({
        "e", "m", "d", "a", "c", "12 12 12 10 10", "a", "b",
        "a",   // first weapon proficiency
    });
    const CharacterDraft& d = sock->getCreationDraft();
    EXPECT_GE(d.weaponSkills.size(), 2u);   // bare-hand + the pick
}

TEST_F(CreationWizard, FullWizardMaterializesCharacter) {
    auto sock = driveWizard({
        "e", "m", "d", "a", "c", "12 12 12 10 10", "a", "b",
        "a", "b",       // weapon proficiencies (fighter picks two)
        "Wizfighter",   // name
    });
    const CharacterDraft& d = sock->getCreationDraft();
    ASSERT_EQ(d.name, "Wizfighter");   // confirms we reached the name step

    Account account{"wizacct"};
    auto p = Create::materialize(d, account);
    ASSERT_TRUE(p);
    EXPECT_EQ(p->getClass(), CreatureClass::FIGHTER);
    EXPECT_EQ(p->getRace(), HUMAN);
    EXPECT_EQ(p->getName(), "Wizfighter");
    EXPECT_EQ(p->getLevel(), 1);
    EXPECT_TRUE(p->knowsSkill("bare-hand"));
}

TEST_F(CreationWizard, PredefinedStatsPath) {
    auto sock = driveWizard({ "e", "m", "d", "a", "u" });
    const CharacterDraft& d = sock->getCreationDraft();
    EXPECT_EQ(d.cClass, CreatureClass::FIGHTER);
    EXPECT_TRUE(d.usePredefinedStats);
    EXPECT_GT(d.baseStats[0], 0);   // class defaults were applied
}

// Predefined stats enter the Prof step through a different transition than the custom
// path; the auto-grants must still run exactly once.
TEST_F(CreationWizard, PredefinedStatsGrantsOnce) {
    Wizard wizard;
    wizard.pick(std::to_string(HUMAN));
    wizard.send("m");
    wizard.pick(std::to_string(static_cast<int>(CreatureClass::FIGHTER)));
    wizard.pick("highport");
    wizard.send("u");

    ASSERT_EQ(wizard.step(), Step::Prof);
    const auto& skills = wizard.draft().weaponSkills;
    EXPECT_EQ(std::count(skills.begin(), skills.end(), "bare-hand"), 1);
    const std::set<std::string> unique(skills.begin(), skills.end());
    EXPECT_EQ(unique.size(), skills.size());

    const std::size_t before = skills.size();
    wizard.pick(Flow::options(wizard.draft(), Step::Prof).front().value);
    EXPECT_EQ(wizard.draft().weaponSkills.size(), before + 1);
}

TEST_F(CreationWizard, SecondProfExcludesFirstPick) {
    Wizard wizard;
    walkHumanFighter(wizard);
    ASSERT_EQ(wizard.step(), Step::Prof);

    const std::string first = Flow::options(wizard.draft(), Step::Prof).front().value;
    wizard.pick(first);
    ASSERT_EQ(wizard.step(), Step::SecondProf);

    const auto remaining = Flow::options(wizard.draft(), Step::SecondProf);
    EXPECT_TRUE(std::none_of(remaining.begin(), remaining.end(),
                             [&first](const Flow::ViewOption& o) { return o.value == first; }));

    const std::string second = remaining.front().value;
    wizard.pick(second);
    EXPECT_EQ(wizard.step(), Step::Name);
    const auto& skills = wizard.draft().weaponSkills;
    EXPECT_EQ(std::count(skills.begin(), skills.end(), first), 1);
    EXPECT_EQ(std::count(skills.begin(), skills.end(), second), 1);
}

// Socket::restoreState re-enters the current step with an empty line after the pager.
TEST_F(CreationWizard, EmptyInputRerendersWithoutMutation) {
    Wizard wizard;
    const auto expectUnchanged = [&wizard] {
        const CharacterDraft before = wizard.draft();
        const Step stepBefore = wizard.step();
        wizard.send("");
        EXPECT_EQ(wizard.draft(), before);
        EXPECT_EQ(wizard.step(), stepBefore);
    };

    expectUnchanged();
    wizard.pick(std::to_string(HUMAN));
    expectUnchanged();
    wizard.send("m");
    expectUnchanged();
    wizard.pick(std::to_string(static_cast<int>(CreatureClass::FIGHTER)));
    expectUnchanged();
    wizard.pick("highport");
    expectUnchanged();
    wizard.send("c");
    wizard.send("12 12 12 10 10");
    wizard.send("a");
    wizard.send("b");
    ASSERT_EQ(wizard.step(), Step::Prof);
    expectUnchanged();
}

TEST_F(CreationWizard, InvalidLetterRepromptsWithoutMutation) {
    Wizard wizard;
    const auto expectRejected = [&wizard] {
        const CharacterDraft before = wizard.draft();
        const Step stepBefore = wizard.step();
        wizard.send(wizard.pastEndLetter());
        EXPECT_EQ(wizard.draft(), before);
        EXPECT_EQ(wizard.step(), stepBefore);
    };

    expectRejected();
    wizard.pick(std::to_string(HUMAN));
    wizard.send("m");
    expectRejected();
    wizard.pick(std::to_string(static_cast<int>(CreatureClass::FIGHTER)));
    expectRejected();
    wizard.pick("highport");
    wizard.send("c");
    wizard.send("12 12 12 10 10");
    wizard.send("a");
    wizard.send("b");
    ASSERT_EQ(wizard.step(), Step::Prof);
    expectRejected();
}

TEST_F(CreationWizard, GenderedSubraceAsksSex) {
    constexpr int kChildId = 99;
    const InjectedRace child(kChildId,
        R"(<Race id="99" name="Testkin"><ParentRace>1</ParentRace></Race>)");

    Wizard wizard;
    wizard.pick(std::to_string(DWARF));
    ASSERT_EQ(wizard.step(), Step::SubRace);

    wizard.pick(std::to_string(kChildId));
    EXPECT_EQ(wizard.draft().race, kChildId);
    ASSERT_EQ(wizard.step(), Step::Sex);

    wizard.send("f");
    EXPECT_EQ(wizard.draft().sex, SEX_FEMALE);
    EXPECT_EQ(wizard.step(), Step::Class);
}

TEST_F(CreationWizard, UngenderedRaceSkipsSexWithSexNone) {
    constexpr int kRaceId = 98;
    const InjectedRace ungendered(kRaceId,
        R"(<Race id="98" name="Testgolem"><Data><Gendered>0</Gendered></Data></Race>)");

    Wizard wizard;
    wizard.pick(std::to_string(kRaceId));
    EXPECT_EQ(wizard.draft().race, kRaceId);
    EXPECT_EQ(wizard.draft().sex, SEX_NONE);
    EXPECT_EQ(wizard.step(), Step::Class);
}

// A still-free DM name is accepted at the Name step and gated by the DM password.
TEST_F(CreationWizard, DmNameRoutesToDmPasswordStep) {
    const std::string name = reservedName(false);
    if(name.empty())
        GTEST_SKIP() << "every reserved DM name has a player file on disk";
    auto sock = driveWizard({
        "e", "m", "d", "a", "c", "12 12 12 10 10", "a", "b",
        "a", "b",
        name,
    });
    EXPECT_EQ(sock->getCreationDraft().name, name);
    EXPECT_EQ(sock->getCreationSession().step, Step::DmPassword);
    EXPECT_NE(sock->getState(), CON_PLAYING);
}

TEST_F(CreationWizard, ExistingDmNameIsRejectedAtNameStep) {
    const std::string name = reservedName(true);
    if(name.empty())
        GTEST_SKIP() << "no reserved DM name has a player file on disk";
    auto sock = driveWizard({
        "e", "m", "d", "a", "c", "12 12 12 10 10", "a", "b",
        "a", "b",
        name,
    });
    EXPECT_TRUE(sock->getCreationDraft().name.empty());
    EXPECT_EQ(sock->getCreationSession().step, Step::Name);
    EXPECT_NE(sock->getState(), CON_PLAYING);
}

// createPlayer is reached from any state via Socket::restoreState after the pager.
TEST_F(CreationWizard, IgnoresInputOutsideCreationState) {
    Wizard w;
    w.sock->setState(CON_PLAYING);
    const CharacterDraft before = w.sock->getCreationDraft();
    createPlayer(w.sock, "");
    createPlayer(w.sock, "e");
    EXPECT_EQ(w.sock->getState(), CON_PLAYING);
    EXPECT_EQ(w.sock->getCreationDraft(), before);
}

TEST_F(CreationWizard, RejectsBadNameStaysAtNameStep) {
    auto sock = driveWizard({
        "e", "m", "d", "a", "c", "12 12 12 10 10", "a", "b",
        "a", "b",
        "Bo",   // too short -> rejected
    });
    EXPECT_TRUE(sock->getCreationDraft().name.empty());
    EXPECT_NE(sock->getState(), CON_PLAYING);
}

// A race/class with exactly one legal start location must still record the StartLoc
// key into the draft (regression: the auto-pick branch used to skip the bind key, so
// materialize never ran initialBind for these characters).
TEST_F(CreationWizard, SingleOptionLocationRecordsDraftKey) {
    Wizard wizard;
    wizard.pick(std::to_string(MINOTAUR));   // minotaurs start in ruhrdan, nowhere else
    wizard.send("m");
    wizard.pick(std::to_string(static_cast<int>(CreatureClass::FIGHTER)));

    EXPECT_EQ(wizard.draft().startLocation, "ruhrdan");
    EXPECT_EQ(wizard.step(), Step::StatsChoice);   // the Location menu was skipped
}

// Regression for the renderWeaponMenu/processWeaponChoice split: CON_CHOSING_WEAPONS still learns the chosen skill.
TEST_F(CreationWizard, InGameWeaponTrainingLearnsSkill) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    nonBlock(fds[0]);
    nonBlock(fds[1]);

    auto sock = std::make_shared<Socket>(fds[0]);
    auto player = makePlayer(CreatureClass::FIGHTER);
    player->setName("Wpntrainer");
    player->setLevel(5);
    player->setWeaponTrains(1);
    player->setSock(sock);
    sock->setPlayer(player);
    sock->setState(CON_CHOSING_WEAPONS);

    // The menu grants bare-hand before it is built, so it is never offered.
    const auto knows = [&player](const std::string& name) {
        return name == "bare-hand" || player->knowsSkill(name);
    };
    const auto options = Create::weaponMenuOptions(player->getClass(), player->getSecondClass(),
                                                   player->getDeity(), player->getLevel(), knows);
    ASSERT_FALSE(options.empty());
    const std::string expected = options.front().value;

    convertNewWeaponSkills(sock, "a");   // pick the first listed weapon skill
    sock->flush();
    drainFd(fds[1]);
    ::close(fds[1]);

    EXPECT_EQ(player->getWeaponTrains(), 0u);   // a train was consumed
    EXPECT_TRUE(player->knowsSkill("bare-hand"));
    EXPECT_TRUE(player->knowsSkill(expected));
}

// Monks and werewolves have a single innate option instead of the weapon grid.
TEST_F(CreationWizard, InGameInnateWeaponTrainingLearnsBareHand) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    nonBlock(fds[0]);
    nonBlock(fds[1]);

    auto sock = std::make_shared<Socket>(fds[0]);
    auto player = makePlayer(CreatureClass::MONK);
    player->setName("Wpnmonk");
    player->setLevel(5);
    player->setWeaponTrains(1);
    player->setSock(sock);
    sock->setPlayer(player);
    sock->setState(CON_CHOSING_WEAPONS);

    convertNewWeaponSkills(sock, "a");
    sock->flush();
    drainFd(fds[1]);
    ::close(fds[1]);

    EXPECT_EQ(player->getWeaponTrains(), 0u);
    EXPECT_TRUE(player->knowsSkill("bare-hand"));
}

TEST_F(CreationWizard, DoneCommitsCharacterIntoPlay) {
    namespace fs = std::filesystem;
    const std::string acctName = "zzdoneacct";
    const std::string charName = "Zzdonechar";
    const fs::path playerFile = fs::path(Path::Player) / (charName + ".xml");
    const fs::path acctFile = fs::path(Path::Account) / (acctName + ".json");
    fs::remove(playerFile);
    fs::remove(acctFile);

    auto sock = driveWizard({
        "e", "m", "d", "a", "c", "12 12 12 10 10", "a", "b",
        "a", "b", charName,
        "",   // continue past the policy screen -> Create::processDone
    }, acctName);

    EXPECT_EQ(sock->getState(), CON_PLAYING);
    ASSERT_TRUE(sock->getPlayer());
    EXPECT_EQ(sock->getPlayer()->getName(), charName);
    EXPECT_EQ(sock->getPlayer()->getClass(), CreatureClass::FIGHTER);

    auto acct = gServer->getOrLoadAccount(acctName);
    ASSERT_TRUE(acct);
    EXPECT_TRUE(acct->hasCharacter(charName));

    EXPECT_TRUE(fs::exists(playerFile));   // done() persisted the character

    fs::remove(playerFile);
    fs::remove(acctFile);
}

} // namespace
