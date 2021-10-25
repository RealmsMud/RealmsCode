/*
 * color.h
 *   Color functions
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef REALMSCODE_COLOR_HPP
#define REALMSCODE_COLOR_HPP


// color.cpp
std::string stripColor(std::string_view colored);
std::string escapeColor(std::string_view colored);
std::string padColor(const std::string &toPad, size_t pad);
size_t lengthNoColor(std::string_view colored);

#endif //REALMSCODE_COLOR_HPP
