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
    void monCopy(const Monster& cr, bool assign=false);
    void monReset();
    int doDeleteFromRoom(std::shared_ptr<BaseRoom> room, bool delPortal);

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
    unsigned short updateAggro{};
    unsigned short loadAggro{};
    unsigned short numwander{};
    unsigned short magicResistance{};
    unsigned short mobTrade{};
    int skillLevel{};
    unsigned short maxLevel{};
    unsigned short cast{};
    Realm baseRealm; // For pets/elementals -> What realm they are
    std::string primeFaction;
    std::string talk;
    // Not giving monsters skills right now, so store it on their character
    int weaponSkill{};
    int defenseSkill{};

    std::weak_ptr<Creature> myMaster;

    std::list<QuestInfo*> quests;

public:
// Data
    char last_mod[25]{}; // last staff member to modify creature.
    char ttalk[72]{};
    char aggroString[80]{};
    char attack[3][CRT_ATTACK_LENGTH]{};
    std::list<TalkResponse*> responses;
    boost::dynamic_bitset<> cClassAggro{32};
    boost::dynamic_bitset<> raceAggro{64};
    boost::dynamic_bitset<> deityAggro{32};

    CatRef info;
    CatRef assist_mob[NUM_ASSIST_MOB];
    CatRef enemy_mob[NUM_ENEMY_MOB];
    CatRef jail;
    CatRef rescue[NUM_RESCUE];
    Carry carry[10]; // Random items monster carries


// Combat & Death
    bool addEnemy(const std::shared_ptr<Creature>& target, bool print=false);
    long clearEnemy(const std::shared_ptr<Creature>& target);

    bool isEnemy(const Creature* target) const;
    bool isEnemy(const std::shared_ptr<const Creature> & target) const;
    bool hasEnemy() const;

    long adjustThreat(const std::shared_ptr<Creature>& target, long modAmt, double threatFactor = 1.0);
    long adjustContribution(const std::shared_ptr<Creature>& target, long modAmt);
    void clearEnemyList();
    bool checkForYell(const std::shared_ptr<Creature>& target);

    std::shared_ptr<Creature> getTarget(bool sameRoom=true);
    bool nearEnemy(const std::shared_ptr<Creature> & target) const;

    ThreatTable threatTable;

    void setMaster(std::shared_ptr<Creature> pMaster);
    std::shared_ptr<Creature> getMaster() const;

    std::shared_ptr<Player> whoToAggro() const;
    bool willAggro(const std::shared_ptr<Player>& player) const;
    int getAdjustedAlignment() const;
    int toJail(std::shared_ptr<Player> player);
    void dieToPet(const std::shared_ptr<Monster>& killer, bool &freeTarget);
    void dieToMonster(const std::shared_ptr<Monster>& killer, bool &freeTarget);
    void dieToPlayer(const std::shared_ptr<Player>&killer, bool &freeTarget);
    void mobDeath(const std::shared_ptr<Creature>&killer=0);
    void mobDeath(const std::shared_ptr<Creature>&killer, bool &freeTarget);
    void finishMobDeath(const std::shared_ptr<Creature>&killer);
    void logDeath(const std::shared_ptr<Creature>&killer);
    void dropCorpse(const std::shared_ptr<Creature>&killer);
    void cleanFollow(const std::shared_ptr<Creature>&killer);
    void distributeExperience(const std::shared_ptr<Creature>&killer);
    void monsterCombat(const std::shared_ptr<Monster>& target);
    bool nearEnemy() const;
    bool nearEnmPly() const;
    bool checkEnemyMobs();
    bool updateCombat();
    bool checkAssist();
    bool willAssist(const std::shared_ptr<Monster> victim) const;
    void adjust(int buffswitch);
    bool tryToDisease(const std::shared_ptr<Creature>& target, SpecialAttack* pAttack = nullptr);
    bool tryToStone(const std::shared_ptr<Creature>& target, SpecialAttack* pAttack = nullptr);
    bool tryToPoison(const std::shared_ptr<Creature>& target, SpecialAttack* pAttack = nullptr);
    bool tryToBlind(const std::shared_ptr<Creature>& target, SpecialAttack* pAttack = nullptr);
    bool zapMp(std::shared_ptr<Creature>victim, SpecialAttack* attack = nullptr);
    bool steal(std::shared_ptr<Player>victim);
    void berserk();
    bool summonMobs(const std::shared_ptr<Creature>&victim);
    int castSpell(const std::shared_ptr<Creature>&target);
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
    void gainExperience(const std::shared_ptr<Monster> &victim, const std::shared_ptr<Creature> &killer, int expAmount,
                        bool groupExp = false);

    int computeDamage(std::shared_ptr<Creature> victim, std::shared_ptr<Object>  weapon, AttackType attackType,
                      AttackResult& result, Damage& attackDamage, bool computeBonus,
                      int &drain, float multiplier = 1.0);
    int mobWield();
    int checkScrollDrop();
    int grabCoins(const std::shared_ptr<Player>& player);
    bool isEnemyMob(const std::shared_ptr<Monster>  target) const;
    void diePermCrt();

// Get
    unsigned short getMobTrade() const;
    std::string getMobTradeName() const;
    int getSkillLevel() const;
    int getMaxLevel() const;
    unsigned short getNumWander() const;
    unsigned short getLoadAggro() const;
    unsigned short getUpdateAggro() const;
    unsigned short getCastChance() const;
    unsigned short getMagicResistance() const;
    std::string getPrimeFaction() const;
    std::string getTalk() const;
    int getWeaponSkill(const std::shared_ptr<Object>  weapon = nullptr) const;
    int getDefenseSkill() const;
    bool getEnchantmentImmunity(const std::shared_ptr<Creature>& caster, const std::string spell, bool print=false);
    bool getEnchantmentVsMontype(const std::shared_ptr<Creature>& caster, const std::string spell, bool print=false);
    void doCheckFailedCastAggro(const std::shared_ptr<Creature>& caster, bool willAggro, bool print);

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
    void addToRoom(const std::shared_ptr<BaseRoom>& room, int num=1);

    void checkSpellWearoff();
    void checkScavange(long t);
    int checkWander(long t);
    bool canScavange(const std::shared_ptr<Object>&  object);

    bool doTalkAction(const std::shared_ptr<Player>& target, std::string action, QuestInfo* quest = nullptr);
    void sayTo(const std::shared_ptr<Player>& player, const std::string& message);
    void pulseTick(long t);
    void beneficialCaster();
    int initMonster(bool loadOriginal = false, bool prototype = false);
    bool willCastHolds(const std::shared_ptr<Creature>& target, int splNo);

    int mobileCrt();
    void getMobSave();
    int getNumMobs() const;
    void clearMobInventory();
    int cleanMobForSaving();

    Realm getBaseRealm() const;
    void setBaseRealm(Realm toSet);
    std::string customColorize(const std::string& text, bool caret=true) const;

    bool hasQuests() const;
    QuestEligibility getEligibleQuestDisplay(const std::shared_ptr<const Creature> & viewer) const;
    QuestTurninStatus checkQuestTurninStatus(const std::shared_ptr<const Creature> & viewer) const;
};
