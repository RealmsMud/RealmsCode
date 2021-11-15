/*
 * msdpBuilder.h
 *   Fluent API style builders for MSDP
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
#include "msdp.hpp"

#include "fluentBuilder.hpp"

#define BUILDER(name, type) GENERIC_BUILDER(MsdpBuilder, msdpVar, name, type)


class MsdpBuilder {
public:
    STRING_BUILDER(name);

    BOOL_BUILDER(reportable);
    BOOL_BUILDER(requiresPlayer);
    BOOL_BUILDER(configurable);
    BOOL_BUILDER(writeOnce);

    INT_BUILDER(updateInterval);

    MsdpBuilder& valueFn(std::function<std::string(Socket&, Player*)> valueFn) {
        msdpVar.valueFn = std::move(valueFn);
        return *this;
    }

    BOOL_BUILDER(updateable);
    BOOL_BUILDER(isGroup);

    // NOLINTNEXTLINE - We want implicit conversion
    operator MsdpVariable&&() {
        if(msdpVar.name.empty()){
            throw std::runtime_error("MsdpVariable with empty name");
        }
        return std::move(msdpVar);
    }

private:
    MsdpVariable msdpVar;
};

#include "fluentBuilderEnd.hpp"
