/*
 * uniqueRooms.hpp
 *   Header file for UniqueRoom class
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

#include <string>
#include <boost/dynamic_bitset.hpp>

#include "mudObjects/rooms.hpp"

class UniqueRoom: public BaseRoom {
public:
    UniqueRoom();
    ~UniqueRoom();
    bool operator< (const UniqueRoom& t) const;

    void escapeText();
    int readFromXml(xmlNodePtr rootNode, bool offline=false);
    int saveToXml(xmlNodePtr rootNode, int permOnly) const;
    int saveToFile(int permOnly, LoadType saveType=LoadType::LS_NORMAL);

    [[nodiscard]] std::string getShortDescription() const;
    [[nodiscard]] std::string getLongDescription() const;
    [[nodiscard]] short getLowLevel() const;
    [[nodiscard]] short getHighLevel() const;
    [[nodiscard]] short getMaxMobs() const;
    [[nodiscard]] short getTrap() const;
    [[nodiscard]] CatRef getTrapExit() const;
    [[nodiscard]] short getTrapWeight() const;
    [[nodiscard]] short getTrapStrength() const;
    [[nodiscard]] std::string getFaction() const;
    [[nodiscard]] long getBeenHere() const;
    [[nodiscard]] short getTerrain() const;
    [[nodiscard]] int getRoomExperience() const;
    [[nodiscard]] Size getSize() const;
    [[nodiscard]] bool canPortHere(const Creature* creature=0) const;

    void setShortDescription(std::string_view desc);
    void setLongDescription(std::string_view desc);
    void appendShortDescription(std::string_view desc);
    void appendLongDescription(std::string_view desc);
    void setLowLevel(short lvl);
    void setHighLevel(short lvl);
    void setMaxMobs(short m);
    void setTrap(short t);
    void setTrapExit(const CatRef& t);
    void setTrapWeight(short weight);
    void setTrapStrength(short strength);
    void setFaction(std::string_view f);
    void incBeenHere();
    void setTerrain(short t);
    void setRoomExperience(int exp);
    void setSize(Size s);

    bool swap(const Swap& s);
    bool swapIsInteresting(const Swap& s) const;

    std::string getMsdp(bool showExits = true) const;
protected:
    boost::dynamic_bitset<> flags{128};
    std::string fishing;

    std::string short_desc;     // Descriptions
    std::string long_desc;
    short   lowLevel;       // Lowest level allowed in
    short   highLevel;      // Highest level allowed in
    short   maxmobs;

    short   trap;
    CatRef  trapexit;
    short   trapweight;
    short   trapstrength;
    std::string faction;

    long    beenhere;       // # times room visited

    int     roomExp;
    Size    size;
public:

    CatRef  info;

    Track   track;
    WanderInfo wander;          // Random monster info
    std::map<int, crlasttime> permMonsters; // Permanent/reappearing monsters
    std::map<int, crlasttime> permObjects;  // Permanent/reappearing items

    char    last_mod[24]{};   // Last staff member to modify room.
    struct lasttime lasttime[16];   // For timed room things, like darkness spells.

    char    lastModTime[32]{}; // date created (first *saved)

    char    lastPly[32]{};
    char    lastPlyTime[32]{};

public:

    int getWeight();
    void validatePerms();
    void addPermCrt();
    void addPermObj();

    void destroy();

    bool flagIsSet(int flag) const;
    void setFlag(int flag);
    void clearFlag(int flag);
    bool toggleFlag(int flag);

    std::string getFishingStr() const;
    void setFishing(std::string_view id);
    const Fishing* getFishing() const;
};
