/*
 * config.cpp
 *   Handles config options for the entire mud
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>

#include "calendar.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "factions.hpp"
#include "fishing.hpp"
#include "guilds.hpp"
#include "mud.hpp"
#include "msdp.hpp"
#include "proxy.hpp"
#include "rooms.hpp"
#include "xml.hpp"

// Globals
Config *gConfig = nullptr;

// Static initialization
Config* Config::myInstance = nullptr;

Config::Config() {
    reset();
    inUse = true;
    listing = false;
}

Config::~Config() {
    if(inUse)
        throw(std::runtime_error("Error, trying to destroy config\n"));
    else
        std::clog << "Properly deconstructing Config class";
}

//--------------------------------------------------------------------
// Instance Functions

//********************************************************************
// Get Instance - Return the static instance of config
//********************************************************************
Config* Config::getInstance() {
    if(myInstance == nullptr)
        myInstance = new Config;
    return(myInstance);
}
//********************************************************************
// Destroy Instance - Destroy the static instance
//********************************************************************
void Config::destroyInstance() {
    if(myInstance != nullptr) {
        myInstance->cleanUp();
        delete myInstance;
    }
    myInstance = nullptr;
}

// End - Instance Functions
//--------------------------------------------------------------------





//--------------------------------------------------------------------
// Memory Functions
void Config::cleanUp() {
    clearClans();
    clearGuildList();
    clearBanList();
    clearFactionList();
    clearSkills();
    calendar->clear();
    clearShips();
    clearClasses();
    clearRaces();
    clearRecipes();
    clearDeities();
    clearStartLoc();
    clearLimited();
    clearProperties();
    clearCatRefInfo();
    clearQuests();
    clearAlchemy();
    clearCommands();
    clearSocials();
    clearSpells();
    clearMxpElements();
    clearEffects();
    clearSongs();
    clearProxyAccess();
    clearMsdp();
    inUse = false;
    listing = false;
}
void Config::clearProxyAccess() {
    proxyManager->clear();
    proxyManager = 0;
}

//********************************************************************
// resetConfig
//********************************************************************
// Sets all config options to their default value.

void Config::reset(bool reload) {
    mudName = "Default Mud Name";

    saveOnDrop = true;
    checkDouble = true;
    getHostByName = true;
    logSuicide = true;

    logDeath = true;
    autoShutdown = false;
    doAprilFools = false;
    charCreationDisabled = false;
    lessExpLoss = false;
    pkillInCombatDisabled = false;
    recordAll = false;

    flashPolicyPort = 0;
    shopNumObjects = shopNumLines = 0;

    dmPass = "default_dm_pw";
    webserver = qs = userAgent = reviewer = "";

    if(!bHavePort)
        portNum = 3333;

    memset(customColors, CUSTOM_COLOR_DEFAULT, sizeof(customColors));
    customColors[CUSTOM_COLOR_SIZE-1] = 0;

    customColors[CUSTOM_COLOR_BROADCAST] = 'x';
    customColors[CUSTOM_COLOR_GOSSIP] = 'M';
    customColors[CUSTOM_COLOR_PTEST] = 'Y';
    customColors[CUSTOM_COLOR_NEWBIE] = 'M';
    customColors[CUSTOM_COLOR_DM] = 'g';
    customColors[CUSTOM_COLOR_ADMIN] = 'g';
    customColors[CUSTOM_COLOR_SEND] = 'Y';
    customColors[CUSTOM_COLOR_MESSAGE] = 'G';
    customColors[CUSTOM_COLOR_WATCHER] = 'C';
    customColors[CUSTOM_COLOR_CLASS] = 'y';
    customColors[CUSTOM_COLOR_RACE] = 'c';
    customColors[CUSTOM_COLOR_CLAN] = 'm';
    customColors[CUSTOM_COLOR_TELL] = 'x';
    customColors[CUSTOM_COLOR_GROUP] = 'g';
    customColors[CUSTOM_COLOR_DAMAGE] = 'm';
    customColors[CUSTOM_COLOR_SELF] = 'w';
    customColors[CUSTOM_COLOR_GUILD] = 'g';

    lotteryEnabled = true;
    lotteryTicketPrice = 100;
    lotteryTicketsSold = 0;
    lotteryJackpot = 500000;
    lotteryWon = 0;
    lotteryWinnings = 0;
    lotteryRunTime = 0;

    lotteryNumbers[0] =
    lotteryNumbers[1] =
    lotteryNumbers[2] =
    lotteryNumbers[3] =
    lotteryNumbers[4] =
    lotteryNumbers[5] = 0;
    lotteryCycle = 1;

    for(LottoTicket* ticket : tickets) {
        delete ticket;
    }

    logDbType="";
    logDbUser="";
    logDbPass="";
    logDbDatabase="";

    
    // the following settings aren't cleared on reload
    if(reload)
        return;


    
    conjureDisabled = false;

    // Numerical defaults
    crashes = 0;
    supportRequiredForGuild = 2;
    minBroadcastLevel = 2;
    //maxGuilds = 8;
    numGuilds = 0;

    calendar = nullptr;
    proxyManager = nullptr;

    tickets.clear();
    defaultArea = "misc";
    swapping = roomSearchFailure = txtOnCrash = false;
}


bool Config::loadBeforePython() {
    std::clog << "Checking Directories..." << (Path::checkPaths() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Initializing command table..." << (initCommands() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Initializing MSDP..." << (initMsdp() ? "done" : "*** FAILED ***") << std::endl;

    std::clog << "Loading Config..." << (loadConfig() ? "done" : "*** FAILED ***")<< std::endl;

    std::clog << "Loading Socials..." << (loadSocials() ? "done" : "*** FAILED ***") << std::endl;
    
    std::clog << "Loading Recipes..." << (loadRecipes() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Flags..." << (loadFlags() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Effects..." << (loadEffects() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Writing Help Files..." << (writeHelpFiles() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Spell List..." << (loadSpells() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Song List..." << (loadSongs() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Quest Table..." << (loadQuestTable() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading New Quests..." << (loadQuests() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading StartLocs..." << (loadStartLoc() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading CatRefInfo..." << (loadCatRefInfo() ? "done" : "*** FAILED ***") << std::endl;

    std::clog << "Loading Bans..." << (loadBans() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Fishing..." << (loadFishing() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Guilds..." << (loadGuilds() ? "done" : "*** FAILED ***") << std::endl;

    std::clog << "Loading Skills...";
    if(loadSkills())
        std::clog << "done." << std::endl;
    else {
        std::clog << "*** FAILED *** " << std::endl;
        exit(-15);
    }

    std::clog << "Loading Deities..." << (loadDeities() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Clans..." << (loadClans() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Classes..." << (loadClasses() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Races..." << (loadRaces() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Factions..." << (loadFactions() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Alchemy..." << (loadAlchemy() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading MXP Elements..." << (loadMxpElements() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Limited Items..." << (loadLimited() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Double Log Info..." << (loadDoubleLog() ? "done" : "*** FAILED ***") << std::endl;

    std::clog << "Loading Calendar...";
    loadCalendar();
    std::clog << "done.\n";

    std::clog << "Loading Proxy Access...";
    loadProxyAccess();
    std::clog << "done." << std::endl;

    return(true);
}

// These items depend on python so load them after python has been initialized
bool Config::loadAfterPython() {
    if(!listing)
        std::clog << "Loading Ships..." << (loadShips() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Properties..." << (loadProperties() ? "done" : "*** FAILED ***") << std::endl;
    return (true);
}
bool Config::startFlashPolicy() const {
    return(false);
    char script[256], policy[256], cmd[256];

    if(!flashPolicyPort) {
        broadcast(isDm, "^oNo flash policy port specified.");
        return(false);
    }
    if(flashPolicyPort == portNum) {
        broadcast(isDm, "^oFlash policy port equals main mud port.");
        return(false);
    }

    sprintf(script, "%s/flashpolicyd.py", Path::UniqueRoom);
    if(!file_exists(script)) {
        broadcast(isDm, "^oUnable to find flash policy server.");
        return(false);
    }

    sprintf(policy, "%s/flashpolicy.xml", Path::Config);
    if(!file_exists(policy)) {
        broadcast(isDm, "^oUnable to find flash policy file.");
        return(false);
    }

    if(!fork()) {
        sprintf(cmd, "python %s --file=%s --port=%d", script, policy, flashPolicyPort);
        system(cmd);
        exit(0);
    }
    return(true);
}

bool Config::loadConfig(bool reload) {
    char filename[256];
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr  curNode;

    sprintf(filename, "%s/config.xml", Path::Config);

    if(!file_exists(filename))
        return(false);

    if((xmlDoc = xml::loadFile(filename, "Config")) == nullptr)
        return(false);

    rootNode = xmlDocGetRootElement(xmlDoc);
    curNode = rootNode->children;

    // Reset current config
    reset(reload);
    while(curNode) {
             if(NODE_NAME(curNode, "General")) loadGeneral(curNode);
        else if(NODE_NAME(curNode, "Lottery")) loadLottery(curNode);

        curNode = curNode->next;
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();

    //gConfig->numGuilds = gConfig->numGuilds;
    return(true);
}

void Config::loadGeneral(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
             if(NODE_NAME(curNode, "AutoShutdown")) xml::copyToBool(autoShutdown, curNode);
        else if(NODE_NAME(curNode, "AprilFools")) xml::copyToBool(doAprilFools, curNode);
        else if(NODE_NAME(curNode, "FlashPolicyPort")) xml::copyToNum(flashPolicyPort, curNode);
        else if(NODE_NAME(curNode, "CharCreationDisabled")) xml::copyToBool(charCreationDisabled, curNode);
        else if(NODE_NAME(curNode, "CheckDouble")) xml::copyToBool(checkDouble, curNode);
        else if(NODE_NAME(curNode, "GetHostByName")) xml::copyToBool(getHostByName, curNode);
        else if(NODE_NAME(curNode, "LessExpLoss")) xml::copyToBool(lessExpLoss, curNode);
        else if(NODE_NAME(curNode, "LogDatabaseType")) xml::copyToBString(logDbType, curNode);
        else if(NODE_NAME(curNode, "LogDatabaseUser")) xml::copyToBString(logDbUser, curNode);
        else if(NODE_NAME(curNode, "LogDatabasePassword")) xml::copyToBString(logDbPass, curNode);
        else if(NODE_NAME(curNode, "LogDatabaseDatabase")) xml::copyToBString(logDbDatabase, curNode);
        else if(NODE_NAME(curNode, "LogDeath")) xml::copyToBool(logDeath, curNode);
        else if(NODE_NAME(curNode, "PkillInCombatDisabled")) xml::copyToBool(pkillInCombatDisabled, curNode);
        else if(NODE_NAME(curNode, "RecordAll")) xml::copyToBool(recordAll, curNode);
        else if(NODE_NAME(curNode, "LogSuicide")) xml::copyToBool(logSuicide, curNode);
        else if(NODE_NAME(curNode, "SaveOnDrop")) xml::copyToBool(saveOnDrop, curNode);
        //else if(NODE_NAME(curNode, "MaxGuild")) xml::copyToNum(maxGuilds, curNode);
        else if(NODE_NAME(curNode, "DmPass")) xml::copyToBString(dmPass, curNode);
        else if(NODE_NAME(curNode, "MudName")) xml::copyToBString(mudName, curNode);
        else if(NODE_NAME(curNode, "Webserver")) xml::copyToBString(webserver, curNode);
        else if(NODE_NAME(curNode, "QS")) { xml::copyToBString(qs, curNode);
            std::clog << "Loaded QS: " << qs << std::endl;
        }
        else if(NODE_NAME(curNode, "UserAgent")) xml::copyToBString(userAgent, curNode);
        else if(NODE_NAME(curNode, "Reviewer")) xml::copyToBString(reviewer, curNode);
        else if(NODE_NAME(curNode, "ShopNumObjects")) xml::copyToNum(shopNumObjects, curNode);
        else if(NODE_NAME(curNode, "ShopNumLines")) xml::copyToNum(shopNumLines, curNode);
        else if(NODE_NAME(curNode, "CustomColors")) xml::copyToCString(customColors, curNode);
        else if(!bHavePort && NODE_NAME(curNode, "Port")) xml::copyToNum(portNum, curNode);

        curNode = curNode->next;
    }
}


void Config::loadLottery(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
             if(NODE_NAME(curNode, "Enabled")) xml::copyToBool(lotteryEnabled, curNode);
        else if(NODE_NAME(curNode, "curNoderentCycle")) xml::copyToNum(lotteryCycle, curNode);
        else if(NODE_NAME(curNode, "curNoderentJackpot")) xml::copyToNum(lotteryJackpot, curNode);
        else if(NODE_NAME(curNode, "TicketPrice")) xml::copyToNum(lotteryTicketPrice, curNode);
        else if(NODE_NAME(curNode, "LotteryWon")) xml::copyToBool(lotteryWon, curNode);
        else if(NODE_NAME(curNode, "TicketsSold")) xml::copyToNum(lotteryTicketsSold, curNode);
        else if(NODE_NAME(curNode, "WinningsThisCycle")) xml::copyToNum(lotteryWinnings, curNode);
        else if(NODE_NAME(curNode, "LotteryRunTime")) xml::copyToNum(lotteryRunTime, curNode);

        else if(NODE_NAME(curNode, "WinningNumbers")) {
            xml::loadNumArray<short>(curNode, lotteryNumbers, "LotteryNum", 6);
        }
        else if(NODE_NAME(curNode, "Tickets")) {
            loadTickets(curNode);
        }

        curNode = curNode->next;
    }
}

void Config::loadTickets(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    LottoTicket* ticket=0;

    while(curNode) {
        if(NODE_NAME(curNode, "Ticket")) {
             if((ticket = new LottoTicket(curNode)) != nullptr) {
                 tickets.push_back(ticket);
             }
        }

        curNode = curNode->next;
    }
}

bool Config::save() const {
    saveConfig();
    return(true);
}

// Functions to get configured options
bool Config::saveConfig() const {
    xmlDocPtr   xmlDoc;
    xmlNodePtr      rootNode, curNode;
    char            filename[256];

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Config", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    // Make general section
    curNode = xmlNewChild(rootNode, nullptr, BAD_CAST "General", nullptr);
    if(!bHavePort)
        xml::saveNonZeroNum(curNode, "Port", portNum);

    xml::saveNonNullString(curNode, "MudName", mudName);
    xml::saveNonNullString(curNode, "DmPass", dmPass);
    xml::saveNonNullString(curNode, "Webserver", webserver);
    xml::saveNonNullString(curNode, "QS", qs);
    xml::saveNonNullString(curNode, "UserAgent", userAgent);
    xml::saveNonNullString(curNode, "CustomColors", customColors);
    xml::saveNonNullString(curNode, "Reviewer", reviewer);

    xml::newStringChild(curNode, "LogDatabaseType", logDbType);
    xml::newStringChild(curNode, "LogDatabaseUser", logDbUser);
    xml::newStringChild(curNode, "LogDatabasePassword", logDbPass);
    xml::newStringChild(curNode, "LogDatabaseDatabase", logDbDatabase);

    xml::newBoolChild(curNode, "AprilFools", doAprilFools);
    xml::saveNonZeroNum(curNode, "FlashPolicyPort", flashPolicyPort);
    xml::newBoolChild(curNode, "AutoShutdown", autoShutdown);
    xml::newBoolChild(curNode, "CharCreationDisabled", charCreationDisabled);
    xml::newBoolChild(curNode, "CheckDouble", checkDouble);
    xml::newBoolChild(curNode, "GetHostByName", getHostByName);
    xml::newBoolChild(curNode, "LessExpLoss", lessExpLoss);
    xml::newBoolChild(curNode, "LogDeath", logDeath);
    xml::newBoolChild(curNode, "PkillInCombatDisabled", pkillInCombatDisabled);
    xml::newBoolChild(curNode, "RecordAll", recordAll);
    xml::newBoolChild(curNode, "LogSuicide", logSuicide);
    xml::newBoolChild(curNode, "SaveOnDrop", saveOnDrop);
    //xml::newBoolChild(curNode, "MaxGuild", maxGuilds);

    xml::saveNonZeroNum(curNode, "ShopNumObjects", shopNumObjects);
    xml::saveNonZeroNum(curNode, "ShopNumLines", shopNumLines);

    // Lottery Section
    curNode = xmlNewChild(rootNode, nullptr, BAD_CAST "Lottery", nullptr);
    xml::saveNonZeroNum(curNode, "CurrentCycle", lotteryCycle);
    xml::saveNonZeroNum(curNode, "CurrentJackpot", lotteryJackpot);
    xml::newBoolChild(curNode, "Enabled", BAD_CAST iToYesNo(lotteryEnabled));
    xml::saveNonZeroNum(curNode, "TicketPrice", lotteryTicketPrice);
    xml::saveNonZeroNum(curNode, "TicketsSold", lotteryTicketsSold);
    xml::saveNonZeroNum(curNode, "LotteryWon", lotteryWon);
    xml::saveNonZeroNum(curNode, "WinningsThisCycle", lotteryWinnings);
    xml::saveNonZeroNum(curNode, "LotteryRunTime", lotteryRunTime);
    saveShortIntArray(curNode, "WinningNumbers", "LotteryNum", lotteryNumbers, 6);
    xmlNodePtr ticketsNode = xml::newStringChild(curNode, "Tickets");
    for(LottoTicket* ticket : tickets) {
        ticket->saveToXml(ticketsNode);
    }

    sprintf(filename, "%s/config.xml", Path::Config);
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);

    return(true);
}

short Config::getPortNum() const {
    return(portNum);
}
void Config::setPortNum(short pPort) {
    portNum = pPort;
}
int getPkillInCombatDisabled() {
    return(gConfig->pkillInCombatDisabled);
}

bstring Config::getMonthDay() const {
    long    t = time(0);
    bstring str = ctime(&t);
    return(str.substr(4,6));
}

unsigned long Config::expNeeded(int level) {
    if(level < 1)
        return(0);
    if(level > MAXALVL)
        return(MAXINT);
    return(needed_exp[level-1]);
}

bool Config::isAprilFools() const { return(doAprilFools && getMonthDay() == "Apr  1"); }
bool Config::willAprilFools() const { return(doAprilFools); }
int Config::getFlashPolicyPort() const { return(flashPolicyPort); }

bool Config::sendTxtOnCrash() const { return(txtOnCrash); }
void Config::toggleTxtOnCrash() { txtOnCrash = !txtOnCrash; }

int Config::getShopNumObjects() const { return(shopNumObjects ? shopNumObjects : 400); }
int Config::getShopNumLines() const { return(shopNumLines ? shopNumLines : 150); }

const cWeather* Config::getWeather() const { return(calendar->getCurSeason()->getWeather()); }

bstring Config::getDmPass() const { return(dmPass); }
bstring Config::getWebserver() const { return(webserver); }
bstring Config::getQS() const { return(qs); }
bstring Config::getUserAgent() const { return(userAgent); }
bstring Config::getReviewer() const { return(reviewer); }

#include "specials.hpp"

bstring Config::getSpecialFlag(int index) {
    if(index >= SA_MAX_FLAG || index <= SA_NO_FLAG )
        return("Invalid Flag");

    return(specialFlags[index].name);
}
#include "version.hpp"
bstring Config::getVersion() {
    return(VERSION);
}
bstring Config::getMudName() {
    return(mudName);
}

bstring Config::getMudNameAndVersion() {
    return(getMudName() + " v" + getVersion());
}
// End Config Functions
//--------------------------------------------------------------------

// **************
//   Save Lists
// **************

template<class Type>
bool saveList(bstring xmlDocName, bstring fName, const std::map<bstring, Type*, comp>& sMap) {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char        filename[80];

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST xmlDocName.c_str(), nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    for(std::pair<bstring, Type*> sp : sMap) {
        Type* curItem = sp.second;
        curItem->save(rootNode);
    }

    snprintf(filename, 80, "%s/%s", Path::Config, fName.c_str());
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
    return(true);

}

bool Config::saveSpells() const {
    return(saveList<Spell>("Spells", "spelllist.xml", spells));
}

bool Config::saveSongs() const {
    return(saveList<Song>("Songs", "songlist.xml", songs));
}

// **************
//   Load Lists
// **************

template<class Type>
bool loadList(bstring xmlDocName, bstring xmlNodeName, bstring fName, std::map<bstring, Type*, comp>& sMap) {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;

    char filename[80];
    snprintf(filename, 80, "%s/%s", Path::Code, fName.c_str());
    xmlDoc = xml::loadFile(filename, xmlDocName.c_str());
    if(xmlDoc == nullptr)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode))
        curNode = curNode->next;

    if(curNode == 0) {
        xmlFreeDoc(xmlDoc);
        return(false);
    }

    while(curNode != nullptr) {
        if(NODE_NAME(curNode, xmlNodeName.c_str())) {
            Type* curItem = new Type(curNode);
            sMap[curItem->getName()] = curItem;
        }
        curNode = curNode->next;
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

bool Config::loadEffects() {
    clearEffects();
    return(loadList<Effect>("Effects", "Effect", "effects.xml", effects));
}

bool Config::loadSpells() {
    clearSpells();
    return(loadList<Spell>("Spells", "Spell", "spelllist.xml", spells));
}

bool Config::loadSongs() {
    clearSongs();
    return(loadList<Song>("Songs", "Song", "songlist.xml", songs));
}


//**********************************************************************
//                      writeHelpFiles
//**********************************************************************
// commands are written in initCommands
// socials are written in initCommands
// flags are written in loadFlags

bool Config::writeHelpFiles() const {
    bool success = true;

    success &= writeSpellFiles();
    success &= writeSocialFile();


    return(success);
}

namespace Path {
    const char* Bin = "/home/realms/realms/bin/";
    const char* Log = "/home/realms/realms/log/";
    const char* BugLog = "/home/realms/realms/log/bug/";
    const char* StaffLog = "/home/realms/realms/log/staff/";
    const char* BankLog = "/home/realms/realms/log/bank/";
    const char* GuildBankLog = "/home/realms/realms/log/guildbank/";

    const char* UniqueRoom = "/home/realms/realms/rooms/";
    const char* AreaRoom = "/home/realms/realms/rooms/area/";
    const char* Monster = "/home/realms/realms/monsters/";
    const char* Object = "/home/realms/realms/objects/";
    const char* Player = "/home/realms/realms/player/";
    const char* PlayerBackup = "/home/realms/realms/player/backup/";

    const char* Config = "/home/realms/realms/config/";

    const char* Code = "/home/realms/realms/config/code/";
    const char* Python = "/home/realms/realms/config/code/python/";
    const char* Game = "/home/realms/realms/config/game/";
    const char* AreaData = "/home/realms/realms/config/game/area/";
    const char* Talk = "/home/realms/realms/config/game/talk/";
    const char* Desc = "/home/realms/realms/config/game/ddesc/";
    const char* Sign = "/home/realms/realms/config/game/signs/";

    const char* PlayerData = "/home/realms/realms/config/player/";
    const char* Bank = "/home/realms/realms/config/player/bank/";
    const char* GuildBank = "/home/realms/realms/config/player/guildbank/";
    const char* History = "/home/realms/realms/config/player/history/";
    const char* Post = "/home/realms/realms/config/player/post/";

    const char* BaseHelp = "/home/realms/realms/help/";
    const char* Help = "/home/realms/realms/help/help/";
    const char* CreateHelp = "/home/realms/realms/help/create/";
    const char* Wiki = "/home/realms/realms/help/wiki/";
    const char* DMHelp = "/home/realms/realms/help/dmhelp/";
    const char* BuilderHelp = "/home/realms/realms/help/bhelp/";
    const char* HelpTemplate = "/home/realms/realms/help/template/";
}
//*********************************************************************
//                      path functions
//*********************************************************************

char* objectPath(const CatRef cr) {
    static char filename[256];
    if(cr.id < 0)
        sprintf(filename, "%s/%s/", Path::Object, cr.area.c_str());
    else
        sprintf(filename, "%s/%s/o%05d.xml", Path::Object, cr.area.c_str(), cr.id);
    return(filename);
}
char* monsterPath(const CatRef cr) {
    static char filename[256];
    if(cr.id < 0)
        sprintf(filename, "%s/%s/", Path::Monster, cr.area.c_str());
    else
        sprintf(filename, "%s/%s/m%05d.xml", Path::Monster, cr.area.c_str(), cr.id);
    return(filename);
}
char* roomPath(const CatRef cr) {
    static char filename[256];
    if(cr.id < 0)
        sprintf(filename, "%s/%s/", Path::UniqueRoom, cr.area.c_str());
    else
        sprintf(filename, "%s/%s/r%05d.xml", Path::UniqueRoom, cr.area.c_str(), cr.id);
    return(filename);
}
char* roomBackupPath(const CatRef cr) {
    static char filename[256];
    if(cr.id < 0)
        sprintf(filename, "%s/%s/backup/", Path::UniqueRoom, cr.area.c_str());
    else
        sprintf(filename, "%s/%s/backup/r%05d.xml", Path::UniqueRoom, cr.area.c_str(), cr.id);
    return(filename);
}

//*********************************************************************
//                      checkPaths
//*********************************************************************

bool Path::checkPaths() {
    bool ok = true;

    ok = Path::checkDirExists(Path::Bin) && ok;
    ok = Path::checkDirExists(Path::Log) && ok;
    ok = Path::checkDirExists(Path::BugLog) && ok;
    ok = Path::checkDirExists(Path::StaffLog) && ok;
    ok = Path::checkDirExists(Path::BankLog) && ok;
    ok = Path::checkDirExists(Path::GuildBankLog) && ok;

    ok = Path::checkDirExists(Path::UniqueRoom) && ok;
    ok = Path::checkDirExists(Path::AreaRoom) && ok;
    ok = Path::checkDirExists(Path::Monster) && ok;
    ok = Path::checkDirExists(Path::Object) && ok;
    ok = Path::checkDirExists(Path::Player) && ok;
    ok = Path::checkDirExists(Path::PlayerBackup) && ok;

    ok = Path::checkDirExists(Path::Config) && ok;

    ok = Path::checkDirExists(Path::Code) && ok;
    ok = Path::checkDirExists(Path::Python) && ok;
    ok = Path::checkDirExists(Path::Game) && ok;
    ok = Path::checkDirExists(Path::AreaData) && ok;
    ok = Path::checkDirExists(Path::Talk) && ok;
    ok = Path::checkDirExists(Path::Desc) && ok;
    ok = Path::checkDirExists(Path::Sign) && ok;

    ok = Path::checkDirExists(Path::PlayerData) && ok;
    ok = Path::checkDirExists(Path::Bank) && ok;
    ok = Path::checkDirExists(Path::GuildBank) && ok;
    ok = Path::checkDirExists(Path::History) && ok;
    ok = Path::checkDirExists(Path::Post) && ok;

    ok = Path::checkDirExists(Path::BaseHelp) && ok;
    ok = Path::checkDirExists(Path::Help) && ok;
    ok = Path::checkDirExists(Path::Wiki) && ok;
    ok = Path::checkDirExists(Path::DMHelp) && ok;
    ok = Path::checkDirExists(Path::BuilderHelp) && ok;
    ok = Path::checkDirExists(Path::HelpTemplate) && ok;

    return(ok);
}

//*********************************************************************
//                      checkDirExists
//*********************************************************************

bool Path::checkDirExists(const char* filename) {
    struct stat     f_stat;
    if(stat(filename, &f_stat)) {
        return(!mkdir(filename, 0755));
    }
    return(true);
}

bool Path::checkDirExists(bstring area, char* (*fn)(const CatRef cr)) {
    char    filename[256];
    CatRef  cr;

    // this will trigger the dir-only mode
    cr.setArea(area.c_str());
    cr.id = -1;
    strcpy(filename, (*fn)(cr));

    return(checkDirExists(filename));
}


void Config::setListing(bool isListing) {
    listing = isListing;
}

bool Config::isListing() {
    return listing;
}


//********************************************************************
//                      resetShipsFile
//********************************************************************

void Config::resetShipsFile() {
    // copying ships.midnight.xml to ships.xml
    char sfile1[80], sfile2[80], command[255];
    sprintf(sfile1, "%s/ships.midnight.xml", Path::Game);
    sprintf(sfile2, "%s/ships.xml", Path::Game);

    sprintf(command, "cp %s %s", sfile1, sfile2);
    system(command);
}

bstring Config::getFlag(int flagNum, MudFlagMap& flagMap) {
    // Flags are offset by one
    auto flag = flagMap.find(flagNum + 1);
    if (flag == flagMap.end())
        return "Unknown";
    return flag->second.name;
}

