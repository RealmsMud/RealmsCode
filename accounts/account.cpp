/*
 * account.cpp
 *   Account system implementation
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

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "account.hpp"
#include "json.hpp"
#include "paths.hpp"
#include "mudObjects/players.hpp"
#include "socket.hpp"
#include "server.hpp"
#include "xml.hpp"
#include "config.hpp"

namespace fs = std::filesystem;

//*********************************************************************
//                      Constructors & Destructor
//*********************************************************************

Account::Account() {
    reset();
}

Account::Account(const std::string& name) {
    reset();
    accountName = name;
    created = time(nullptr);
    lastLogin = created;
}

Account::Account(const Account& other) {
    copyFrom(other);
}

Account& Account::operator=(const Account& other) {
    if (this != &other) {
        copyFrom(other);
    }
    return *this;
}

Account::~Account() = default;

//*********************************************************************
//                      Helper Functions
//*********************************************************************

void Account::reset() {
    accountName.clear();
    password.clear();
    email.clear();
    created = 0;
    lastLogin = 0;
    characterLimit = 60;  // Default character limit
    characterNames.clear();
    banned = false;
    banReason.clear();
    experience = 0;
    version.clear();
    clearUpgradeLevels();
}

void Account::copyFrom(const Account& other) {
    accountName = other.accountName;
    password = other.password;
    email = other.email;
    created = other.created;
    lastLogin = other.lastLogin;
    characterLimit = other.characterLimit;
    characterNames = other.characterNames;
    banned = other.banned;
    banReason = other.banReason;
    experience = other.experience;
    version = other.version;
    upgradeLevels = other.upgradeLevels;
}

//*********************************************************************
//                      File Operations
//*********************************************************************

bool Account::save() const {
    if (accountName.empty()) {
        std::clog << "Account::save() - Cannot save account with empty name\n";
        return false;
    }

    const auto filename = (Path::Account / accountName).replace_extension("json");
    
    // Ensure directory exists
    fs::create_directories(Path::Account);

    try {
        nlohmann::json j = *this;
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::clog << "Account::save() - Cannot open file: " << filename << "\n";
            return false;
        }
        file << j.dump(4);  // Pretty print with 4 space indentation
        return true;
    } catch (const std::exception& e) {
        std::clog << "Account::save() - Error saving account " << accountName << ": " << e.what() << "\n";
        return false;
    }
}

bool Account::load(const std::string& accountName, std::shared_ptr<Account>& account) {
    if (accountName.empty()) {
        return false;
    }

    const auto filename = (Path::Account / accountName).replace_extension("json");
    
    if (!fs::exists(filename)) {
        return false;
    }

    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::clog << "Account::load() - Cannot open file: " << filename << "\n";
            return false;
        }

        nlohmann::json j;
        file >> j;

        account = std::make_shared<Account>();
        j.get_to(*account);

        return true;
    } catch (const std::exception& e) {
        std::clog << "Account::load() - Error loading account " << accountName << ": " << e.what() << "\n";
        return false;
    }
}

bool Account::exists(const std::string& accountName) {
    if (accountName.empty()) {
        return false;
    }
    const auto filename = (Path::Account / accountName).replace_extension("json");
    return fs::exists(filename);
}

//*********************************************************************
//                      Password Functions
//*********************************************************************

std::string Account::hashPassword(const std::string& password) {
    // Use the same password hashing as Player class
    return Player::hashPassword(password);
}

bool Account::isPassword(const std::string& pass) const {
    return password == hashPassword(pass);
}

void Account::updateLastLogin() {
    lastLogin = time(nullptr);
    // Record the last game version this account logged in with
    if(gConfig) {
        setVersion(gConfig->getVersion());
    }
}

//*********************************************************************
//                      Getters
//*********************************************************************

const std::string& Account::getName() const { return accountName; }
const std::string& Account::getPassword() const { return password; }
const std::string& Account::getEmail() const { return email; }
time_t Account::getCreated() const { return created; }
time_t Account::getLastLogin() const { return lastLogin; }
int Account::getCharacterLimit() const { return characterLimit; }
const std::vector<std::string>& Account::getCharacterNames() const { return characterNames; }
bool Account::isBanned() const { return banned; }
const std::string& Account::getBanReason() const { return banReason; }
unsigned long Account::getExperience() const { return experience; }
const std::string& Account::getVersion() const { return version; }

//*********************************************************************
//                      Setters
//*********************************************************************

void Account::setName(const std::string& name) {
    if(name == accountName)
        return;

    std::string oldName = accountName;
    accountName = name;

    // Update server caches and connected players' account mapping
    if(gServer) {
        // Move accountConnections entry
        auto itConn = gServer->accountConnections.find(oldName);
        if(itConn != gServer->accountConnections.end()) {
            gServer->accountConnections[name] = itConn->second;
            gServer->accountConnections.erase(itConn);
        }

        // Fix accountCache key if present
        for(auto it = gServer->accountCache.begin(); it != gServer->accountCache.end(); ++it) {
            if(it->second.get() == this) {
                if(it->first != name) {
                    auto accPtr = it->second;
                    gServer->accountCache.erase(it);
                    gServer->accountCache.emplace(name, accPtr);
                }
                break;
            }
        }
    }

    // Update all characters linked to this account (online and offline)
    for(const auto& charName : characterNames) {
        std::shared_ptr<Player> player = nullptr;
        if(gServer) player = gServer->findPlayer(charName);
        if(player) {
            player->setAccountName(accountName);
            player->save(true);
        } else {
            // Load from disk, update, and save
            std::shared_ptr<Player> diskPlayer;
            if(loadPlayer(charName, diskPlayer)) {
                diskPlayer->setAccountName(accountName);
                diskPlayer->save(true);
            }
        }
    }
}

void Account::setPassword(const std::string& pass) { password = hashPassword(pass); }
void Account::setEmail(const std::string& mail) { email = mail; }
void Account::setCreated(time_t time) { created = time; }
void Account::setLastLogin(time_t time) { lastLogin = time; }
void Account::setBanned(bool ban) { banned = ban; }
void Account::setBanReason(const std::string& reason) { banReason = reason; }
void Account::setExperience(unsigned long exp) { experience = exp; }
void Account::addExperience(unsigned long exp) { experience += exp; }
void Account::setVersion(const std::string& v) { version = v; }

bool Account::spendExperience(unsigned long exp) {
    if(exp > experience) {
        return false;
    }
    experience -= exp;
    return true;
}

unsigned short Account::getUpgradeLevel(AccountUpgradeId id) const {
    auto it = upgradeLevels.find(id);
    if(it == upgradeLevels.end()) {
        return 0;
    }
    return it->second;
}

void Account::setUpgradeLevel(AccountUpgradeId id, unsigned short level) {
    const auto& def = getAccountUpgrade(id);
    unsigned short clamped = static_cast<unsigned short>(std::min<unsigned int>(level, def.maxRank));
    if(clamped == 0) {
        upgradeLevels.erase(id);
    } else {
        upgradeLevels[id] = clamped;
    }
}

const std::unordered_map<AccountUpgradeId, unsigned short>& Account::getUpgradeLevels() const {
    return upgradeLevels;
}

void Account::clearUpgradeLevels() {
    upgradeLevels.clear();
}

unsigned int Account::getUpgradeValue(AccountUpgradeId id) const {
    const auto& def = getAccountUpgrade(id);
    return static_cast<unsigned int>(getUpgradeLevel(id)) * def.magnitudePerRank;
}

unsigned int Account::getExperienceBonusPercent() const {
    return getUpgradeValue(AccountUpgradeId::ExperienceGain);
}

//*********************************************************************
//                      Character Management
//*********************************************************************

bool Account::addCharacter(const std::string& characterName) {
    if (characterName.empty()) {
        return false;
    }

    // Check if we already have this character
    if (hasCharacter(characterName)) {
        return true;  // Already have it, consider success
    }

    // Check character limit
    if (!canCreateCharacter()) {
        return false;
    }

    characterNames.push_back(characterName);
    // Keep list sorted alphabetically (case-insensitive)
    std::sort(characterNames.begin(), characterNames.end(), [](const std::string& a, const std::string& b) {
        return std::lexicographical_compare(
            a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char ac, unsigned char bc) {
                return std::tolower(ac) < std::tolower(bc);
            }
        );
    });
    return true;
}

bool Account::removeCharacter(const std::string& characterName) {
    auto it = std::find(characterNames.begin(), characterNames.end(), characterName);
    if (it != characterNames.end()) {
        characterNames.erase(it);
        return true;
    }
    return false;
}

bool Account::hasCharacter(const std::string& characterName) const {
    return std::find(characterNames.begin(), characterNames.end(), characterName) != characterNames.end();
}

bool Account::canCreateCharacter() const {
    return static_cast<int>(characterNames.size()) < characterLimit;
}

int Account::getCharacterCount() const {
    return static_cast<int>(characterNames.size());
}

//*********************************************************************
//                      Validation Functions
//*********************************************************************


bool Account::isValidPassword(const std::string& password) {
    // Same validation as used elsewhere in the codebase
    return password.length() >= 5 && password.length() <= 35;
} 

//*********************************************************************
//                      UI Helpers
//*********************************************************************

void Account::printInfoFields(const std::shared_ptr<Player>& player) const {
    if(!player) return;
    player->print("^W%-12s^C%s^x\n", "Account:", getName().c_str());
    if(!getEmail().empty()) {
        player->print("^W%-12s^x%s\n", "Email:", getEmail().c_str());
    }
    player->print("^W%-12s^x%d/%d\n", "Characters:", getCharacterCount(), getCharacterLimit());
    player->print("^W%-12s^G%lu^x\n\n", "Experience:", getExperience());
}

void Account::printInfoFields(const std::shared_ptr<Socket>& sock) const {
    if(!sock) return;
    sock->print("^W%-12s^C%s^x\n", "Account:", getName().c_str());
    if(!getEmail().empty()) {
        sock->print("^W%-12s^x%s\n", "Email:", getEmail().c_str());
    }
    sock->print("^W%-12s^x%d/%d\n", "Characters:", getCharacterCount(), getCharacterLimit());
    sock->print("^W%-12s^G%lu^x\n\n", "Experience:", getExperience());
}

void Account::printCharacterList(const std::shared_ptr<Player>& player) const {
    if(!player) return;
    const auto& chars = getCharacterNames();
    if(chars.empty()) {
        player->print("No characters.\n");
        return;
    }
    player->print("^WYour Characters:^x\n");
    for(const auto& name : chars) {
        player->print("  ^C%s^x\n", name.c_str());
    }
}

void Account::printCharacterList(const std::shared_ptr<Socket>& sock) const {
    if(!sock) return;
    const auto& chars = getCharacterNames();
    if(chars.empty()) {
        sock->print("^KYou have no characters.^x\n");
        return;
    }
    sock->print("^WYour Characters:^x\n");
    for(const auto& name : chars) {
        sock->print("  ^C%s^x\n", name.c_str());
    }
}

void Account::printUpgradeSummary(const std::shared_ptr<Player>& player) const {
    if(!player) return;

    player->print("\n^W~~~~~~~ Account Upgrades ~~~~~~~^x\n\n");
    player->print("^WAccount Experience:^x ^G%lu^x\n\n", getExperience());

    const auto& defs = getAccountUpgradeDefinitions();
    for(const auto& def : defs) {
        std::string name(def.displayName);
        std::string token(def.token);
        unsigned short rank = getUpgradeLevel(def.id);
        bool maxed = rank >= def.maxRank;
        auto totalBonus = describeAccountUpgradeBonus(def, getUpgradeValue(def.id));
        auto perRankBonus = describeAccountUpgradeBonus(def, def.magnitudePerRank);

        player->print("  ^W%-16s^x (^C%s^x) Rank ^G%u/%u^x  %s\n",
                      name.c_str(), token.c_str(), rank, def.maxRank, totalBonus.c_str());
        if(maxed) {
            player->print("      ^BMAXED^x\n");
        } else {
            player->print("      Next rank: %s (Cost ^G%u^x account exp)\n",
                          perRankBonus.c_str(), def.costPerRank);
        }
    }

}

//*********************************************************************
//                      Player Account Functions
//*********************************************************************

bool Player::hasAccount() const { 
    return(!accountName.empty()); 
}

std::shared_ptr<Account> Player::getAccount() const {
    auto sock = getSock();
    if (sock) {
        return sock->getAccount();
    }
    return nullptr;
}

void Player::setAccountName(const std::string& name) {
    accountName = name;
}

std::string Player::getAccountName() const { return(accountName); }

void Player::applyAccountUpgradeBonuses() {
    std::shared_ptr<Account> accountPtr = nullptr;
    if(hasAccount()) {
        accountPtr = gServer->getOrLoadAccount(getAccountName());
    }

    bool modified = false;
    const auto& definitions = getAccountUpgradeDefinitions();
    for(const auto& def : definitions) {
        if(!isStatUpgrade(def.id)) {
            continue;
        }

        auto statName = getStatName(def.id);
        if(statName.empty()) {
            continue;
        }

        std::string modifierName = std::string("AccountUpgrade_") + std::string(def.token);
        int bonus = 0;
        if(accountPtr) {
            bonus = static_cast<int>(accountPtr->getUpgradeValue(def.id));
        }

        Stat* stat = getStat(statName);
        if(!stat) {
            continue;
        }

        stat->setModifier(modifierName, bonus, MOD_CUR_MAX);
        modified = true;
    }

    if(modified) {
        computeAttackPower();
        computeAC();
    }
}
 