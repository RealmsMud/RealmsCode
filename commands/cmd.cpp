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
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <cstdio>                     // for snprintf, sprintf
#include <strings.h>                  // for strncasecmp
#include <unistd.h>                   // for link
#include <fstream>                    // for ofstream, operator<<, basic_ost...
#include <iomanip>                    // for operator<<, setw
#include <map>                        // for map, operator==, operator!=
#include <string>                     // for operator<<, allocator, operator==
#include <utility>                    // for pair
#include <boost/tokenizer.hpp>


#include "bstring.hpp"                // for bstring
#include "cmd.hpp"                    // for cmd, CMD_NOT_UNIQUE, CMD_NOT_FOUND
#include "commands.hpp"               // for cmdAction, channel, cmdMove
#include "config.hpp"                 // for Config, PlyCommandMap, CrtComma...
#include "creatureStreams.hpp"        // for Streamable
#include "creatures.hpp"              // for Player, Creature
#include "dm.hpp"                     // for dmKillSwitch, builderMob, build...
#include "global.hpp"                 // for CreatureClass, CreatureClass::NONE
#include "help.hpp"                   // for loadHelpTemplate
#include "os.hpp"                     // for ASSERTLOG
#include "paths.hpp"                  // for Help, BuilderHelp, DMHelp
#include "proto.hpp"                  // for isDm, isCt, cmdGo, broadcast
#include "server.hpp"                 // for Server, gServer
#include "ships.hpp"                  // for cmdQueryShips
#include "skills.hpp"                 // for SkillCommand
#include "socials.hpp"                // for SocialCommand
#include "songs.hpp"                  // for Song
#include "structs.hpp"                // for PlyCommand, Command, CrtCommand

class MudObject;



int dmTest(Player* player, cmd* cmnd) {
    *player << gConfig->getProxyList();

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
    MudObject *target = nullptr;

    //target = player->getParent()->findTarget(cmnd->fullstr);

    gServer->runPython(spell->script, args, player, target);
    return(0);
}




//**********************************************************************
//                      compileSocialList
//**********************************************************************

template<class Type>
void compileSocialList(std::map<bstring, bstring>& list, const std::map<bstring, Type, comp>& cList) {
    bstring name = "";

    for(const auto& pp : cList) {
        ASSERTLOG(pp.second);
        Command* cmnd = pp.second;

        auto* cc = dynamic_cast<CrtCommand*>(cmnd);
        auto* pc = dynamic_cast<PlyCommand*>(cmnd);
        auto* sc = dynamic_cast<SocialCommand*>(cmnd);

        if((!cc || cc->fn != cmdAction) &&
           (!pc || pc->fn != plyAction) &&
           !sc )
            continue;

        name = cmnd->getName();

        if(name == "vomjom" || name == "usagi" || name == "defenestrate")
            continue;

        list[name] = "";
    }

    }

//**********************************************************************
//                      writeSocialFile
//**********************************************************************
// Updates helpfiles for the game's socials.

bool Config::writeSocialFile() const {
    char    file[100], fileLink[100];
    std::map<bstring,bstring> list;
    std::map<bstring,bstring>::iterator it;

    sprintf(file, "%s/socials.txt", Path::Help);
    sprintf(fileLink, "%s/social.txt", Path::Help);

    // prepare to write the help file
    std::ofstream out(file);
    out.setf(std::ios::left, std::ios::adjustfield);
    out.imbue(std::locale(""));

    out << Help::loadHelpTemplate("socials");


    compileSocialList<PlyCommand*>(list, playerCommands);
    compileSocialList<CrtCommand*>(list, generalCommands);
    compileSocialList<SocialCommand*>(list, socials);


    int i = 0;
    for(it = list.begin(); it != list.end(); it++) {
        if(i && !(i%6))
            out << "\n";
        out << std::setw(13) << (*it).first;
        i++;
    }

    out << "\n";
    

    out << Help::loadHelpTemplate("socials.post");

    out.close();
    link(file, fileLink);
    return(true);
}

//**********************************************************************
//                      compileCommandList
//**********************************************************************
// Updates helpfiles for the various game commands.

template<class Type>
std::map<bstring,bstring> compileCommandList(std::map<bstring, bstring>& list, CreatureClass cls, std::map<bstring, Type, comp>& cList) {
    char    line[100];
    line[99] = '\0';

    for(const auto& pp : cList) {
        ASSERTLOG(pp.second);
        Command* cmd = pp.second;
        
        if(cmd->getDescription().empty())
            continue;
        if( cls == CreatureClass::BUILDER &&
            cmd->auth &&
            cmd->auth != builderMob &&
            cmd->auth != builderObj
        )
            continue;

        snprintf(line, 99, " ^W%-19s^x %s\n", cmd->getName().c_str(), cmd->getDescription().c_str());
        list[cmd->getName()] = line;
    }

    return(list);
}

//**********************************************************************
//                      writeCommandFile
//**********************************************************************
// Updates helpfiles for the various game commands.

void writeCommandFile(CreatureClass cls, const char* path, const char* tpl) {
    char    file[100], fileLink[100];
    file[99] = fileLink[99] = '\0';

    std::map<bstring,bstring> list;
    std::map<bstring,bstring> list2;
    std::map<bstring,bstring>::iterator it;

    snprintf(file, 99, "%s/commands.txt", path);
    snprintf(fileLink, 99, "%s/command.txt", path);

    // prepare to write the help file
    std::ofstream out(file);
    out.setf(std::ios::left, std::ios::adjustfield);
    out.imbue(std::locale(""));

    out << Help::loadHelpTemplate(tpl);


    if(cls == CreatureClass::DUNGEONMASTER || cls == CreatureClass::BUILDER) {
        compileCommandList<PlyCommand*>(list, cls, gConfig->staffCommands);
    } else {
        compileCommandList<PlyCommand*>(list, CreatureClass::NONE, gConfig->playerCommands);
        compileCommandList<CrtCommand*>(list, CreatureClass::NONE, gConfig->generalCommands);
        compileCommandList<SkillCommand*>(list, CreatureClass::NONE, gConfig->skillCommands);
    }

    for(it = list.begin(); it != list.end(); it++) {
        out << (*it).second;
    }

    out << "\n";
    out.close();
    link(file, fileLink);
}


//**********************************************************************
//                      initCommands
//**********************************************************************
// everything on the staff array already checks isStaff - the listed
// authorization is an additional requirement to use the command.

