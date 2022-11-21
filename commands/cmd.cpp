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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <fmt/format.h>                          // for format
#include <strings.h>                             // for strncasecmp
#include <unistd.h>                              // for link
#include <boost/algorithm/string/predicate.hpp>  // for iequals
#include <boost/iterator/iterator_facade.hpp>    // for operator!=, iterator...
#include <boost/token_functions.hpp>             // for char_separator
#include <boost/token_iterator.hpp>              // for token_iterator
#include <boost/tokenizer.hpp>                   // for tokenizer
#include <cstdio>                                // for snprintf, sprintf
#include <fstream>                               // for ofstream, operator<<
#include <iomanip>                               // for operator<<, setw
#include <list>                                  // for list
#include <locale>                                // for locale
#include <map>                                   // for operator==, map, map...
#include <set>                                   // for set, set<>::iterator
#include <string>                                // for string, allocator
#include <string_view>                           // for string_view
#include <utility>                               // for pair

#include "cmd.hpp"                               // for cmd, CMD_NOT_UNIQUE
#include "commands.hpp"                          // for cmdAction, channel
#include "config.hpp"                            // for Config, PlyCommandSet
#include "creatureStreams.hpp"                   // for Streamable
#include "dm.hpp"                                // for dmKillSwitch, builde...
#include "global.hpp"                            // for CreatureClass, Creat...
#include "help.hpp"                              // for loadHelpTemplate
#include "mudObjects/creatures.hpp"              // for Creature
#include "mudObjects/players.hpp"                // for Player
#include "namable.hpp"                           // for Nameable
#include "paths.hpp"                             // for Help, BuilderHelp
#include "server.hpp"                            // for Server, gServer
#include "ships.hpp"                             // for cmdQueryShips
#include "skillCommand.hpp"                      // for SkillCommand
#include "socials.hpp"                           // for SocialCommand
#include "songs.hpp"                             // for Song
#include "structs.hpp"                           // for Command, Spell, PlyC...

class MudObject;



int dmTest(const std::shared_ptr<Player>& player, cmd* cmnd) {
    *player << gConfig->getProxyList();

    return(0);
}


int pcast(const std::shared_ptr<Player>& player, cmd* cmnd) {
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

    std::string args = getFullstrText(cmnd->fullstr, 2);
    std::shared_ptr<MudObject>target = nullptr;

    //target = player->getParent()->findTarget(cmnd->fullstr);

    gServer->runPython(spell->script, args, player, target);
    return(0);
}




//**********************************************************************
//                      compileSocialList
//**********************************************************************

