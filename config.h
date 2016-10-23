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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef CONFIG_H
#define CONFIG_H

class Lore;
class Unique;
class PlayerClass;
class SkillInfo;
class GuildCreation;
class Ban;
class Clan;
class Property;
class CatRefInfo;
class Ship;
class QuestInfo;
class Calendar;
class Fishing;
class Effect;
class MsdpVariable;
class MxpElement;
class SocialCommand;
class ProxyManager;

typedef std::pair<bstring, bstring> accountDouble;
typedef std::map<bstring, MxpElement*> MxpElementMap;
typedef std::map<bstring, bstring> BstringMap;
typedef std::map<bstring, SocialCommand*> SocialMap;
typedef std::map<bstring, SkillInfo*> SkillInfoMap;
typedef std::map<bstring, PlyCommand*> PlyCommandMap;
typedef std::map<bstring, CrtCommand*> CrtCommandMap;
typedef std::map<bstring, SkillCommand*> SkillCommandMap;
typedef std::map<bstring, Spell*> SpellMap;
typedef std::map<bstring, Song*> SongMap;
typedef std::map<bstring, AlchemyInfo*> AlchemyMap;
typedef std::map<int, MudFlag> MudFlagMap;

class LottoTicket {
public:
    LottoTicket(const char* name, int pNumbers[], int pCycle);
    LottoTicket(xmlNodePtr rootNode);
    void saveToXml(xmlNodePtr rootNode);

    bstring owner;  // Owner of this ticket
    short numbers[6]; // Numbers
    int lottoCycle; // Lottery Cycle
};

class Config {
public:
    static Config* getInstance();
    static void destroyInstance();

// Instance
private:
    static Config* myInstance;
    bool inUse;


public:
    ~Config();

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
    bool removeProxyAccess(bstring id, Player* proxied);
    void clearProxyAccess();

    bstring getProxyList(Player* player = nullptr);

// MSDP
    bool loadMsdpVariables();
    void clearMsdpVariables();


// Ships
    // these functions deal with time and ships
    int currentHour(bool format=false) const;
    int currentMinutes() const;
    void resetMinutes();
    void resetShipsFile();
    int expectedShipUpdates() const;


// Mxp Elements
    bool loadMxpElements();
    void clearMxpElements();
    bstring& getMxpColorTag(bstring str);

// Commands
    bool initCommands();
    void clearCommands();

// Socials
    bool loadSocials();
    bool saveSocials();
    void clearSocials();
    // For help socials
    bool writeSocialFile() const;

// Effects
    bool loadEffects();
    void clearEffects();
    Effect* getEffect(bstring eName);
    bool effectExists(bstring eName);

// Spells
    bool loadSpells();
    bool saveSpells() const;
    void clearSpells();
    Spell* getSpell(bstring sName, int& ret);

// New Songs
    bool loadSongs();
    bool saveSongs() const;
    void clearSongs();
    Song* getSong(bstring sName, int& ret);
    Song* getSong(bstring sName);


// Alchemy
    bool loadAlchemy();
    bool clearAlchemy();
    const AlchemyInfo *getAlchemyInfo(bstring effect) const;

// Skills
    bool skillExists(const bstring& skillName) const;
    SkillInfo* getSkill(const bstring& skillName) const;
    bstring getSkillGroupDisplayName(const bstring& groupName) const;
    bstring getSkillGroup(const bstring& skillName) const;
    bstring getSkillDisplayName(const bstring& skillName) const;
    bool isKnownOnly(const bstring& skillName) const;


// Bans
    bool addBan(Ban* toAdd);
    bool deleteBan(int toDel);
    bool isBanned(const char *site);
    int isLockedOut(Socket* sock);


// Guilds
    bool guildExists(const bstring guildName);
    bool addGuild(Guild* toAdd);

