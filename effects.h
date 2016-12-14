/*
 * effects.h
 *   effects on creatures
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */


#ifndef EFFECTS_H_
#define EFFECTS_H_

#define EFFECT_MAX_DURATION 10800
#define EFFECT_MAX_STRENGTH 5000

#include <iostream>
#include <list>

#include "common.h"

class MudObject;

class Effect {
public:
    Effect(xmlNodePtr rootNode);

    bstring     getPulseScript() const;
    bstring     getUnApplyScript() const;
    bstring     getApplyScript() const;
    bstring     getPreApplyScript() const;
    bstring     getPostApplyScript() const;
    bstring     getComputeScript() const;
    bstring     getType() const;
    bstring     getRoomDelStr() const;
    bstring     getRoomAddStr() const;
    bstring     getSelfDelStr() const;
    bstring     getSelfAddStr() const;
    bstring     getOppositeEffect() const;
    bstring     getDisplay() const;
    bool        hasBaseEffect(const bstring& effect) const;
    bstring     getName() const;

    int         getPulseDelay() const;

    bool        isPulsed() const;
    bool        isSpell() const;
    bool        usesStrength() const;

    const std::list<bstring>& getBaseEffects();

    static bool objectCanBestowEffect(const bstring& effect);
private:
    Effect();

    bstring     name;
    std::list<bstring> baseEffects;  // For multiple effects that confer the same type of effect, ie: Fly

    bstring     display;

    bstring     oppositeEffect;

    bstring     selfAddStr;
    bstring     selfDelStr;
    bstring     roomAddStr;
    bstring     roomDelStr;
    bool        pulsed;      // Does this effect need to be pulsed?
    int         pulseDelay;  // Time between pulses

    bstring     type;

    bstring     computeScript;
    bstring     applyScript;
    bstring     unApplyScript;
    bstring     preApplyScript;
    bstring     postApplyScript;
    bstring     pulseScript;

    bool        isSpellEffect;  // Decides if the effect will show up under "spells under"
    bool        usesStr;

    int         baseDuration;       // Base duration of the effect
    float       potionMultiplyer;   // Multiplier of duration for potion
    int         magicRoomBonus;     // Bonus in +magic room

};


// Actions to be taken by the effect function
enum EffectType {
    NO_EFFECT_TYPE,

    GOOD_EFFECT,
    NEUTRAL_EFFECT,
    BAD_EFFECT,

    MAX_EFFECT_TYPE
};

// Forward Delcaration
class Creature;

// // Effects that are conferred from spells or items
// class ConferredEffect {
// private:
//     bstring name;       // Name of the effect
//     int strength;   // Strength of the effect
//     int duration;   // How long this effect will last
// };

// Information about an effect on a creature
class EffectInfo
{
friend std::ostream& operator<<(std::ostream& out, const EffectInfo& eff);
friend class Effects;
public:
    EffectInfo(bstring pName, time_t pLastMod, long pDuration, int pStrength, MudObject* pParent=0, const Creature* owner=0);
    EffectInfo(xmlNodePtr rootNode);
    virtual ~EffectInfo();

    bool        compute(MudObject* applier);
    bool        add();
    bool        apply();
    bool        preApply();
    bool        postApply(bool keepApplier);
    bool        remove(bool show = true);

    bool        willOverWrite(EffectInfo* existingEffect) const;

    void        save(xmlNodePtr rootNode) const;


    const       bstring getName() const;
    bstring     getDisplayName() const;
    const bstring getOwner() const;
    time_t      getLastMod() const;
    long        getDuration() const;
    int         getStrength() const;
    int         getExtra() const;
    MudObject*  getParent() const;
    Effect*     getEffect() const;
    bool        hasBaseEffect(const bstring& effect) const;
    bool        runScript(const bstring& pyScript, MudObject* applier = nullptr);

    bool        updateLastMod(time_t t);    // True if it's time to wear off
    bool        timeForPulse(time_t t);  // True if it's time to pulse

    bool        pulse(time_t t);

    void        setOwner(const Creature* owner);
    void        setStrength(int pStrength);
    void        setExtra(int pExtra);
    void        setDuration(long pDuration);
    void        setParent(MudObject* parent);

    bool        isOwner(const Creature* owner) const;
    bool        isCurse() const;
    bool        isDisease() const;
    bool        isPoison() const;
    bool        isPermanent() const;
    MudObject* getApplier() const;

protected:
    EffectInfo();

private:
    bstring     name;               // Which effect is this
    bstring     pOwner;             // Who cast this effect (player)
    time_t      lastMod = 0;        // When did we last update duration
    time_t      lastPulse  = 0;     // Last Pulsed time
    int         pulseModifier = 0;  // Adjustment to base pulse timer
    long        duration = 0;       // How much longer will this effect last
    int         strength = 0;       // How strong is this effect (for overwriting effects)
    int         extra = 0;          // Extra info
    Effect*     myEffect = 0;       // Pointer to the effect listing

    MudObject*  myParent = 0;       // Pointer to parent MudObject

    // MudObject applying this effect; only valid during application unless otherwise
    // specified. And if you DO specify otherwise, be sure you know what you're doing!
    // The effect will keep a pointer to the applier, and if the applier is removed
    // from memory, you'll get an invalid pointer that might crash the game.
    MudObject*  myApplier = 0;

};

typedef std::list<EffectInfo*> EffectList;
// this class holds effect information and makes effects portable
// across multiple objects
class Effects {
public:
    void    load(xmlNodePtr rootNode, MudObject* pParent=0);
    void    save(xmlNodePtr rootNode, const char* name) const;
    EffectInfo* getEffect(const bstring& effect) const;
    EffectInfo* getExactEffect(const bstring& effect) const;
    bool    isEffected(const bstring& effect, bool exactMatch = false) const;
    bool    isEffected(EffectInfo* effect) const;
    //EffectInfo* addEffect(const bstring& effect, MudObject* applier, bool show, MudObject* pParent=0, const Creature* onwer=0, bool keepApplier=false);
    EffectInfo* addEffect(EffectInfo* newEffect, bool show, MudObject* parent=0, bool keepApplier=false);
    EffectInfo* addEffect(const bstring& effect, long duration, int strength, MudObject* applier = nullptr, bool show = true, MudObject* pParent=0, const Creature* onwer=0, bool keepApplier=false);
    bool    removeEffect(const bstring& effect, bool show, bool remPerm, MudObject* fromApplier=0);
    bool    removeEffect(EffectInfo* toDel, bool show);
    bool    removeOppositeEffect(const EffectInfo *effect);
    void    removeAll();
    void    removeOwner(const Creature* owner);
    void    copy(const Effects* source, MudObject* pParent=0);
    bool    hasPoison() const;
    bool    removePoison();
    bool    hasDisease() const;
    bool    removeDisease();
    bool    removeCurse();
    bstring getEffectsString(const Creature* viewer);
    bstring getEffectsList() const;

    void    pulse(time_t t, MudObject* pParent=0);

    EffectList effectList;
};

#endif /*EFFECTS_H_*/
