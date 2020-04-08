/*
 * dice.h
 *   Dice
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _DICE_H
#define _DICE_H

#include "common.hpp"

class Dice {
public:
    Dice();
    Dice(unsigned short n, unsigned short s, short p);
    bool operator==(const Dice& d) const;
    bool operator!=(const Dice& d) const;
    void clear();
    void load(xmlNodePtr curNode);
    void save(xmlNodePtr curNode, const char* name) const;

    [[nodiscard]] int roll() const;
    [[nodiscard]] int average() const;
    [[nodiscard]] int low() const;
    [[nodiscard]] int high() const;
    [[nodiscard]] bstring str() const;

    [[nodiscard]] double getMean() const;
    [[nodiscard]] unsigned short getNumber() const;
    [[nodiscard]] unsigned short getSides() const;
    [[nodiscard]] short getPlus() const;

    void setMean(double m);
    void setNumber(unsigned short n);
    void setSides(unsigned short s);
    void setPlus(short p);
protected:
    double mean{};        // If set, replaces number/sides/plus
    unsigned short number{};
    unsigned short sides{};
    short plus{};
};


#endif  /* _DICE_H */

