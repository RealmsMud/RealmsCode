/*
 * Money-json.cpp
 *   Money json
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
#include "money.hpp"

void to_json(nlohmann::json &j, const Money &money) {
    to_json("coins", j, money);
}

void to_json(const char* name, nlohmann::json &j, const Money &money) {
    j[name] = money.m;
}

void from_json(const nlohmann::json &j, Money &money) {
    from_json("coins", j, money);
}

void from_json(const char* name, const nlohmann::json &j, Money &money) {
    j.at(name).get_to(money.m);
}

