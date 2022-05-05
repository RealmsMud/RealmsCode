/*
 * areaRooms.hpp
 *   Header file for AreaRoom class
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

#include "mudObjects/rooms.hpp"
#include "mudObjects/players.hpp"

class AreaRoom: public BaseRoom {
public:
    AreaRoom(std::shared_ptr<Area> a, const MapMarker *m=0);
    ~AreaRoom();
    bool operator< (const AreaRoom& t) const;

    void reset();
    WanderInfo* getRandomWanderInfo();
    Size getSize() const;

    bool    canDelete();
    void    recycle();
    bool    updateExit(std::string_view dir);
    void    updateExits();
    bool    isInteresting(const std::shared_ptr<Player> viewer) const;
    bool    isRoad() const;
    bool    isWater() const;
    bool    canSave() const;
    void    save(const std::shared_ptr<Player>& player=0) const;
    void    load(xmlNodePtr rootNode);
    CatRef  getUnique(std::shared_ptr<Creature> creature, bool skipDec=false);
    void    setMapMarker(const MapMarker* m);
    bool    spawnHerbs();

    bool flagIsSet(int flag) const;
    void setFlag(int flag);

    const Fishing* doGetFishing(short y, short x) const;
    const Fishing* getFishing() const;

    // any attempt to enter this area room (from any direction)
    // will lead you to a unique room
    CatRef      unique;

    std::shared_ptr<Area> area;
    MapMarker   mapmarker;

    bool getNeedsCompass() const;
    bool getDecCompass() const;
    bool getStayInMemory() const;

    void setNeedsCompass(bool need);
    void setDecCompass(bool dec);
    void setStayInMemory(bool stay);

    bool swap(const Swap& s);
    bool swapIsInteresting(const Swap& s) const;

    std::string getMsdp(bool showExits = true) const;
protected:
    bool    needsCompass{};
    bool    decCompass{};
    bool    stayInMemory{};
};
