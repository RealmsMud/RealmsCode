/*
 * msdpLoader.h
 *   MSDP Loader
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

#include <map>


#include "config.hpp"
#include "msdp.hpp"
#include "builders/msdpBuilder.hpp"

void addToSet(MsdpVariable &&msdpVar, MsdpVariableMap &msdpVariables) {
    msdpVariables.emplace(msdpVar.getName(), std::move(msdpVar));
}

bool Config::initMsdp() {
    addToSet(MsdpBuilder().name("SERVER_ID").valueFn(msdp::getServerId)
        .reportable(false).requiresPlayer(false).configurable(false)
        .writeOnce(false).updateInterval(1).updateable(false).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("SERVER_TIME").valueFn(msdp::getServerTime)
        .reportable(false).requiresPlayer(false).configurable(false)
        .writeOnce(false).updateInterval(1).updateable(false).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("CHARACTER_NAME").valueFn(msdp::getCharacterName)
        .reportable(false).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(1).updateable(false).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("HEALTH").valueFn(msdp::getHealth)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(5).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("HEALTH_MAX").valueFn(msdp::getHealthMax)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(5).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("MANA").valueFn(msdp::getMana)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(5).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("MANA_MAX").valueFn(msdp::getManaMax)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(5).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("EXPERIENCE").valueFn(msdp::getExperience)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("EXPERIENCE_MAX").valueFn(msdp::getExperienceMax)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("EXPERIENCE_TNL").valueFn(msdp::getExperienceTNL)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("EXPERIENCE_TNL_MAX").valueFn(msdp::getExperienceTNLMax)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("WIMPY").valueFn(msdp::getWimpy)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("MONEY").valueFn(msdp::getMoney)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("BANK").valueFn(msdp::getBank)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("ARMOR").valueFn(msdp::getArmor)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("ARMOR_ABSORB").valueFn(msdp::getArmorAbsorb)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("GROUP").valueFn(msdp::getGroup)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(true)
      , msdpVariables);
    addToSet(MsdpBuilder().name("TARGET").valueFn(msdp::getTarget)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(5).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("TARGET_ID").valueFn(msdp::getTargetID)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(5).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("TARGET_HEALTH").valueFn(msdp::getTargetHealth)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(5).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("TARGET_HEALTH_MAX").valueFn(msdp::getTargetHealthMax)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("TARGET_STRENGTH").valueFn(msdp::getTargetStrength)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(10).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("ROOM").valueFn(msdp::getRoom)
        .reportable(true).requiresPlayer(true).configurable(false)
        .writeOnce(false).updateInterval(5).updateable(true).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("CLIENT_ID")
        .reportable(true).requiresPlayer(false).configurable(true)
        .writeOnce(true).updateInterval(1).valueFn(nullptr).updateable(false).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("CLIENT_VERSION")
        .reportable(true).requiresPlayer(false).configurable(true)
        .writeOnce(true).updateInterval(1).valueFn(nullptr).updateable(false).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("PLUGIN_ID")
        .reportable(true).requiresPlayer(false).configurable(true)
        .writeOnce(false).updateInterval(1).valueFn(nullptr).updateable(false).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("ANSI_COLORS")
        .reportable(true).requiresPlayer(false).configurable(true)
        .writeOnce(false).updateInterval(1).valueFn(nullptr).updateable(false).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("XTERM_256_COLORS")
        .reportable(true).requiresPlayer(false).configurable(true)
        .writeOnce(false).updateInterval(1).valueFn(nullptr).updateable(false).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("UTF_8")
        .reportable(true).requiresPlayer(false).configurable(true)
        .writeOnce(false).updateInterval(1).valueFn(nullptr).updateable(false).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("SOUND")
        .reportable(true).requiresPlayer(false).configurable(true)
        .writeOnce(false).updateInterval(1).valueFn(nullptr).updateable(false).isGroup(false)
      , msdpVariables);
    addToSet(MsdpBuilder().name("MXP")
        .reportable(true).requiresPlayer(false).configurable(true)
        .writeOnce(false).updateInterval(1).valueFn(nullptr).updateable(false).isGroup(false)
      , msdpVariables);

    return true;
}

void Config::clearMsdp() {
    msdpVariables.clear();
}