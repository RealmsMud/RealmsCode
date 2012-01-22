/*
 * cmd.cpp
 *   Handle player/pet input commands.
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
#include "mud.h"
#include "bstring.h"
#include "import.h"
#include "commands.h"
#include "dm.h"
#include <dirent.h>
#include "ships.h"
#include "unique.h"
#include "web.h"
#include "server.h"
#include "math.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <iostream>
#include <fstream>
#include <boost/tokenizer.hpp>



int dmTest(Player* player, cmd* cmnd) {
    *player << ColorOn << "^rhi" << ColorOff << "\n";
    *player << player << " " << player << "\n";
    int i = 0;
    for(Monster* pet : player->pets ){
        *player << bstring(++i) << ") " << pet << " " << setf(CAP) << setn(10) << pet << "\n";
    }
	return(0);
}


int pcast(Player* player, cmd* cmnd) {
	int retVal = 0;

	const Spell* spell = gConfig->getSpell(cmnd->str[1], retVal);

	if(retVal == CMD_NOT_UNIQUE) {
		player->print("The command \"%s\" does not exist.\n", cmnd->str[0]);
		return(0);
	} else if(retVal == CMD_NOT_UNIQUE ) {
		player->print("Song name is not unique.\n");
		return(0);
	}

	if(!spell) {
		player->print("Error loading spell \"%s\"\n", cmnd->str[1]);
		return(0);
	}

	bstring args = getFullstrText(cmnd->fullstr, 2);
	MudObject *target = NULL;

	//target = player->getRoom()->findTarget(cmnd->fullstr);

	gServer->runPython(spell->script, args, player, target);
	return(0);
}




//**********************************************************************
//						compileSocialList
//**********************************************************************

template<class Type>
std::map<bstring,bstring> compileSocialList(std::map<bstring, Type>& cList) {
	std::map<bstring,bstring> list;
	bstring name = "";

	for(std::pair<bstring, Type> pp : cList) {
		ASSERTLOG(pp.second);
		Command* cmnd = pp.second;

		CrtCommand* cc = dynamic_cast<CrtCommand*>(cmnd);
		PlyCommand* pc = dynamic_cast<PlyCommand*>(cmnd);

		if(	(!cc || (int(*)(Creature*, cmd*))cc->fn != cmdAction) &&
			(!pc || (int(*)(Player*, cmd*))pc->fn != plyAction)
		)
			continue;

		name = cmnd->getName();

		if(name == "vomjom" || name == "usagi" || name == "defenestrate")
			continue;

		list[name] = "";
	}

	return(list);
}

//**********************************************************************
//						writeSocialFile
//**********************************************************************
// Updates helpfiles for the game's socials.

void writeSocialFile() {
	char	file[100], fileLink[100];
	std::map<bstring,bstring> list;
	std::map<bstring,bstring> list2;
	std::map<bstring,bstring>::iterator it;

	sprintf(file, "%s/socials.txt", Path::Help);
	sprintf(fileLink, "%s/social.txt", Path::Help);

	// prepare to write the help file
	std::ofstream out(file);
	out.setf(std::ios::left, std::ios::adjustfield);
	out.imbue(std::locale(""));

	out << loadHelpTemplate("socials");


	list = compileSocialList<PlyCommand*>(gConfig->playerCommands);
	list2 = compileSocialList<CrtCommand*>(gConfig->generalCommands);

	// combine the lists
	for(it = list2.begin(); it != list2.end(); it++) {
		list[(*it).first] = (*it).second;
	}


	int i = 0;
	for(it = list.begin(); it != list.end(); it++) {
		if(i && !(i%6))
			out << "\n";
		out << std::setw(13) << (*it).first;
		i++;
	}

	out << "\n";
	

	out << loadHelpTemplate("socials.post");

	out.close();
	link(file, fileLink);
}

//**********************************************************************
//						compileCommandList
//**********************************************************************
// Updates helpfiles for the various game commands.

template<class Type>
std::map<bstring,bstring> compileCommandList(int cls, std::map<bstring, Type>& cList) {
	std::map<bstring,bstring> list;
	char	line[100];

	for(std::pair<bstring, Type> pp : cList) {
		ASSERTLOG(pp.second);
		Command* cmd = pp.second;
		
		if(cmd->getDesc() == "")
			continue;
		if(	cls == BUILDER &&
			cmd->auth &&
			(bool(*)(const Creature*))cmd->auth != builderMob &&
			(bool(*)(const Creature*))cmd->auth != builderObj
		)
			continue;

		sprintf(line, " ^W%-19s^x %s\n", cmd->getName().c_str(), cmd->getDesc().c_str());
		list[cmd->getName()] = line;
	}

	return(list);
}

//**********************************************************************
//						writeCommandFile
//**********************************************************************
// Updates helpfiles for the various game commands.

void writeCommandFile(int cls, const char* path, const char* tpl) {
	char	file[100], fileLink[100];
	std::map<bstring,bstring> list;
	std::map<bstring,bstring> list2;
	std::map<bstring,bstring>::iterator it;

	sprintf(file, "%s/commands.txt", path);
	sprintf(fileLink, "%s/command.txt", path);

	// prepare to write the help file
	std::ofstream out(file);
	out.setf(std::ios::left, std::ios::adjustfield);
	out.imbue(std::locale(""));

	out << loadHelpTemplate(tpl);


	if(cls == DUNGEONMASTER || cls == BUILDER) {
		list = compileCommandList<PlyCommand*>(cls, gConfig->staffCommands);
	} else {
		list = compileCommandList<PlyCommand*>(0, gConfig->playerCommands);
		list2 = compileCommandList<CrtCommand*>(0, gConfig->generalCommands);
	}

	// combine the lists
	for(it = list2.begin(); it != list2.end(); it++) {
		list[(*it).first] = (*it).second;
	}

	for(it = list.begin(); it != list.end(); it++) {
		out << (*it).second;
	}

	out << "\n";
	out.close();
	link(file, fileLink);
}


//**********************************************************************
//						initCommands
//**********************************************************************
// everything on the staff array already checks isStaff - the listed
// authorization is an additional requirement to use the command.

bool Config::initCommands() {
// *************************************************************************************
// Staff Commands

	staffCommands["play"] = new PlyCommand("play", 			100,	cmdPlay,			isCt,		"Play a song.");
	staffCommands["*cache"] = new PlyCommand("*cache",				100,	dmCache,			isDm,	"Show DNS cache.");
	staffCommands["*test"] = new PlyCommand("*test",				100,	dmTest,				isDm,	"");
	staffCommands["pcast"] = new PlyCommand("pcast",				100,	pcast,				isDm,	"");

	// channels
//	staffCommands["*s"] = new PlyCommand("*s",						100,	channel,			isCt,	"");
	staffCommands["*send"] = new PlyCommand("*send",				 10,	channel,			isCt,	"");
	staffCommands["dm"] = new PlyCommand("dm",						100,	channel,			isDm,	"");
	staffCommands["admin"] = new PlyCommand("admin",				100,	channel,			isDm,	"");
	staffCommands["dmrace"] = new PlyCommand("dmrace",				100,	channel,			isCt,	"Use specified race channel.");
	staffCommands["dmclass"] = new PlyCommand("dmclass",			100,	channel,			isCt,	"Use specified class channel.");
	staffCommands["dmcls"] = new PlyCommand("dmcls",				100,	channel,			isCt,	"");
	staffCommands["dmclan"] = new PlyCommand("dmclan",				100,	channel,			isCt,	"Use specified clan channel.");
	staffCommands["dmguild"] = new PlyCommand("dmguild",			100,	channel,			isCt,	"Use specified guild channel.");
	staffCommands["*msg"] = new PlyCommand("*msg",					100,	channel,			0,		"");

	// dm.c
	staffCommands["*reboot"] = new PlyCommand("*reboot",			100,	dmReboot,			isCt,	"Reboot the mud.");
	staffCommands["*inv"] = new PlyCommand("*inv",					100,	dmMobInventory,		0,		"");
	staffCommands["*sockets"] = new PlyCommand("*sockets",			100,	dmSockets,			isDm,	"Show all connected sockets.");
	staffCommands["*dmload"] = new PlyCommand("*dmload",			100,	dmLoad,				isDm,	"Reload configuration files.");
	staffCommands["*txtoncrash"] = new PlyCommand("*txtoncrash",	100,	dmTxtOnCrash,		isDm,	"Toggle whether the mud sends a text message when it crashes.");
	staffCommands["*dmsave"] = new PlyCommand("*dmsave",			100,	dmSave,				isDm,	"Resave configuration files.");
	staffCommands["*teleport"] = new PlyCommand("*teleport",		 10,	dmTeleport,			0,		"Teleport to rooms or players, or teleport players to each other.");
	//staffCommands["*t"] = new PlyCommand("*t",						100,	dmTeleport,			0,		"");
	staffCommands["*users"] = new PlyCommand("*users",				 10,	dmUsers,			isCt,	"See a list of connected users.");
	//staffCommands["*u"] = new PlyCommand("*u",						100,	dmUsers,			isCt,	"");
	staffCommands["*shutdown"] = new PlyCommand("*shutdown",		100,	dmShutdown,			isDm,	"Shut down the mud (may auto-restart).");
	staffCommands["*flushrooms"] = new PlyCommand("*flushrooms",	100,	dmFlushSave,		isDm,	"Flush rooms from memory.");
	staffCommands["*flushcrtobj"] = new PlyCommand("*flushcrtobj",	100,	dmFlushCrtObj,		0,		"Flush creatures and objects from memory.");
	staffCommands["*save"] = new PlyCommand("*save",				100,	dmResave,			0,		"Save the room, creature, or object.");
	staffCommands["*perm"] = new PlyCommand("*perm",				100,	dmPerm,				0,		"");
	//staffCommands["*i"] = new PlyCommand("*i",						100,	dmInvis,			0,		"");
	staffCommands["*invis"] = new PlyCommand("*invis",				 10,	dmInvis,			0,		"Toggle DM invis (only staff can see you).");
	staffCommands["*incog"] = new PlyCommand("*incog",				 20,	dmIncog,			0,		"");
	staffCommands["incog"] = new PlyCommand("incog",				100,	dmIncog,			0,		"Toggle incognito status (only people in the room can see you).");
	staffCommands["*ac"] = new PlyCommand("*ac",					100,	dmAc,				0,		"Show your Weapon/Defense/Armor and completely heal yourself.");
	staffCommands["*wipe"] = new PlyCommand("*wipe",				100,	dmWipe,				0,		"");
	staffCommands["*clear"] = new PlyCommand("*clear",				100,	dmDeleteDb,			isDm,	"");
	staffCommands["*gamestat"] = new PlyCommand("*gamestat",		100,	dmGameStatus,		isDm,	"Show configuration options.");
	staffCommands["*weather"] = new PlyCommand("*weather",			100,	dmWeather,			0,		"Show weather strings.");
	staffCommands["*questlist"] = new PlyCommand("*questlist",		100,	dmQuestList,		isCt,	"Show quests in memory.");
	staffCommands["*alchemylist"] = new PlyCommand("*alchemylist",	100,	dmAlchemyList,		isCt,	"Show the Alchemy List.");
	staffCommands["*clanlist"] = new PlyCommand("*clanlist",		100,	dmClanList,			isCt,	"Show clans in memory.");
	staffCommands["*bane"] = new PlyCommand("*bane",				500,	dmBane,				isDm,	"Crash the game");
	staffCommands["*dmhelp"] = new PlyCommand("*dmhelp",			 50,	dmHelp,				isCt,	"Access help files.");
	staffCommands["*bhelp"] = new PlyCommand("*bhelp",				 50,	bhHelp,				0,		"Access help files.");
	staffCommands["*fishing"] = new PlyCommand("*fishing",			100,	dmFishing,			0,		"View fishing lists.");
	staffCommands["*parameter"] = new PlyCommand("*parameter",		100,	dmParam,			isDm,	"");
	staffCommands["*outlaw"] = new PlyCommand("*outlaw",			100,	dmOutlaw,			isCt,	"Make a player an outlaw.");
	staffCommands["*broadcast"] = new PlyCommand("*broadcast",		 20,	dmBroadecho,		isCt,	"Broadcast a message to all players.");
	staffCommands["*gcast"] = new PlyCommand("*gcast",				100,	dmCast,				isCt,	"Cast a spell on all players.");
	staffCommands["*set"] = new PlyCommand("*set",					100,	dmSet,				0,		"Modify a room/object/exit/player/object/monster.");
	staffCommands["*log"] = new PlyCommand("*log",					100,	dmLog,				isCt,	"");
	staffCommands["*list"] = new PlyCommand("*list",				100,	dmList,				isCt,	"");
	staffCommands["*info"] = new PlyCommand("*info",				100,	dmInfo,				isCt,	"Show game info (includes some memory).");
	staffCommands["*md5"] = new PlyCommand("*md5",					100,	dmMd5,				isCt,	"Show md5 of input string.");
	staffCommands["*ids"] = new PlyCommand("*ids",					100,	dmIds,				isDm,	"Shows registered ids.");
	staffCommands["*status"] = new PlyCommand("*status",			 80,	dmStat,				0,		"Show info about a room/player/object/monster.");


	// dmcrt.c
	staffCommands["*monster"] = new PlyCommand("*monster",			100,	dmCreateMob,		builderMob,	"Summon a monster.");
	staffCommands["*cname"] = new PlyCommand("*cname",				100,	dmCrtName,			builderMob,	"Rename a monster.");
	staffCommands["*possess"] = new PlyCommand("*possess",			100,	dmAlias,			isCt,	"Possess a monster.");
	staffCommands["*cfollow"] = new PlyCommand("*cfollow",			100,	dmFollow,			isDm,	"Make a monster follow you.");
	staffCommands["*attack"] = new PlyCommand("*attack",			100,	dmAttack,			builderMob,	"Make a monster attack a player/monster.");
	staffCommands["*enemy"] = new PlyCommand("*enemy",				100,	dmListEnemy,		isCt,	"Show the enemies of a monster.");
	staffCommands["*charm"] = new PlyCommand("*charm",				100,	dmListCharm,		isCt,	"Show the charm list of a player.");
	staffCommands["*wander"] = new PlyCommand("*wander",			100,	dmForceWander,		builderMob,	"Force a monster to wander away.");
	staffCommands["*balance"] = new PlyCommand("*balance",			 95,	dmBalance,			builderMob,	"Balance a monster to a certain level.");


	// dmobj.c
	staffCommands["*create"] = new PlyCommand("*create",			 20,	dmCreateObj,		builderObj,	"Summon an object.");
	staffCommands["*object"] = new PlyCommand("*object",			 20,	dmCreateObj,		builderObj,	"Summon an object.");
	staffCommands["*oname"] = new PlyCommand("*oname",				100,	dmObjName,			builderObj,	"Rename an object");
	staffCommands["*size"] = new PlyCommand("*size",				100,	dmSize,				builderObj,	"");
	staffCommands["*clone"] = new PlyCommand("*clone",				100,	dmClone,			builderObj,	"Create an armor set from a single piece of armor.");


	// dmply.c
	staffCommands["*force"] = new PlyCommand("*force",				 20,	dmForce,			isDm,	"Force a player to perform an action.");
	staffCommands["*spy"] = new PlyCommand("*spy",					100,	dmSpy,				isCt,	"Spy on a player.");
	staffCommands["*silence"] = new PlyCommand("*silence",			100,	dmSilence,			isCt,	"Toggle broadcast silence on a player.");
	staffCommands["*title"] = new PlyCommand("*title",				100,	dmTitle,			isCt,	"Set a title on a player.");
	staffCommands["*surname"] = new PlyCommand("*surname",			100,	dmSurname,			isCt,	"Set a surname on a player.");
	staffCommands["*group"] = new PlyCommand("*group",				100,	dmGroup,			isCt,	"See who is grouped with a player.");
	staffCommands["*dust"] = new PlyCommand("*dust",				100,	dmDust,				isCt,	"Permanently delete a player.");
	staffCommands["*tell"] = new PlyCommand("*tell",				100,	dmFlash,			isDm,	"Send a player a message.");
	staffCommands["*award"] = new PlyCommand("*award",				100,	dmAward,			isCt,	"Award a player roleplaying exp/gold.");
	staffCommands["*beep"] = new PlyCommand("*beep",				100,	dmBeep,				isDm,	"Make the player's terminal beep.");
	staffCommands["*advance"] = new PlyCommand("*advance",			100,	dmAdvance,			isDm,	"Change the player's level and adjust stats appropriately.");
	staffCommands["dmfinger"] = new PlyCommand("dmfinger",			100,	dmFinger,			isDm,	"fup an offline player's information.");
	staffCommands["*disconnect"] = new PlyCommand("*disconnect",	100,	dmDisconnect,		isCt,	"Disconnect a player.");
	staffCommands["*take"] = new PlyCommand("*take",				100,	dmTake,				isCt,	"Take an item from a player.");
	staffCommands["*remove"] = new PlyCommand("*remove",			100,	dmRemove,			isCt,	"Unequip and take an item from a player.");
	staffCommands["*put"] = new PlyCommand("*put",					100,	dmPut,				isCt,	"Put an item into a player's inventory.");
	staffCommands["*move"] = new PlyCommand("*move",				100,	dmMove,				isCt,	"Move and offline player.");
	staffCommands["*word"] = new PlyCommand("*word",				100,	dmWordAll,			isDm,	"Send all players to their recall room.");
	staffCommands["*pass"] = new PlyCommand("*pass",				100,	dmPassword,			isDm,	"Change a player's password.");
	staffCommands["*restore"] = new PlyCommand("*restore",			100,	dmRestorePlayer,	isDm,	"Restore a player's experience.");
	staffCommands["*bank"] = new PlyCommand("*bank",				100,	dmBank,				isCt,	"");
	staffCommands["*assets"] = new PlyCommand("*assets",			100,	dmInventoryValue,	isCt,	"");
	staffCommands["*proxy"] = new PlyCommand("*proxy",				100,	dmProxy,			isCt,	"Allow a player to double log.");
	staffCommands["*2x"] = new PlyCommand("*2x",					100,	dm2x,				isCt,	"Allow two accounts to double log.");
	staffCommands["*bug"] = new PlyCommand("*bug",					100,	dmBugPlayer,		isCt,	"Record a player's actions.");
	staffCommands["*kill"] = new PlyCommand("*kill",				100,	dmKillSwitch,		isDm,	"Kill a player.");
	staffCommands["*nomnom"] = new PlyCommand("*nomnom",			100,	dmKillSwitch,		isDm,	"Kill a player.");
	staffCommands["*combust"] = new PlyCommand("*combust",			100,	dmKillSwitch,		isDm,	"Kill a player.");
	staffCommands["*slime"] = new PlyCommand("*slime",				100,	dmKillSwitch,		isDm,	"Kill a player.");
	staffCommands["*gnat"] = new PlyCommand("*gnat",				100,	dmKillSwitch,		isDm,	"Kill a player.");
	staffCommands["*trip"] = new PlyCommand("*trip",				100,	dmKillSwitch,		isDm,	"Kill a player.");
	staffCommands["*crush"] = new PlyCommand("*crush",				100,	dmKillSwitch,		isDm,	"Kill a player.");
	staffCommands["*missile"] = new PlyCommand("*missile",			100,	dmKillSwitch,		isDm,	"Kill a player.");
	staffCommands["*bomb"] = new PlyCommand("*bomb",				100,	dmKillSwitch,		isDm,	"Kill a player and everyone in room.");
	staffCommands["*nuke"] = new PlyCommand("*nuke",				100,	dmKillSwitch,		isDm,	"Kill a player and everyone in room.");
	staffCommands["*arma"] = new PlyCommand("*arma",				100,	dmKillSwitch,		isDm,	"");
	staffCommands["*armageddon"] = new PlyCommand("*armageddon",	100,	dmKillSwitch,		isDm,	"Kill all players.");
	staffCommands["*supernova"] = new PlyCommand("*supernova",		100,	dmKillSwitch,		isDm,	"Kill all players.");
	staffCommands["*igmoo"] = new PlyCommand("*igmoo",				100,	dmKillSwitch,		isDm,	"Kill all players.");
	staffCommands["*rape"] = new PlyCommand("*rape",				100,	dmKillSwitch,		isDm,	"Set a player to 1hp and 1mp.");
	staffCommands["*drain"] = new PlyCommand("*drain",				100,	dmKillSwitch,		isDm,	"Set a player to 1hp and 1mp.");
	staffCommands["*repair"] = new PlyCommand("*repair",			100,	dmRepair,			isDm,	"Fix a player's armor.");
	staffCommands["*max"] = new PlyCommand("*max",					100,	dmMax,				0,		"Set your stats to max.");
	staffCommands["*backup"] = new PlyCommand("*backup",			100,	dmBackupPlayer,		isDm,	"Backup a player or restore a player from backup.");
	staffCommands["*changestats"] = new PlyCommand("*changestats",	100,	dmChangeStats,		isCt,	"Allow a player to pick new stats.");
	staffCommands["*lt"] = new PlyCommand("*lt",					100,	dmLts,				isDm,	"Show a player's last times.");
	staffCommands["*ltclear"] = new PlyCommand("*ltclear",			100,	dmLtClear,			isDm,	"Clear a player's last times.");


	// dmroom.c
	staffCommands["*swap"] = new PlyCommand("*swap",				100,	dmSwap,				0,		"Swap utility.");
	staffCommands["*rswap"] = new PlyCommand("*rswap",				100,	dmRoomSwap,			0,		"Swap current room with another.");
	staffCommands["*oswap"] = new PlyCommand("*oswap",				100,	dmObjSwap,			0,		"Swap object with another.");
	staffCommands["*mswap"] = new PlyCommand("*mswap",				100,	dmMobSwap,			0,		"Swap monster with another.");
	staffCommands["*purge"] = new PlyCommand("*purge",				100,	dmPurge,			0,		"Purge all objects and monsters (not pets) in room.");
	staffCommands["*echo"] = new PlyCommand("*echo",				100,	dmEcho,				0,		"Send a message to all players in the room.");
	staffCommands["*reload"] = new PlyCommand("*reload",			100,	dmReloadRoom,		0,		"Reload the room from disk.");
	staffCommands["*reset"] = new PlyCommand("*reset",				100,	dmResetPerms,		0,		"Reset perm timeouts.");
	staffCommands["*add"] = new PlyCommand("*add",					100,	dmAddRoom,			0,		"Create a new room/object/monster.");
	staffCommands["*replace"] = new PlyCommand("*replace",			100,	dmReplace,			0,		"Replace text in a room description.");
	staffCommands["*substitute"] = new PlyCommand("*substitute",	100,	dmReplace,			0,		"");
	staffCommands["*delete"] = new PlyCommand("*delete",			100,	dmDelete,			0,		"Delete text from a room description.");
	staffCommands["*name"] = new PlyCommand("*name",				100,	dmNameRoom,			0,		"Rename the room.");
	staffCommands["*append"] = new PlyCommand("*append",			100,	dmAppend,			0,		"Add text to a room description.");
	staffCommands["*app"] = new PlyCommand("*app",					100,	dmAppend,			0,		"");
	staffCommands["*prepend"] = new PlyCommand("*prepend",			100,	dmPrepend,			0,		"Prepend text to a room description.");
	staffCommands["*moblist"] = new PlyCommand("*moblist",			100,	dmMobList,			builderMob,	"List wandering monsters in the room.");
	staffCommands["*wrap"] = new PlyCommand("*wrap",				100,	dmWrap,				0,		"Wrap the room description to a number of characters.");
	staffCommands["*xdelete"] = new PlyCommand("*xdelete",			100,	dmDeleteAllExits,	0,		"Delete all exits in the room.");
	staffCommands["*xarrange"] = new PlyCommand("*xarrange",		100,	dmArrangeExits,		0,		"Arrange all the exits in the room according to standards.");
	staffCommands["*fixup"] = new PlyCommand("*fixup",				100,	dmFixExit,			0,		"Remove an underscore from an exit.");
	staffCommands["*unfixup"] = new PlyCommand("*unfixup",			100,	dmUnfixExit,		0,		"Add an underscore to an exit.");
	staffCommands["*xrename"] = new PlyCommand("*xrename",			100,	dmRenameExit,		0,		"Rename an exit.");
	staffCommands["*destroy"] = new PlyCommand("*destroy",			100,	dmDestroyRoom,		0,		"Destroy the room.");
	staffCommands["*find"] = new PlyCommand("*find",				100,	dmFind,				0,		"Find the next available room/object/monster.");


	// post.c
	staffCommands["*readmail"] = new PlyCommand("*readmail",		100,	dmReadmail,			isDm,	"Read a player's mail.");
	staffCommands["*rmmail"] = new PlyCommand("*rmmail",			100,	dmDeletemail,		isDm,	"Delete a player's mail.");
	staffCommands["*notepad"] = new PlyCommand("*notepad",			100,	notepad,			0,		"Access your staff notepad.");
	staffCommands["*rmhist"] = new PlyCommand("*rmhist",			100,	dmDeletehist,		isDm,	"Delete a player's history.");


	// skills.c
	staffCommands["*skills"] = new PlyCommand("*skills",			100,	dmSkills,			isDm,	"Show a list of skills / a player/monsters's skills.");
	staffCommands["*setskills"] = new PlyCommand("*setskills",		100,	dmSetSkills,		isDm,	"Set a player's skills to an appropriate level.");
//	staffCommands["*addskills"] = new PlyCommand("*addskills",		100,	dmAddSkills			0,		"");


	// guilds.c
	staffCommands["*guildlist"] = new PlyCommand("*guildlist",		100,	dmListGuilds,		isDm,	"Show all guilds.");
	staffCommands["*approve"] = new PlyCommand("*approve",			100,	dmApproveGuild,		isCt,	"Approve a proposed guild.");
	staffCommands["*reject"] = new PlyCommand("*reject",			100,	dmRejectGuild,		isCt,	"Reject a proposed guild.");


	// bans.c
	staffCommands["*ban"] = new PlyCommand("*ban",					 60,	dmBan,				isCt,	"Ban a player.");
	staffCommands["*unban"] = new PlyCommand("*unban",				100,	dmUnban,			isCt,	"Delete a ban.");
	staffCommands["*banlist"] = new PlyCommand("*banlist",			100,	dmListbans,			isDm,	"Show a list of bans.");


	// various files
	staffCommands["*fifo"] = new PlyCommand("*fifo",				100,	dmFifo,				0,		"Delete and recreate the web interface fifos.");
	staffCommands["*locations"] = new PlyCommand("*locations",		100,	dmStartLocs,		isCt,	"Show starting locations.");
	staffCommands["*specials"] = new PlyCommand("*specials",		100,	dmSpecials,			0,		"Show a monster's special attacks.");
	staffCommands["*range"] = new PlyCommand("*range",				100,	dmRange,			0,		"Show your assigned room range.");
	staffCommands["*builder"] = new PlyCommand("*builder",			100,	dmMakeBuilder,		isCt,	"Make a new character a builder.");
	staffCommands["*wat"] = new PlyCommand("*wat",					100,	dmWatcherBroad,		isCt,	"Force a watcher to broadcast.");
	staffCommands["*demographics"] = new PlyCommand("*demographics",100,	cmdDemographics,	isDm,	"Start the demographics routine.");
	staffCommands["*unique"] = new PlyCommand("*unique",			100,	dmUnique,			isCt,	"Work with uniques.");
	staffCommands["*shipquery"] = new PlyCommand("*shipquery",		100,	dmQueryShips,		isDm,	"See data about ships.");
	staffCommands["*spelling"] = new PlyCommand("*spelling",		 50,	dmSpelling,			0,		"Spell-check a room.");
	staffCommands["*spells"] = new PlyCommand("*spells",			100,	dmSpellList,		0,		"List spells in the game.");
	staffCommands["*effects"] = new PlyCommand("*effects",			90,		dmEffectList,		0,		"List effects in the game.");
	staffCommands["*effindex"] = new PlyCommand("*effindex",		100,	dmShowEffectsIndex,	isCt,	"List rooms in the effects index.");
	staffCommands["*songs"] = new PlyCommand("*songs",				100,	dmSongList,			0,		"List songs in the game.");
	staffCommands["*lottery"] = new PlyCommand("*lottery",			100,	dmLottery,			isDm,	"Run the lottery.");
	staffCommands["*memory"] = new PlyCommand("*memory",			100,	dmMemory,			isCt,	"Show memory usage.");
	staffCommands["*active"] = new PlyCommand("*active",			100,	list_act,			isCt,	"Show monsters on the active list.");
	staffCommands["*classlist"] = new PlyCommand("*classlist",		100,	dmShowClasses,		0,		"List all classes.");
	staffCommands["*racelist"] = new PlyCommand("*racelist",		100,	dmShowRaces,		0,		"List all races.");
	staffCommands["*deitylist"] = new PlyCommand("*deitylist",		100,	dmShowDeities,		0,		"List all deities.");
	staffCommands["*faction"] = new PlyCommand("*faction",			100,	dmShowFactions,		0,		"List all factions or show a player/monster's faction.");
	staffCommands["bfart"] = new PlyCommand("bfart",				100,	plyAction,			isDm,	"");
	staffCommands["*arealist"] = new PlyCommand("*arealist",		100,	dmListArea,			isCt,	"List area information.");
	staffCommands["*recipes"] = new PlyCommand("*recipes",			100,	dmRecipes,			0,		"List recipes.");
	staffCommands["*combine"] = new PlyCommand("*combine",			100,	dmCombine,			0,		"Create new recipes.");
	staffCommands["*property"] = new PlyCommand("*property",		100,	dmProperties,		isCt,	"List properties.");
	staffCommands["*catrefinfo"] = new PlyCommand("*catrefinfo",	100,	dmCatRefInfo,		0,		"Show CatRefInfo.");

	// unused functions
	//staffCommands["*task"] = new PlyCommand("*task",			100,	dmTask,				0,		"");
	//staffCommands["boards"] = new PlyCommand("boards",			100,	listboards,			0,		"");
	//staffCommands["bug"] = new PlyCommand("bug",				100,	bugReport,			0,		"");
	//staffCommands["petition"] = new PlyCommand("petition",		100,	plyPetition,		0,		"");
	//staffCommands["*message"] = new PlyCommand("*message",		100,	dmMsg,				0,		"");
	//staffCommands["gamble"] = new PlyCommand("gamble",			100,	cmdGamble,			0,		"");
	//staffCommands["*wlog"] = new PlyCommand("*wlog",			100,	watcher_log,		0,		"");

// *************************************************************************************
// Player Commands

	playerCommands["target"] = new PlyCommand("target",				100,	cmdTarget,			0,		"Target a creature or player." );
	playerCommands["assist"] = new PlyCommand("assist",				100,	cmdAssist,			0,		"Assist a player, targeting their target." );
	// watcher functions
	playerCommands["*rename"] = new PlyCommand("*rename",			100,	dmRename,			0,		"");
	playerCommands["*warn"] = new PlyCommand("*warn",				100,	dmWarn,				0,		"");
	playerCommands["*jail"] = new PlyCommand("*jail",				100,	dmJailPlayer,		0,		"");
	playerCommands["*checkstats"] = new PlyCommand("*checkstats",	100,	dmCheckStats,		0,		"");
	playerCommands["*locate"] = new PlyCommand("*locate",			100,	dmLocatePlayer,		0,		"");
	playerCommands["*typo"] = new PlyCommand("*typo",				100,	reportTypo,			0,		"");
	playerCommands["*gag"] = new PlyCommand("*gag",					100,	dmGag,				0,		"");
	playerCommands["*wts"] = new PlyCommand("*wts",					100,	channel,			0,		"");

	playerCommands["weapons"] = new PlyCommand("weapons",			100,	cmdWeapons,			0,		"Use weapon trains.");
	playerCommands["fish"] = new PlyCommand("fish",					100,	cmdFish,			0,		"Go fishing.");
	playerCommands["religion"] = new PlyCommand("religion",			100,	cmdReligion,		0,		"");
	playerCommands["craft"] = new PlyCommand("craft",				100,	cmdCraft,			0,		"Create new items.");
	playerCommands["cook"] = new PlyCommand("cook",					100,	cmdCraft,			0,		"Create new items.");
	playerCommands["smith"] = new PlyCommand("smith",				100,	cmdCraft,			0,		"Create new items.");
	playerCommands["tailor"] = new PlyCommand("tailor",				100,	cmdCraft,			0,		"Create new items.");
	playerCommands["carpenter"] = new PlyCommand("carpenter",		100,	cmdCraft,			0,		"");
//	playerCommands["alchemy"] = new PlyCommand("alchemy",			100,	cmdCraft,			0,		"");
//	playerCommands["brew"] = new PlyCommand("brew",					100,	cmdCraft,			0,		"");
	playerCommands["prepare"] = new PlyCommand("prepare",			100,	cmdPrepare,			0,		"Prepare for traps or prepare an item for crafting.");
	playerCommands["unprepare"] = new PlyCommand("unprepare",		100,	cmdUnprepareObject,	0,		"Unprepare an item.");
	playerCommands["combine"] = new PlyCommand("combine",			100,	cmdCraft,			0,		"Create new items.");


	// player functions
	playerCommands["forum"] = new PlyCommand("forum",				100,	cmdForum,			0,		"Information on your forum account.");
	playerCommands["brew"] = new PlyCommand("brew",					100,	cmdBrew,			0,		"");
	playerCommands["house"] = new PlyCommand("house",				100,	cmdHouse,			0,		"");
	playerCommands["property"] = new PlyCommand("property",			100,	cmdProperties,		0,		"Manage your properties.");
	playerCommands["endurance"] = new PlyCommand("endurance",		100,	cmdEndurance,		0,		"");
	playerCommands["title"] = new PlyCommand("title",				100,	cmdTitle,			0,		"Choose a custom title.");
	playerCommands["defecate"] = new PlyCommand("defecate",			 80,	plyAction,			0,		"");
	playerCommands["tnl"] = new PlyCommand("tnl",					100,	plyAction,			0,		"Show the room how much experience you have to level.");
	playerCommands["mark"] = new PlyCommand("mark",					100,	plyAction,			0,		"");
	playerCommands["beckon"] = new PlyCommand("beckon",				100,	plyAction,			0,		"");
	playerCommands["show"] = new PlyCommand("show",					100,	plyAction,			0,		"");
	playerCommands["dream"] = new PlyCommand("dream",				100,	plyAction,			0,		"");
	playerCommands["rollover"] = new PlyCommand("rollover",			100,	plyAction,			0,		"");
	playerCommands["where"] = new PlyCommand("where",				100,	cmdLook,			0,		"");
	playerCommands["look"] = new PlyCommand("look",					 10,	cmdLook,			0,		"");
	playerCommands["consider"] = new PlyCommand("consider",			 50,	cmdLook,			0,		"");
	playerCommands["examine"] = new PlyCommand("examine",			100,	cmdLook,			0,		"");
	playerCommands["reconnect"] = new PlyCommand("reconnect",		100,	cmdReconnect,		0,		"Log in as another character.");
	playerCommands["relog"] = new PlyCommand("relog",				100,	cmdReconnect,		0,		"");
	playerCommands["quit"] = new PlyCommand("quit",					110,	cmdQuit,			0,		"Log off.");
	playerCommands["goodbye"] = new PlyCommand("goodbye",			100,	cmdQuit,			0,		"");
	playerCommands["logout"] = new PlyCommand("logout",				100,	cmdQuit,			0,		"");
	playerCommands["inventory"] = new PlyCommand("inventory",		 10,	cmdInventory,		0,		"View your inventory.");
//	playerCommands["inv"] = new PlyCommand("inv",					100,	cmdInventory,		0,		"");
//	playerCommands["i"] = new PlyCommand("i",						100,	cmdInventory,		0,		"");
	playerCommands["who"] = new PlyCommand("who",					 15,	cmdWho,				0,		"See who is online.");
	playerCommands["scan"] = new PlyCommand("scan",					100,	cmdWho,				0,		"");
	playerCommands["classwho"] = new PlyCommand("classwho",			 90,	cmdClasswho,		0,		"");
	playerCommands["whois"] = new PlyCommand("whois",				100,	cmdWhois,			0,		"View information about a specific character.");
	playerCommands["wear"] = new PlyCommand("wear",					100,	cmdWear,			0,		"");
	playerCommands["remove"] = new PlyCommand("remove",				100,	cmdRemoveObj,		0,		"");
	playerCommands["rm"] = new PlyCommand("rm",						110,	cmdRemoveObj,		0,		"");
	playerCommands["equipment"] = new PlyCommand("equipment",		100,	cmdEquipment,		0,		"View the equipment you are currently wearing.");
	playerCommands["hold"] = new PlyCommand("hold",					100,	cmdHold,			0,		"");
	playerCommands["wield"] = new PlyCommand("wield",				100,	cmdReady,			0,		"");
	playerCommands["ready"] = new PlyCommand("ready",				100,	cmdReady,			0,		"");
	playerCommands["second"] = new PlyCommand("second",				100,	cmdSecond,			0,		"");
	playerCommands["help"] = new PlyCommand("help",					100,	cmdHelp,			0,		"");
	playerCommands["?"] = new PlyCommand("?",						100,	cmdHelp,			0,		"");
	playerCommands["wiki"] = new PlyCommand("wiki",					100,	cmdWiki,			0,		"Look up a wiki entry.");
	playerCommands["health"] = new PlyCommand("health",				100,	cmdScore,			0,		"");
	playerCommands["score"] = new PlyCommand("score",				 50,	cmdScore,			0,		"Show brief information about your character.");
	playerCommands["status"] = new PlyCommand("status",				100,	cmdInfo,			0,		"");
	playerCommands["stats"] = new PlyCommand("stats",				100,	cmdStatistics,		0,		"");
	playerCommands["statistics"] = new PlyCommand("statistics",		100,	cmdStatistics,		0,		"Show character-related statistics.");
	playerCommands["information"] = new PlyCommand("information",	 50,	cmdInfo,			0,		"Show extended information about your character.");
	playerCommands["attributes"] = new PlyCommand("attributes",		100,	cmdInfo,			0,		"");
	playerCommands["skills"] = new PlyCommand("skills",				100,	cmdSkills,			0,		"Show what skills your character knows");
	playerCommands["version"] = new PlyCommand("version",			100,	cmdVersion,			0,		"");
	playerCommands["age"] = new PlyCommand("age",					100,	cmdAge,				0,		"Show your character's age and time played.");
	playerCommands["birthday"] = new PlyCommand("birthday",			100,	checkBirthdays,		0,		"Show which players online have birthdays.");
	playerCommands["colors"] = new PlyCommand("colors",				100,	cmdColors,			0,		"Show colors and set custom colors.");
	playerCommands["colours"] = new PlyCommand("colours",			100,	cmdColors,			0,		"");
	playerCommands["factions"] = new PlyCommand("factions",			100,	cmdFactions,		0,		"Show your standing with various factions.");
	playerCommands["recipes"] = new PlyCommand("recipes",			100,	cmdRecipes,			0,		"Show what recipes you know.");

	// player-only communication functions
	playerCommands["send"] = new PlyCommand("send",					100,	communicateWith,	0,		"");
	playerCommands["tell"] = new PlyCommand("tell",					100,	communicateWith,	0,		"Send a message to another player.");
	playerCommands["osend"] = new PlyCommand("osend",				100,	communicateWith,	0,		"");
	playerCommands["otell"] = new PlyCommand("otell",				100,	communicateWith,	0,		"");
	playerCommands["reply"] = new PlyCommand("reply",				100,	communicateWith,	0,		"Reply to a direct message.");
	playerCommands["sign"] = new PlyCommand("sign",					100,	communicateWith,	0,		"");
	playerCommands["whisper"] = new PlyCommand("whisper",			100,	communicateWith,	0,		"Whisper a message to another player.");
	playerCommands["yell"] = new PlyCommand("yell",					100,	pCommunicate,		0,		"Yell something that everyone in the room and nearby rooms can hear.");
	playerCommands["recite"] = new PlyCommand("recite",				100,	pCommunicate,		0,		"Recite a phrase to open certain locked doors.");
//	playerCommands["os"] = new PlyCommand("os",						 90,	pCommunicate,		0,		"");
	playerCommands["osay"] = new PlyCommand("osay",					 90,	pCommunicate,		0,		"");
	playerCommands["gtoc"] = new PlyCommand("gtoc",					100,	pCommunicate,		0,		"");
	playerCommands["gtalk"] = new PlyCommand("gtalk",				100,	pCommunicate,		0,		"Send a message to your group.");
	playerCommands["gt"] = new PlyCommand("gt",						100,	pCommunicate,		0,		"");
	playerCommands["ignore"] = new PlyCommand("ignore",				100,	cmdIgnore,			0,		"Ignore a player.");
	playerCommands["speak"] = new PlyCommand("speak",				100,	cmdSpeak,			0,		"Speak a different language.");
	playerCommands["languages"] = new PlyCommand("languages",		100,	cmdLanguages,		0,		"Show what languages you know.");
	playerCommands["talk"] = new PlyCommand("talk",					100,	cmdTalk,			0,		"Talk to a monster.");
	//playerCommands["ask"] = new PlyCommand("ask",					100,	cmdTalkNew,			0,		"");
	playerCommands["ask"] = new PlyCommand("ask",					100,	cmdTalk,			0,		"");
	playerCommands["parley"] = new PlyCommand("parley",				100,	cmdTalk,			0,		"");
	playerCommands["gag"] = new PlyCommand("gag",					100,	cmdGag,				0,		"Gag a player.");
	playerCommands["sendclan"] = new PlyCommand("sendclan",			100,	channel,			0,		"");
	playerCommands["clansend"] = new PlyCommand("clansend",			100,	channel,			0,		"Communicate via the clan channel.");
	playerCommands["bemote"] = new PlyCommand("bemote",				100,	channel,			0,		"");
	playerCommands["broademote"] = new PlyCommand("broademote",		100,	channel,			0,		"");
	playerCommands["raemote"] = new PlyCommand("raemote",			100,	channel,			0,		"");
	playerCommands["sendrace"] = new PlyCommand("sendrace",			100,	channel,			0,		"");
	playerCommands["racesend"] = new PlyCommand("racesend",			100,	channel,			0,		"Communicate via the race channel.");
	playerCommands["cls"] = new PlyCommand("cls",					100,	channel,			0,		"");
	playerCommands["sendclass"] = new PlyCommand("sendclass",		100,	channel,			0,		"");
	playerCommands["classsend"] = new PlyCommand("classsend",		100,	channel,			0,		"Communicate via the class channel.");
	playerCommands["clem"] = new PlyCommand("clem",					100,	channel,			0,		"");
	playerCommands["classemote"] = new PlyCommand("classemote",		100,	channel,			0,		"");
	playerCommands["newbie"] = new PlyCommand("newbie",				100,	channel,			0,		"Communicate via the newbie channel.");
	playerCommands["ptest"] = new PlyCommand("ptest",				100,	channel,			0,		"");
	playerCommands["gossip"] = new PlyCommand("gossip",				100,	channel,			0,		"Communicate via the gossip channel.");
	//playerCommands["gloat"] = new PlyCommand("gloat",				100,	channel,			0,		"");
	playerCommands["adult"] = new PlyCommand("adult",				100,	channel,			0,		"");
	playerCommands["broadcast"] = new PlyCommand("broadcast",		 80,	channel,			0,		"");
	playerCommands["gsend"] = new PlyCommand("gsend",				100,	cmdGuildSend,		0,		"Communicate via the guild channel.");

	// import functions
	playerCommands["lookup"] = new PlyCommand("lookup",				100,	lookup,				0,		"");
	playerCommands["restore"] = new PlyCommand("restore",			100,	restore,			0,		"");

	playerCommands["guild"] = new PlyCommand("guild",				100,	cmdGuild,			0,		"Perform guild-related commands.");
	playerCommands["guildhall"] = new PlyCommand("guildhall",		100,	cmdGuildHall,		0,		"");
	playerCommands["gh"] = new PlyCommand("gh",						100,	cmdGuildHall,		0,		"");
	playerCommands["follow"] = new PlyCommand("follow",				100,	cmdFollow,			0,		"Follow another player.");
	playerCommands["lose"] = new PlyCommand("lose",					100,	cmdLose,			0,		"Stop following another player.");
	playerCommands["group"] = new PlyCommand("group",				100,	cmdGroup,			0,		"Show group members.");
	playerCommands["party"] = new PlyCommand("party",				100,	cmdGroup,			0,		"");
	playerCommands["pay"] = new PlyCommand("pay",					100,	cmdPayToll,			0,		"Pay a monster a toll.");
	playerCommands["track"] = new PlyCommand("track",				100,	cmdTrack,			0,		"");
	playerCommands["peek"] = new PlyCommand("peek",					100,	cmdPeek,			0,		"");
	playerCommands["search"] = new PlyCommand("search",				100,	cmdSearch,			0,		"Search for anything hidden.");
	playerCommands["hide"] = new PlyCommand("hide",					100,	cmdHide,			0,		"Hide from view.");
	playerCommands["telnet"] = new PlyCommand("telnet",				100,	cmdTelOpts,			0,		"Shows what telnet options have been negotiated.");
	playerCommands["telopts"] = new PlyCommand("telopts",			100,	cmdTelOpts,			0,		"Shows what telnet options have been negotiated.");
	playerCommands["set"] = new PlyCommand("set",					100,	cmdPrefs,			0,		"Set a preference.");
	playerCommands["clear"] = new PlyCommand("clear",				100,	cmdPrefs,			0,		"Clear a preference.");
	playerCommands["toggle"] = new PlyCommand("toggle",				100,	cmdPrefs,			0,		"Toggle a preference.");
	playerCommands["preferences"] = new PlyCommand("preferences",	100,	cmdPrefs,			0,		"Show preferences.");
	playerCommands["open"] = new PlyCommand("open",					100,	cmdOpen,			0,		"Open a door.");
	playerCommands["close"] = new PlyCommand("close",				100,	cmdClose,			0,		"Close a door.");
	playerCommands["shut"] = new PlyCommand("shut",					100,	cmdClose,			0,		"");
	playerCommands["unlock"] = new PlyCommand("unlock",				100,	cmdUnlock,			0,		"");
	playerCommands["lock"] = new PlyCommand("lock",					100,	cmdLock,			0,		"");
	playerCommands["pick"] = new PlyCommand("pick",					100,	cmdPickLock,		0,		"");
	playerCommands["steal"] = new PlyCommand("steal",				100,	cmdSteal,			0,		"");
	playerCommands["flee"] = new PlyCommand("flee",					 40,	cmdFlee,			0,		"");
	playerCommands["run"] = new PlyCommand("run",					 90,	cmdFlee,			0,		"");
	playerCommands["study"] = new PlyCommand("study",				100,	cmdStudy,			0,		"Study a scroll to learn a spell.");
	playerCommands["learn"] = new PlyCommand("learn",				100,	cmdStudy,			0,		"");
	playerCommands["read"] = new PlyCommand("read",					100,	cmdReadScroll,		0,		"");
	playerCommands["commune"] = new PlyCommand("commune",			100,	cmdCommune,			0,		"");
	playerCommands["bs"] = new PlyCommand("bs",						100,	cmdBackstab,		0,		"");
	playerCommands["backstab"] = new PlyCommand("backstab",			100,	cmdBackstab,		0,		"");
	playerCommands["ambush"] = new PlyCommand("ambush",				100,	cmdAmbush,			0,		"");
	playerCommands["ab"] = new PlyCommand("ab",						100,	cmdAmbush,			0,		"");
	playerCommands["train"] = new PlyCommand("train",				100,	cmdTrain,			0,		"Go up a level.");
	playerCommands["save"] = new PlyCommand("save",					100,	cmdSave,			0,		"");
	playerCommands["time"] = new PlyCommand("time",					100,	cmdTime,			0,		"Show the current time");
	playerCommands["circle"] = new PlyCommand("circle",				 50,	cmdCircle,			0,		"");
	playerCommands["bash"] = new PlyCommand("bash",					 50,	cmdBash,			0,		"");
	playerCommands["barkskin"] = new PlyCommand("barkskin",			100,	cmdBarkskin,		0,		"");
	playerCommands["kick"] = new PlyCommand("kick",					100,	cmdKick,			0,		"");
	playerCommands["gamestat"] = new PlyCommand("gamestat",			100,	infoGamestat,		0,		"Game time statistics.");

	playerCommands["list"] = new PlyCommand("list",					100,	cmdList,			0,		"Show items for sale.");
	playerCommands["shop"] = new PlyCommand("shop",					100,	cmdShop,			0,		"Perform player-shop-related commands.");
	playerCommands["buy"] = new PlyCommand("buy",					100,	cmdBuy,				0,		"Purchase an item.");
	playerCommands["refund"] = new PlyCommand("refund",				100,	cmdRefund,			0,		"Refund an item you just purchased.");
	playerCommands["reclaim"] = new PlyCommand("reclaim",			100,	cmdReclaim,			0,		"Reclaim an item you just pawned.");
	playerCommands["shoplift"] = new PlyCommand("shoplift",			100,	cmdShoplift,		0,		"");
	playerCommands["sell"] = new PlyCommand("sell",					100,	cmdSell,			0,		"Sell an item.");
	playerCommands["value"] = new PlyCommand("value",				100,	cmdValue,			0,		"Check the value of an item.");
	playerCommands["cost"] = new PlyCommand("cost",					100,	cmdCost,			0,		"");
	playerCommands["purchase"] = new PlyCommand("purchase",			90,		cmdPurchase,		0,		"");
	playerCommands["selection"] = new PlyCommand("selection",		100,	cmdSelection,		0,		"");
	playerCommands["trade"] = new PlyCommand("trade",				100,	cmdTrade,			0,		"Trade an item with a monster.");
	playerCommands["auction"] = new PlyCommand("auction",			100,	cmdAuction,			0,		"Auction an item for players to buy.");
	playerCommands["repair"] = new PlyCommand("repair",				100,	cmdRepair,			0,		"Have a smithy repair an item.");
	playerCommands["fix"] = new PlyCommand("fix",					100,	cmdRepair,			0,		"");

	playerCommands["sendmail"] = new PlyCommand("sendmail",			100,	cmdSendMail,		0,		"Send a mudmail to another player.");
	playerCommands["readmail"] = new PlyCommand("readmail",			100,	cmdReadMail,		0,		"Read your mudmail.");
	playerCommands["deletemail"] = new PlyCommand("deletemail",		100,	cmdDeleteMail,		0,		"");
	playerCommands["edithistory"] = new PlyCommand("edithistory",	100,	cmdEditHistory,		0,		"");
	playerCommands["history"] = new PlyCommand("history",			100,	cmdHistory,			0,		"");
	playerCommands["deletehistory"] = new PlyCommand("deletehistory",100,	cmdDeleteHistory,	0,		"");

	playerCommands["drink"] = new PlyCommand("drink",				100,	cmdConsume,			0,		"Drink a potion.");
	playerCommands["quaff"] = new PlyCommand("quaff",				100,	cmdConsume,			0,		"");
	playerCommands["eat"] = new PlyCommand("eat",					100,	cmdConsume,			0,		"Eat some food.");
	playerCommands["recall"] = new PlyCommand("recall",				100,	cmdRecall,			0,		"");
	playerCommands["hazy"] = new PlyCommand("hazy",					100,	cmdRecall,			0,		"");
	playerCommands["teach"] = new PlyCommand("teach",				100,	cmdTeach,			0,		"");
	playerCommands["welcome"] = new PlyCommand("welcome",			100,	cmdWelcome,			0,		"");
	playerCommands["turn"] = new PlyCommand("turn",					100,	cmdTurn,			0,		"");
	playerCommands["renounce"] = new PlyCommand("renounce",			100,	cmdRenounce,		0,		"");
	playerCommands["holyword"] = new PlyCommand("holyword",			100,	cmdHolyword,		0,		"");
	playerCommands["creep"] = new PlyCommand("creep",				100,	cmdCreepingDoom,	0,		"");
	playerCommands["poison"] = new PlyCommand("poison",				100,	cmdPoison,			0,		"");
	playerCommands["smother"] = new PlyCommand("smother",			100,	cmdEarthSmother,	0,		"");
	playerCommands["bribe"] = new PlyCommand("bribe",				100,	cmdBribe,			0,		"Bribe a monster to leave the room.");
	playerCommands["frenzy"] = new PlyCommand("frenzy",				100,	cmdFrenzy,			0,		"");
	playerCommands["pray"] = new PlyCommand("pray",					100,	cmdPray,			0,		"");
	playerCommands["use"] = new PlyCommand("use",					100,	cmdUse,				0,		"");
	playerCommands["us"] = new PlyCommand("us",						100,	cmdUse,				0,		"");
	playerCommands["break"] = new PlyCommand("break",				100,	cmdBreak,			0,		"Break an object.");
	playerCommands["pledge"] = new PlyCommand("pledge",				100,	cmdPledge,			0,		"Pledge your allegience to a clan.");
	playerCommands["rescind"] = new PlyCommand("rescind",			100,	cmdRescind,			0,		"Rescind your allegience from a clan.");
	playerCommands["suicide"] = new PlyCommand("suicide",			100,	cmdSuicide,			0,		"Delete your character.");
	playerCommands["convert"] = new PlyCommand("convert",			100,	cmdConvert,			0,		"Convert from chaotic alignment to lawful alignment.");
	playerCommands["password"] = new PlyCommand("password",			100,	cmdPassword,		0,		"Change your password.");
	playerCommands["finger"] = new PlyCommand("finger",				100,	cmdFinger,			0,		"Look up an offline character.");
	playerCommands["hypnotize"] = new PlyCommand("hypnotize",		100,	cmdHypnotize,		0,		"");
	playerCommands["bite"] = new PlyCommand("bite",					 60,	cmdBite,			0,		"");
	playerCommands["mist"] = new PlyCommand("mist",					100,	cmdMist,			0,		"");
	playerCommands["unmist"] = new PlyCommand("unmist",				100,	cmdUnmist,			0,		"");
	playerCommands["regenerate"] = new PlyCommand("regenerate",		100,	cmdRegenerate,		0,		"");
	playerCommands["drain"] = new PlyCommand("drain",				100,	cmdDrainLife,		0,		"");

	playerCommands["charm"] = new PlyCommand("charm",				100,	cmdCharm,			0,		"");
	playerCommands["identify"] = new PlyCommand("identify",			100,	cmdIdentify,		0,		"");
	playerCommands["songs"] = new PlyCommand("songs",				100,	cmdSongs,			0,		"");

	playerCommands["enthrall"] = new PlyCommand("enthrall",			100,	cmdEnthrall,		0,		"");
	playerCommands["meditate"] = new PlyCommand("meditate",			100,	cmdMeditate,		0,		"");
	playerCommands["focus"] = new PlyCommand("focus",				100,	cmdFocus,			0,		"");
	playerCommands["mistbane"] = new PlyCommand("mistbane",			 90,	cmdMistbane,		0,		"");
	playerCommands["mb"] = new PlyCommand("mb",						100,	cmdMistbane,		0,		"");
	playerCommands["touch"] = new PlyCommand("touch",				100,	cmdTouchOfDeath,	0,		"");
	playerCommands["quests"] = new PlyCommand("quests",				100,	cmdQuests,			0,		"View your quests and perform quest-related commands.");
	playerCommands["hands"] = new PlyCommand("hands",				100,	cmdLayHands,		0,		"");
	playerCommands["harm"] = new PlyCommand("harm",					100,	cmdHarmTouch,		0,		"");
	playerCommands["scout"] = new PlyCommand("scout",				100,	cmdScout,			0,		"");
	playerCommands["berserk"] = new PlyCommand("berserk",			100,	cmdBerserk,			0,		"");
	playerCommands["transmute"] = new PlyCommand("transmute",		100,	cmdTransmute,		0,		"");
	playerCommands["daily"] = new PlyCommand("daily",				100,	cmdDaily,			0,		"See our usage of daily-limited abilities.");
	playerCommands["description"] = new PlyCommand("description",	100,	cmdDescription,		0,		"Change your character's description.");
	playerCommands["bloodsacrifice"] = new PlyCommand("bloodsacrifice",100,	cmdBloodsacrifice,	0,		"");
	playerCommands["disarm"] = new PlyCommand("disarm",				100,	cmdDisarm,			0,		"Disarm an opponent or remove your wielded weapons.");
	playerCommands["visible"] = new PlyCommand("visible",			100,	cmdVisible,			0,		"Cancel invisibility.");
	playerCommands["watch"] = new PlyCommand("watch",				100,	cmdWatch,			0,		"");
	playerCommands["conjure"] = new PlyCommand("conjure",			100,	conjureCmd,			0,		"");
	playerCommands["maul"] = new PlyCommand("maul",					100,	cmdMaul,			0,		"");
	playerCommands["enchant"] = new PlyCommand("enchant",			100,	cmdEnchant,			0,		"");
	playerCommands["envenom"] = new PlyCommand("envenom",			100,	cmdEnvenom,			0,		"");

	playerCommands["balance"] = new PlyCommand("balance",			 90,	cmdBalance,			0,		"See your bank balance.");
	playerCommands["deposit"] = new PlyCommand("deposit",			100,	cmdDeposit,			0,		"Deposit gold in the bank.");
	playerCommands["withdraw"] = new PlyCommand("withdraw",			100,	cmdWithdraw,		0,		"Withdraw gold from the bank.");
	playerCommands["deletestatement"] = new PlyCommand("deletestatement",100,	cmdDeleteStatement,	0,		"");
	playerCommands["transfer"] = new PlyCommand("transfer",			100,	cmdTransfer,		0,		"Transfer gold to another character.");
//	playerCommands["stat"] = new PlyCommand("stat",					100,	cmdStatement,		0,		"");
	playerCommands["statement"] = new PlyCommand("statement",		 90,	cmdStatement,		0,		"View your bank statement.");

	playerCommands["refuse"] = new PlyCommand("refuse",				100,	cmdRefuse,			0,		"");
	playerCommands["duel"] = new PlyCommand("duel",					100,	cmdDuel,			0,		"Challenge another player to a duel.");
	playerCommands["animate"] = new PlyCommand("animate",			100,	animateDeadCmd,		0,		"");
	playerCommands["lottery"] = new PlyCommand("lottery",			100,	cmdLottery,			0,		"View lottery info.");
	playerCommands["claim"] = new PlyCommand("claim",				100,	cmdClaim,			0,		"Claim a lottery ticket.");
	playerCommands["mccp"] = new PlyCommand("mccp",					100,	mccp,				0,		"Toggle mccp (compression).");
	playerCommands["surname"] = new PlyCommand("surname",			100,	cmdSurname,			0,		"Choose a surname.");
	playerCommands["changestats"] = new PlyCommand("changestats",	100,	cmdChangeStats,		0,		"");
	playerCommands["keep"] = new PlyCommand("keep",					100,	cmdKeep,			0,		"Prevent accidentially throwing away an item.");
	playerCommands["unkeep"] = new PlyCommand("unkeep",				100,	cmdUnkeep,			0,		"Unkeep an item.");
	playerCommands["alignment"] = new PlyCommand("alignment",		100,	cmdChooseAlignment,	0,		"Choose your alignment.");
	playerCommands["push"] = new PlyCommand("push",					100,	cmdPush,			0,		"");
	playerCommands["pull"] = new PlyCommand("pull",					100,	cmdPull,			0,		"");
	playerCommands["press"] = new PlyCommand("press",				100,	cmdPress,			0,		"");
	playerCommands["shipquery"] = new PlyCommand("shipquery",		90,		cmdQueryShips,		0,		"See where chartered ships are.");
	playerCommands["ships"] = new PlyCommand("ships",				100,	cmdQueryShips,		0,		"");

	playerCommands["bandage"] = new PlyCommand("bandage",			100,	cmdBandage,			0,		"Use bandages to heal yourself.");

	// movement
//	playerCommands["n"] = new PlyCommand("n",						100,	cmdMove,			0,		"");
	playerCommands["north"] = new PlyCommand("north",				 10,	cmdMove,			0,		"");
//	playerCommands["s"] = new PlyCommand("s",						100,	cmdMove,			0,		"");
	playerCommands["south"] = new PlyCommand("south",				 10,	cmdMove,			0,		"");
//	playerCommands["e"] = new PlyCommand("e",						100,	cmdMove,			0,		"");
	playerCommands["east"] = new PlyCommand("east",					 10,	cmdMove,			0,		"");
//	playerCommands["w"] = new PlyCommand("w",						100,	cmdMove,			0,		"");
	playerCommands["west"] = new PlyCommand("west",				 	 10,	cmdMove,			0,		"");
	playerCommands["northeast"] = new PlyCommand("northeast",		 15,	cmdMove,			0,		"");
	playerCommands["ne"] = new PlyCommand("ne",						 15,	cmdMove,			0,		"");
	playerCommands["northwest"] = new PlyCommand("northwest",		 15,	cmdMove,			0,		"");
	playerCommands["nw"] = new PlyCommand("nw",						 15,	cmdMove,			0,		"");
	playerCommands["southeast"] = new PlyCommand("southeast",		 15,	cmdMove,			0,		"");
	playerCommands["se"] = new PlyCommand("se",						 15,	cmdMove,			0,		"");
	playerCommands["southwest"] = new PlyCommand("southwest",		 15,	cmdMove,			0,		"");
	playerCommands["sw"] = new PlyCommand("sw",						 15,	cmdMove,			0,		"");
//	playerCommands["u"] = new PlyCommand("u",						 10,	cmdMove,			0,		"");
	playerCommands["up"] = new PlyCommand("up",						 10,	cmdMove,			0,		"");
//	playerCommands["d"] = new PlyCommand("d",						100,	cmdMove,			0,		"");
	playerCommands["down"] = new PlyCommand("down",					 10,	cmdMove,			0,		"");
	playerCommands["out"] = new PlyCommand("out",					100,	cmdMove,			0,		"");
	playerCommands["leave"] = new PlyCommand("leave",				100,	cmdMove,			0,		"");
	//		playerCommands["aft"] = new PlyCommand("aft",			100,		move,				0,		"");
	//		playerCommands["fore"] = new PlyCommand("fore",			100,		move,				0,		"");
	//		playerCommands["starb"] = new PlyCommand("starb",		100,		move,				0,		"");
	//		playerCommands["port"] = new PlyCommand("port",			100,		move,				0,		"");
	//playerCommands["sn"] = new PlyCommand("sn",						100,	cmdGo,				0,		"");
	playerCommands["sneak"] = new PlyCommand("sneak",				 50,	cmdGo,				0,		"Sneak to an exit so you are not seen.");
	playerCommands["go"] = new PlyCommand("go",						100,	cmdGo,				0,		"");
	playerCommands["exit"] = new PlyCommand("exit",					100,	cmdGo,				0,		"");
	playerCommands["enter"] = new PlyCommand("enter",				100,	cmdGo,				0,		"");

// *************************************************************************************
// General player/pet Commands

	// general player/pet commands
	generalCommands["say"] = new CrtCommand("say",					100,	communicate,		0,		"");
	generalCommands["\\"] = new CrtCommand("\"",					100,	communicate,		0,		"");
	generalCommands["'"] = new CrtCommand("'",						100,	communicate,		0,		"");
	generalCommands["emote"] = new CrtCommand("emote",				100,	communicate,		0,		"");
	generalCommands[":"] = new CrtCommand(":",						100,	communicate,		0,		"");

	generalCommands["spells"] = new CrtCommand("spells",			100,	cmdSpells,			0,		"Show what spells you know.");
//	generalCommands["c"] = new CrtCommand("c",						100,	cmdCast,			0,		"");
	generalCommands["cast"] = new CrtCommand("cast",				 10,	cmdCast,			0,		"");

	generalCommands["attack"] = new CrtCommand("attack",			 50,	cmdAttack,			0,		"");
	generalCommands["kill"] = new CrtCommand("kill",				 10,	cmdAttack,			0,		"");
//	generalCommands["k"] = new CrtCommand("k",						100,	cmdAttack,			0,		"");

	generalCommands["get"] = new CrtCommand("get",					100,	cmdGet,				0,		"Pick up an item and put it in your inventory.");
	generalCommands["take"] = new CrtCommand("take",				100,	cmdGet,				0,		"");
	generalCommands["drop"] = new CrtCommand("drop",				100,	cmdDrop,			0,		"Remove an item from your inventory and drop it on the ground.");
	generalCommands["put"] = new CrtCommand("put",					100,	cmdDrop,			0,		"");
	generalCommands["give"] = new CrtCommand("give",				100,	cmdGive,			0,		"");

	generalCommands["throw"] = new CrtCommand("throw",				100,	cmdThrow,			0,		"Throw an object.");
	generalCommands["knock"] = new CrtCommand("knock",				100,	cmdKnock,			0,		"Knock on a door to make a noise on the other side.");
	generalCommands["effects"] = new CrtCommand("effects",			100,	cmdEffects,			0,		"Show what effects you are currently under.");
	generalCommands["howl"] = new CrtCommand("howl",				100,	cmdHowl,			0,		"");
	generalCommands["sing"] = new CrtCommand("sing",				100,	cmdSing,			0,		"");

	// socials
	generalCommands["chest"] = new CrtCommand("chest",				100,	cmdAction,				0,		"");
	generalCommands["anvil"] = new CrtCommand("anvil",				100,	cmdAction,				0,		"");
	generalCommands["stab"] = new CrtCommand("stab",				100,	cmdAction,				0,		"");
	generalCommands["vangogh"] = new CrtCommand("vangogh",			100,	cmdAction,				0,		"");
	generalCommands["pan"] = new CrtCommand("pan",					100,	cmdAction,				0,		"");
	generalCommands["smooch"] = new CrtCommand("smooch",			100,	cmdAction,				0,		"");
	generalCommands["peck"] = new CrtCommand("peck",				100,	cmdAction,				0,		"");
	generalCommands["squeeze"] = new CrtCommand("squeeze",			100,	cmdAction,				0,		"");
	generalCommands["french"] = new CrtCommand("french",			100,	cmdAction,				0,		"");
	generalCommands["froth"] = new CrtCommand("froth",				100,	cmdAction,				0,		"");
	generalCommands["lobotomy"] = new CrtCommand("lobotomy",		100,	cmdAction,				0,		"");
	generalCommands["maniac"] = new CrtCommand("maniac",			100,	cmdAction,				0,		"");
	generalCommands["hail"] = new CrtCommand("hail",				100,	cmdAction,				0,		"");
	generalCommands["boo"] = new CrtCommand("boo",					100,	cmdAction,				0,		"");
	generalCommands["punch"] = new CrtCommand("punch",				100,	cmdAction,				0,		"");
	generalCommands["bubble"] = new CrtCommand("bubble",			100,	cmdAction,				0,		"");
	generalCommands["moan"] = new CrtCommand("moan",				100,	cmdAction,				0,		"");
	generalCommands["nod"] = new CrtCommand("nod",					100,	cmdAction,				0,		"");
	generalCommands["sleep"] = new CrtCommand("sleep",				100,	cmdAction,				0,		"Go to sleep to heal faster.");
	generalCommands["stand"] = new CrtCommand("stand",				100,	cmdAction,				0,		"Stand up from a sitting position.");
	generalCommands["sit"] = new CrtCommand("sit",					100,	cmdAction,				0,		"Sit down to heal faster.");
	generalCommands["grab"] = new CrtCommand("grab",				100,	cmdAction,				0,		"");
	generalCommands["shove"] = new CrtCommand("shove",				100,	cmdAction,				0,		"");
	generalCommands["nervous"] = new CrtCommand("nervous",			100,	cmdAction,				0,		"");
	generalCommands["bird"] = new CrtCommand("bird",				100,	cmdAction,				0,		"");
	generalCommands["ogle"] = new CrtCommand("ogle",				100,	cmdAction,				0,		"");
	generalCommands["nod"] = new CrtCommand("nod",					100,	cmdAction,				0,		"");
	generalCommands["relax"] = new CrtCommand("relax",				100,	cmdAction,				0,		"");
	generalCommands["puke"] = new CrtCommand("puke",				100,	cmdAction,				0,		"");
	generalCommands["think"] = new CrtCommand("think",				100,	cmdAction,				0,		"");
	generalCommands["dirt"] = new CrtCommand("dirt",				100,	cmdAction,				0,		"");
	generalCommands["cheer"] = new CrtCommand("cheer",				100,	cmdAction,				0,		"");
	generalCommands["ponder"] = new CrtCommand("ponder",			100,	cmdAction,				0,		"");
	generalCommands["ack"] = new CrtCommand("ack",					100,	cmdAction,				0,		"");
	generalCommands["dismiss"] = new CrtCommand("dismiss",			100,	cmdAction,				0,		"");
	generalCommands["laugh"] = new CrtCommand("laugh",				100,	cmdAction,				0,		"");
	generalCommands["hysterical"] = new CrtCommand("hysterical",	100,	cmdAction,				0,		"");
	generalCommands["burp"] = new CrtCommand("burp",				100,	cmdAction,				0,		"");
	generalCommands["psycho"] = new CrtCommand("psycho",			100,	cmdAction,				0,		"");
	generalCommands["frustrate"] = new CrtCommand("frustrate",		100,	cmdAction,				0,		"");
	generalCommands["warm"] = new CrtCommand("warm",				100,	cmdAction,				0,		"");
	generalCommands["kic"] = new CrtCommand("kic",					100,	cmdAction,				0,		"");
	generalCommands["tackle"] = new CrtCommand("tackle",			100,	cmdAction,				0,		"");
	generalCommands["knee"] = new CrtCommand("knee",				100,	cmdAction,				0,		"");
	generalCommands["pounce"] = new CrtCommand("pounce",			100,	cmdAction,				0,		"");
	generalCommands["rps"] = new CrtCommand("rps",					100,	cmdAction,				0,		"");
	generalCommands["tickle"] = new CrtCommand("tickle",			100,	cmdAction,				0,		"");
	generalCommands["snicker"] = new CrtCommand("snicker",			100,	cmdAction,				0,		"");
	generalCommands["tap"] = new CrtCommand("tap",					100,	cmdAction,				0,		"");
	generalCommands["smile"] = new CrtCommand("smile",				100,	cmdAction,				0,		"");
	generalCommands["beam"] = new CrtCommand("beam",				100,	cmdAction,				0,		"");
	generalCommands["masturbate"] = new CrtCommand("masturbate",	90,		cmdAction,				0,		"");
	generalCommands["smoke"] = new CrtCommand("smoke",				100,	cmdAction,				0,		"");
	generalCommands["shake"] = new CrtCommand("shake",				100,	cmdAction,				0,		"");
	generalCommands["cackle"] = new CrtCommand("cackle",			100,	cmdAction,				0,		"");
	generalCommands["chuckle"] = new CrtCommand("chuckle",			100,	cmdAction,				0,		"");
	generalCommands["wave"] = new CrtCommand("wave",				100,	cmdAction,				0,		"");
	generalCommands["poke"] = new CrtCommand("poke",				100,	cmdAction,				0,		"");
	generalCommands["yawn"] = new CrtCommand("yawn",				100,	cmdAction,				0,		"");
	generalCommands["sigh"] = new CrtCommand("sigh",				100,	cmdAction,				0,		"");
	generalCommands["bounce"] = new CrtCommand("bounce",			100,	cmdAction,				0,		"");
	generalCommands["shrug"] = new CrtCommand("shrug",				100,	cmdAction,				0,		"");
	generalCommands["twiddle"] = new CrtCommand("twiddle",			100,	cmdAction,				0,		"");
	generalCommands["grin"] = new CrtCommand("grin",				100,	cmdAction,				0,		"");
	generalCommands["frown"] = new CrtCommand("frown",				100,	cmdAction,				0,		"");
	generalCommands["giggle"] = new CrtCommand("giggle",			100,	cmdAction,				0,		"");
	generalCommands["behind"] = new CrtCommand("behind",			100,	cmdAction,				0,		"");
	generalCommands["hum"] = new CrtCommand("hum",					100,	cmdAction,				0,		"");
	generalCommands["hump"] = new CrtCommand("hump",				100,	cmdAction,				0,		"");
	generalCommands["snap"] = new CrtCommand("snap",				100,	cmdAction,				0,		"");
	generalCommands["jump"] = new CrtCommand("jump",				50,		cmdAction,				0,		"");
	generalCommands["skip"] = new CrtCommand("skip",				100,	cmdAction,				0,		"");
	generalCommands["dance"] = new CrtCommand("dance",				100,	cmdAction,				0,		"");
	generalCommands["cry"] = new CrtCommand("cry",					100,	cmdAction,				0,		"");
	generalCommands["bleed"] = new CrtCommand("bleed",				100,	cmdAction,				0,		"");
	generalCommands["sniff"] = new CrtCommand("sniff",				100,	cmdAction,				0,		"");
	generalCommands["whimper"] = new CrtCommand("whimper",			100,	cmdAction,				0,		"");
	generalCommands["cringe"] = new CrtCommand("cringe",			100,	cmdAction,				0,		"");
	generalCommands["whistle"] = new CrtCommand("whistle",			90,		cmdAction,				0,		"");
	generalCommands["smirk"] = new CrtCommand("smirk",				100,	cmdAction,				0,		"");
	generalCommands["gasp"] = new CrtCommand("gasp",				100,	cmdAction,				0,		"");
	generalCommands["grunt"] = new CrtCommand("grunt",				100,	cmdAction,				0,		"");
	generalCommands["stomp"] = new CrtCommand("stomp",				100,	cmdAction,				0,		"");
	generalCommands["flex"] = new CrtCommand("flex",				100,	cmdAction,				0,		"");
	generalCommands["curtsy"] = new CrtCommand("curtsy",			100,	cmdAction,				0,		"");
	generalCommands["grr"] = new CrtCommand("grr",					100,	cmdAction,				0,		"");
	// "pet" also links to orderPet within action
	generalCommands["pet"] = new CrtCommand("pet",					100,	cmdAction,				0,		"");
	generalCommands["blush"] = new CrtCommand("blush",				100,	cmdAction,				0,		"");
	generalCommands["faint"] = new CrtCommand("faint",				100,	cmdAction,				0,		"");
	generalCommands["hug"] = new CrtCommand("hug",					100,	cmdAction,				0,		"");
	generalCommands["cstr"] = new CrtCommand("cstr",				100,	cmdAction,				0,		"");
	generalCommands["expose"] = new CrtCommand("expose",			100,	cmdAction,				0,		"");
	generalCommands["wink"] = new CrtCommand("wink",				100,	cmdAction,				0,		"");
	generalCommands["blink"] = new CrtCommand("blink",				100,	cmdAction,				0,		"");
	generalCommands["clap"] = new CrtCommand("clap",				100,	cmdAction,				0,		"");
	generalCommands["drool"] = new CrtCommand("drool",				100,	cmdAction,				0,		"");
	generalCommands["copulate"] = new CrtCommand("copulate",		100,	cmdAction,				0,		"");
	generalCommands["goose"] = new CrtCommand("goose",				100,	cmdAction,				0,		"");
	generalCommands["fume"] = new CrtCommand("fume",				100,	cmdAction,				0,		"");
	generalCommands["rage"] = new CrtCommand("rage",				100,	cmdAction,				0,		"");
	generalCommands["pout"] = new CrtCommand("pout",				100,	cmdAction,				0,		"");
	generalCommands["spit"] = new CrtCommand("spit",				100,	cmdAction,				0,		"");
	generalCommands["fart"] = new CrtCommand("fart",				100,	cmdAction,				0,		"");
	generalCommands["comfort"] = new CrtCommand("comfort",			100,	cmdAction,				0,		"");
	generalCommands["pat"] = new CrtCommand("pat",					100,	cmdAction,				0,		"");
	generalCommands["jk"] = new CrtCommand("jk",					100,	cmdAction,				0,		"");
	generalCommands["thank"] = new CrtCommand("thank",				100,	cmdAction,				0,		"");
	generalCommands["bitchslap"] = new CrtCommand("bitchslap",		100,	cmdAction,				0,		"");
	generalCommands["kiss"] = new CrtCommand("kiss",				100,	cmdAction,				0,		"");
	generalCommands["glare"] = new CrtCommand("glare",				100,	cmdAction,				0,		"");
	generalCommands["bless"] = new CrtCommand("bless",				100,	cmdAction,				0,		"");
	generalCommands["hbeer"] = new CrtCommand("hbeer",				100,	cmdAction,				0,		"");
	generalCommands["slap"] = new CrtCommand("slap",				100,	cmdAction,				0,		"");
	generalCommands["woot"] = new CrtCommand("woot",				100,	cmdAction,				0,		"");
	generalCommands["hammer"] = new CrtCommand("hammer",			100,	cmdAction,				0,		"");
	generalCommands["suck"] = new CrtCommand("suck",				100,	cmdAction,				0,		"");
	generalCommands["bow"] = new CrtCommand("bow",					100,	cmdAction,				0,		"");
	generalCommands["cough"] = new CrtCommand("cough",				100,	cmdAction,				0,		"");
	generalCommands["confused"] = new CrtCommand("confused",		100,	cmdAction,				0,		"");
	generalCommands["grumble"] = new CrtCommand("grumble",			100,	cmdAction,				0,		"");
	generalCommands["hiccup"] = new CrtCommand("hiccup",			100,	cmdAction,				0,		"");
	generalCommands["mutter"] = new CrtCommand("mutter",			100,	cmdAction,				0,		"");
	generalCommands["scratch"] = new CrtCommand("scratch",			90,		cmdAction,				0,		"");
	generalCommands["bonk"] = new CrtCommand("bonk",				100,	cmdAction,				0,		"");
	generalCommands["boggle"] = new CrtCommand("boggle",			100,	cmdAction,				0,		"");
	generalCommands["strut"] = new CrtCommand("strut",				100,	cmdAction,				0,		"");
	generalCommands["sulk"] = new CrtCommand("sulk",				100,	cmdAction,				0,		"");
	generalCommands["satisfied"] = new CrtCommand("satisfied",		100,	cmdAction,				0,		"");
	generalCommands["wince"] = new CrtCommand("wince",				100,	cmdAction,				0,		"");
	generalCommands["roll"] = new CrtCommand("roll",				100,	cmdAction,				0,		"");
	generalCommands["raise"] = new CrtCommand("raise",				100,	cmdAction,				0,		"");
	generalCommands["whine"] = new CrtCommand("whine",				100,	cmdAction,				0,		"");
	generalCommands["growl"] = new CrtCommand("growl",				100,	cmdAction,				0,		"");
	generalCommands["high5"] = new CrtCommand("high5",				100,	cmdAction,				0,		"");
	generalCommands["moon"] = new CrtCommand("moon",				100,	cmdAction,				0,		"");
	generalCommands["purr"] = new CrtCommand("purr",				100,	cmdAction,				0,		"");
	generalCommands["oink"] = new CrtCommand("oink",				100,	cmdAction,				0,		"");
	generalCommands["taunt"] = new CrtCommand("taunt",				100,	cmdAction,				0,		"");
	generalCommands["eye"] = new CrtCommand("eye",					100,	cmdAction,				0,		"");
	generalCommands["worship"] = new CrtCommand("worship",			100,	cmdAction,				0,		"");
	generalCommands["flip"] = new CrtCommand("flip",				100,	cmdAction,				0,		"");
	generalCommands["groan"] = new CrtCommand("groan",				100,	cmdAction,				0,		"");
	generalCommands["meiko"] = new CrtCommand("meiko",				100,	cmdAction,				0,		"");
	generalCommands["halo"] = new CrtCommand("halo",				100,	cmdAction,				0,		"");
	generalCommands["twitch"] = new CrtCommand("twitch",			100,	cmdAction,				0,		"");
	generalCommands["point"] = new CrtCommand("point",				100,	cmdAction,				0,		"");
	generalCommands["die"] = new CrtCommand("die",					100,	cmdAction,				0,		"");
	generalCommands["afk"] = new CrtCommand("afk",					100,	cmdAction,				0,		"");
	generalCommands["ahem"] = new CrtCommand("ahem",				100,	cmdAction,				0,		"");
	generalCommands["attention"] = new CrtCommand("attention",		100,	cmdAction,				0,		"");
	generalCommands["bark"] = new CrtCommand("bark",				100,	cmdAction,				0,		"");
	generalCommands["babble"] = new CrtCommand("babble",			100,	cmdAction,				0,		"");
	generalCommands["pants"] = new CrtCommand("pants",				100,	cmdAction,				0,		"");
	generalCommands["beer"] = new CrtCommand("beer",				100,	cmdAction,				0,		"");
	generalCommands["collapse"] = new CrtCommand("collapse",		100,	cmdAction,				0,		"");
	generalCommands["comb"] = new CrtCommand("comb",				100,	cmdAction,				0,		"");
	generalCommands["fangs"] = new CrtCommand("fangs",				100,	cmdAction,				0,		"");
	generalCommands["hiss"] = new CrtCommand("hiss",				100,	cmdAction,				0,		"");
	generalCommands["hunger"] = new CrtCommand("hunger",			100,	cmdAction,				0,		"");
	generalCommands["compose"] = new CrtCommand("compose",			100,	cmdAction,				0,		"");
	generalCommands["crazy"] = new CrtCommand("crazy",				100,	cmdAction,				0,		"");
	generalCommands["curl"] = new CrtCommand("curl",				100,	cmdAction,				0,		"");
	generalCommands["curse"] = new CrtCommand("curse",				100,	cmdAction,				0,		"");
	generalCommands["daydream"] = new CrtCommand("daydream",		100,	cmdAction,				0,		"");
	generalCommands["doh"] = new CrtCommand("doh",					100,	cmdAction,				0,		"");
	generalCommands["fidget"] = new CrtCommand("fidget",			100,	cmdAction,				0,		"");
	generalCommands["foam"] = new CrtCommand("foam",				100,	cmdAction,				0,		"");
	generalCommands["freak"] = new CrtCommand("freak",				100,	cmdAction,				0,		"");
	generalCommands["freeze"] = new CrtCommand("freeze",			100,	cmdAction,				0,		"");
	generalCommands["gibber"] = new CrtCommand("gibber",			100,	cmdAction,				0,		"");
	generalCommands["grimace"] = new CrtCommand("grimace",			100,	cmdAction,				0,		"");
	generalCommands["grind"] = new CrtCommand("grind",				100,	cmdAction,				0,		"");
	generalCommands["jiggy"] = new CrtCommand("jiggy",				100,	cmdAction,				0,		"");
	generalCommands["ballz"] = new CrtCommand("ballz",				100,	cmdAction,				0,		"");
	generalCommands["wag"] = new CrtCommand("wag",					100,	cmdAction,				0,		"");
	generalCommands["lag"] = new CrtCommand("lag",					100,	cmdAction,				0,		"");
	generalCommands["monkey"] = new CrtCommand("monkey",			100,	cmdAction,				0,		"");
	generalCommands["moo"] = new CrtCommand("moo",					100,	cmdAction,				0,		"");
	generalCommands["mope"] = new CrtCommand("mope",				100,	cmdAction,				0,		"");
	generalCommands["mosh"] = new CrtCommand("mosh",				100,	cmdAction,				0,		"");
	generalCommands["murmur"] = new CrtCommand("murmur",			100,	cmdAction,				0,		"");
	generalCommands["pace"] = new CrtCommand("pace",				100,	cmdAction,				0,		"");
	generalCommands["panic"] = new CrtCommand("panic",				100,	cmdAction,				0,		"");
	generalCommands["pant"] = new CrtCommand("pant",				100,	cmdAction,				0,		"");
	generalCommands["quack"] = new CrtCommand("quack",				100,	cmdAction,				0,		"");
	generalCommands["rant"] = new CrtCommand("rant",				100,	cmdAction,				0,		"");
	generalCommands["roar"] = new CrtCommand("roar",				100,	cmdAction,				0,		"");
	generalCommands["rofl"] = new CrtCommand("rofl",				100,	cmdAction,				0,		"");
	generalCommands["sad"] = new CrtCommand("sad",					100,	cmdAction,				0,		"");
	generalCommands["scream"] = new CrtCommand("scream",			100,	cmdAction,				0,		"");
	generalCommands["shiver"] = new CrtCommand("shiver",			100,	cmdAction,				0,		"");
	generalCommands["shudder"] = new CrtCommand("shudder",			100,	cmdAction,				0,		"");
	generalCommands["snore"] = new CrtCommand("snore",				100,	cmdAction,				0,		"");
	generalCommands["snort"] = new CrtCommand("snort",				100,	cmdAction,				0,		"");
	generalCommands["spaz"] = new CrtCommand("spaz",				100,	cmdAction,				0,		"");
	generalCommands["spin"] = new CrtCommand("spin",				100,	cmdAction,				0,		"");
	generalCommands["squeal"] = new CrtCommand("squeal",			100,	cmdAction,				0,		"");
	generalCommands["squirm"] = new CrtCommand("squirm",			100,	cmdAction,				0,		"");
	generalCommands["tag"] = new CrtCommand("tag",					100,	cmdAction,				0,		"");
	generalCommands["stretch"] = new CrtCommand("stretch",			100,	cmdAction,				0,		"");
	generalCommands["strip"] = new CrtCommand("strip",				100,	cmdAction,				0,		"");
	generalCommands["stumble"] = new CrtCommand("stumble",			100,	cmdAction,				0,		"");
	generalCommands["meow"] = new CrtCommand("meow",				100,	cmdAction,				0,		"");
	generalCommands["narrow"] = new CrtCommand("narrow",			100,	cmdAction,				0,		"");
	generalCommands["surrender"] = new CrtCommand("surrender",		100,	cmdAction,				0,		"");
	generalCommands["sweat"] = new CrtCommand("sweat",				100,	cmdAction,				0,		"");
	generalCommands["tantrum"] = new CrtCommand("tantrum",			100,	cmdAction,				0,		"");
	generalCommands["triumph"] = new CrtCommand("triumph",			100,	cmdAction,				0,		"");
	generalCommands["wail"] = new CrtCommand("wail",				100,	cmdAction,				0,		"");
	generalCommands["wall"] = new CrtCommand("wall",				100,	cmdAction,				0,		"");
	generalCommands["wiggle"] = new CrtCommand("wiggle",			100,	cmdAction,				0,		"");
	generalCommands["writhe"] = new CrtCommand("writhe",			100,	cmdAction,				0,		"");
	generalCommands["yodel"] = new CrtCommand("yodel",				100,	cmdAction,				0,		"");
	generalCommands["glaze"] = new CrtCommand("glaze",				100,	cmdAction,				0,		"");
	generalCommands["count"] = new CrtCommand("count",				100,	cmdAction,				0,		"");
	generalCommands["worry"] = new CrtCommand("worry",				100,	cmdAction,				0,		"");
	generalCommands["fall"] = new CrtCommand("fall",				100,	cmdAction,				0,		"");
	generalCommands["cluck"] = new CrtCommand("cluck",				100,	cmdAction,				0,		"");
	generalCommands["stagger"] = new CrtCommand("stagger",			100,	cmdAction,				0,		"");
	generalCommands["crow"] = new CrtCommand("crow",				100,	cmdAction,				0,		"");
	generalCommands["munch"] = new CrtCommand("munch",				100,	cmdAction,				0,		"");
	generalCommands["cornholio"] = new CrtCommand("cornholio",		100,	cmdAction,				0,		"");
	generalCommands["fft"] = new CrtCommand("fft",					100,	cmdAction,				0,		"");
	generalCommands["tease"] = new CrtCommand("tease",				100,	cmdAction,				0,		"");
	generalCommands["bhand"] = new CrtCommand("bhand",				100,	cmdAction,				0,		"");
	generalCommands["beg"] = new CrtCommand("beg",					100,	cmdAction,				0,		"");
	generalCommands["blow"] = new CrtCommand("blow",				100,	cmdAction,				0,		"");
	generalCommands["caress"] = new CrtCommand("caress",			100,	cmdAction,				0,		"");
	generalCommands["cuddle"] = new CrtCommand("cuddle",			100,	cmdAction,				0,		"");
	generalCommands["cower"] = new CrtCommand("cower",				100,	cmdAction,				0,		"");
	generalCommands["disgust"] = new CrtCommand("disgust",			100,	cmdAction,				0,		"");
	generalCommands["embrace"] = new CrtCommand("embrace",			100,	cmdAction,				0,		"");
	generalCommands["fondle"] = new CrtCommand("fondle",			100,	cmdAction,				0,		"");
	generalCommands["gnaw"] = new CrtCommand("gnaw",				100,	cmdAction,				0,		"");
	generalCommands["grope"] = new CrtCommand("grope",				100,	cmdAction,				0,		"");
	generalCommands["grovel"] = new CrtCommand("grovel",			100,	cmdAction,				0,		"");
	generalCommands["listen"] = new CrtCommand("listen",			100,	cmdAction,				0,		"");
	generalCommands["loom"] = new CrtCommand("loom",				100,	cmdAction,				0,		"");
	generalCommands["lust"] = new CrtCommand("lust",				100,	cmdAction,				0,		"");
	generalCommands["massage"] = new CrtCommand("massage",			100,	cmdAction,				0,		"");
	generalCommands["nose"] = new CrtCommand("nose",				100,	cmdAction,				0,		"");
	generalCommands["nibble"] = new CrtCommand("nibble",			100,	cmdAction,				0,		"");
	generalCommands["noogie"] = new CrtCommand("noogie",			100,	cmdAction,				0,		"");
	generalCommands["nudge"] = new CrtCommand("nudge",				100,	cmdAction,				0,		"");
	generalCommands["pinch"] = new CrtCommand("pinch",				100,	cmdAction,				0,		"");
	generalCommands["pummel"] = new CrtCommand("pummel",			100,	cmdAction,				0,		"");
	generalCommands["rub"] = new CrtCommand("rub",					100,	cmdAction,				0,		"");
	generalCommands["ruffle"] = new CrtCommand("ruffle",			100,	cmdAction,				0,		"");
	generalCommands["salute"] = new CrtCommand("salute",			100,	cmdAction,				0,		"");
	generalCommands["scowl"] = new CrtCommand("scowl",				100,	cmdAction,				0,		"");
	generalCommands["seduce"] = new CrtCommand("seduce",			100,	cmdAction,				0,		"");
	generalCommands["shove"] = new CrtCommand("shove",				100,	cmdAction,				0,		"");
	generalCommands["slobber"] = new CrtCommand("slobber",			100,	cmdAction,				0,		"");
	generalCommands["smack"] = new CrtCommand("smack",				100,	cmdAction,				0,		"");
	generalCommands["smell"] = new CrtCommand("smell",				100,	cmdAction,				0,		"");
	generalCommands["snarl"] = new CrtCommand("snarl",				100,	cmdAction,				0,		"");
	generalCommands["sneer"] = new CrtCommand("sneer",				100,	cmdAction,				0,		"");
	generalCommands["snuggle"] = new CrtCommand("snuggle",			100,	cmdAction,				0,		"");
	generalCommands["squint"] = new CrtCommand("squint",			100,	cmdAction,				0,		"");
	generalCommands["stare"] = new CrtCommand("stare",				100,	cmdAction,				0,		"");
	generalCommands["spank"] = new CrtCommand("spank",				100,	cmdAction,				0,		"");
	generalCommands["thumb"] = new CrtCommand("thumb",				100,	cmdAction,				0,		"");
	generalCommands["thwack"] = new CrtCommand("thwack",			100,	cmdAction,				0,		"");
	generalCommands["tip"] = new CrtCommand("tip",					100,	cmdAction,				0,		"");
	generalCommands["tug"] = new CrtCommand("tug",					100,	cmdAction,				0,		"");
	generalCommands["choke"] = new CrtCommand("choke",				100,	cmdAction,				0,		"");
	generalCommands["warn"] = new CrtCommand("warn",				100,	cmdAction,				0,		"");
	generalCommands["fun"] = new CrtCommand("fun",					100,	cmdAction,				0,		"");
	generalCommands["evil"] = new CrtCommand("evil",				100,	cmdAction,				0,		"");
	generalCommands["lol"] = new CrtCommand("lol",					100,	cmdAction,				0,		"");
	generalCommands["bear"] = new CrtCommand("bear",				100,	cmdAction,				0,		"");
	generalCommands["wait"] = new CrtCommand("wait",				100,	cmdAction,				0,		"");
	generalCommands["sneeze"] = new CrtCommand("sneeze",			100,	cmdAction,				0,		"");
	generalCommands["wedgy"] = new CrtCommand("wedgy",				100,	cmdAction,				0,		"");
	generalCommands["cigar"] = new CrtCommand("cigar",				100,	cmdAction,				0,		"");
	generalCommands["zip"] = new CrtCommand("zip",					100,	cmdAction,				0,		"");
	generalCommands["wicked"] = new CrtCommand("wicked",			100,	cmdAction,				0,		"");
	generalCommands["bored"] = new CrtCommand("bored",				100,	cmdAction,				0,		"");
	generalCommands["rag"] = new CrtCommand("rag",					100,	cmdAction,				0,		"");
	generalCommands["trip"] = new CrtCommand("trip",				100,	cmdAction,				0,		"");
	generalCommands["innocent"] = new CrtCommand("innocent",		100,	cmdAction,				0,		"");
	generalCommands["phew"] = new CrtCommand("phew",				100,	cmdAction,				0,		"");
	generalCommands["pester"] = new CrtCommand("pester",			100,	cmdAction,				0,		"");
	generalCommands["usagi"] = new CrtCommand("usagi",				100,	cmdAction,				0,		"");
	generalCommands["doodle"] = new CrtCommand("doodle",			100,	cmdAction,				0,		"");
	generalCommands["whip"] = new CrtCommand("whip",				100,	cmdAction,				0,		"");
	generalCommands["pose"] = new CrtCommand("pose",				100,	cmdAction,				0,		"");
	generalCommands["sniffle"] = new CrtCommand("sniffle",			100,	cmdAction,				0,		"");
	generalCommands["sob"] = new CrtCommand("sob",					100,	cmdAction,				0,		"");
	generalCommands["shuffle"] = new CrtCommand("shuffle",			100,	cmdAction,				0,		"");
	generalCommands["lick"] = new CrtCommand("lick",				100,	cmdAction,				0,		"");
	generalCommands["shush"] = new CrtCommand("shush",				100,	cmdAction,				0,		"");
	// wake is a special command that you might be able to
	// use while unconscious. rules are in Creature::ableToDoCommand
	generalCommands["wake"] = new CrtCommand("wake",				100,	cmdAction,				0,		"Wake up from being asleep.");
	generalCommands["ok"] = new CrtCommand("ok",					100,	cmdAction,				0,		"");
	generalCommands["congrats"] = new CrtCommand("congrats",		100,	cmdAction,				0,		"");
	generalCommands["imitate"] = new CrtCommand("imitate",			100,	cmdAction,				0,		"");
	generalCommands["paranoid"] = new CrtCommand("paranoid",		100,	cmdAction,				0,		"");
	generalCommands["wild"] = new CrtCommand("wild",				100,	cmdAction,				0,		"");
	generalCommands["damn"] = new CrtCommand("damn",				100,	cmdAction,				0,		"");
	generalCommands["fist"] = new CrtCommand("fist",				100,	cmdAction,				0,		"");
	generalCommands["defenestrate"] = new CrtCommand("defenestrate",100,	cmdAction,				0,		"");
	generalCommands["clean"] = new CrtCommand("clean",				100,	cmdAction,				0,		"");
	generalCommands["bathe"] = new CrtCommand("bathe",				100,	cmdAction,				0,		"");
	generalCommands["vomjom"] = new CrtCommand("vomjom",			100,	cmdAction,				0,		"");
	generalCommands["haha"] = new CrtCommand("haha",				100,	cmdAction,				0,		"");
	generalCommands["swat"] = new CrtCommand("swat",				100,	cmdAction,				0,		"");
	generalCommands["crack"] = new CrtCommand("crack",				100,	cmdAction,				0,		"");
	generalCommands["livejournal"] = new CrtCommand("livejournal",	100,	cmdAction,				0,		"");

	generalCommands["dice"] = new CrtCommand("dice",				100,	cmdDice,			0,		"Roll dice.");

	
	// Once we've built the command tables, write their help files to the help directory
	writeCommandFile(0, Path::Help, "commands");
	writeCommandFile(DUNGEONMASTER, Path::DMHelp, "dmcommands");
	writeCommandFile(BUILDER, Path::BuilderHelp, "bhcommands");
	writeSocialFile();

	return(true);
}

void Config::clearCommands() {

	for(std::pair<bstring, PlyCommand*> pc : staffCommands)
		if(pc.second)
			delete pc.second;

	for(std::pair<bstring, PlyCommand*> pc : playerCommands)
		if(pc.second)
			delete pc.second;

	for(std::pair<bstring, CrtCommand*> cc : generalCommands)
		if(cc.second)
			delete cc.second;
}

bool MudMethod::exactMatch(bstring toMatch) {
    return(name.equals(toMatch.c_str(), false));
}

bool MudMethod::partialMatch(bstring toMatch) {
    return(!strncasecmp(name.c_str(), toMatch.c_str(), toMatch.length()));
}

bool MysticMethod::exactMatch(bstring toMatch) {
    return(name.equals(toMatch.c_str(), false));
}

bool MysticMethod::partialMatch(bstring toMatch) {
	for(bstring str : nameParts) {
		if(!strncasecmp(str.c_str(), toMatch.c_str(), toMatch.length())) 
			return true;
	}
	return(false);
}      

void MysticMethod::parseName() {
    boost::char_separator<char> sep(" ");
    boost::tokenizer< boost::char_separator<char> > tokens(name, sep);

    for(bstring t : tokens) {
        nameParts.push_back(t);
    }
}

//*********************************************************************
//						getCommand
//*********************************************************************

// bestMethod is a reference to a pointer
template<class Type, class Type2>
void examineList(std::map<bstring, Type>& myMap, bstring& str, int& match, bool& found, Type2*& bestMethod) {

	Type2* curMethod = 0;

	for(std::pair<bstring, Type> cp : myMap) {
		if(!cp.second) continue;
		curMethod = cp.second; 

		if(curMethod->exactMatch(str)) {
			// Full match, stop here
			bestMethod = curMethod;
			found = true;
			match = 1;
			//std::cout << "Found an exact match!\n";
			break;
		} else if(curMethod->partialMatch(str)) {
			// Partial Match, see how good of a match
			if(bestMethod == 0) {
				// No best match yet, this is our new best match
				bestMethod = curMethod;
				match = 1;
				//std::cout << "First Match: " << bestMethod->getStr() << "'\n";
			} else {
				// Already have a best match, compare it to this to see which is better

				// Lower priority always wins
				if(curMethod->priority < bestMethod->priority) {
					// New winner
					bestMethod = curMethod;
					match = 1;
					//std::cout << "New Best Match: " << bestMethod->getStr() << "'\n";
				} else if(curMethod->priority == bestMethod->priority) {
					// Same priority, check % of match
					int curLen = curMethod->name.length();
					int bestLen = bestMethod->name.length();

					if(curLen < bestLen) {
						// Current command's length is lower, so we match a higher percentage of it.
						// It is the new winner
						//std::cout << "New Best Match: " << bestMethod->getStr() << "'\n";
						bestMethod = curMethod;
						match = 1;
					} else if(curLen == bestLen) {
						// Drats we have a tie
						match++;
						//std::cout << "Tied!\n";
					} else {
						// Best command still wins, do nothing
					}
				}
			}
		}
	}
}

void getCommand(Creature *user, cmd* cmnd) {
	bstring str = "";
	int		match=0;
	bool	found=false;
//	int		i=0;

	Player* pUser = user->getPlayer();
	str = cmnd->str[0];

	//std::cout << "Looking for a match for '" << str << "'\n";

	examineList<CrtCommand*, Command>(gConfig->generalCommands, str, match, found, cmnd->myCommand);

	// If we're a player, examine a few extra lists
	if(pUser) {
		if(!found) // No exact match so far, check player list
			examineList<PlyCommand*, Command>(gConfig->playerCommands, str, match, found, cmnd->myCommand);
		if(!found && pUser->isStaff()) // Still no exact match, check staff commands
			examineList<PlyCommand*, Command>(gConfig->staffCommands, str, match, found, cmnd->myCommand);
	}

	if(!match)
		cmnd->ret = CMD_NOT_FOUND;
	else if(match > 1)
		cmnd->ret = CMD_NOT_UNIQUE;
	else if(cmnd->myCommand->auth && !cmnd->myCommand->auth(user))
		cmnd->ret = CMD_NOT_AUTH;
	else
		cmnd->ret = 0;
}



//*********************************************************************
//						getSpell
//*********************************************************************

Spell *Config::getSpell(bstring id, int& ret) {
	Spell *toReturn = 0;
	int match = 0;
	bool found = false;

	examineList<Spell*, Spell>(gConfig->spells, id, match, found, toReturn);

	if(!match)
		ret = CMD_NOT_FOUND;
	else if(match > 1)
		ret = CMD_NOT_UNIQUE;
	else
		ret = 0;


	return(toReturn);
}

//*********************************************************************
//						getSong
//*********************************************************************

Song *Config::getSong(bstring pName) {
	Song *toReturn = 0;
	int match = 0;
	bool found = false;

	examineList<Song*, Song>(gConfig->songs, pName, match, found, toReturn);

	return(toReturn);
}

Song *Config::getSong(bstring name, int& ret){
	Song *toReturn = 0;
	int match = 0;
	bool found = false;

	examineList<Song*, Song>(gConfig->songs, name, match, found, toReturn);

	if(!match)
		ret = CMD_NOT_FOUND;
	else if(match > 1)
		ret = CMD_NOT_UNIQUE;
	else
		ret = 0;

	return(toReturn);
}

//*********************************************************************
//						allowedWhilePetrified
//*********************************************************************

int allowedWhilePetrified(bstring str) {
	return(	str == "quit" ||
			str == "score" || str == "sc" || str == "health" ||
			str == "set" || str == "clear" || str == "toggle" || str == "prefs" ||
			str == "time" ||
			str == "info" ||
			str == "who" ||
			str == "whois" ||
			str == "finger" ||
			str == "help" ||
			str == "pass" || str == "passwd" || str == "password"
	);
}


//*********************************************************************
//						cmdProcess
//*********************************************************************
// This function takes the command structure of the person at the socket
// in the first parameter and interprets the person's command.

int cmdProcess(Creature *user, cmd* cmnd, Creature* pet) {
	Player	*player=0;
	int		fd = user->fd;

	if(fd > 0) {
		if(!pet) {
			player = user->getPlayer();
		} else
			user = pet;
	}

	if(player && player->afterProf) {
		broadcast(isDm, "^RafterProf field set on %s. Last command: %s", player->name, player->getLastCommand().c_str());
		broadcast(isDm, "^RResetting afterProf to 0.");
		player->afterProf = 0;
	}
	if(!user) {
		printf("invalid creature trying to use cmdProcess.\n");
		return(0);
	}

	getCommand(user, cmnd);

	if(cmnd->ret == CMD_NOT_FOUND) {
		if(pet) {
			user->print("Tell %N to do what?\n", user);
		} else if(player)
			return(cmdNoExist(player, cmnd));
		return(0);
	} else if(cmnd->ret == CMD_NOT_UNIQUE) {
		user->print("Command is not unique.\n");
		return(0);
	} else if(cmnd->ret == CMD_NOT_AUTH) {
		if(player)
			return(cmdNoAuth(player));
		return(0);
	}

	if(player && !player->isStaff() && player->isEffected("petrification") && !allowedWhilePetrified(cmnd->myCommand->getName())) {
		player->print("You can't do that. You're petrified!\n");
		return(0);
	}

	cmnd->ret = cmnd->myCommand->execute(user, cmnd);

	return(cmnd->ret);
}

bstring MudMethod::getName() const {
	return(name);
}

bstring MudMethod::getDesc() const {
	return(desc);
}

int PlyCommand::execute(Creature* player, cmd* cmnd) {
	Player *ply = player->getPlayer();
	if(!ply)
		return(false);
	return((fn)(ply, cmnd));
}

int CrtCommand::execute(Creature* player, cmd* cmnd) {
	return((fn)(player, cmnd));
}

const bstring& MysticMethod::getScript() const {
	return(script);
}
