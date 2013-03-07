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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
//#include "os.h"
#include "mud.h"
#include "bans.h"
#include "factions.h"
#include "guilds.h"
#include "calendar.h"
#include "effects.h"
#include "msdp.h"

//#include "config.h"

extern bool listing;

// Globals
Config *gConfig = NULL;

// Static initialization
Config* Config::myInstance = NULL;

Config::Config() {
	reset();
	inUse = true;
}

Config::~Config() {
	if(inUse)
		throw(std::runtime_error("Error, trying to destroy config\n"));
	else
		std::cout << "Properly deconstructing Config class";
}

//--------------------------------------------------------------------
// Instance Functions

//********************************************************************
// Get Instance - Return the static instance of config
//********************************************************************
Config* Config::getInstance() {
	if(myInstance == NULL)
		myInstance = new Config;
	return(myInstance);
}
//********************************************************************
// Destroy Instance - Destroy the static instance
//********************************************************************
void Config::destroyInstance() {
	if(myInstance != NULL) {
		myInstance->cleanUp();
		delete myInstance;
	}
	myInstance = NULL;
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
	clearAreas();
	clearStartLoc();
	clearLimited();
	clearProperties();
	clearCatRefInfo();
	clearQuests();
	clearAlchemy();
	clearCommands();
	clearSocials();
    clearSpells();
	clearMsdpVariables();
	clearMxpElements();
	clearEffects();
	clearSongs();
	clearProxyAccess();
	inUse = false;
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


	
	roomHead =
	roomTail =
	monsterHead =
	monsterTail =
	objectHead =
	objectTail = 0;

	conjureDisabled = false;

	// Numerical defaults
	crashes = 0;
	supportRequiredForGuild = 2;
	minBroadcastLevel = 2;
	//maxGuilds = 8;
	numGuilds = 0;


	calendar = NULL;
	proxyManager = NULL;

	tickets.clear();
	defaultArea = "misc";
	swapping = roomSearchFailure = txtOnCrash = false;
}


bool Config::loadBeforePython() {
	printf("Checking Directories...%s.\n", Path::checkPaths() ? "done" : "*** FAILED ***");
	printf("Initializing command table...%s.\n", initCommands() ? "done" : "*** FAILED ***");

	printf("Loading Config...%s.\n", loadConfig() ? "done" : "*** FAILED ***");

	printf("Loading Socials...%s.\n", loadSocials() ? "done" : "*** FAILED ***");
	
	printf("Loading Recipes...%s.\n", loadRecipes() ? "done" : "*** FAILED ***");
	printf("Loading Flags...%s.\n", loadFlags() ? "done" : "*** FAILED ***");
    printf("Loading Effects...%s\n", loadEffects() ? "done" : "*** FAILED ***");
	printf("Writing Help Files...%s.\n", writeHelpFiles() ? "done" : "*** FAILED ***");
    printf("Loading Spell List...%s.\n", loadSpells() ? "done" : "*** FAILED ***");
    printf("Loading Song List...%s.\n", loadSongs() ? "done" : "*** FAILED ***");
	printf("Loading Quest Table...%s.\n", loadQuestTable() ? "done" : "*** FAILED ***");
	printf("Loading New Quests...%s.\n", loadQuests() ? "done" : "*** FAILED ***");
	printf("Loading StartLocs...%s.\n", loadStartLoc() ? "done" : "*** FAILED ***");
	printf("Loading CatRefInfo...%s.\n", loadCatRefInfo() ? "done" : "*** FAILED ***");

	printf("Loading Bans...%s.\n", loadBans() ? "done" : "*** FAILED ***");
	printf("Loading Fishing...%s.\n", loadFishing() ? "done" : "*** FAILED ***");
	printf("Loading Guilds...%s.\n", loadGuilds() ? "done" : "*** FAILED ***");

	std::cout << "Loading Skills...";
	if(loadSkills())
		std::cout << "done." << std::endl;
	else {
		std::cout << "*** FAILED *** " << std::endl;
		exit(-15);
	}

	printf("Loading Deities...%s.\n", loadDeities() ? "done" : "*** FAILED ***");
	printf("Loading Clans...%s.\n", loadClans() ? "done" : "*** FAILED ***");
	printf("Loading Classes...%s.\n", loadClasses() ? "done" : "*** FAILED ***");
	printf("Loading Races...%s.\n", loadRaces() ? "done" : "*** FAILED ***");
	printf("Loading Factions...%s.\n", loadFactions() ? "done" : "*** FAILED ***");
	printf("Loading Alchemy...%s.\n", loadAlchemy() ? "done" : "*** FAILED ***");
	printf("Loading MSDP Variables...%s.\n", loadMsdpVariables() ? "done" : "*** FAILED ***");
	printf("Loading MXP Elements...%s.\n", loadMxpElements() ? "done" : "*** FAILED ***");
	printf("Loading Limited Items...%s.\n", loadLimited() ? "done" : "*** FAILED ***");
	printf("Loading Double Log Info...%s.\n", loadDoubleLog() ? "done" : "*** FAILED ***");

	printf("Loading Calendar...");
	loadCalendar();
	printf("done.\n");

	std::cout << "Loading Proxy Access...";
	loadProxyAccess();
	std::cout << "done." << std::endl;

	return(true);
}

