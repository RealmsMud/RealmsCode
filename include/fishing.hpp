/*
 * fishing.h
 *   Fishing
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

#ifndef _FISHING_H
#define _FISHING_H

#include <list>

#include "catRef.hpp"

class FishingItem {
public:
    FishingItem();
    void    load(xmlNodePtr rootNode);

    [[nodiscard]] CatRef getFish() const;
    [[nodiscard]] bool isDayOnly() const;
    [[nodiscard]] bool isNightOnly() const;
    [[nodiscard]] short getWeight() const;    // as in: probability
    [[nodiscard]] short getMinQuality() const;
    [[nodiscard]] short getMinSkill() const;
    [[nodiscard]] long getExp() const;
    [[nodiscard]] bool isMonster() const;
    [[nodiscard]] bool willAggro() const;
protected:
    CatRef fish;
    bool dayOnly;
    bool nightOnly;
    short weight;   // as in: probability
    short minQuality;
    short minSkill;
    long exp;

    bool monster;
    bool aggro;
};

class Fishing {
public:
    Fishing();
    void    load(xmlNodePtr rootNode);
    [[nodiscard]] const FishingItem* getItem(short skill, short quality) const;
    [[nodiscard]] bool empty() const;
    bstring id;
    std::list<FishingItem> items;
};


#endif  /* _FISHING_H */

