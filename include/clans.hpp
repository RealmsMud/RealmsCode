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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef CLAN_H_
#define CLAN_H_

#include <map>
#include <libxml/parser.h>  // for xmlNodePtr


class Clan {
public:
    Clan();
    void load(xmlNodePtr curNode);

    [[nodiscard]] int getId() const;
    [[nodiscard]]     int getJoin() const;
    [[nodiscard]] int getRescind() const;
    [[nodiscard]] int getDeity() const;
    [[nodiscard]] std::string getName() const;
    [[nodiscard]] short   getSkillBonus(const std::string &skill) const;
protected:
    int id;
    int join;
    int rescind;
    int deity;
    std::string name;
public:
    // only to make iteration easy
    std::map<std::string, short> skillBonus;
};

#endif /*CLAN_H_*/
