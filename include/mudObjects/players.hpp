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

class Player : public Creature {
public:
    static std::string hashPassword(const std::string &pass);

protected:
    void doCopy(const Player& cr);
    void reset();

    bool inList(const std::list<std::string> &list, const std::string &name) const;
    std::string showList(const std::list<std::string> &list) const;
    void addList(std::list<std::string> &list, const std::string &name);
    void delList(std::list<std::string> &list, const std::string &name);
    int doDeleteFromRoom(BaseRoom* room, bool delPortal);
    void finishAddPlayer(BaseRoom* room);
    long getInterest(long principal, double annualRate, long seconds);

public:
    // Constructors, Deconstructors, etc
    Player();
    Player(Player& cr);
    Player(const Player& cr);
    Player& operator=(const Player& cr);
    bool operator< (const Player& t) const;
    ~Player();
    int save(bool updateTime=false, LoadType saveType=LoadType::LS_NORMAL);
    int saveToFile(LoadType saveType=LoadType::LS_NORMAL);
    void loadAnchors(xmlNodePtr curNode);
    void readXml(xmlNodePtr curNode, bool offline=false);
    void saveXml(xmlNodePtr curNode) const;
    void validateId();
    void upgradeStats();
    void recordLevelInfo();

protected:
// Data

    std::string proxyName;
    std::string proxyId;

    char customColors[CUSTOM_COLOR_SIZE];
    unsigned short warnings;
    unsigned short actual_level;
    unsigned short tickDmg;
    unsigned short negativeLevels;
    unsigned short guild; // Guild this player is in
    unsigned short guildRank; // Rank this player holds in their guild
    CreatureClass cClass2;
    unsigned short wimpy;
    short barkskin;
    unsigned short pkin;
    unsigned short pkwon;
    int wrap;
    int luck;
    int ansi;
    unsigned short weaponTrains; // Number of weapons they are able to train in
    long lastLogin;
    long lastInterest;
    unsigned long timeout;
    std::string lastPassword;
    std::string afflictedBy;    // used to determine master with vampirism/lycanthropy
    int tnum[5];
    std::list<std::string> ignoring;
    std::list<std::string> gagging;
    std::list<std::string> refusing;
    std::list<std::string> dueling;
    std::list<std::string> maybeDueling;
    std::list<std::string> watching;
    std::string password;
    std::string title;
    std::string tempTitle;  // this field is purposely not saved
    Socket* mySock;
    std::string surname;
    std::string oldCreated;
    long created;
    std::string lastCommand;
    std::string lastCommunicate;
    std::string forum;      // forum account this character is associated with
    char songs[32];
    struct vstat tstat;
    Anchor *anchor[MAX_DIMEN_ANCHORS];
    cDay *birthday;
    Monster* alias_crt;
    std::list<CatRef> roomExp; // rooms they have gotten exp from
    unsigned short thirst;
    Object* lastPawn;
    int uniqueObjId;
    typedef std::map<std::string, bool> KnownAlchemyEffectsMap;
    KnownAlchemyEffectsMap knownAlchemyEffects;

    bool fleeing = false;

public:
    std::string getFlagList(std::string_view sep=", ") const;

// Data
    std::list<CatRef> storesRefunded;   // Shops the player has refunded an item in
    // (No haggling allowed until a full priced purchase has been made)
    std::list<CatRef> objIncrease; // rooms they have gotten exp from

    std::set<std::string> charms;
    int *scared_of;

    std::list<CatRef> lore;
    std::list<int> recipes;
    typedef std::map<int, QuestCompletion*> QuestCompletionMap;
    typedef std::map<int, QuestCompleted*> QuestCompletedMap;

    QuestCompletionMap questsInProgress;
    QuestCompletedMap questsCompleted;    // List of all quests we've finished and how many times

    Money bank;
    Stat focus; // Battle focus points for fighters
    CustomCrt custom;
    Location bound; // the room the player is bound to
    Statistics statistics;
    Range   bRange[MAX_BUILDER_RANGE];


    bool checkProxyAccess(Player* proxy);

    void setProxy(Player* proxy);
    void setProxy(std::string_view pProxyName, std::string_view pProxyId);
    void setProxyName(std::string_view pProxyName);
    void setProxyId(std::string_view pProxyId);

    std::string getProxyName() const;
    std::string getProxyId() const;

