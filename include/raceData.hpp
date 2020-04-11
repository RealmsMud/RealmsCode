/*
 * raceData.h
 *   Header file for race data storage
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

#ifndef _RACEDATA_H
#define _RACEDATA_H

#include <list>
#include <map>

#include "global.hpp"
#include "size.hpp"
#include "structs.hpp"

class SkillGain;

class RaceData {
protected:
    int id;
    bstring name;
    bstring adjective;
    bstring abbr;

    bool infra;
    bool bonus;
    Size size;
    int startAge;

    bool playable;
    bool isParentRace;
    bool gendered;
    int parentRace;

    int stats[MAX_STAT]{};
    int saves[MAX_SAVE]{};
    short porphyriaResistance;

    bool classes[CLASS_COUNT_MULT]{};
    bool multiClerDeities[DEITY_COUNT]{};
    bool clerDeities[DEITY_COUNT]{};
    bool palDeities[DEITY_COUNT]{};
    bool dkDeities[DEITY_COUNT]{};

    // Skills they start with
    std::list<SkillGain*> baseSkills;
public:
    RaceData(xmlNodePtr rootNode);
    ~RaceData();

    int getId() const;
    bstring getName(int tryToShorten=0) const;
    bstring getAdjective() const;
    bstring getAbbr() const;

    int getParentRace() const;
    void makeParent();
    bool isParent() const;
    bool isPlayable() const;
    bool isGendered() const;

    bool hasInfravision() const;
    bool bonusStat() const;
    Size getSize() const;
    int getStartAge() const;
    int getStatAdj(int stat) const;
    int getSave(int save) const;
    short getPorphyriaResistance() const;

    bool allowedClass(int cls) const;
    bool allowedDeity(CreatureClass cls, CreatureClass cls2, int dty) const;
    bool allowedMultiClericDeity(int dty) const;
    bool allowedClericDeity(int dty) const;
    bool allowedPaladinDeity(int dty) const;
    bool allowedDeathknightDeity(int dty) const;

    std::list<SkillGain*>::const_iterator getSkillBegin() const;
    std::list<SkillGain*>::const_iterator getSkillEnd() const;

    std::map<Sex, short> baseHeight;
    std::map<Sex, short> baseWeight;
    std::map<Sex, Dice> varHeight;
    std::map<Sex, Dice> varWeight;

    std::list<bstring> effects;
};


#endif  /* _RACEDATA_H */