bool Config::initCommands() {
// *************************************************************************************
// Staff Commands

    staffCommands["play"] = new PlyCommand("play",                  100,    cmdPlay,            isCt,   "Play a song.");
    staffCommands["*cache"] = new PlyCommand("*cache",              100,    dmCache,            isDm,   "Show DNS cache.");
    staffCommands["*test"] = new PlyCommand("*test",                100,    dmTest,             isDm,   "");
    staffCommands["pcast"] = new PlyCommand("pcast",                100,    pcast,              isDm,   "");

    // channels
//  staffCommands["*s"] = new PlyCommand("*s",                      100,    channel,            isCt,   "");
    staffCommands["*send"] = new PlyCommand("*send",                 10,    channel,            isCt,   "");
    staffCommands["dm"] = new PlyCommand("dm",                      100,    channel,            isDm,   "");
    staffCommands["admin"] = new PlyCommand("admin",                100,    channel,            isDm,   "");
    staffCommands["dmrace"] = new PlyCommand("dmrace",              100,    channel,            isCt,   "Use specified race channel.");
    staffCommands["dmclass"] = new PlyCommand("dmclass",            100,    channel,            isCt,   "Use specified class channel.");
    staffCommands["dmcls"] = new PlyCommand("dmcls",                100,    channel,            isCt,   "");
    staffCommands["dmclan"] = new PlyCommand("dmclan",              100,    channel,            isCt,   "Use specified clan channel.");
    staffCommands["dmguild"] = new PlyCommand("dmguild",            100,    channel,            isCt,   "Use specified guild channel.");
    staffCommands["*msg"] = new PlyCommand("*msg",                  100,    channel,            nullptr,      "");

    // dm.c
    staffCommands["*reboot"] = new PlyCommand("*reboot",            100,    dmReboot,           isCt,   "Reboot the mud.");
    staffCommands["*inv"] = new PlyCommand("*inv",                  100,    dmMobInventory,     nullptr,      "");
    staffCommands["*sockets"] = new PlyCommand("*sockets",          100,    dmSockets,          isDm,   "Show all connected sockets.");
    staffCommands["*dmload"] = new PlyCommand("*dmload",            100,    dmLoad,             isDm,   "Reload configuration files.");
    staffCommands["*txtoncrash"] = new PlyCommand("*txtoncrash",    100,    dmTxtOnCrash,       isDm,   "Toggle whether the mud sends a text message when it crashes.");
    staffCommands["*dmsave"] = new PlyCommand("*dmsave",            100,    dmSave,             isDm,   "Resave configuration files.");
    staffCommands["*teleport"] = new PlyCommand("*teleport",         10,    dmTeleport,         nullptr,      "Teleport to rooms or players, or teleport players to each other.");
    //staffCommands["*t"] = new PlyCommand("*t",                        100,    dmTeleport,         0,      "");
    staffCommands["*users"] = new PlyCommand("*users",               10,    dmUsers,            isCt,   "See a list of connected users.");
    //staffCommands["*u"] = new PlyCommand("*u",                        100,    dmUsers,            isCt,   "");
    staffCommands["*shutdown"] = new PlyCommand("*shutdown",        100,    dmShutdown,         isDm,   "Shut down the mud (may auto-restart).");
    staffCommands["*flushrooms"] = new PlyCommand("*flushrooms",    100,    dmFlushSave,        isDm,   "Flush rooms from memory.");
    staffCommands["*flushcrtobj"] = new PlyCommand("*flushcrtobj",  100,    dmFlushCrtObj,      nullptr,      "Flush creatures and objects from memory.");
    staffCommands["*save"] = new PlyCommand("*save",                100,    dmResave,           nullptr,      "Save the room, creature, or object.");
    staffCommands["*perm"] = new PlyCommand("*perm",                100,    dmPerm,             nullptr,      "");
    //staffCommands["*i"] = new PlyCommand("*i",                        100,    dmInvis,            0,      "");
    staffCommands["*invis"] = new PlyCommand("*invis",               10,    dmInvis,            nullptr,      "Toggle DM invis (only staff can see you).");
    staffCommands["*incog"] = new PlyCommand("*incog",               20,    dmIncog,            nullptr,      "");
    staffCommands["incog"] = new PlyCommand("incog",                100,    dmIncog,            nullptr,      "Toggle incognito status (only people in the room can see you).");
    staffCommands["*ac"] = new PlyCommand("*ac",                    100,    dmAc,               nullptr,      "Show your Weapon/Defense/Armor and completely heal yourself.");
    staffCommands["*wipe"] = new PlyCommand("*wipe",                100,    dmWipe,             nullptr,      "");
    staffCommands["*clear"] = new PlyCommand("*clear",              100,    dmDeleteDb,         isDm,   "");
    staffCommands["*gamestat"] = new PlyCommand("*gamestat",        100,    dmGameStatus,       isDm,   "Show configuration options.");
    staffCommands["*weather"] = new PlyCommand("*weather",          100,    dmWeather,          nullptr,      "Show weather strings.");
    staffCommands["*questlist"] = new PlyCommand("*questlist",      100,    dmQuestList,        isCt,   "Show quests in memory.");
    staffCommands["*alchemylist"] = new PlyCommand("*alchemylist",  100,    dmAlchemyList,      isCt,   "Show the Alchemy List.");
    staffCommands["*clanlist"] = new PlyCommand("*clanlist",        100,    dmClanList,         isCt,   "Show clans in memory.");
    staffCommands["*bane"] = new PlyCommand("*bane",                500,    dmBane,             isDm,   "Crash the game");
    staffCommands["*dmhelp"] = new PlyCommand("*dmhelp",             50,    dmHelp,             isCt,   "Access help files.");
    staffCommands["*bhelp"] = new PlyCommand("*bhelp",               50,    bhHelp,             nullptr,      "Access help files.");
    staffCommands["*fishing"] = new PlyCommand("*fishing",          100,    dmFishing,          nullptr,      "View fishing lists.");
    staffCommands["*parameter"] = new PlyCommand("*parameter",      100,    dmParam,            isDm,   "");
    staffCommands["*outlaw"] = new PlyCommand("*outlaw",            100,    dmOutlaw,           isCt,   "Make a player an outlaw.");
    staffCommands["*broadcast"] = new PlyCommand("*broadcast",       20,    dmBroadecho,        isCt,   "Broadcast a message to all players.");
    staffCommands["*gcast"] = new PlyCommand("*gcast",              100,    dmCast,             isCt,   "Cast a spell on all players.");
    staffCommands["*set"] = new PlyCommand("*set",                  100,    dmSet,              nullptr,      "Modify a room/object/exit/player/object/monster.");
    staffCommands["*log"] = new PlyCommand("*log",                  100,    dmLog,              isCt,   "");
    staffCommands["*list"] = new PlyCommand("*list",                100,    dmList,             isCt,   "");
    staffCommands["*info"] = new PlyCommand("*info",                100,    dmInfo,             isCt,   "Show game info (includes some memory).");
    staffCommands["*md5"] = new PlyCommand("*md5",                  100,    dmMd5,              isCt,   "Show md5 of input string.");
    staffCommands["*ids"] = new PlyCommand("*ids",                  100,    dmIds,              isDm,   "Shows registered ids.");
    staffCommands["*status"] = new PlyCommand("*status",             80,    dmStat,             nullptr,      "Show info about a room/player/object/monster.");
    staffCommands["*sd"] = new PlyCommand("*sd",                    100,    dmStatDetail,       isCt,   "Show detailed information about a creature's stats.");


    // dmcrt.c
    staffCommands["*monster"] = new PlyCommand("*monster",          100,    dmCreateMob,        builderMob, "Summon a monster.");
    staffCommands["*cname"] = new PlyCommand("*cname",              100,    dmCrtName,          builderMob, "Rename a monster.");
    staffCommands["*possess"] = new PlyCommand("*possess",          100,    dmAlias,            isCt,   "Possess a monster.");
    staffCommands["*cfollow"] = new PlyCommand("*cfollow",          100,    dmFollow,           isDm,   "Make a monster follow you.");
    staffCommands["*attack"] = new PlyCommand("*attack",            100,    dmAttack,           builderMob, "Make a monster attack a player/monster.");
    staffCommands["*enemy"] = new PlyCommand("*enemy",              100,    dmListEnemy,        isCt,   "Show the enemies of a monster.");
    staffCommands["*charm"] = new PlyCommand("*charm",              100,    dmListCharm,        isCt,   "Show the charm list of a player.");
    staffCommands["*wander"] = new PlyCommand("*wander",            100,    dmForceWander,      builderMob, "Force a monster to wander away.");
    staffCommands["*balance"] = new PlyCommand("*balance",           95,    dmBalance,          builderMob, "Balance a monster to a certain level.");


    // dmobj.c
    staffCommands["*create"] = new PlyCommand("*create",             20,    dmCreateObj,        builderObj, "Summon an object.");
    staffCommands["*object"] = new PlyCommand("*object",             20,    dmCreateObj,        builderObj, "Summon an object.");
    staffCommands["*oname"] = new PlyCommand("*oname",              100,    dmObjName,          builderObj, "Rename an object");
    staffCommands["*size"] = new PlyCommand("*size",                100,    dmSize,             builderObj, "");
    staffCommands["*clone"] = new PlyCommand("*clone",              100,    dmClone,            builderObj, "Create an armor set from a single piece of armor.");


    // dmply.c
    staffCommands["*force"] = new PlyCommand("*force",               20,    dmForce,            isDm,   "Force a player to perform an action.");
    staffCommands["*spy"] = new PlyCommand("*spy",                  100,    dmSpy,              isCt,   "Spy on a player.");
    staffCommands["*silence"] = new PlyCommand("*silence",          100,    dmSilence,          isCt,   "Toggle broadcast silence on a player.");
    staffCommands["*title"] = new PlyCommand("*title",              100,    dmTitle,            isCt,   "Set a title on a player.");
    staffCommands["*surname"] = new PlyCommand("*surname",          100,    dmSurname,          isCt,   "Set a surname on a player.");
    staffCommands["*group"] = new PlyCommand("*group",              100,    dmGroup,            isCt,   "See who is grouped with a player.");
    staffCommands["*dust"] = new PlyCommand("*dust",                100,    dmDust,             isCt,   "Permanently delete a player.");
    staffCommands["*tell"] = new PlyCommand("*tell",                100,    dmFlash,            isDm,   "Send a player a message.");
    staffCommands["*award"] = new PlyCommand("*award",              100,    dmAward,            isCt,   "Award a player roleplaying exp/gold.");
    staffCommands["*beep"] = new PlyCommand("*beep",                100,    dmBeep,             isDm,   "Make the player's terminal beep.");
    staffCommands["*advance"] = new PlyCommand("*advance",          100,    dmAdvance,          isDm,   "Change the player's level and adjust stats appropriately.");
    staffCommands["dmfinger"] = new PlyCommand("dmfinger",          100,    dmFinger,           isDm,   "fup an offline player's information.");
    staffCommands["*disconnect"] = new PlyCommand("*disconnect",    100,    dmDisconnect,       isCt,   "Disconnect a player.");
    staffCommands["*take"] = new PlyCommand("*take",                100,    dmTake,             isCt,   "Take an item from a player.");
    staffCommands["*remove"] = new PlyCommand("*remove",            100,    dmRemove,           isCt,   "Unequip and take an item from a player.");
    staffCommands["*put"] = new PlyCommand("*put",                  100,    dmPut,              isCt,   "Put an item into a player's inventory.");
    staffCommands["*move"] = new PlyCommand("*move",                100,    dmMove,             isCt,   "Move and offline player.");
    staffCommands["*word"] = new PlyCommand("*word",                100,    dmWordAll,          isDm,   "Send all players to their recall room.");
    staffCommands["*pass"] = new PlyCommand("*pass",                100,    dmPassword,         isDm,   "Change a player's password.");
    staffCommands["*restore"] = new PlyCommand("*restore",          100,    dmRestorePlayer,    isDm,   "Restore a player's experience.");
    staffCommands["*bank"] = new PlyCommand("*bank",                100,    dmBank,             isCt,   "");
    staffCommands["*assets"] = new PlyCommand("*assets",            100,    dmInventoryValue,   isCt,   "");
    staffCommands["*proxy"] = new PlyCommand("*proxy",              100,    dmProxy,            isCt,   "Allow a player to double log.");
    staffCommands["*2x"] = new PlyCommand("*2x",                    100,    dm2x,               isCt,   "Allow two accounts to double log.");
    staffCommands["*bug"] = new PlyCommand("*bug",                  100,    dmBugPlayer,        isCt,   "Record a player's actions.");
    staffCommands["*kill"] = new PlyCommand("*kill",                100,    dmKillSwitch,       isDm,   "Kill a player.");
    staffCommands["*nomnom"] = new PlyCommand("*nomnom",            100,    dmKillSwitch,       isDm,   "Kill a player.");
    staffCommands["*combust"] = new PlyCommand("*combust",          100,    dmKillSwitch,       isDm,   "Kill a player.");
    staffCommands["*slime"] = new PlyCommand("*slime",              100,    dmKillSwitch,       isDm,   "Kill a player.");
    staffCommands["*gnat"] = new PlyCommand("*gnat",                100,    dmKillSwitch,       isDm,   "Kill a player.");
    staffCommands["*trip"] = new PlyCommand("*trip",                100,    dmKillSwitch,       isDm,   "Kill a player.");
    staffCommands["*crush"] = new PlyCommand("*crush",              100,    dmKillSwitch,       isDm,   "Kill a player.");
    staffCommands["*missile"] = new PlyCommand("*missile",          100,    dmKillSwitch,       isDm,   "Kill a player.");
    staffCommands["*bomb"] = new PlyCommand("*bomb",                100,    dmKillSwitch,       isDm,   "Kill a player and everyone in room.");
    staffCommands["*nuke"] = new PlyCommand("*nuke",                100,    dmKillSwitch,       isDm,   "Kill a player and everyone in room.");
    staffCommands["*arma"] = new PlyCommand("*arma",                100,    dmKillSwitch,       isDm,   "");
    staffCommands["*armageddon"] = new PlyCommand("*armageddon",    100,    dmKillSwitch,       isDm,   "Kill all players.");
    staffCommands["*supernova"] = new PlyCommand("*supernova",      100,    dmKillSwitch,       isDm,   "Kill all players.");
    staffCommands["*igmoo"] = new PlyCommand("*igmoo",              100,    dmKillSwitch,       isDm,   "Kill all players.");
    staffCommands["*rape"] = new PlyCommand("*rape",                100,    dmKillSwitch,       isDm,   "Set a player to 1hp and 1mp.");
    staffCommands["*drain"] = new PlyCommand("*drain",              100,    dmKillSwitch,       isDm,   "Set a player to 1hp and 1mp.");
    staffCommands["*repair"] = new PlyCommand("*repair",            100,    dmRepair,           isDm,   "Fix a player's armor.");
    staffCommands["*max"] = new PlyCommand("*max",                  100,    dmMax,              nullptr,      "Set your stats to max.");
    staffCommands["*backup"] = new PlyCommand("*backup",            100,    dmBackupPlayer,     isDm,   "Backup a player or restore a player from backup.");
    staffCommands["*changestats"] = new PlyCommand("*changestats",  100,    dmChangeStats,      isCt,   "Allow a player to pick new stats.");
    staffCommands["*lt"] = new PlyCommand("*lt",                    100,    dmLts,              isDm,   "Show a player's last times.");
    staffCommands["*ltclear"] = new PlyCommand("*ltclear",          100,    dmLtClear,          isDm,   "Clear a player's last times.");


    // dmroom.c
    staffCommands["*swap"] = new PlyCommand("*swap",                100,    dmSwap,             nullptr,      "Swap utility.");
    staffCommands["*rswap"] = new PlyCommand("*rswap",              100,    dmRoomSwap,         nullptr,      "Swap current room with another.");
    staffCommands["*oswap"] = new PlyCommand("*oswap",              100,    dmObjSwap,          nullptr,      "Swap object with another.");
    staffCommands["*mswap"] = new PlyCommand("*mswap",              100,    dmMobSwap,          nullptr,      "Swap monster with another.");
    staffCommands["*purge"] = new PlyCommand("*purge",              100,    dmPurge,            nullptr,      "Purge all objects and monsters (not pets) in room.");
    staffCommands["*echo"] = new PlyCommand("*echo",                100,    dmEcho,             nullptr,      "Send a message to all players in the room.");
    staffCommands["*reload"] = new PlyCommand("*reload",            100,    dmReloadRoom,       nullptr,      "Reload the room from disk.");
    staffCommands["*reset"] = new PlyCommand("*reset",              100,    dmResetPerms,       nullptr,      "Reset perm timeouts.");
    staffCommands["*add"] = new PlyCommand("*add",                  100,    dmAddRoom,          nullptr,      "Create a new room/object/monster.");
    staffCommands["*replace"] = new PlyCommand("*replace",          100,    dmReplace,          nullptr,      "Replace text in a room description.");
    staffCommands["*substitute"] = new PlyCommand("*substitute",    100,    dmReplace,          nullptr,      "");
    staffCommands["*delete"] = new PlyCommand("*delete",            100,    dmDelete,           nullptr,      "Delete text from a room description.");
    staffCommands["*name"] = new PlyCommand("*name",                100,    dmNameRoom,         nullptr,      "Rename the room.");
    staffCommands["*append"] = new PlyCommand("*append",            100,    dmAppend,           nullptr,      "Add text to a room description.");
    staffCommands["*app"] = new PlyCommand("*app",                  100,    dmAppend,           nullptr,      "");
    staffCommands["*prepend"] = new PlyCommand("*prepend",          100,    dmPrepend,          nullptr,      "Prepend text to a room description.");
    staffCommands["*moblist"] = new PlyCommand("*moblist",          100,    dmMobList,          builderMob, "List wandering monsters in the room.");
    staffCommands["*wrap"] = new PlyCommand("*wrap",                100,    dmWrap,             nullptr,      "Wrap the room description to a number of characters.");
    staffCommands["*xdelete"] = new PlyCommand("*xdelete",          100,    dmDeleteAllExits,   nullptr,      "Delete all exits in the room.");
    staffCommands["*xarrange"] = new PlyCommand("*xarrange",        100,    dmArrangeExits,     nullptr,      "Arrange all the exits in the room according to standards.");
    staffCommands["*fixup"] = new PlyCommand("*fixup",              100,    dmFixExit,          nullptr,      "Remove an underscore from an exit.");
    staffCommands["*unfixup"] = new PlyCommand("*unfixup",          100,    dmUnfixExit,        nullptr,      "Add an underscore to an exit.");
    staffCommands["*xrename"] = new PlyCommand("*xrename",          100,    dmRenameExit,       nullptr,      "Rename an exit.");
    staffCommands["*destroy"] = new PlyCommand("*destroy",          100,    dmDestroyRoom,      nullptr,      "Destroy the room.");
    staffCommands["*find"] = new PlyCommand("*find",                100,    dmFind,             nullptr,      "Find the next available room/object/monster.");


    // post.c
    staffCommands["*readmail"] = new PlyCommand("*readmail",        100,    dmReadmail,         isDm,   "Read a player's mail.");
    staffCommands["*rmmail"] = new PlyCommand("*rmmail",            100,    dmDeletemail,       isDm,   "Delete a player's mail.");
    staffCommands["*notepad"] = new PlyCommand("*notepad",          100,    notepad,            nullptr,      "Access your staff notepad.");
    staffCommands["*rmhist"] = new PlyCommand("*rmhist",            100,    dmDeletehist,       isDm,   "Delete a player's history.");


    // skills.c
    staffCommands["*skills"] = new PlyCommand("*skills",            100,    dmSkills,           isDm,   "Show a list of skills / a player/monsters's skills.");
    staffCommands["*setskills"] = new PlyCommand("*setskills",      100,    dmSetSkills,        isDm,   "Set a player's skills to an appropriate level.");
//  staffCommands["*addskills"] = new PlyCommand("*addskills",      100,    dmAddSkills         0,      "");


    // guilds.c
    staffCommands["*guildlist"] = new PlyCommand("*guildlist",      100,    dmListGuilds,       isDm,   "Show all guilds.");
    staffCommands["*approve"] = new PlyCommand("*approve",          100,    dmApproveGuild,     isCt,   "Approve a proposed guild.");
    staffCommands["*reject"] = new PlyCommand("*reject",            100,    dmRejectGuild,      isCt,   "Reject a proposed guild.");


    // bans.c
    staffCommands["*ban"] = new PlyCommand("*ban",                   60,    dmBan,              isCt,   "Ban a player.");
    staffCommands["*unban"] = new PlyCommand("*unban",              100,    dmUnban,            isCt,   "Delete a ban.");
    staffCommands["*banlist"] = new PlyCommand("*banlist",          100,    dmListbans,         isDm,   "Show a list of bans.");


    // various files
    staffCommands["*fifo"] = new PlyCommand("*fifo",                100,    dmFifo,             nullptr,      "Delete and recreate the web interface fifos.");
    staffCommands["*locations"] = new PlyCommand("*locations",      100,    dmStartLocs,        isCt,   "Show starting locations.");
    staffCommands["*specials"] = new PlyCommand("*specials",        100,    dmSpecials,         nullptr,      "Show a monster's special attacks.");
    staffCommands["*range"] = new PlyCommand("*range",              100,    dmRange,            nullptr,      "Show your assigned room range.");
    staffCommands["*builder"] = new PlyCommand("*builder",          100,    dmMakeBuilder,      isCt,   "Make a new character a builder.");
    staffCommands["*wat"] = new PlyCommand("*wat",                  100,    dmWatcherBroad,     isCt,   "Force a watcher to broadcast.");
    staffCommands["*demographics"] = new PlyCommand("*demographics",100,    cmdDemographics,    isDm,   "Start the demographics routine.");
    staffCommands["*unique"] = new PlyCommand("*unique",            100,    dmUnique,           isCt,   "Work with uniques.");
    staffCommands["*shipquery"] = new PlyCommand("*shipquery",      100,    dmQueryShips,       isDm,   "See data about ships.");
    staffCommands["*spelling"] = new PlyCommand("*spelling",         50,    dmSpelling,         nullptr,      "Spell-check a room.");
    staffCommands["*spells"] = new PlyCommand("*spells",            100,    dmSpellList,        nullptr,      "List spells in the game.");
    staffCommands["*effects"] = new PlyCommand("*effects",          90,     dmEffectList,       nullptr,      "List effects in the game.");
    staffCommands["*effindex"] = new PlyCommand("*effindex",        100,    dmShowEffectsIndex, isCt,   "List rooms in the effects index.");
    staffCommands["*songs"] = new PlyCommand("*songs",              100,    dmSongList,         nullptr,      "List songs in the game.");
    staffCommands["*lottery"] = new PlyCommand("*lottery",          100,    dmLottery,          isDm,   "Run the lottery.");
    staffCommands["*memory"] = new PlyCommand("*memory",            100,    dmMemory,           isCt,   "Show memory usage.");
    staffCommands["*active"] = new PlyCommand("*active",            100,    list_act,           isCt,   "Show monsters on the active list.");
    staffCommands["*classlist"] = new PlyCommand("*classlist",      100,    dmShowClasses,      nullptr,      "List all classes.");
    staffCommands["*racelist"] = new PlyCommand("*racelist",        100,    dmShowRaces,        nullptr,      "List all races.");
    staffCommands["*deitylist"] = new PlyCommand("*deitylist",      100,    dmShowDeities,      nullptr,      "List all deities.");
    staffCommands["*faction"] = new PlyCommand("*faction",          100,    dmShowFactions,     nullptr,      "List all factions or show a player/monster's faction.");
    staffCommands["bfart"] = new PlyCommand("bfart",                100,    plyAction,          isDm,   "");
    staffCommands["*arealist"] = new PlyCommand("*arealist",        100,    dmListArea,         isCt,   "List area information.");
    staffCommands["*recipes"] = new PlyCommand("*recipes",          100,    dmRecipes,          nullptr,      "List recipes.");
    staffCommands["*combine"] = new PlyCommand("*combine",          100,    dmCombine,          nullptr,      "Create new recipes.");
    staffCommands["*property"] = new PlyCommand("*property",        100,    dmProperties,       isCt,   "List properties.");
    staffCommands["*catrefinfo"] = new PlyCommand("*catrefinfo",    100,    dmCatRefInfo,       nullptr,      "Show CatRefInfo.");

    // unused functions
    //staffCommands["*task"] = new PlyCommand("*task",          100,    dmTask,             0,      "");
    //staffCommands["boards"] = new PlyCommand("boards",            100,    listboards,         0,      "");
    //staffCommands["bug"] = new PlyCommand("bug",              100,    bugReport,          0,      "");
    //staffCommands["petition"] = new PlyCommand("petition",        100,    plyPetition,        0,      "");
    //staffCommands["*message"] = new PlyCommand("*message",        100,    dmMsg,              0,      "");
    //staffCommands["gamble"] = new PlyCommand("gamble",            100,    cmdGamble,          0,      "");
    //staffCommands["*wlog"] = new PlyCommand("*wlog",          100,    watcher_log,        0,      "");

// *************************************************************************************
// Player Commands

    playerCommands["target"] = new PlyCommand("target",             100,    cmdTarget,          nullptr,      "Target a creature or player." );
    playerCommands["assist"] = new PlyCommand("assist",             100,    cmdAssist,          nullptr,      "Assist a player, targeting their target." );
    // watcher functions
    playerCommands["*rename"] = new PlyCommand("*rename",           100,    dmRename,           nullptr,      "");
    playerCommands["*warn"] = new PlyCommand("*warn",               100,    dmWarn,             nullptr,      "");
    playerCommands["*jail"] = new PlyCommand("*jail",               100,    dmJailPlayer,       nullptr,      "");
    playerCommands["*checkstats"] = new PlyCommand("*checkstats",   100,    dmCheckStats,       nullptr,      "");
    playerCommands["*locate"] = new PlyCommand("*locate",           100,    dmLocatePlayer,     nullptr,      "");
    playerCommands["*typo"] = new PlyCommand("*typo",               100,    reportTypo,         nullptr,      "");
    playerCommands["*gag"] = new PlyCommand("*gag",                 100,    dmGag,              nullptr,      "");
    playerCommands["*wts"] = new PlyCommand("*wts",                 100,    channel,            nullptr,      "");

    playerCommands["compare"] = new PlyCommand("compare",           90,    cmdCompare,         nullptr,      "Compare two items.");
    playerCommands["weapons"] = new PlyCommand("weapons",           100,    cmdWeapons,         nullptr,      "Use weapon trains.");
    playerCommands["fish"] = new PlyCommand("fish",                 100,    cmdFish,            nullptr,      "Go fishing.");
    playerCommands["religion"] = new PlyCommand("religion",         100,    cmdReligion,        nullptr,      "");
    playerCommands["craft"] = new PlyCommand("craft",               100,    cmdCraft,           nullptr,      "Create new items.");
    playerCommands["cook"] = new PlyCommand("cook",                 100,    cmdCraft,           nullptr,      "Create new items.");
    playerCommands["smith"] = new PlyCommand("smith",               100,    cmdCraft,           nullptr,      "Create new items.");
    playerCommands["tailor"] = new PlyCommand("tailor",             100,    cmdCraft,           nullptr,      "Create new items.");
    playerCommands["carpenter"] = new PlyCommand("carpenter",       100,    cmdCraft,           nullptr,      "");
//  playerCommands["alchemy"] = new PlyCommand("alchemy",           100,    cmdCraft,           0,      "");
//  playerCommands["brew"] = new PlyCommand("brew",                 100,    cmdCraft,           0,      "");
    playerCommands["prepare"] = new PlyCommand("prepare",           100,    cmdPrepare,         nullptr,      "Prepare for traps or prepare an item for crafting.");
    playerCommands["unprepare"] = new PlyCommand("unprepare",       100,    cmdUnprepareObject, nullptr,      "Unprepare an item.");
    playerCommands["combine"] = new PlyCommand("combine",           100,    cmdCraft,           nullptr,      "Create new items.");


    // player functions
    playerCommands["forum"] = new PlyCommand("forum",               100,    cmdForum,           nullptr,      "Information on your forum account.");
    playerCommands["brew"] = new PlyCommand("brew",                 100,    cmdBrew,            nullptr,      "");
    playerCommands["house"] = new PlyCommand("house",               100,    cmdHouse,           nullptr,      "");
    playerCommands["property"] = new PlyCommand("property",         100,    cmdProperties,      nullptr,      "Manage your properties.");
    playerCommands["title"] = new PlyCommand("title",               100,    cmdTitle,           nullptr,      "Choose a custom title.");
    playerCommands["defecate"] = new PlyCommand("defecate",          80,    plyAction,          nullptr,      "");
    playerCommands["tnl"] = new PlyCommand("tnl",                   100,    plyAction,          nullptr,      "Show the room how much experience you have to level.");
    playerCommands["mark"] = new PlyCommand("mark",                 100,    plyAction,          nullptr,      "");
    playerCommands["show"] = new PlyCommand("show",                 100,    plyAction,          nullptr,      "");
    playerCommands["dream"] = new PlyCommand("dream",               100,    plyAction,          nullptr,      "");
    playerCommands["rollover"] = new PlyCommand("rollover",         100,    plyAction,          nullptr,      "");
    playerCommands["where"] = new PlyCommand("where",               100,    cmdLook,            nullptr,      "");
    playerCommands["look"] = new PlyCommand("look",                  10,    cmdLook,            nullptr,      "");
    playerCommands["consider"] = new PlyCommand("consider",          50,    cmdLook,            nullptr,      "");
    playerCommands["examine"] = new PlyCommand("examine",           100,    cmdLook,            nullptr,      "");
    playerCommands["reconnect"] = new PlyCommand("reconnect",       100,    cmdReconnect,       nullptr,      "Log in as another character.");
    playerCommands["relog"] = new PlyCommand("relog",               100,    cmdReconnect,       nullptr,      "");
    playerCommands["quit"] = new PlyCommand("quit",                 110,    cmdQuit,            nullptr,      "Log off.");
    playerCommands["goodbye"] = new PlyCommand("goodbye",           100,    cmdQuit,            nullptr,      "");
    playerCommands["logout"] = new PlyCommand("logout",             100,    cmdQuit,            nullptr,      "");
    playerCommands["inventory"] = new PlyCommand("inventory",        10,    cmdInventory,       nullptr,      "View your inventory.");
//  playerCommands["inv"] = new PlyCommand("inv",                   100,    cmdInventory,       0,      "");
//  playerCommands["i"] = new PlyCommand("i",                       100,    cmdInventory,       0,      "");
    playerCommands["who"] = new PlyCommand("who",                    15,    cmdWho,             nullptr,      "See who is online.");
    playerCommands["scan"] = new PlyCommand("scan",                 100,    cmdWho,             nullptr,      "");
    playerCommands["classwho"] = new PlyCommand("classwho",          90,    cmdClasswho,        nullptr,      "");
    playerCommands["whois"] = new PlyCommand("whois",               100,    cmdWhois,           nullptr,      "View information about a specific character.");
    playerCommands["wear"] = new PlyCommand("wear",                 100,    cmdWear,            nullptr,      "");
    playerCommands["remove"] = new PlyCommand("remove",             100,    cmdRemoveObj,       nullptr,      "");
    playerCommands["rm"] = new PlyCommand("rm",                     110,    cmdRemoveObj,       nullptr,      "");
    playerCommands["equipment"] = new PlyCommand("equipment",       100,    cmdEquipment,       nullptr,      "View the equipment you are currently wearing.");
    playerCommands["hold"] = new PlyCommand("hold",                 100,    cmdHold,            nullptr,      "");
    playerCommands["wield"] = new PlyCommand("wield",               100,    cmdReady,           nullptr,      "");
    playerCommands["ready"] = new PlyCommand("ready",               100,    cmdReady,           nullptr,      "");
    playerCommands["second"] = new PlyCommand("second",             100,    cmdSecond,          nullptr,      "");
    playerCommands["help"] = new PlyCommand("help",                 100,    cmdHelp,            nullptr,      "");
    playerCommands["?"] = new PlyCommand("?",                       100,    cmdHelp,            nullptr,      "");
    playerCommands["wiki"] = new PlyCommand("wiki",                 100,    cmdWiki,            nullptr,      "Look up a wiki entry.");
    playerCommands["health"] = new PlyCommand("health",             100,    cmdScore,           nullptr,      "");
    playerCommands["score"] = new PlyCommand("score",                50,    cmdScore,           nullptr,      "Show brief information about your character.");
    playerCommands["status"] = new PlyCommand("status",             100,    cmdInfo,            nullptr,      "");
    playerCommands["levelhistory"] = new PlyCommand("levelhistory", 100,    cmdLevelHistory,    nullptr,      "Show character level history.");
    playerCommands["stats"] = new PlyCommand("stats",               100,    cmdStatistics,      nullptr,      "");
    playerCommands["statistics"] = new PlyCommand("statistics",     100,    cmdStatistics,      nullptr,      "Show character-related statistics.");
    playerCommands["information"] = new PlyCommand("information",    50,    cmdInfo,            nullptr,      "Show extended information about your character.");
    playerCommands["attributes"] = new PlyCommand("attributes",     100,    cmdInfo,            nullptr,      "");
    playerCommands["skills"] = new PlyCommand("skills",             100,    cmdSkills,          nullptr,      "Show what skills your character knows");
    playerCommands["version"] = new PlyCommand("version",           100,    cmdVersion,         nullptr,      "");
    playerCommands["age"] = new PlyCommand("age",                   100,    cmdAge,             nullptr,      "Show your character's age and time played.");
    playerCommands["birthday"] = new PlyCommand("birthday",         100,    checkBirthdays,     nullptr,      "Show which players online have birthdays.");
    playerCommands["colors"] = new PlyCommand("colors",             100,    cmdColors,          nullptr,      "Show colors and set custom colors.");
    playerCommands["colours"] = new PlyCommand("colours",           100,    cmdColors,          nullptr,      "");
    playerCommands["factions"] = new PlyCommand("factions",         100,    cmdFactions,        nullptr,      "Show your standing with various factions.");
    playerCommands["recipes"] = new PlyCommand("recipes",           100,    cmdRecipes,         nullptr,      "Show what recipes you know.");

    // player-only communication functions
    playerCommands["send"] = new PlyCommand("send",                 100,    communicateWith,    nullptr,      "");
    playerCommands["tell"] = new PlyCommand("tell",                 100,    communicateWith,    nullptr,      "Send a message to another player.");
    playerCommands["osend"] = new PlyCommand("osend",               100,    communicateWith,    nullptr,      "");
    playerCommands["otell"] = new PlyCommand("otell",               100,    communicateWith,    nullptr,      "");
    playerCommands["reply"] = new PlyCommand("reply",               100,    communicateWith,    nullptr,      "Reply to a direct message.");
    playerCommands["sign"] = new PlyCommand("sign",                 100,    communicateWith,    nullptr,      "");
    playerCommands["whisper"] = new PlyCommand("whisper",           100,    communicateWith,    nullptr,      "Whisper a message to another player.");
    playerCommands["yell"] = new PlyCommand("yell",                 100,    pCommunicate,       nullptr,      "Yell something that everyone in the room and nearby rooms can hear.");
    playerCommands["recite"] = new PlyCommand("recite",             100,    pCommunicate,       nullptr,      "Recite a phrase to open certain locked doors.");
//  playerCommands["os"] = new PlyCommand("os",                      90,    pCommunicate,       0,      "");
    playerCommands["osay"] = new PlyCommand("osay",                  90,    pCommunicate,       nullptr,      "");
    playerCommands["gtoc"] = new PlyCommand("gtoc",                 100,    pCommunicate,       nullptr,      "");
    playerCommands["gtalk"] = new PlyCommand("gtalk",               100,    pCommunicate,       nullptr,      "Send a message to your group.");
    playerCommands["gt"] = new PlyCommand("gt",                     100,    pCommunicate,       nullptr,      "");
    playerCommands["ignore"] = new PlyCommand("ignore",             100,    cmdIgnore,          nullptr,      "Ignore a player.");
    playerCommands["speak"] = new PlyCommand("speak",               100,    cmdSpeak,           nullptr,      "Speak a different language.");
    playerCommands["languages"] = new PlyCommand("languages",       100,    cmdLanguages,       nullptr,      "Show what languages you know.");
    playerCommands["talk"] = new PlyCommand("talk",                 100,    cmdTalk,            nullptr,      "Talk to a monster.");
    //playerCommands["ask"] = new PlyCommand("ask",                 100,    cmdTalkNew,         0,      "");
    playerCommands["ask"] = new PlyCommand("ask",                   100,    cmdTalk,            nullptr,      "");
    playerCommands["parley"] = new PlyCommand("parley",             100,    cmdTalk,            nullptr,      "");
    playerCommands["sendclan"] = new PlyCommand("sendclan",         100,    channel,            nullptr,      "");
    playerCommands["clansend"] = new PlyCommand("clansend",         100,    channel,            nullptr,      "Communicate via the clan channel.");
    playerCommands["bemote"] = new PlyCommand("bemote",             100,    channel,            nullptr,      "");
    playerCommands["broademote"] = new PlyCommand("broademote",     100,    channel,            nullptr,      "");
    playerCommands["raemote"] = new PlyCommand("raemote",           100,    channel,            nullptr,      "");
    playerCommands["sendrace"] = new PlyCommand("sendrace",         100,    channel,            nullptr,      "");
    playerCommands["racesend"] = new PlyCommand("racesend",         100,    channel,            nullptr,      "Communicate via the race channel.");
    playerCommands["cls"] = new PlyCommand("cls",                   100,    channel,            nullptr,      "");
    playerCommands["sendclass"] = new PlyCommand("sendclass",       100,    channel,            nullptr,      "");
    playerCommands["classsend"] = new PlyCommand("classsend",       100,    channel,            nullptr,      "Communicate via the class channel.");
    playerCommands["clem"] = new PlyCommand("clem",                 100,    channel,            nullptr,      "");
    playerCommands["classemote"] = new PlyCommand("classemote",     100,    channel,            nullptr,      "");
    playerCommands["newbie"] = new PlyCommand("newbie",             100,    channel,            nullptr,      "Communicate via the newbie channel.");
    playerCommands["ptest"] = new PlyCommand("ptest",               100,    channel,            nullptr,      "");
    playerCommands["gossip"] = new PlyCommand("gossip",             100,    channel,            nullptr,      "Communicate via the gossip channel.");
    //playerCommands["gloat"] = new PlyCommand("gloat",             100,    channel,            0,      "");
    playerCommands["adult"] = new PlyCommand("adult",               100,    channel,            nullptr,      "");
    playerCommands["broadcast"] = new PlyCommand("broadcast",        80,    channel,            nullptr,      "");
    playerCommands["gsend"] = new PlyCommand("gsend",               100,    cmdGuildSend,       nullptr,      "Communicate via the guild channel.");

    playerCommands["guild"] = new PlyCommand("guild",               100,    cmdGuild,           nullptr,      "Perform guild-related commands.");
    playerCommands["guildhall"] = new PlyCommand("guildhall",       100,    cmdGuildHall,       nullptr,      "");
    playerCommands["gh"] = new PlyCommand("gh",                     100,    cmdGuildHall,       nullptr,      "");
    playerCommands["follow"] = new PlyCommand("follow",             100,    cmdFollow,          nullptr,      "Follow another player.");
    playerCommands["lose"] = new PlyCommand("lose",                 100,    cmdLose,            nullptr,      "Stop following another player.");
    playerCommands["group"] = new PlyCommand("group",               100,    cmdGroup,           nullptr,      "Show group members.");
    playerCommands["party"] = new PlyCommand("party",               100,    cmdGroup,           nullptr,      "");
    playerCommands["pay"] = new PlyCommand("pay",                   100,    cmdPayToll,         nullptr,      "Pay a monster a toll.");
    playerCommands["track"] = new PlyCommand("track",               100,    cmdTrack,           nullptr,      "");
    playerCommands["peek"] = new PlyCommand("peek",                 100,    cmdPeek,            nullptr,      "");
    playerCommands["search"] = new PlyCommand("search",             100,    cmdSearch,          nullptr,      "Search for anything hidden.");
    playerCommands["hide"] = new PlyCommand("hide",                 100,    cmdHide,            nullptr,      "Hide from view.");
    playerCommands["telnet"] = new PlyCommand("telnet",             100,    cmdTelOpts,         nullptr,      "Shows what telnet options have been negotiated.");
    playerCommands["telopts"] = new PlyCommand("telopts",           100,    cmdTelOpts,         nullptr,      "Shows what telnet options have been negotiated.");
    playerCommands["set"] = new PlyCommand("set",                   100,    cmdPrefs,           nullptr,      "Set a preference.");
    playerCommands["clear"] = new PlyCommand("clear",               100,    cmdPrefs,           nullptr,      "Clear a preference.");
    playerCommands["toggle"] = new PlyCommand("toggle",             100,    cmdPrefs,           nullptr,      "Toggle a preference.");
    playerCommands["preferences"] = new PlyCommand("preferences",   100,    cmdPrefs,           nullptr,      "Show preferences.");
    playerCommands["open"] = new PlyCommand("open",                 100,    cmdOpen,            nullptr,      "Open a door.");
    playerCommands["close"] = new PlyCommand("close",               100,    cmdClose,           nullptr,      "Close a door.");
    playerCommands["shut"] = new PlyCommand("shut",                 100,    cmdClose,           nullptr,      "");
    playerCommands["unlock"] = new PlyCommand("unlock",             100,    cmdUnlock,          nullptr,      "");
    playerCommands["lock"] = new PlyCommand("lock",                 100,    cmdLock,            nullptr,      "");
    playerCommands["pick"] = new PlyCommand("pick",                 100,    cmdPickLock,        nullptr,      "");
    playerCommands["steal"] = new PlyCommand("steal",               100,    cmdSteal,           nullptr,      "");
    playerCommands["flee"] = new PlyCommand("flee",                  40,    cmdFlee,            nullptr,      "");
    playerCommands["run"] = new PlyCommand("run",                    90,    cmdFlee,            nullptr,      "");
    playerCommands["study"] = new PlyCommand("study",               100,    cmdStudy,           nullptr,      "Study a scroll to learn a spell.");
    playerCommands["learn"] = new PlyCommand("learn",               100,    cmdStudy,           nullptr,      "");
    playerCommands["read"] = new PlyCommand("read",                 100,    cmdReadScroll,      nullptr,      "");
    playerCommands["commune"] = new PlyCommand("commune",           100,    cmdCommune,         nullptr,      "");
    playerCommands["bs"] = new PlyCommand("bs",                     100,    cmdBackstab,        nullptr,      "");
    playerCommands["backstab"] = new PlyCommand("backstab",         100,    cmdBackstab,        nullptr,      "");
    playerCommands["ambush"] = new PlyCommand("ambush",             100,    cmdAmbush,          nullptr,      "");
    playerCommands["ab"] = new PlyCommand("ab",                     100,    cmdAmbush,          nullptr,      "");
    playerCommands["train"] = new PlyCommand("train",               100,    cmdTrain,           nullptr,      "Go up a level.");
    playerCommands["save"] = new PlyCommand("save",                 100,    cmdSave,            nullptr,      "");
    playerCommands["time"] = new PlyCommand("time",                 100,    cmdTime,            nullptr,      "Show the current time");
    playerCommands["circle"] = new PlyCommand("circle",              50,    cmdCircle,          nullptr,      "");
    playerCommands["bash"] = new PlyCommand("bash",                  50,    cmdBash,            nullptr,      "");
    playerCommands["barkskin"] = new PlyCommand("barkskin",         100,    cmdBarkskin,        nullptr,      "");
    playerCommands["kick"] = new PlyCommand("kick",                 100,    cmdKick,            nullptr,      "");
    playerCommands["gamestat"] = new PlyCommand("gamestat",         100,    infoGamestat,       nullptr,      "Game time statistics.");

    playerCommands["list"] = new PlyCommand("list",                 100,    cmdList,            nullptr,      "Show items for sale.");
    playerCommands["shop"] = new PlyCommand("shop",                 100,    cmdShop,            nullptr,      "Perform player-shop-related commands.");
    playerCommands["buy"] = new PlyCommand("buy",                   100,    cmdBuy,             nullptr,      "Purchase an item.");
    playerCommands["refund"] = new PlyCommand("refund",             100,    cmdRefund,          nullptr,      "Refund an item you just purchased.");
    playerCommands["reclaim"] = new PlyCommand("reclaim",           100,    cmdReclaim,         nullptr,      "Reclaim an item you just pawned.");
    playerCommands["shoplift"] = new PlyCommand("shoplift",         100,    cmdShoplift,        nullptr,      "");
    playerCommands["sell"] = new PlyCommand("sell",                 100,    cmdSell,            nullptr,      "Sell an item.");
    playerCommands["value"] = new PlyCommand("value",               100,    cmdValue,           nullptr,      "Check the value of an item.");
    playerCommands["cost"] = new PlyCommand("cost",                 100,    cmdCost,            nullptr,      "");
    playerCommands["purchase"] = new PlyCommand("purchase",         90,     cmdPurchase,        nullptr,      "");
    playerCommands["selection"] = new PlyCommand("selection",       100,    cmdSelection,       nullptr,      "");
    playerCommands["trade"] = new PlyCommand("trade",               100,    cmdTrade,           nullptr,      "Trade an item with a monster.");
    playerCommands["auction"] = new PlyCommand("auction",           100,    cmdAuction,         nullptr,      "Auction an item for players to buy.");
    playerCommands["repair"] = new PlyCommand("repair",             100,    cmdRepair,          nullptr,      "Have a smithy repair an item.");
    playerCommands["fix"] = new PlyCommand("fix",                   100,    cmdRepair,          nullptr,      "");

    playerCommands["sendmail"] = new PlyCommand("sendmail",         100,    cmdSendMail,        nullptr,      "Send a mudmail to another player.");
    playerCommands["readmail"] = new PlyCommand("readmail",         100,    cmdReadMail,        nullptr,      "Read your mudmail.");
    playerCommands["deletemail"] = new PlyCommand("deletemail",     100,    cmdDeleteMail,      nullptr,      "");
    playerCommands["edithistory"] = new PlyCommand("edithistory",   100,    cmdEditHistory,     nullptr,      "");
    playerCommands["history"] = new PlyCommand("history",           100,    cmdHistory,         nullptr,      "");
    playerCommands["deletehistory"] = new PlyCommand("deletehistory",100,   cmdDeleteHistory,   nullptr,      "");

    playerCommands["drink"] = new PlyCommand("drink",               100,    cmdConsume,         nullptr,      "Drink a potion.");
    playerCommands["quaff"] = new PlyCommand("quaff",               100,    cmdConsume,         nullptr,      "");
    playerCommands["eat"] = new PlyCommand("eat",                   100,    cmdConsume,         nullptr,      "Eat some food.");
    playerCommands["recall"] = new PlyCommand("recall",             100,    cmdRecall,          nullptr,      "");
    playerCommands["hazy"] = new PlyCommand("hazy",                 100,    cmdRecall,          nullptr,      "");
    playerCommands["teach"] = new PlyCommand("teach",               100,    cmdTeach,           nullptr,      "");
    playerCommands["welcome"] = new PlyCommand("welcome",           100,    cmdWelcome,         nullptr,      "");
    playerCommands["turn"] = new PlyCommand("turn",                 100,    cmdTurn,            nullptr,      "");
    playerCommands["renounce"] = new PlyCommand("renounce",         100,    cmdRenounce,        nullptr,      "");
    playerCommands["holyword"] = new PlyCommand("holyword",         100,    cmdHolyword,        nullptr,      "");
    playerCommands["creep"] = new PlyCommand("creep",               100,    cmdCreepingDoom,    nullptr,      "");
    playerCommands["poison"] = new PlyCommand("poison",             100,    cmdPoison,          nullptr,      "");
    playerCommands["smother"] = new PlyCommand("smother",           100,    cmdEarthSmother,    nullptr,      "");
    playerCommands["bribe"] = new PlyCommand("bribe",               100,    cmdBribe,           nullptr,      "Bribe a monster to leave the room.");
    playerCommands["frenzy"] = new PlyCommand("frenzy",             100,    cmdFrenzy,          nullptr,      "");
    playerCommands["pray"] = new PlyCommand("pray",                 100,    cmdPray,            nullptr,      "");
    playerCommands["use"] = new PlyCommand("use",                   100,    cmdUse,             nullptr,      "");
    playerCommands["us"] = new PlyCommand("us",                     100,    cmdUse,             nullptr,      "");
    playerCommands["break"] = new PlyCommand("break",               100,    cmdBreak,           nullptr,      "Break an object.");
    playerCommands["pledge"] = new PlyCommand("pledge",             100,    cmdPledge,          nullptr,      "Pledge your allegience to a clan.");
    playerCommands["rescind"] = new PlyCommand("rescind",           100,    cmdRescind,         nullptr,      "Rescind your allegience from a clan.");
    playerCommands["suicide"] = new PlyCommand("suicide",           100,    cmdSuicide,         nullptr,      "Delete your character.");
    playerCommands["convert"] = new PlyCommand("convert",           100,    cmdConvert,         nullptr,      "Convert from chaotic alignment to lawful alignment.");
    playerCommands["password"] = new PlyCommand("password",         100,    cmdPassword,        nullptr,      "Change your password.");
    playerCommands["proxy"] = new PlyCommand("proxy",               100,    cmdProxy,           nullptr,      "Allow proxy access to this character.");
    playerCommands["finger"] = new PlyCommand("finger",             100,    cmdFinger,          nullptr,      "Look up an offline character.");
    playerCommands["hypnotize"] = new PlyCommand("hypnotize",       100,    cmdHypnotize,       nullptr,      "");
    playerCommands["bite"] = new PlyCommand("bite",                  60,    cmdBite,            nullptr,      "");
    playerCommands["mist"] = new PlyCommand("mist",                 100,    cmdMist,            nullptr,      "");
    playerCommands["unmist"] = new PlyCommand("unmist",             100,    cmdUnmist,          nullptr,      "");
    playerCommands["regenerate"] = new PlyCommand("regenerate",     100,    cmdRegenerate,      nullptr,      "");
    playerCommands["drain"] = new PlyCommand("drain",               100,    cmdDrainLife,       nullptr,      "");

    playerCommands["charm"] = new PlyCommand("charm",               100,    cmdCharm,           nullptr,      "");
    playerCommands["identify"] = new PlyCommand("identify",         100,    cmdIdentify,        nullptr,      "");
    playerCommands["songs"] = new PlyCommand("songs",               100,    cmdSongs,           nullptr,      "");

    playerCommands["enthrall"] = new PlyCommand("enthrall",         100,    cmdEnthrall,        nullptr,      "");
    playerCommands["meditate"] = new PlyCommand("meditate",         100,    cmdMeditate,        nullptr,      "");
    playerCommands["focus"] = new PlyCommand("focus",               100,    cmdFocus,           nullptr,      "");
    playerCommands["mistbane"] = new PlyCommand("mistbane",          90,    cmdMistbane,        nullptr,      "");
    playerCommands["mb"] = new PlyCommand("mb",                     100,    cmdMistbane,        nullptr,      "");
    playerCommands["touch"] = new PlyCommand("touch",               100,    cmdTouchOfDeath,    nullptr,      "");
    playerCommands["quests"] = new PlyCommand("quests",             100,    cmdQuests,          nullptr,      "View your quests and perform quest-related commands.");
    playerCommands["hands"] = new PlyCommand("hands",               100,    cmdLayHands,        nullptr,      "");
    playerCommands["harm"] = new PlyCommand("harm",                 100,    cmdHarmTouch,       nullptr,      "");
    playerCommands["scout"] = new PlyCommand("scout",               100,    cmdScout,           nullptr,      "");
    playerCommands["berserk"] = new PlyCommand("berserk",           100,    cmdBerserk,         nullptr,      "");
    playerCommands["transmute"] = new PlyCommand("transmute",       100,    cmdTransmute,       nullptr,      "");
    playerCommands["daily"] = new PlyCommand("daily",               100,    cmdDaily,           nullptr,      "See our usage of daily-limited abilities.");
    playerCommands["description"] = new PlyCommand("description",   100,    cmdDescription,     nullptr,      "Change your character's description.");
    playerCommands["bsacrifice"] = new PlyCommand("bsacrifice",     100,    cmdBloodsacrifice,  nullptr,      "");
    playerCommands["bloodsacrifice"] = new PlyCommand("bloodsacrifice",100, cmdBloodsacrifice,  nullptr,      "");
    playerCommands["disarm"] = new PlyCommand("disarm",             100,    cmdDisarm,          nullptr,      "Disarm an opponent or remove your wielded weapons.");
    playerCommands["visible"] = new PlyCommand("visible",           100,    cmdVisible,         nullptr,      "Cancel invisibility.");
    playerCommands["watch"] = new PlyCommand("watch",               100,    cmdWatch,           nullptr,      "");
    playerCommands["conjure"] = new PlyCommand("conjure",           100,    conjureCmd,         nullptr,      "");
    playerCommands["maul"] = new PlyCommand("maul",                 100,    cmdMaul,            nullptr,      "");
    playerCommands["enchant"] = new PlyCommand("enchant",           100,    cmdEnchant,         nullptr,      "");
    playerCommands["envenom"] = new PlyCommand("envenom",           100,    cmdEnvenom,         nullptr,      "");

    playerCommands["balance"] = new PlyCommand("balance",            90,    cmdBalance,         nullptr,      "See your bank balance.");
    playerCommands["deposit"] = new PlyCommand("deposit",           100,    cmdDeposit,         nullptr,      "Deposit gold in the bank.");
    playerCommands["withdraw"] = new PlyCommand("withdraw",         100,    cmdWithdraw,        nullptr,      "Withdraw gold from the bank.");
    playerCommands["deletestatement"] = new PlyCommand("deletestatement",100,   cmdDeleteStatement, nullptr,      "");
    playerCommands["transfer"] = new PlyCommand("transfer",         100,    cmdTransfer,        nullptr,      "Transfer gold to another character.");
//  playerCommands["stat"] = new PlyCommand("stat",                 100,    cmdStatement,       0,      "");
    playerCommands["statement"] = new PlyCommand("statement",        90,    cmdStatement,       nullptr,      "View your bank statement.");

    playerCommands["refuse"] = new PlyCommand("refuse",             100,    cmdRefuse,          nullptr,      "");
    playerCommands["duel"] = new PlyCommand("duel",                 100,    cmdDuel,            nullptr,      "Challenge another player to a duel.");
    playerCommands["animate"] = new PlyCommand("animate",           100,    animateDeadCmd,     nullptr,      "");
    playerCommands["lottery"] = new PlyCommand("lottery",           100,    cmdLottery,         nullptr,      "View lottery info.");
    playerCommands["claim"] = new PlyCommand("claim",               100,    cmdClaim,           nullptr,      "Claim a lottery ticket.");
    playerCommands["mccp"] = new PlyCommand("mccp",                 100,    mccp,               nullptr,      "Toggle mccp (compression).");
    playerCommands["surname"] = new PlyCommand("surname",           100,    cmdSurname,         nullptr,      "Choose a surname.");
    playerCommands["changestats"] = new PlyCommand("changestats",   100,    cmdChangeStats,     nullptr,      "");
    playerCommands["keep"] = new PlyCommand("keep",                 100,    cmdKeep,            nullptr,      "Prevent accidentially throwing away an item.");
    playerCommands["unkeep"] = new PlyCommand("unkeep",             100,    cmdUnkeep,          nullptr,      "Unkeep an item.");
    playerCommands["alignment"] = new PlyCommand("alignment",       100,    cmdChooseAlignment, nullptr,      "Choose your alignment.");
    playerCommands["push"] = new PlyCommand("push",                 100,    cmdPush,            nullptr,      "");
    playerCommands["pull"] = new PlyCommand("pull",                 100,    cmdPull,            nullptr,      "");
    playerCommands["press"] = new PlyCommand("press",               100,    cmdPress,           nullptr,      "");
    playerCommands["shipquery"] = new PlyCommand("shipquery",       90,     cmdQueryShips,      nullptr,      "See where chartered ships are.");
    playerCommands["ships"] = new PlyCommand("ships",               100,    cmdQueryShips,      nullptr,      "");

    playerCommands["bandage"] = new PlyCommand("bandage",           100,    cmdBandage,         nullptr,      "Use bandages to heal yourself.");

    // movement
//  playerCommands["n"] = new PlyCommand("n",                       100,    cmdMove,            0,      "");
    playerCommands["north"] = new PlyCommand("north",                10,    cmdMove,            nullptr,      "");
//  playerCommands["s"] = new PlyCommand("s",                       100,    cmdMove,            0,      "");
    playerCommands["south"] = new PlyCommand("south",                10,    cmdMove,            nullptr,      "");
//  playerCommands["e"] = new PlyCommand("e",                       100,    cmdMove,            0,      "");
    playerCommands["east"] = new PlyCommand("east",                  10,    cmdMove,            nullptr,      "");
//  playerCommands["w"] = new PlyCommand("w",                       100,    cmdMove,            0,      "");
    playerCommands["west"] = new PlyCommand("west",                  10,    cmdMove,            nullptr,      "");
    playerCommands["northeast"] = new PlyCommand("northeast",        15,    cmdMove,            nullptr,      "");
    playerCommands["ne"] = new PlyCommand("ne",                      15,    cmdMove,            nullptr,      "");
    playerCommands["northwest"] = new PlyCommand("northwest",        15,    cmdMove,            nullptr,      "");
    playerCommands["nw"] = new PlyCommand("nw",                      15,    cmdMove,            nullptr,      "");
    playerCommands["southeast"] = new PlyCommand("southeast",        15,    cmdMove,            nullptr,      "");
    playerCommands["se"] = new PlyCommand("se",                      15,    cmdMove,            nullptr,      "");
    playerCommands["southwest"] = new PlyCommand("southwest",        15,    cmdMove,            nullptr,      "");
    playerCommands["sw"] = new PlyCommand("sw",                      15,    cmdMove,            nullptr,      "");
//  playerCommands["u"] = new PlyCommand("u",                        10,    cmdMove,            0,      "");
    playerCommands["up"] = new PlyCommand("up",                      10,    cmdMove,            nullptr,      "");
//  playerCommands["d"] = new PlyCommand("d",                       100,    cmdMove,            0,      "");
    playerCommands["down"] = new PlyCommand("down",                  10,    cmdMove,            nullptr,      "");
    playerCommands["out"] = new PlyCommand("out",                   100,    cmdMove,            nullptr,      "");
    playerCommands["leave"] = new PlyCommand("leave",               100,    cmdMove,            nullptr,      "");
    //      playerCommands["aft"] = new PlyCommand("aft",           100,        move,               0,      "");
    //      playerCommands["fore"] = new PlyCommand("fore",         100,        move,               0,      "");
    //      playerCommands["starb"] = new PlyCommand("starb",       100,        move,               0,      "");
    //      playerCommands["port"] = new PlyCommand("port",         100,        move,               0,      "");
    //playerCommands["sn"] = new PlyCommand("sn",                       100,    cmdGo,              0,      "");
    playerCommands["sneak"] = new PlyCommand("sneak",                50,    cmdGo,              nullptr,      "Sneak to an exit so you are not seen.");
    playerCommands["go"] = new PlyCommand("go",                     100,    cmdGo,              nullptr,      "");
    playerCommands["exit"] = new PlyCommand("exit",                 100,    cmdGo,              nullptr,      "");
    playerCommands["enter"] = new PlyCommand("enter",               100,    cmdGo,              nullptr,      "");

// *************************************************************************************
// General player/pet Commands

    // general player/pet commands
    generalCommands["say"] = new CrtCommand("say",                  100,    communicate,        nullptr,      "");
    generalCommands["\\"] = new CrtCommand("\"",                    100,    communicate,        nullptr,      "");
    generalCommands["'"] = new CrtCommand("'",                      100,    communicate,        nullptr,      "");
    generalCommands["emote"] = new CrtCommand("emote",              100,    communicate,        nullptr,      "");
    generalCommands[":"] = new CrtCommand(":",                      100,    communicate,        nullptr,      "");

    generalCommands["spells"] = new CrtCommand("spells",            100,    cmdSpells,          nullptr,      "Show what spells you know.");
//  generalCommands["c"] = new CrtCommand("c",                      100,    cmdCast,            0,      "");
    generalCommands["cast"] = new CrtCommand("cast",                 10,    cmdCast,            nullptr,      "");

    generalCommands["attack"] = new CrtCommand("attack",             50,    cmdAttack,          nullptr,      "");
    generalCommands["kill"] = new CrtCommand("kill",                 10,    cmdAttack,          nullptr,      "");
//  generalCommands["k"] = new CrtCommand("k",                      100,    cmdAttack,          0,      "");

    generalCommands["get"] = new CrtCommand("get",                  100,    cmdGet,             nullptr,      "Pick up an item and put it in your inventory.");
    generalCommands["take"] = new CrtCommand("take",                100,    cmdGet,             nullptr,      "");
    generalCommands["drop"] = new CrtCommand("drop",                100,    cmdDrop,            nullptr,      "Remove an item from your inventory and drop it on the ground.");
    generalCommands["put"] = new CrtCommand("put",                  100,    cmdDrop,            nullptr,      "");
    generalCommands["give"] = new CrtCommand("give",                100,    cmdGive,            nullptr,      "");

    generalCommands["throw"] = new CrtCommand("throw",              100,    cmdThrow,           nullptr,      "Throw an object.");
    generalCommands["knock"] = new CrtCommand("knock",              100,    cmdKnock,           nullptr,      "Knock on a door to make a noise on the other side.");
    generalCommands["effects"] = new CrtCommand("effects",          100,    cmdEffects,         nullptr,      "Show what effects you are currently under.");
    generalCommands["howl"] = new CrtCommand("howl",                100,    cmdHowl,            nullptr,      "");
    generalCommands["sing"] = new CrtCommand("sing",                100,    cmdSing,            nullptr,      "");

    // socials
    generalCommands["chest"] = new CrtCommand("chest",              100,    cmdAction,              nullptr,      "");
    generalCommands["stab"] = new CrtCommand("stab",                100,    cmdAction,              nullptr,      "");
    generalCommands["vangogh"] = new CrtCommand("vangogh",          100,    cmdAction,              nullptr,      "");
    generalCommands["smooch"] = new CrtCommand("smooch",            100,    cmdAction,              nullptr,      "");
    generalCommands["lobotomy"] = new CrtCommand("lobotomy",        100,    cmdAction,              nullptr,      "");
    generalCommands["sleep"] = new CrtCommand("sleep",              100,    cmdAction,              nullptr,      "Go to sleep to heal faster.");
    generalCommands["stand"] = new CrtCommand("stand",              100,    cmdAction,              nullptr,      "Stand up from a sitting position.");
    generalCommands["sit"] = new CrtCommand("sit",                  100,    cmdAction,              nullptr,      "Sit down to heal faster.");
    generalCommands["dismiss"] = new CrtCommand("dismiss",          100,    cmdAction,              nullptr,      "");
    generalCommands["burp"] = new CrtCommand("burp",                100,    cmdAction,              nullptr,      "");
    generalCommands["rps"] = new CrtCommand("rps",                  100,    cmdAction,              nullptr,      "");
    generalCommands["masturbate"] = new CrtCommand("masturbate",    90,     cmdAction,              nullptr,      "");
    generalCommands["hum"] = new CrtCommand("hum",                  100,    cmdAction,              nullptr,      "");
    // "pet" also links to orderPet within action
    generalCommands["pet"] = new CrtCommand("pet",                  100,    cmdAction,              nullptr,      "");
    generalCommands["pat"] = new CrtCommand("pat",                  100,    cmdAction,              nullptr,      "");
    generalCommands["glare"] = new CrtCommand("glare",              100,    cmdAction,              nullptr,      "");
    generalCommands["bless"] = new CrtCommand("bless",              100,    cmdAction,              nullptr,      "");
    generalCommands["flip"] = new CrtCommand("flip",                100,    cmdAction,              nullptr,      "");
    generalCommands["point"] = new CrtCommand("point",              100,    cmdAction,              nullptr,      "");
    generalCommands["afk"] = new CrtCommand("afk",                  100,    cmdAction,              nullptr,      "");
    generalCommands["fangs"] = new CrtCommand("fangs",              100,    cmdAction,              nullptr,      "");
    generalCommands["hiss"] = new CrtCommand("hiss",                100,    cmdAction,              nullptr,      "");
    generalCommands["hunger"] = new CrtCommand("hunger",            100,    cmdAction,              nullptr,      "");
    generalCommands["freeze"] = new CrtCommand("freeze",            100,    cmdAction,              nullptr,      "");
    generalCommands["ballz"] = new CrtCommand("ballz",              100,    cmdAction,              nullptr,      "");
    generalCommands["wag"] = new CrtCommand("wag",                  100,    cmdAction,              nullptr,      "");
    generalCommands["murmur"] = new CrtCommand("murmur",            100,    cmdAction,              nullptr,      "");
    generalCommands["snore"] = new CrtCommand("snore",              100,    cmdAction,              nullptr,      "");
    generalCommands["tag"] = new CrtCommand("tag",                  100,    cmdAction,              nullptr,      "");
    generalCommands["narrow"] = new CrtCommand("narrow",            100,    cmdAction,              nullptr,      "");
    generalCommands["gnaw"] = new CrtCommand("gnaw",                100,    cmdAction,              nullptr,      "");
    generalCommands["sneeze"] = new CrtCommand("sneeze",            100,    cmdAction,              nullptr,      "");
    // wake is a special command that you might be able to
    // use while unconscious. rules are in Creature::ableToDoCommand
    generalCommands["wake"] = new CrtCommand("wake",                100,    cmdAction,              nullptr,      "Wake up from being asleep.");

    generalCommands["dice"] = new CrtCommand("dice",                100,    cmdDice,            nullptr,      "Roll dice.");

    
    // Once we've built the command tables, write their help files to the help directory
    writeCommandFile(CreatureClass::NONE, Path::Help, "commands");
    writeCommandFile(CreatureClass::DUNGEONMASTER, Path::DMHelp, "dmcommands");
    writeCommandFile(CreatureClass::BUILDER, Path::BuilderHelp, "bhcommands");

    return(true);
}

