/*
 * Realms-json.cpp
 *   General json
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
#include <boost/dynamic_bitset.hpp>
#include "json.hpp"


namespace boost {
    void to_json(nlohmann::json &j, const boost::dynamic_bitset<> &b) {
        std::string str;
        to_string(b, str);
        j = str;
    }

    void from_json(const nlohmann::json &j, boost::dynamic_bitset<> &b) {
        b = boost::dynamic_bitset<>(j.get<std::string>());
    }
}
