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

    [[nodiscard]] int getId() const;
    [[nodiscard]] bstring getName(int tryToShorten=0) const;
    [[nodiscard]] bstring getAdjective() const;
    [[nodiscard]] bstring getAbbr() const;

    [[nodiscard]] int getParentRace() const;
    void makeParent();
    [[nodiscard]] bool isParent() const;
    [[nodiscard]] bool isPlayable() const;
    [[nodiscard]] bool isGendered() const;

    [[nodiscard]] bool hasInfravision() const;
    [[nodiscard]] bool bonusStat() const;
    [[nodiscard]] Size getSize() const;
    [[nodiscard]] int getStartAge() const;
    [[nodiscard]] int getStatAdj(int stat) const;
    [[nodiscard]] int getSave(int save) const;
    [[nodiscard]] short getPorphyriaResistance() const;

    [[nodiscard]] bool allowedClass(int cls) const;
    [[nodiscard]] bool allowedDeity(CreatureClass cls, CreatureClass cls2, int dty) const;
    [[nodiscard]] bool allowedMultiClericDeity(int dty) const;
    [[nodiscard]] bool allowedClericDeity(int dty) const;
    [[nodiscard]] bool allowedPaladinDeity(int dty) const;
    [[nodiscard]] bool allowedDeathknightDeity(int dty) const;

    [[nodiscard]] std::list<SkillGain*>::const_iterator getSkillBegin() const;
    [[nodiscard]] std::list<SkillGain*>::const_iterator getSkillEnd() const;

    std::map<Sex, short> baseHeight;
    std::map<Sex, short> baseWeight;
    std::map<Sex, Dice> varHeight;
    std::map<Sex, Dice> varWeight;

    std::list<bstring> effects;
};


#endif  /* _RACEDATA_H */

