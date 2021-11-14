/*
 * creatureSkills.cpp
 *   Creature Skill routines.
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

#include "config.hpp"                       // for Config, gConfig
#include "mudObjects/creatures.hpp"         // for Creature, PetList
#include "skillGain.hpp"                    // for SkillGain

//*********************************************************************
//                      checkSkillsGain
//*********************************************************************
// setToLevel: Set skill level to player level - 1, otherwise set to whatever the skill gain tells us to

void Creature::checkSkillsGain(const std::list<SkillGain*>::const_iterator& begin, const std::list<SkillGain*>::const_iterator& end, bool setToLevel) {
    SkillGain *sGain=nullptr;
    std::list<SkillGain*>::const_iterator sgIt;
    for(sgIt = begin ; sgIt != end ; sgIt++) {
        sGain = (*sgIt);
        if(sGain->getName().empty())
            continue;
        if(!sGain->hasDeities() || sGain->deityIsAllowed(deity)) {
            if(!knowsSkill(sGain->getName())) {
                if(setToLevel)
                    addSkill(sGain->getName(), (level-1)*10);
                else
                    addSkill(sGain->getName(), sGain->getGained());
                print("You have learned the fine art of '%s'.\n", gConfig->getSkillDisplayName(sGain->getName()).c_str());
            }
        }
    }
}

