/*
 * ships.h
 *   Header file for ships
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
#ifndef SHIPS_H_
#define SHIPS_H_

#include <list>
#include <libxml/parser.h>  // for xmlNodePtr

#include "catRef.hpp"
#include "location.hpp"
#include "range.hpp"
#include "swap.hpp"

#define SHIP_SEARCH_CHANCE  75

class cmd;
class BaseRoom;
class Player;

class ShipRaid {
protected:
    bool    record;
    int     searchMob;
    int     raidMob;
    int     minSpawnNum;
    int     maxSpawnNum;
    CatRef  dump;
    CatRef  prison;
    std::string dumpTalk;
    std::string prisonTalk;
    std::string dumpAction;
    std::string prisonAction;
    bool    unconInPrison;
    
public:
    ShipRaid();
    void load(xmlNodePtr curNode);
    void save(xmlNodePtr node) const;
        
    int getRecord() const;
    int getSearchMob() const;
    int getRaidMob() const;
    int getMinSpawnNum() const;
    int getMaxSpawnNum() const;
    CatRef getDump() const;
    CatRef getPrison() const;
    std::string getDumpTalk() const;
    std::string getPrisonTalk() const;
    std::string getDumpAction() const;
    std::string getPrisonAction() const;
    bool getUnconInPrison() const;
    bool swap(const Swap& s);
};



class ShipExit {
protected:
    // note that flag X_MOVING gets set on these exits automatically.
    // so don't set that yourself
    std::string name;

    bool    raid;
    char    flags[16]{};
    std::string arrives;
    std::string departs;

public:
    ShipExit();

    Location    origin;
    Location    target;
    
    void save(xmlNodePtr curNode) const;
    void load(xmlNodePtr curNode);
    bool createExit();
    void removeExit();
    void spawnRaiders(ShipRaid* sRaid);
    BaseRoom* getRoom(bool useOrigin);
    bool swap(const Swap& s);
    
    std::string getName() const;
    bool getRaid() const;
    std::string getArrives() const;
    std::string getDeparts() const;
    
    const char *getFlags() const;
    void setFlags(char f);
};


class ShipStop {
public:
    ShipStop();
    ~ShipStop();
    
    void load(xmlNodePtr curNode);
    void loadExits(xmlNodePtr curNode);
    void save(xmlNodePtr rootNode) const;
    void spawnRaiders();
    bool belongs(const CatRef& cr);
    bool swap(const Swap& s);
    
    // name of the stop, only needed if canQuery in Ship is true,
    // but still helpful
    std::string name;
    
    // if empty, it won't be announced
    std::string arrives;        // announced when ship arrives
    std::string lastcall;       // announced 1 hour before ship departs
    std::string departs;        // announced when ship departs
    
    // announce room range
    std::list<Range> announce;

    std::list<ShipExit*> exits;
    ShipRaid    *raid;
    
    // stored in minutes
    int         to_next;        // how long until it reaches the next stop
    int         in_dock;        // how long it stays in dock
};


class Ship {
public:
    Ship();
    ~Ship();
    
    void load(xmlNodePtr curNode);
    void loadStops(xmlNodePtr curNode);
    void save(xmlNodePtr rootNode) const;
    bool belongs(const CatRef& cr);
    bool swap(const Swap& s);
    
    // this info gets changed by the mud to keep track of where stuff is
    bool    inPort;         // if the ship is docked
    int     timeLeft;       // time left until departure / arrival
    int     clan;           // which clan can view where this ship is
    bool    canQuery;       // will report when mud where the ship is

    // room range of the boat, used in announcing
    std::list<Range> ranges;

    std::string name;               // name of the ship
    // Message: At sea, sailing to Highport.
    //          [transit], [movement] to [stop name, declared in stop]
    // Message: In dock at Highport.
    //          [docked] at [stop name, declared in stop]
    std::string transit;
    std::string movement;
    std::string docked;
    
    // the current stop is always at the front - this list will
    // cycle through the stops
    std::list<ShipStop*> stops;
};



void shipBroadcastRange(Ship *ship, ShipStop *stop, const std::string& message);
int cmdQueryShips(Player* player, cmd* cmnd);
int shipSetExits(Ship *ship, ShipStop *stop);
int shipDeleteExits(Ship *ship, ShipStop *stop);

#endif /*SHIPS_H_*/
