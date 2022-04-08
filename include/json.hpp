/*
 * json.hpp
 *   Realms JSON functions
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later

 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

#include <nlohmann/json.hpp>
#include <list>


/**
 * Construct a Json List from the given std::list<Type> if the list is not empty
 */
template <class Type>
void toJsonList(const char* name, nlohmann::json &j, const std::list<Type>& myList) {
    if(myList.empty())
        return;

    auto &jp = j[name] = nlohmann::json();

    for (const auto& item : myList) {
        jp.push_back(nlohmann::json(item));
    }
}