template<class Type>
void compileSocialList(std::map<std::string, std::string>& list, const std::set<Type, namableCmp>& cList) {
    std::string name;

    for(const auto& cmnd : cList) {
        auto* cc = dynamic_cast<const CrtCommand*>(&cmnd);
        auto* pc = dynamic_cast<const PlyCommand*>(&cmnd);
        auto* sc = dynamic_cast<const SocialCommand*>(&cmnd);

        if((!cc || cc->fn != cmdAction) &&
           (!pc || pc->fn != plyAction) &&
           !sc )
            continue;

        name = cmnd.getName();

        if(name == "vomjom" || name == "usagi" || name == "defenestrate" || name == "mithas")
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
    std::map<std::string,std::string> list;
    std::map<std::string,std::string>::iterator it;

    sprintf(file, "%s/socials.txt", Path::Help.c_str());
    sprintf(fileLink, "%s/social.txt", Path::Help.c_str());

    // prepare to write the help file
    std::ofstream out(file);
    out.setf(std::ios::left, std::ios::adjustfield);
    out.imbue(std::locale(""));

    out << Help::loadHelpTemplate("socials");

    compileSocialList<PlyCommand>(list, playerCommands);
    compileSocialList<CrtCommand>(list, generalCommands);
    compileSocialList<SocialCommand>(list, socials);


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
std::map<std::string,std::string> compileCommandList(std::map<std::string, std::string>& list, CreatureClass cls, std::set<Type, namableCmp>& cList) {
    for(const auto& cmd : cList) {
        if(cmd.getDescription().empty()) continue;
        if( cls == CreatureClass::BUILDER && cmd.auth && cmd.auth != builderMob && cmd.auth != builderObj) continue;

        list[cmd.getName()] = fmt::format(" ^W{}^x {}\n", cmd.getName(), cmd.getDescription());
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

    std::map<std::string,std::string> list;
    std::map<std::string,std::string> list2;
    std::map<std::string,std::string>::iterator it;

    snprintf(file, 99, "%s/commands.txt", path);
    snprintf(fileLink, 99, "%s/command.txt", path);

    // prepare to write the help file
    std::ofstream out(file);
    out.setf(std::ios::left, std::ios::adjustfield);
    out.imbue(std::locale(""));

    out << Help::loadHelpTemplate(tpl);

    if(cls == CreatureClass::DUNGEONMASTER || cls == CreatureClass::BUILDER) {
        compileCommandList<PlyCommand>(list, cls, gConfig->staffCommands);
    } else {
        compileCommandList<PlyCommand>(list, CreatureClass::NONE, gConfig->playerCommands);
        compileCommandList<CrtCommand>(list, CreatureClass::NONE, gConfig->generalCommands);
        compileCommandList<SkillCommand>(list, CreatureClass::NONE, gConfig->skillCommands);
    }

    for(it = list.begin(); it != list.end(); it++) {
        out << (*it).second;
    }

    out << "\n";
    out.close();
    link(file, fileLink);
}

bool isCt(const std::shared_ptr<Creature> & player);
bool isDm(const std::shared_ptr<Creature> & player);

//**********************************************************************
//                      initCommands
//**********************************************************************
// everything on the staff array already checks isStaff - the listed authorization is an additional requirement to use the command.

bool Config::initCommands() {
// *************************************************************************************
// Staff Commands

    staffCommands.emplace("play", 100, cmdPlay, isCt, "Play a song.");
    staffCommands.emplace("*cache", 100, dmCache, isDm, "Show DNS cache.");
    staffCommands.emplace("*test", 100, dmTest, isDm, "");
    staffCommands.emplace("pcast", 100, pcast, isDm, "");

    // channels
    staffCommands.emplace("*send", 10, channel, isCt, "");
    staffCommands.emplace("dm", 100, channel, isDm, "");
    staffCommands.emplace("admin", 100, channel, isDm, "");
    staffCommands.emplace("dmrace", 100, channel, isCt, "Use specified race channel.");
    staffCommands.emplace("dmclass", 100, channel, isCt, "Use specified class channel.");
    staffCommands.emplace("dmcls", 100, channel, isCt, "");
    staffCommands.emplace("dmclan", 100, channel, isCt, "Use specified clan channel.");
    staffCommands.emplace("dmguild", 100, channel, isCt, "Use specified guild channel.");
    staffCommands.emplace("*msg", 100, channel, nullptr, "");

    // dm.c
    staffCommands.emplace("*reboot", 100, dmReboot, isCt, "Reboot the mud.");
    staffCommands.emplace("*inv", 100, dmMobInventory, nullptr, "");
    staffCommands.emplace("*sockets", 100, dmSockets, isDm, "Show all connected sockets.");
    staffCommands.emplace("*dmload", 100, dmLoad, isDm, "Reload configuration files.");
    staffCommands.emplace("*txtoncrash", 100, dmTxtOnCrash, isDm, "Toggle whether the mud sends a text message when it crashes.");
    staffCommands.emplace("*dmsave", 100, dmSave, isDm, "Resave configuration files.");
    staffCommands.emplace("*teleport", 10, dmTeleport, nullptr, "Teleport to rooms or players, or teleport players to each other.");
    staffCommands.emplace("*users", 10, dmUsers, isCt, "See a list of connected users.");
    staffCommands.emplace("*shutdown", 100, dmShutdown, isDm, "Shut down the mud (may auto-restart).");
    staffCommands.emplace("*flushrooms", 100, dmFlushSave, isDm, "Flush rooms from memory.");
    staffCommands.emplace("*flushcrtobj", 100, dmFlushCrtObj, nullptr, "Flush creatures and objects from memory.");
    staffCommands.emplace("*save", 100, dmResave, nullptr, "Save the room, creature, or object.");
    staffCommands.emplace("*perm", 100, dmPerm, nullptr, "");
    staffCommands.emplace("*invis", 10, dmInvis, nullptr, "Toggle DM invis (only staff can see you).");
    staffCommands.emplace("*incog", 20, dmIncog, nullptr, "");
    staffCommands.emplace("incog", 100, dmIncog, nullptr, "Toggle incognito status (only people in the room can see you).");
    staffCommands.emplace("*ac", 100, dmAc, nullptr, "Show your Weapon/Defense/Armor and completely heal yourself.");
    staffCommands.emplace("*wipe", 100, dmWipe, nullptr, "");
    staffCommands.emplace("*clear", 100, dmDeleteDb, isDm, "");
    staffCommands.emplace("*gamestat", 100, dmGameStatus, isDm, "Show configuration options.");
    staffCommands.emplace("*weather", 100, dmWeather, nullptr, "Show weather strings.");
    staffCommands.emplace("*questlist", 100, dmQuestList, isCt, "Show quests in memory.");
    staffCommands.emplace("*alchemylist", 100, dmAlchemyList, isCt, "Show the Alchemy List.");
    staffCommands.emplace("*clanlist", 100, dmClanList, isCt, "Show clans in memory.");
    staffCommands.emplace("*bane", 500, dmBane, isDm, "Crash the game");
    staffCommands.emplace("*dmhelp", 50, dmHelp, isCt, "Access help files.");
    staffCommands.emplace("*bhelp", 50, bhHelp, nullptr, "Access help files.");
    staffCommands.emplace("*fishing", 100, dmFishing, nullptr, "View fishing lists.");
    staffCommands.emplace("*parameter", 100, dmParam, isDm, "");
    staffCommands.emplace("*outlaw", 100, dmOutlaw, isCt, "Make a player an outlaw.");
    staffCommands.emplace("*broadcast", 20, dmBroadecho, isCt, "Broadcast a message to all players.");
    staffCommands.emplace("*gcast", 100, dmCast, isCt, "Cast a spell on all players.");
    staffCommands.emplace("*set", 100, dmSet, nullptr, "Modify a room/object/exit/player/object/monster.");
    staffCommands.emplace("*log", 100, dmLog, isCt, "");
    staffCommands.emplace("*list", 100, dmList, isCt, "");
    staffCommands.emplace("*info", 100, dmInfo, isCt, "Show game info (includes some memory).");
    staffCommands.emplace("*md5", 100, dmMd5, isCt, "Show md5 of input string.");
    staffCommands.emplace("*ids", 100, dmIds, isDm, "Shows registered ids.");
    staffCommands.emplace("*status", 80, dmStat, nullptr, "Show info about a room/player/object/monster.");
    staffCommands.emplace("*sd", 100, dmStatDetail, isCt, "Show detailed information about a creature's stats.");


    // dmcrt.c
    staffCommands.emplace("*monster", 100, dmCreateMob, builderMob, "Summon a monster.");
    staffCommands.emplace("*cname", 100, dmCrtName, builderMob, "Rename a monster.");
    staffCommands.emplace("*possess", 100, dmAlias, isCt, "Possess a monster.");
    staffCommands.emplace("*cfollow", 100, dmFollow, isDm, "Make a monster follow you.");
    staffCommands.emplace("*attack", 100, dmAttack, builderMob, "Make a monster attack a player/monster.");
    staffCommands.emplace("*enemy", 100, dmListEnemy, isCt, "Show the enemies of a monster.");
    staffCommands.emplace("*charm", 100, dmListCharm, isCt, "Show the charm list of a player.");
    staffCommands.emplace("*wander", 100, dmForceWander, builderMob, "Force a monster to wander away.");
    staffCommands.emplace("*balance", 95, dmBalance, builderMob, "Balance a monster to a certain level.");


    // dmobj.c
    staffCommands.emplace("*create", 20, dmCreateObj, builderObj, "Summon an object.");
    staffCommands.emplace("*object", 20, dmCreateObj, builderObj, "Summon an object.");
    staffCommands.emplace("*oname", 100, dmObjName, builderObj, "Rename an object");
    staffCommands.emplace("*size", 100, dmSize, builderObj, "");
    staffCommands.emplace("*clone", 100, dmClone, builderObj, "Create an armor set from a single piece of armor.");


    // dmply.c
    staffCommands.emplace("*force", 20, dmForce, isDm, "Force a player to perform an action.");
    staffCommands.emplace("*spy", 100, dmSpy, isCt, "Spy on a player.");
    staffCommands.emplace("*silence", 100, dmSilence, isCt, "Toggle broadcast silence on a player.");
    staffCommands.emplace("*title", 100, dmTitle, isCt, "Set a title on a player.");
    staffCommands.emplace("*surname", 100, dmSurname, isCt, "Set a surname on a player.");
    staffCommands.emplace("*group", 100, dmGroup, isCt, "See who is grouped with a player.");
    staffCommands.emplace("*dust", 100, dmDust, isCt, "Permanently delete a player.");
    staffCommands.emplace("*tell", 100, dmFlash, isDm, "Send a player a message.");
    staffCommands.emplace("*beep", 100, dmBeep, isDm, "Make the player's terminal beep.");
    staffCommands.emplace("*advance", 100, dmAdvance, isDm, "Change the player's level and adjust stats appropriately.");
    staffCommands.emplace("dmfinger", 100, dmFinger, isDm, "fup an offline player's information.");
    staffCommands.emplace("*disconnect", 100, dmDisconnect, isCt, "Disconnect a player.");
    staffCommands.emplace("*take", 100, dmTake, isCt, "Take an item from a player.");
    staffCommands.emplace("*remove", 100, dmRemove, isCt, "Unequip and take an item from a player.");
    staffCommands.emplace("*computeluck", 100, dmComputeLuck, isCt, "Re-compute a player's luck value.");
    staffCommands.emplace("*put", 100, dmPut, isCt, "Put an item into a player's inventory.");
    staffCommands.emplace("*move", 100, dmMove, isCt, "Move and offline player.");
    staffCommands.emplace("*word", 100, dmWordAll, isDm, "Send all players to their recall room.");
    staffCommands.emplace("*pass", 100, dmPassword, isDm, "Change a player's password.");
    staffCommands.emplace("*restore", 100, dmRestorePlayer, isDm, "Restore a player's experience.");
    staffCommands.emplace("*bank", 100, dmBank, isCt, "");
    staffCommands.emplace("*assets", 100, dmInventoryValue, isCt, "");
    staffCommands.emplace("*bug", 100, dmBugPlayer, isCt, "Record a player's actions.");
    staffCommands.emplace("*kill", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*nomnom", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*combust", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*slime", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*gnat", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*trip", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*crush", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*missile", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*bomb", 100, dmKillSwitch, isCt, "Kill a player and everyone in room.");
    staffCommands.emplace("*nuke", 100, dmKillSwitch, isCt, "Kill a player and everyone in room.");
    staffCommands.emplace("*arma", 100, dmKillSwitch, isCt, "");
    staffCommands.emplace("*armageddon", 100, dmKillSwitch, isCt, "Kill all players.");
    staffCommands.emplace("*supernova", 100, dmKillSwitch, isCt, "Kill all players.");
    staffCommands.emplace("*igmoo", 100, dmKillSwitch, isCt, "Kill all players.");
    staffCommands.emplace("*rape", 100, dmKillSwitch, isCt, "Set a player to 1hp and 1mp.");
    staffCommands.emplace("*drain", 100, dmKillSwitch, isCt, "Set a player to 1hp and 1mp.");
    staffCommands.emplace("*grenade", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*head", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*stab", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*suicide", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*alcohol", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*drown", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*dragon", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*choke", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*demon", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*rats", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*comet", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*abyss", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*shard", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*flatulence", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*diarreah", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*kinetic", 100, dmKillSwitch, isCt, "Kill a player.");
    staffCommands.emplace("*repair", 100, dmRepair, isDm, "Fix a player's armor.");
    staffCommands.emplace("*max", 100, dmMax, nullptr, "Set your stats to max.");
    staffCommands.emplace("*backup", 100, dmBackupPlayer, isDm, "Backup a player or restore a player from backup.");
    staffCommands.emplace("*changestats", 100, dmChangeStats, isCt, "Allow a player to pick new stats.");
    staffCommands.emplace("*lt", 100, dmLts, isDm, "Show a player's last times.");
    staffCommands.emplace("*ltclear", 100, dmLtClear, isDm, "Clear a player's last times.");


    // dmroom.c
    staffCommands.emplace("*swap", 100, dmSwap, nullptr, "Swap utility.");
    staffCommands.emplace("*rswap", 100, dmRoomSwap, nullptr, "Swap current room with another.");
    staffCommands.emplace("*oswap", 100, dmObjSwap, nullptr, "Swap object with another.");
    staffCommands.emplace("*mswap", 100, dmMobSwap, nullptr, "Swap monster with another.");
    staffCommands.emplace("*purge", 100, dmPurge, nullptr, "Purge all objects and monsters (not pets) in room.");
    staffCommands.emplace("*echo", 100, dmEcho, nullptr, "Send a message to all players in the room.");
    staffCommands.emplace("*reload", 100, dmReloadRoom, nullptr, "Reload the room from disk.");
    staffCommands.emplace("*reset", 100, dmResetPerms, nullptr, "Reset perm timeouts.");
    staffCommands.emplace("*add", 100, dmAddRoom, nullptr, "Create a new room/object/monster.");
    staffCommands.emplace("*replace", 100, dmReplace, nullptr, "Replace text in a room description.");
    staffCommands.emplace("*substitute", 100, dmReplace, nullptr, "");
    staffCommands.emplace("*delete", 100, dmDelete, nullptr, "Delete text from a room description.");
    staffCommands.emplace("*name", 100, dmNameRoom, nullptr, "Rename the room.");
    staffCommands.emplace("*append", 100, dmAppend, nullptr, "Add text to a room description.");
    staffCommands.emplace("*app", 100, dmAppend, nullptr, "");
    staffCommands.emplace("*prepend", 100, dmPrepend, nullptr, "Prepend text to a room description.");
    staffCommands.emplace("*moblist", 100, dmMobList, builderMob, "List wandering monsters in the room.");
    staffCommands.emplace("*wrap", 100, dmWrap, nullptr, "Wrap the room description to a number of characters.");
    staffCommands.emplace("*xdelete", 100, dmDeleteAllExits, nullptr, "Delete all exits in the room.");
    staffCommands.emplace("*xarrange", 100, dmArrangeExits, nullptr, "Arrange all the exits in the room according to standards.");
    staffCommands.emplace("*fixup", 100, dmFixExit, nullptr, "Remove an underscore from an exit.");
    staffCommands.emplace("*unfixup", 100, dmUnfixExit, nullptr, "Add an underscore to an exit.");
    staffCommands.emplace("*xrename", 100, dmRenameExit, nullptr, "Rename an exit.");
    staffCommands.emplace("*destroy", 100, dmDestroyRoom, nullptr, "Destroy the room.");
    staffCommands.emplace("*find", 100, dmFind, nullptr, "Find the next available room/object/monster.");


    // post.c
    staffCommands.emplace("*readmail", 100, dmReadmail, isDm, "Read a player's mail.");
    staffCommands.emplace("*rmmail", 100, dmDeletemail, isDm, "Delete a player's mail.");
    staffCommands.emplace("*rmhist", 100, dmDeletehist, isDm, "Delete a player's history.");


    // skills.c
    staffCommands.emplace("*skills", 100, dmSkills, isDm, "Show a list of skills / a player/monsters's skills.");
    staffCommands.emplace("*setskills", 100, dmSetSkills, isDm, "Set a player's skills to an appropriate level.");
//  staffCommands.emplace("*addskills",      100,    dmAddSkills         0,      "");


    // guilds.c
    staffCommands.emplace("*guildlist", 100, dmListGuilds, isDm, "Show all guilds.");
    staffCommands.emplace("*approve", 100, dmApproveGuild, isCt, "Approve a proposed guild.");
    staffCommands.emplace("*reject", 100, dmRejectGuild, isCt, "Reject a proposed guild.");


    // bans.c
    staffCommands.emplace("*ban", 60, dmBan, isCt, "Ban a player.");
    staffCommands.emplace("*unban", 100, dmUnban, isCt, "Delete a ban.");
    staffCommands.emplace("*banlist", 100, dmListbans, isDm, "Show a list of bans.");


    // various files
    staffCommands.emplace("*fifo", 100, dmFifo, nullptr, "Delete and recreate the web interface fifos.");
    staffCommands.emplace("*locations", 100, dmStartLocs, isCt, "Show starting locations.");
    staffCommands.emplace("*specials", 100, dmSpecials, nullptr, "Show a monster's special attacks.");
    staffCommands.emplace("*range", 100, dmRange, nullptr, "Show your assigned room range.");
    staffCommands.emplace("*builder", 100, dmMakeBuilder, isCt, "Make a new character a builder.");
    staffCommands.emplace("*wat", 100, dmWatcherBroad, isCt, "Force a watcher to broadcast.");
    staffCommands.emplace("*demographics", 100, cmdDemographics, isDm, "Start the demographics routine.");
    staffCommands.emplace("*unique", 100, dmUnique, isCt, "Work with uniques.");
    staffCommands.emplace("*shipquery", 100, dmQueryShips, isDm, "See data about ships.");
    staffCommands.emplace("*spelling", 50, dmSpelling, nullptr, "Spell-check a room.");
    staffCommands.emplace("*spells", 100, dmSpellList, nullptr, "List spells in the game.");
    staffCommands.emplace("*effects", 90, dmEffectList, nullptr, "List effects in the game.");
    staffCommands.emplace("*effindex", 100, dmShowEffectsIndex, isCt, "List rooms in the effects index.");
    staffCommands.emplace("*songs", 100, dmSongList, nullptr, "List songs in the game.");
    staffCommands.emplace("*lottery", 100, dmLottery, isDm, "Run the lottery.");
    staffCommands.emplace("*memory", 100, dmMemory, isCt, "Show memory usage.");
    staffCommands.emplace("*active", 100, list_act, isCt, "Show monsters on the active list.");
    staffCommands.emplace("*classlist", 100, dmShowClasses, nullptr, "List all classes.");
    staffCommands.emplace("*racelist", 100, dmShowRaces, nullptr, "List all races.");
    staffCommands.emplace("*deitylist", 100, dmShowDeities, nullptr, "List all deities.");
    staffCommands.emplace("*faction", 100, dmShowFactions, nullptr, "List all factions or show a player/monster's faction.");
    staffCommands.emplace("bfart", 100, plyAction, isDm, "");
    staffCommands.emplace("*arealist", 100, dmListArea, isCt, "List area information.");
    staffCommands.emplace("*recipes", 100, dmRecipes, nullptr, "List recipes.");
    staffCommands.emplace("*combine", 100, dmCombine, nullptr, "Create new recipes.");
    staffCommands.emplace("*property", 100, dmProperties, isCt, "List properties.");
    staffCommands.emplace("*catrefinfo", 100, dmCatRefInfo, nullptr, "Show CatRefInfo.");

// *************************************************************************************
// Player Commands

    playerCommands.emplace("target", 100, cmdTarget, nullptr, "Target a creature or player.");
    playerCommands.emplace("assist", 100, cmdAssist, nullptr, "Assist a player, targeting their target.");
    // watcher functions
    playerCommands.emplace("*rename", 100, dmRename, nullptr, "");
    playerCommands.emplace("*warn", 100, dmWarn, nullptr, "");
    playerCommands.emplace("*jail", 100, dmJailPlayer, nullptr, "");
    playerCommands.emplace("*checkstats", 100, dmCheckStats, nullptr, "");
    playerCommands.emplace("*locate", 100, dmLocatePlayer, nullptr, "");
    playerCommands.emplace("*typo", 100, reportTypo, nullptr, "");
    playerCommands.emplace("bug", 100, reportBug, nullptr, "Report a bug in a room");
    playerCommands.emplace("*gag", 100, dmGag, nullptr, "");
    playerCommands.emplace("*wts", 100, channel, nullptr, "");
    playerCommands.emplace("*award", 100, dmAward, nullptr, "Award a player roleplaying exp/gold.");

    playerCommands.emplace("compare", 90, cmdCompare, nullptr, "Compare two items.");
    playerCommands.emplace("weapons", 100, cmdWeapons, nullptr, "Use weapon trains.");
    playerCommands.emplace("fish", 100, cmdFish, nullptr, "Go fishing.");
    playerCommands.emplace("religion", 100, cmdReligion, nullptr, "");
    playerCommands.emplace("craft", 100, cmdCraft, nullptr, "Create new items.");
    playerCommands.emplace("cook", 100, cmdCraft, nullptr, "Create new items.");
    playerCommands.emplace("smith", 100, cmdCraft, nullptr, "Create new items.");
    playerCommands.emplace("tailor", 100, cmdCraft, nullptr, "Create new items.");
    playerCommands.emplace("carpenter", 100, cmdCraft, nullptr, "");
    playerCommands.emplace("prepare", 100, cmdPrepare, nullptr, "Prepare for traps or prepare an item for crafting.");
    playerCommands.emplace("unprepare", 100, cmdUnprepareObject, nullptr, "Unprepare an item.");
    playerCommands.emplace("combine", 100, cmdCraft, nullptr, "Create new items.");


    // player functions
    playerCommands.emplace("forum", 100, cmdForum, nullptr, "Information on your forum account.");
    playerCommands.emplace("brew", 100, cmdBrew, nullptr, "");
    playerCommands.emplace("house", 100, cmdHouse, nullptr, "");
    playerCommands.emplace("property", 100, cmdProperties, nullptr, "Manage your properties.");
    playerCommands.emplace("title", 100, cmdTitle, nullptr, "Choose a custom title.");
    playerCommands.emplace("defecate", 80, plyAction, nullptr, "");
    playerCommands.emplace("tnl", 100, plyAction, nullptr, "Show the room how much experience you have to level.");
    playerCommands.emplace("mark", 100, plyAction, nullptr, "");
    playerCommands.emplace("show", 100, plyAction, nullptr, "");
    playerCommands.emplace("dream", 100, plyAction, nullptr, "");
    playerCommands.emplace("rollover", 100, plyAction, nullptr, "");
    playerCommands.emplace("where", 100, cmdLook, nullptr, "");
    playerCommands.emplace("traffic", 100, cmdTraffic, nullptr, "");
    playerCommands.emplace("look", 10, cmdLook, nullptr, "");
    playerCommands.emplace("consider", 50, cmdLook, nullptr, "");
    playerCommands.emplace("examine", 100, cmdLook, nullptr, "");
    playerCommands.emplace("reconnect", 100, cmdReconnect, nullptr, "Log in as another character.");
    playerCommands.emplace("relog", 100, cmdReconnect, nullptr, "");
    playerCommands.emplace("quit", 110, cmdQuit, nullptr, "Log off.");
    playerCommands.emplace("goodbye", 100, cmdQuit, nullptr, "");
    playerCommands.emplace("logout", 100, cmdQuit, nullptr, "");
    playerCommands.emplace("inventory", 10, cmdInventory, nullptr, "View your inventory.");
    playerCommands.emplace("who", 15, cmdWho, nullptr, "See who is online.");
    playerCommands.emplace("scan", 100, cmdWho, nullptr, "");
    playerCommands.emplace("classwho", 90, cmdClasswho, nullptr, "");
    playerCommands.emplace("whois", 100, cmdWhois, nullptr, "View information about a specific character.");
    playerCommands.emplace("wear", 100, cmdWear, nullptr, "");
    playerCommands.emplace("remove", 100, cmdRemoveObj, nullptr, "");
    playerCommands.emplace("rm", 110, cmdRemoveObj, nullptr, "");
    playerCommands.emplace("equipment", 100, cmdEquipment, nullptr, "View the equipment you are currently wearing.");
    playerCommands.emplace("hold", 100, cmdHold, nullptr, "");
    playerCommands.emplace("wield", 100, cmdReady, nullptr, "");
    playerCommands.emplace("ready", 100, cmdReady, nullptr, "");
    playerCommands.emplace("second", 100, cmdSecond, nullptr, "");
    playerCommands.emplace("help", 100, cmdHelp, nullptr, "");
    playerCommands.emplace("?", 100, cmdHelp, nullptr, "");
    playerCommands.emplace("wiki", 100, cmdWiki, nullptr, "Look up a wiki entry.");
    playerCommands.emplace("health", 100, cmdScore, nullptr, "");
    playerCommands.emplace("score", 50, cmdScore, nullptr, "Show brief information about your character.");
    playerCommands.emplace("status", 100, cmdInfo, nullptr, "");
    playerCommands.emplace("levelhistory", 100, cmdLevelHistory, nullptr, "Show character level history.");
    playerCommands.emplace("stats", 100, cmdStatistics, nullptr, "");
    playerCommands.emplace("statistics", 100, cmdStatistics, nullptr, "Show character-related statistics.");
    playerCommands.emplace("information", 50, cmdInfo, nullptr, "Show extended information about your character.");
    playerCommands.emplace("attributes", 100, cmdInfo, nullptr, "");
    playerCommands.emplace("skills", 100, cmdSkills, nullptr, "Show what skills your character knows");
    playerCommands.emplace("version", 100, cmdVersion, nullptr, "");
    playerCommands.emplace("age", 100, cmdAge, nullptr, "Show your character's age and time played.");
    playerCommands.emplace("birthday", 100, checkBirthdays, nullptr, "Show which players online have birthdays.");
    playerCommands.emplace("colors", 100, cmdColors, nullptr, "Show colors and set custom colors.");
    playerCommands.emplace("colours", 100, cmdColors, nullptr, "");
    playerCommands.emplace("factions", 100, cmdFactions, nullptr, "Show your standing with various factions.");
    playerCommands.emplace("recipes", 100, cmdRecipes, nullptr, "Show what recipes you know.");

    // player-only communication functions
    playerCommands.emplace("send", 100, communicateWith, nullptr, "");
    playerCommands.emplace("tell", 100, communicateWith, nullptr, "Send a message to another player.");
    playerCommands.emplace("osend", 100, communicateWith, nullptr, "");
    playerCommands.emplace("otell", 100, communicateWith, nullptr, "");
    playerCommands.emplace("reply", 100, communicateWith, nullptr, "Reply to a direct message.");
    playerCommands.emplace("sign", 100, communicateWith, nullptr, "");
    playerCommands.emplace("whisper", 100, communicateWith, nullptr, "Whisper a message to another player.");
    playerCommands.emplace("yell", 100, pCommunicate, nullptr, "Yell something that everyone in the room and nearby rooms can hear.");
    playerCommands.emplace("recite", 100, pCommunicate, nullptr, "Recite a phrase to open certain locked doors.");
    playerCommands.emplace("osay", 90, pCommunicate, nullptr, "");
    playerCommands.emplace("gtoc", 100, pCommunicate, nullptr, "");
    playerCommands.emplace("gtalk", 100, pCommunicate, nullptr, "Send a message to your group.");
    playerCommands.emplace("gt", 100, pCommunicate, nullptr, "");
    playerCommands.emplace("ignore", 100, cmdIgnore, nullptr, "Ignore a player.");
    playerCommands.emplace("speak", 100, cmdSpeak, nullptr, "Speak a different language.");
    playerCommands.emplace("languages", 100, cmdLanguages, nullptr, "Show what languages you know.");
    playerCommands.emplace("talk", 100, cmdTalk, nullptr, "Talk to a monster.");
    playerCommands.emplace("ask", 100, cmdTalk, nullptr, "");
    playerCommands.emplace("parley", 100, cmdTalk, nullptr, "");
    playerCommands.emplace("sendclan", 100, channel, nullptr, "");
    playerCommands.emplace("clansend", 100, channel, nullptr, "Communicate via the clan channel.");
    playerCommands.emplace("bemote", 100, channel, nullptr, "");
    playerCommands.emplace("broademote", 100, channel, nullptr, "");
    playerCommands.emplace("raemote", 100, channel, nullptr, "");
    playerCommands.emplace("sendrace", 100, channel, nullptr, "");
    playerCommands.emplace("racesend", 100, channel, nullptr, "Communicate via the race channel.");
    playerCommands.emplace("cls", 100, channel, nullptr, "");
    playerCommands.emplace("sendclass", 100, channel, nullptr, "");
    playerCommands.emplace("classsend", 100, channel, nullptr, "Communicate via the class channel.");
    playerCommands.emplace("clem", 100, channel, nullptr, "");
    playerCommands.emplace("classemote", 100, channel, nullptr, "");
    playerCommands.emplace("newbie", 100, channel, nullptr, "Communicate via the newbie channel.");
    playerCommands.emplace("ptest", 100, channel, nullptr, "");
    playerCommands.emplace("gossip", 100, channel, nullptr, "Communicate via the gossip channel.");
    playerCommands.emplace("sports", 100, channel, nullptr, "Communicate via the sports-talk channel.");
    playerCommands.emplace("adult", 100, channel, nullptr, "");
    playerCommands.emplace("broadcast", 80, channel, nullptr, "");
    playerCommands.emplace("gsend", 100, cmdGuildSend, nullptr, "Communicate via the guild channel.");

    playerCommands.emplace("guild", 100, cmdGuild, nullptr, "Perform guild-related commands.");
    playerCommands.emplace("guildhall", 100, cmdGuildHall, nullptr, "");
    playerCommands.emplace("gh", 100, cmdGuildHall, nullptr, "");
    playerCommands.emplace("follow", 100, cmdFollow, nullptr, "Follow another player.");
    playerCommands.emplace("lose", 100, cmdLose, nullptr, "Stop following another player.");
    playerCommands.emplace("group", 100, cmdGroup, nullptr, "Show group members.");
    playerCommands.emplace("party", 100, cmdGroup, nullptr, "");
    playerCommands.emplace("pay", 100, cmdPayToll, nullptr, "Pay a monster a toll.");
    playerCommands.emplace("track", 100, cmdTrack, nullptr, "");
    playerCommands.emplace("peek", 100, cmdPeek, nullptr, "");
    playerCommands.emplace("search", 100, cmdSearch, nullptr, "Search for anything hidden.");
    playerCommands.emplace("hide", 100, cmdHide, nullptr, "Hide from view.");
    playerCommands.emplace("telnet", 100, cmdTelOpts, nullptr, "Shows what telnet options have been negotiated.");
    playerCommands.emplace("telopts", 100, cmdTelOpts, nullptr, "Shows what telnet options have been negotiated.");
    playerCommands.emplace("set", 100, cmdPrefs, nullptr, "Set a preference.");
    playerCommands.emplace("clear", 100, cmdPrefs, nullptr, "Clear a preference.");
    playerCommands.emplace("toggle", 100, cmdPrefs, nullptr, "Toggle a preference.");
    playerCommands.emplace("preferences", 100, cmdPrefs, nullptr, "Show preferences.");
    playerCommands.emplace("open", 100, cmdOpen, nullptr, "Open a door.");
    playerCommands.emplace("close", 100, cmdClose, nullptr, "Close a door.");
    playerCommands.emplace("shut", 100, cmdClose, nullptr, "");
    playerCommands.emplace("unlock", 100, cmdUnlock, nullptr, "");
    playerCommands.emplace("lock", 100, cmdLock, nullptr, "");
    playerCommands.emplace("pick", 100, cmdPickLock, nullptr, "");
    playerCommands.emplace("steal", 100, cmdSteal, nullptr, "");
    playerCommands.emplace("flee", 40, cmdFlee, nullptr, "");
    playerCommands.emplace("run", 90, cmdFlee, nullptr, "");
    playerCommands.emplace("study", 100, cmdStudy, nullptr, "Study a scroll to learn a spell.");
    playerCommands.emplace("learn", 100, cmdStudy, nullptr, "");
    playerCommands.emplace("read", 100, cmdReadScroll, nullptr, "");
    playerCommands.emplace("commune", 100, cmdCommune, nullptr, "");
    playerCommands.emplace("bs", 100, cmdBackstab, nullptr, "");
    playerCommands.emplace("backstab", 100, cmdBackstab, nullptr, "");
    playerCommands.emplace("ambush", 100, cmdAmbush, nullptr, "");
    playerCommands.emplace("ab", 100, cmdAmbush, nullptr, "");
    playerCommands.emplace("train", 100, cmdTrain, nullptr, "Go up a level.");
    playerCommands.emplace("save", 100, cmdSave, nullptr, "");
    playerCommands.emplace("time", 100, cmdTime, nullptr, "Show the current time");
    playerCommands.emplace("circle", 50, cmdCircle, nullptr, "");
    playerCommands.emplace("bash", 50, cmdBash, nullptr, "");
    playerCommands.emplace("barkskin", 100, cmdBarkskin, nullptr, "");
    playerCommands.emplace("kick", 100, cmdKick, nullptr, "");
    playerCommands.emplace("gamestat", 100, infoGamestat, nullptr, "Game time statistics.");

    playerCommands.emplace("list", 100, cmdList, nullptr, "Show items for sale.");
    playerCommands.emplace("shop", 100, cmdShop, nullptr, "Perform player-shop-related commands.");
    playerCommands.emplace("buy", 100, cmdBuy, nullptr, "Purchase an item.");
    playerCommands.emplace("refund", 100, cmdRefund, nullptr, "Refund an item you just purchased.");
    playerCommands.emplace("reclaim", 100, cmdReclaim, nullptr, "Reclaim an item you just pawned.");
    playerCommands.emplace("shoplift", 100, cmdShoplift, nullptr, "");
    playerCommands.emplace("sell", 100, cmdSell, nullptr, "Sell an item.");
    playerCommands.emplace("value", 100, cmdValue, nullptr, "Check the value of an item.");
    playerCommands.emplace("cost", 100, cmdCost, nullptr, "");
    playerCommands.emplace("purchase", 90, cmdPurchase, nullptr, "");
    playerCommands.emplace("selection", 100, cmdSelection, nullptr, "");
    playerCommands.emplace("trade", 100, cmdTrade, nullptr, "Trade an item with a monster.");
    playerCommands.emplace("auction", 100, cmdAuction, nullptr, "Auction an item for players to buy.");
    playerCommands.emplace("repair", 100, cmdRepair, nullptr, "Have a smithy repair an item.");
    playerCommands.emplace("fix", 100, cmdRepair, nullptr, "");

    playerCommands.emplace("sendmail", 100, cmdSendMail, nullptr, "Send a mudmail to another player.");
    playerCommands.emplace("readmail", 100, cmdReadMail, nullptr, "Read your mudmail.");
    playerCommands.emplace("deletemail", 100, cmdDeleteMail, nullptr, "");
    playerCommands.emplace("edithistory", 100, cmdEditHistory, nullptr, "");
    playerCommands.emplace("history", 100, cmdHistory, nullptr, "");
    playerCommands.emplace("deletehistory", 100, cmdDeleteHistory, nullptr, "");

    playerCommands.emplace("drink", 100, cmdConsume, nullptr, "Drink a potion.");
    playerCommands.emplace("quaff", 100, cmdConsume, nullptr, "");
    playerCommands.emplace("eat", 100, cmdConsume, nullptr, "Eat some food.");
    playerCommands.emplace("recall", 100, cmdRecall, nullptr, "");
    playerCommands.emplace("hazy", 100, cmdRecall, nullptr, "");
    playerCommands.emplace("teach", 100, cmdTeach, nullptr, "");
    playerCommands.emplace("welcome", 100, cmdWelcome, nullptr, "");
    playerCommands.emplace("turn", 100, cmdTurn, nullptr, "");
    playerCommands.emplace("renounce", 100, cmdRenounce, nullptr, "");
    playerCommands.emplace("holyword", 100, cmdHolyword, nullptr, "");
    playerCommands.emplace("creep", 100, cmdCreepingDoom, nullptr, "");
    playerCommands.emplace("poison", 100, cmdPoison, nullptr, "");
    playerCommands.emplace("smother", 100, cmdEarthSmother, nullptr, "");
    playerCommands.emplace("bribe", 100, cmdBribe, nullptr, "Bribe a monster to leave the room.");
    playerCommands.emplace("frenzy", 100, cmdFrenzy, nullptr, "");
    playerCommands.emplace("pray", 100, cmdPray, nullptr, "");
    playerCommands.emplace("use", 100, cmdUse, nullptr, "");
    playerCommands.emplace("us", 100, cmdUse, nullptr, "");
    playerCommands.emplace("break", 100, cmdBreak, nullptr, "Break an object.");
    playerCommands.emplace("pledge", 100, cmdPledge, nullptr, "Pledge your allegience to a clan.");
    playerCommands.emplace("rescind", 100, cmdRescind, nullptr, "Rescind your allegience from a clan.");
    playerCommands.emplace("suicide", 100, cmdSuicide, nullptr, "Delete your character.");
    playerCommands.emplace("convert", 100, cmdConvert, nullptr, "Convert from chaotic alignment to lawful alignment.");
    playerCommands.emplace("password", 100, cmdPassword, nullptr, "Change your password.");
    playerCommands.emplace("proxy", 100, cmdProxy, nullptr, "Allow proxy access to this character.");
    playerCommands.emplace("finger", 100, cmdFinger, nullptr, "Look up an offline character.");
    playerCommands.emplace("hypnotize", 100, cmdHypnotize, nullptr, "");
    playerCommands.emplace("bite", 60, cmdBite, nullptr, "");
    playerCommands.emplace("mist", 100, cmdMist, nullptr, "");
    playerCommands.emplace("unmist", 100, cmdUnmist, nullptr, "");
    playerCommands.emplace("regenerate", 100, cmdRegenerate, nullptr, "");
    playerCommands.emplace("drain", 100, cmdDrainLife, nullptr, "");

    playerCommands.emplace("charm", 100, cmdCharm, nullptr, "");
    playerCommands.emplace("identify", 100, cmdIdentify, nullptr, "");
    playerCommands.emplace("songs", 100, cmdSongs, nullptr, "");

    playerCommands.emplace("enthrall", 100, cmdEnthrall, nullptr, "");
    playerCommands.emplace("meditate", 100, cmdMeditate, nullptr, "");
    playerCommands.emplace("focus", 100, cmdFocus, nullptr, "");
    playerCommands.emplace("mistbane", 90, cmdMistbane, nullptr, "");
    playerCommands.emplace("mb", 100, cmdMistbane, nullptr, "");
    playerCommands.emplace("touch", 100, cmdTouchOfDeath, nullptr, "");
    playerCommands.emplace("quests", 100, cmdQuests, nullptr, "View your quests and perform quest-related commands.");
    playerCommands.emplace("hands", 100, cmdLayHands, nullptr, "");
    playerCommands.emplace("harm", 100, cmdHarmTouch, nullptr, "");
    playerCommands.emplace("scout", 100, cmdScout, nullptr, "");
    playerCommands.emplace("berserk", 100, cmdBerserk, nullptr, "");
    playerCommands.emplace("transmute", 100, cmdTransmute, nullptr, "");
    playerCommands.emplace("daily", 100, cmdDaily, nullptr, "See your usage of daily-limited abilities.");
    playerCommands.emplace("checksaves", 100, cmdChecksaves, nullptr, "See your current saves and saves gained.");
    playerCommands.emplace("description", 100, cmdDescription, nullptr, "Change your character's description.");
    playerCommands.emplace("bsacrifice", 100, cmdBloodsacrifice, nullptr, "");
    playerCommands.emplace("bloodsacrifice", 100, cmdBloodsacrifice, nullptr, "");
    playerCommands.emplace("disarm", 100, cmdDisarm, nullptr, "Disarm an opponent or remove your wielded weapons.");
    playerCommands.emplace("visible", 100, cmdVisible, nullptr, "Cancel invisibility.");
    playerCommands.emplace("watch", 100, cmdWatch, nullptr, "");
    playerCommands.emplace("conjure", 100, conjureCmd, nullptr, "");
    playerCommands.emplace("maul", 100, cmdMaul, nullptr, "");
    playerCommands.emplace("enchant", 100, cmdEnchant, nullptr, "");
    playerCommands.emplace("envenom", 100, cmdEnvenom, nullptr, "");

    playerCommands.emplace("balance", 90, cmdBalance, nullptr, "See your bank balance.");
    playerCommands.emplace("deposit", 100, cmdDeposit, nullptr, "Deposit gold in the bank.");
    playerCommands.emplace("withdraw", 100, cmdWithdraw, nullptr, "Withdraw gold from the bank.");
    playerCommands.emplace("deletestatement", 100, cmdDeleteStatement, nullptr, "");
    playerCommands.emplace("transfer", 100, cmdTransfer, nullptr, "Transfer gold to another character.");
    playerCommands.emplace("statement", 90, cmdStatement, nullptr, "View your bank statement.");

    playerCommands.emplace("refuse", 100, cmdRefuse, nullptr, "");
    playerCommands.emplace("duel", 100, cmdDuel, nullptr, "Challenge another player to a duel.");
    playerCommands.emplace("animate", 100, animateDeadCmd, nullptr, "");
    playerCommands.emplace("lottery", 100, cmdLottery, nullptr, "View lottery info.");
    playerCommands.emplace("claim", 100, cmdClaim, nullptr, "Claim a lottery ticket.");
    playerCommands.emplace("mccp", 100, mccp, nullptr, "Toggle mccp (compression).");
    playerCommands.emplace("surname", 100, cmdSurname, nullptr, "Choose a surname.");
    playerCommands.emplace("changestats", 100, cmdChangeStats, nullptr, "");
    playerCommands.emplace("keep", 100, cmdKeep, nullptr, "Prevent accidentially throwing away an item.");
    playerCommands.emplace("unkeep", 100, cmdUnkeep, nullptr, "Unkeep an item.");
    playerCommands.emplace("label", 100, cmdLabel, nullptr, "Set a custom label on an item.");
    playerCommands.emplace("alignment", 100, cmdChooseAlignment, nullptr, "Choose your alignment.");
    playerCommands.emplace("push", 100, cmdPush, nullptr, "");
    playerCommands.emplace("pull", 100, cmdPull, nullptr, "");
    playerCommands.emplace("press", 100, cmdPress, nullptr, "");
    playerCommands.emplace("shipquery", 90, cmdQueryShips, nullptr, "See where chartered ships are.");
    playerCommands.emplace("ships", 100, cmdQueryShips, nullptr, "");

    playerCommands.emplace("bandage", 100, cmdBandage, nullptr, "Use bandages to heal yourself.");

    // movement
    playerCommands.emplace("north", 10, cmdMove, nullptr, "");
    playerCommands.emplace("south", 10, cmdMove, nullptr, "");
    playerCommands.emplace("east", 10, cmdMove, nullptr, "");
    playerCommands.emplace("west", 10, cmdMove, nullptr, "");
    playerCommands.emplace("northeast", 15, cmdMove, nullptr, "");
    playerCommands.emplace("ne", 15, cmdMove, nullptr, "");
    playerCommands.emplace("northwest", 15, cmdMove, nullptr, "");
    playerCommands.emplace("nw", 15, cmdMove, nullptr, "");
    playerCommands.emplace("southeast", 15, cmdMove, nullptr, "");
    playerCommands.emplace("se", 15, cmdMove, nullptr, "");
    playerCommands.emplace("southwest", 15, cmdMove, nullptr, "");
    playerCommands.emplace("sw", 15, cmdMove, nullptr, "");
    playerCommands.emplace("up", 10, cmdMove, nullptr, "");
    playerCommands.emplace("down", 10, cmdMove, nullptr, "");
    playerCommands.emplace("out", 100, cmdMove, nullptr, "");
    playerCommands.emplace("leave", 100, cmdMove, nullptr, "");
    playerCommands.emplace("sneak", 50, cmdGo, nullptr, "Sneak to an exit so you are not seen.");
    playerCommands.emplace("go", 100, cmdGo, nullptr, "");
    playerCommands.emplace("exit", 100, cmdGo, nullptr, "");
    playerCommands.emplace("enter", 100, cmdGo, nullptr, "");

// *************************************************************************************
// General player/pet Commands

    // general player/pet commands
    generalCommands.emplace("say", 100, communicate, nullptr, "");
    generalCommands.emplace("\"", 100, communicate, nullptr, "");
    generalCommands.emplace("'", 100, communicate, nullptr, "");
    generalCommands.emplace("emote", 100, communicate, nullptr, "");
    generalCommands.emplace(":", 100, communicate, nullptr, "");

    generalCommands.emplace("spells", 100, cmdSpells, nullptr, "Show what spells you know.");
    generalCommands.emplace("cast", 10, cmdCast, nullptr, "");

    generalCommands.emplace("attack", 50, cmdAttack, nullptr, "");
    generalCommands.emplace("kill", 10, cmdAttack, nullptr, "");

    generalCommands.emplace("get", 100, cmdGet, nullptr, "Pick up an item and put it in your inventory.");
    generalCommands.emplace("take", 100, cmdGet, nullptr, "");
    generalCommands.emplace("drop", 100, cmdDrop, nullptr, "Remove an item from your inventory and drop it on the ground.");
    generalCommands.emplace("put", 100, cmdDrop, nullptr, "");
    generalCommands.emplace("give", 100, cmdGive, nullptr, "");

    generalCommands.emplace("throw", 100, cmdThrow, nullptr, "Throw an object.");
    generalCommands.emplace("knock", 100, cmdKnock, nullptr, "Knock on a door to make a noise on the other side.");
    generalCommands.emplace("effects", 100, cmdEffects, nullptr, "Show what effects you are currently under.");
    generalCommands.emplace("howl", 100, cmdHowl, nullptr, "");
    generalCommands.emplace("sing", 100, cmdSing, nullptr, "");

    // socials
    generalCommands.emplace("chest", 100, cmdAction, nullptr, "");
    generalCommands.emplace("stab", 100, cmdAction, nullptr, "");
    generalCommands.emplace("vangogh", 100, cmdAction, nullptr, "");
    generalCommands.emplace("smooch", 100, cmdAction, nullptr, "");
    generalCommands.emplace("lobotomy", 100, cmdAction, nullptr, "");
    generalCommands.emplace("sleep", 100, cmdAction, nullptr, "Go to sleep to heal faster.");
    generalCommands.emplace("stand", 100, cmdAction, nullptr, "Stand up from a sitting position.");
    generalCommands.emplace("sit", 100, cmdAction, nullptr, "Sit down to heal faster.");
    generalCommands.emplace("dismiss", 100, cmdAction, nullptr, "");
    generalCommands.emplace("burp", 100, cmdAction, nullptr, "");
    generalCommands.emplace("rps", 100, cmdAction, nullptr, "");
    generalCommands.emplace("masturbate", 90, cmdAction, nullptr, "");
    generalCommands.emplace("hum", 100, cmdAction, nullptr, "");
    // "pet" also links to orderPet within action
    generalCommands.emplace("pet", 100, cmdAction, nullptr, "");
    generalCommands.emplace("pat", 100, cmdAction, nullptr, "");
    generalCommands.emplace("glare", 100, cmdAction, nullptr, "");
    generalCommands.emplace("bless", 100, cmdAction, nullptr, "");
    generalCommands.emplace("flip", 100, cmdAction, nullptr, "");
    generalCommands.emplace("point", 100, cmdAction, nullptr, "");
    generalCommands.emplace("afk", 100, cmdAction, nullptr, "");
    generalCommands.emplace("fangs", 100, cmdAction, nullptr, "");
    generalCommands.emplace("hiss", 100, cmdAction, nullptr, "");
    generalCommands.emplace("hunger", 100, cmdAction, nullptr, "");
    generalCommands.emplace("freeze", 100, cmdAction, nullptr, "");
    generalCommands.emplace("ballz", 100, cmdAction, nullptr, "");
    generalCommands.emplace("wag", 100, cmdAction, nullptr, "");
    generalCommands.emplace("murmur", 100, cmdAction, nullptr, "");
    generalCommands.emplace("snore", 100, cmdAction, nullptr, "");
    generalCommands.emplace("tag", 100, cmdAction, nullptr, "");
    generalCommands.emplace("narrow", 100, cmdAction, nullptr, "");
    generalCommands.emplace("gnaw", 100, cmdAction, nullptr, "");
    generalCommands.emplace("sneeze", 100, cmdAction, nullptr, "");
    // wake is a special command that you might be able to
    // use while unconscious. rules are in Creature::ableToDoCommand
    generalCommands.emplace("wake", 100, cmdAction, nullptr, "Wake up from being asleep.");

    generalCommands.emplace("dice", 100, cmdDice, nullptr, "Roll dice.");


    // Once we've built the command tables, write their help files to the help directory
    writeCommandFile(CreatureClass::NONE, Path::Help.c_str(), "commands");
    writeCommandFile(CreatureClass::DUNGEONMASTER, Path::DMHelp.c_str(), "dmcommands");
    writeCommandFile(CreatureClass::BUILDER, Path::BuilderHelp.c_str(), "bhcommands");

    return (true);
}

void Config::clearCommands() {
    staffCommands.clear();
    playerCommands.clear();
    generalCommands.clear();
}

bool MudMethod::exactMatch(const std::string& toMatch) const {
    return boost::iequals(name, toMatch);
}

bool MudMethod::partialMatch(const std::string& toMatch) const {
    return(!strncasecmp(name.c_str(), toMatch.c_str(), toMatch.length()));
}

void MysticMethod::parseName() {
    boost::char_separator<char> sep(" ");
    boost::tokenizer< boost::char_separator<char> > tokens(name, sep);

    for(const auto& t : tokens) {
        nameParts.emplace_back(t);
    }
}

//*********************************************************************
//                      getCommand
//*********************************************************************

// bestMethod is a reference to a pointer
template<class Type, class Type2>
void examineList(std::set<Type, namableCmp>& mySet, const std::string &str, int& match, bool& found, const Type2*& bestMethod) {
    if(str.length() == 0)
        return;

    const Type2* curMethod = nullptr;

    // Narrow down the range of the map we'll be looking at
    auto it = mySet.lower_bound({str});
    auto endIt = mySet.upper_bound({std::string(1, char(str.at(0) + 1))});

    while(it != endIt) {
        curMethod = &(*it++);

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

void getCommand(const std::shared_ptr<Creature>& user, cmd* cmnd) {
    std::string str;
    // Match - Partial matches
    int     match=0;
    // Found - Exact match
    bool    found=false;
//  int     i=0;

    std::shared_ptr<Player> pUser = user->getAsPlayer();
    str = cmnd->str[0];

    //std::clog << "Looking for a match for '" << str << "'\n";

    examineList<CrtCommand, Command>(gConfig->generalCommands, str, match, found, cmnd->myCommand);
    if(!found)
        examineList<SkillCommand, Command>(gConfig->skillCommands, str, match, found, cmnd->myCommand);
    // If we're a player, examine a few extra lists
    if(!found && pUser) {
        if(!found) // No exact match so far, check player list
            examineList<PlyCommand, Command>(gConfig->playerCommands, str, match, found, cmnd->myCommand);
        if(!found && pUser->isStaff()) // Still no exact match, check staff commands
            examineList<PlyCommand, Command>(gConfig->staffCommands, str, match, found, cmnd->myCommand);
    }
    if(!found)
        examineList<SocialCommand, Command>(gConfig->socials, str, match, found, cmnd->myCommand);

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

const Spell *Config::getSpell(const std::string &id, int& ret) {
    const Spell *toReturn = nullptr;
    int match = 0;
    bool found = false;

    examineList<Spell, Spell>(spells, id, match, found, toReturn);

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

const Song *Config::getSong(const std::string &pName) {
    const Song *toReturn = nullptr;
    int match = 0;
    bool found = false;

    examineList<Song, Song>(songs, pName, match, found, toReturn);

    return(toReturn);
}

const Song *Config::getSong(const std::string &name, int& ret){
    const Song *toReturn = nullptr;
    int match = 0;
    bool found = false;

    examineList<Song, Song>(songs, name, match, found, toReturn);

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

static std::set<std::string> commandsAllowedWhilePetrified = {
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
int allowedWhilePetrified(const std::string &str) {
    return commandsAllowedWhilePetrified.find(str) != commandsAllowedWhilePetrified.end();
}


//*********************************************************************
//                      cmdProcess
//*********************************************************************
// This function takes the command structure of the person at the socket
// in the first parameter and interprets the person's command.

int cmdProcess(std::shared_ptr<Creature> user, cmd* cmnd, const std::shared_ptr<Creature>& pet) {
    if(!user) {
        std::clog << "invalid creature trying to use cmdProcess.\n";
        return(0);
    }

    std::shared_ptr<Player> player=nullptr;
    int     fd = user->fd;

    if(fd > 0) {
        if(!pet) {
            player = user->getAsPlayer();
        } else
            user = pet;
    }

    if(player && player->afterProf) {
        player->afterProf = 0;
    }

    getCommand(user, cmnd);

    if(cmnd->ret == CMD_NOT_FOUND) {
        if(pet) {
            user->print("Tell %N to do what?\n", user.get());
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

std::string Nameable::getName() const {
    return(name);
}

std::string Nameable::getDescription() const {
    return(description);
}

int PlyCommand::execute(const std::shared_ptr<Creature>& player, cmd* cmnd) const {
    std::shared_ptr<Player>ply = player->getAsPlayer();
    if(!ply) return(false);

    return((fn)(ply, cmnd));
}

int CrtCommand::execute(const std::shared_ptr<Creature>& player, cmd* cmnd) const {
    return((fn)(player, cmnd));
}

std::string_view MysticMethod::getScript() const {
    return(script);
}
