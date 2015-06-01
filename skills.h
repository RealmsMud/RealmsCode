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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef SKILLS_H_
#define SKILLS_H_

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
    virtual void setName(bstring pName);
protected:
    bool readNode(xmlNodePtr rootNode);

    bstring baseSkill;
    bstring group;                  // Group the skill belongs to
    bstring displayName;            // Display name
    int gainType;                   // Adjustments for skills with long timers
    bool knownOnly;

public:
    bstring getGroup() const;
    bstring getBaseSkill() const;
    bstring getDisplayName() const;
    int getGainType() const;
    bool isKnownOnly() const;
    bool hasBaseSkill() const;



    bool setGroup(bstring &pGroup);
    bool setBase(bstring &pBase);
};

//**********************************************************************
// SkillCommand - Subclass of SkillInfo - A skill that is a command
//**********************************************************************

class SkillCommand : public virtual SkillInfo, public virtual Command {
    friend class Config;
public:
    SkillCommand(xmlNodePtr rootNode);
    int execute(Creature* player, cmd* cmnd);

    void setName(bstring pName);
protected:
    bool readNode(xmlNodePtr rootNode);
    void loadResources(xmlNodePtr rootNode);

    TargetType targetType;          // What sort of target?
    bool offensive;                 // Is this an offensive skill? Default: Yes         // *

    bool usesAttackTimer;           // Delay/cooldown is also affected by the attack timer (True by default)
    int cooldown;                   // Delay/cooldown on this skill * 10.  (10 = 1.0s delay)
    int failCooldown;               // Delay/cooldown on this skill on failure
    std::list<SkillCost> resources; // Resources this skill uses
    bstring pyScript;                 // Python script for this skillCommand

    std::list<bstring> aliases;

private:
    int (*fn)(Creature* player, cmd* cmnd);

public:
    bool checkResources(Creature* creature);
    void subResources(Creature* creature);

    TargetType getTargetType() const;
    bool isOffensive() const;
    bool runScript(Creature* actor, MudObject* target, Skill* skill);

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
    Skill(const bstring& pName, int pGained);
    void reset();
protected:
    bstring name;
    int gained;             // How many points they have gained so far
    int gainBonus;          // Used for hard to gain skills, giving them an increased chance to improve
#ifndef PYTHON_CODE_GEN
    Timer timer;            // Timer for cooldown
#endif
    SkillInfo* skillInfo;   // Pointer to parent skill for additional info
public:
    void save(xmlNodePtr rootNode) const;

    bool hasBaseSkill() const;
    bstring getBaseSkill();


    bstring getName() const;
    bstring getDisplayName() const;
    bstring getGroup() const;
    int getGainType() const;
    int getGained() const;
    int getGainBonus() const;

    void setName(bstring pName);
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
