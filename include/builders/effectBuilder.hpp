/*
 * effectsBuilder.h
 *   Fluent API style builder for effects
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
#include "effects.hpp"

#include "fluentBuilder.hpp"
#define BUILDER(name, type) GENERIC_BUILDER(EffectBuilder, effect, name, type)


class EffectBuilder {
public:
    STRING_BUILDER(name);
    STRING_BUILDER(display);
    STRING_BUILDER(oppositeEffect);

    EffectBuilder& addBaseEffect(const std::string& baseEffect) {
        effect.baseEffects.emplace_back(baseEffect);
        return *this;
    }

    STRING_BUILDER(selfAddStr);
    STRING_BUILDER(selfDelStr);
    STRING_BUILDER(roomAddStr);
    STRING_BUILDER(roomDelStr);

    STRING_BUILDER(type);

    STRING_BUILDER(computeScript);
    STRING_BUILDER(applyScript);
    STRING_BUILDER(unApplyScript);
    STRING_BUILDER(preApplyScript);
    STRING_BUILDER(postApplyScript);
    STRING_BUILDER(pulseScript);

    BOOL_BUILDER(pulsed);
    BOOL_BUILDER(isSpellEffect);
    BOOL_BUILDER(useStrength);

    INT_BUILDER(pulseDelay);
    INT_BUILDER(baseDuration);
    INT_BUILDER(magicRoomBonus);

    FLOAT_BUILDER(potionMultiplyer);

    // NOLINTNEXTLINE - We want implicit conversion
    operator Effect&&() {
        if(effect.name.empty()) {
            throw std::runtime_error("Effect with empty name");
        }
        return std::move(effect);
    }

private:
    Effect effect;
};

#include "fluentBuilderEnd.hpp"