    Guild* getGuild(int guildId);
    Guild* getGuild(const bstring& name);
    Guild* getGuild(const Player* player, bstring txt);
    bool deleteGuild(int guildId);

// GuildCreations
    bstring removeGuildCreation(const bstring leaderName);
    GuildCreation* findGuildCreation(const bstring name);
    bool addGuildCreation( GuildCreation* toAdd);
    void creationToGuild(GuildCreation* toApprove);
    void guildCreationsRenameSupporter(bstring oldName, bstring newName);

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
    const Faction *getFaction(bstring factionStr) const;

// Fishing
    bool loadFishing();
    void clearFishing();
    const Fishing *getFishing(bstring id) const;

// Old Quests (Quest Table)
    bool loadQuestTable();
    void clearQuestTable();

// New Quests
    bool loadQuests();
    void resetParentQuests();
    void clearQuests();

// Areas
    Area *getArea(int id);
    bool loadAreas();
    void clearAreas();
    void areaInit(Creature* player, xmlNodePtr curNode);
    void areaInit(Creature* player, MapMarker mapmarker);
    void saveAreas(bool saveRooms) const;
    void cleanUpAreas();

// Ships
    bool loadShips();
    void saveShips() const;
    void clearShips();

// Calendar
    void loadCalendar();
    const Calendar* getCalendar() const;

    int classtoNum(bstring str);
    int racetoNum(bstring str);
    int deitytoNum(bstring str);
    int stattoNum(bstring str);
    int savetoNum(bstring str);

// Uniques
    bool loadLimited();
    void saveLimited() const;
    void clearLimited();
    void addUnique(Unique* unique);
    void listLimited(const Player* player);
    void showUnique(const Player* player, int id);
    Unique* getUnique(const Object* object) const;
    Unique* getUnique(int id) const;
    void deleteUniques(Player* player);
    void deleteUnique(Unique* unique);
    Lore* getLore(const CatRef cr) const;
    void addLore(const CatRef cr, int i);
    void delLore(const CatRef cr);
    void uniqueDecay(Player* player=0);

// Clans
    bool loadClans();
    void clearClans();
    const Clan *getClan(unsigned int id) const;
    const Clan *getClanByDeity(unsigned int deity) const;

// Recipes
    bool loadRecipes();
    void clearRecipes();
    bool saveRecipes() const;
    Recipe *getRecipe(int id);
    Recipe *searchRecipes(const Player* player, bstring skill, Size recipeSize, int numIngredients, const Object* object=0);
    void addRecipe(Recipe* recipe);
    void remRecipe(Recipe* recipe);

// StartLocs
    bool loadStartLoc();
    void clearStartLoc();
    const StartLoc* getStartLoc(bstring id) const;
    const StartLoc* getDefaultStartLoc() const;
    const StartLoc* getStartLocByReq(CatRef cr) const;
    void saveStartLocs() const;

// Properties
    bool loadProperties();
    bool saveProperties() const;
    void clearProperties();
    void addProperty(Property* p);
    void showProperties(Player* viewer, Player* player, PropType propType = PROP_NONE);
    Property* getProperty(CatRef cr);
    void destroyProperty(Property *p);
    void destroyProperties(bstring owner);
    CatRef getAvailableProperty(PropType type, int numRequired);
    void renamePropertyOwner(bstring oldName, Player *player);
    CatRef getSingleProperty(const Player* player, PropType type);

// CatRefInfo
    bool loadCatRefInfo();
    void clearCatRefInfo();
    void saveCatRefInfo() const;
    bstring catRefName(bstring area) const;
    const CatRefInfo* getCatRefInfo(bstring area, int id=0, int shouldGetParent=0) const;
    const CatRefInfo* getCatRefInfo(const BaseRoom* room, int shouldGetParent=0) const;
    const CatRefInfo* getRandomCatRefInfo(int zone) const;


// swap
    void swapLog(const bstring log, bool external=true);
    void swap(Player* player, bstring name);
    void swap(bstring str);
    void offlineSwap(Server::childProcess &child, bool onReap);
    void offlineSwap();
    void findNextEmpty(Server::childProcess &child, bool onReap);
    void finishSwap(bstring mover);
    void endSwap(int id=1);
    bool moveRoomRestrictedArea(bstring area) const;
    bool moveObjectRestricted(CatRef cr) const;
    void swapEmptyQueue();
    void swapNextInQueue();
    void swapAddQueue(Swap s);
    bool inSwapQueue(CatRef origin, SwapType type, bool checkTarget=false);
    int swapQueueSize();
    bool isSwapping() const;
    void setMovingRoom(CatRef o, CatRef t);
    void swapInfo(const Player* player);
    void swapAbort();
    bool checkSpecialArea(CatRef origin, CatRef target, int (CatRefInfo::*toCheck), Player* player, bool online, bstring type);
    bool swapChecks(const Player* player, Swap s);
    bool swapIsInteresting(const MudObject* target) const;

// Double Logging
    void addDoubleLog(bstring forum1, bstring forum2);
    void remDoubleLog(bstring forum1, bstring forum2);
    bool canDoubleLog(bstring forum1, bstring forum2) const;
    bool loadDoubleLog();
    void saveDoubleLog() const;
    void listDoubleLog(const Player* viewer) const;

// Misc
    const RaceData* getRace(bstring race) const;
    const RaceData* getRace(int id) const;
    const DeityData* getDeity(int id) const;

