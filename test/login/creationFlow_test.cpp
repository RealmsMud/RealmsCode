// flow tests need REALMS_BASE_PATH; JSON tests are data-free.
#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "account.hpp"
#include "config.hpp"
#include "global.hpp"
#include "json.hpp"
#include "paths.hpp"
#include "login/characterDraft.hpp"
#include "login/characterMaterializer.hpp"
#include "login/creationFlow.hpp"
#include "mudObjects/players.hpp"
#include "apiTestSupport.hpp"

namespace Flow = Create::Flow;

extern const char *dmname[];

namespace {

bool gameConfigReady() {
    static const bool ready = [] {
        ensureConfig();
        ensureServer();
        const bool ok = gConfig->loadConfig() && gConfig->loadStartLoc() && gConfig->loadSkillGroups() && gConfig->loadSkills() && gConfig->loadDeities() && gConfig->loadClasses() && gConfig->loadRaces();
        gConfig->loadCalendar();
        return ok;
    }();
    return ready;
}

// GTEST_SKIP returns, so the data gate has to expand inside the test body; the JSON
// cases below must stay runnable without game data.
#define SKIP_WITHOUT_GAME_DATA()                                                                                                                                                                                                               \
    do {                                                                                                                                                                                                                                       \
        if (!gameConfigReady())                                                                                                                                                                                                                \
            GTEST_SKIP() << "game config not available (set REALMS_BASE_PATH)";                                                                                                                                                                \
    } while (false)

// First reserved DM name with a player file on disk; "" when there is none.
std::string takenReservedName() {
    for (int i = 0; dmname[i]; i++)
        if (Player::exists(dmname[i]))
            return dmname[i];
    return "";
}

// A sensible input for a step: first legal option for menus, fixed values otherwise.
std::string valueForStep(const CharacterDraft &d, Flow::Step step) {
    using S = Flow::Step;
    if (step == S::Stats)
        return "12 12 12 10 10"; // sums to 56
    if (step == S::Name)
        return "Flowtestchar";
    const auto opts = Flow::options(d, step);
    return opts.empty() ? "" : opts.front().value;
}

// Walks the default path up to the Name step.
bool walkToName(CharacterDraft &draft, const Account &acct) {
    Flow::Step step = Flow::firstStep();
    for (int guard = 0; step != Flow::Step::Name && step != Flow::Step::Done && guard < 64; guard++) {
        const Flow::AdvanceResult r = Flow::advance(draft, step, valueForStep(draft, step), {.account = &acct});
        if (!r.ok)
            return false;
        step = r.next;
    }
    return step == Flow::Step::Name;
}

class CreationFlow : public ::testing::Test {};

TEST_F(CreationFlow, WalkReplayMaterialize) {
    SKIP_WITHOUT_GAME_DATA();
    Account acct("flowwalk");
    CharacterDraft draft;
    std::vector<Flow::Choice> path;
    Flow::Step step = Flow::firstStep();

    int guard = 0;
    while (step != Flow::Step::Done && guard++ < 64) {
        const std::string value = valueForStep(draft, step);
        const Flow::AdvanceResult r = Flow::advance(draft, step, value, {.account = &acct});
        ASSERT_TRUE(r.ok) << "step=" << Flow::stepToken(step) << " value='" << value << "' err=" << r.error.message;
        path.push_back({step, value});
        step = r.next;
    }
    ASSERT_EQ(step, Flow::Step::Done);

    CharacterDraft replayed;
    const Flow::ReplayResult rr = Flow::replay(path, replayed, &acct);
    EXPECT_TRUE(rr.ok) << rr.message;
    EXPECT_EQ(replayed.name, draft.name);
    EXPECT_EQ(replayed.race, draft.race);
    EXPECT_EQ(replayed.cClass, draft.cClass);

    const auto player = Create::materialize(replayed, acct);
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->getLevel(), 1);
    EXPECT_FALSE(player->getName().empty());
}

