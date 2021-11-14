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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _ALCHEMY_H
#define _ALCHEMY_H

#include <libxml/parser.h>  // for xmlNodePtr
#include <iosfwd>           // for ostream
#include <map>              // for map, map<>::value_compare
#include <string>           // for string, operator<=>, basic_string
#include <string_view>      // for string_view
#include <vector>           // for vector

class Creature;
class Object;
class AlchemyBuilder;

typedef std::vector<Object*> HerbVector;
typedef std::map<std::string, HerbVector > HerbMap;

namespace Alchemy {
    std::string getEffectString(Object* obj, std::string_view effect);
    int numEffectsVisisble(int skillLevel);
};

//########################################################################
//# AlchemyInfo
//########################################################################

class AlchemyInfo {
public:
    friend class AlchemyBuilder; // The builder can access the internals

    [[nodiscard]] std::string getDisplayString() const;

    [[nodiscard]] const std::string & getName() const;
    [[nodiscard]] const std::string & getPotionPrefix() const;
    [[nodiscard]] const std::string & getPotionDisplayName() const;
    [[nodiscard]] const std::string & getAction() const;
    [[nodiscard]] const std::string & getPythonScript() const;
    [[nodiscard]] long getBaseDuration() const;
    [[nodiscard]] short getBaseStrength() const;
    [[nodiscard]] bool potionNameHasPrefix() const;
    [[nodiscard]] bool isThrowable() const;
    [[nodiscard]] bool isPositive() const;


    AlchemyInfo(const AlchemyInfo&) = delete;  // No Copies
    AlchemyInfo(AlchemyInfo&&) = default;      // Only Moves
protected:
    AlchemyInfo() = default;

    std::string name;
    std::string potionDisplayName;
    std::string potionPrefix;
    bool positive = false;

    // Standard duration and strength for this effect - will be modified by alchemy skill,
    // equipment quality, and herb quality
    long baseDuration = 10;
    short baseStrength = 1;

    std::string action;       // effect, python
    bool throwable = false;   // Can this be thrown at a door/creature/etc
    std::string pythonScript; // if action == python, or if throwable, needs to handle both
};

//########################################################################
//# AlchemyEffect
//########################################################################

class AlchemyEffect {
protected:
    std::string     effect;
    short           quality;

    long            duration;
    short           strength;

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

    [[nodiscard]] const std::string & getEffect() const;
    [[nodiscard]] long getDuration() const;
    [[nodiscard]] short getStrength() const;
    [[nodiscard]] short getQuality() const;

    void setDuration(long newDuration);

};


#endif  /* _ALCHEMY_H */


