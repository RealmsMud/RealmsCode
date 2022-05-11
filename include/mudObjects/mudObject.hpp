/*
 * mudobject.h
 *   The parent MudObject class
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

#include <list>
#include <map>
#include <memory>
#include <set>

#include "delayedAction.hpp"
#include "hooks.hpp"
#include "effects.hpp"

class AreaRoom;
class BaseRoom;
class Creature;
class EffectInfo;
class Exit;
class MudObject;
class Monster;
class Object;
class Player;
class UniqueRoom;


class MudObject: public std::enable_shared_from_this<MudObject> {
private:
    std::string name;

public:
    void setName(std::string_view newName);
    [[nodiscard]] const std::string & getName() const;
    [[nodiscard]] const char* getCName() const;

protected:
    virtual void removeFromSet();
    virtual void addToSet();

public:
    bool isRegistered();
    void setRegistered();
    void setUnRegistered();

    bool registerMo(const std::shared_ptr<MudObject>& mo);
    bool unRegisterMo();

    virtual void registerContainedItems();
    virtual void unRegisterContainedItems();

protected:
    bool registered;


public:
    //char name[80];
    std::string id;     // Unique identifier
    Hooks hooks;
    void moCopy(const MudObject& mo);


public:
    MudObject();
    virtual ~MudObject();
    void moReset();

    void setId(std::string_view newId, bool handleParentSet = true);

    std::shared_ptr<MudObject> getAsMudObject();
    std::shared_ptr<Monster>  getAsMonster();
    std::shared_ptr<Player> getAsPlayer();
    std::shared_ptr<Creature> getAsCreature();
    std::shared_ptr<Object>  getAsObject();
    std::shared_ptr<UniqueRoom> getAsUniqueRoom();
    std::shared_ptr<AreaRoom> getAsAreaRoom();
    std::shared_ptr<BaseRoom> getAsRoom();
    std::shared_ptr<Exit> getAsExit();

    [[nodiscard]] std::shared_ptr<const Monster>  getAsConstMonster() const;
    [[nodiscard]] std::shared_ptr<const Player> getAsConstPlayer() const;
    [[nodiscard]] std::shared_ptr<const Creature> getAsConstCreature() const;
    [[nodiscard]] std::shared_ptr<const Object>  getAsConstObject() const;
    [[nodiscard]] std::shared_ptr<const UniqueRoom> getAsConstUniqueRoom() const;
    [[nodiscard]] std::shared_ptr<const AreaRoom> getAsConstAreaRoom() const;
    [[nodiscard]] std::shared_ptr<const BaseRoom> getAsConstRoom() const;
    [[nodiscard]] std::shared_ptr<const Exit> getAsConstExit() const;

    [[nodiscard]] virtual bool isRoom() const;
    [[nodiscard]] virtual bool isUniqueRoom() const;
    [[nodiscard]] virtual bool isAreaRoom() const;
    [[nodiscard]] virtual bool isObject() const;
    [[nodiscard]] virtual bool isPlayer() const;
    [[nodiscard]] virtual bool isMonster() const;
    [[nodiscard]] virtual bool isCreature() const;
    [[nodiscard]] virtual bool isExit() const;

    [[nodiscard]] const std::string & getId() const;

    virtual void validateId() {};
    Effects effects;

    virtual std::string getFlagList(std::string_view sep=", ") const;



// Effects
    [[nodiscard]] bool isEffected(const std::string &effect, bool exactMatch = false) const;
    [[nodiscard]] bool isEffected(EffectInfo* effect) const;
    [[nodiscard]] bool hasPermEffect(std::string_view effect) const;
    [[nodiscard]] EffectInfo* getEffect(std::string_view effect) const;
    [[nodiscard]] EffectInfo* getExactEffect(std::string_view effect) const;
    EffectInfo* addEffect(EffectInfo* newEffect, bool show = true, bool keepApplier=false);
    EffectInfo* addEffect(const std::string&effect, long duration = -2, int strength = -2, const std::shared_ptr<MudObject>& applier = nullptr, bool show = true, const std::shared_ptr<Creature> & owner=nullptr, bool keepApplier=false);
    EffectInfo* addPermEffect(const std::string& effect, int strength = 1, bool show = true);
    bool removeEffect(const std::string& effect, bool show = true, bool remPerm = true, const std::shared_ptr<MudObject>& fromApplier=nullptr);
    bool removeEffect(EffectInfo* toDel, bool show = true);
    bool removeOppositeEffect(const EffectInfo *effect);
    virtual bool pulseEffects(time_t t) = 0;

    bool equals(const std::shared_ptr<MudObject>& other);

    void readCreatures(xmlNodePtr curNode, bool offline=false);
    void readObjects(xmlNodePtr curNode, bool offline=false);

// Delayed Actions
protected:
    std::list<DelayedAction*> delayedActionQueue;
public:
    void interruptDelayedActions();
    void removeDelayedAction(DelayedAction* action);
    void addDelayedAction(DelayedAction* action);
    void clearDelayedActions();
};


template <class T>
class inheritable_enable_shared_from_this : virtual public MudObject {
public:
    std::shared_ptr<T> shared_from_this() {
        return std::dynamic_pointer_cast<T>(MudObject::shared_from_this());
    }

    std::shared_ptr<const T> shared_from_this() const {
        return std::dynamic_pointer_cast<const T>(MudObject::shared_from_this());
    }

    /* Utility method to easily downcast.
     * Useful when a child doesn't inherit directly from enable_shared_from_this
     * but wants to use the feature.
     */
    template <class Down>
    std::shared_ptr<Down> downcasted_shared_from_this() {
        return std::dynamic_pointer_cast<Down>(MudObject::shared_from_this());
    }
    template <class Down>
    std::shared_ptr<const Down> downcasted_shared_from_this() const {
        return std::dynamic_pointer_cast<const Down>(MudObject::shared_from_this());
    }
};