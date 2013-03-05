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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _ALCHEMY_H
#define	_ALCHEMY_H

//class XmlNodePtr;

typedef std::vector<Object*> HerbVector;
typedef std::map<bstring, HerbVector > HerbMap;

namespace Alchemy {
	bstring getEffectString(Object* obj, const bstring& effect);
};

class AlchemyInfo {
public:
    AlchemyInfo(xmlNodePtr rootNode);
    bstring getDisplayString();
    
    bstring name;
    bool positive;

    // Standard duration and strength for this effect - will be modified by alchemy skill,
    // equipment quality, and herb quality
    long baseDuration;
    short baseStrength;

    bstring action; // effect, python
    bool throwable; // Can this be thrown at a door/creature/etc
    bstring pythonScript; // if action == python, or if throwable, needs to handle both
};

class AlchemyEffect {
private:
    bstring     effect;
    short       quality;

    long	duration;
    short	strength;

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


#endif	/* _ALCHEMY_H */


