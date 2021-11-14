/*
 * monsters.hpp
 *   Header file for monster class
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

#include <string>

#include "mudObjects/creatures.hpp"
#include "mudObjects/players.hpp"

//*********************************************************************
//                      Monster
//*********************************************************************

class Monster : public Creature {
public:
    static char mob_trade_str[][16];


protected:
    void doCopy(const Monster& cr);
    void reset();
    int doDeleteFromRoom(BaseRoom* room, bool delPortal);

public:
// Constructors, Deconstructors, etc
    Monster();
    Monster(Monster& cr);
    Monster(const Monster& cr);
    Monster& operator=(const Monster& cr);
    bool operator< (const Monster& t) const;
    ~Monster();
    void readXml(xmlNodePtr curNode, bool offline=false);
    void saveXml(xmlNodePtr curNode) const;
    int saveToFile();
    void validateId();
    void upgradeStats();

    std::string getFlagList(std::string_view sep=", ") const;

protected:
// Data
    unsigned short updateAggro;
    unsigned short loadAggro;
    unsigned short numwander;
    unsigned short magicResistance;
    unsigned short mobTrade;
    int skillLevel;
    unsigned short maxLevel;
    unsigned short cast;
    Realm baseRealm; // For pets/elementals -> What realm they are
    std::string primeFaction;
    std::string talk;
    // Not giving monsters skills right now, so store it on their character
    int weaponSkill;
    int defenseSkill;

    Creature* myMaster;

    std::list<QuestInfo*> quests;

public:
// Data
    char last_mod[25]; // last staff member to modify creature.
    char ttalk[72];
    char aggroString[80];
    char attack[3][CRT_ATTACK_LENGTH];
    std::list<TalkResponse*> responses;
    char cClassAggro[4]; // 32 max
    char raceAggro[4]; // 32 max
    char deityAggro[4]; // 32 max
    CatRef info;
    CatRef assist_mob[NUM_ASSIST_MOB];
    CatRef enemy_mob[NUM_ENEMY_MOB];
    CatRef jail;
    CatRef rescue[NUM_RESCUE];
    Carry carry[10]; // Random items monster carries


// Combat & Death
    bool addEnemy(Creature* target, bool print=false);
    long clearEnemy(Creature* target);

    bool isEnemy(const Creature* target) const;
    bool hasEnemy() const;

    long adjustThreat(Creature* target, long modAmt, double threatFactor = 1.0);
    long adjustContribution(Creature* target, long modAmt);
    void clearEnemyList();
    bool checkForYell(Creature* target);

    Creature* getTarget(bool sameRoom=true);
    bool nearEnemy(const Creature* target) const;

    ThreatTable* threatTable;

    void setMaster(Creature* pMaster);
    Creature* getMaster() const;

    Player* whoToAggro() const;
    bool willAggro(const Player *player) const;
    int getAdjustedAlignment() const;
    int toJail(Player* player);
    void dieToPet(Monster *killer, bool &freeTarget);
    void dieToMonster(Monster *killer, bool &freeTarget);
    void dieToPlayer(Player *killer, bool &freeTarget);
    void mobDeath(Creature *killer=0);
    void mobDeath(Creature *killer, bool &freeTarget);
    void finishMobDeath(Creature *killer);
    void logDeath(Creature *killer);
    void dropCorpse(Creature *killer);
    void cleanFollow(Creature *killer);
    void distributeExperience(Creature *killer);
    void monsterCombat(Monster *target);
    bool nearEnemy() const;
    bool nearEnmPly() const;
    bool checkEnemyMobs();
    bool updateCombat();
    bool checkAssist();
    bool willAssist(const Monster *victim) const;
    void adjust(int buffswitch);
    bool tryToDisease(Creature* target, SpecialAttack* pAttack = nullptr);
    bool tryToStone(Creature* target, SpecialAttack* pAttack = nullptr);
    bool tryToPoison(Creature* target, SpecialAttack* pAttack = nullptr);
    bool tryToBlind(Creature* target, SpecialAttack* pAttack = nullptr);
    bool zapMp(Creature *victim, SpecialAttack* attack = nullptr);
    bool steal(Player *victim);
    void berserk();
    bool summonMobs(Creature *victim);
    int castSpell(Creature *target);
    bool petCaster();
    int mobDeathScream();
    bool possibleEnemy();
    int doHarmfulAuras();
    void validateAc();
    bool isRaceAggro(int x, bool checkInvert) const;
    void setRaceAggro(int x);
    void clearRaceAggro(int x);
    bool isClassAggro(int x, bool checkInvert) const;
    void setClassAggro(int x);
    void clearClassAggro(int x);
    bool isDeityAggro(int x, bool checkInvert) const;
    void setDeityAggro(int x);
    void clearDeityAggro(int x);
    void gainExperience(Monster* victim, Creature* killer, int expAmount,
                        bool groupExp = false);

    int computeDamage(Creature* victim, Object* weapon, AttackType attackType,
                      AttackResult& result, Damage& attackDamage, bool computeBonus,
                      int& drain, float multiplier = 1.0);
    int mobWield();
    int checkScrollDrop();
    int grabCoins(Player* player);
    bool isEnemyMob(const Monster* target) const;
    void diePermCrt();

// Get
    unsigned short getMobTrade() const;
    std::string getMobTradeName() const;
    int getSkillLevel() const;
    unsigned int getMaxLevel() const;
    unsigned short getNumWander() const;
    unsigned short getLoadAggro() const;
    unsigned short getUpdateAggro() const;
    unsigned short getCastChance() const;
    unsigned short getMagicResistance() const;
    std::string getPrimeFaction() const;
    std::string getTalk() const;
    int getWeaponSkill(const Object* weapon = nullptr) const;
    int getDefenseSkill() const;

// Set
    void setMaxLevel(unsigned short l);
    void setCastChance(unsigned short c);
    void setMagicResistance(unsigned short m);
    void setLoadAggro(unsigned short a);
    void setUpdateAggro(unsigned short a);
    void setNumWander(unsigned short n);
    void setSkillLevel(int l);
    void setMobTrade(unsigned short t);
    void setPrimeFaction(std::string_view f);
    void setTalk(std::string_view t);
    void setWeaponSkill(int amt);
    void setDefenseSkill(int amt);


// Miscellaneous
    bool swap(const Swap& s);
    bool swapIsInteresting(const Swap& s) const;

    void killUniques();
    void escapeText();
    void convertOldTalks();
    void addToRoom(BaseRoom* room, int num=1);

    void checkSpellWearoff();
    void checkScavange(long t);
    int checkWander(long t);
    bool canScavange(Object* object);

    bool doTalkAction(Player* target, std::string action, QuestInfo* quest = nullptr);
    void sayTo(const Player* player, const std::string& message);
    void pulseTick(long t);
    void beneficialCaster();
    int initMonster(bool loadOriginal = false, bool prototype = false);

    int mobileCrt();
    void getMobSave();
    int getNumMobs() const;
    void clearMobInventory();
    int cleanMobForSaving();

    Realm getBaseRealm() const;
    void setBaseRealm(Realm toSet);
    std::string customColorize(const std::string& text, bool caret=true) const;

    bool hasQuests() const;
    QuestEligibility getEligibleQuestDisplay(const Creature* viewer) const;
    QuestTurninStatus checkQuestTurninStatus(const Creature* viewer) const;
};
