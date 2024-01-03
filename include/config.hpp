/*
 * config.h
 *   Misc items needed to configure the mud
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
#include <set>
#include <utility>
#include <vector>
#include <cstring>  // strcasecmp

#include "global.hpp"
#include "money.hpp"
#include "msdp.hpp"
#include "namable.hpp"
#include "proc.hpp"
#include "size.hpp"
#include "swap.hpp"
#include "namable.hpp"
#include "weather.hpp"

#include "oldquest.hpp"

class AlchemyInfo;
class Ban;
class BaseRoom;
class Calendar;
class CatRef;
class CatRefInfo;
class Clan;
class ClassData;
class Creature;
class CrtCommand;
class DeityData;
class Effect;
class Faction;
class Fishing;
class Guild;
class GuildCreation;
class Lore;
class MapMarker;
class Monster;
class MsdpVariable;
class MudFlag;
class MudObject;
class MxpElement;
class Object;
class Player;
class PlayerClass;
class PlyCommand;
class Property;
class ProxyManager;
class QuestInfo;
class RaceData;
class Recipe;
class Server;
class Ship;
class SkillCommand;
class SkillInfo;
class SocialCommand;
class Socket;
class Song;
class Spell;
class StartLoc;
class Swap;
class Unique;
class UniqueRoom;
class Zone;
class cWeather;

struct comp {
    bool operator() (const std::string& lhs, const std::string& rhs) const {
        return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
    }
};

struct namableCmp {
    bool operator() (const Nameable& a, const Nameable& b) const {
        return strcasecmp(a.getName().c_str(), b.getName().c_str()) < 0;
    }
};

typedef std::pair<std::string, std::string> accountDouble;
typedef std::map<std::string, MxpElement, comp> MxpElementMap;
typedef std::map<std::string, std::string, comp> stringMap;
typedef std::map<std::string, SkillInfo, comp> SkillInfoMap;
typedef std::map<std::string, Effect, comp> EffectMap;
typedef std::set<SocialCommand, namableCmp> SocialSet;
typedef std::set<PlyCommand, namableCmp> PlyCommandSet;
typedef std::set<CrtCommand, namableCmp> CrtCommandSet;
typedef std::set<SkillCommand, namableCmp> SkillCommandSet;
typedef std::set<Spell, namableCmp> SpellSet;
typedef std::set<Song, namableCmp> SongSet;
typedef std::map<std::string, AlchemyInfo, comp> AlchemyMap;
typedef std::map<std::string, MsdpVariable> MsdpVariableMap;
typedef std::map<int, MudFlag> MudFlagMap;

typedef std::map<int, RaceData*> RaceDataMap;
typedef std::map<int, DeityData*> DeityDataMap;
typedef std::map<int, Recipe*> RecipeMap;
typedef std::map<int, Clan*> ClanMap;
typedef std::map<int, Guild*> GuildMap;
typedef std::map<CatRef, QuestInfo*> QuestInfoMap;
typedef std::map<std::string, Zone> ZoneMap;
typedef std::map<long, std::string> DiscordTokenMap; // webhookId --> token

// Case insensitive

class LottoTicket {
public:
    LottoTicket(const char* name, const int pNumbers[], int pCycle);
    explicit LottoTicket(xmlNodePtr rootNode);
    void saveToXml(xmlNodePtr rootNode);

    std::string owner;  // Owner of this ticket
    short numbers[6]{}; // Numbers
    int lottoCycle; // Lottery Cycle
};

class Config {
public:
    static Config* getInstance();
    static void destroyInstance();

// Instance
    int getPkillInCombatDisabled();

    void setNumGuilds(int guildId);

    // Singleton; no copying or moving
    Config(const Config&) = delete;
    Config(Config&&) = delete;
private:
    ~Config() noexcept(false);
    static Config* myInstance;
    bool inUse;


public:
    bool loadBeforePython();
    bool loadAfterPython();
    bool save() const;
    void cleanUp();

// Proxy
    void loadProxyAccess();
    void saveProxyAccess();
    bool hasProxyAccess(std::shared_ptr<Player> proxy, std::shared_ptr<Player> proxied);
    void grantProxyAccess(std::shared_ptr<Player> proxy, std::shared_ptr<Player> proxied);
    bool removeProxyAccess(std::shared_ptr<Player> proxy, std::shared_ptr<Player> proxied);
    bool removeProxyAccess(std::string_view id, std::shared_ptr<Player> proxied);
    void clearProxyAccess();

    std::string getProxyList(std::shared_ptr<Player> player = nullptr);

// MSDP
    bool initMsdp();
    void clearMsdp();

// Ships
    // these functions deal with time and ships
    [[nodiscard]] int currentHour(bool format=false) const;
    [[nodiscard]] int currentMinutes() const;
    void resetMinutes();
    static void resetShipsFile();
    [[nodiscard]] int expectedShipUpdates() const;


// Mxp Elements
    bool loadMxpElements();
    void clearMxpElements();
    std::string& getMxpColorTag(const std::string &str);

// Commands
    bool initCommands();
    void clearCommands();

// Socials
    bool loadSocials();
    void clearSocials();
    // For help socials
    bool writeSocialFile() const;

// Effects
    bool loadEffects();
    void clearEffects();
    const Effect* getEffect(const std::string &eName);
    bool effectExists(const std::string &eName);

    void clearSpells();
    const Spell* getSpell(const std::string &id, int& ret);

    bool loadSongs();
    void clearSongs();
    const Song* getSong(const std::string &name, int& ret);
    const Song* getSong(const std::string &pName);


// Alchemy
    bool loadAlchemy();
    bool clearAlchemy();
    const AlchemyInfo *getAlchemyInfo(const std::string &effect) const;

// Skills
    [[nodiscard]] bool skillExists(const std::string &skillName) const;
    [[nodiscard]] const SkillInfo * getSkill(const std::string &skillName) const;
    [[nodiscard]] const std::string & getSkillGroupDisplayName(const std::string &groupName) const;
    [[nodiscard]] const std::string & getSkillGroup(const std::string &skillName) const;
    [[nodiscard]] const std::string & getSkillDisplayName(const std::string &skillName) const;
    [[nodiscard]] bool isKnownOnly(const std::string &skillName) const;


// Bans
    bool addBan(Ban* toAdd);
    bool deleteBan(int toDel);
    bool isBanned(std::string_view site);
    int isLockedOut(const std::shared_ptr<Socket>& sock);


// Guilds
    bool guildExists(std::string_view guildName);
    bool addGuild(Guild* toAdd);

    Guild* getGuild(int guildId);
    Guild* getGuild(std::string_view name);
    Guild* getGuild(const std::shared_ptr<Player> player, std::string txt);
    bool deleteGuild(int guildId);

// GuildCreations
    std::string removeGuildCreation(std::string_view leaderName);
    GuildCreation* findGuildCreation(std::string_view name);
    bool addGuildCreation( GuildCreation* toAdd);
    void creationToGuild(GuildCreation* toApprove);
    void guildCreationsRenameSupporter(const std::string &oldName, const std::string &newName);

// Bans
    bool loadBans();
    bool saveBans() const;
    void clearBanList();

// Guilds
    bool loadGuilds();
    bool saveGuilds() const;
    void clearGuildList();

// Factions
    bool loadFactions();
    void clearFactionList();
    [[nodiscard]] const Faction *getFaction(const std::string &factionStr) const;

// Fishing
    bool loadFishing();
    void clearFishing();
    const Fishing *getFishing(const std::string &id) const;

// Old Quests (Quest Table)
    bool loadQuestTable();
    void clearQuestTable();

// New Quests
    bool loadQuests();
    void clearQuests();

// Zones
    bool loadZones();


// Ships
    bool loadShips();
    void saveShips() const;

// Calendar
    void loadCalendar();
    [[nodiscard]] const Calendar* getCalendar() const;

    static int classtoNum(std::string_view str);
    int racetoNum(std::string_view str);
    int deitytoNum(std::string_view str);
    static int stattoNum(std::string_view str);
    static int savetoNum(std::string_view str);

// Uniques
    bool loadLimited();
    void saveLimited() const;
    void clearLimited();
    void addUnique(Unique* unique);
    void listLimited(const std::shared_ptr<Player>& player);
    [[nodiscard]] Unique* getUnique(const std::shared_ptr<const Object>&  object) const;
    [[nodiscard]] Unique* getUnique(int id) const;
    void deleteUniques(const std::shared_ptr<Player>& player);
    void deleteUnique(Unique* unique);
    [[nodiscard]] Lore* getLore(const CatRef& cr) const;
    void addLore(const CatRef& cr, int i);
    void delLore(const CatRef& cr);
    void uniqueDecay(const std::shared_ptr<Player>& player=nullptr);

// Clans
    bool loadClans();
    void clearClans();
    [[nodiscard]] const Clan *getClan(int id) const;
    [[nodiscard]] const Clan *getClanByDeity(int deity) const;

// Recipes
    bool loadRecipes();
    void clearRecipes();
    bool saveRecipes() const;
    Recipe *getRecipe(int id);
    Recipe *searchRecipes(const std::shared_ptr<const Player> &player, std::string_view skill, Size recipeSize, int numIngredients, const std::shared_ptr<Object> &object=nullptr);
    void addRecipe(Recipe* recipe);
    void remRecipe(Recipe* recipe);

// StartLocs
    bool loadStartLoc();
    void clearStartLoc();
    [[nodiscard]] const StartLoc* getStartLoc(const std::string &id) const;
    [[nodiscard]] const StartLoc* getDefaultStartLoc() const;
    [[nodiscard]] const StartLoc* getStartLocByReq(const CatRef& cr) const;
    void saveStartLocs() const;

// Properties
    bool loadProperties();
    bool saveProperties() const;
    void clearProperties();
    void addProperty(Property* p);
    void showProperties(const std::shared_ptr<Player>& viewer, const std::shared_ptr<Player>& player, PropType propType = PROP_NONE);
    Property* getProperty(const CatRef& cr);
    void destroyProperty(Property *p);
    void destroyProperties(std::string_view owner);
    CatRef getAvailableProperty(PropType type, int numRequired);
    void renamePropertyOwner(std::string_view oldName, const std::shared_ptr<Player>& player);
    CatRef getSingleProperty(const std::shared_ptr<const Player>& player, PropType type);

// CatRefInfo
    bool loadCatRefInfo();
    void clearCatRefInfo();
    void saveCatRefInfo() const;
    [[nodiscard]] std::string catRefName(std::string_view area) const;
    [[nodiscard]] const CatRefInfo* getCatRefInfo(std::string_view area, int id=0, int shouldGetParent=0) const;
    [[nodiscard]] const CatRefInfo* getCatRefInfo(const std::shared_ptr<const BaseRoom>& room, int shouldGetParent=0) const;
    [[nodiscard]] const CatRefInfo* getRandomCatRefInfo(int zone) const;


// swap
    void swapLog(const std::string& log, bool external=true);
    void swap(std::shared_ptr<Player> player, std::string_view name);
    void swap(std::string str);
    void offlineSwap(childProcess &child, bool onReap);
    void offlineSwap();
    void findNextEmpty(childProcess &child, bool onReap);
    void finishSwap(const std::string &mover);
    void endSwap(int id=1);
    bool moveRoomRestrictedArea(std::string_view area) const;
    bool moveObjectRestricted(const CatRef& cr) const;
    void swapEmptyQueue();
    void swapNextInQueue();
    void swapAddQueue(const Swap& s);
    bool inSwapQueue(const CatRef& origin, SwapType type, bool checkTarget=false);
    int swapQueueSize();
    bool isSwapping() const;
    void setMovingRoom(const CatRef& o, const CatRef& t);
    void swapInfo(const std::shared_ptr<Player>& player);
    void swapAbort();
    bool checkSpecialArea(const CatRef& origin, const CatRef& target, int (CatRefInfo::*toCheck), const std::shared_ptr<Player>& player, bool online, std::string_view type);
    bool swapChecks(const std::shared_ptr<Player>& player, const Swap& s) const;
    bool swapIsInteresting(const std::shared_ptr<const MudObject>& target) const;

// Misc
    [[nodiscard]] const RaceData* getRace(std::string race) const;
    [[nodiscard]] const RaceData* getRace(int id) const;
    [[nodiscard]] const DeityData* getDeity(int id) const;

    static unsigned long expNeeded(int level);
    int getMaxSong();

    [[nodiscard]] std::string getVersion();
    [[nodiscard]] std::string getMudName();
    [[nodiscard]] std::string getMudNameAndVersion();
    [[nodiscard]] short getPortNum() const;
    void setPortNum(short pPort);
    [[nodiscard]] std::string weatherize(WeatherString w, const std::shared_ptr<BaseRoom>& room) const;
    [[nodiscard]] std::string getMonthDay() const;
    [[nodiscard]] bool isAprilFools() const;
    [[nodiscard]] bool willAprilFools() const;
    [[nodiscard]] bool bonusXpActive() const;
    [[nodiscard]] int getFlashPolicyPort() const;
    [[nodiscard]] bool sendTxtOnCrash() const;
    void toggleTxtOnCrash();
    [[nodiscard]] int getShopNumObjects() const;
    [[nodiscard]] int getShopNumLines() const;

    std::string getSpecialFlag(int index);

// Get
    [[nodiscard]] const cWeather* getWeather() const;

    [[nodiscard]] std::string getDmPass() const;
    [[nodiscard]] std::string getWebserver() const;
    [[nodiscard]] std::string getQS() const;
    [[nodiscard]] std::string getUserAgent() const;
    [[nodiscard]] std::string getReviewer() const;

    [[nodiscard]] std::string getCustomColor(CustomColor i, bool caret) const;

// Lottery
    void addTicket(LottoTicket* ticket);
    int getCurrentLotteryCycle();
    int getLotteryEnabled();
    long getLotteryJackpot();
    int getLotteryTicketPrice();
    void increaseJackpot(int amnt);
    void getNumbers(short numberArray[]);

    void runLottery();
    void setLotteryRunTime();

    long getLotteryWinnings();
    void winLottery();
    void addLotteryWinnings(long prize);
    std::string getLotteryRunTimeStr();
    time_t getLotteryRunTime();
    int getLotteryTicketsSold();

protected:
    Config();   // Can only be instantiated by itself -- Instance only

    void reset(bool reload=false);

    void loadGeneral(xmlNodePtr rootNode);
    void loadLottery(xmlNodePtr rootNode);
    void loadTickets(xmlNodePtr rootNode);
    void loadWebhookTokens(xmlNodePtr rootNode);

    void clearSkills();
    bool loadSkillGroups();
    bool loadSkills();


public:
    bool loadConfig(bool reload=false);
    bool loadDiscordConfig();
    bool saveConfig() const;

// Player Classes
    bool loadClasses();
    void clearClasses();

// Races
    bool loadRaces();
    void clearRaces();
    unsigned short raceCount() const;
    unsigned short getPlayableRaceCount();

// Deities
    bool loadDeities();
    void clearDeities();

    bool loadFlags();
    void clearFlags();
    bool writeSpellFiles() const;

    bool writeHelpFiles() const;

private:
    bool bHavePort = false;
public:
    [[nodiscard]] bool hasPort() const;

private:

    // Config Options
    bool    conjureDisabled{};
    bool    lessExpLoss{};
    bool    pkillInCombatDisabled{};
    bool    autoShutdown{};
    bool    checkDouble{};
    bool    getHostByName{};
    bool    recordAll{};
    bool    logSuicide{};
    bool    charCreationDisabled{};
    short   portNum{};
    short   minBroadcastLevel{};
    bool    saveOnDrop{};
    bool    logDeath{};
    short   crashes{};
    short   supportRequiredForGuild{};
    int     numGuilds{};
    int     nextGuildId{};
    int     maxDouble = 6; // Defaults to 6, change in config if you want it different
public:
    [[nodiscard]] int getNextGuildId() const;
    [[nodiscard]] bool getCheckDouble() const;

    void setNextGuildId(int pNextGuildId);

private:

    std::string mudName;
    std::string dmPass;
    std::string webserver;
    std::string qs;             // authorization query string
    std::string userAgent;
    std::string defaultArea;
public:
    void setDefaultArea(const std::string &pDefaultArea);
    [[nodiscard]] const std::string &getDefaultArea() const;

private:
    bool listing;

    // Swap
    std::list<std::string> swapList;
    std::list<Swap> swapQueue;
    Swap    currentSwap;

    // Lottery
    std::list<LottoTicket*> tickets;
    bool    lotteryEnabled{};
    int     lotteryCycle{};
    long    lotteryJackpot{};
    bool    lotteryWon{};
    long    lotteryWinnings{}; // Money made from the lottery since the last jackpot
    int     lotteryTicketPrice{};
    long    lotteryTicketsSold{};
    time_t  lotteryRunTime{};
    short   lotteryNumbers[6]{};
    bool    swapping{};
    bool    roomSearchFailure{};
    char    customColors[CUSTOM_COLOR_SIZE]{};
    bool    doAprilFools{};
    bool    doBonusXP{};
    bool    txtOnCrash{};
    int     flashPolicyPort{};
    int     shopNumObjects{};
    int     shopNumLines{};
    std::string reviewer;

    std::list<Unique*> uniques;
    std::list<Lore*> lore;
    std::list<accountDouble> accountDoubleLog;

    ProxyManager* proxyManager{};

    // Quests
public:
    // Global pointer to quests; quests are allocated in the individual zones
    QuestInfoMap quests;
    MxpElementMap mxpElements;
    stringMap mxpColors;
    QuestInfo* getQuest(const CatRef& questNum);

    // Zones
public:
    ZoneMap zones;

public:
    // Misc
    std::string cmdline{};

    // MSDP
    MsdpVariableMap msdpVariables;
    MsdpVariable* getMsdpVariable(const std::string &name);


    // Alchemy
    AlchemyMap alchemy;

    // Bans
    std::list<Ban*> bans;

    // Effects
    EffectMap effects;

    // Commands
    PlyCommandSet staffCommands;
    PlyCommandSet playerCommands;
    CrtCommandSet generalCommands;

    SpellSet spells;
    SongSet songs;

    SocialSet socials;

    // All Skill Commands are SkillInfos, but not all SkillInfos are SkillCommands
    SkillCommandSet skillCommands;
    SkillInfoMap skills;

    // Guilds
    std::list<GuildCreation*> guildCreations;
    GuildMap guilds;

    // Factions
    std::map<std::string, Faction*> factions;

    std::map<std::string, std::string> skillGroups;
    std::map<std::string, PlayerClass*> classes;
    std::map<std::string, StartLoc*> start;
    std::map<std::string, Fishing> fishing;
    std::list<Property*> properties;
    std::list<CatRefInfo*> catRefInfo;

    RaceDataMap races;
    DeityDataMap deities;
    RecipeMap recipes;
    ClanMap clans;

    questPtr questTable[MAX_QUEST]{};

    MudFlagMap rflags;
    MudFlagMap xflags;
    MudFlagMap pflags;
    MudFlagMap mflags;
    MudFlagMap oflags;
    MudFlagMap specialFlags;
    MudFlagMap propStorFlags;
    MudFlagMap propShopFlags;
    MudFlagMap propHouseFlags;
    MudFlagMap propGuildFlags;

    static std::string getFlag(int flagNum, MudFlagMap& flagMap);

    inline std::string getRFlag(int flagNum) { return getFlag(flagNum, rflags); };
    inline std::string getXFlag(int flagNum) { return getFlag(flagNum, xflags); };
    inline std::string getPFlag(int flagNum) { return getFlag(flagNum, pflags); };
    inline std::string getMFlag(int flagNum) { return getFlag(flagNum, mflags); };
    inline std::string getOFlag(int flagNum) { return getFlag(flagNum, oflags); };

    Calendar    *calendar{};
    std::list<Ship> ships;

public:
    void setListing(bool isListing);
    bool isListing();

private:
    static unsigned long needed_exp[];


    // Discord Integration
private:
    bool botEnabled = false;
public:
    [[nodiscard]] bool isBotEnabled() const;

private:
    std::string botToken;
    DiscordTokenMap webhookTokens;

public:
    [[nodiscard]] const std::string &getBotToken() const;
    [[nodiscard]] const std::string &getWebhookToken(long webhookID) const;
    void clearWebhookTokens();

    [[nodiscard]] int getMaxDouble() const;
};

extern Config *gConfig;

