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
#ifndef CONFIG_H
#define CONFIG_H

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

class SkillInfo;
class AlchemyInfo;
class CatRefInfo;
class QuestInfo;
class MudFlag;
class cWeather;

class Lore;
class Unique;
class PlayerClass;
class GuildCreation;
class Ban;
class Property;
class Ship;
class Effect;
class Calendar;

class ProxyManager;

class MsdpVariable;
class MxpElement;

class CatRef;
class MapMarker;
class StartLoc;

class Server;
class Socket;

class Guild;
class Faction;
class Clan;

class BaseRoom;
class Creature;
class Monster;
class MudObject;
class Object;
class Player;
class UniqueRoom;

class Spell;
class Song;

class PlyCommand;
class CrtCommand;
class SkillCommand;
class SocialCommand;

class Fishing;
class Recipe;

class ClassData;
class DeityData;
class RaceData;

class Swap;

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
typedef std::map<std::string, MxpElement*, comp> MxpElementMap;
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
typedef std::map<unsigned int, MudFlag> MudFlagMap;

typedef std::map<unsigned int, RaceData*> RaceDataMap;
typedef std::map<unsigned int, DeityData*> DeityDataMap;
typedef std::map<unsigned int, Recipe*> RecipeMap;
typedef std::map<unsigned int, Clan*> ClanMap;
typedef std::map<unsigned int, Guild*> GuildMap;
typedef std::map<unsigned int, QuestInfo*> QuestInfoMap;
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
    ~Config();
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
    bool hasProxyAccess(Player* proxy, Player* proxied);
    void grantProxyAccess(Player* proxy, Player* proxied);
    bool removeProxyAccess(Player* proxy, Player* proxied);
    bool removeProxyAccess(std::string_view id, Player* proxied);
    void clearProxyAccess();

    std::string getProxyList(Player* player = nullptr);

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
    int isLockedOut(Socket* sock);


// Guilds
    bool guildExists(std::string_view guildName);
    bool addGuild(Guild* toAdd);

    Guild* getGuild(int guildId);
    Guild* getGuild(std::string_view name);
    Guild* getGuild(const Player* player, std::string txt);
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

// Ships
    bool loadShips();
    void saveShips() const;
    void clearShips();

// Calendar
    void loadCalendar();
    [[nodiscard]] const Calendar* getCalendar() const;

    int classtoNum(std::string_view str);
    int racetoNum(std::string_view str);
    int deitytoNum(std::string_view str);
    int stattoNum(std::string_view str);
    int savetoNum(std::string_view str);

// Uniques
    bool loadLimited();
    void saveLimited() const;
    void clearLimited();
    void addUnique(Unique* unique);
    void listLimited(const Player* player);
    [[nodiscard]] Unique* getUnique(const Object* object) const;
    [[nodiscard]] Unique* getUnique(int id) const;
    void deleteUniques(Player* player);
    void deleteUnique(Unique* unique);
    [[nodiscard]] Lore* getLore(const CatRef& cr) const;
    void addLore(const CatRef& cr, int i);
    void delLore(const CatRef& cr);
    void uniqueDecay(Player* player=nullptr);

// Clans
    bool loadClans();
    void clearClans();
    [[nodiscard]] const Clan *getClan(unsigned int id) const;
    [[nodiscard]] const Clan *getClanByDeity(unsigned int deity) const;

// Recipes
    bool loadRecipes();
    void clearRecipes();
    bool saveRecipes() const;
    Recipe *getRecipe(int id);
    Recipe *searchRecipes(const Player* player, std::string_view skill, Size recipeSize, int numIngredients, const Object* object=nullptr);
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
    void showProperties(Player* viewer, Player* player, PropType propType = PROP_NONE);
    Property* getProperty(const CatRef& cr);
    void destroyProperty(Property *p);
    void destroyProperties(std::string_view owner);
    CatRef getAvailableProperty(PropType type, int numRequired);
    void renamePropertyOwner(std::string_view oldName, Player *player);
    CatRef getSingleProperty(const Player* player, PropType type);

