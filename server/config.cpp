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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */


#include <fmt/format.h>        // for format
#include <sys/stat.h>          // for mkdir, stat
#include <cstdio>              // for sprintf
#include <cstdlib>             // for exit, system
#include <cstring>             // for memset, strcpy
#include <ctime>               // for ctime, time
#include <iostream>            // for std::clog
#include <list>                // for list, operator==, _List_iterator, _Lis...
#include <map>                 // for operator==, _Rb_tree_iterator, _Rb_tre...
#include <ostream>             // for operator<<, endl, basic_ostream, ostream
#include <stdexcept>           // for runtime_error
#include <string>              // for string, allocator, operator==
#include <utility>             // for pair

#include "alchemy.hpp"         // for AlchemyInfo
#include "calendar.hpp"        // for Calendar, cSeason, cWeather (ptr only)
#include "catRef.hpp"          // for CatRef
#include "config.hpp"          // for Config, accountDouble, MudFlagMap, Dis...
#include "effects.hpp"         // for Effect
#include "fishing.hpp"         // for Fishing
#include "global.hpp"          // for CUSTOM_COLOR_ADMIN, CUSTOM_COLOR_BROAD...
#include "mud.hpp"             // for MAXINT
#include "mxp.hpp"             // for MxpElement
#include "paths.hpp"           // for checkDirExists, checkPaths
#include "proxy.hpp"           // for ProxyManager
#include "ships.hpp"           // for Ship
#include "skills.hpp"          // for SkillInfo
#include "skillCommand.hpp"    // for SkillCommand
#include "socials.hpp"         // for SocialCommand
#include "specials.hpp"        // for SA_MAX_FLAG, SA_NO_FLAG
#include "structs.hpp"         // for MudFlag
#include "swap.hpp"            // for Swap
#include "zone.hpp"
#include "version.hpp"         // for VERSION


// Globals
Config *gConfig = nullptr;

// Static initialization
Config* Config::myInstance = nullptr;

unsigned long Config::needed_exp[] = {
    //2   3  4   5   6   7    8   9   10
    500, 1000, 2000, 4000, 8000, 16000, 32000, 64000, 100000,
    // 500  1k   2k   4k     8k 16k 32k   36k
    //11      12      13      14      15      16       17
    160000, 270000, 390000, 560000, 750000, 1000000, 1300000,
    // 60k   110k   120k   170k 190k    250k     300k
    //18      19        20     21      22      23      24
    1700000, 2200000, 2800000, 3500000, 4300000, 5300000, 6500000,
    //400k   500k   600k     700k    800k    1mil    1.5mil
    // 25      26     27        28        29       30        31
    8000000, 10000000, 12200000, 14700000, 17500000, 21500000, 25700000,
    // 32       33     34      35        36     37      38
    30100000, 34700000, 39600000, 44800000, 50300000, 56200000, 62500000,
    // 39       40      41
    69200000, 76200000, 2000000000

    };

Config::Config() {
    reset();
    inUse = true;
    listing = false;
}

