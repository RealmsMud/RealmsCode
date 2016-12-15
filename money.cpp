/*
 * money.cpp
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
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "xml.h"

//*********************************************************************
//                      Money
//*********************************************************************

Money::Money() {
    zero();
}

// Create some new money with n amount of Coin c
Money::Money(unsigned long n, Coin c) {
    zero();
    set(n,c);
}
//*********************************************************************
//                      load
//*********************************************************************

void Money::load(xmlNodePtr curNode) {
    zero();
    xml::loadNumArray<unsigned long>(curNode, m, "Coin", MAX_COINS+1);
}

//*********************************************************************
//                      save
//*********************************************************************

void Money::save(const char* name, xmlNodePtr curNode) const {
    saveULongArray(curNode, name, "Coin", m, MAX_COINS+1);
}

//*********************************************************************
//                      isZero
//*********************************************************************

bool Money::isZero() const {
    for(Coin i = MIN_COINS; i < MAX_COINS; i = (Coin)((int)i + 1))
        if(m[i])
            return(false);
    return(true);
}

//*********************************************************************
//                      zero
//*********************************************************************

void Money::zero() { ::zero(m, sizeof(m)); }

//*********************************************************************
//                      operators
//*********************************************************************

bool Money::operator==(const Money& mn) const {
    for(Coin i = MIN_COINS; i < MAX_COINS; i = (Coin)((int)i + 1))
        if(m[i] != mn.get(i))
            return(false);
    return(true);
}

bool Money::operator!=(const Money& mn) const {
    return(!(*this==mn));
}

unsigned long Money::operator[](Coin c) const { return(m[c]); }

//*********************************************************************
//                      get
//*********************************************************************

unsigned long Money::get(Coin c) const { return(m[c]); }

//*********************************************************************
//                      add
//*********************************************************************

void Money::add(unsigned long n, Coin c) { set(m[c] + n, c); }

void Money::add(Money mn) {
    for(Coin i = MIN_COINS; i < MAX_COINS; i = (Coin)((int)i + 1))
        add(mn[i], i);
}

//*********************************************************************
//                      sub
//*********************************************************************

void Money::sub(unsigned long n, Coin c) { set(m[c] - n, c); }

void Money::sub(Money mn) {
    for(Coin i = MIN_COINS; i < MAX_COINS; i = (Coin)((int)i + 1))
        sub(mn[i], i);
}

//*********************************************************************
//                      set
//*********************************************************************

void Money::set(unsigned long n, Coin c) { m[c] = MIN(2000000000, n); }

void Money::set(Money mn) {
    for(Coin i = MIN_COINS; i < MAX_COINS; i = (Coin)((int)i + 1))
        set(mn[i], i);
}

//*********************************************************************
//                      str
//*********************************************************************

bstring Money::str() const {
    bool found=false;
    std::stringstream oStr;
    oStr.imbue(std::locale(""));

    for(Coin i = MIN_COINS; i < MAX_COINS; i = (Coin)((int)i + 1)) {
        if(m[i]) {
            if(found)
                oStr << ", ";
            found = true;
            oStr << m[i] << " " << coinNames(i).toLower() << " coin" << (m[i] != 1 ? "s" : "");
        }
    }

    if(!found)
        oStr << "0 coins";

    return(oStr.str());
}

//*********************************************************************
//                      coinNames
//*********************************************************************

bstring Money::coinNames(Coin c) {
    switch(c) {
    case COPPER:
        return("Copper");
    case SILVER:
        return("Silver");
    case GOLD:
        return("Gold");
    case PLATINUM:
        return("Platinum");
    case ALANTHIUM:
        return("Alanthium");
    default:
        break;
    }
    return("");
}
