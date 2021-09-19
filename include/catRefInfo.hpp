/*
 * catRefInfo.h
 *   Category Reference info
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

#ifndef _CATREFINFO_H
#define _CATREFINFO_H

#include <map>

#include "season.hpp"
#include "swap.hpp"

class cWeather;
class cSeason;

class CatRefInfo {
public:
    CatRefInfo();
    ~CatRefInfo();
    void    clear();
    void    load(xmlNodePtr rootNode);
    void    loadSeasons(xmlNodePtr curNode);
    void    save(xmlNodePtr curNode) const;
    [[nodiscard]] bstring str() const;
    void    copyVars(const CatRefInfo* cri);
    bool    swap(const Swap& s);
    [[nodiscard]] const cWeather* getWeather() const;

    [[nodiscard]] bstring getFishing() const;
    [[nodiscard]] int getId() const;
    [[nodiscard]] bstring getArea() const;
    [[nodiscard]] bstring getName() const;
    [[nodiscard]] bstring getWorldName() const;
    [[nodiscard]] bstring getYearsSince() const;
    [[nodiscard]] bstring getParent() const;
    [[nodiscard]] int getYearOffset() const;
    [[nodiscard]] int getLimbo() const;
    [[nodiscard]] int getRecall() const;
    [[nodiscard]] int getTeleportWeight() const;
    [[nodiscard]] int getTeleportZone() const;
    [[nodiscard]] int getTrackZone() const;

    std::map<Season,cSeason*> seasons;  // the seaons hold the weather

    // Some code is written such that these attributes need to be public.
    // That code ought to be rewritten:
    // Config::finishSwap in swap.cpp
    // getSpecialArea in rooms.cpp
    int     limbo{};
    int     recall{};
protected:
    bstring fishing;

    int     id{};         // 0 for unique rooms, a number for areas
    bstring area;       // "area" is a special area for the overland
    bstring name;       // display purposes only
    bstring worldName;  // for cmdTime
    bstring yearsSince; // for printtime
    bstring parent;     // which catrefinfo to inherit limbo/recall from, misc by default
    int     yearOffset{};
    int     teleportWeight{};
    int     teleportZone{};
    int     trackZone{};
};


#endif  /* _CATREFINFO_H */

