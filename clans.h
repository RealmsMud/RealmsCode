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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef CLAN_H_
#define CLAN_H_

class Clan {
public:
    Clan();
    void load(xmlNodePtr curNode);

    unsigned int getId() const;
    unsigned int getJoin() const;
    unsigned int getRescind() const;
    unsigned int getDeity() const;
    bstring getName() const;
    short   getSkillBonus(bstring skill) const;
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