Config::~Config() noexcept(false) {
    if(inUse)
        throw(std::runtime_error("Error, trying to destroy config\n"));
    else
        std::clog << "Properly deconstructing Config class\n";
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
    ships.clear();
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
    clearWebhookTokens();
    inUse = false;
    listing = false;
}
void Config::clearProxyAccess() {
    proxyManager->clear();
    proxyManager = nullptr;
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
    customColors[CUSTOM_COLOR_SPORTS] = 'c';

    lotteryEnabled = true;
    lotteryTicketPrice = 100;
    lotteryTicketsSold = 0;
    lotteryJackpot = 500000;
    lotteryWon = false;
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
    botToken="";
    
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
    if(!isListing()) {
        std::clog << "Loading Discord Config..." << (loadDiscordConfig() ? "done" : "*** FAILED ***") << std::endl;
    }
    std::clog << "Loading Zones..." << (loadZones() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Socials..." << (loadSocials() ? "done" : "*** FAILED ***") << std::endl;
    
    std::clog << "Loading Recipes..." << (loadRecipes() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Flags..." << (loadFlags() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Effects..." << (loadEffects() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Writing Help Files..." << (writeHelpFiles() ? "done" : "*** FAILED ***") << std::endl;
//    std::clog << "Loading Spell List..." << (loadSpells() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Song List..." << (loadSongs() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Quest Table..." << (loadQuestTable() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading New Quests..." << (loadQuests() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading StartLocs..." << (loadStartLoc() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading CatRefInfo..." << (loadCatRefInfo() ? "done" : "*** FAILED ***") << std::endl;

    std::clog << "Loading Bans..." << (loadBans() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Fishing..." << (loadFishing() ? "done" : "*** FAILED ***") << std::endl;
    std::clog << "Loading Guilds..." << (loadGuilds() ? "done" : "*** FAILED ***") << std::endl;

    std::clog << "Loading Skills...";
    if(loadSkillGroups() && loadSkills())
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


bool Config::save() const {
    saveConfig();
    return(true);
}



short Config::getPortNum() const {
    return(portNum);
}
void Config::setPortNum(short pPort) {
    portNum = pPort;
    bHavePort = true;
}

int Config::getPkillInCombatDisabled() {
    return(pkillInCombatDisabled);
}

std::string Config::getMonthDay() const {
    long    t = time(nullptr);
    std::string str = ctime(&t);
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

std::string Config::getDmPass() const { return(dmPass); }
std::string Config::getWebserver() const { return(webserver); }
std::string Config::getQS() const { return(qs); }
std::string Config::getUserAgent() const { return(userAgent); }
std::string Config::getReviewer() const { return(reviewer); }


std::string Config::getSpecialFlag(int index) {
    if(index >= SA_MAX_FLAG || index <= SA_NO_FLAG )
        return("Invalid Flag");

    return(specialFlags[index].name);
}

std::string Config::getVersion() {
    return(VERSION);
}
std::string Config::getMudName() {
    return(mudName);
}

std::string Config::getMudNameAndVersion() {
    return(fmt::format("{} v{}", getMudName(), getVersion()));
}
// End Config Functions
//--------------------------------------------------------------------


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

//*********************************************************************
//                      path functions
//*********************************************************************

fs::path Path::objectPath(const CatRef& cr) {
    auto path = Path::Object / cr.area;
    if(cr.id < 0)
        return path;
    return (path / fmt::format("o{:05d}", cr.id)).replace_extension("xml");
}
fs::path Path::monsterPath(const CatRef& cr) {
    auto path = Path::Monster / cr.area;
    if(cr.id < 0)
        return path;
    return (path / fmt::format("m{:05d}", cr.id)).replace_extension("xml");
}
fs::path Path::roomPath(const CatRef& cr) {
    auto path = Path::UniqueRoom / cr.area;
    if(cr.id < 0)
        return path;
    return (path / fmt::format("r{:05d}", cr.id)).replace_extension("xml");
}
fs::path Path::roomBackupPath(const CatRef& cr) {
    auto path = Path::UniqueRoom / cr.area / "backup";
    if(cr.id < 0)
        return path;
    return (path / fmt::format("r{:05d}", cr.id)).replace_extension("xml");
}

//*********************************************************************
//                      checkPaths
//*********************************************************************

bool Path::checkPaths() {
    fs::create_directory(Path::Bin);
    fs::create_directory(Path::Log);
    fs::create_directory(Path::BugLog);
    fs::create_directory(Path::StaffLog);
    fs::create_directory(Path::BankLog);
    fs::create_directory(Path::GuildBankLog);

    fs::create_directory(Path::UniqueRoom);
    fs::create_directory(Path::AreaRoom);
    fs::create_directory(Path::Monster);
    fs::create_directory(Path::Object);
    fs::create_directory(Path::Player);
    fs::create_directory(Path::PlayerBackup);

    fs::create_directory(Path::Config);

    fs::create_directory(Path::Code);
//    fs::create_directory(Path::Python);
    fs::create_directory(Path::Game);
    fs::create_directory(Path::AreaData);
    fs::create_directory(Path::Talk);
    fs::create_directory(Path::Desc);
    fs::create_directory(Path::Sign);

    fs::create_directory(Path::PlayerData);
    fs::create_directory(Path::Bank);
    fs::create_directory(Path::GuildBank);
    fs::create_directory(Path::History);
    fs::create_directory(Path::Post);

    fs::create_directory(Path::BaseHelp);
    fs::create_directory(Path::Help);
    fs::create_directory(Path::Wiki);
    fs::create_directory(Path::DMHelp);
    fs::create_directory(Path::BuilderHelp);
    fs::create_directory(Path::HelpTemplate);

    return(true);
}

//*********************************************************************
//                      checkDirExists
//*********************************************************************

bool Path::checkDirExists(const fs::path& path) {
    if(!fs::exists(path)) {
        return(fs::create_directory(path));
    }
    return(true);
}

bool Path::checkDirExists(const std::string &area, fs::path (*fn)(const CatRef&)) {
    CatRef  cr;

    // this will trigger the dir-only mode
    cr.setArea(area);
    cr.id = -1;

    return(checkDirExists((*fn)(cr)));
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
    auto sfile1 = Path::Game / "%s/ships.midnight.xml";
    auto sfile2 = Path::Game / "%s/ships.xml";

    fs::copy(sfile1, sfile2, fs::copy_options::overwrite_existing);
}

std::string Config::getFlag(int flagNum, MudFlagMap& flagMap) {
    // Flags are offset by one
    auto flag = flagMap.find(flagNum + 1);
    if (flag == flagMap.end())
        return "Unknown";
    return flag->second.name;
}

const std::string &Config::getDefaultArea() const {
    return defaultArea;
}

void Config::setDefaultArea(const std::string &pDefaultArea) {
    Config::defaultArea = pDefaultArea;
}

bool Config::hasPort() const {
    return bHavePort;
}

int Config::getNextGuildId() const {
    return nextGuildId;
}

void Config::setNextGuildId(int pNextGuildId) {
    Config::nextGuildId = pNextGuildId;
}

bool Config::getCheckDouble() const {
    return checkDouble;
}

int Config::getMaxDouble() const {
    return maxDouble;
}

void Config::setNumGuilds(int guildId) {
    numGuilds = std::max(numGuilds, guildId);
}

const std::string &Config::getBotToken() const {
    return botToken;
}

const std::string &Config::getWebhookToken(long webhookID) const {
    return webhookTokens.at(webhookID);
}

bool Config::isBotEnabled() const {
    return botEnabled;
}