void Config::clearCommands() {

    for(const auto& pc : staffCommands)
        delete pc.second;

    for(const auto& pc : playerCommands)
        delete pc.second;

    for(const auto& cc : generalCommands)
        delete cc.second;

    // Note: Skill commands are cleared as a part of clearSkills!

    staffCommands.clear();
    playerCommands.clear();
    generalCommands.clear();
}

bool MudMethod::exactMatch(const bstring& toMatch) {
    return(name.equals(toMatch.c_str(), false));
}

bool MudMethod::partialMatch(const bstring& toMatch) {
    return(!strncasecmp(name.c_str(), toMatch.c_str(), toMatch.length()));
}

bool MysticMethod::exactMatch(const bstring& toMatch) {
    return(name.equals(toMatch.c_str(), false));
}

bool MysticMethod::partialMatch(const bstring& toMatch) {
    return(!strncasecmp(name.c_str(), toMatch.c_str(), toMatch.length()));
}

void MysticMethod::parseName() {
    boost::char_separator<char> sep(" ");
    boost::tokenizer< boost::char_separator<char> > tokens(name, sep);

    for(const bstring& t : tokens) {
        nameParts.push_back(t);
    }
}

//*********************************************************************
//                      getCommand
//*********************************************************************

// bestMethod is a reference to a pointer
template<class Type, class Type2>
void examineList(std::map<bstring, Type, comp>& myMap, bstring& str, int& match, bool& found, Type2*& bestMethod) {

    Type2* curMethod = nullptr;

    // Narrow down the range of the map we'll be looking at
    auto it = myMap.lower_bound(str);
    auto endIt = myMap.upper_bound(bstring(1,char(str.at(0)+1)));

    while(it != endIt) {
        std::pair<bstring, Type> cp = (*it++);

        if(!cp.second) continue;
        curMethod = cp.second; 

        if(curMethod->exactMatch(str)) {
            // Full match, stop here
            bestMethod = curMethod;
            found = true;
            match = 1;
            //std::clog << "Found an exact match!\n";
            break;
        } else if(curMethod->partialMatch(str)) {
            // Partial Match, see how good of a match
            if(bestMethod == nullptr) {
                // No best match yet, this is our new best match
                bestMethod = curMethod;
                match = 1;
                //std::clog << "First Match: " << bestMethod->getStr() << "'\n";
            } else {
                // Already have a best match, compare it to this to see which is better

                // Lower priority always wins
                if(curMethod->priority < bestMethod->priority) {
                    // New winner
                    bestMethod = curMethod;
                    match = 1;
                    //std::clog << "New Best Match: " << bestMethod->getStr() << "'\n";
                } else if(curMethod->priority == bestMethod->priority) {
                    // Same priority, check % of match
                    int curLen = curMethod->name.length();
                    int bestLen = bestMethod->name.length();

                    if(curLen < bestLen) {
                        // Current command's length is lower, so we match a higher percentage of it.
                        // It is the new winner
                        //std::clog << "New Best Match: " << bestMethod->getStr() << "'\n";
                        bestMethod = curMethod;
                        match = 1;
                    } else if(curLen == bestLen) {
                        // Drats we have a tie
                        match++;
                        //std::clog << "Tied!\n";
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
    // Match - Partial matches
    int     match=0;
    // Found - Exact match
    bool    found=false;
//  int     i=0;

    Player* pUser = user->getAsPlayer();
    str = cmnd->str[0];

    //std::clog << "Looking for a match for '" << str << "'\n";

    examineList<CrtCommand*, Command>(gConfig->generalCommands, str, match, found, cmnd->myCommand);
    if(!found)
        examineList<SkillCommand*, Command>(gConfig->skillCommands, str, match, found, cmnd->myCommand);
    // If we're a player, examine a few extra lists
    if(!found && pUser) {
        if(!found) // No exact match so far, check player list
            examineList<PlyCommand*, Command>(gConfig->playerCommands, str, match, found, cmnd->myCommand);
        if(!found && pUser->isStaff()) // Still no exact match, check staff commands
            examineList<PlyCommand*, Command>(gConfig->staffCommands, str, match, found, cmnd->myCommand);
    }
    if(!found)
        examineList<SocialCommand*, Command>(gConfig->socials, str, match, found, cmnd->myCommand);

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
//                      getSpell
//*********************************************************************

Spell *Config::getSpell(bstring id, int& ret) {
    Spell *toReturn = nullptr;
    int match = 0;
    bool found = false;

    examineList<Spell*, Spell>(spells, id, match, found, toReturn);

    if(!match)
        ret = CMD_NOT_FOUND;
    else if(match > 1)
        ret = CMD_NOT_UNIQUE;
    else
        ret = 0;


    return(toReturn);
}

//*********************************************************************
//                      getSong
//*********************************************************************

Song *Config::getSong(bstring pName) {
    Song *toReturn = nullptr;
    int match = 0;
    bool found = false;

    examineList<Song*, Song>(songs, pName, match, found, toReturn);

    return(toReturn);
}

Song *Config::getSong(bstring name, int& ret){
    Song *toReturn = nullptr;
    int match = 0;
    bool found = false;

    examineList<Song*, Song>(songs, name, match, found, toReturn);

    if(!match)
        ret = CMD_NOT_FOUND;
    else if(match > 1)
        ret = CMD_NOT_UNIQUE;
    else
        ret = 0;

    return(toReturn);
}

//*********************************************************************
//                      allowedWhilePetrified
//*********************************************************************

static std::set<bstring> commandsAllowedWhilePetrified = {
    {"clear"},
    {"eff"},
    {"effects"},
    {"finger"},
    {"health"},
    {"help"},
    {"info"},
    {"pass"},
    {"passwd"},
    {"password"},
    {"pref"},
    {"quit"},
    {"sc"},
    {"score"},
    {"set"},
    {"time"},
    {"toggle"},
    {"who"},
    {"whois"},
};
int allowedWhilePetrified(const bstring& str) {
    return commandsAllowedWhilePetrified.find(str) != commandsAllowedWhilePetrified.end();
}


//*********************************************************************
//                      cmdProcess
//*********************************************************************
// This function takes the command structure of the person at the socket
// in the first parameter and interprets the person's command.

int cmdProcess(Creature *user, cmd* cmnd, Creature* pet) {
    Player  *player=nullptr;
    int     fd = user->fd;

    if(fd > 0) {
        if(!pet) {
            player = user->getAsPlayer();
        } else
            user = pet;
    }

    if(player && player->afterProf) {
        broadcast(isDm, "^RafterProf field set on %s. Last command: %s", player->getCName(), player->getLastCommand().c_str());
        broadcast(isDm, "^RResetting afterProf to 0.");
        player->afterProf = 0;
    }
    if(!user) {
        std::clog << "invalid creature trying to use cmdProcess.\n";
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

bstring Nameable::getName() const {
    return(name);
}

bstring Nameable::getDescription() const {
    return(description);
}

int PlyCommand::execute(Creature* player, cmd* cmnd) {
    Player *ply = player->getAsPlayer();
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