// describe() of the first step must build and serialize without dereferencing an
// unset draft (the /start handler does exactly this).
TEST_F(CreationFlow, DescribeRaceSerializes) {
    SKIP_WITHOUT_GAME_DATA();
    CharacterDraft d;
    const Flow::StepView view = Flow::describe(d, Flow::firstStep());
    const json j = view;
    EXPECT_EQ(j.at("step"), "race");
    EXPECT_FALSE(j.at("options").empty());
    EXPECT_FALSE(to_string(j).empty());
}

TEST_F(CreationFlow, ReplayRejectsReorderedPath) {
    SKIP_WITHOUT_GAME_DATA();
    std::vector<Flow::Choice> path = {{Flow::Step::Class, "1"}};
    CharacterDraft d;
    const Flow::ReplayResult rr = Flow::replay(path, d);
    EXPECT_FALSE(rr.ok);
    EXPECT_EQ(rr.code, "stepMismatch");
}

TEST_F(CreationFlow, ReplayRejectsIncompletePath) {
    SKIP_WITHOUT_GAME_DATA();
    const auto raceOpts = Flow::options(CharacterDraft{}, Flow::Step::Race);
    ASSERT_FALSE(raceOpts.empty());
    std::vector<Flow::Choice> path = {{Flow::Step::Race, raceOpts.front().value}};
    CharacterDraft d;
    const Flow::ReplayResult rr = Flow::replay(path, d);
    EXPECT_FALSE(rr.ok);
    EXPECT_EQ(rr.code, "incomplete");
}

TEST_F(CreationFlow, AdvanceRejectsIllegalRace) {
    SKIP_WITHOUT_GAME_DATA();
    CharacterDraft d;
    const Flow::AdvanceResult r = Flow::advance(d, Flow::Step::Race, "99999");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error.code, "invalidRace");
}

TEST_F(CreationFlow, CheckEntryOkForFreshAccount) {
    SKIP_WITHOUT_GAME_DATA();
    Account acct("flowentry");
    Flow::EntryContext ec;
    ec.ip = "127.0.0.1";
    const Flow::EntryResult er = Flow::checkEntry(acct, ec);
    EXPECT_TRUE(er.ok);
    EXPECT_EQ(er.reason, Flow::EntryReason::Ok);
}

// gate is on the character name, not the account
TEST_F(CreationFlow, CheckEntryAllowsDmAccount) {
    SKIP_WITHOUT_GAME_DATA();
    Account acct("Bane"); // a dmname[] entry
    Flow::EntryContext ec;
    ec.ip = "127.0.0.1";
    EXPECT_TRUE(Flow::checkEntry(acct, ec).ok);
}

TEST_F(CreationFlow, AdvanceNameAcceptsFreeName) {
    SKIP_WITHOUT_GAME_DATA();
    Account acct("flowname");
    CharacterDraft draft;
    ASSERT_TRUE(walkToName(draft, acct));

    const Flow::AdvanceResult good = Flow::advance(draft, Flow::Step::Name, "Goodmortalname", {.account = &acct});
    EXPECT_TRUE(good.ok) << good.error.message;
    EXPECT_TRUE(good.done); // Name -> Done
}

// Web rejects a DM name outright. The exists probe is off so the reserved branch is
// reached whether or not that DM character has a player file.
TEST_F(CreationFlow, AdvanceNameRejectsDmName) {
    SKIP_WITHOUT_GAME_DATA();
    Account acct("flowname");
    CharacterDraft draft;
    ASSERT_TRUE(walkToName(draft, acct));

    const Flow::AdvanceResult dm = Flow::advance(draft, Flow::Step::Name, dmname[0], {.account = &acct, .checkNameExists = false});
    EXPECT_FALSE(dm.ok);
    EXPECT_EQ(dm.error.code, "nameReserved");
}

