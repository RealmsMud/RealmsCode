/*
 * skillBuilder.h
 *   Fluent API style builder for skills
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

#include <fmt/format.h>              // for format
#include <string>

#include "config.hpp"
#include "skills.hpp"

#include "fluentBuilder.hpp"


#define BUILDER(name, type) GENERIC_BUILDER(SkillInfoBuilder, skillInfo, name, type)

class SkillInfoBuilder {
public:
    STRING_BUILDER(name);
    STRING_BUILDER(displayName);
    STRING_BUILDER(baseSkill);
    STRING_BUILDER(group);
    BOOL_BUILDER(knownOnly);

    SkillInfoBuilder &gainType(const SkillGainType &gainType) {
        skillInfo.gainType = gainType;
        return *this;
    }

    // NOLINTNEXTLINE - We want implicit conversion
    operator SkillInfo&&() {
        if(skillInfo.name.empty() || skillInfo.displayName.empty()) {
            throw(std::runtime_error(fmt::format("Invalid skill (Name: {} DisplayName: {}", skillInfo.name, skillInfo.displayName)));
        }
        if(!skillInfo.baseSkill.empty() && gConfig->skills.find(skillInfo.baseSkill) == gConfig->skills.end()) {
            throw(std::runtime_error(fmt::format("Invalid Base Skill {}", skillInfo.baseSkill)));
        }
        if(!skillInfo.group.empty() && gConfig->skillGroups.find(skillInfo.group) == gConfig->skillGroups.end()) {
            throw(std::runtime_error(fmt::format("Invalid Skill Group {}", skillInfo.group)));
        }

        return std::move(skillInfo);
    };
private:
    SkillInfo skillInfo;
};

#include "fluentBuilderEnd.hpp"
