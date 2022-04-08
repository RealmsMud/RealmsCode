/*
 * Quests-json.cpp
 *   Quest json
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
#include <nlohmann/json.hpp>

#include "json.hpp"
#include "quests.hpp"

using json = nlohmann::json;


void to_json(nlohmann::json &j, const QuestInfo &quest) {
    j["name"] = quest.name;
    j["description"] = quest.description;
    j["receiveString"] = quest.receiveString;
    j["completionString"] = quest.completionString;
    j["revision"] = quest.revision;

    j["repeatable"] = quest.repeatable;
    j["repeatFrequency"] = quest.repeatFrequency;
    j["timesRepeatable"] = quest.timesRepeatable;

    j["sharable"] = quest.sharable;
    j["minLevel"] = quest.minLevel;
    j["minFaction"] = quest.minFaction;
    j["level"] = quest.level;

    toJsonList<CatRef>("preRequisites", j, quest.preRequisites);

    auto &req = j["requirements"] = json();
    toJsonList<QuestCatRef>("initialItems", req, quest.initialItems);
    toJsonList<QuestCatRef>("mobsToKill", req, quest.mobsToKill);
    toJsonList<QuestCatRef>("itemsToGet", req, quest.itemsToGet);
    toJsonList<QuestCatRef>("roomsToVisit", req, quest.roomsToVisit);

    j["turnInMob"] = quest.turnInMob;

    auto &rewards = j["rewards"] = json();
    to_json("cashReward", rewards, quest.cashReward);
    rewards["expReward"] = quest.expReward;
    rewards["alignmentChange"] = quest.alignmentChange;
    toJsonList<QuestCatRef>("itemRewards", rewards, quest.itemRewards);
    if(!quest.factionRewards.empty()) rewards["factionRewards"] = quest.factionRewards;

}

void from_json(const nlohmann::json &j, QuestInfo &quest) {
    quest.name = j.at("name").get<std::string>();
    quest.description = j.at("description").get<std::string>();
    quest.receiveString = j.at("receiveString").get<std::string>();
    quest.completionString = j.at("completionString").get<std::string>();
    quest.revision = j.at("revision").get<std::string>();

    quest.repeatable = j.at("repeatable").get<bool>();
    quest.repeatFrequency = j.at("repeatFrequency").get<QuestRepeatFrequency>();
    quest.timesRepeatable = j.at("timesRepeatable").get<int>();

    quest.sharable = j.at("sharable").get<bool>();
    quest.minLevel = j.at("minLevel").get<int>();
    quest.minFaction = j.at("minFaction").get<int>();
    quest.level = j.at("level").get<int>();

    j.at("preRequisites").get_to(quest.preRequisites);

    auto &req = j.at("requirements");
    if(req.contains("initialItems")) req.at("initialItems").get_to(quest.initialItems);
    if(req.contains("mobsToKill")) req.at("mobsToKill").get_to(quest.mobsToKill);
    if(req.contains("itemsToGet")) req.at("itemsToGet").get_to(quest.itemsToGet);
    if(req.contains("roomsToVisit")) req.at("roomsToVisit").get_to(quest.roomsToVisit);

    j.at("turnInMob").get_to(quest.turnInMob);

    auto &rewards = j.at("rewards");
    from_json("cashReward", rewards, quest.cashReward);
    quest.expReward = rewards.at("expReward").get<long>();
    quest.alignmentChange = rewards.at("alignmentChange").get<short>();
    if(req.contains("itemRewards")) rewards.at("itemRewards").get_to(quest.itemRewards);
    if(req.contains("factionRewards")) rewards.at("factionRewards").get_to(quest.factionRewards);
}
