/*
 * alchemy.cpp
 *   Alchmey classes, functions, and other handlers
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _ALCHEMY_H
#define _ALCHEMY_H

#include <map>
#include <vector>

#include "common.h"

class Creature;
class Object;

typedef std::vector<Object*> HerbVector;
typedef std::map<bstring, HerbVector > HerbMap;

namespace Alchemy {
    bstring getEffectString(Object* obj, const bstring& effect);
    int numEffectsVisisble(const int skillLevel);
};

//########################################################################
//# AlchemyInfo
//########################################################################

class AlchemyInfo {
public:
    AlchemyInfo(xmlNodePtr rootNode);

    bstring getDisplayString();

    const bstring& getName() const;
    const bstring& getPotionPrefix() const;
    const bstring& getPotionDisplayName() const;
    const bstring& getAction() const;
    const bstring& getPythonScript() const;
    long getBaseDuration() const;
    short getBaseStrength() const;
    bool potionNameHasPrefix() const;
    bool isThrowable() const;
    bool isPositive() const;


protected:
    void init();
    bstring name;
    bstring potionDisplayName;
    bstring potionPrefix;
    bool positive = false;

    // Standard duration and strength for this effect - will be modified by alchemy skill,
    // equipment quality, and herb quality
    long baseDuration = 10;
    short baseStrength = 1;

    bstring action; // effect, python
    bool throwable = false; // Can this be thrown at a door/creature/etc
    bstring pythonScript; // if action == python, or if throwable, needs to handle both
};

//########################################################################
//# AlchemyEffect
//########################################################################

class AlchemyEffect {
protected:
    bstring     effect;
    short       quality;

    long    duration;
    short   strength;

public:
    AlchemyEffect();
    AlchemyEffect(xmlNodePtr curNode);
    AlchemyEffect(const AlchemyEffect &ae);
    
    int saveToXml(xmlNodePtr rootNode);

    // Combine with another effect and caculate the average quality of the two
    void combineWith(const AlchemyEffect &ae);

    // Apply this effect to the creature:
    bool apply(Creature* target);

    const bstring& getEffect() const;
    long getDuration() const;
    short getStrength() const;
    short getQuality() const;

    void setDuration(const long newDuration);

};


#endif  /* _ALCHEMY_H */


