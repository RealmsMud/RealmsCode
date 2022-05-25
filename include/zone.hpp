/*
 * Zone.h
 *   Header file for zones code.
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

#pragma once

#include <string>                       // for hash, string
#include <memory>
#include <boost/dynamic_bitset.hpp>
#include <nlohmann/json_fwd.hpp>

class QuestInfo;

class CatRef;

typedef std::map<CatRef, std::shared_ptr<QuestInfo>> ZoneQuestInfoMap;

class Zone {
public:
    Zone();
    ~Zone();

public:
    friend void to_json(nlohmann::json &j, const Zone &zone);
    friend void from_json(const nlohmann::json &j, Zone &zone);
    friend std::ostream& operator<<(std::ostream&, const Zone&);

private:
    std::string name;
    std::string display;

    boost::dynamic_bitset<> flags{64};

    // Zone specific quests.  The zone owns these quests, but there will also be a reference to it
    // in the global config class, ensure that is cleaned up as well
    ZoneQuestInfoMap quests;

};
