/*
 * accounts-json.cpp
 *   Account JSON serialization
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

#include "json.hpp"
#include "account.hpp"

void to_json(nlohmann::json &j, const Account &account) {
    j = nlohmann::json{
        {"accountName", account.getName()},
        {"password", account.getPassword()},
        {"email", account.getEmail()},
        {"created", account.getCreated()},
        {"lastLogin", account.getLastLogin()},
        {"version", account.getVersion()},
        {"characterNames", account.getCharacterNames()},
        {"banned", account.isBanned()},
        {"banReason", account.getBanReason()},
        {"experience", account.getExperience()}
    };

    nlohmann::json upgrades = nlohmann::json::object();
    for(const auto& def : getAccountUpgradeDefinitions()) {
        auto level = account.getUpgradeLevel(def.id);
        if(level > 0) {
            upgrades[std::string(def.token)] = level;
        }
    }
    if(!upgrades.empty()) {
        j["upgrades"] = upgrades;
    }
}

void from_json(const nlohmann::json &j, Account &account) {
    // Required fields
    account.setName(j.at("accountName").get<std::string>());
    
    // Password is already hashed when loaded from JSON, so set directly
    // Note: We need a way to set the raw hashed password without re-hashing
    account.setPassword(j.at("password").get<std::string>());
    
    // Optional fields with defaults
    if (j.contains("email")) {
        account.setEmail(j.at("email").get<std::string>());
    }
    
    if (j.contains("created")) {
        account.setCreated(j.at("created").get<time_t>());
    }
    
    if (j.contains("lastLogin")) {
        account.setLastLogin(j.at("lastLogin").get<time_t>());
    }

    if (j.contains("version")) {
        account.setVersion(j.at("version").get<std::string>());
    }
    
    if (j.contains("characterNames")) {        
        // Add each character from JSON
        auto jsonCharNames = j.at("characterNames").get<std::vector<std::string>>();
        for (const auto& charName : jsonCharNames) {
            account.addCharacter(charName);
        }
    }
    
    if (j.contains("banned")) {
        account.setBanned(j.at("banned").get<bool>());
    }
    
    if (j.contains("banReason")) {
        account.setBanReason(j.at("banReason").get<std::string>());
    }
    
    if (j.contains("experience")) {
        account.setExperience(j.at("experience").get<unsigned long>());
    }

    account.clearUpgradeLevels();
    if(j.contains("upgrades")) {
        const auto& upgrades = j.at("upgrades");
        if(upgrades.is_object()) {
            for(auto it = upgrades.begin(); it != upgrades.end(); ++it) {
                const auto* def = findAccountUpgradeByToken(it.key());
                if(!def) {
                    continue;
                }
                account.setUpgradeLevel(def->id, it.value().get<unsigned short>());
            }
        }
    }
} 