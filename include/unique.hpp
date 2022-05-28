/*
 * unique.h
 *   Header file for unique items
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

#ifndef UNIQUE_H_
#define UNIQUE_H_

#include <list>
#include <libxml/parser.h>  // for xmlNodePtr

#include "catRef.hpp"

class Creature;
class Object;
class Player;

class Lore {
protected:
    CatRef  info;
    int     limit;
public:
    Lore();
    void save(xmlNodePtr curNode) const;
    void load(xmlNodePtr rootNode);

    CatRef getInfo() const;
    int getLimit() const;
    void setInfo(const CatRef& cr);
    void setLimit(int i);

    static bool isLore(const std::shared_ptr<const Object>&  object);
    static bool hasLore(const std::shared_ptr<const Object>&  object);
    static bool canHave(const std::shared_ptr<const Player>& player, const CatRef& cr);
    static bool canHave(const std::shared_ptr<const Player>& player, const std::shared_ptr<const Object>  object);
    static bool canHave(const std::shared_ptr<const Player>& player, const std::shared_ptr<const Object>  object, bool checkBagOnly);

    static void add(const std::shared_ptr<Player>& player, const std::shared_ptr<const Object>  object, bool subObjects=false);
    static bool remove(const std::shared_ptr<Player>& player, const std::shared_ptr<const Object>  object, bool subObjects=false);
    static void reset(const std::shared_ptr<Player>& player, std::shared_ptr<Creature> creature=0);
};



class UniqueObject {
public:
    UniqueObject();

    CatRef  item;
    int     itemLimit;

    void save(xmlNodePtr curNode) const;
    void load(xmlNodePtr rootNode);
};



class UniqueOwner {
protected:
    long    time;       // time they picked it up
    CatRef  item;
    std::string owner;

public:
    UniqueOwner();
    void save(xmlNodePtr curNode) const;
    void load(xmlNodePtr rootNode);
    void show(const std::shared_ptr<Player>& player);

    long getTime() const;
    bool is(const std::shared_ptr<Player>& player, const CatRef& cr) const;
    bool is(const std::shared_ptr<Player>& player, const std::shared_ptr<const Object> & object) const;
    void set(const std::shared_ptr<Player>& player, const std::shared_ptr<const Object>&  object);
    void set(const std::shared_ptr<Player>& player);

    bool runDecay(long t, int decay, int max);
    void removeUnique(bool destroy);
    void doRemove(std::shared_ptr<Player> player, const std::shared_ptr<Object>&  parent, std::shared_ptr<Object>  object, bool online, bool destroy);
};


class Unique {
private:
    int     globalLimit;    // only X of the items below may be in game, if 0, acts as lore item
    int     playerLimit;    // you may have X number of items out of the list
    int     inGame;         // essentially equal to owners.size()

    int     decay;      // how long until it starts decaying
    int     max;        // how long until 100% decay

    std::list<UniqueOwner> owners;      // who has unique items
    std::list<UniqueObject> objects;    // group of items, itemlimit
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

    void show(const std::shared_ptr<Player>& player);
    bool inObjectsList(const CatRef& cr) const;
    bool setObjectLimit(const CatRef& cr, int id);
    int numInOwners(const CatRef& cr) const;
    CatRef firstObject();
    int numObjects(const std::shared_ptr<Player>& player) const;

    void addObject(const CatRef& cr);
    void deUnique(const CatRef& cr);
    void runDecay(long t, const std::shared_ptr<Player>& player=0);

    bool addOwner(const std::shared_ptr<Player>& player, const std::shared_ptr<const Object> & object);
    bool deleteOwner(const std::shared_ptr<Player>& player, const std::shared_ptr<const  Object> & object);
    void transferOwner(const std::shared_ptr<Player>& owner, const std::shared_ptr<Player> target, const std::shared_ptr<const Object> & object);
    void remove(const std::shared_ptr<Player>& player);
    bool checkItemLimit(const CatRef& item) const;
    bool canGet(const std::shared_ptr<Player>& player, const CatRef& item, bool transfer) const;

    static void broadcastDestruction(const std::string &owner, const std::shared_ptr<const Object>&  object);
    static bool canLoad(const std::shared_ptr<Object>&  object);
    static bool is(const std::shared_ptr<const Object>&  object);
    static bool isUnique(const std::shared_ptr<const Object>&  object);
    static bool hasUnique(const std::shared_ptr<const Object>&  object);
    static bool canGet(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>  object, bool transfer=false);
};


namespace Limited {
    bool is(const std::shared_ptr<const Object>&  object);
    bool isLimited(const std::shared_ptr<const Object>&  object);
    bool hasLimited(const std::shared_ptr<const Object>&  object);
    bool remove(const std::shared_ptr<Player>& player, const std::shared_ptr<const Object> & object, bool save=true);
    int addOwner(const std::shared_ptr<Player>& player, const std::shared_ptr<const Object> & object);
    bool deleteOwner(const std::shared_ptr<Player>& player, const std::shared_ptr<const Object> & object, bool save=true, bool lore=true);
    void transferOwner(std::shared_ptr<Player> owner, const std::shared_ptr<Player>& target, const std::shared_ptr<const Object> & object);
}


#endif /*UNIQUE_H_*/
