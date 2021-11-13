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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
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


typedef struct _xmlNode xmlNode;
typedef xmlNode *xmlNodePtr;


class MudObject;
class EffectBuilder;

class Effect {
public:
    friend class EffectBuilder; // The builder can access the internals

    [[nodiscard]] const std::string & getPulseScript() const;
    [[nodiscard]] const std::string & getUnApplyScript() const;
    [[nodiscard]] const std::string & getApplyScript() const;
    [[nodiscard]] const std::string & getPreApplyScript() const;
    [[nodiscard]] const std::string & getPostApplyScript() const;
    [[nodiscard]] const std::string & getComputeScript() const;
    [[nodiscard]] const std::string & getType() const;
    [[nodiscard]] const std::string & getRoomDelStr() const;
    [[nodiscard]] const std::string & getRoomAddStr() const;
    [[nodiscard]] const std::string & getSelfDelStr() const;
    [[nodiscard]] const std::string & getSelfAddStr() const;
    [[nodiscard]] const std::string & getOppositeEffect() const;
    [[nodiscard]] const std::string & getDisplay() const;
    [[nodiscard]] const std::string & getName() const;
    [[nodiscard]] bool hasBaseEffect(std::string_view effect) const;
    [[nodiscard]] int getPulseDelay() const;
    [[nodiscard]] bool isPulsed() const;
    [[nodiscard]] bool isSpell() const;
    [[nodiscard]] bool usesStrength() const;

    // Base effect(s) - for multiple effects that confer the same type of effect (fly, etc)
    const std::list<std::string> &getBaseEffects();
    static bool objectCanBestowEffect(std::string_view effect);

    Effect(const Effect&) = delete;  // No Copies
    Effect(Effect&&) = default;      // Only Moves
private:
    Effect() = default;
    std::string name;
    std::list<std::string> baseEffects;  // For multiple effects that confer the same type of effect, ie: Fly

    std::string display;

    std::string oppositeEffect;

    std::string selfAddStr;
    std::string selfDelStr;
    std::string roomAddStr;
    std::string roomDelStr;
    bool pulsed{};      // Does this effect need to be pulsed?
    int pulseDelay{};  // Time between pulses

    std::string type;

    std::string computeScript;
    std::string applyScript;
    std::string unApplyScript;
    std::string preApplyScript;
    std::string postApplyScript;
    std::string pulseScript;

    bool isSpellEffect{};  // Decides if the effect will show up under "spells under"
    bool useStrength{};

    int baseDuration{};       // Base duration of the effect
    float potionMultiplyer{}; // Multiplier of duration for potion
    int magicRoomBonus{};     // Bonus in +magic room

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
//     std::string name;       // Name of the effect
//     int strength;   // Strength of the effect
//     int duration;   // How long this effect will last
// };

// Information about an effect on a creature
class EffectInfo {

    friend class Effects;

public:
    EffectInfo(const std::string &pName, time_t pLastMod, long pDuration, int pStrength, MudObject *pParent = 0, const Creature *owner = 0);
    EffectInfo(xmlNodePtr rootNode);

    virtual ~EffectInfo();

    friend std::ostream &operator<<(std::ostream &out, const EffectInfo &effectinfo);
    friend std::ostream &operator<<(std::ostream &out, EffectInfo *effectinfo);

    bool compute(MudObject *applier);
    bool add();
    bool apply();
    bool preApply();
    bool postApply(bool keepApplier);
    bool remove(bool show = true);
    bool willOverWrite(EffectInfo *existingEffect) const;
    void save(xmlNodePtr rootNode) const;


