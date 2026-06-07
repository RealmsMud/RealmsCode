/*
 * Monsters-json.cpp
 *   Creature + Monster JSON for the REST API read path
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

#include <string>

#include "json.hpp"
#include "carry.hpp"
#include "catRef.hpp"
#include "dice.hpp"
#include "global.hpp"
#include "money.hpp"
#include "skills.hpp"
#include "specials.hpp"
#include "stats.hpp"
#include "structs.hpp"
#include "quests.hpp"               // TalkResponse
#include "mudObjects/mudObject.hpp"
#include "mudObjects/creatures.hpp"
#include "mudObjects/monsters.hpp"
#include "mudObjects/objects.hpp"   // inventory/equipment serialization (full mode)

using json = nlohmann::json;

void to_json(nlohmann::json &j, const SpecialAttack &sa) {
    j = json{
        {"name", sa.name},
        {"verb", sa.verb},
        {"selfStr", sa.selfStr},
        {"selfFailStr", sa.selfFailStr},
        {"targetStr", sa.targetStr},
        {"roomStr", sa.roomStr},
        {"targetFailStr", sa.targetFailStr},
        {"roomFailStr", sa.roomFailStr},
        {"targetSaveStr", sa.targetSaveStr},
        {"selfSaveStr", sa.selfSaveStr},
        {"roomSaveStr", sa.roomSaveStr},
        {"saveType", static_cast<int>(sa.saveType)},
        {"saveBonus", static_cast<int>(sa.saveBonus)},
        {"maxBonus", sa.maxBonus},
        {"chance", sa.chance},
        {"delay", sa.delay},
        {"stunLength", sa.stunLength},
        {"type", static_cast<int>(sa.type)},
        {"flags", sa.flags},
        {"damage", sa.damage},
        {"limit", sa.limit},
        {"ltime", sa.ltime},
    };
}

void to_json(nlohmann::json &j, const Carry &c) {
    j = json{{"info", c.info}, {"numTrade", c.numTrade}};
}

static void skillToJson(json &j, const Skill &s) {
    j = json{{"name", s.getName()}, {"gained", s.getGained()}, {"gainBonus", s.getGainBonus()}};
}

void to_json(nlohmann::json &j, const Stat &stat) {
    j = json{{"cur", stat.cur}, {"max", stat.max}, {"initial", stat.initial}};
}

void to_json(nlohmann::json &j, const saves &s) {
    j = json{{"chance", s.chance}, {"gained", s.gained}, {"misc", s.misc}};
}

void to_json(nlohmann::json &j, const daily &d) {
    j = json{{"max", d.max}, {"cur", d.cur}, {"ltime", d.ltime}};
}

static void talkResponseToJson(json &j, const TalkResponse &t) {
    j = json{
        {"keywords", t.keywords},
        {"response", t.response},
        {"action", t.action},
    };
}

void to_json(nlohmann::json &j, const Creature &cr) {
    to_json(j, cr, LoadType::LS_FULL);
}

void to_json(nlohmann::json &j, const Creature &cr, LoadType mode) {
    const bool full = (mode == LoadType::LS_FULL);
    to_json(j, static_cast<const MudObject&>(cr), full);    // name, id (full only), hooks, effects

    j.update({
        {"race", cr.race},
        {"class", static_cast<int>(cr.cClass)},
        {"level", cr.level},
        {"type", static_cast<int>(cr.type)},
        {"experience", cr.experience},
        {"coins", cr.coins},
        {"alignment", cr.alignment},
        {"armor", cr.armor},
        {"deity", cr.deity},
        {"clan", cr.clan},
        {"poisonDuration", cr.poison_dur},
        {"poisonDamage", cr.poison_dmg},
        {"size", static_cast<int>(cr.size)},
        {"description", cr.description},
        {"version", cr.version},
        {"damage", cr.damage},
        {"flags", cr.flags},
        {"spells", cr.spells},
        {"languages", cr.languages},
        {"currentLanguage", cr.current_language},
        {"factions", cr.factions},
        {"stats", json{
            {"strength", cr.strength}, {"dexterity", cr.dexterity},
            {"constitution", cr.constitution}, {"intelligence", cr.intelligence},
            {"piety", cr.piety}, {"hp", cr.hp}, {"mp", cr.mp},
        }},
    });

    json keys = json::array();
    for(int i = 0; i < 3; i++) keys.push_back(std::string(cr.key[i]));
    j["keys"] = keys;
    json moves = json::array();
    for(int i = 0; i < 3; i++) moves.push_back(std::string(cr.movetype[i]));
    j["moveTypes"] = moves;
    json realms = json::array();
    for(int i = 0; i < MAX_REALM - 1; i++) realms.push_back(cr.realm[i]);
    j["realms"] = realms;
    json prof = json::array();
    for(int i = 0; i < 6; i++) prof.push_back(cr.proficiency[i]);
    j["proficiencies"] = prof;
    json saving = json::array();
    for(int i = 0; i < 6; i++) saving.push_back(cr.saves[i]);
    j["savingThrows"] = saving;

    json specials = json::array();
    for(const SpecialAttack& sa : cr.specials) {
        json s;
        to_json(s, sa);
        specials.push_back(s);
    }
    j["specialAttacks"] = specials;

    if(full) {
        if(!cr.skills.empty()) {
            json sk = json::array();
            for(const auto& [sname, skill] : cr.skills)
                if(skill) { json s; skillToJson(s, *skill); sk.push_back(s); }
            j["skills"] = sk;
        }
        json dailies = json::array();
        for(int i = 0; i < 20; i++) dailies.push_back(cr.daily[i]);
        j["daily"] = dailies;
        json lts = json::array();
        for(int i = 0; i < 128; i++) lts.push_back(cr.lasttime[i]);
        j["lasttime"] = lts;
        if(!cr.minions.empty()) j["minions"] = cr.minions;
        json inv = json::array();
        for(const auto& o : cr.objects)
            if(o) { json oj; to_json(oj, *o); inv.push_back(oj); }
        j["inventory"] = inv;
        json eq = json::array();
        for(int i = 0; i < MAXWEAR; i++)
            if(cr.ready[i]) { json oj; to_json(oj, *cr.ready[i]); eq.push_back(json{{"wearLoc", i}, {"object", oj}}); }
        j["equipment"] = eq;
    }
}

void to_json(nlohmann::json &j, const Monster &mon) {
    to_json(j, mon, LoadType::LS_FULL);
}

void to_json(nlohmann::json &j, const Monster &mon, LoadType mode) {
    to_json(j, static_cast<const Creature&>(mon), mode);    // all Creature fields

    j.update({
        {"info", mon.info},
        {"plural", mon.plural},
        {"skillLevel", mon.skillLevel},
        {"updateAggro", mon.updateAggro},
        {"loadAggro", mon.loadAggro},
        {"lastMod", std::string(mon.last_mod)},
        {"talk", mon.talk},
        {"tradeTalk", std::string(mon.ttalk)},
        {"numWander", mon.numwander},
        {"magicResistance", mon.magicResistance},
        {"defenseSkill", mon.defenseSkill},
        {"attackPower", mon.attackPower},
        {"weaponSkill", mon.weaponSkill},
        {"maxLevel", mon.maxLevel},
        {"cast", mon.cast},
        {"mobTrade", mon.mobTrade},
        {"primeFaction", mon.primeFaction},
        {"aggroString", std::string(mon.aggroString)},
        {"classAggro", mon.cClassAggro},
        {"raceAggro", mon.raceAggro},
        {"deityAggro", mon.deityAggro},
    });

    json attacks = json::array();
    for(int i = 0; i < 3; i++) attacks.push_back(std::string(mon.attack[i]));
    j["attacks"] = attacks;

    json talks = json::array();
    for(const TalkResponse* tr : mon.responses)
        if(tr) {
            json t;
            talkResponseToJson(t, *tr);
            talks.push_back(t);
        }
    j["talkResponses"] = talks;

    j["jail"] = mon.jail;

    json assist = json::array();
    for(int i = 0; i < NUM_ASSIST_MOB; i++)
        if(mon.assist_mob[i].id) assist.push_back(mon.assist_mob[i]);
    j["assistMobs"] = assist;

    json enemies = json::array();
    for(int i = 0; i < NUM_ENEMY_MOB; i++)
        if(mon.enemy_mob[i].id) enemies.push_back(mon.enemy_mob[i]);
    j["enemyMobs"] = enemies;

    json rescue = json::array();
    for(int i = 0; i < NUM_RESCUE; i++)
        if(mon.rescue[i].id) rescue.push_back(mon.rescue[i]);
    j["rescue"] = rescue;

    json carried = json::array();
    for(int i = 0; i < 10; i++)
        if(mon.carry[i].info.id) carried.push_back(mon.carry[i]);
    j["carry"] = carried;
}
