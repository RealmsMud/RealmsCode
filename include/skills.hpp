/*
 * skills.h
 *   Header file for skills
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

#include "global.hpp"
#include "structs.hpp"
#include "timer.hpp"

class Skill;
class SkillInfoBuilder;

enum SkillGainType {
    EASY,
    NORMAL,
    MEDIUM,
    HARD
};


// This should be a class, not an enum
typedef enum {
    RES_NONE,
    RES_GOLD,
    RES_MANA,
    RES_HIT_POINTS,
    RES_FOCUS,
    RES_ENERGY,
    RES_MAX
} ResourceType;


// This should also probably be a class and not an enum, and it should be shared with songs
enum TargetType {
    TARGET_NONE,
    TARGET_CREATURE,
    TARGET_MONSTER,
    TARGET_PLAYER,
    TARGET_OBJECT,
    TARGET_OBJECT_CREATURE,
    TARGET_EXIT,
    TARGET_MUDOBJECT
};

std::string getSkillLevelStr(int gained);

//**********************************************************************
// SkillInfo - Class to store base information about skills
//**********************************************************************


// Generic information for a skill
class SkillInfo : public virtual Nameable {
    friend class Skill;
    friend class SkillInfoBuilder;
public:
    SkillInfo();
    SkillInfo(const SkillInfo&) = delete; // No Copies
    SkillInfo(SkillInfo&&) = default;     // Only Moves

    virtual ~SkillInfo() {};
protected:

    std::string baseSkill;
    std::string group;                  // Group the skill belongs to
    std::string displayName;            // Display name
    SkillGainType gainType;             // Adjustments for skills with long timers
    bool knownOnly;

public:
    [[nodiscard]] const std::string & getGroup() const;
    [[nodiscard]] const std::string & getBaseSkill() const;
    [[nodiscard]] const std::string & getDisplayName() const;
    [[nodiscard]] SkillGainType getGainType() const;
    [[nodiscard]] bool isKnownOnly() const;
    [[nodiscard]] bool hasBaseSkill() const;

};


//**********************************************************************
// Skill - Keeps track of skill information for a Creature
//**********************************************************************



class Skill {
public:
    Skill();
    Skill(xmlNodePtr rootNode);
    Skill(std::string_view pName, int pGained);
    void reset();
protected:
    std::string name;
    int gained{};                   // How many points they have gained so far
    int gainBonus{};                // Used for hard to gain skills, giving them an increased chance to improve
    Timer timer;                    // Timer for cooldown
    const SkillInfo* skillInfo{};   // Pointer to parent skill for additional info
public:
    void save(xmlNodePtr rootNode) const;

    [[nodiscard]] bool hasBaseSkill() const;
    [[nodiscard]] const std::string & getBaseSkill();


    [[nodiscard]] const std::string & getName() const;
    [[nodiscard]] const std::string & getDisplayName() const;
    [[nodiscard]] const std::string & getGroup() const;
    [[nodiscard]] int getGainType() const;
    [[nodiscard]] int getGained() const;
    [[nodiscard]] int getGainBonus() const;

    void setName(std::string_view pName);
    void updateParent();
    void setGained(int pGained);

    void upBonus(int amt=1);    // Increase the gainBonus
    void clearBonus();          // Clear the bonus (after an increase)
    void improve(int amt=1);    // Improve the skill

    bool checkTimer(Creature* creature, bool displayFail);
    void updateTimer(bool setDelay = false, int delay = 0);
    void modifyDelay(int amt);
    void setDelay(int newDelay);

    const SkillInfo * getSkillInfo();

};
