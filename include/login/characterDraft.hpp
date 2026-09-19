/*
 * characterDraft.hpp
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
#include <string>
#include <vector>

#include "global.hpp"   // CreatureClass

struct CharacterDraft {
    std::string name;

    short race{0};
    int sex{0};                                   // SEX_NONE / SEX_MALE / SEX_FEMALE
    CreatureClass cClass{CreatureClass::NONE};
    CreatureClass secondClass{CreatureClass::NONE};
    int deity{0};

    std::string startLocation;                    // StartLoc key (gConfig->getStartLoc)

    bool usePredefinedStats{false};
    std::array<int, 5> baseStats{{0, 0, 0, 0, 0}};
    int bonusStat{0};
    int penaltyStat{0};

    std::vector<std::string> weaponSkills;

    bool operator==(const CharacterDraft&) const = default;
};