// Telnet accepts a free DM name and the FSM routes it to the DM-password step.
TEST_F(CreationFlow, AdvanceNameRoutesReservedToDmPassword) {
    SKIP_WITHOUT_GAME_DATA();
    Account acct("flowdmname");
    CharacterDraft draft;
    ASSERT_TRUE(walkToName(draft, acct));

    const Flow::AdvanceResult dm = Flow::advance(draft, Flow::Step::Name, dmname[0], {.account = &acct, .checkNameExists = false, .allowReservedName = true});
    EXPECT_TRUE(dm.ok) << dm.error.message;
    EXPECT_EQ(dm.next, Flow::Step::DmPassword);
    EXPECT_FALSE(dm.done);
    EXPECT_EQ(draft.name, dmname[0]);
}

// The taken probe precedes the reserved flag, so an existing DM character is a plain
// nameTaken even for telnet.
TEST_F(CreationFlow, AdvanceNameTakenDmNameIsNotReserved) {
    SKIP_WITHOUT_GAME_DATA();
    const std::string name = takenReservedName();
    if (name.empty())
        GTEST_SKIP() << "no reserved DM name has a player file on disk";

    Account acct("flowdmtaken");
    CharacterDraft draft;
    ASSERT_TRUE(walkToName(draft, acct));

    const Flow::AdvanceResult dm = Flow::advance(draft, Flow::Step::Name, name, {.account = &acct, .allowReservedName = true});
    EXPECT_FALSE(dm.ok);
    EXPECT_EQ(dm.error.code, "nameTaken");
    EXPECT_TRUE(draft.name.empty());
}

// The deity tables are bool[DEITY_COUNT]; an option outside that range means an out-of-bounds read.
TEST_F(CreationFlow, DeityOptionsStayWithinDeityCount) {
    SKIP_WITHOUT_GAME_DATA();
    for (const int sex: {SEX_MALE, SEX_FEMALE}) {
        CharacterDraft d;
        d.race = HUMAN;
        d.cClass = CreatureClass::CLERIC;
        d.sex = sex;

        const auto opts = Flow::options(d, Flow::Step::Deity);
        ASSERT_FALSE(opts.empty()) << "sex=" << sex;
        for (const auto &opt: opts) {
            const int id = std::stoi(opt.value);
            EXPECT_GE(id, 1);
            EXPECT_LT(id, DEITY_COUNT);
            EXPECT_NE(Create::sanitizeDeity(id), 0) << opt.value;
        }
    }
}

TEST_F(CreationFlow, LetterStepOptionsAreNonEmptyAndUnique) {
    SKIP_WITHOUT_GAME_DATA();
    using S = Flow::Step;
    const auto isLetterStep = [](Flow::Step s) { return s == S::Race || s == S::SubRace || s == S::Class || s == S::Deity || s == S::Location || s == S::Prof || s == S::SecondProf; };

    CharacterDraft draft;
    Flow::Step step = Flow::firstStep();
    int guard = 0;
    while (step != S::Done && guard++ < 64) {
        if (isLetterStep(step)) {
            const auto opts = Flow::options(draft, step);
            ASSERT_FALSE(opts.empty()) << Flow::stepToken(step);
            std::vector<std::string> values;
            for (const auto &opt: opts) {
                EXPECT_FALSE(opt.label.empty()) << Flow::stepToken(step);
                values.push_back(opt.value);
            }
            std::sort(values.begin(), values.end());
            EXPECT_EQ(std::adjacent_find(values.begin(), values.end()), values.end()) << "duplicate option value at " << Flow::stepToken(step);
        }
        const Flow::AdvanceResult r = Flow::advance(draft, step, valueForStep(draft, step));
        ASSERT_TRUE(r.ok) << Flow::stepToken(step) << ": " << r.error.message;
        step = r.next;
    }
    EXPECT_EQ(step, S::Done);
}

TEST_F(CreationFlow, AdvanceRejectsTrailingGarbage) {
    SKIP_WITHOUT_GAME_DATA();
    const CharacterDraft before;
    for (const char *value: {"5abc", "abc"}) {
        CharacterDraft d;
        const Flow::AdvanceResult r = Flow::advance(d, Flow::Step::Race, value);
        EXPECT_FALSE(r.ok) << value;
        EXPECT_EQ(r.error.code, "invalidRace");
        EXPECT_EQ(d, before);
    }
}

