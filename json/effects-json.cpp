/*
 * Effects-json.cpp
 *   Effects json
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later

 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "json.hpp"
#include "effects.hpp"

void to_json(nlohmann::json &j, const Effects &e) {
    j = json {e.effectList};
}
void from_json(const nlohmann::json &j, Effects &e) {

}

void to_json(nlohmann::json &j, const EffectInfo *e) {
    to_json(j, *e);
}

void to_json(nlohmann::json &j, const EffectInfo &e) {
    j = json {
        {"name", e.name},
        {"duration", e.duration},
        {"strength", e.strength},
        {"extra", e.extra},
        {"pulseModifier", e.pulseModifier},
    };
}

void from_json(const nlohmann::json &j, EffectInfo &e) {

}