/*
 * skills.h
 *	 Header file for skills
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
#ifndef SKILLS_H_
#define SKILLS_H_

typedef enum {
    SKILL_EASY,
    SKILL_NORMAL,
    SKILL_MEDIUM,
    SKILL_HARD
} SkillGainLevel;

//**********************************************************************
// Skill - Class to store base information about skills
//**********************************************************************

class Skill {
public:
	Skill();
protected:
	bstring name;			// Name of the skill
	bstring group;			// Group the skill belongs to
	bstring displayName;	// Display name
	bstring description;	// Description
	int gainType;				// Adjustments for skills with long timers
	bool knownOnly;
public:
	bstring getName() const;
	bstring getGroup() const;
	bstring getDisplayName() const;
	bstring getDescription() const;
	int getGainType() const;
	bool isKnownOnly() const;
	bool load(xmlNodePtr rootNode);
	bool setGroup(bstring &pGroup);
};

//**********************************************************************
// CrtSkills - Keeps track of skill information for a Creature
//**********************************************************************

class CrtSkill {
public:
	CrtSkill();
	CrtSkill(const bstring& pName, int pGained);
protected:
	bstring name;
	int gained;			// How many points they have gained so far
	int gainBonus; 		// Used for hard to gain skills, giving them an increased chance to improve
public:
	void save(xmlNodePtr rootNode) const;
	bool load(xmlNodePtr rootNode);

	bstring getName() const;
	bstring getDisplayName() const;
	bstring getGroup() const;
	int getGainType() const;
	int getGained() const;
	int getGainBonus() const;
	void setName(bstring &pName);
	void setGained(int pGained);

	void upBonus(int amt=1);	// Increase the gainBonus
	void clearBonus();			// Clear the bonus (after an increase)
	void improve(int amt=1);	// Improve the skill
};


#endif /*SKILLS_H_*/
