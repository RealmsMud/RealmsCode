/*
 * Catref-json.cpp
 *   Catref json
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

#include <string>
#include "json.hpp"
#include "catRef.hpp"

void to_json(nlohmann::json &j, const CatRef &cr) {
    j = json{{
        {"area", cr.area},
        {"id", cr.id},
    }};
}

void from_json(const nlohmann::json &j, CatRef &cr) {
    cr.area = j.at("area").get<std::string>();
    cr.id = j.at("id").get<short>();
}



void to_json(nlohmann::json &j, const QuestCatRef &cr) {
    // Call the super to_json first
    to_json(j, static_cast<const CatRef&>(cr));
    j.update({
        {"curNum", cr.curNum},
        {"reqNum", cr.reqNum},
    });
}

void from_json(const nlohmann::json &j, QuestCatRef &cr) {
    // Call the super from_json first
    from_json(j, static_cast<CatRef&>(cr));

    cr.curNum = j.at("curNum").get<int>();
    cr.reqNum = j.at("reqNum").get<int>();
}