    unsigned long expNeeded(int level);
    int getMaxSong();

    bstring getVersion();
    bstring getMudName();
    bstring getMudNameAndVersion();
    short getPortNum() const;
    void setPortNum(short pPort);
    bstring weatherize(WeatherString w, const BaseRoom* room) const;
    bstring getMonthDay() const;
    bool isAprilFools() const;
    bool willAprilFools() const;
    int getFlashPolicyPort() const;
    bool startFlashPolicy() const;
    bool sendTxtOnCrash() const;
    void toggleTxtOnCrash();
    int getShopNumObjects() const;
    int getShopNumLines() const;

    void showMemory(Socket* sock);
    void killMortalObjects();

    bstring getSpecialFlag(int index);

// Get
    const cWeather* getWeather() const;

    bstring getDmPass() const;
    bstring getWebserver() const;
    bstring getQS() const;
    bstring getUserAgent() const;
    bstring getReviewer() const;

    bstring getCustomColor(CustomColor i, bool caret) const;

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
    bstring getLotteryRunTimeStr();
    time_t getLotteryRunTime();
    int getLotteryTicketsSold();

protected:
    Config();   // Can only be instantiated by itself -- Instance only

    void reset(bool reload=false);

    void loadGeneral(xmlNodePtr rootNode);
    void loadLottery(xmlNodePtr rootNode);
    void loadTickets(xmlNodePtr rootNode);

    void clearSkills();
    void updateSkillPointers();
    void loadSkillGroups(xmlNodePtr rootNode);
    void loadSkillGroup(xmlNodePtr rootNode);
    void loadSkills(xmlNodePtr rootNode);
    void loadSkill(xmlNodePtr rootNode);


public:
    bool loadConfig(bool reload=false);
    bool saveConfig() const;
    bool loadSkills();

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
    bstring getDbConnectionString();
#endif

public:
    // Config Options
    bool    conjureDisabled;
    bool    lessExpLoss;
    bool    pkillInCombatDisabled;
    bool    autoShutdown;
    bool    checkDouble;
    bool    getHostByName;
    bool    recordAll;
    bool    logSuicide;
    bool    charCreationDisabled;
    int     portNum;
    short   minBroadcastLevel;
    bool    saveOnDrop;
    bool    logDeath;
    short   crashes;
    short   supportRequiredForGuild;
    int numGuilds;
    //int   maxGuilds;
    int nextGuildId;

    bstring mudName;
    bstring dmPass;
    bstring webserver;
    bstring qs;             // authorization query string
    bstring userAgent;
    bstring defaultArea;    // loaded from catrefinfo file

//#ifdef SQL_LOGGER
    bstring logDbType;
    bstring logDbUser;
    bstring logDbPass;
    bstring logDbDatabase;
//#endif

    // Lottery
private:
    std::list<bstring> swapList;
    std::list<Swap> swapQueue;
    Swap    currentSwap;
    
    std::list<LottoTicket*> tickets;
    bool    lotteryEnabled;
    int lotteryCycle;
    long    lotteryJackpot;
    bool    lotteryWon;
    long    lotteryWinnings; // Money made from the lottery since the last jackpot
    int lotteryTicketPrice;
    long    lotteryTicketsSold;
    time_t  lotteryRunTime;
    short   lotteryNumbers[6];
    bool    swapping;
    bool    roomSearchFailure;
    char    customColors[CUSTOM_COLOR_SIZE];
    bool    doAprilFools;
    bool    txtOnCrash;
    int flashPolicyPort;
    int shopNumObjects;
    int shopNumLines;
    bstring reviewer;

