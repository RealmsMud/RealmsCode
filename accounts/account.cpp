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

//*********************************************************************
//                      Setters
//*********************************************************************

void Account::setName(const std::string& name) { accountName = name; }
void Account::setPassword(const std::string& pass) { password = hashPassword(pass); }
void Account::setEmail(const std::string& mail) { email = mail; }
void Account::setCreated(time_t time) { created = time; }
void Account::setLastLogin(time_t time) { lastLogin = time; }
void Account::setBanned(bool ban) { banned = ban; }
void Account::setBanReason(const std::string& reason) { banReason = reason; }
void Account::setExperience(unsigned long exp) { experience = exp; }
void Account::addExperience(unsigned long exp) { experience += exp; }

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
 