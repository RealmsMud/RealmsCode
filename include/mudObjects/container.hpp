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
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef CONTAINER_H_
#define CONTAINER_H_

#include "mudObject.hpp"

#include <set>

class AreaRoom;
class BaseRoom;
class Containable;
class Container;
class EffectInfo;
class Monster;
class MudObject;
class Object;
class Player;
class Socket;
class UniqueRoom;

class cmd;

struct PlayerPtrLess {
    bool operator()(const std::weak_ptr<Player>& lhs, const std::weak_ptr<Player>& rhs) const;
};

struct MonsterPtrLess {
    bool operator()(const std::shared_ptr<Monster>&  lhs, const std::shared_ptr<Monster>&  rhs) const;
};

struct ObjectPtrLess  {
    bool operator()(const std::shared_ptr<Object>&  lhs, const std::shared_ptr<Object>&  rhs) const;
};

typedef std::set<std::weak_ptr<Player>, PlayerPtrLess> PlayerSet;
typedef std::set<std::shared_ptr<Monster> , MonsterPtrLess> MonsterSet;
typedef std::set<std::shared_ptr<Object> , ObjectPtrLess> ObjectSet;

// Any container or containable item is a MudObject.  Since an object can be both a container and containable...
// make sure we use virtual MudObject as the parent to avoid the "dreaded" diamond

class Container : public virtual MudObject, public inheritable_enable_shared_from_this<Container> {
public:
    Container();
    ~Container() override = default;

    PlayerSet players;
    MonsterSet monsters;
    ObjectSet objects;

    bool purge(bool includePets = false);
    bool purgeMonsters(bool includePets = false);
    bool purgeObjects();

    std::list<std::shared_ptr<Player>> getPlayers();


    std::shared_ptr<Container> remove(Containable* toRemove);
    bool add(const std::shared_ptr<Containable>& toAdd);


    void registerContainedItems() override;
    void unRegisterContainedItems() override;


    bool checkAntiMagic(const std::shared_ptr<Monster>&  ignore = nullptr);

    void doSocialEcho(std::string str, const std::shared_ptr<Creature> & actor, const std::shared_ptr<Creature> & target = nullptr);

    void effectEcho(const std::string &fmt, const std::shared_ptr<MudObject>& actor = nullptr, const std::shared_ptr<MudObject>& applier = nullptr, std::shared_ptr<Socket> ignore = nullptr);

    void wake(const std::string &str, bool noise) const;


    std::string listObjects(const std::shared_ptr<const Player>& player, bool showAll, char endColor ='x' ) const;

    // Find routines
    std::shared_ptr<Creature> findCreaturePython(const std::shared_ptr<Creature>& searcher, const std::string& name, bool monFirst = true, bool firstAggro = false, bool exactMatch = false );
    std::shared_ptr<Creature> findCreature(const std::shared_ptr<const Creature>& searcher,  cmd* cmnd, int num=1) const;
    std::shared_ptr<Creature> findCreature(const std::shared_ptr<const Creature>& searcher, const std::string& name, int num, bool monFirst = true, bool firstAggro = false, bool exactMatch = false) const;
    std::shared_ptr<Creature> findCreature(const std::shared_ptr<const Creature>& searcher, const std::string& name, int num, bool monFirst, bool firstAggro, bool exactMatch, int& match) const;
    std::shared_ptr<Monster>  findMonster(std::shared_ptr<const Creature> searcher,  cmd* cmnd, int num=1) const;
    std::shared_ptr<Monster>  findMonster(std::shared_ptr<const Creature> searcher, const std::string& name, int num, bool firstAggro = false, bool exactMatch = false) const;
    std::shared_ptr<Monster>  findMonster(const std::shared_ptr<const Creature>& searcher, const std::string& name, int num, bool firstAggro, bool exactMatch, int& match) const;
    std::shared_ptr<Monster>  findNpcTrader(const std::shared_ptr<const Creature>& searcher, const short profession) const;
    std::shared_ptr<Player> findPlayer(const std::shared_ptr<const Creature>& searcher, cmd* cmnd, int num= 1) const;
    std::shared_ptr<Player> findPlayer(const std::shared_ptr<const Creature>& searcher, const std::string& name, int num, bool exactMatch = false) const;
    std::shared_ptr<Player> findPlayer(const std::shared_ptr<const Creature>& searcher, const std::string& name, int num, bool exactMatch, int& match) const;

    std::shared_ptr<Object>  findObject(const std::shared_ptr<const Creature>& searcher, const cmd* cmnd, int val) const;
    std::shared_ptr<Object>  findObject(const std::shared_ptr<const Creature>& searcher, const std::string& name, int num, bool exactMatch = false) const;
    std::shared_ptr<Object>  findObject(const std::shared_ptr<const Creature>& searcher, const std::string& name, int num, bool exactMatch, int& match) const;

    std::shared_ptr<MudObject> findTarget(const std::shared_ptr<const Creature>& searcher,  cmd* cmnd, int num=1) const;
    std::shared_ptr<MudObject> findTarget(const std::shared_ptr<const Creature>& searcher,  const std::string& name, int num, bool monFirst= true, bool firstAggro = false, bool exactMatch = false) const;
    std::shared_ptr<MudObject> findTarget(const std::shared_ptr<const Creature>& searcher,  const std::string& name, int num, bool monFirst, bool firstAggro, bool exactMatch, int& match) const;

};

class Containable : public virtual MudObject,  public inheritable_enable_shared_from_this<Containable> {
public:
    Containable();
    ~Containable() override = default;

    bool addTo(const std::shared_ptr<Container>& container);
    std::shared_ptr<Container> removeFrom();

    void setParent(const std::shared_ptr<Container>& container);

    std::shared_ptr<Container> getParent() const;
    std::string getParentId();


    // What type of parent are we contained in?
    [[nodiscard]] bool inRoom() const;
    [[nodiscard]] bool inUniqueRoom() const;
    [[nodiscard]] bool inAreaRoom() const;
    [[nodiscard]] bool inObject() const;
    [[nodiscard]] bool inPlayer() const;
    [[nodiscard]] bool inMonster() const;
    [[nodiscard]] bool inCreature() const;

    std::shared_ptr<BaseRoom> getRoomParent();
    std::shared_ptr<UniqueRoom> getUniqueRoomParent();
    std::shared_ptr<AreaRoom> getAreaRoomParent();
    std::shared_ptr<Object>  getObjectParent();
    std::shared_ptr<Player> getPlayerParent();
    std::shared_ptr<Monster>  getMonsterParent();
    std::shared_ptr<Creature> getCreatureParent();

    [[nodiscard]] std::shared_ptr<const BaseRoom> getConstRoomParent() const;
    [[nodiscard]] std::shared_ptr<const UniqueRoom> getConstUniqueRoomParent() const;
    [[nodiscard]] std::shared_ptr<const AreaRoom> getConstAreaRoomParent() const;
    [[nodiscard]] std::shared_ptr<const Object>  getConstObjectParent() const;
    [[nodiscard]] std::shared_ptr<const Player> getConstPlayerParent() const;
    [[nodiscard]] std::shared_ptr<const Monster>  getConstMonsterParent() const;
    [[nodiscard]] std::shared_ptr<const Creature> getConstCreatureParent() const;

protected:
    std::weak_ptr<Container> parent;   // Parent Container

    // Last parent is only used in removeFromSet and addToSet and should not be used anywhere else
    std::shared_ptr<Container> lastParent;
    void removeFromSet() override;
    void addToSet() override;

};

#endif /* CONTAINER_H_ */
