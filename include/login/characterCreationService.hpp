/*
 * characterCreationService.hpp
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

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "global.hpp"                    // CreatureClass
#include "login/accountAuthService.hpp"  // AccountAuth::NameError

struct CharacterDraft;
class Account;
class RaceData;
class PlayerClass;

namespace Create {
    inline constexpr int kMinStat = 3;
    inline constexpr int kMaxStat = 18;
    inline constexpr int kStatTotal = 56;
    inline constexpr int kStatScale = 10;          // player-facing stat points are stored x10
    inline constexpr std::size_t kStatCount = 5;
    inline constexpr std::size_t kMaxCharNameLen = 20;
    inline constexpr int kCreationLevel = 1;       // weapon menus during creation are level-1 menus

    struct InitialSkills {
        std::string raceSkills;
        short raceCount = 0;
        std::string classSkills;
        short classCount = 0;
    };
    InitialSkills grantInitialWeaponSkills(CharacterDraft& draft, const RaceData* race, PlayerClass* pClass);

    struct WeaponSkillInfo {
        std::string group;
        std::string groupDisplay;
        std::string name;
        std::string displayName;
    };
    std::vector<WeaponSkillInfo> availableWeaponSkills(CreatureClass cls, CreatureClass second, int deity, int level, const std::function<bool(const std::string&)>& isKnown);

    // A menu entry: value is the token the flow consumes (never the rendered letter),
    // group buckets weapon skills.
    struct Option {
        std::string value;
        std::string label;
        std::string group;
    };
    using WeaponOption = Option;

    // Weapon menu for a Prof step or in-game training: werewolf/monk get their single innate
    // option, everyone else the learnable list. Callers grant bare-hand before asking.
    std::vector<WeaponOption> weaponMenuOptions(CreatureClass cls, CreatureClass second, int deity, int level,
                                                const std::function<bool(const std::string&)>& isKnown);

    // The class-menu ASCII art. Telnet prints both halves around the menu; the web view
    // takes the top only and renders its own options between them.
    std::string classBannerTop(const std::string& raceName);
    std::string classBannerBottom();

    std::vector<short> playableRaceChoices();                  // playable, non-subrace races
    std::vector<short> subraceChoices(short selectedRaceId);   // the race itself + its playable subraces

    struct StartLocOption { std::string key; std::string label; };
    std::vector<StartLocOption> startingLocationChoices(short raceId, CreatureClass cClass, int deity);
    std::vector<int>  allowedClassCodes(short raceId);         // class menu codes (1..CLASS_COUNT_MULT) the race allows
    std::vector<int>  allowedDeityChoices(short raceId, CreatureClass cClass, CreatureClass second, int sex);
    int sanitizeDeity(int deity);

    std::string draftClassKey(const CharacterDraft& draft);
    PlayerClass* classForDraft(const CharacterDraft& draft);

    struct ClassPair {
        CreatureClass cClass = CreatureClass::NONE;
        CreatureClass second = CreatureClass::NONE;
    };
    ClassPair resolveClassPair(int classCode);

    struct SexResult { bool ok = false; int sex = 0; };
    SexResult validateSex(char input, bool isGendered);

    enum class StatsChoiceKind { Invalid, Custom, Predefined };
    StatsChoiceKind parseStatsChoice(const std::string& input);

    // TooFew means "not exactly kStatCount integer tokens": a non-numeric token or a sixth number too.
    enum class StatsError { Ok, TooFew, OutOfRange, BadSum };
    struct StatsResult { StatsError error = StatsError::Ok; int sum = 0; std::array<int, kStatCount> stats{}; };
    StatsResult parseStats(const std::string& input);

    struct StatModResult { bool ok = false; int statId = 0; };
    StatModResult validateStatModifier(char input);

    std::array<int, kStatCount> computeFinalStats(const CharacterDraft& draft, const RaceData* race);

    enum class CharNameError { Ok, Empty, TooLong, BadName, Reserved, Taken, InAccount };
    struct CharNameResult {
        CharNameError error = CharNameError::Ok;
        AccountAuth::NameError nameError = AccountAuth::NameError::Ok;
        std::string normalized;
    };
    CharNameResult validateNewCharacterName(const std::string& raw, const Account* account, bool checkExists = true);
}