// CatRefInfo
    bool loadCatRefInfo();
    void clearCatRefInfo();
    void saveCatRefInfo() const;
    [[nodiscard]] std::string catRefName(std::string_view area) const;
    [[nodiscard]] const CatRefInfo* getCatRefInfo(std::string_view area, int id=0, int shouldGetParent=0) const;
    [[nodiscard]] const CatRefInfo* getCatRefInfo(const BaseRoom* room, int shouldGetParent=0) const;
    [[nodiscard]] const CatRefInfo* getRandomCatRefInfo(int zone) const;


// swap
    void swapLog(const std::string& log, bool external=true);
    void swap(Player* player, std::string_view name);
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
    void swapInfo(const Player* player);
    void swapAbort();
    bool checkSpecialArea(const CatRef& origin, const CatRef& target, int (CatRefInfo::*toCheck), Player* player, bool online, std::string_view type);
    bool swapChecks(const Player* player, const Swap& s);
    bool swapIsInteresting(const MudObject* target) const;

// Double Logging
    void addDoubleLog(std::string_view forum1, std::string_view forum2);
    void remDoubleLog(std::string_view forum1, std::string_view forum2);
    bool canDoubleLog(std::string_view forum1, std::string_view forum2) const;
    bool loadDoubleLog();
    void saveDoubleLog() const;
    void listDoubleLog(const Player* viewer) const;

// Misc
    [[nodiscard]] const RaceData* getRace(std::string race) const;
    [[nodiscard]] const RaceData* getRace(unsigned int id) const;
    [[nodiscard]] const DeityData* getDeity(int id) const;

    static unsigned long expNeeded(int level);
    int getMaxSong();

    [[nodiscard]] std::string getVersion();
    [[nodiscard]] std::string getMudName();
    [[nodiscard]] std::string getMudNameAndVersion();
    [[nodiscard]] short getPortNum() const;
    void setPortNum(short pPort);
    [[nodiscard]] std::string weatherize(WeatherString w, const BaseRoom* room) const;
    [[nodiscard]] std::string getMonthDay() const;
    [[nodiscard]] bool isAprilFools() const;
    [[nodiscard]] bool willAprilFools() const;
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
    unsigned short raceCount();
    unsigned short getPlayableRaceCount();

// Deities
    bool loadDeities();
    void clearDeities();

    bool loadFlags();
    void clearFlags();
    bool writeSpellFiles() const;

    bool writeHelpFiles() const;

#ifdef SQL_LOGGER
// Sql Logging
    std::string getDbConnectionString();
#endif

private:
    bool bHavePort = false;
public:
    bool hasPort() const;

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
    // loaded from catrefinfo file

//#ifdef SQL_LOGGER
    std::string logDbType;
    std::string logDbUser;
    std::string logDbPass;
    std::string logDbDatabase;
//#endif

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
    QuestInfoMap quests;
    MxpElementMap mxpElements;
    stringMap mxpColors;
    QuestInfo* getQuest(unsigned int questNum);

public:
    // Misc
    char        cmdline[256]{};

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

    static std::string getFlag(unsigned int flagNum, MudFlagMap& flagMap);

    inline std::string getRFlag(int flagNum) { return getFlag(flagNum, rflags); };
    inline std::string getXFlag(int flagNum) { return getFlag(flagNum, xflags); };
    inline std::string getPFlag(int flagNum) { return getFlag(flagNum, pflags); };
    inline std::string getMFlag(int flagNum) { return getFlag(flagNum, mflags); };
    inline std::string getOFlag(int flagNum) { return getFlag(flagNum, oflags); };

    Calendar    *calendar{};
    std::list<Ship*> ships;

public:
    void setListing(bool isListing);
    bool isListing();

private:
    static unsigned long needed_exp[];


    // Discord Integration
private:
    bool botEnabled = false;
public:
    bool isBotEnabled() const;

private:
    std::string botToken;
    DiscordTokenMap webhookTokens;

public:
    [[nodiscard]] const std::string &getBotToken() const;
    [[nodiscard]] const std::string &getWebhookToken(long webhookID) const;
    void clearWebhookTokens();

};

extern Config *gConfig;

#endif