// walk() with checkNameExists=false skips the on-disk probe for an already-validated
// path; replay() (the commit gate) stays strict and still rejects a now-taken name.
TEST_F(CreationFlow, WalkSkipsExistsProbeReplayStaysStrict) {
    SKIP_WITHOUT_GAME_DATA();
    Account acct("flowprobe");
    const std::string name = "Zzwalkprobe";

    // Walk to the Name step, then append the name choice to the path.
    CharacterDraft draft;
    std::vector<Flow::Choice> path;
    Flow::Step step = Flow::firstStep();
    int guard = 0;
    while (step != Flow::Step::Name && step != Flow::Step::Done && guard++ < 64) {
        const std::string value = valueForStep(draft, step);
        const Flow::AdvanceResult r = Flow::advance(draft, step, value, {.account = &acct});
        ASSERT_TRUE(r.ok) << Flow::stepToken(step) << ": " << r.error.message;
        path.push_back({step, value});
        step = r.next;
    }
    ASSERT_EQ(step, Flow::Step::Name);
    path.push_back({Flow::Step::Name, name});

    const fs::path playerFile = (fs::path(Path::Player) / name).replace_extension("xml");
    {
        std::ofstream stub(playerFile);
        stub << "<Player/>";
    }

    CharacterDraft relaxed;
    Flow::Step next = Flow::firstStep();
    EXPECT_TRUE(Flow::walk(path, relaxed, next, &acct, false).ok); // probe skipped
    EXPECT_EQ(next, Flow::Step::Done);

    CharacterDraft strict;
    const Flow::ReplayResult rr = Flow::replay(path, strict, &acct);
    EXPECT_FALSE(rr.ok); // commit-style replay re-probes
    EXPECT_EQ(rr.code, "nameTaken");

    fs::remove(playerFile);
}

// Service-layer strings are presentation-free: no telnet caret codes in what the
// JSON API serializes.
TEST_F(CreationFlow, PromptsAndErrorsCarryNoColorCodes) {
    SKIP_WITHOUT_GAME_DATA();
    CharacterDraft d;
    d.race = HUMAN;
    EXPECT_EQ(Flow::describe(d, Flow::Step::Sex).prompt.find('^'), std::string::npos);

    const Flow::AdvanceResult bad = Flow::advance(d, Flow::Step::Sex, "x");
    ASSERT_FALSE(bad.ok);
    EXPECT_EQ(bad.error.message.find('^'), std::string::npos);
}

TEST_F(CreationFlow, JsonChoiceRoundTrip) {
    const Flow::Choice c{Flow::Step::Class, "5"};
    const json j = c;
    EXPECT_EQ(j.at("step"), "class");
    EXPECT_EQ(j.at("value"), "5");

    const auto back = j.get<Flow::Choice>();
    EXPECT_EQ(back.step, Flow::Step::Class);
    EXPECT_EQ(back.value, "5");
}

TEST_F(CreationFlow, JsonStepViewSerializes) {
    Flow::StepView v;
    v.step = Flow::Step::Sex;
    v.inputKind = Flow::InputKind::Letter;
    v.prompt = "pick";
    v.options = {{"m", "^WMale^x", ""}, {"f", "Female", ""}};
    Flow::ViewBlock art;
    art.kind = Flow::BlockKind::Art;
    art.text = "ascii";
    v.blocks = {art};

    const json j = v;
    EXPECT_EQ(j.at("step"), "sex");
    EXPECT_EQ(j.at("inputKind"), "letter");
    EXPECT_EQ(j.at("options").size(), 2u);
    EXPECT_EQ(j.at("options")[0].at("value"), "m");
    EXPECT_EQ(j.at("options")[0].at("label"), "^WMale^x"); // caret codes preserved
    EXPECT_EQ(j.at("blocks")[0].at("kind"), "art");
}

TEST_F(CreationFlow, JsonUnknownStepTokenThrows) {
    json j = {{"step", "bogus"}, {"value", "x"}};
    EXPECT_THROW(j.get<Flow::Choice>(), std::exception);
}

} // namespace
