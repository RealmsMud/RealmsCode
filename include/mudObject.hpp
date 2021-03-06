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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef MUDOBJECTS_H
#define MUDOBJECTS_H

#include <list>
#include <map>
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



class MudObject {
private:
    bstring name;

public:
    void setName(const bstring& newName);
    const bstring& getName() const;
    const char* getCName() const;

protected:
    virtual void removeFromSet();
    virtual void addToSet();

public:
    bool isRegistered();
    void setRegistered();
    void setUnRegistered();

    bool registerMo();
    bool unRegisterMo();

    virtual void registerContainedItems();
    virtual void unRegisterContainedItems();

protected:
    bool registered;


public:
    //char name[80];
    bstring id;     // Unique identifier
    Hooks hooks;
    void moCopy(const MudObject& mo);


public:
    MudObject();
    virtual ~MudObject();
    void moReset();
    void moDestroy();

    void setId(const bstring& newId, bool handleParentSet = true);

    MudObject* getAsMudObject();
    Monster* getAsMonster();
    Player* getAsPlayer();
    Creature* getAsCreature();
    Object* getAsObject();
    UniqueRoom *getAsUniqueRoom();
    AreaRoom *getAsAreaRoom();
    BaseRoom* getAsRoom();
    Exit* getAsExit();

    const Monster* getAsConstMonster() const;
    const Player* getAsConstPlayer() const;
    const Creature* getAsConstCreature() const;
    const Object* getAsConstObject() const;
    const UniqueRoom *getAsConstUniqueRoom() const;
    const AreaRoom *getAsConstAreaRoom() const;
    const BaseRoom* getAsConstRoom() const;
    const Exit* getAsConstExit() const;

    bool isRoom() const;
    bool isUniqueRoom() const;
    bool isAreaRoom() const;
    bool isObject() const;
    bool isPlayer() const;
    bool isMonster() const;
    bool isCreature() const;
    bool isExit() const;

    const bstring& getId() const;
    bstring getIdPython() const;
    virtual void validateId() {};
    Effects effects;

    virtual bstring getFlagList(bstring sep=", ") const;



// Effects
    bool isEffected(const bstring& effect, bool exactMatch = false) const;
    bool isEffected(EffectInfo* effect) const;
    bool hasPermEffect(const bstring& effect) const;
    EffectInfo* getEffect(const bstring& effect) const;
    EffectInfo* getExactEffect(const bstring& effect) const;
    EffectInfo* addEffect(EffectInfo* newEffect, bool show = true, bool keepApplier=false);
    EffectInfo* addEffect(const bstring& effect, long duration = -2, int strength = -2, MudObject* applier = nullptr, bool show = true, const Creature* owner=nullptr, bool keepApplier=false);
    EffectInfo* addPermEffect(const bstring& effect, int strength = 1, bool show = true);
    bool removeEffect(const bstring& effect, bool show = true, bool remPerm = true, MudObject* fromApplier=nullptr);
    bool removeEffect(EffectInfo* toDel, bool show = true);
    bool removeOppositeEffect(const EffectInfo *effect);
    virtual bool pulseEffects(time_t t) = 0;

    bool equals(MudObject* other);

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


#endif
