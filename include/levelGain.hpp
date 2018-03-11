/*
 * levelGain.h
 *   Level Gain
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef _LEVELGAIN_H
#define _LEVELGAIN_H

#include <list>

#include "common.hpp"

class SkillGain;

// Class to describe what is gained at each level
class LevelGain {
public:
    LevelGain(xmlNodePtr rootNode);
    ~LevelGain();
    std::list<SkillGain*>::const_iterator getSkillBegin();
    std::list<SkillGain*>::const_iterator getSkillEnd();
    bstring getStatStr();
    bstring getSaveStr();
    int getStat();
    int getSave();
    int getHp();
    int getMp();
    bool hasSkills();
protected:
    void load(xmlNodePtr rootNode); // Read in a level gain from the given node;

    int stat; // The stat they get this level
    int hp; // HP & MP gain
    int mp;
    int save; // Saving throw to go up
    std::list<SkillGain*> skills; // Skills they gain at this level
public:


};


#endif  /* _LEVELGAIN_H */

