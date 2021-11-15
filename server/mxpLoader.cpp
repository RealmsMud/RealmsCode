/*
 * mxpLoader.h
 *   mxp Loader
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

#include <string>                    // for string, allocator, operator<=>
#include <utility>                   // for move

#include "builders/mxpBuilder.hpp"  // for mxpBuilder
#include "config.hpp"               // for Config, mxpVariableMap
#include "mxp.hpp"                  // for mxpVariable, getArmor, getArmor...

void addToMap(MxpElement &&mxpElement, MxpElementMap &mxpElementMap) {
    mxpElementMap.emplace(mxpElement.getName(), std::move(mxpElement));
}

bool Config::loadMxpElements() {
    addToMap(MxpBuilder()
        .name("gold")
        .command("COLOR #FFD700")
        .color("l")
    , mxpElements);
    addToMap(MxpBuilder()
        .name("cerulean")
        .command("COLOR #009CFF")
        .color("e")
    , mxpElements);
    addToMap(MxpBuilder()
        .name("pink")
        .command("COLOR #FF5ADE")
        .color("p")
    , mxpElements);
    addToMap(MxpBuilder()
        .name("skyblue")
        .command("COLOR #82E6FF")
        .color("s")
    , mxpElements);
    addToMap(MxpBuilder()
        .name("darkgrey")
        .command("COLOR #484848")
        .color("E")
    , mxpElements);
    addToMap(MxpBuilder()
        .name("brown")
        .command("COLOR #95601A")
        .color("o")
    , mxpElements);
    addToMap(MxpBuilder()
        .name("player")
        .mxpType("send")
        .command("tell &Name; ")
        .hint("Send a message to &Name;")
        .prompt("true")
        .attributes("Name")
    , mxpElements);


    return true;
}