    [[nodiscard]] const std::string & getName() const;
    [[nodiscard]] std::string getDisplayName() const;
    [[nodiscard]] const std::string & getOwner() const;
    [[nodiscard]] time_t getLastMod() const;
    [[nodiscard]] long getDuration() const;
    [[nodiscard]] int getStrength() const;
    [[nodiscard]] int getExtra() const;
    [[nodiscard]] MudObject *getParent() const;
    [[nodiscard]] const Effect *getEffect() const;
    [[nodiscard]] bool hasBaseEffect(std::string_view effect) const;
    bool runScript(const std::string& pyScript, MudObject *applier = nullptr);
    bool updateLastMod(time_t t);    // True if it's time to wear off
    bool timeForPulse(time_t t);     // True if it's time to pulse
    bool pulse(time_t t);

    void setOwner(const Creature *owner);
    void setStrength(int pStrength);
    void setExtra(int pExtra);
    void setDuration(long pDuration);
    void setParent(MudObject *parent);

    bool isOwner(const Creature *owner) const;
    [[nodiscard]] bool isCurse() const;
    [[nodiscard]] bool isDisease() const;
    [[nodiscard]] bool isPoison() const;
    [[nodiscard]] bool isPermanent() const;

    [[nodiscard]] MudObject *getApplier() const;

protected:
    EffectInfo();

private:
    std::string name;           // Which effect is this
    std::string pOwner;         // Who cast this effect (player)
    time_t lastMod = 0;         // When did we last update duration
    time_t lastPulse = 0;       // Last Pulsed time
    int pulseModifier = 0;      // Adjustment to base pulse timer
    long duration = 0;          // How much longer will this effect last
    int strength = 0;           // How strong is this effect (for overwriting effects)
    int extra = 0;              // Extra info
    const Effect *myEffect = nullptr; // Pointer to the effect listing

    MudObject *myParent = nullptr;    // Pointer to parent MudObject

    // MudObject applying this effect; only valid during application unless otherwise
    // specified. And if you DO specify otherwise, be sure you know what you're doing!
    // The effect will keep a pointer to the applier, and if the applier is removed
    // from memory, you'll get an invalid pointer that might crash the game.
    MudObject *myApplier = nullptr;

};

typedef std::list<EffectInfo *> EffectList;

// this class holds effect information and makes effects portable
// across multiple objects
class Effects {
public:
    void load(xmlNodePtr rootNode, MudObject *pParent = nullptr);
    void save(xmlNodePtr rootNode, const char *name) const;

    [[nodiscard]] EffectInfo *getEffect(std::string_view effect) const;
    [[nodiscard]] EffectInfo *getExactEffect(std::string_view effect) const;

    [[nodiscard]] bool isEffected(const std::string &effect, bool exactMatch = false) const;
    [[nodiscard]] bool isEffected(EffectInfo *effect) const;

    //EffectInfo* addEffect(std::string_view effect, MudObject* applier, bool show, MudObject* pParent=0, const Creature* onwer=0, bool keepApplier=false);
    EffectInfo *addEffect(EffectInfo *newEffect, bool show, MudObject *parent = nullptr, bool keepApplier = false);
    EffectInfo *addEffect(const std::string& effect, long duration, int strength, MudObject *applier = nullptr, bool show = true, MudObject *pParent = nullptr,
                          const Creature *onwer = nullptr, bool keepApplier = false);
    void copy(const Effects *source, MudObject *pParent = nullptr);

    bool removeEffect(const std::string& effect, bool show, bool remPerm, MudObject *fromApplier = nullptr);
    bool removeEffect(EffectInfo *toDel, bool show);
    bool removeOppositeEffect(const EffectInfo *effect);
    void removeOwner(const Creature *owner);
    void removeAll();
    bool removePoison();
    bool removeDisease();
    bool removeCurse();

    [[nodiscard]] bool hasPoison() const;
    [[nodiscard]] bool hasDisease() const;
    [[nodiscard]] std::string getEffectsString(const Creature *viewer);
    [[nodiscard]] std::string getEffectsList() const;

    void pulse(time_t t, MudObject *pParent = nullptr);

    EffectList effectList;
};

#endif /*EFFECTS_H_*/
