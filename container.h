/*
 * Container.h
 *   Classes to handle classes that can be containers or contained by a container
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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef CONTAINER_H_
#define CONTAINER_H_

#include <set>

class Player;
class Monster;
class Object;
class Containable;
class Container;

struct PlayerPtrLess : public std::binary_function<const Player*, const Player*, bool> {
    bool operator()(const Player* lhs, const Player* rhs) const;
};

struct MonsterPtrLess : public std::binary_function<const Monster*, const Monster*, bool> {
    bool operator()(const Monster* lhs, const Monster* rhs) const;
};

struct ObjectPtrLess : public std::binary_function<const Object*, const Object*, bool> {
    bool operator()(const Object* lhs, const Object* rhs) const;
};

typedef std::set<Player*, PlayerPtrLess> PlayerSet;
typedef std::set<Monster*, MonsterPtrLess> MonsterSet;
typedef std::set<Object*, ObjectPtrLess> ObjectSet;



class Container {
public:
    Container();
    virtual ~Container() {};

    PlayerSet players;
    MonsterSet monsters;
    ObjectSet objects;

    bool remove(Containable* toRemove);
    bool add(Containable* toAdd);
    bool checkAntiMagic(Monster* ignore = 0);
};

class Containable {
public:
    Containable();
    virtual ~Containable() {};

    bool addTo(Container* container);
    bool removeFrom(void);

    void setParent(Container* container);

    Container* getParent();

private:
    Container* parent;   // Parent Container
};

#endif /* CONTAINER_H_ */
