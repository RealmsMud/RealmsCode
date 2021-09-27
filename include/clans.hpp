/*
 * clans.h
 *   Header file for clans
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
#ifndef CLAN_H_
#define CLAN_H_

#include <map>
#include <libxml/parser.h>  // for xmlNodePtr

#include "bstring.hpp"

class Clan {
public:
    Clan();
    void load(xmlNodePtr curNode);

    [[nodiscard]] unsigned int getId() const;
    [[nodiscard]]     unsigned int getJoin() const;
    [[nodiscard]] unsigned int getRescind() const;
    [[nodiscard]] unsigned int getDeity() const;
    [[nodiscard]] bstring getName() const;
    [[nodiscard]] short   getSkillBonus(std::string_view skill) const;
protected:
    unsigned int id;
    unsigned int join;
    unsigned int rescind;
    unsigned int deity;
    bstring name;
public:
    // only to make iteration easy
    std::map<bstring, short> skillBonus;
};

#endif /*CLAN_H_*/
