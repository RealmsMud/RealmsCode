/*
 * mxpBuilder.h
 *   Fluent API style builders for mxp
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
#include <utility>
#include <fmt/format.h>
#include "mxp.hpp"

#include "fluentBuilder.hpp"

#define BUILDER(name, type) GENERIC_BUILDER(MxpBuilder, mxpElement, name, type)


class MxpBuilder {
public:
    STRING_BUILDER(name);
    STRING_BUILDER(mxpType);
    STRING_BUILDER(command);
    STRING_BUILDER(hint);
    STRING_BUILDER(attributes);
    STRING_BUILDER(expire);
    STRING_BUILDER(color);

    BOOL_BUILDER(prompt);

    // NOLINTNEXTLINE - We want implicit conversion
    operator MxpElement&&() {
        if(mxpElement.name.empty()){
            throw std::runtime_error("mxpElement with empty name");
        }
        return std::move(mxpElement);
    }

private:
    MxpElement mxpElement;
};

#include "fluentBuilderEnd.hpp"

