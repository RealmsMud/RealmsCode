/*
 * accountAuthService.hpp
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

#include <cstddef>
#include <memory>
#include <string>

class Account;
class Player;

namespace AccountAuth {
    enum class NameError {
        Ok,
        FirstNotLetter,        // first character is not a letter
        TooShort,              // fewer than 3 characters
        TooLong,               // 20 or more characters
        NonAlphaNeedsLength,   // contains '/- but is shorter than 6 characters
        TooManyNonAlpha,       // more than one non-alpha character
        NonAlphaChar,          // contains a disallowed (non ' or -) character
        LastCharsNotLetters,   // last two characters are not letters
        NotAllowed,            // rejected by the banned-name filter (parse_name)
    };
    NameError checkName(const std::string& name);

    // Single copy of the rejection text. Telnet wording: NonAlphaChar carries the embedded
    // newline of the two lines the telnet prompt has always printed.
    std::string nameErrorMessage(NameError error);

    enum class EmailError {
        Ok,
        BadFormat,   // missing '@' or '.'
        TooLong,     // longer than kMaxEmailLen
    };
    EmailError checkEmail(const std::string& email);

    inline constexpr std::string::size_type kMinPasswordLen = 5;
    inline constexpr std::string::size_type kMaxPasswordLen = 35;
    inline constexpr std::size_t kMaxNameLen = 25;    // transport bound; checkName is stricter
    inline constexpr std::size_t kMinNameLen = 3;
    inline constexpr std::size_t kNameLenLimit = 20;  // checkName rejects names this long or longer
    inline constexpr std::size_t kMinNonAlphaNameLen = 6;   // shortest name that may carry a ' or -
    inline constexpr std::size_t kMaxEmailLen = 255;

    struct ResolveResult {
        bool found = false;
        bool banned = false;
        std::string banReason;
        std::shared_ptr<Account> account;
    };
    ResolveResult resolveAccount(const std::string& name);

    bool verifyPassword(const Account& account, const std::string& password);
    void recordLogin(Account& account);
    bool isValidPassword(const std::string& password);

    std::shared_ptr<Account> createAccount(const std::string& name, const std::string& password);

    struct LegacyLookup {
        bool found = false;
        bool alreadyClaimed = false;
        std::shared_ptr<Player> player;
    };
    LegacyLookup lookupLegacyCharacter(const std::string& characterName);

    bool verifyCharacterPassword(const Player& player, const std::string& password);

    bool claimCharacter(Account& account, Player& player);
}