    // Combat & Death
    unsigned int computeAttackPower();
    void dieToPet(Monster *killer);
    void dieToMonster(Monster *killer);
    void dieToPlayer(Player *killer);
    void loseExperience(Monster *killer);
    void dropEquipment(bool dropAll=false, Socket* killSock = nullptr);
    void dropBodyPart(Player *killer);
    bool isGuildKill(const Player *killer) const;
    bool isClanKill(const Player *killer) const;
    bool isGodKill(const Player *killer) const;
    int guildKill(Player *killer);
    int godKill(Player *killer);
    int clanKill(Player *killer);
    int checkLevel();
    void updatePkill(Player *killer);
    void logDeath(Creature *killer);
    void resetPlayer(Creature *killer);
    void getPkilled(Player *killer, bool dueling, bool reset=true);
    void die(DeathType dt);
    bool dropWeapons();
    int checkPoison(Creature* target, Object* weapon);
    int attackCreature(Creature *victim, AttackType attackType = ATTACK_NORMAL);
    void adjustAlignment(Monster *victim);
    void checkWeaponSkillGain();
    void loseAcid();
    void dissolveItem(Creature* creature);
    int computeDamage(Creature* victim, Object* weapon, AttackType attackType,
                      AttackResult& result, Damage& attackDamage, bool computeBonus,
                      int& drain, float multiplier = 1.0);
    int packBonus();
    int getWeaponSkill(const Object* weapon = nullptr) const;
    int getDefenseSkill() const;
    void damageArmor(int dmg);
    void checkArmor(int wear);
    void gainExperience(Monster* victim, Creature* killer, int expAmount, bool groupExp = false);
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
    bool hasQuest(int questId) const;
    bool hasQuest(const QuestInfo *quest) const;
    bool hasDoneQuest(int questId) const;
    QuestCompleted* getQuestCompleted(const int questId) const;
    void updateMobKills(Monster* monster);
    int countItems(const QuestCatRef& obj);
    void updateItems(Object* object);
    void updateRooms(UniqueRoom* room);
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
    Monster* getAlias() const;
    cDay* getBirthday() const;
    std::string getAnchorAlias(int i) const;
    std::string getAnchorRoomName(int i) const;
    const Anchor* getAnchor(int i) const;
    bool hasAnchor(int i) const;
    bool isAnchor(int i, const BaseRoom* room) const;
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
    void setAlias(Monster* m);
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
    bool checkBuilder(UniqueRoom* room, bool reading=true) const;
    bool checkBuilder(const CatRef& cr, bool reading=true) const;
    void listRanges(const Player* viewer) const;
    void initBuilder();
    bool builderCanEditRoom(std::string_view action);

// Movement
    Location getBound();
    void bind(const StartLoc* location);
    void dmPoof(BaseRoom* room, BaseRoom *newRoom);
    void doFollow(BaseRoom* oldRoom);
    void doPetFollow();
    void doRecall(int roomNum = -1);
    void addToSameRoom(Creature* target);
    void addToRoom(BaseRoom* room);
    void addToRoom(AreaRoom* aUoom);
    void addToRoom(UniqueRoom* uRoom);
    bool showExit(const Exit* exit, int magicShowHidden=0) const;
    int checkTraps(UniqueRoom* room, bool self=true, bool isEnter=true);
    int doCheckTraps(UniqueRoom* room);
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
    bool alchemyEffectVisible(Object* obj, std::string_view effect);
    bool learnAlchemyEffect(Object* obj, std::string_view effect);
    int endConsume(Object* object, bool forceDelete=false);
    int consume(Object* object, cmd* cmnd);
    void recallLog(std::string_view name, std::string_view cname, std::string_view room);
    int recallCheckBag(Object *cont, cmd* cmnd, int show, int log);
    int useRecallPotion(int show, int log);


// Stats & Ticking
    long tickInterval(Stat& stat, bool fastTick, bool deathSickness, bool vampAndDay, std::string_view effectName);
    void pulseTick(long t);
    int getHpTickBonus() const;
    int getMpTickBonus() const;
    void calcStats(vstat sendStat, vstat *toStat);
    void changeStats();
    void changingStats(std::string str);

    bool isPureFighter();
    void decreaseFocus();
    void increaseFocus(FocusAction action, unsigned int amt = 0, Creature* target = nullptr);
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
    std::string consider(Creature* creature) const;
    int displayCreature(Creature* target);
    void sendPrompt();
    void defineColors();
    void setSockColors();
    void vprint(const char *fmt, va_list ap) const;
    void escapeText();
    std::string getClassString() const;
    void score(const Player* viewer);
    void information(const Player* viewer=0, bool online=true);
    void showAge(const Player* viewer) const;
    std::string customColorize(const std::string& text, bool caret=true) const;
    void resetCustomColors();

// Misellaneous
    bool swap(const Swap& s);
    bool swapIsInteresting(const Swap& s) const;

    void doRemove(int i);
    int getAge() const;
    unsigned long expToLevel() const;
    std::string expToLevel(bool addX) const;
    std::string expInLevel() const;
    std::string expForLevel() const;
    std::string expNeededDisplay() const;
    int getAdjustedAlignment() const;
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
    bool canWear(const Object* object, bool all=false) const;
    bool canUse(Object* object, bool all=false);
    void checkInventory();
    void checkTempEnchant(Object* object);
    void checkEnvenom(Object* object);
    bool breakObject(Object* object, int loc);
    void wearCursed();
    bool canChooseCustomTitle() const;
    bool alignInOrder() const;

    Socket* getSock() const;
    bool hasSock() const;
    bool isConnected() const;
    void setSock(Socket* pSock);

    bool songIsKnown(int song) const;
    void learnSong(int song);
    void forgetSong(int song);

    std::string getPassword() const;
    bool isPassword(const std::string &pass) const;
    void setPassword(const std::string &pass);

    static bool exists(std::string_view name);
    time_t getIdle();
    int delCharm(Creature* creature);
    int addCharm(Creature* creature);

    void setLastPawn(Object* object);
    bool restoreLastPawn();
    void checkFreeSkills(const std::string &skill);
    void computeInterest(long t, bool online);
};