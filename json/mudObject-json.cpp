/*
 *  MudObjects-json.cpp
 *   MudObjects json
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
#include "mudObjects/mudObject.hpp"

void to_json(nlohmann::json &j, const MudObject &mo) {
    to_json(j, mo, true);
}

void to_json(nlohmann::json &j, const MudObject &mo, bool saveId) {
    j = json{
        {"name", mo.name},
    };
    if (saveId) {
        j["id"] = mo.id;
    }
    if (!mo.hooks.empty()) {
        j["hooks"] = mo.hooks;
    }
    if (!mo.effects.effectList.empty()) {
        j["effects"] = mo.effects;
    }
}

void from_json(const nlohmann::json &j, MudObject &mo) {

}