// These items depend on python so load them after python has been initialized
bool Config::loadAfterPython() {
	printf("Loading Areas...%s.\n", loadAreas() ? "done" : "*** FAILED ***");
	if(!listing)
		printf("Loading Ships...%s.\n", loadShips() ? "done" : "*** FAILED ***");
	printf("Loading Properties...%s.\n", loadProperties() ? "done" : "*** FAILED ***");
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
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	xmlNodePtr	curNode;

	sprintf(filename, "%s/config.xml", Path::Config);

	if(!file_exists(filename))
		return(false);

	if((xmlDoc = xml::loadFile(filename, "Config")) == NULL)
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
			std::cout << "Loaded QS: " << qs << std::endl;
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
			 if((ticket = new LottoTicket(curNode)) != NULL) {
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
	xmlDocPtr	xmlDoc;
	xmlNodePtr		rootNode, curNode;
	char			filename[256];

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Config", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	// Make general section
	curNode = xmlNewChild(rootNode, NULL, BAD_CAST "General", NULL);
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
	curNode = xmlNewChild(rootNode, NULL, BAD_CAST "Lottery", NULL);
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

int Config::getPortNum() const {
	return(portNum);
}
void Config::setPortNum(int pPort) {
	portNum = pPort;
}
int getPkillInCombatDisabled() {
	return(gConfig->pkillInCombatDisabled);
}

bstring Config::getMonthDay() const {
	long	t = time(0);
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

#include "specials.h"

bstring Config::getSpecialFlag(int index) {
	if(index >= SA_MAX_FLAG || index <= SA_NO_FLAG )
		return("Invalid Flag");

	return(specialFlags[index].name);
}
#include "version.h"
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
bool saveList(bstring xmlDocName, bstring fName, const std::map<bstring, Type*>& sMap) {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	char		filename[80];

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST xmlDocName.c_str(), NULL);
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
bool loadList(bstring xmlDocName, bstring xmlNodeName, bstring fName, std::map<bstring, Type*>& sMap) {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;

    char filename[80];
    snprintf(filename, 80, "%s/%s", Path::Code, fName.c_str());
	xmlDoc = xml::loadFile(filename, xmlDocName.c_str());
	if(xmlDoc == NULL)
		return(false);

	curNode = xmlDocGetRootElement(xmlDoc);

	curNode = curNode->children;
	while(curNode && xmlIsBlankNode(curNode))
		curNode = curNode->next;

	if(curNode == 0) {
		xmlFreeDoc(xmlDoc);
		return(false);
	}

	while(curNode != NULL) {
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
//						writeHelpFiles
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
//						path functions
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
//						checkPaths
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
//						checkDirExists
//*********************************************************************

bool Path::checkDirExists(const char* filename) {
	struct stat     f_stat;
	if(stat(filename, &f_stat)) {
		return(!mkdir(filename, 0755));
	}
	return(true);
}

bool Path::checkDirExists(bstring area, char* (*fn)(const CatRef cr)) {
	char	filename[256];
	CatRef	cr;

	// this will trigger the dir-only mode
	cr.setArea(area.c_str());
	cr.id = -1;
	strcpy(filename, (*fn)(cr));

	return(checkDirExists(filename));
}




//*********************************************************************
//						reloadRoom
//*********************************************************************
// This function reloads a room from disk, if it's already loaded. This
// allows you to make changes to a room, and then reload it, even if it's
// already in the memory room queue.

bool Config::reloadRoom(BaseRoom* room) {
	UniqueRoom*	uRoom = room->getAsUniqueRoom();

	if(uRoom) {
		bstring str = uRoom->info.str();
		if(	roomQueue.find(str) != roomQueue.end() &&
			roomQueue[str].rom &&
			reloadRoom(uRoom->info) >= 0
		) {
			roomQueue[str].rom->addPermCrt();
			return(true);
		}
	} else {

		AreaRoom* aRoom = room->getAsAreaRoom();
		char	filename[256];
		sprintf(filename, "%s/%d/%s", Path::AreaRoom, aRoom->area->id,
			aRoom->mapmarker.filename().c_str());

		if(file_exists(filename)) {
			xmlDocPtr	xmlDoc;
			xmlNodePtr	rootNode;

			if((xmlDoc = xml::loadFile(filename, "AreaRoom")) == NULL)
				merror("Unable to read arearoom file", FATAL);

			rootNode = xmlDocGetRootElement(xmlDoc);

			Area *area = aRoom->area;
			aRoom->reset();
			aRoom->area = area;
			aRoom->load(rootNode);
			return(true);
		}
	}
	return(false);
}

int Config::reloadRoom(CatRef cr) {
	UniqueRoom	*room=0;

	bstring str = cr.str();
	if(roomQueue.find(str) == roomQueue.end())
		return(0);
	if(!roomQueue[str].rom)
		return(0);

//	roomQueue[str].rom->purge();

	if(!loadRoomFromFile(cr, &room))
		return(-1);
	// Move any current players & monsters into the new room
	for(Player* ply : roomQueue[str].rom->players) {
		room->players.insert(ply);
		ply->setParent(room);
	}
	roomQueue[str].rom->players.clear();

	if(room->monsters.empty()) {
		for(Monster* mons : roomQueue[str].rom->monsters) {
			room->monsters.insert(mons);
			mons->setParent(room);
		}
		roomQueue[str].rom->monsters.clear();
	}
	if(room->objects.empty()) {
		for(Object* obj : roomQueue[str].rom->objects) {
			room->objects.insert(obj);
			obj->setParent(room);
		}
		roomQueue[str].rom->objects.clear();
	}


	delete roomQueue[str].rom;
	roomQueue[str].rom = room;

	room->registerMo();

	return(0);
}

//*********************************************************************
//						resaveRoom
//*********************************************************************
// This function saves an already-loaded room back to memory without
// altering its position on the queue.

int saveRoomToFile(UniqueRoom* pRoom, int permOnly);

int Config::resaveRoom(CatRef cr) {
	bstring str = cr.str();
	if(roomQueue[str].rom)
		roomQueue[str].rom->saveToFile(ALLITEMS);
	return(0);
}


int Config::saveStorage(UniqueRoom* uRoom) {
	if(uRoom->flagIsSet(R_SHOP))
		saveStorage(shopStorageRoom(uRoom));
	return(saveStorage(uRoom->info));
}
int Config::saveStorage(CatRef cr) {
	bstring str = cr.str();
	if(!roomQueue[str].rom)
		return(0);

	if(roomQueue[str].rom->saveToFile(ALLITEMS) < 0)
		return(-1);

	return(0);
}

//********************************************************************
//						resaveAllRooms
//********************************************************************
// This function saves all memory-resident rooms back to disk.  If the
// permonly parameter is non-zero, then only permanent items in those
// rooms are saved back.

void Config::resaveAllRooms(char permonly) {
	qtag 	*qt = roomHead;
	while(qt) {
		if(roomQueue[qt->str].rom) {
			if(roomQueue[qt->str].rom->saveToFile(permonly) < 0)
				return;
		}
		qt = qt->next;
	}
}

//*********************************************************************
//						replaceMonsterInQueue
//*********************************************************************
void Config::replaceMonsterInQueue(CatRef cr, Monster *monster) {
	if(this->monsterInQueue(cr))
		*monsterQueue[cr.str()].mob = *monster;
	else
		addMonsterQueue(cr, &monster);
}

//*********************************************************************
//						replaceObjectInQueue
//*********************************************************************
void Config::replaceObjectInQueue(CatRef cr, Object* object) {
	if(objectInQueue(cr))
		*objectQueue[cr.str()].obj = *object;
	else
		addObjectQueue(cr, &object);
}
