/*
 * account.hpp
 *   Account system for multiple characters per user
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

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <ctime>
#include <memory>

class Player;

class Account {
public:
    // Constructors & Destructor
    Account();
    Account(const std::string& name);
    Account(const Account& other);
    Account& operator=(const Account& other);
    ~Account();

    // JSON serialization friends
    friend void to_json(nlohmann::json &j, const Account &account);
    friend void from_json(const nlohmann::json &j, Account &account);

    // Core account functions
    bool save() const;
    static bool load(const std::string& accountName, std::shared_ptr<Account>& account);
    static bool exists(const std::string& accountName);
    static std::string hashPassword(const std::string& password);

    // Getters
    const std::string& getName() const;
    const std::string& getPassword() const;
    const std::string& getEmail() const;
    time_t getCreated() const;
    time_t getLastLogin() const;
    int getCharacterLimit() const;
    const std::vector<std::string>& getCharacterNames() const;
    bool isBanned() const;
    const std::string& getBanReason() const;
    unsigned long getExperience() const;

    // Setters
    void setName(const std::string& name);
    void setPassword(const std::string& password);
    void setEmail(const std::string& email);
    void setCreated(time_t created);
    void setLastLogin(time_t lastLogin);
    void setBanned(bool banned);
    void setBanReason(const std::string& reason);
    void setExperience(unsigned long exp);
    void addExperience(unsigned long exp);

    // Character management
    bool addCharacter(const std::string& characterName);
    bool removeCharacter(const std::string& characterName);
    bool hasCharacter(const std::string& characterName) const;
    bool canCreateCharacter() const;
    int getCharacterCount() const;

    // Password verification
    bool isPassword(const std::string& password) const;
    void updateLastLogin();

    // Validation
    static bool isValidAccountName(const std::string& name);
    static bool isValidPassword(const std::string& password);

private:
    std::string accountName;        // Unique account identifier
    std::string password;           // Hashed password
    std::string email;              // Optional email address
    time_t created;                 // Account creation time
    time_t lastLogin;               // Last login time
    int characterLimit;             // Maximum characters allowed
    std::vector<std::string> characterNames; // List of character names
    bool banned;                    // Is account banned
    std::string banReason;          // Reason for ban if applicable
    unsigned long experience;       // Account experience points

    // Helper functions
    void copyFrom(const Account& other);
    void reset();
}; 