/*
 * Objects-json.cpp
 *   Objects json
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
#include "mudObjects/mudObject.hpp"
#include "mudObjects/objects.hpp"
#include "flags.hpp"

void to_json(nlohmann::json &j, const Object &obj) {
    to_json(j, obj, false, LoadType::LS_FULL, 1, true, nullptr);
}

void to_json(nlohmann::json &j, const Object &obj, bool permOnly, LoadType saveType, int quantity, bool saveId, const std::list<std::string> *idList) {
    to_json(j, static_cast<const MudObject&>(obj), saveId && quantity == 1);
    j.update({
        {"info", obj.info},
        {"version", obj.version},

        // TODO: Filter out 0/empty?
        {"plural", obj.plural},
        {"adjustment", obj.adjustment},
        {"shots", json{{"cur", obj.shotsCur}, {"max", obj.shotsMax}}},
        {"charges", json{{"cur", obj.chargesCur}, {"max", obj.chargesMax}}},
        {"armor", obj.armor},
        {"numAttacks", obj.numAttacks},
        {"delay", obj.delay},
        {"extra", obj.extra},
        {"lotteryCycle", obj.lotteryCycle},
        {"value", obj.value},
        {"dice", obj.damage},
        {"keys", obj.key},
        {"lastTimes", obj.lasttime},
        {"effect", json{{"name",     obj.effect},
                        {"duration", obj.effectDuration},
                        {"strength", obj.effectStrength}}},
        {"recipe", obj.recipe},
        {"made", obj.made},
        {"owner", obj.questOwner},
        {"custom", obj.label},
        {"saveType", saveType},
    });

    if(saveType != LoadType::LS_FULL && saveType != LoadType::LS_PROTOTYPE) {
        // Save only a subset of flags for obj references
        j["flags"] = obj.flags & Object::objRefFlagsSet;
    } else if(saveType == LoadType::LS_FULL || saveType == LoadType::LS_PROTOTYPE) {
        j.update({
            {"flags", obj.flags},
            {"useOutput", obj.use_output},
            {"useAttack", obj.use_attack},
            {"lastMod", obj.lastMod},
            {"weight", obj.weight},
            {"type", obj.type},
            {"subType", obj.subType},
            {"wearFlag", obj.wearflag},
            {"magicPower", obj.magicpower},
            {"level", obj.level},
            {"quality", obj.quality},
            {"requiredSkill", obj.requiredSkill},
            {"clan", obj.clan},
            {"special", obj.special},
            {"questNum", obj.questnum},
            {"bulk", obj.bulk },
            {"size", obj.size },
            {"maxBulk", obj.maxbulk },
            {"coinCost", obj.coinCost },
            {"deed", obj.deed },
            {"keyVal", obj.keyVal},
            {"material", obj.material},
            {"minStrength", obj.minStrength},
            {"inBag", obj.in_bag}

        });

        if(obj.type != ObjectType::LOTTERYTICKET) {
            // Handled elsewhere
            j["description"] = obj.description;
        }

        if(!obj.alchemyEffects.empty()) {
            j["alchemyEffects"] = obj.alchemyEffects;
        }

        if(!obj.randomObjects.empty()) {
            j["randomObjects"] = obj.randomObjects;
        }
        if(obj.compass) {
            j["compass"] = *obj.compass;
        }
        if(obj.increase) {
            j["increase"] = *obj.increase;
        }
    }

    if(obj.type == ObjectType::LOTTERYTICKET) {
        j["lotteryTicket"] = json{
            {"desc", obj.description},
            {"lotteryNumbers", obj.lotteryNumbers},
        };
    }

    if (quantity > 1) {
        j["quantity"] = json({
            {"amount", quantity},
            {"ids", *idList},
        });
    }
    if(saveType != LoadType::LS_PROTOTYPE) {
        j.update({
            {"shopValue", obj.shopValue},
            {"droppedBy", obj.droppedBy},
        });
        if(!obj.objects.empty()) {
            if (obj.type == ObjectType::CONTAINER && obj.flagIsSet(O_PERM_ITEM)) {
                permOnly = true;
            }
            json objects = json::array();
            for (auto it = obj.objects.begin(); it != obj.objects.end();) {
                auto subObj = (*it++);
                if (permOnly && !subObj->flagIsSet(O_PERM_ITEM)) continue;

                json subObject;
                LoadType subSaveType = (subObj->flagIsSet(O_CUSTOM_OBJ) || subObj->flagIsSet(O_SAVE_FULL) || !subObj->info.id) ? LoadType::LS_FULL : LoadType::LS_PROTOTYPE;
                int subQty = 1;
                std::list<std::string> subIdList;
                subIdList.push_back(subObj->getId());
                while(it != obj.objects.end() && *(*it) == *subObj) {
                    subIdList.push_back((*it)->getId());
                    subQty++;
                    it++;
                }

                to_json(subObject, *subObj, permOnly, subSaveType, subQty, true, &subIdList);
                objects.emplace_back(subObject);
            }
            j["objects"] = objects;

        }
    }

}

void from_json(const nlohmann::json &j, Object &obj) {

}