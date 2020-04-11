/*
 * money.h
 *   Money
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

#ifndef _MONEY_H
#define _MONEY_H

#include <libxml/parser.h>  // for xmlNodePtr

#include "bstring.hpp"

enum Coin {
    MIN_COINS = 0,

    COPPER =    0,
    SILVER =    1,
    GOLD =      2,
    PLATINUM =  3,
    ALANTHIUM = 4,

    MAX_COINS = 4
};



class Money {
public:
    Money();
    Money(unsigned long n, Coin c);
    void load(xmlNodePtr curNode);
    void save(const char* name, xmlNodePtr curNode) const;

    [[nodiscard]] bool isZero() const;
    void zero();

    bool operator==(const Money& mn) const;
    bool operator!=(const Money& mn) const;
    unsigned long operator[](Coin c) const;
    [[nodiscard]] unsigned long get(Coin c) const;

    void set(unsigned long n, Coin c);
    void add(unsigned long n, Coin c);
    void sub(unsigned long n, Coin c);
    void set(Money mn);
    void add(Money mn);
    void sub(Money mn);

    [[nodiscard]] bstring str() const;

    static bstring coinNames(Coin c);
protected:
    unsigned long m[MAX_COINS+1]{};
};


#endif  /* _MONEY_H */

