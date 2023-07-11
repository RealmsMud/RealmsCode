/*
 * players.hpp
 *   Header file for player class
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

class Fishing;

class Player : public Creature {
public:
    static std::string hashPassword(const std::string &pass);

protected:
    void plyCopy(const Player& cr, bool assign=false);
    void plyReset();

    bool inList(const std::list<std::string> &list, const std::string &name) const;
    std::string showList(const std::list<std::string> &list) const;
    void addList(std::list<std::string> &list, const std::string &name);
    void delList(std::list<std::string> &list, const std::string &name);
    int doDeleteFromRoom(std::shared_ptr<BaseRoom> room, bool delPortal) override;
    void finishAddPlayer(const std::shared_ptr<BaseRoom>& room);
    long getInterest(long principal, double annualRate, long seconds);

public:
    // Constructors, Deconstructors, etc
    Player();
    Player(Player& cr);
    Player(const Player& cr);
    Player& operator=(const Player& cr);
    bool operator< (const Player& t) const;
    ~Player() override;
    int save(bool updateTime=false, LoadType saveType=LoadType::LS_NORMAL);
    int saveToFile(LoadType saveType=LoadType::LS_NORMAL);
    void loadAnchors(xmlNodePtr curNode);
    void readXml(xmlNodePtr curNode, bool offline=false);
    void saveXml(xmlNodePtr curNode) const;
    void validateId() override;
    void upgradeStats() override;
    void recordLevelInfo();

protected:
// Data

    std::string proxyName;
    std::string proxyId;

    char customColors[CUSTOM_COLOR_SIZE]{};
    unsigned short warnings{};
    unsigned short actual_level{};
    unsigned short tickDmg{};
    unsigned short negativeLevels{};
    unsigned short guild{}; // Guild this player is in
    unsigned short guildRank{}; // Rank this player holds in their guild
    CreatureClass cClass2;
    unsigned short wimpy{};
    short barkskin{};
    unsigned short pkin{};
    unsigned short pkwon{};
    int wrap{};
    int luck{};
    int ansi{};
    unsigned short weaponTrains{}; // Number of weapons they are able to train in
    long lastLogin{};
    long lastInterest{};
    unsigned long timeout{};
    std::string lastPassword;
    std::string afflictedBy;    // used to determine master with vampirism/lycanthropy
    int tnum[5]{};
    std::list<std::string> ignoring;
    std::list<std::string> gagging;
    std::list<std::string> refusing;
    std::list<std::string> dueling;
    std::list<std::string> maybeDueling;
    std::list<std::string> watching;
    std::string password;
    std::string title;
    std::string tempTitle;  // this field is purposely not saved
    std::weak_ptr<Socket> mySock;
    std::string surname;
    std::string oldCreated;
    long created{};
    std::string lastCommand;
    std::string lastCommunicate;
    std::string forum;      // forum account this character is associated with
    boost::dynamic_bitset<> songs{256};
    struct StatsContainer oldStats{};
    struct StatsContainer newStats{};
    Anchor *anchor[MAX_DIMEN_ANCHORS]{};
    cDay *birthday{};
    std::shared_ptr<Monster>  alias_crt;
    std::list<CatRef> roomExp; // rooms they have gotten exp from
    unsigned short thirst{};
    std::shared_ptr<Object>  lastPawn;
    int uniqueObjId{};
    typedef std::map<std::string, bool> KnownAlchemyEffectsMap;
    KnownAlchemyEffectsMap knownAlchemyEffects;

    bool fleeing = false;

public:
    std::string getFlagList(std::string_view sep=", ") const override;
    void hardcoreDeath();
    void deletePlayer();

// Data
    std::list<CatRef> storesRefunded;   // Shops the player has refunded an item in
    // (No haggling allowed until a full priced purchase has been made)
    std::list<CatRef> objIncrease; // rooms they have gotten exp from

    std::set<std::string> charms;
    int *scared_of{};

    std::list<CatRef> lore;
    std::list<int> recipes;

    // TODO: emplace
    typedef std::map<CatRef, QuestCompletion*> QuestCompletionMap;
    typedef std::map<CatRef, QuestCompleted*> QuestCompletedMap;

    QuestCompletionMap questsInProgress;
    QuestCompletedMap questsCompleted;    // List of all quests we've finished and how many times

    Money bank;
    Stat focus; // Battle focus points for fighters
    CustomCrt custom;
    Location bound; // the room the player is bound to
    Statistics statistics;
    Range   bRange[MAX_BUILDER_RANGE];


    bool checkProxyAccess(const std::shared_ptr<Player>& proxy);

    void setProxy(std::shared_ptr<Player> proxy);
    void setProxy(std::string_view pProxyName, std::string_view pProxyId);
    void setProxyName(std::string_view pProxyName);
    void setProxyId(std::string_view pProxyId);

    std::string getProxyName() const;
    std::string getProxyId() const;

    // Combat & Death
    int computeAttackPower();
    void dieToPet(const std::shared_ptr<Monster>& killer);
    void dieToMonster(const std::shared_ptr<Monster>& killer);
    void dieToPlayer(const std::shared_ptr<Player>&killer);
    void loseExperience(const std::shared_ptr<Monster>& killer);
    void dropEquipment(bool dropAll=false, std::shared_ptr<Creature> killer = nullptr);
    void dropBodyPart(const std::shared_ptr<Player>&killer);
    bool isGuildKill(std::shared_ptr<Player>killer) const;
    bool isClanKill(const std::shared_ptr<Player>&killer) const;
    bool isGodKill(const std::shared_ptr<Player>&killer) const;
    int guildKill(const std::shared_ptr<Player>&killer);
    int godKill(const std::shared_ptr<Player>&killer);
    int clanKill(const std::shared_ptr<Player>&killer);
    int checkLevel();
    void updatePkill(const std::shared_ptr<Player>&killer);
    void logDeath(const std::shared_ptr<Creature>&killer);
    void resetPlayer(const std::shared_ptr<Creature>&killer);
    void getPkilled(const std::shared_ptr<Player>&killer, bool pDueling, bool reset=true);
    void die(DeathType dt);
    bool dropWeapons();
    int checkPoison(std::shared_ptr<Creature> target, std::shared_ptr<Object>  weapon);
    int attackCreature(const std::shared_ptr<Creature>&victim, AttackType attackType = ATTACK_NORMAL);
    void adjustAlignment(std::shared_ptr<Monster> victim);
    void checkWeaponSkillGain();
    void loseAcid();
    void dissolveItem(std::shared_ptr<Creature> creature);
    int computeDamage(std::shared_ptr<Creature> victim, std::shared_ptr<Object>  weapon, AttackType attackType,
                      AttackResult& result, Damage& attackDamage, bool computeBonus,
                      int &drain, float multiplier = 1.0) override;
    int packBonus();
    int getWeaponSkill(std::shared_ptr<Object>  weapon = nullptr) const override;
    int getClassWeaponskillBonus(const std::shared_ptr<Object>  weapon) const;
    int getRacialWeaponskillBonus(const std::shared_ptr<Object>  weapon) const;
    int getDefenseSkill() const override;
    void damageArmor(int dmg);
    void checkArmor(int wear);
    void gainExperience(const std::shared_ptr<Monster> &victim, const std::shared_ptr<Creature> &killer, int expAmount, bool groupExp = false) override;
    void disarmSelf();
    bool lagProtection();
    
    void computeAC();
    void alignAdjustAcThaco();
    void checkOutlawAggro();
    std::string getUnarmedWeaponSkill() const;
    double winterProtection() const;
    bool isHardcore() const;
    bool canMistNow() const;
    bool autoAttackEnabled() const;
    bool isFleeing() const;
    void setFleeing(bool pFleeing);

// Lists
    bool isIgnoring(const std::string &name) const;
    bool isGagging(const std::string &name) const;
    bool isRefusing(const std::string &name) const;
    bool isDueling(const std::string &name) const;
    bool isWatching(const std::string &name) const;
    void clearIgnoring();
    void clearGagging();
    void clearRefusing();
    void clearDueling();
    void clearMaybeDueling();
    void clearWatching();
    void addIgnoring(const std::string &name);
    void addGagging(const std::string &name);
    void addRefusing(const std::string &name);
    void addDueling(const std::string &name);
    void addMaybeDueling(const std::string &name);
    void addWatching(const std::string &name);
    void delIgnoring(const std::string &name);
    void delGagging(const std::string &name);
    void delRefusing(const std::string &name);
    void delDueling(const std::string &name);
    void delMaybeDueling(std::string name);
    void delWatching(const std::string &name);
    std::string showIgnoring() const;
    std::string showGagging() const;
    std::string showRefusing() const;
    std::string showDueling() const;
    std::string showWatching() const;
    void addRefundedInStore(CatRef& store);
    void removeRefundedInStore(CatRef& store);
    bool hasRefundedInStore(CatRef& store);

// Quests
    bool addQuest(QuestInfo* toAdd);
    bool hasQuest(const CatRef& questId) const;
    bool hasQuest(const QuestInfo *quest) const;
    bool hasDoneQuest(const CatRef& questId) const;
    QuestCompleted* getQuestCompleted(const CatRef& questId) const;
    void updateMobKills(const std::shared_ptr<Monster>&  monster);
    int countItems(const QuestCatRef& obj);
    void updateItems(const std::shared_ptr<Object>&  object);
    void updateRooms(const std::shared_ptr<UniqueRoom>& room);
    void saveQuests(xmlNodePtr rootNode) const;
    bool questIsSet(int quest) const;
    void setQuest(int quest);
    void clearQuest(int quest);

// Get
    bool hasSecondClass() const;
    CreatureClass getSecondClass() const;
    unsigned short getGuild() const;
    unsigned short getGuildRank() const;
    unsigned short getActualLevel() const;
    unsigned short getNegativeLevels() const;
    unsigned short getWimpy() const;
    unsigned short getTickDamage() const;
    unsigned short getWarnings() const;
    unsigned short getPkin() const;
    unsigned short getPkwon() const;
    std::string getBankDisplay() const;
    std::string getCoinDisplay() const;
    int getWrap() const;

    short getLuck() const;
    unsigned short getWeaponTrains() const;
    long getLastLogin() const;
    long getLastInterest() const;
    std::string getLastPassword() const;
    std::string getAfflictedBy() const;
    std::string getTitle() const;

    std::string getTempTitle() const;
    std::string getLastCommunicate() const;
    std::string getLastCommand() const;
    std::string getSurname() const;
    std::string getForum() const;
    long getCreated() const;
    std::string getCreatedStr() const;
    std::shared_ptr<Monster>  getAlias() const;
    cDay* getBirthday() const;
    std::string getAnchorAlias(int i) const;
    std::string getAnchorRoomName(int i) const;
    const Anchor* getAnchor(int i) const;
    bool hasAnchor(int i) const;
    bool isAnchor(int i, const std::shared_ptr<BaseRoom>& room) const;
    unsigned short getThirst() const;
    std::string getCustomColor(CustomColor i, bool caret) const;
    int numDiscoveredRooms() const;
    int getUniqueObjId() const;

// Set
    void setTickDamage(unsigned short t);
    void setWarnings(unsigned short w);
    void addWarnings(unsigned short w);
    void subWarnings(unsigned short w);
    void setWimpy(unsigned short w);
    void setActualLevel(unsigned short l);
    void setSecondClass(CreatureClass c);
    void setGuild(unsigned short g);
    void setGuildRank(unsigned short g);
    void setNegativeLevels(unsigned short l);
    void setLuck(int l);
    void setWeaponTrains(unsigned short t);
    void subWeaponTrains(unsigned short t);
    void setLostExperience(unsigned long e);
    void setLastPassword(std::string_view p);
    void setAfflictedBy(std::string_view a);
    void setLastLogin(long l);
    void setLastInterest(long l);
    void setTitle(std::string_view newTitle);
    void setTempTitle(std::string_view newTitle);
    void setLastCommunicate(std::string_view c);
    void setLastCommand(std::string_view c);
    void setCreated();
    void setSurname(const std::string& s);
    void setForum(std::string_view f);
    void setAlias(std::shared_ptr<Monster>  m);
    void setBirthday();
    void delAnchor(int i);
    void setAnchor(int i, std::string_view a);
    void setThirst(unsigned short t);
    void setCustomColor(CustomColor i, char c);
    int setWrap(int newWrap);

// Factions
    int getFactionStanding(const std::string &factionStr) const;
    std::string getFactionMessage(const std::string &factionStr) const;
    void adjustFactionStanding(const std::map<std::string, long>& factionList);

// Staff
    bool checkRangeRestrict(const CatRef& cr, bool reading=true) const;
    bool checkBuilder(std::shared_ptr<UniqueRoom> room, bool reading=true) const;
    bool checkBuilder(const CatRef& cr, bool reading=true) const;
    void listRanges(const std::shared_ptr<const Player> &viewer) const;
    void initBuilder();
    bool builderCanEditRoom(std::string_view action);

// Movement
    Location getBound();
    void bind(const StartLoc* location);
    void dmPoof(const std::shared_ptr<BaseRoom>& room, std::shared_ptr<BaseRoom> newRoom);
    void doFollow(const std::shared_ptr<BaseRoom>& oldRoom);
    void doPetFollow();
    void doRecall(int roomNum = -1);
    void addToSameRoom(const std::shared_ptr<Creature>& target);
    void addToRoom(const std::shared_ptr<BaseRoom>& room);
    void addToRoom(const std::shared_ptr<AreaRoom>& aUoom);
    void addToRoom(const std::shared_ptr<UniqueRoom>& uRoom);
    bool showExit(const std::shared_ptr<Exit>& exit, int magicShowHidden=0) const;
    int checkTraps(std::shared_ptr<UniqueRoom> room, bool self=true, bool isEnter=true);
    int doCheckTraps(const std::shared_ptr<UniqueRoom>& room);
    bool checkHeavyRestrict(std::string_view skill) const;

// Recipes
    void unprepareAllObjects() const;
    void removeItems(const std::list<CatRef>* list, int numIngredients);
    void learnRecipe(int id);
    void learnRecipe(Recipe* recipe);
    bool knowsRecipe(int id) const;
    bool knowsRecipe(Recipe* recipe) const;
    Recipe* findRecipe(cmd* cmnd, std::string_view skill, bool* searchRecipes, Size recipeSize=NO_SIZE, int numIngredients=1) const;

    // Alchemy
    bool alchemyEffectVisible(const std::shared_ptr<const Object>&  obj, std::string_view effect) const;
    bool learnAlchemyEffect(const std::shared_ptr<const Object>&  obj, std::string_view effect);
    int endConsume(const std::shared_ptr<Object>&  object, bool forceDelete=false);
    int consume(const std::shared_ptr<Object>&  object, cmd* cmnd);
    void recallLog(std::string_view name, std::string_view cname, std::string_view room);
    int recallCheckBag(const std::shared_ptr<Object>&cont, cmd* cmnd, int show, int log);
    int useRecallPotion(int show, int log);


// Stats & Ticking
    long tickInterval(Stat& stat, bool fastTick, bool deathSickness, bool vampAndDay, std::string_view effectName);
    void pulseTick(long t) override;
    int getHpTickBonus() const;
    int getMpTickBonus() const;
    void changeStats();
    void changingStats(std::string str);

    bool isPureFighter();
    void decreaseFocus();
    void increaseFocus(FocusAction action, int amt = 0, std::shared_ptr<Creature> target = nullptr);
    void clearFocus();
    bool doPlayerHarmRooms();
    bool doDoTs();
    void loseRage();
    void losePray();
    void loseFrenzy();
    bool statsAddUp() const;
    bool canDefecate() const;

// Formating
    std::string getWhoString(bool whois=false, bool color=true, bool ignoreIllusion=false) const;
    std::string getTimePlayed() const;
    std::string consider(const std::shared_ptr<Creature>& creature) const;
    int displayCreature(const std::shared_ptr<Creature>& target);
    void sendPrompt();
    void defineColors();
    void setSockColors();
    void vprint(const char *fmt, va_list ap) const override;
    void escapeText() override;
    std::string getClassString() const override;
    void score(std::shared_ptr<Player> viewer);
    void information(std::shared_ptr<Player> viewer=0, bool online=true);
    void showAge(std::shared_ptr<Player> viewer) const;
    std::string customColorize(const std::string& text, bool caret=true) const override;
    void resetCustomColors();

// Misc
    bool swap(const Swap& s);
    bool swapIsInteresting(const Swap& s) const;

    void doRemove(int i);
    int getAge() const;
    unsigned long expToLevel() const;
    std::string expToLevel(bool addX) const;
    std::string expInLevel() const;
    std::string expForLevel() const;
    std::string expNeededDisplay() const;
    int getAdjustedAlignment() const override;
    void hasNewMudmail() const;
    bool checkConfusion();
    int autosplit(long amt);

    void courageous();
    void init();
    void uninit();
    void checkEffectsWearingOff();
    void update();
    void upLevel();
    void downLevel();
    void initLanguages();
    void setMonkDice();

    bool checkForSpam();
    void silenceSpammer();
    int getDeityClan() const;
    int chooseItem();

    bool checkOppositeResistSpell(std::string_view effect);

    int computeLuck();
    int getArmorWeight() const;
    int getFallBonus();
    int getSneakChance();
    int getLight() const;
    int getVision() const;
    bool halftolevel() const;
    bool canWear(const std::shared_ptr<Object>&  object, bool all=false) const;
    bool canUseWands(bool print=false);
    bool canUse(const std::shared_ptr<Object>&  object, bool all=false);
    void checkInventory();
    void checkTempEnchant(const std::shared_ptr<Object>&  object);
    void checkEnvenom(const std::shared_ptr<Object>&  object);
    bool breakObject(const std::shared_ptr<Object>&  object, int loc);
    void wearCursed();
    bool canChooseCustomTitle() const;
    bool alignInOrder() const;

    std::shared_ptr<Socket> getSock() const override;
    bool hasSock() const override;
    bool isConnected() const;
    void setSock(std::shared_ptr<Socket> pSock);

    bool songIsKnown(int song) const;
    void learnSong(int song);
    void forgetSong(int song);

    std::string getPassword() const;
    bool isPassword(const std::string &pass) const;
    void setPassword(const std::string &pass);

    static bool exists(std::string_view name);
    time_t getIdle();
    int delCharm(const std::shared_ptr<Creature>& creature);
    int addCharm(const std::shared_ptr<Creature>& creature);

    void setLastPawn(const std::shared_ptr<Object>&  object);
    bool restoreLastPawn();
    void checkFreeSkills(const std::string &skill);
    void computeInterest(long t, bool online);

    // Traps
    void loseAll(bool destroyAll, std::string lostTo);
    void teleportTrap();
    void rockSlide();
    void etherealTravel();

    // Skills
    bool canTrack();
    bool canFish(const Fishing** list, std::shared_ptr<Object>& pole) const;

    bool scentTrack();
    void doTrack();

    bool doFish();

};