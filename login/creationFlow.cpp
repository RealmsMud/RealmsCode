/*
 * creationFlow.cpp
 *   The character-creation FSM. See creationFlow.hpp.
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

#include <fmt/format.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "login/creationFlow.hpp"
#include "login/characterDraft.hpp"
#include "account.hpp"
#include "config.hpp"
#include "deityData.hpp"
#include "global.hpp"
#include "paths.hpp"
#include "playerClass.hpp"
#include "proto.hpp" // isdm, get_class_string, getFullClassName, up, low
#include "raceData.hpp"
#include "structs.hpp"

namespace Create::Flow {
namespace {
    // -1 for anything that is not a complete decimal integer ("5abc" included).
    int parseInt(const std::string &s) {
        int v = 0;
        const char *const end = s.data() + s.size();
        const auto [ptr, ec] = std::from_chars(s.data(), end, v);
        if (ec != std::errc{} || ptr != end)
            return -1;
        return v;
    }

    std::string classLabel(int classCode) {
        const ClassPair pair = resolveClassPair(classCode);
        if (pair.cClass == CreatureClass::NONE)
            return "";
        if (pair.second != CreatureClass::NONE)
            return getFullClassName(pair.cClass, pair.second);
        return get_class_string(static_cast<int>(pair.cClass));
    }

    // Stat-letter step labels, in CrtStats (STR..PTY) order.
    constexpr std::array<const char *, kStatCount> kStatLabels{"Strength", "Dexterity", "Constitution", "Intelligence", "Piety"};

    std::vector<ViewOption> weaponOptions(const CharacterDraft &d) {
        const auto knows = [&d](const std::string &n) { return std::find(d.weaponSkills.begin(), d.weaponSkills.end(), n) != d.weaponSkills.end(); };
        return weaponMenuOptions(d.cClass, d.secondClass, d.deity, kCreationLevel, knows);
    }

    // mirrors renderProf's side effects so describe() stays pure
    InitialSkills enterProf(CharacterDraft &d) {
        InitialSkills granted;
        const RaceData *race = gConfig->getRace(d.race);
        PlayerClass *pc = classForDraft(d);
        if (race && pc)
            granted = grantInitialWeaponSkills(d, race, pc);
        if (d.cClass != CreatureClass::MONK && d.cClass != CreatureClass::WEREWOLF && std::find(d.weaponSkills.begin(), d.weaponSkills.end(), "bare-hand") == d.weaponSkills.end())
            d.weaponSkills.push_back("bare-hand");
        return granted;
    }

    Step afterRace(CharacterDraft &d) {
        const RaceData *race = gConfig->getRace(d.race);
        if (race && race->isGendered())
            return Step::Sex;
        d.sex = SEX_NONE;
        return Step::Class;
    }

    Step statsEntry(const CharacterDraft &d) {
        PlayerClass *pc = classForDraft(d);
        return (pc && pc->hasDefaultStats()) ? Step::StatsChoice : Step::Stats;
    }

    Step afterClassOrDeity(CharacterDraft &d) {
        const auto locs = startingLocationChoices(d.race, d.cClass, d.deity);
        if (locs.size() == 1) {
            d.startLocation = locs.front().key; // auto-resolved: no Location step
            return statsEntry(d);
        }
        return Step::Location;
    }

    // cached for process lifetime: edits need a restart
    std::string loadConfigText(const std::string &filename) {
        static std::mutex cacheMutex;
        static std::unordered_map<std::string, std::string> cache;

        const std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = cache.find(filename);
        if (it != cache.end())
            return it->second;

        std::string text;
        if (const std::ifstream file(Path::Config / filename); file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            text = ss.str();
        }
        return cache.emplace(filename, std::move(text)).first->second;
    }

    std::vector<std::string> splitLines(const std::string &text) {
        std::vector<std::string> lines;
        std::istringstream iss(text);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(line);
        }
        return lines;
    }

    // frontend may render options between top and bottom
    ViewBlock classBanner(const std::string &raceName) {
        ViewBlock b;
        b.kind = BlockKind::Art;
        b.text = Create::classBannerTop(raceName);
        b.lines = splitLines(b.text);
        return b;
    }

    ViewBlock textBlock(const std::string &text) {
        ViewBlock b;
        b.kind = BlockKind::Text;
        b.text = text;
        return b;
    }
} // namespace

Step firstStep() { return Step::Race; }

std::string stepToken(Step step) {
    switch (step) {
        case Step::DmPassword:
            return "dmPassword";
        case Step::Race:
            return "race";
        case Step::SubRace:
            return "subRace";
        case Step::Sex:
            return "sex";
        case Step::Class:
            return "class";
        case Step::Deity:
            return "deity";
        case Step::Location:
            return "location";
        case Step::StatsChoice:
            return "statsChoice";
        case Step::Stats:
            return "stats";
        case Step::BonusStat:
            return "bonusStat";
        case Step::PenaltyStat:
            return "penaltyStat";
        case Step::Prof:
            return "prof";
        case Step::SecondProf:
            return "secondProf";
        case Step::Name:
            return "name";
        case Step::Done:
            return "done";
    }
    return "";
}

std::optional<Step> stepFromToken(const std::string &token) {
    for (int i = 0; i <= static_cast<int>(Step::Done); i++) {
        const auto s = static_cast<Step>(i);
        if (stepToken(s) == token)
            return s;
    }
    return std::nullopt;
}

EntryResult checkEntry(const Account &account, const EntryContext &ctx) {
    if (gConfig->isCharCreationDisabled())
        return {false, EntryReason::CreationDisabled};
    if (gConfig->races.empty() || gConfig->classes.empty() || gConfig->deities.empty() || gConfig->start.empty())
        return {false, EntryReason::ConfigUnavailable};
    if (gConfig->isSiteLockedOut(ctx.ip, ctx.hostname) != 0) // 1 or 2: web has no lockout-password flow
        return {false, EntryReason::LockedOut};
    if (!account.canCreateCharacter())
        return {false, EntryReason::LimitReached};
    return {true, EntryReason::Ok};
}

std::vector<ViewOption> options(const CharacterDraft &draft, Step step) {
    std::vector<ViewOption> out;
    switch (step) {
        case Step::Race:
            for (const short id: playableRaceChoices())
                out.push_back({std::to_string(id), gConfig->getRace(id)->getName(), ""});
            break;
        case Step::SubRace:
            for (const short id: subraceChoices(draft.race))
                out.push_back({std::to_string(id), gConfig->getRace(id)->getName(), ""});
            break;
        case Step::Sex:
            out.push_back({"m", "Male", ""});
            out.push_back({"f", "Female", ""});
            break;
        case Step::Class:
            for (const int code: allowedClassCodes(draft.race))
                out.push_back({std::to_string(code), classLabel(code), ""});
            break;
        case Step::Deity:
            for (const int id: allowedDeityChoices(draft.race, draft.cClass, draft.secondClass, draft.sex))
                out.push_back({std::to_string(id), gConfig->getDeity(id)->getName(), ""});
            break;
        case Step::Location:
            for (const auto &loc: startingLocationChoices(draft.race, draft.cClass, draft.deity))
                out.push_back({loc.key, loc.label, ""});
            break;
        case Step::StatsChoice:
            out.push_back({"c", "Choose your own stats", ""});
            out.push_back({"u", "Use predefined stats provided by the mud", ""});
            break;
        case Step::BonusStat:
        case Step::PenaltyStat:
            for (std::size_t i = 0; i < kStatLabels.size(); i++)
                out.push_back({std::string(1, static_cast<char>('a' + i)), kStatLabels[i], ""});
            break;
        case Step::Prof:
        case Step::SecondProf:
            out = weaponOptions(draft);
            break;
        case Step::DmPassword:
        case Step::Stats:
        case Step::Name:
        case Step::Done:
            break; // free-text / confirm steps carry no options
    }
    return out;
}

StepView describe(const CharacterDraft &draft, Step step) {
    StepView view;
    view.step = step;
    view.inputKind = InputKind::Letter;
    view.options = options(draft, step);

    switch (step) {
        case Step::Race:
            view.blocks.push_back([] {
                ViewBlock b;
                b.kind = BlockKind::Art;
                b.text = loadConfigText("race_menu.0.txt");
                b.lines = splitLines(b.text);
                return b;
            }());
            view.blocks.push_back([] {
                ViewBlock b;
                b.kind = BlockKind::Art;
                b.text = loadConfigText("race_menu.1.txt");
                b.lines = splitLines(b.text);
                return b;
            }());
            view.prompt = "Choose one: ";
            break;
        case Step::SubRace:
            view.blocks.push_back(textBlock("Choose a sub race:"));
            view.prompt = "Choose one: ";
            break;
        case Step::Sex:
            view.prompt = "Do you wish to be [M] Male or [F] Female? ";
            break;
        case Step::Class:
            view.blocks.push_back(classBanner(gConfig->getRace(draft.race)->getName()));
            view.prompt = "Choose one: ";
            break;
        case Step::Deity:
            if (draft.cClass == CreatureClass::CLERIC)
                view.blocks.push_back(textBlock("Note: Clerics of Linothan must be male. Clerics of Mara must be female."));
            view.blocks.push_back(textBlock("Please choose a deity:"));
            view.prompt = "Choose one: ";
            break;
        case Step::Location:
            view.blocks.push_back(textBlock("Please choose a starting location:"));
            view.prompt = ": ";
            break;
        case Step::StatsChoice:
            view.blocks.push_back(textBlock("For character stats, you may choose your own, or use predefined stats "
                                            "provided by the mud. Beginners unfamiliar with game mechanics are encouraged "
                                            "to use predefined stats."));
            view.prompt = ": ";
            break;
        case Step::Stats:
            view.inputKind = InputKind::Stats;
            view.blocks.push_back(textBlock(fmt::format("You have {} points to distribute among your {} stats. Enter your {} numbers "
                                                        "in order: Strength, Dexterity, Constitution, Intelligence, Piety. No stat "
                                                        "may be smaller than {} or larger than {}. Format: ## ## ## ## ##",
                                                        kStatTotal, kStatCount, kStatCount, kMinStat, kMaxStat)));
            view.prompt = ": ";
            break;
        case Step::BonusStat:
            view.blocks.push_back(textBlock("Raise which stat?"));
            view.prompt = ": ";
            break;
        case Step::PenaltyStat:
            view.blocks.push_back(textBlock("Choose your stat to lower:"));
            view.prompt = ": ";
            break;
        case Step::Prof: {
            PlayerClass *pc = classForDraft(draft);
            const int profs = pc ? pc->numProfs() : 1;
            view.blocks.push_back(textBlock(profs > 1 ? fmt::format("Pick {} weapon skills:", profs) : "Choose a weapon skill:"));
            view.prompt = ": ";
            break;
        }
        case Step::SecondProf:
            view.blocks.push_back(textBlock("Pick a second weapon skill:"));
            view.prompt = ": ";
            break;
        case Step::Name:
            view.inputKind = InputKind::Text;
            view.blocks.push_back(textBlock("Choose a name appropriate for a fantasy setting. Names that are offensive, "
                                            "reference real-world people/places, or are otherwise inappropriate may be "
                                            "changed by staff."));
            view.prompt = "Please choose a character name: ";
            break;
        case Step::Done:
            view.inputKind = InputKind::Confirm;
            view.blocks.push_back([] {
                ViewBlock b;
                b.kind = BlockKind::Art;
                b.text = loadConfigText("policy_login.txt");
                b.lines = splitLines(b.text);
                return b;
            }());
            view.prompt = "[Press Enter to Continue]";
            break;
        case Step::DmPassword:
            view.inputKind = InputKind::Text;
            view.prompt = "Please enter password: ";
            break;
    }
    return view;
}

AdvanceResult advance(CharacterDraft &d, Step step, const std::string &value, const AdvanceOptions &opts) {
    AdvanceResult result;
    const auto ok = [&](Step next) {
        result.ok = true;
        result.error = {};
        result.next = next;
        result.done = (next == Step::Done);
        return result;
    };
    const auto err = [&](std::string code, std::string msg) {
        result.ok = false;
        result.error = {false, std::move(code), std::move(msg)};
        result.next = step;
        result.done = false;
        return result;
    };
    const auto enterProfStep = [&] {
        result.granted = enterProf(d);
        return ok(Step::Prof);
    };

    switch (step) {
        case Step::Race: {
            const int id = parseInt(value);
            const auto choices = playableRaceChoices();
            if (id < 0 || std::find(choices.begin(), choices.end(), static_cast<short>(id)) == choices.end())
                return err("invalidRace", "Invalid race selection.");
            d.race = static_cast<short>(id);
            const RaceData *race = gConfig->getRace(d.race);
            if (race && race->isParent()) {
                const auto subs = subraceChoices(d.race);
                if (std::any_of(subs.begin(), subs.end(), [&](short s) { return s != d.race; }))
                    return ok(Step::SubRace);
            }
            return ok(afterRace(d));
        }
        case Step::SubRace: {
            const int id = parseInt(value);
            const auto subs = subraceChoices(d.race);
            if (id < 0 || std::find(subs.begin(), subs.end(), static_cast<short>(id)) == subs.end())
                return err("invalidSubRace", "Invalid sub race selection.");
            d.race = static_cast<short>(id);
            return ok(afterRace(d));
        }
        case Step::Sex: {
            const RaceData *race = gConfig->getRace(d.race);
            const bool gendered = race && race->isGendered();
            const SexResult r = validateSex(value.empty() ? 0 : value[0], gendered);
            if (!r.ok)
                return err("invalidSex", "Please choose [M] Male or [F] Female.");
            d.sex = r.sex;
            return ok(Step::Class);
        }
        case Step::Class: {
            const int code = parseInt(value);
            const auto codes = allowedClassCodes(d.race);
            if (code < 0 || std::find(codes.begin(), codes.end(), code) == codes.end())
                return err("invalidClass", "Invalid class selection.");
            const ClassPair pair = resolveClassPair(code);
            if (pair.cClass == CreatureClass::NONE)
                return err("invalidClass", "Invalid class selection.");
            d.cClass = pair.cClass;
            d.secondClass = pair.second;
            PlayerClass *pc = classForDraft(d);
            if (pc && pc->needsDeity())
                return ok(Step::Deity);
            return ok(afterClassOrDeity(d));
        }
        case Step::Deity: {
            const int id = parseInt(value);
            const auto deities = allowedDeityChoices(d.race, d.cClass, d.secondClass, d.sex);
            if (id < 0 || std::find(deities.begin(), deities.end(), id) == deities.end())
                return err("invalidDeity", "Invalid deity selection.");
            const int dn = sanitizeDeity(id);
            if (!dn)
                return err("invalidDeity", "Invalid deity selection.");
            d.deity = dn;
            return ok(afterClassOrDeity(d));
        }
        case Step::Location: {
            const auto locs = startingLocationChoices(d.race, d.cClass, d.deity);
            if (std::none_of(locs.begin(), locs.end(), [&](const StartLocOption &o) { return o.key == value; }))
                return err("invalidLocation", "Invalid selection.");
            d.startLocation = value;
            return ok(statsEntry(d));
        }
        case Step::StatsChoice: {
            switch (parseStatsChoice(value)) {
                case StatsChoiceKind::Custom:
                    return ok(Step::Stats);
                case StatsChoiceKind::Predefined: {
                    PlayerClass const *pc = classForDraft(d);
                    if (!pc)
                        return ok(Step::Stats);
                    const std::array<short, 5> defaults = pc->getDefaultBaseStats();
                    for (size_t i = 0; i < defaults.size(); i++)
                        d.baseStats[i] = defaults[i];
                    d.usePredefinedStats = true;
                    return enterProfStep(); // predefined stats skip bonus/penalty
                }
                case StatsChoiceKind::Invalid:
                    return err("invalidStatsChoice", "Please choose C or U.");
            }
            return err("invalidStatsChoice", "Please choose C or U.");
        }
        case Step::Stats: {
            const StatsResult r = parseStats(value);
            switch (r.error) {
                case StatsError::TooFew:
                    return err("statsTooFew", fmt::format("Please enter all {} numbers.", kStatCount));
                case StatsError::OutOfRange:
                    return err("statsOutOfRange", fmt::format("No stats < {} or > {} please.", kMinStat, kMaxStat));
                case StatsError::BadSum:
                    return err("statsBadSum", fmt::format("Stat total must equal {} points, yours totaled {}.", kStatTotal, r.sum));
                case StatsError::Ok:
                    break;
            }
            d.usePredefinedStats = false;
            for (std::size_t i = 0; i < r.stats.size(); i++)
                d.baseStats[i] = r.stats[i];
            const RaceData *race = gConfig->getRace(d.race);
            if (race && race->bonusStat())
                return ok(Step::BonusStat);
            return enterProfStep();
        }
        case Step::BonusStat: {
            const StatModResult r = validateStatModifier(value.empty() ? 0 : value[0]);
            if (!r.ok)
                return err("invalidStat", "Choose A through E.");
            d.bonusStat = r.statId;
            return ok(Step::PenaltyStat);
        }
        case Step::PenaltyStat: {
            const StatModResult r = validateStatModifier(value.empty() ? 0 : value[0]);
            if (!r.ok)
                return err("invalidStat", "Choose A through E.");
            d.penaltyStat = r.statId;
            return enterProfStep();
        }
        case Step::Prof:
        case Step::SecondProf: {
            const auto opts = weaponOptions(d);
            if (std::none_of(opts.begin(), opts.end(), [&](const ViewOption &o) { return o.value == value; }))
                return err("invalidWeapon", "Choose a weapon skill.");
            d.weaponSkills.push_back(value);
            if (step == Step::Prof) {
                PlayerClass *pc = classForDraft(d);
                if (pc && pc->numProfs() == 2)
                    return ok(Step::SecondProf);
            }
            return ok(Step::Name);
        }
        case Step::Name: {
            const CharNameResult r = validateNewCharacterName(value, opts.account, opts.checkNameExists);
            switch (r.error) {
                case CharNameError::Empty:
                    return err("nameEmpty", "Name cannot be empty.");
                case CharNameError::TooLong:
                    return err("nameTooLong", fmt::format("Name too long (max {} characters).", kMaxCharNameLen));
                case CharNameError::BadName:
                    return err("nameBadFormat", AccountAuth::nameErrorMessage(r.nameError));
                case CharNameError::Reserved:
                    if (!opts.allowReservedName)
                        return err("nameReserved", "DM characters must be created in the game, not on the web.");
                    d.name = r.normalized;
                    return ok(Step::DmPassword); // telnet-only detour; the password step gates the name
                case CharNameError::Taken:
                    return err("nameTaken", "That character name is already taken.");
                case CharNameError::InAccount:
                    return err("nameInAccount", "You already have a character with that name.");
                case CharNameError::Ok:
                    break;
            }
            d.name = r.normalized;
            return ok(Step::Done);
        }
        case Step::Done:
            result.ok = true;
            result.done = true;
            result.next = Step::Done;
            return result;
        case Step::DmPassword:
            return err("dmPassword", "DM accounts must create characters via the game.");
    }
    return err("unknownStep", "Unknown creation step.");
}

ReplayResult walk(const std::vector<Choice> &path, CharacterDraft &outDraft, Step &outNext, const Account *account, bool checkNameExists) {
    CharacterDraft draft;
    Step expected = firstStep();

    for (const auto &choice: path) {
        if (choice.step != expected)
            return {false, expected, "stepMismatch", fmt::format("Expected step '{}'.", stepToken(expected))};
        const AdvanceResult r = advance(draft, expected, choice.value, {.account = account, .checkNameExists = checkNameExists});
        if (!r.ok)
            return {false, expected, r.error.code, r.error.message};
        expected = r.next;
    }
    outDraft = draft;
    outNext = expected;
    return {true, expected, "", ""};
}

ReplayResult replay(const std::vector<Choice> &path, CharacterDraft &outDraft, const Account *account) {
    Step next = firstStep();
    const ReplayResult r = walk(path, outDraft, next, account, /*checkNameExists*/ true);
    if (!r.ok)
        return r;
    if (next != Step::Done)
        return {false, next, "incomplete", fmt::format("Creation is not complete (awaiting '{}').", stepToken(next))};
    return {true, Step::Done, "", ""};
}
} // namespace Create::Flow