    std::list<Unique*> uniques;
    std::list<Lore*> lore;
    std::list<accountDouble> accountDoubleLog;

    ProxyManager* proxyManager;

    // Quests
public:
    std::map<int, QuestInfo*> quests;
    MxpElementMap mxpElements;
    BstringMap mxpColors;
    QuestInfo* getQuest(int questNum);

public:
    // Misc
    char        cmdline[256];

    // MSDP
    std::map<bstring, MsdpVariable*> msdpVariables;
    MsdpVariable* getMsdpVariable(bstring& name);


    // Alchemy
    AlchemyMap alchemy;

    // Bans
    std::list<Ban*> bans;

    // Effects
    std::map<bstring, Effect*> effects;

    // Commands
    PlyCommandMap staffCommands;
    PlyCommandMap playerCommands;
    CrtCommandMap generalCommands;

    SpellMap spells;
    SongMap songs;

    SocialMap socials;

    // All Skill Commands are SkillInfos, but not all SkillInfos are SkillCommands
    SkillCommandMap skillCommands;
    SkillInfoMap skills;

    // Guilds
    std::list<GuildCreation*> guildCreations;
    std::map<int, Guild*> guilds;

    // Factions
    std::map<bstring, Faction*> factions;

    std::map<bstring, bstring> skillGroups;
    std::map<bstring, PlayerClass*> classes;
    std::map<bstring, StartLoc*> start;
    std::map<bstring, Fishing> fishing;
    std::list<Property*> properties;
    std::list<CatRefInfo*> catRefInfo;

    std::map<int, RaceData*> races;
    std::map<int, DeityData*> deities;
    std::map<int, Recipe*> recipes;

    std::map<int, Clan*> clans;
    questPtr questTable[MAX_QUEST];

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


    Calendar    *calendar;
    std::list<Ship*> ships;

    std::list<Area*> areas;

public:
    // queue related functions
    void flushRoom();
    void flushMonster();
    void flushObject();

    bool monsterInQueue(const CatRef cr);
    void frontMonsterQueue(const CatRef cr);
    void addMonsterQueue(const CatRef cr, Monster** pMonster);
    void getMonsterQueue(const CatRef cr, Monster** pMonster);

    bool objectInQueue(const CatRef cr);
    void frontObjectQueue(const CatRef cr);
    void addObjectQueue(const CatRef cr, Object** pObject);
    void getObjectQueue(const CatRef cr, Object** pObject);

    bool roomInQueue(const CatRef cr);
    void frontRoomQueue(const CatRef cr);
    void addRoomQueue(const CatRef cr, UniqueRoom** pRoom);
    void delRoomQueue(const CatRef cr);
    void getRoomQueue(const CatRef cr, UniqueRoom** pRoom);

    bool reloadRoom(BaseRoom* room);
    int reloadRoom(CatRef cr);
    int resaveRoom(CatRef cr);
    int saveStorage(UniqueRoom* uRoom);
    int saveStorage(CatRef cr);
    void resaveAllRooms(char permonly);
    void replaceMonsterInQueue(CatRef cr, Monster *creature);
    void replaceObjectInQueue(CatRef cr, Object* object);

    int roomQueueSize();
    int monsterQueueSize();
    int objectQueueSize();

protected:
    void putQueue(qtag **qt, qtag **headptr, qtag **tailptr);
    void pullQueue(qtag **qt, qtag **headptr, qtag **tailptr);
    void frontQueue(qtag **qt, qtag **headptr, qtag **tailptr);
    void delQueue(qtag **qt, qtag **headptr, qtag **tailptr);


protected:
        // Queue header and tail pointers
    qtag *roomHead;
    qtag *roomTail;
    qtag *monsterHead;
    qtag *monsterTail;
    qtag *objectHead;
    qtag *objectTail;

    std::map<bstring, rsparse>  roomQueue;
    std::map<bstring, osparse>  objectQueue;
    std::map<bstring, msparse>  monsterQueue;
};


#endif
