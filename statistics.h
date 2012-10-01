/*
 * statistics.h
 *	 Player statistics
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

#ifndef _STATISTICS_H
#define	_STATISTICS_H


class LevelInfo {
public:
	LevelInfo(const LevelInfo* l);
    LevelInfo(int pLevel, int pHp, int pMp, int pStat, int pSave, time_t pTime);
    LevelInfo(xmlNodePtr rootNode);
    void save(xmlNodePtr rootNode);
    int getLevel();
    int getHpGain();
    int getMpGain();
    int getStatUp();
    int getSaveGain();
    time_t getLevelTime();

private:
    int level;          // What level
    int hpGain;         // Hp gained this level
    int mpGain;         // Mp gained this level
    int statUp;         // Stat increased this level
    int saveGain;       // Save gained this level
    time_t levelTime;   // When they first gained this level
};

typedef std::map<int, LevelInfo*> LevelInfoMap;

class StringStatistic {
public:
	StringStatistic();
	void save(xmlNodePtr rootNode, bstring nodeName) const;
	void load(xmlNodePtr curNode);
	void update(unsigned long num, bstring with);
	void reset();

	unsigned long value;
	bstring name;
};

class Statistics {
public:
	Statistics();
	Statistics(Statistics& cr);
	Statistics(const Statistics& cr);
	Statistics& operator=(const Statistics& cr);
	~Statistics();
	void save(xmlNodePtr rootNode, bstring nodeName) const;
	void load(xmlNodePtr curNode);
	void display(const Player* viewer, bool death=false);
	void reset();
	bstring getTime();
	unsigned long pkDemographics() const;

	static unsigned long calcToughness(Creature* target);
	static bstring damageWith(const Player* player, const Object* weapon);
protected:
	void doCopy(const Statistics& cr);
private:
	bstring start;
	LevelInfoMap levelHistory; // New

	// combat
	unsigned long numSwings;
	unsigned long numHits;
	unsigned long numMisses;
	unsigned long numFumbles;
	unsigned long numDodges;
	unsigned long numCriticals;
	unsigned long numTimesHit;
	unsigned long numTimesMissed;
	unsigned long numTimesFled;
	unsigned long numPkIn;
	unsigned long numPkWon;
	// magic
	unsigned long numCasts;
	unsigned long numOffensiveCasts;
	unsigned long numHealingCasts;
	unsigned long numWandsUsed;
	unsigned long numTransmutes;
	unsigned long numPotionsDrank;
	// death
	unsigned long numKills;
	unsigned long numDeaths;
	unsigned long expLost; // New
	unsigned long lastExpLoss; // New

	// other
	unsigned long numThefts;
	unsigned long numAttemptedThefts;
	unsigned long numSaves;
	unsigned long numAttemptedSaves;
	unsigned long numRecalls;
	unsigned long numLagouts;
	unsigned long numFishCaught;
	unsigned long numItemsCrafted;
	unsigned long numCombosOpened;

	// most
	unsigned long mostGroup;
	StringStatistic mostExperience; // New
	StringStatistic mostMonster;
	StringStatistic mostAttackDamage;
	StringStatistic mostMagicDamage;
	// so we can reference
	Player* parent;
public:
	bool track;
	// combat
	void swing();
	void hit();
	void miss();
	void fumble();
	void dodge();
	void critical();
	void wasHit();
	void wasMissed();
	void flee();
	void winPk();
	void losePk();
	// magic
	void cast();
	void offensiveCast();
	void healingCast();
	void wand();
	void transmute();
	void potion();
	// death
	void kill();
	void die();
	void experienceLost(unsigned long amt);
	void setExperienceLost(unsigned long amt);

	// other
	void steal();
	void attemptSteal();
	void save();
	void attemptSave();
	void recall();
	void lagout();
	void fish();
	void craft();
	void combo();
	void setLevelInfo(int level, LevelInfo* levelInfo);

	// most
	void group(unsigned long num);
	void monster(Monster* monster);
	void attackDamage(unsigned long num, bstring with);
	void magicDamage(unsigned long num, bstring with);
	void experience(unsigned long num, bstring with);

	unsigned long pkRank() const;
	unsigned long getPkin() const;
	unsigned long getPkwon() const;

	unsigned long getLostExperience() const;
	unsigned long getLastExperienceLoss() const;

	// remove when all players are up to 2.42i
	void setPkin(unsigned long p);
	void setPkwon(unsigned long p);
	void setParent(Player* player);

	LevelInfo* getLevelInfo(int level);
};


#endif	/* _STATISTICS_H */

