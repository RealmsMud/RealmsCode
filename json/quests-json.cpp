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
#include "json.hpp"
#include "quests.hpp"

using json = nlohmann::json;


void to_json(nlohmann::json &j, const QuestInfo &quest) {
    j = json{
        {"id", quest.questId},
        {"name", quest.name},
        {"description", quest.description},
        {"description", quest.description},
        { "receiveString", quest.receiveString },
        { "completionString", quest.completionString },
        { "revision", quest.revision },
        { "repeatFrequency", quest.repeatFrequency },
        { "timesRepeatable", quest.timesRepeatable },
        { "sharable", quest.sharable },
        { "minLevel", quest.minLevel },
        { "minFaction", quest.minFaction },
        { "level", quest.level },

    };

    if(!quest.preRequisites.empty()) j["preRequisites"] = quest.preRequisites;

    auto &req = j["requirements"] = json();
    if(!quest.initialItems.empty()) req["initialItems"] = quest.initialItems;
    if(!quest.mobsToKill.empty()) req["mobsToKill"] = quest.mobsToKill;
    if(!quest.itemsToGet.empty()) req["itemsToGet"] = quest.itemsToGet;
    if(!quest.roomsToVisit.empty()) req["roomsToVisit"] = quest.roomsToVisit;

    j["turnInMob"] = quest.turnInMob;

    auto &rewards = j["rewards"] = json();
    to_json("cashReward", rewards, quest.cashReward);
    rewards["expReward"] = quest.expReward;
    rewards["alignmentChange"] = quest.alignmentChange;
    if(!quest.itemRewards.empty()) rewards["itemRewards"] = quest.itemRewards;
    if(!quest.factionRewards.empty()) rewards["factionRewards"] = quest.factionRewards;

}

void from_json(const nlohmann::json &j, QuestInfo &quest) {
    j.at("id").get_to(quest.questId);
    quest.name = j.at("name").get<std::string>();
    quest.description = j.at("description").get<std::string>();
    quest.receiveString = j.at("receiveString").get<std::string>();
    quest.completionString = j.at("completionString").get<std::string>();
    quest.revision = j.at("revision").get<std::string>();

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

    if (quest.repeatFrequency != QuestRepeatFrequency::REPEAT_NEVER)
        quest.repeatable = true;
}
