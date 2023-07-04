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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

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
    [[nodiscard]] std::string str() const;
    void    copyVars(const CatRefInfo* cri);
    bool    swap(const Swap& s);
    [[nodiscard]] const cWeather* getWeather() const;

    [[nodiscard]] std::string getFishing() const;
    [[nodiscard]] int getId() const;
    [[nodiscard]] std::string getArea() const;
    [[nodiscard]] std::string getName() const;
    [[nodiscard]] std::string getWorldName() const;
    [[nodiscard]] std::string getYearsSince() const;
    [[nodiscard]] std::string getParent() const;
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
    std::string fishing;

    int     id{};         // 0 for unique rooms, a number for areas
    std::string area;       // "area" is a special area for the overland
    std::string name;       // display purposes only
    std::string worldName;  // for cmdTime
    std::string yearsSince; // for printtime
    std::string parent;     // which catrefinfo to inherit limbo/recall from, misc by default
    int     yearOffset{};
    int     teleportWeight{};
    int     teleportZone{};
    int     trackZone{};
};
