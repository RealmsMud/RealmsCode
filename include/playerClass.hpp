/*
 * playerClass.h
 *   Player Class
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

#pragma once

#include <list>
#include <map>
#include <libxml/parser.h>  // for xmlNodePtr

#include "dice.hpp"

class LevelGain;
class Player;
class PlayerTitle;
class SkillGain;

// Information about a player class
class PlayerClass {
public:
    PlayerClass(xmlNodePtr rootNode);
    ~PlayerClass();
    int getId() const;
    std::string getName() const;
    short getBaseHp();
    short getBaseMp();
    bool needsDeity();
    short numProfs();
    std::list<SkillGain*>::const_iterator getSkillBegin();
    std::list<SkillGain*>::const_iterator getSkillEnd();
    std::map<int, LevelGain*>::const_iterator getLevelBegin();
    std::map<int, LevelGain*>::const_iterator getLevelEnd();
    LevelGain* getLevelGain(int lvl);
    std::string getTitle(int lvl, bool male, bool ignoreCustom=true) const;
    std::string getUnarmedWeaponSkill() const;

    std::map<int, PlayerTitle*> titles;
    bool hasDefaultStats();
    bool setDefaultStats(std::shared_ptr<Player> player);

    Dice damage;
protected:
    void load(xmlNodePtr rootNode);

    // Base Stats for the class
    std::string name;
    int id{};
    short baseHp{};
    short baseMp{};
    bool hasAutomaticStats;
    void checkAutomaticStats();
    short baseStrength;
    short baseDexterity;
    short baseConstitution;
    short baseIntelligence;
    short basePiety;

    bool needDeity;
    short numProf;
    // Skills they start with
    std::list<SkillGain*> baseSkills;
    // Stuff gained on each additional level
    std::map<int, LevelGain*> levels;
    std::string unarmedWeaponSkill;
};
