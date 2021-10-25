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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef SKILLS_H_
#define SKILLS_H_

#include <list>

#include "global.hpp"
#include "structs.hpp"
#include "timer.hpp"

class Skill;

typedef enum {
    SKILL_EASY,
    SKILL_NORMAL,
    SKILL_MEDIUM,
    SKILL_HARD
} SkillGainLevel;

typedef enum {
    RES_NONE,
    RES_GOLD,
    RES_MANA,
    RES_HIT_POINTS,
    RES_FOCUS,
    RES_ENERGY,
    RES_MAX
} ResourceType;


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

class SkillCost {
public:
    SkillCost(xmlNodePtr rootNode);
    ResourceType resource;  // What type of resource
    int cost;               // How much of the resource

};

// Generic information for a skill
class SkillInfo : public virtual Nameable {
    friend class Skill;
public:
    SkillInfo();
    SkillInfo(xmlNodePtr rootNode);
    virtual ~SkillInfo() {};
    virtual void setName(std::string pName);
protected:
    bool readNode(xmlNodePtr rootNode);

    std::string baseSkill;
    std::string group;                  // Group the skill belongs to
    std::string displayName;            // Display name
    int gainType;                   // Adjustments for skills with long timers
    bool knownOnly;

public:
    std::string getGroup() const;
    std::string getBaseSkill() const;
    std::string getDisplayName() const;
    int getGainType() const;
    bool isKnownOnly() const;
    bool hasBaseSkill() const;



    bool setGroup(std::string &pGroup);
    bool setBase(std::string &pBase);
};

//**********************************************************************
// SkillCommand - Subclass of SkillInfo - A skill that is a command
//**********************************************************************

class SkillCommand : public virtual SkillInfo, public virtual Command {
    friend class Config;
public:
    explicit SkillCommand(xmlNodePtr rootNode);
    SkillCommand(std::string_view pCmdStr) {
        name = pCmdStr;
    }
    int execute(Creature* player, cmd* cmnd) const;

    void setName(std::string pName);
protected:
    bool readNode(xmlNodePtr rootNode);
    void loadResources(xmlNodePtr rootNode);

    TargetType targetType;          // What sort of target?
    bool offensive{};                 // Is this an offensive skill? Default: Yes         // *

    bool usesAttackTimer{};           // Delay/cooldown is also affected by the attack timer (True by default)
    int cooldown{};                   // Delay/cooldown on this skill * 10.  (10 = 1.0s delay)
    int failCooldown{};               // Delay/cooldown on this skill on failure
    std::list<SkillCost> resources; // Resources this skill uses
    std::string pyScript;                 // Python script for this skillCommand

    std::list<std::string> aliases;

private:
    int (*fn)(Creature* player, cmd* cmnd){};

public:
    bool checkResources(Creature* creature) const;
    void subResources(Creature* creature);

    TargetType getTargetType() const;
    bool isOffensive() const;
    bool runScript(Creature* actor, MudObject* target, Skill* skill) const;

    bool getUsesAttackTimer() const;
    bool hasCooldown() const;
    int getCooldown() const;
    int getFailCooldown() const;

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
    int gained{};             // How many points they have gained so far
    int gainBonus{};          // Used for hard to gain skills, giving them an increased chance to improve
    Timer timer;            // Timer for cooldown
    SkillInfo* skillInfo{};   // Pointer to parent skill for additional info
public:
    void save(xmlNodePtr rootNode) const;

    [[nodiscard]] bool hasBaseSkill() const;
    [[nodiscard]] std::string getBaseSkill();


    [[nodiscard]] std::string getName() const;
    [[nodiscard]] std::string getDisplayName() const;
    [[nodiscard]] std::string getGroup() const;
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

    SkillInfo* getSkillInfo();

};


#endif /*SKILLS_H_*/
