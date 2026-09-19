/*
 * loginSession.hpp
 *   Per-socket login scratch strings, no I/O.
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

#include <string>

struct LoginSession {
    // sub-state of LOGIN_ACCOUNT; login() dispatches on it
    enum class Step {
        EntryChoice,
        LockoutPassword,
        AccountName,
        AccountDmPassword,
        CheckCreateAccount,
        AccountPassword,
        AccountCreatePassword,
        LegacyName,
        AccountMenu,
        ClaimCharacter,
        ClaimPassword,
        SetEmail,
        SetEmailConfirm,
        LegacyPassword,
    };
    Step step = Step::EntryChoice;

    std::string accountName;      // pending/active account name (also read by char creation's DM check)
    std::string email;            // email being set/confirmed
    std::string targetCharacter;  // legacy-login name or claim-target character
    std::string lockoutPassword;  // site-ban password the entrant must match
};
