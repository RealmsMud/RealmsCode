/*
 * characterCreationService.cpp
 *   Headless character-creation logic. See characterCreationService.hpp.
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
#include <cctype>
#include <charconv>
#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "login/characterCreationService.hpp"
#include "login/characterDraft.hpp"
#include "account.hpp"
#include "config.hpp"
#include "global.hpp"
#include "structs.hpp"
#include "raceData.hpp"
#include "playerClass.hpp"
#include "skillGain.hpp"
#include "mudObjects/players.hpp"
#include "proto.hpp" // lowercize

namespace Create {

// Parallel to the class/race weapon-skill tables; keep in sync with grantInitialWeaponSkills.
// classMode applies the deity restrictions class skills carry.
static std::string draftInitialWeaponSkills(std::list<SkillGain *>::const_iterator begin, std::list<SkillGain *>::const_iterator end, CharacterDraft &draft, bool classMode, short &num) {
    std::ostringstream oStr;
    for (auto it = begin; it != end; ++it) {
        SkillGain *sGain = *it;
        if (sGain->getName().empty())
            continue;
        const SkillInfo *skill = gConfig->getSkill(sGain->getName());
        if (!skill) { // base skill missing from skills.xml: skip it rather than crash the Prof step
            std::clog << "Unknown base skill '" << sGain->getName() << "' in creation tables.\n";
            continue;
        }
        if (!skill->getGroup().starts_with("weapons"))
            continue;

        const bool known = std::find(draft.weaponSkills.begin(), draft.weaponSkills.end(), sGain->getName()) != draft.weaponSkills.end();
        if (classMode && (known || (draft.deity && !sGain->deityIsAllowed(draft.deity))))
            continue;

        num++;
        oStr << (num == 1 ? "" : ", ") << skill->getDisplayName();

        const bool deityOk = !sGain->hasDeities() || sGain->deityIsAllowed(draft.deity);
        if ((!classMode || deityOk) && !known)
            draft.weaponSkills.push_back(sGain->getName());
    }
    if (num > 0)
        oStr << ".";
    return oStr.str();
}

InitialSkills grantInitialWeaponSkills(CharacterDraft &draft, const RaceData *race, PlayerClass *pClass) {
    InitialSkills out;
    out.raceSkills = draftInitialWeaponSkills(race->getSkillBegin(), race->getSkillEnd(), draft, false, out.raceCount);
    out.classSkills = draftInitialWeaponSkills(pClass->getSkillBegin(), pClass->getSkillEnd(), draft, true, out.classCount);
    return out;
}

std::vector<WeaponSkillInfo> availableWeaponSkills(CreatureClass cls, CreatureClass second, int deity, int level, const std::function<bool(const std::string &)> &isKnown) {
    std::vector<WeaponSkillInfo> out;
    for (const auto &[curGroup, curGroupDisplay]: gConfig->skillGroups) {
        if (!curGroup.starts_with("weapons") || curGroup.length() <= 7)
            continue;
        if (cls == CreatureClass::CLERIC && deity == CERIS && (boost::contains(curGroup, "slashing") || boost::contains(curGroup, "piercing")))
            continue;

        for (const auto &[skillName, curSkill]: gConfig->skills) {
            if (curSkill.getGroup() != curGroup)
                continue;
            if (curSkill.getName() == "claw")
                continue;
            if (cls == CreatureClass::CLERIC && deity == CERIS && curSkill.getName() == "whip")
                continue;
            if (isKnown(curSkill.getName()))
                continue;
            if (level < 4 && (curSkill.getName() == "arcane-weapon" || curSkill.getName() == "divine-weapon"))
                continue;
            if (!(cls == CreatureClass::MAGE || cls == CreatureClass::LICH || cls == CreatureClass::BARD || second == CreatureClass::MAGE) && curSkill.getName() == "arcane-weapon")
                continue;
            if (!(cls == CreatureClass::CLERIC || cls == CreatureClass::PALADIN || cls == CreatureClass::DRUID || cls == CreatureClass::DEATHKNIGHT) && curSkill.getName() == "divine-weapon")
                continue;

            out.push_back({curGroup, curGroupDisplay, curSkill.getName(), curSkill.getDisplayName()});
        }
    }
    return out;
}

std::vector<WeaponOption> weaponMenuOptions(CreatureClass cls, CreatureClass second, int deity, int level, const std::function<bool(const std::string &)> &isKnown) {
    if (cls == CreatureClass::WEREWOLF)
        return {{"claw", "Claws", "Slashing Weapons"}};
    if (cls == CreatureClass::MONK)
        return {{"bare-hand", "Bare-Handed", "Crushing Weapons"}};

    std::vector<WeaponOption> out;
    for (const auto &skill: availableWeaponSkills(cls, second, deity, level, isKnown))
        out.push_back({skill.name, skill.displayName, skill.groupDisplay});
    return out;
}

std::string classBannerTop(const std::string &raceName) {
    return fmt::format("   ____________\n"
                       "  /------------\\            VVVVVVVVVVVVVVVV\n"
                       " // ___    ___ \\\\   {:>12}:  Choose A Class\n"
                       " \\_/   \\  /   \\_/           ^^^^^^^^^^^^^^^^\n"
                       " ______/  \\__________________________________________________\n"
                       "<______    ________\\\\\\\\_____\\\\\\\\_____\\\\\\\\_____\\\\\\\\_____\\\\\\\\__]\n"
                       "  _    \\  /    _\n"
                       " / \\___/  \\___/ \\\n"
                       " \\\\____________//\n"
                       "  `------------'",
                       raceName);
}

std::string classBannerBottom() {
    return "\n                                                         |\\\n"
           "                                                         <<\\         _\n"
           "                                                          / \\       //\n"
           " .--------------------------------------------------------{o}______/|\n"
           "<       -===========================================:::{*}///////////]\n"
           " `--------------------------------------------------------{o}~~~~~~\\|\n"
           "                                                          \\ /       \\\\\n"
           "                                                         <</         -\n"
           "                                                         |/\n";
}

std::string draftClassKey(const CharacterDraft &draft) {
    std::string key = get_class_string(static_cast<int>(draft.cClass));
    if (draft.secondClass != CreatureClass::NONE)
        key += "/" + std::string(get_class_string(static_cast<int>(draft.secondClass)));
    return key;
}

PlayerClass *classForDraft(const CharacterDraft &draft) {
    auto it = gConfig->classes.find(draftClassKey(draft));
    return it != gConfig->classes.end() ? it->second : nullptr;
}

ClassPair resolveClassPair(int classCode) {
    ClassPair pair;
    switch (classCode) {
        case static_cast<int>(CreatureClass::ASSASSIN):
        case static_cast<int>(CreatureClass::BARD):
        case static_cast<int>(CreatureClass::BERSERKER):
        case static_cast<int>(CreatureClass::CLERIC):
        case static_cast<int>(CreatureClass::DEATHKNIGHT):
        case static_cast<int>(CreatureClass::DRUID):
        case static_cast<int>(CreatureClass::FIGHTER):
        case static_cast<int>(CreatureClass::LICH):
        case static_cast<int>(CreatureClass::MAGE):
        case static_cast<int>(CreatureClass::MONK):
        case static_cast<int>(CreatureClass::PALADIN):
        case static_cast<int>(CreatureClass::RANGER):
        case static_cast<int>(CreatureClass::ROGUE):
        case static_cast<int>(CreatureClass::THIEF):
        case static_cast<int>(CreatureClass::PUREBLOOD):
        case static_cast<int>(CreatureClass::WEREWOLF):
            pair.cClass = static_cast<CreatureClass>(classCode);
            break;
        case MULTI_BASE + 0:
            pair.cClass = CreatureClass::FIGHTER;
            pair.second = CreatureClass::MAGE;
            break;
        case MULTI_BASE + 1:
            pair.cClass = CreatureClass::FIGHTER;
            pair.second = CreatureClass::THIEF;
            break;
        case MULTI_BASE + 2:
            pair.cClass = CreatureClass::CLERIC;
            pair.second = CreatureClass::ASSASSIN;
            break;
        case MULTI_BASE + 3:
            pair.cClass = CreatureClass::MAGE;
            pair.second = CreatureClass::THIEF;
            break;
        case MULTI_BASE + 4:
            pair.cClass = CreatureClass::THIEF;
            pair.second = CreatureClass::MAGE;
            break;
        case MULTI_BASE + 5:
            pair.cClass = CreatureClass::CLERIC;
            pair.second = CreatureClass::FIGHTER;
            break;
        case MULTI_BASE + 6:
            pair.cClass = CreatureClass::MAGE;
            pair.second = CreatureClass::ASSASSIN;
            break;
        default:
            break;
    }
    return pair;
}

std::vector<short> playableRaceChoices() {
    std::vector<short> out;
    for (const auto &[id, race]: gConfig->races)
        if (race->isPlayable() && !race->getParentRace())
            out.push_back(race->getId());
    return out;
}

std::vector<short> subraceChoices(short selectedRaceId) {
    std::vector<short> out;
    for (const auto &[id, race]: gConfig->races)
        if (race->isPlayable() && (race->getId() == selectedRaceId || race->getParentRace() == selectedRaceId))
            out.push_back(race->getId());
    return out;
}

std::vector<int> allowedClassCodes(short raceId) {
    std::vector<int> out;
    const RaceData *race = gConfig->getRace(raceId);
    for (int l = 1; l < CLASS_COUNT_MULT; l++)
        if (race->allowedClass(l))
            out.push_back(l);
    return out;
}

std::vector<int> allowedDeityChoices(short raceId, CreatureClass cClass, CreatureClass second, int sex) {
    std::vector<int> out;
    const RaceData *race = gConfig->getRace(raceId);
    const bool male = sex == SEX_MALE;
    for (int l = 1; l < DEITY_COUNT; l++) {
        // Gender-restrict Mara/Arachnus/Linothan clerics.
        if (cClass == CreatureClass::CLERIC && ((male && l == MARA) || (male && l == ARACHNUS) || (!male && l == LINOTHAN)))
            continue;
        if (race->allowedDeity(cClass, second, l))
            out.push_back(l);
    }
    return out;
}

int sanitizeDeity(int deity) {
    switch (deity) {
        case ARAMON:
        case CERIS:
        case ENOCH:
        case GRADIUS:
        case ARES:
        case KAMIRA:
        case LINOTHAN:
        case MARA:
        case JAKAR:
        case ARACHNUS:
            return deity;
        default:
            return 0;
    }
}

SexResult validateSex(char input, bool isGendered) {
    if (!isGendered)
        return {true, SEX_NONE};
    const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(input)));
    if (c == 'f')
        return {true, SEX_FEMALE};
    if (c == 'm')
        return {true, SEX_MALE};
    return {false, 0};
}

StatsChoiceKind parseStatsChoice(const std::string &input) {
    std::string s = input;
    boost::trim(s);
    if (s.empty())
        return StatsChoiceKind::Invalid;
    const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(s[0])));
    if (c == 'c')
        return StatsChoiceKind::Custom;
    if (c == 'u')
        return StatsChoiceKind::Predefined;
    return StatsChoiceKind::Invalid;
}

StatsResult parseStats(const std::string &input) {
    StatsResult result;
    std::istringstream iss(input);
    std::array<int, kStatCount> num{};
    std::size_t count = 0;

    for (std::string token; iss >> token;) {
        if (count == num.size()) {
            result.error = StatsError::TooFew; // a sixth token is not a stat line either
            return result;
        }
        const char *const end = token.data() + token.size();
        const auto [ptr, ec] = std::from_chars(token.data(), end, num[count]);
        if (ec != std::errc{} || ptr != end) {
            result.error = StatsError::TooFew;
            return result;
        }
        count++;
    }
    if (count != num.size()) {
        result.error = StatsError::TooFew;
        return result;
    }

    int sum = 0;
    for (const int v: num) {
        if (v < kMinStat || v > kMaxStat) {
            result.error = StatsError::OutOfRange;
            return result;
        }
        sum += v;
    }
    result.sum = sum;
    if (sum != kStatTotal) {
        result.error = StatsError::BadSum;
        return result;
    }
    result.stats = num;
    return result;
}

StatModResult validateStatModifier(char input) {
    const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(input)));
    if (c < 'a' || c > 'e')
        return {false, 0};
    return {true, c - 'a' + STR};
}

std::array<int, kStatCount> computeFinalStats(const CharacterDraft &draft, const RaceData *race) {
    constexpr std::array<int, kStatCount> order{STR, DEX, CON, INT, PTY};
    std::array<int, kStatCount> finalStats{};
    for (std::size_t i = 0; i < order.size(); i++) {
        int v = draft.baseStats[i] * kStatScale;
        if (draft.bonusStat == order[i])
            v += kStatScale;
        if (draft.penaltyStat == order[i])
            v -= kStatScale;
        v += race->getStatAdj(order[i]);
        finalStats[i] = std::max<int>(MIN_STAT_NUM, v);
    }
    return finalStats;
}

CharNameResult validateNewCharacterName(const std::string &raw, const Account *account, bool checkExists) {
    CharNameResult result;
    std::string name = raw;
    boost::trim(name);

    if (name.empty()) {
        result.error = CharNameError::Empty;
        return result;
    }
    if (name.length() > kMaxCharNameLen) {
        result.error = CharNameError::TooLong;
        return result;
    }
    lowercize(name, 1);
    result.normalized = name; // properly-cased name; valid only when error == Ok

    const AccountAuth::NameError ne = AccountAuth::checkName(name);
    if (ne != AccountAuth::NameError::Ok) {
        result.error = CharNameError::BadName;
        result.nameError = ne;
        return result;
    }
    if (checkExists && Player::exists(name)) {
        result.error = CharNameError::Taken;
        return result;
    }
    if (account && account->hasCharacter(name)) {
        result.error = CharNameError::InAccount;
        return result;
    }
    // A reserved DM name (parse_name lets the exact name through) that is still free:
    // telnet gates it behind the DM password, web rejects it.
    if (isdm(name)) {
        result.error = CharNameError::Reserved;
        return result;
    }
    return result;
}

} // namespace Create
