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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _ALCHEMY_H
#define _ALCHEMY_H

#include <libxml/parser.h>  // for xmlNodePtr
#include <iosfwd>           // for ostream
#include <map>              // for operator==, operator!=
#include <vector>           // for allocator

#include "bstring.hpp"      // for bstring

class bstring;
class Creature;
class Object;

typedef std::vector<Object*> HerbVector;
typedef std::map<bstring, HerbVector > HerbMap;

namespace Alchemy {
    bstring getEffectString(Object* obj, const bstring& effect);
    int numEffectsVisisble(int skillLevel);
};

//########################################################################
//# AlchemyInfo
//########################################################################

class AlchemyInfo {
public:
    explicit AlchemyInfo(xmlNodePtr rootNode);

    bstring getDisplayString();

    [[nodiscard]] const bstring& getName() const;
    [[nodiscard]] const bstring& getPotionPrefix() const;
    [[nodiscard]] const bstring& getPotionDisplayName() const;
    [[nodiscard]] const bstring& getAction() const;
    [[nodiscard]] const bstring& getPythonScript() const;
    [[nodiscard]] long getBaseDuration() const;
    [[nodiscard]] short getBaseStrength() const;
    [[nodiscard]] bool potionNameHasPrefix() const;
    [[nodiscard]] bool isThrowable() const;
    [[nodiscard]] bool isPositive() const;


protected:
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
    explicit AlchemyEffect(xmlNodePtr curNode);
    AlchemyEffect(const AlchemyEffect &ae);
    friend std::ostream& operator<<(std::ostream& out, AlchemyEffect& effect);
    friend std::ostream& operator<<(std::ostream& out, AlchemyEffect* effect);

    int saveToXml(xmlNodePtr rootNode);

    // Combine with another effect and caculate the average quality of the two
    void combineWith(const AlchemyEffect &ae);

    // Apply this effect to the creature:
    bool apply(Creature* target);

    [[nodiscard]] const bstring& getEffect() const;
    [[nodiscard]] long getDuration() const;
    [[nodiscard]] short getStrength() const;
    [[nodiscard]] short getQuality() const;

    void setDuration(long newDuration);

};


#endif  /* _ALCHEMY_H */


