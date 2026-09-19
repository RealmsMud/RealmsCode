/*
 * characterMaterializer.hpp
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

#include <memory>
#include <string>

struct CharacterDraft;
class Player;
class Account;

namespace Create {
    std::shared_ptr<Player> materialize(const CharacterDraft& draft, const Account& account);

    struct PersistResult {
        bool ok = false;
        std::string error;
    };
    PersistResult persistNewCharacter(const std::shared_ptr<Player>& player, Account& account);

    struct CommitResult {
        bool ok = false;
        bool nameTaken = false;
        std::string error;
        std::shared_ptr<Player> player;
    };
    CommitResult commitCharacter(const CharacterDraft& draft, Account& account);
}
