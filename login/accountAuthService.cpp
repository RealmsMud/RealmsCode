/*
 * accountAuthService.cpp
 *   Headless account/auth logic. See accountAuthService.hpp.
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

#include <cctype>
#include <ctime>
#include <memory>
#include <string>

#include "login/accountAuthService.hpp"
#include "account.hpp"
#include "mudObjects/players.hpp"
#include "server.hpp"
#include "proto.hpp"   // parse_name
#include "xml.hpp"     // loadPlayer

namespace AccountAuth {

NameError checkName(const std::string& name) {
    const std::size_t len = name.length();

    if(len == 0 || !std::isalpha(static_cast<unsigned char>(name[0])))
        return NameError::FirstNotLetter;
    if(len < kMinNameLen)
        return NameError::TooShort;
    if(len >= kNameLenLimit)
        return NameError::TooLong;

    int nonalpha = 0;
    for(const char c : name)
        if(!std::isalpha(static_cast<unsigned char>(c)))
            nonalpha++;

    if(nonalpha && len < kMinNonAlphaNameLen)
        return NameError::NonAlphaNeedsLength;
    if(nonalpha > 1)
        return NameError::TooManyNonAlpha;

    for(const char c : name)
        if(!std::isalpha(static_cast<unsigned char>(c)) && c != '\'' && c != '-')
            return NameError::NonAlphaChar;

    if(name[len-1] == '\'' || name[len-1] == '-' ||
       (len > 1 && (name[len-2] == '\'' || name[len-2] == '-')))
        return NameError::LastCharsNotLetters;

    if(!parse_name(name))
        return NameError::NotAllowed;

    return NameError::Ok;
}

std::string nameErrorMessage(NameError error) {
    switch(error) {
    case NameError::Ok:                  return "";
    case NameError::FirstNotLetter:      return "The first character of your name must be a letter.";
    case NameError::TooShort:            return "Name must be at least 3 characters.";
    case NameError::TooLong:             return "Name must be less than 20 characters.";
    case NameError::NonAlphaNeedsLength: return "Name must be at least 6 characters in order to contain a - or '.";
    case NameError::TooManyNonAlpha:     return "May not have more than one non-alpha character in your name.";
    case NameError::NonAlphaChar:        return "Name must be alphabetic.\nIt may only contain the non-alpha characters ' and -.";
    case NameError::LastCharsNotLetters: return "The last two characters of your name must be letters.";
    case NameError::NotAllowed:          return "That name is not allowed.";
    }
    return "";
}

EmailError checkEmail(const std::string& email) {
    if(email.find('@') == std::string::npos || email.find('.') == std::string::npos)
        return EmailError::BadFormat;
    if(email.length() > kMaxEmailLen)
        return EmailError::TooLong;
    return EmailError::Ok;
}

ResolveResult resolveAccount(const std::string& name) {
    ResolveResult result;
    result.account = gServer->getOrLoadAccount(name);
    if(!result.account)
        return result;

    result.found = true;
    if(result.account->isBanned()) {
        result.banned = true;
        result.banReason = result.account->getBanReason();
    }
    return result;
}

bool verifyPassword(const Account& account, const std::string& password) {
    return account.isPassword(password);
}

void recordLogin(Account& account) {
    account.updateLastLogin();
    account.save();
}

bool isValidPassword(const std::string& password) {
    return Account::isValidPassword(password);
}

std::shared_ptr<Account> createAccount(const std::string& name, const std::string& password) {
    auto account = std::make_shared<Account>(name);
    account->setPassword(password);
    account->setCreated(time(nullptr));
    account->updateLastLogin();
    if(!account->save())
        return nullptr;
    gServer->cacheAccount(account);
    return account;
}

LegacyLookup lookupLegacyCharacter(const std::string& characterName) {
    LegacyLookup lookup;
    if(!loadPlayer(characterName, lookup.player)) {
        lookup.player = nullptr;
        return lookup;
    }
    lookup.found = true;
    lookup.alreadyClaimed = !lookup.player->getAccountName().empty();
    return lookup;
}

bool verifyCharacterPassword(const Player& player, const std::string& password) {
    return player.isPassword(password);
}

bool claimCharacter(Account& account, Player& player) {
    if(!account.addCharacter(player.getName()))
        return false;
    player.setAccountName(account.getName());
    return true;
}

} // namespace AccountAuth
