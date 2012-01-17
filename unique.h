/*
 * unique.h
 *	 Header file for unique items
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

#ifndef UNIQUE_H_
#define UNIQUE_H_



class Lore {
protected:
	CatRef	info;
	int		limit;
public:
	Lore();
	void save(xmlNodePtr curNode) const;
	void load(xmlNodePtr rootNode);

	CatRef getInfo() const;
	int getLimit() const;
	void setInfo(const CatRef cr);
	void setLimit(int i);

	static bool isLore(const Object* object);
	static bool hasLore(const Object* object);
	static bool canHave(const Player* player, const CatRef cr);
	static bool canHave(const Player* player, const Object* object);
	static bool canHave(const Player* player, const Object* object, bool checkBagOnly);

	static void add(Player* player, const Object* object, bool subObjects=false);
	static bool remove(Player* player, const Object* object, bool subObjects=false);
	static void reset(Player* player, Creature* creature=0);
};



class UniqueObject {
public:
	UniqueObject();

	CatRef	item;
	int		itemLimit;

	void save(xmlNodePtr curNode) const;
	void load(xmlNodePtr rootNode);
};



class UniqueOwner {
protected:
	long	time;		// time they picked it up
	CatRef	item;
	bstring	owner;	

public:
	UniqueOwner();
	void save(xmlNodePtr curNode) const;
	void load(xmlNodePtr rootNode);
	void show(const Player* player);

	long getTime() const;
	bool is(const Player* player, const CatRef cr) const;
	bool is(const Player* player, const Object* object) const;
	void set(const Player* player, const Object* object);
	void set(const Player* player);

	bool runDecay(long t, int decay, int max);
	void removeUnique(bool destroy);
	void doRemove(Player* player, Object* parent, Object* object, bool online, bool destroy);
};


class Unique {
private:
	int		globalLimit;	// only X of the items below may be in game, if 0, acts as lore item
	int		playerLimit;	// you may have X number of items out of the list
	int		inGame;			// essentially equal to owners.size()

	int		decay;		// how long until it starts decaying
	int		max;		// how long until 100% decay

	std::list<UniqueOwner> owners;		// who has unique items
	std::list<UniqueObject> objects;	// group of items, itemlimit
public:
	Unique();
	~Unique();
	void save(xmlNodePtr curNode) const;
	void load(xmlNodePtr rootNode);

	int getGlobalLimit() const;
	int getPlayerLimit() const;
	int getInGame() const;
	int getDecay() const;
	int getMax() const;

	void setGlobalLimit(int num);
	void setPlayerLimit(int num);
	void setDecay(int num);
	void setMax(int num);

	void show(const Player* player);
	bool inObjectsList(const CatRef cr) const;
	bool setObjectLimit(const CatRef cr, int id);
	int numInOwners(const CatRef cr) const;
	const CatRef firstObject();
	int numObjects(const Player* player) const;

	void addObject(CatRef cr);
	void deUnique(CatRef cr);
	void runDecay(long t, Player* player=0);

	bool addOwner(const Player* player, const Object* object);
	bool deleteOwner(const Player* player, const Object* object);
	void transferOwner(const Player* owner, const Player* target, const Object* object);
	void remove(const Player* player);
	bool checkItemLimit(const CatRef item) const;
	bool canGet(const Player* player, const CatRef item, bool transfer) const;

	static void broadcastDestruction(const bstring owner, const Object* object);
	static bool canLoad(const Object* object);
	static bool is(const Object* object);
	static bool isUnique(const Object* object);
	static bool hasUnique(const Object* object);
	static bool canGet(const Player* player, const Object* object, bool transfer=false);
};


namespace Limited {
	bool is(const Object* object);
	bool isLimited(const Object* object);
	bool hasLimited(const Object* object);
	bool remove(Player* player, const Object* object, bool save=true);
	int	addOwner(Player* player, const Object* object);
	bool deleteOwner(Player* player, const Object* object, bool save=true, bool lore=true);
	void transferOwner(Player* owner, Player* target, const Object* object);
}


#endif /*UNIQUE_H_*/
