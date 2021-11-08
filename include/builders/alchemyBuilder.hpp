/*
 * alchemyBuilder.h
 *   Fluent API style builders for alchemy
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

#include <string>
#include <fmt/format.h>
#include "alchemy.hpp"

#include "fluentBuilder.hpp"
#define BUILDER(name, type) GENERIC_BUILDER(AlchemyBuilder, alchemyInfo, name, type)


class AlchemyBuilder {
public:
    STRING_BUILDER(name);
    STRING_BUILDER(action);
    STRING_BUILDER(pythonScript);
    STRING_BUILDER(potionDisplayName);
    STRING_BUILDER(potionPrefix);

    BOOL_BUILDER(positive);
    BOOL_BUILDER(throwable);

    LONG_BUILDER(baseDuration);
    SHORT_BUILDER(baseStrength);


    // NOLINTNEXTLINE - We want implicit conversion
    operator AlchemyInfo&&() {
        // Validate
        if(alchemyInfo.name.empty()) {
            throw std::runtime_error("AlchemyInfo with empty name");
        }
        if(alchemyInfo.action == "python" && alchemyInfo.pythonScript.empty()) {
            throw std::runtime_error(fmt::format("Python action without python script for {}", alchemyInfo.name));
        }
        return std::move(alchemyInfo);
    }

private:
    AlchemyInfo alchemyInfo;
};

#include "fluentBuilderEnd.hpp"
