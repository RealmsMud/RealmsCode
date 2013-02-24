/*
 * ships.h
 *	 Header file for ships
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *  
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef SHIPS_H_
#define SHIPS_H_

#define SHIP_SEARCH_CHANCE	75

class ShipRaid {
protected:
	bool	record;
	int		searchMob;
	int		raidMob;
	int		minSpawnNum;
	int		maxSpawnNum;
	CatRef	dump;
	CatRef	prison;
	bstring dumpTalk;
	bstring prisonTalk;
	bstring dumpAction;
	bstring prisonAction;
	bool	unconInPrison;
	
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
	bstring getDumpTalk() const;
	bstring getPrisonTalk() const;
	bstring getDumpAction() const;
	bstring getPrisonAction() const;
	bool getUnconInPrison() const;
	bool swap(Swap s);
};



class ShipExit {
protected:
	// note that flag X_MOVING gets set on these exits automatically.
	// so don't set that yourself
	bstring	name;

	bool	raid;
	char	flags[16];
	bstring	arrives;
	bstring	departs;

public:
	ShipExit();

	Location	origin;
	Location	target;
	
	void save(xmlNodePtr curNode) const;
	void load(xmlNodePtr curNode);
	bool createExit();
	void removeExit();
	void spawnRaiders(ShipRaid* sRaid);
	BaseRoom* getRoom(bool useOrigin);
	bool swap(Swap s);
	
	bstring getName() const;
	bool getRaid() const;
	bstring getArrives() const;
	bstring getDeparts() const;
	
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
	bool belongs(CatRef cr);
	bool swap(Swap s);
	
	// name of the stop, only needed if canQuery in Ship is true,
	// but still helpful
	bstring name;
	
	// if empty, it won't be announced
	bstring arrives;		// announced when ship arrives
	bstring lastcall;		// announced 1 hour before ship departs
	bstring departs;		// announced when ship departs
	
	// announce room range
	std::list<Range> announce;

	std::list<ShipExit*> exits;
	ShipRaid	*raid;
	
	// stored in minutes
	int			to_next;		// how long until it reaches the next stop
	int			in_dock;		// how long it stays in dock
};


class Ship {
public:
	Ship();
	~Ship();
	
	void load(xmlNodePtr curNode);
	void loadStops(xmlNodePtr curNode);
	void save(xmlNodePtr rootNode) const;
	bool belongs(CatRef cr);
	bool swap(Swap s);
	
	// this info gets changed by the mud to keep track of where stuff is
	bool	inPort;			// if the ship is docked
	int		timeLeft;		// time left until departure / arrival
	int		clan;			// which clan can view where this ship is
	bool	canQuery;		// will report when mud where the ship is

	// room range of the boat, used in announcing
	std::list<Range> ranges;

	bstring name;				// name of the ship
	// Message:	At sea, sailing to Highport.
	// 			[transit], [movement] to [stop name, declared in stop]
	// Message:	In dock at Highport.
	//			[docked] at [stop name, declared in stop]
	bstring transit;
	bstring movement;
	bstring docked;
	
	// the current stop is always at the front - this list will
	// cycle through the stops
	std::list<ShipStop*> stops;
};



void shipBroadcastRange(Ship *ship, ShipStop *stop, bstring message);
int cmdQueryShips(Player* player, cmd* cmnd);
void update_ships();
int shipSetExits(Ship *ship, ShipStop *stop);
int shipDeleteExits(Ship *ship, ShipStop *stop);

#endif /*SHIPS_H_*/
