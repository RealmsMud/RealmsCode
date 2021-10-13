/*
 * dm.cpp
 *   Standard staff functions
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

#include <cctype>                 // for isspace, tolower
#include <cstdio>                 // for sprintf
#include <cstring>                // for strcmp, strlen, strcpy, strcat, strchr
#include <ctime>                  // for time
#include <unistd.h>               // for getpid
#include <iomanip>                // for operator<<, setw
#include <list>                   // for operator==, operator!=
#include <map>                    // for operator==, operator!=, map
#include <sstream>                // for operator<<, basic_ostream, ostrings...
#include <string>                 // for operator<<, operator!=, operator==
#include <utility>                // for pair
#include <fmt/format.h>

#include "area.hpp"               // for MapMarker, Area
#include "bstring.hpp"            // for bstring, operator+
#include "catRef.hpp"             // for CatRef
#include "cmd.hpp"                // for cmd
#include "commands.hpp"           // for cmdNoAuth, getFullstrText, dmStatDe...
#include "config.hpp"             // for Config, gConfig, AlchemyMap
#include "creatureStreams.hpp"    // for Streamable, ColorOff, ColorOn
#include "creatures.hpp"          // for Player, Creature, Monster
#include "dm.hpp"                 // for stat_rom, dmLastCommand, dmListbans
#include "flags.hpp"              // for P_DM_INVIS, P_OUTLAW, P_NO_SUMMON
#include "global.hpp"             // for CreatureClass, PROMPT, CreatureClas...
#include "lasttime.hpp"           // for lasttime, crlasttime
#include "location.hpp"           // for Location
#include "magic.hpp"              // for S_ANNUL_MAGIC, S_AURA_OF_FLAME, S_B...
#include "mud.hpp"                // for StartTime, LT_OUTLAW, LT_FREE_ACTION
#include "objects.hpp"            // for Object
#include "oldquest.hpp"           // for quest, questPtr
#include "os.hpp"                 // for merror
#include "paths.hpp"              // for Log, BuilderHelp, DMHelp, Post
#include "proto.hpp"              // for get_spell_name, broadcast, log_immort
#include "quests.hpp"             // for QuestInfo
#include "random.hpp"             // for Random
#include "rooms.hpp"              // for UniqueRoom, BaseRoom, AreaRoom, NUM...
#include "server.hpp"             // for Server, gServer, SocketList, Monste...
#include "socket.hpp"             // for Socket, OutBytes, UnCompressedBytes
#include "utils.hpp"              // for MAX
#include "weather.hpp"            // for WEATHER_BEAUTIFUL_DAY, WEATHER_BRIG...
#include "xml.hpp"                // for iToYesNo, loadRoom, loadObject


long        last_dust_output;
extern long last_weather_update;



//*********************************************************************
//                      dmReboot
//*********************************************************************

int dmReboot(Player* player, cmd* cmnd) {
    bool    resetShips=false;


    if( !player->isDm() &&
        !(player->flagIsSet(P_CAN_REBOOT) &&
        player->getClass() == CreatureClass::CARETAKER)
    )
        return(cmdNoAuth(player));

    if(cmnd->num==2 && !strcmp(cmnd->str[1], "-ships"))
        resetShips = true;

    player->print("Rebooting now!\n");
    gConfig->swapAbort();
    logn("log.bane", "Reboot by %s.\n", player->getCName());
    broadcast("### Going for a reboot, hang onto your seats!");
    if(resetShips)
        player->print("Resetting game time to midnight, updating ships...\n");
    gServer->processOutput();
    loge("--- Attempting game reboot ---\n");
    gServer->resaveAllRooms(0);

    gServer->startReboot(resetShips);

    merror("dmReboot failed!!!", FATAL);
    return(0);

}

//*********************************************************************
//                      dmMobInventory
//*********************************************************************

int dmMobInventory(Player* player, cmd* cmnd) {
    Monster *monster=nullptr;
    Object  *object;
    //char  str[2048];
    int     i=0;

    if(!player->canBuildMonsters())
        return(cmdNoAuth(player));


    if(cmnd->num < 2) {
        player->print("Which monster?\n");
        return(0);
    }

    monster = player->getParent()->findMonster(player, cmnd);
    if(!monster) {
        player->print("That's not here.\n");
        return(0);
    }



    if(!strcmp(cmnd->str[2], "-l") || player->getClass() == CreatureClass::BUILDER) {
        player->print("Items %s will possibly drop:\n", monster->getCName());
        for(i=0;i<10;i++) {
            if(!monster->carry[i].info.id) {
                player->print("Carry slot %d: %sNothing.\n", i+1, i < 9 ? " " : "");
                continue;
            }

            if(!loadObject(monster->carry[i].info, &object))
                continue;
            if(!object)
                continue;

            if(player->getClass() == CreatureClass::BUILDER && player->checkRangeRestrict(object->info))
                player->print("Carry slot %d: %sout of range(%s).\n", i+1, i < 9 ? " " : "", object->info.str().c_str());
            else
                player->printColor("Carry slot %d: %s%s(%s).\n", i+1, i < 9 ? " " : "", object->getCName(), object->info.str().c_str());

            delete object;
        }

        return(0);
    }
    bstring str = monster->listObjects(player, true);
    bstring prefix =bstring(monster->flagIsSet(M_NO_PREFIX) ? "":"The ") + monster->getName() + " is carrying: ";
    if(!str.empty()) {
        str =  prefix + str + ".";
    } else {
        str = prefix + "Nothing.";
    }

    *player << ColorOn << str << "\n" << ColorOff;

    return(0);
}

//*********************************************************************
//                      dmSockets
//*********************************************************************

int dmSockets(Player* player, cmd* cmnd) {
    int num=0;

    player->print("Connected Sockets:\n");

    for(Socket &sock : gServer->sockets) {
        num += 1;
        player->bPrint(fmt::format("Fd: {:-2}   {} ({})\n", sock.getFd(), sock.getHostname(), sock.getIdle()));
    }
    player->print("%d total connection%s.\n", num, num != 1 ? "s" : "");
    return(PROMPT);
}




//*********************************************************************
//                      dmLoadSave
//*********************************************************************
// handles loading and saving of a bunch of config files

int dmLoadSave(Player* player, cmd* cmnd, bool load) {
    if(cmnd->num < 2) {
        std::list<bstring> loadList;
        std::list<bstring> saveList;
        std::list<bstring>::const_iterator it;

        // create a list of possible effects
        loadList.emplace_back("areas");
        loadList.emplace_back("bans");
        loadList.emplace_back("calendar");
        loadList.emplace_back("catrefinfo");
        loadList.emplace_back("config");
        loadList.emplace_back("classes");
        loadList.emplace_back("clans");
        loadList.emplace_back("deities");
        loadList.emplace_back("faction");
        loadList.emplace_back("fishing");
        loadList.emplace_back("flags");
        loadList.emplace_back("guilds");
        loadList.emplace_back("limited");
        loadList.emplace_back("properties");
        loadList.emplace_back("proxies");
        loadList.emplace_back("quests");
        loadList.emplace_back("races");
        loadList.emplace_back("recipes");
        loadList.emplace_back("ships");
        loadList.emplace_back("skills");
        loadList.emplace_back("socials");
        loadList.emplace_back("spells");
        loadList.emplace_back("spl [spell id]");
        loadList.emplace_back("startlocs");

        saveList.emplace_back("bans");
        saveList.emplace_back("config");
        saveList.emplace_back("guilds");
        saveList.emplace_back("limited");
        saveList.emplace_back("properties");
        saveList.emplace_back("proxies");
        saveList.emplace_back("recipes");
        saveList.emplace_back("socials");
        saveList.emplace_back("spells");

        // run through the lists and display them
        player->print("*dmload / *dmsave supports the following options:\n");

        player->printColor("^yLOAD:\n");
        bool leftColumn = true;
        for(it = loadList.begin() ; it != loadList.end() ; it++) {
            player->printColor("   %-15s", (*it).c_str());
            if(!leftColumn)
                player->print("\n");
            leftColumn = !leftColumn;
        }
        if(leftColumn)
            player->print("\n");
        player->print("   \n");


        // now print the save list
        player->printColor("^ySAVE:\n");
        leftColumn = true;
        for(it = saveList.begin() ; it != saveList.end() ; it++) {
            player->printColor("   %-15s", (*it).c_str());
            if(!leftColumn)
                player->print("\n");
            leftColumn = !leftColumn;
        }
        if(leftColumn)
            player->print("\n");
        player->print("\n");
        return(0);
    }

    if(!strcmp(cmnd->str[1], "bans")) {
        if(load) {
            gConfig->loadBans();
            player->print("Bans reloaded.\n");
            dmListbans(player, nullptr);
        } else {
            gConfig->saveBans();
            player->print("Bans saved.\n");
        }
    } else if(!strcmp(cmnd->str[1], "config")) {
        if(load) {
            player->print("Reloading configuration file.\n");
            bool result = gConfig->loadConfig(true);
            if(result)
                player->print("Success!\n");
            else
                player->print("Failed!\n");
        } else {
            player->print("Saving configuration file.\n");
            gConfig->saveConfig();
        }
    } else if(!strcmp(cmnd->str[1], "guilds")) {
        if(load) {
            player->print("Reloading guilds.\n");
            gConfig->loadGuilds();
        } else {
            player->print("Saving guilds.\n");
            gConfig->saveGuilds();
        }
    }
    else if(!strcmp(cmnd->str[1], "socials")) {
        if(load) {
            gConfig->clearSocials();
            gConfig->loadSocials();
            gConfig->writeSocialFile();

            *player << "Socials reloaded.\n";
        } else {
            gConfig->saveSocials();
            *player << "Socials saved\n";
        }
    }
    else if(!strcmp(cmnd->str[1], "recipes")) {
        if(load) {
            gConfig->loadRecipes();
            player->print("Recipes reloaded.\n");
        } else {
            gConfig->saveRecipes();
            player->print("Recipes saved.\n");
        }
    } else if(!strcmp(cmnd->str[1], "properties")) {
        if(load) {
            gConfig->loadProperties();
            player->print("Properties reloaded.\n");
        } else {
            gConfig->saveProperties();
            player->print("Properties saved.\n");
        }
    } else if(!strcmp(cmnd->str[1], "proxies")) {
        if(load) {
            gConfig->loadProxyAccess();
            player->print("Proxies reloaded.\n");
        } else {
            gConfig->saveProxyAccess();
            player->print("Proxies saved.\n");
        }
    } else if(!strcmp(cmnd->str[1], "limited")) {
        if(load) {
            gConfig->loadLimited();
            player->print("Limited items reloaded.\n");
        } else {
            gConfig->saveLimited();
            player->print("Limited items saved.\n");
        }
    } else if(!strcmp(cmnd->str[1], "spells")) {
        if(load) {
            gConfig->loadSpells();
            player->print("Spell list reloaded.\n");
        } else {
            gConfig->saveSpells();
            gConfig->writeSpellFiles();
            player->print("Spell list saved.\n");
        }
    } else if(!strcmp(cmnd->str[1], "areas") && load) {
        if(!strcmp(cmnd->str[2], "confirm")) {
            for(const auto& area : gServer->areas) {
                for (const auto& [roomId, aRoom] : area->rooms) {
                    aRoom->setStayInMemory(true);
                    aRoom->expelPlayers(false, true, true);
                }
            }

            gServer->loadAreas();
            player->print("Area reloaded.\n");
        } else {
            player->print("This will expel all current players in the overland to their recall rooms.\n");
            player->printColor("Type ^y*dmload areas confirm^x to continue.\n");
        }
    } else if(!strcmp(cmnd->str[1], "quests") && load) {
        gConfig->loadQuestTable();
        gConfig->loadQuests();
        player->print("Quests reloaded.\n");
    }
    else if(!strcmp(cmnd->str[1], "clans") && load) {
        gConfig->loadClans();
        player->print("Clans reloaded.\n");
    } else if(!strcmp(cmnd->str[1], "classes") && load) {
        gConfig->loadClasses();
        player->print("Classes reloaded.\n");
    } else if(!strcmp(cmnd->str[1], "races") && load) {
        gConfig->loadRaces();
        player->print("Races reloaded.\n");
    } else if(!strcmp(cmnd->str[1], "deities") && load) {
        gConfig->loadDeities();
        player->print("Deities reloaded.\n");
    } else if(!strcmp(cmnd->str[1], "skills") && load) {
        gConfig->loadSkills();
        player->print("Skills reloaded.\n");
    } else if(!strcmp(cmnd->str[1], "startlocs") && load) {
        gConfig->loadStartLoc();
        player->print("StartLocs reloaded.\n");
    } else if(!strcmp(cmnd->str[1], "catrefinfo") && load) {
        gConfig->loadCatRefInfo();
        player->print("CatRefInfo reloaded.\n");
    } else if(!strcmp(cmnd->str[1], "flags") && load) {
        gConfig->loadFlags();
        player->print("Flags reloaded.\n");
    } else if(!strcmp(cmnd->str[1], "faction") && load) {
        gConfig->loadFactions();
        player->print("Factions reloaded.\n");
    } else if(!strcmp(cmnd->str[1], "fishing") && load) {
        gConfig->loadFishing();
        player->print("Fishing reloaded.\n");
    } else if(!strcmp(cmnd->str[1], "calendar") && load) {
        reloadCalendar(player);
    } else {
        player->print("Invalid request!\n");
        cmnd->num = 1;
        dmLoadSave(player, cmnd, load);
    }

    return(0);
}

//*********************************************************************
//                      dmLoad
//*********************************************************************
// a wrapper

int dmLoad(Player* player, cmd* cmnd) {
    dmLoadSave(player, cmnd, true);
    return(0);
}

//*********************************************************************
//                      dmSave
//*********************************************************************
// a wrapper

int dmSave(Player* player, cmd* cmnd) {
    dmLoadSave(player, cmnd, false);
    return(0);
}

//*********************************************************************
//                      dmTeleport
//*********************************************************************
// This function allows staff to teleport to a given room number, or to
// a player's location.  It will also teleport a player to the DM or
// one player to another.

void Player::dmPoof(BaseRoom* room, BaseRoom *newRoom) {
    if(flagIsSet(P_ALIASING)) {
        alias_crt->deleteFromRoom();
        broadcast(getSock(), room, "%M just wandered away.", alias_crt);
        if(newRoom)
            alias_crt->addToRoom(newRoom);
    }

    if(flagIsSet(P_DM_INVIS)) {
        if(isDm())
            broadcast(::isDm, getSock(), room, "*DM* %s disappears in a puff of smoke.", getCName());
        if(cClass == CreatureClass::CARETAKER)
            broadcast(::isCt, getSock(), room, "*DM* %s disappears in a puff of smoke.", getCName());
        if(!isCt())
            broadcast(::isStaff, getSock(), room, "*DM* %s disappears in a puff of smoke.", getCName());
    } else {
        broadcast(getSock(), room, "%M disappears in a puff of smoke.", this);
    }
}

int dmTeleport(Player* player, cmd* cmnd) {
    BaseRoom    *room=nullptr, *old_room=nullptr;
    UniqueRoom      *uRoom=nullptr;
    Creature    *creature=nullptr;
    Player      *target=nullptr, *target2=nullptr;

    Location    l;


    // *t 1 and *t 1.-10.7 will trigger num < 2,
    // *t misc.100 will not, so we need to make an exception
    bstring str = getFullstrText(cmnd->fullstr, 1);
    bstring txt = getFullstrText(str, 1, '.');

    // set default *t room
    if(str.empty()) {
        l = player->getRecallRoom();

        if(player->inAreaRoom() && l.mapmarker == player->currentLocation.mapmarker) {
            player->print("You are already there!\n");
            return(0);
        }

        room =  l.loadRoom();
    } else if(cmnd->num < 2 || (!txt.empty() && txt.getAt(0))) {
        Area        *area=nullptr;

        getDestination(str, &l, player);

        if(player->getClass() !=  CreatureClass::BUILDER && l.mapmarker.getArea()) {
            if(player->inAreaRoom() && l.mapmarker == player->currentLocation.mapmarker) {
                player->print("You are already there!\n");
                return(0);
            }

            area = gServer->getArea(l.mapmarker.getArea());
            if(!area) {
                player->print("Area does not exist.\n");
                return(0);
            }

            // pointer to old room
            player->dmPoof(player->getRoomParent(), nullptr);
            area->move(player, &l.mapmarker);
            // manual
            if(player->flagIsSet(P_ALIASING))
                player->getAlias()->addToRoom(player->getRoomParent());
            return(0);
        }


        if(!validRoomId(l.room)) {
            player->print("Error: out of range.\n");
            return(0);
        }
        if(!loadRoom(l.room, &uRoom)) {
            player->print("Error (%s)\n", l.str().c_str());
            return(0);
        }

        if(!player->checkBuilder(l.room))
            return(0);

        room = uRoom;

    } else if(cmnd->num < 3) {

        lowercize(cmnd->str[1], 1);
        creature = gServer->findPlayer(cmnd->str[1]);
        if(!creature || creature == player || !player->canSee(creature)) {
            player->print("%s is not on.\n", cmnd->str[1]);
            return(0);
        }

        if(player->getClass() == CreatureClass::BUILDER && creature->getClass() !=  CreatureClass::BUILDER) {
            player->print("You are only allowed to teleport to other builder.\n");
            return(0);
        }


        old_room = player->getRoomParent();
        room = creature->getRoomParent();

        player->dmPoof(old_room, room);

        player->deleteFromRoom();
        player->addToSameRoom(creature);
        player->doFollow(old_room);
        return(0);

    } else {
        if(!player->isCt()) {
            player->print("You are not allowed to teleport others to you.\n");
            return(0);
        }

        lowercize(cmnd->str[1], 1);
        target = gServer->findPlayer(cmnd->str[1]);
        if(!target || target == player || !player->canSee(target)) {
            player->print("%s is not on.\n", cmnd->str[1]);
            return(0);
        }
        lowercize(cmnd->str[2], 1);
        if(*cmnd->str[2] == '.')
            target2 = player;
        else
            target2 = gServer->findPlayer(cmnd->str[2]);
        if(!target2) {
            player->print("%s is not on.\n", cmnd->str[1]);
            return(0);
        }
        if(target == target2) {
            player->print("You can't do that.\n");
            return(0);
        }


        old_room = target->getRoomParent();
        room = target2->getRoomParent();

        target->dmPoof(old_room, room);

        target->deleteFromRoom();
        target->addToSameRoom(target2);
        target->doPetFollow();
        return(0);
    }

    old_room = player->getRoomParent();

    player->dmPoof(old_room, room);

    player->deleteFromRoom();
    player->addToRoom(room);
    player->doFollow(old_room);

    return(0);
}


//*********************************************************************
//                      dmUsers
//*********************************************************************
// This function allows staff to list all users online, displaying
// level, name, current room # and name, and address.

int dmUsers(Player* player, cmd* cmnd) {
    long    t = time(nullptr);
    bool    full=false;
    bstring tmp="", host="";
    Player* user=nullptr;
    //Socket* sock=0;

    std::ostringstream oStr;
    char    str[100];
    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr.imbue(std::locale(""));

    bstring cr = gConfig->getDefaultArea();
    if(player->inUniqueRoom())
        cr = player->getUniqueRoomParent()->info.area;

    if(cmnd->num > 1 && cmnd->str[1][0] == 'f')
        full = true;

    oStr << "^bLev  Clas  Player     ";
    if(full)
        oStr << "Address                                                    Idle";
    else
        oStr << "Room                 Address             Last Command      Idle";
    oStr << "\n---------------------------------------------------------------------------------------\n";

    //std::pair<bstring, Player*> p;
    for(Socket &sock : gServer->sockets) {
        //user = p.second;
        //sock = user->getSock();
        user = sock.getPlayer();

        if(!user || !player->canSee(user))
            continue;

        host = sock.getHostname();
        if(user->isDm() && !player->isDm())
            host = "mud.rohonline.net";

        oStr.setf(std::ios::right, std::ios::adjustfield);
        oStr << "^x[" << std::setw(2) << user->getLevel() << "] ";
        oStr.setf(std::ios::left, std::ios::adjustfield);

        if(user->isStaff())
            oStr << "^g";
        oStr << std::setw(4) << bstring(getShortClassName(user)).left(4) << "^w ";

        if(!user->flagIsSet(P_SECURITY_CHECK_OK))
            oStr << "^r";
        else
            oStr << "^y";

        if(user->flagIsSet(P_DM_INVIS))
            oStr << "+";
        else if(user->isEffected("incognito"))
            oStr << "g";
        else if(user->isInvisible())
            oStr << "*";
        else
            oStr << " ";

        if(user->flagIsSet(P_OUTLAW))
            oStr << "^R";
        else if(user->flagIsSet(P_LINKDEAD))
            oStr << "^G";
        else if(user->flagIsSet(P_WATCHER))
            oStr << "^w";
        else if(user->flagIsSet(P_GLOBAL_GAG))
            oStr << "^m";

        if(user->getProxyName().empty())
            oStr << std::setw(10) << user->getName().left(10) << "^w ";
        else
            oStr << std::setw(10) << bstring(user->getName() + "(" + user->getProxyName() + ")").left(10) << "^w ";

        if(!sock.isConnected()) {
            sprintf(str, "connecting (Fd: %d)", sock.getFd());
            oStr << "^Y" << std::setw(20) << str << " ^c" << std::setw(37) << host.left(37);
        } else if(full) {
            oStr << "^m" << std::setw(58) << host.left(58);
        } else {
            if(user->inUniqueRoom()) {
                sprintf(str, "%s: ^b%s", user->getUniqueRoomParent()->info.str(cr, 'b').c_str(), stripColor(user->getUniqueRoomParent()->getCName()).c_str());
                oStr << std::setw(22 + (str[0] == '^' ? 4 : 0)) << bstring(str).left(22 + (str[0] == '^' ? 4 : 0));
            } else if(user->inAreaRoom()){
                //sprintf(str, "%s", user->area_room->mapmarker.str(true).c_str());
                //oStr << std::setw(26) << bstring(str).left(26);
                // TODO: Strip Color
                oStr << std::setw(38) << user->getAreaRoomParent()->mapmarker.str(true).left(38);
            } else {
                oStr << std::setw(38) << "(Unknown)";
            }


            oStr << " ^c" << std::setw(19) << host.left(19)
                 << " ^g";
            if(!user->isDm() || player->isDm())
                oStr << std::setw(17) << dmLastCommand(user).left(17);
            else
                oStr << std::setw(17) << "l";
        }

        sprintf(str, "%02ld:%02ld", (t-sock.ltime)/60L, (t-sock.ltime)%60L);
        oStr << " ^w" << str << "\n";
    }

    player->printColor("%s", oStr.str().c_str());
    player->hasNewMudmail();
    player->print("\n");

    return(0);
}

//*********************************************************************
//                      dmFlushSave
//*********************************************************************
// This function allows staff to save all the rooms in memory back to
// disk in one fell swoop.

int dmFlushSave(Player* player, cmd* cmnd) {
    if(cmnd->num < 2) {
        player->print("All rooms and contents flushed to disk.\n");
        gServer->resaveAllRooms(0);
    } else {
        player->print("All rooms and PERM contents flushed to disk.\n");
        gServer->resaveAllRooms(PERMONLY);
    }
    return(0);
}

//*********************************************************************
//                      dmShutdown
//*********************************************************************
// This function allows staff to shut down the game in a given number of
// minutes.

int dmShutdown(Player* player, cmd* cmnd) {
    player->print("Ok.\n");

    log_immort(true, player, "*** Shutdown by %s.\n", player->getCName());

    Shutdown.ltime = time(nullptr);
    Shutdown.interval = cmnd->val[0] * 60 + 1;

    return(0);
}

//*********************************************************************
//                      dmFlushCrtObj
//*********************************************************************
// This function allows staff to flush the object and creature data so
//  that updated data can be loaded into memory instead.

int dmFlushCrtObj(Player* player, cmd* cmnd) {

    if(!player->canBuildObjects() && !player->canBuildMonsters())
        return(cmdNoAuth(player));

    if(player->canBuildObjects())
        gServer->flushObject();
    if(player->canBuildMonsters())
        gServer->flushMonster();

    player->print("Basic object and monster data flushed from memory.\n");
    return(0);
}



//*********************************************************************
//                      dmResave
//*********************************************************************
// This function allows staff to save a room back to disk.

int dmResave(Player* player, cmd* cmnd) {
    int     s=0;

    if(!player->builderCanEditRoom("use *save"))
        return(0);

    if( cmnd->num > 1 &&
        (!strcmp(cmnd->str[1], "c") || !strcmp(cmnd->str[1], "o"))
    ) {
        bstring str = getFullstrText(cmnd->fullstr, 3);
        if(str.empty()) {
            player->print("Syntax: *save %c [name] [number]\n", cmnd->str[1]);
            return(0);
        }

        CatRef  cr;
        getCatRef(str, &cr, player);

        if(!strcmp(cmnd->str[1], "c"))
            dmSaveMob(player, cmnd, cr);
        else if(!strcmp(cmnd->str[1], "o"))
            dmSaveObj(player, cmnd, cr);
        return(0);
    }


    if(!player->checkBuilder(player->getUniqueRoomParent())) {
        player->print("Error: this room is out of your range; you cannot save this room.\n");
        return(0);
    }


    if(player->inUniqueRoom()) {

        s = gServer->resaveRoom(player->getUniqueRoomParent()->info);
        if(s < 0)
            player->print("Resave failed. Tell this number to Bane: (%d)\n",s);
        else
            player->print("Room saved.\n");

    } else if(player->inAreaRoom()) {

        player->getAreaRoomParent()->save(player);

    } else
        player->print("Nothing to save!\n");

    return(0);
}

//*********************************************************************
//                      dmPerm
//*********************************************************************
// This function allows staff to make a given object sitting on the
// floor into a permanent object.

int dmPerm(Player* player, cmd* cmnd) {
    std::map<int, crlasttime>::iterator it;
    Monster* target=nullptr;
    Object  *object=nullptr;
    int     x=0;

    if(!player->canBuildMonsters() && !player->canBuildObjects())
        return(cmdNoAuth(player));

    if(!player->checkBuilder(player->getUniqueRoomParent())) {
        player->print("Room number not in any of your alotted ranges.\n");
        return(0);
    }
    if(!player->builderCanEditRoom("use *perm"))
        return(0);

    if(!needUniqueRoom(player))
        return(0);

    if(cmnd->num < 2) {
        player->print("Syntax: *perm [o|c|t] [name|d|exit] [timeout|slot]\n");
        return(0);
    }

    switch(low(cmnd->str[1][0])) {
    case 'o':

        if(!player->canBuildObjects()) {
            player->print("Error: you cannot perm objects.\n");
            return(PROMPT);
        }

        if(!strcmp(cmnd->str[2], "d") && strlen(cmnd->str[2])<2) {
            if(cmnd->val[2] > NUM_PERM_SLOTS || cmnd->val[2] < 1) {
                player->print("Slot to delete out of range.\n");
                return(0);
            }

            it = player->getUniqueRoomParent()->permObjects.find(cmnd->val[2]-1);
            if(it != player->getUniqueRoomParent()->permObjects.end()) {
                player->getUniqueRoomParent()->permObjects.erase(it);
                player->print("Perm Object slot #%d cleared.\n", cmnd->val[2]);
            } else {
                player->print("Perm object slot #%d already empty.\n", cmnd->val[2]);
            }

            return(0);
        }


        object = player->getUniqueRoomParent()->findObject(player, cmnd->str[2], 1);

        if(!object) {
            player->print("Object not found.\n");
            return(0);
        }

        if(!object->info.id) {
            player->print("Object is not in database. Not permed.\n");
            return(0);
        }

        if(cmnd->val[2] < 2)
            cmnd->val[2] = 7200;

        x = player->getUniqueRoomParent()->permObjects.size();
        if(x > NUM_PERM_SLOTS) {
            player->print("Room is already full.\n");
            return(0);
        }

        player->getUniqueRoomParent()->permObjects[x].cr = object->info;
        player->getUniqueRoomParent()->permObjects[x].interval = (long)cmnd->val[2];

        log_immort(true, player, "%s permed %s^g in room %s.\n", player->getCName(),
            object->getCName(), player->getUniqueRoomParent()->info.str().c_str());

        player->printColor("%s^x (%s) permed with timeout of %d.\n", object->getCName(), object->info.str().c_str(), cmnd->val[2]);

        return(0);
        // perm Creature
    case 'c':

        if(!player->canBuildMonsters()) {
            player->print("Error: you cannot perm mobs.\n");
            return(PROMPT);
        }

        if(!strcmp(cmnd->str[2], "d") && strlen(cmnd->str[2])<2) {
            if(cmnd->val[2] > NUM_PERM_SLOTS || cmnd->val[2] < 1) {
                player->print("Slot to delete out of range.\n");
                return(0);
            }

            it = player->getUniqueRoomParent()->permMonsters.find(cmnd->val[2]-1);
            if(it != player->getUniqueRoomParent()->permMonsters.end()) {
                player->getUniqueRoomParent()->permMonsters.erase(it);
                player->print("Perm monster slot #%d cleared.\n", cmnd->val[2]);
            } else {
                player->print("Perm monster slot #%d already empty.\n", cmnd->val[2]);
            }

            return(0);
        }

        target = player->getUniqueRoomParent()->findMonster(player, cmnd->str[2], 1);
        if(!target) {
            player->print("Creature not found.\n");
            return(0);
        }



        if(!target->info.id) {
            player->print("Monster is not in database. Not permed.\n");
            return(0);
        }
        if(!player->checkBuilder(target->info)) {
            player->print("Monster number not in any of your alotted ranges.\n");
            return(0);
        }

        if(cmnd->val[2] < 2)
            cmnd->val[2] = 7200;

        x = player->getUniqueRoomParent()->permMonsters.size();
        if(x > NUM_PERM_SLOTS) {
            player->print("Room is already full.\n");
            return(0);
        }

        player->getUniqueRoomParent()->permMonsters[x].cr = target->info;
        player->getUniqueRoomParent()->permMonsters[x].interval = (long)cmnd->val[2];

        log_immort(true, player, "%s permed %s in room %s.\n", player->getCName(),
            target->getCName(), player->getUniqueRoomParent()->info.str().c_str());

        player->print("%s (%s) permed with timeout of %d.\n", target->getCName(), target->info.str().c_str(), cmnd->val[2]);

        return(0);
        // perm tracks
    case 't':
        if(!strcmp(cmnd->str[2], "d") || cmnd->num < 3) {
            player->getUniqueRoomParent()->clearFlag(R_PERMENANT_TRACKS);
            player->print("Perm tracks deleted.\n");
            return(0);
        }
        player->getUniqueRoomParent()->track.setDirection(cmnd->str[2]);
        player->getUniqueRoomParent()->setFlag(R_PERMENANT_TRACKS);
        player->print("Perm tracks added leading %s.\n", player->getUniqueRoomParent()->track.getDirection().c_str());
        return(0);

    default:
        player->print("Syntax: *perm [o|c|t] [name|d|exit] [timeout|slot]\n");
        return(0);
    }
}

//*********************************************************************
//                      dmInvis
//*********************************************************************
// This function allows staff to turn themself invisible.

int dmInvis(Player* player, cmd* cmnd) {

    if(player->flagIsSet(P_DM_INVIS)) {
        if(!player->builderCanEditRoom("turn off invis"))
            return(0);
        player->clearFlag(P_DM_INVIS);
        player->printColor("^mInvisibility off.\n");
    } else {
        player->setFlag(P_DM_INVIS);
        player->printColor("^yInvisibility on.\n");
    }
    return(0);
}


//*********************************************************************
//                      dmIncog
//*********************************************************************

int dmIncog(Player* player, cmd* cmnd) {

    if(player->isEffected("incognito")) {
        if(!player->isCt()) {
            player->print("You cannot unlock your presence.\n");
        } else {
            player->removeEffect("incognito");
        }
    } else {
        player->addEffect("incognito", -1);
    }
    return(0);
}

//*********************************************************************
//                      dmAc
//*********************************************************************
// This function allows staff to take a look at their own special stats.
// or another user's stats.

int dmAc(Player* player, cmd* cmnd) {
    Player  *target=nullptr;

    if(cmnd->num == 2) {
        lowercize(cmnd->str[1], 1);
        target = gServer->findPlayer(cmnd->str[1]);
        if(!target || !player->canSee(target)) {
            player->print("%s is not on.\n", cmnd->str[1]);
            return(0);
        }
    } else {
        player->hp.restore();
        player->mp.restore();
        target = player;
    }

    player->print("WeaponSkill: %d  Defense Skill: %d  Armor: %d(%.0f%%)\n",
            target->getWeaponSkill(player->ready[WIELD-1]), target->getDefenseSkill(),  target->getArmor(), (target->getDamageReduction(target)*100.0));
    return(0);
}

//*********************************************************************
//                      dmWipe
//*********************************************************************

int dmWipe(Player* player, cmd* cmnd) {
//  Room* room=0;
    //otag  *op;
//  int     a=0, fd = player->fd, low=0, high=0;

    // disabled for now
    return(cmdNoAuth(player));
#if 0
    if(cmnd->num < 2) {
        player->print("Usage: *wipe r (low#) r (high#)\n");
        return(0);
    }

    if(cmnd->str[1][0] != 'r' || cmnd->str[2][0] != 'r') {
        player->print("Usage: *wipe r (low#) r (high#)\n");
        return(0);
    }

    low = cmnd->val[1];
    high = cmnd->val[2];

    if(low == high) {
        player->print("Low and high number cannot be the same thing.\n");
        return(0);
    }

    if(low < 1 || high < 1) {
        player->print("Positive numbers....\n");
        return(0);
    }

    if(low > high) {
        player->print("Usage: *wipe r (low#) r (high#)\n");
        return(0);
    }

    for(a=low; a<high+1; a++) {
        if(a == player->currentLocation.room.id) {
            player->printColor("^rCURRENT ROOM SKIPPED.\n");
            continue;
        }

        loadRoom(a, &room);

        if(!room) {
            player->print("ALLOCATION ERROR!\n");
            continue;
        }


        resave_rom(a);
        player->print("Room %d cleared and saved.\n", a);
        //delete room;

    }

    return(0);
#endif
}


/*************************************************************************
*  This function allows the deletion of the database.
*
*/
// TODO: -- REDO THIS to work with xml, easy enough to do.
int dmDeleteDb(Player* player, cmd* cmnd) {
//  int         fd = player->fd, index, blah=0;

    if(cmnd->num < 2) {
        player->print("Syntax: *clear [o|c] [index]\n");
        return(0);
    }
    player->print("Currently disabled\n");
    return(0);

}
//*********************************************************************
//                      dmGameStatus
//*********************************************************************
// Show the status of all configurable options in the game.

int dmGameStatus(Player* player, cmd* cmnd) {
    char **d;
    char buf[2048];

    if(cmnd->num == 2 && !strcmp(cmnd->str[1], "r")) {
        gConfig->loadBeforePython();
        player->print("Config Reloaded.\n");
        return(0);
    } else if(cmnd->num == 2 && !strcmp(cmnd->str[1], "s")) {
        gConfig->save();
        player->print("Config Saved.\n");
        return(0);
    }


    player->printColor("^B\n\nGame Variable Status\n");
    player->printColor("^CGame Port: %d      PID: %d\n",gConfig->getPortNum(), getpid());
#ifdef SQL_LOGGER
     player->printColor("^CSQL Logger Connection: %s  Timeout: %d\n", (gServer->getConnStatus() == true ? "Active" : "Inactive"), gServer->getConnTimeout());
#endif // SQL_LOGGER

    player->printColor("\n^cDMs here are: ");
    strcpy(buf,"");
    d = dmname;
    while(*d) {
        strcat(buf, *d);
        strcat(buf, ", ");
        d++;
    }
    strcpy(buf+strlen(buf)-2, ".\n");
    player->printColor(buf);
    player->printColor("^cDM password: ^x%s\n", gConfig->getDmPass().c_str());
    player->printColor("^cWebserver:   ^x%s\n", gConfig->getWebserver().c_str());
    player->printColor("^cUser Agent:  ^x%s\n", gConfig->getUserAgent().c_str());

    player->printColor("\n");
    player->printColor("^BLottery Information\n");
    player->printColor("^cLottery Enabled:         ^C%3s\n", iToYesNo(gConfig->getLotteryEnabled()));
    player->printColor("^cCurrent lottery cycle:   ^C%d\n", gConfig->getCurrentLotteryCycle());
    player->printColor("^cCurrent ticket price:    ^C%d\n", gConfig->getLotteryTicketPrice());
    player->printColor("^cCurrent lottery jackpot: ^C%ld\n", gConfig->getLotteryJackpot());
    short numbers[6];
    gConfig->getNumbers(numbers);
    player->printColor("^cLast Winning Numbers:    ");
    player->printColor("%02d %02d %02d %02d %02d  (%02d)\n", numbers[0], numbers[1], numbers[2],
            numbers[3], numbers[4], numbers[5]);

    player->printColor("\n");

    player->printColor("^B\nSettings\n");
    player->printColor("^cCheckdouble      = ^C%-3s     ", iToYesNo(gConfig->getCheckDouble()));
    player->printColor("^cNopkillcombat    = ^C%-3s\n", iToYesNo(gConfig->getPkillInCombatDisabled()));
    player->printColor("^cAprilFools       = ^C%-3s     ", iToYesNo(gConfig->willAprilFools()));
    player->printColor("^cFlashPolicyPort  = ^C%d\n", gConfig->getFlashPolicyPort());

    player->printColor("^cShopNumObjects   = ^C%-4d    ", gConfig->getShopNumObjects());
    player->printColor("^cShopNumLines     = ^C%d\n", gConfig->getShopNumLines());

    player->printColor("^cTxtOnCrash       = ^C%-3s     ", iToYesNo(gConfig->sendTxtOnCrash()));
    player->printColor("^cReviewer         = ^C%s\n", gConfig->getReviewer().c_str());
    player->print("\n");

    return(0);
}


//*********************************************************************
//                      dmWeather
//*********************************************************************

int dmWeather(Player* player, cmd* cmnd) {
    BaseRoom* room = player->getRoomParent();
    player->printColor("^BWeather Strings: note that these strings may be specific to this room.\n");
    player->printColor("^cSunrise: ^x%s\n", gConfig->weatherize(WEATHER_SUNRISE, room).c_str());
    player->printColor("^cSunset: ^x%s\n", gConfig->weatherize(WEATHER_SUNSET, room).c_str());
    player->printColor("^cEarth Trembles: ^x%s\n", gConfig->weatherize(WEATHER_EARTH_TREMBLES, room).c_str());
    player->printColor("^cHeavy Fog: ^x%s\n", gConfig->weatherize(WEATHER_HEAVY_FOG, room).c_str());
    player->printColor("^cBeautiful Day: ^x%s\n", gConfig->weatherize(WEATHER_BEAUTIFUL_DAY, room).c_str());
    player->printColor("^cBright Sun: ^x%s\n", gConfig->weatherize(WEATHER_BRIGHT_SUN, room).c_str());
    player->printColor("^cGlaring Sun: ^x%s\n", gConfig->weatherize(WEATHER_GLARING_SUN, room).c_str());
    player->printColor("^cHeat: ^x%s\n", gConfig->weatherize(WEATHER_HEAT, room).c_str());
    player->printColor("^cStill: ^x%s\n", gConfig->weatherize(WEATHER_STILL, room).c_str());
    player->printColor("^cLight Breeze: ^x%s\n", gConfig->weatherize(WEATHER_LIGHT_BREEZE, room).c_str());
    player->printColor("^cStrong Wind: ^x%s\n", gConfig->weatherize(WEATHER_STRONG_WIND, room).c_str());
    player->printColor("^cWind Gusts: ^x%s\n", gConfig->weatherize(WEATHER_WIND_GUSTS, room).c_str());
    player->printColor("^cGale Force: ^x%s\n", gConfig->weatherize(WEATHER_GALE_FORCE, room).c_str());
    player->printColor("^cClear Skies: ^x%s\n", gConfig->weatherize(WEATHER_CLEAR_SKIES, room).c_str());
    player->printColor("^cLight Clouds: ^x%s\n", gConfig->weatherize(WEATHER_LIGHT_CLOUDS, room).c_str());
    player->printColor("^cThunderheads: ^x%s\n", gConfig->weatherize(WEATHER_THUNDERHEADS, room).c_str());
    player->printColor("^cLight Rain : ^x%s\n", gConfig->weatherize(WEATHER_LIGHT_RAIN, room).c_str());
    player->printColor("^cHeavy Rain: ^x%s\n", gConfig->weatherize(WEATHER_HEAVY_RAIN, room).c_str());
    player->printColor("^cSheets Rain: ^x%s\n", gConfig->weatherize(WEATHER_SHEETS_RAIN, room).c_str());
    player->printColor("^cTorrent Rain: ^x%s\n", gConfig->weatherize(WEATHER_TORRENT_RAIN, room).c_str());
    player->printColor("^cNo Moon: ^x%s\n", gConfig->weatherize(WEATHER_NO_MOON, room).c_str());
    player->printColor("^cSliver Moon: ^x%s\n", gConfig->weatherize(WEATHER_SLIVER_MOON, room).c_str());
    player->printColor("^cHalf Moon: ^x%s\n", gConfig->weatherize(WEATHER_HALF_MOON, room).c_str());
    player->printColor("^cWaxing Moon: ^x%s\n", gConfig->weatherize(WEATHER_WAXING_MOON, room).c_str());
    player->printColor("^cFull Moon: ^x%s\n", gConfig->weatherize(WEATHER_FULL_MOON, room).c_str());
    return(0);
}

//*********************************************************************
//                      dmAlchemyList
//*********************************************************************

int dmAlchemyList(Player* player, cmd* cmnd) {
    player->print("Alchemy List:\n");

    player->printColor("^B%25s - %3s - %-15s^x\n", "Name", "Pos", "Action");

    for(auto& it: gConfig->alchemy) {
        player->printColor("%s\n", it.second->getDisplayString().c_str());
    }
    return(0);
}

void printOldQuests(Player* player) {

    player->print("Quest Table:\n\n");
    player->print("| # |             Name             |    Exp    |\n");
    player->print("|---|------------------------------|-----------|\n");
    for(int i=0; i < numQuests; i++) {
        if(gConfig->questTable[i]->num-1 == i) {
            player->print("|%3d|%-30s|%11d|\n",i+1,
                          get_quest_name(i), gConfig->questTable[i]->exp);
        }
    }
    player->print("|---|------------------------------|-----------|\n");

}

//*********************************************************************
//                      dmQuestList
//*********************************************************************

int dmQuestList(Player* player, cmd* cmnd) {
    bool all = false;
    bstring output = getFullstrText(cmnd->fullstr, 1);

    if(output.equals("all")) {
        all = true;
    } else if (output.equals("old")) {
        printOldQuests(player);
        return(0);
    } else if(!output.empty()) {
        output.trimLeft('#');
        int questId = output.toInt();
        *player << "Looking for quest " << questId << ".\n";
        auto qPair = gConfig->quests.find(questId);
        if(qPair == gConfig->quests.end()) {
            *player << "Not found\n";
        } else {
            *player << ColorOn << qPair->first << ") " << qPair->second->getDisplayString() << ColorOff;
        }
        return(0);
    } else {
        *player << ColorOn << "Old Quests: Type ^y*questlist old^x to see old style quests.\nNew Quests: Type ^y*questlist all^x to see all details or ^y*questlist [num]^x to see a specific quest.\n" << ColorOff;
    }

    player->printPaged("New style Quests:");
    for(auto& [questId, quest] : gConfig->quests) {
        player->printPaged(fmt::format("{}) {}\n", questId, (all ? quest->getDisplayString() : quest->getDisplayName())));
    }
    player->donePaging();

    return(0);
}


//*********************************************************************
//                      dmBane
//*********************************************************************

int dmBane(Player* player, cmd* cmnd) {
    if(player->getName() != "Bane" && player->getName() != "Dominus")
        return(PROMPT);

    crash(-69);
    return(0);
}


//*********************************************************************
//                      dmHelp
//*********************************************************************
// This function allows a DM or CT to obtain a list of flags for rooms, exits,
// monsters, players and objects.

int dmHelp(Player* player, cmd* cmnd) {

    if(player->getClass() == CreatureClass::BUILDER) {
        player->print("You must use the builder help file system.\n");
        player->print("Type *bhelp or *bh to see a list of files.\n");
        return(0);
    }

    if(cmnd->num < 2) {
        player->getSock()->viewFile(fmt::format("{}/dmHelpfile.txt", Path::DMHelp), true);
        return(DOPROMPT);
    }
    if(strchr(cmnd->str[1], '/')!=nullptr) {
        player->print("You may not use backslashes.\n");
        return(0);
    }

    player->getSock()->viewFile(fmt::format("{}/{}.txt", Path::DMHelp, cmnd->str[1]), true);
    return(DOPROMPT);

}

//*********************************************************************
//                      bhHelp
//*********************************************************************
// This function allows a builder to obtain a list of flags for rooms, exits,
// monsters, players and objects.

int bhHelp(Player* player, cmd* cmnd) {
    if(cmnd->num < 2) {
        player->getSock()->viewFile(fmt::format("{}/build_help.txt", Path::BuilderHelp), true);
        return(DOPROMPT);
    }
    if(strchr(cmnd->str[1], '/')!=nullptr) {
        player->print("You may not use backslashes.\n");
        return(0);
    }

    player->getSock()->viewFile(fmt::format("{}/{}.txt", Path::BuilderHelp, cmnd->str[1]), true);
    return(DOPROMPT);

}


//*********************************************************************
//                      dmParam
//*********************************************************************

int dmParam(Player* player, cmd* cmnd) {
    extern short    Random_update_interval;
    long            t=0, days=0, hours=0, minutes=0;
    char szBuffer[256];

    if(cmnd->num < 2) {
        player->print("Set what parameter?\n");
        return(0);
    }

    t = time(nullptr);

    switch(low(cmnd->str[1][0])) {
    case 'r':
        Random_update_interval = (short) cmnd->val[1];
        return(PROMPT);
    case 'd':
        player->print("\nRandom Update: %d\n",Random_update_interval);

        days = (t-StartTime) / (60*60*24);
        hours =(t-StartTime) / (60*60);
        hours %= 24;
        minutes = (t-StartTime)/60L;
        minutes %= 60;

        if(!days)
            sprintf(szBuffer, "Uptime: %02ld:%02ld:%02ld\n", hours, minutes, (t-StartTime)%60L);
        else if(days==1)
            sprintf(szBuffer, "Uptime: %ld day %02ld:%02ld:%02ld\n", days, hours, minutes, (t-StartTime)%60L);
        else
            sprintf(szBuffer, "Uptime: %ld days %02ld:%02ld:%02ld\n", days, hours, minutes, (t-StartTime)%60L);

        player->print(szBuffer);

        return(PROMPT);
    default:
        player->print("Invalid parameter.\n");
        return(0);
    }

}




//*********************************************************************
//                      dmOutlaw
//*********************************************************************
// dmOutlaw allows staff to outlaw people for a time
// up to 2 hrs of online time max. - TC

int dmOutlaw(Player* player, cmd* cmnd) {
    Creature* target=nullptr;
    int     minutes=0;
    long    t=0, i=0;


    if(!player->flagIsSet(P_CAN_OUTLAW) && !player->isDm())
        return(cmdNoAuth(player));

    if(cmnd->num < 2) {
        player->print("syntax: *outlaw <player> <minutes> [-a|-s|-x]\n");
        player->print("        *outlaw <player> [-f|-r]\n");
        return(0);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target || !player->canSee(target)) {
        player->print("That player is not logged on.\n");
        return(0);
    }

    if(target->isStaff()) {
        player->print("You can't outlaw staff.\n");
        return(0);
    }


    if(!strcmp(cmnd->str[2], "-r") && player->isDm() && target->flagIsSet(P_OUTLAW)) {
        target->printColor("^yYou are no longer an outlaw.\n");
        player->print("%s's outlaw flag is removed.\n", target->getCName());
        logn("log.bane", "*** %s's outlaw flag was removed by %s.\n", target->getCName(), player->getCName());
        target->clearFlag(P_OUTLAW);
        target->clearFlag(P_OUTLAW_WILL_BE_ATTACKED);
        target->clearFlag(P_OUTLAW_WILL_LOSE_XP);
        target->setFlag(P_NO_SUMMON);
        target->clearFlag(P_NO_GET_ALL);
        player->lasttime[LT_OUTLAW].interval = 0;
        return(0);
    }

    i = LT(target, LT_OUTLAW);
    t = time(nullptr);

    if(!strcmp(cmnd->str[2], "-f") && !target->flagIsSet(P_OUTLAW)) {
        player->print("%s is not currently an outlaw.\n", target->getCName());
        return(0);
    }

    if(!strcmp(cmnd->str[2], "-f") && target->flagIsSet(P_OUTLAW)) {
        player->print("%s's current outlaw flags:\n", target->getCName());
        if(target->flagIsSet(P_OUTLAW_WILL_BE_ATTACKED))
            player->print("--Will be attacked by outlaw aggressive mobs.\n");
        if(!target->flagIsSet(P_NO_SUMMON))
            player->print("--Cannot set no summon.\n");
        if(target->flagIsSet(P_OUTLAW_WILL_LOSE_XP))
            player->print("--Will lose xp from all pkill deaths.\n");

        player->print("Outlaw time remaining: ");
        if(i - t > 3600)
            player->print("%02d:%02d:%02d more hours.\n",(i - t) / 3600L, ((i - t) % 3600L) / 60L, (i - t) % 60L);
        else if((i - t > 60) && (i - t < 3600))
            player->print("%d:%02d more minutes.\n", (i - t) / 60L, (i - t) % 60L);
        else
            player->print("%d more seconds.\n", i - t);

        return(0);
    }


    if(target->flagIsSet(P_OUTLAW)) {
        if(i > t) {
            player->print("%s is already outlawed. A DM must use the -r switch to cancel it.\n", target->getCName());
            if(i - t > 3600)
                player->print("%s is an outlaw for %02d:%02d:%02d more hours.\n", target->getCName(),(i - t) / 3600L, ((i - t) % 3600L) / 60L, (i - t) % 60L);
            else if((i - t > 60) && (i - t < 3600))
                player->print("%s is an outlaw %d:%02d more minutes.\n", target->getCName(), (i - t) / 60L, (i - t) % 60L);
            else
                player->print("%s is an outlaw for %d more seconds.\n", target->getCName(), i - t);
            return(0);
        }
    }



    minutes = cmnd->val[1];
    if(minutes > 120) {
        player->print("Time specified must be from 10 to 120 minutes(2 hrs).\n");
        return(0);
    }

    minutes = MAX(minutes, 10);

    target->setFlag(P_OUTLAW);
    target->lasttime[LT_OUTLAW].ltime = time(nullptr);
    target->lasttime[LT_OUTLAW].interval = (60L * minutes);

    player->print("%s is now an outlaw for %d minutes.\n", target->getCName(), minutes);

    logn("log.outlaw", "*** %s was made an outlaw by %s.\n", target->getCName(), player->getCName());


    if(target->isEffected("mist"))
        target->removeEffect("mist");

    broadcast("### %s has just been made an outlaw for being a jackass.", target->getCName());
    broadcast("### %s can now be killed by anyone at any time.", target->upHeShe());

    if(!strcmp(cmnd->str[2], "-a") || !strcmp(cmnd->str[3], "-a") || !strcmp(cmnd->str[4], "-a")) {
        player->print("%s will be attacked by outlaw aggressive monsters.\n", target->getCName());
        logn("log.outlaw", "*** %s was set to be killed by outlaw aggro mobs by %s.\n", target->getCName(), player->getCName());
        target->setFlag(P_OUTLAW_WILL_BE_ATTACKED);
    }


    if(!strcmp(cmnd->str[2], "-s") || !strcmp(cmnd->str[3], "-s") || !strcmp(cmnd->str[4], "-s")) {
        player->print("%s can be summoned to %s death.\n", target->getCName(), target->hisHer());
        broadcast("### %s can now be summoned to %s death.\n", target->getCName(), target->hisHer());
        logn("log.outlaw", "*** %s was set to be summonable to death by %s.\n", target->getCName(), player->getCName());
        target->clearFlag(P_NO_SUMMON);
    }


    if(!strcmp(cmnd->str[2], "-x") || !strcmp(cmnd->str[3], "-x") || !strcmp(cmnd->str[4], "-x")) {
        player->print(".\n", target->getCName(), target->hisHer());
        logn("log.outlaw", "*** %s was set to lose xp from pk loss by %s.\n", target->getCName(), player->getCName());
        broadcast("### %s will now give you experience when you kill %s.", target->getCName(), target->himHer());
        player->print("%s will now lose xp from pkill losses.\n", target->getCName());
        target->clearFlag(P_OUTLAW_WILL_LOSE_XP);
    }

    target->setFlag(P_NO_GET_ALL);

    return(0);
}

//*********************************************************************
//                      dmBroadecho
//*********************************************************************
// dmBroadecho allows staff to broadcast a message to
// the players in the game free of any message format. i.e. the msg
// broadcasted appears exactly as it is typed

int dmBroadecho(Player* player, cmd* cmnd) {
    int     len=0, i=0, found=0;

    if(!player->flagIsSet(P_CT_CAN_DM_BROAD) && !player->isDm())
        return(cmdNoAuth(player));

    len = cmnd->fullstr.length();
    for(i=0; i<len && i < 256; i++) {
        if(cmnd->fullstr[i] == ' ' && cmnd->fullstr[i+1] != ' ')
            found++;
        if(found==1)
            break;
    }

    len = strlen(&cmnd->fullstr[i+1]);
    if(found < 1 || len < 1) {
        player->print("echo what?\n");
        return(0);
    }
    if(cmnd->fullstr[i+1] == '-')
        switch(cmnd->fullstr[i+2]) {
        case 'n':
            if(cmnd->fullstr[i+3] != 0 && cmnd->fullstr[i+4] != 0) {
                broadcast("%s", &cmnd->fullstr[i+4]);
            }

            break;
        }
    else
        broadcast("### %s", &cmnd->fullstr[i+1]);
    return(0);

}




//*********************************************************************
//                      dmGlobalSpells
//*********************************************************************

bool dmGlobalSpells(Player* player, int splno, bool check) {

    // in case dynamic cast fails
    if(!player)
        return(false);

    long    t = time(nullptr);

    switch(splno) {
    case S_VIGOR:
        if(check) return(true);
        if(player->getClass() !=  CreatureClass::LICH)
            player->hp.increase(Random::get(1,6) + 4 + 2);
        break;
    case S_MEND_WOUNDS:
        if(check) return(true);
        if(player->getClass() !=  CreatureClass::LICH)
            player->hp.increase(Random::get(2,10) + 4 + 4);
        break;
    case S_RESTORE:
        if(check) return(true);
        player->hp.restore();
        player->mp.restore();
        break;
    case S_HEAL:
        if(check) return(true);
        if(player->getClass() !=  CreatureClass::LICH)
            player->hp.restore();
        break;
    case S_BLESS:
        if(check) return(true);
        player->addEffect("bless", 3600, 1);
        break;
    case S_PROTECTION:
        if(check) return(true);
        player->addEffect("protection", 3600, 1);
        break;
    case S_INVISIBILITY:
        if(check) return(true);
        player->addEffect("invisibility", 3600, 1);
        break;
    case S_DETECT_MAGIC:
        if(check) return(true);
        player->addEffect("detect-magic", 3600, 1);
        break;
    case S_RESIST_EARTH:
        if(check) return(true);
        player->addEffect("resist-earth", 3600, 1);
        break;
    case S_RESIST_AIR:
        if(check) return(true);
        player->addEffect("resist-air", 3600, 1);
        break;
    case S_RESIST_FIRE:
        if(check) return(true);
        player->addEffect("resist-fire", 3600, 1);
        break;
    case S_RESIST_WATER:
        if(check) return(true);
        player->addEffect("resist-water", 3600, 1);
        break;
    case S_RESIST_MAGIC:
        if(check) return(true);
        player->addEffect("resist-magic", 3600, 1);
        break;
    case S_DETECT_INVISIBILITY:
        if(check) return(true);
        player->addEffect("detect-invisible", 3600, 1);
        break;
    case S_FLY:
        if(check) return(true);
        player->addEffect("fly", 3600, MAXALVL/3);
        break;
    case S_INFRAVISION:
        if(check) return(true);
        player->addEffect("infravision", 3600, 1);
        break;
    case S_LEVITATE:
        if(check) return(true);
        player->addEffect("levitate", 3600, 1);
        break;
    case S_KNOW_AURA:
        if(check) return(true);
        player->addEffect("know-aura", 3600, 1);
        break;
    case S_STONE_SHIELD:
        if(check) return(true);
        player->addEffect("earth-shield", 3600, 1);
        break;
    case S_RESIST_COLD:
        if(check) return(true);
        player->addEffect("resist-cold", 3600, 1);
        break;
    case S_HEAT_PROTECTION:
        if(check) return(true);
        player->addEffect("heat-protection", 3600, 1);
        break;
    case S_BREATHE_WATER:
        if(check) return(true);
        player->addEffect("breathe-water", 3600, 1);
        break;
    case S_BOUNCE_MAGIC:
        if(check) return(true);
        player->addEffect("reflect-magic", 300, 33);
        break;
    case S_REBOUND_MAGIC:
        if(check) return(true);
        player->addEffect("reflect-magic", 300, 66);
        break;
    case S_REFLECT_MAGIC:
        if(check) return(true);
        player->addEffect("reflect-magic", 300, 100);
        break;
    case S_ANNUL_MAGIC:
        if(check) return(true);
        player->doDispelMagic();
        break;
    case S_RADIATION:
    case S_FIERY_RETRIBUTION:
    case S_AURA_OF_FLAME:
    case S_BARRIER_OF_COMBUSTION:
        if(check) return(true);
        player->addEffect("fire-shield", 900, 1);
        break;
    case S_BLUR:
        if(check) return(true);
        player->addEffect("blur", 1800, 1);
        break;
    case S_TRUE_SIGHT:
        if(check) return(true);
        player->addEffect("true-sight", 1800, 1);
        break;
    case S_DRAIN_SHIELD:
        if(check) return(true);
        player->addEffect("drain-shield", 3600, 1);
        break;
    case S_CAMOUFLAGE:
        if(check) return(true);
        player->addEffect("camouflage", 2000, 1);
        break;
    case S_UNDEAD_WARD:
        if(check) return(true);
        player->addEffect("undead-ward", 180, 1);
        break;
    case S_RESIST_ELEC:
        if(check) return(true);
        player->addEffect("resist-electric", 3600, 1);
        break;
    case S_WARMTH:
        if(check) return(true);
        player->addEffect("warmth", 3600, 1);
        break;
    case S_WIND_PROTECTION:
        if(check) return(true);
        player->addEffect("wind-protection", 3600, 1);
        break;
    case S_STATIC_FIELD:
        if(check) return(true);
        player->addEffect("static-field", 3600, 1);
        break;
    case S_FREE_ACTION:
        if(check) return(true);
        player->lasttime[LT_FREE_ACTION].interval = 3600;
        player->lasttime[LT_FREE_ACTION].ltime = t;
        player->setFlag(P_FREE_ACTION);
        break;
    case S_COURAGE:
        if(check) return(true);
        player->addEffect("courage", 3600, 1);
        break;
    case S_COMPREHEND_LANGUAGES:
        if(check) return(true);
        player->addEffect("comprehend-languages", 300, 1);
        break;
    case S_HASTE:
        if(check) return(true);
        if(player->isEffected("haste") || player->isEffected("frenzy"))
            break;
        player->addEffect("haste", 180, 30);
        break;
    case S_SLOW:
        if(check) return(true);
        if(player->isEffected("frenzy")) {
            player->removeEffect("frenzy");
            break;
        }
        if(player->isEffected("slow"))
            break;
        player->addEffect("slow", 60, 30);
        break;
    case S_STRENGTH:
        if(player->isEffected("strength") || player->isEffected("berserk") || (player->isEffected("dkpray")))
            break;
        player->addEffect("strength", 180, 30);
        break;
    case S_ENFEEBLEMENT:
        if(check) return(true);
        if(player->isEffected("berserk")) {
            player->removeEffect("berserk");
            break;
        }
        if(player->isEffected("dkpray")) {
            player->removeEffect("dkpray");
            break;
        }
        if(player->isEffected("enfeeblement"))
            break;
        player->addEffect("enfeeblement", 180, 30);
        break;
    case S_INSIGHT:
        if(check) return(true);
        if(player->isEffected("confusion")) {
            player->removeEffect("confusion");
            break;
        }
        if(player->isEffected("insight"))
            break;
        player->addEffect("insight", 60, 30);
        break;
    case S_FEEBLEMIND:
        if(check) return(true);
        if(player->isEffected("feeblemind"))
            break;
        player->addEffect("feeblemind", 60, 30);
        break;
    case S_PRAYER:
        if(check) return(true);
        if(player->isEffected("prayer") || player->isEffected("pray"))
            break;
        player->addEffect("prayer", 180, 30);
        break;
    case S_DAMNATION:
        if(check) return(true);
        if(player->isEffected("pray")) {
            player->removeEffect("pray");
            break;
        }
        if(player->isEffected("damnation"))
            break;
        player->addEffect("damnation", 60, 30);
        break;
    case S_FORTITUDE:
        if(check) return(true);
        if(player->isUndead())
            break;
        if(player->isEffected("fortitude") || player->isEffected("berserk"))
            break;
        player->addEffect("fortitude", 180, 30);
        break;
    case S_WEAKNESS:
        if(check) return(true);
        if(player->isUndead())
            break;
        if(player->isEffected("berserk")) {
            player->removeEffect("berserk");
            break;
        }
        if(player->isEffected("weakness"))
            break;
        player->addEffect("weakness", 60, 30);
        break;

    case S_CURE_POISON:
        if(check) return(true);
        player->curePoison();
        break;
    case S_CURE_DISEASE:
        if(check) return(true);
        player->cureDisease();
        break;
    case S_CURE_BLINDNESS:
        if(check) return(true);
        player->removeEffect("blindness");
        break;
    case S_FARSIGHT:
        if(check) return(true);
        player->addEffect("farsight", 180, 1);
        break;
    case S_REGENERATION:
        if(check) return(true);
        player->addEffect("regeneration", 3000, 40);
        break;
    case S_WELLOFMAGIC:
        if(check) return(true);
        player->addEffect("well-of-magic", 3000, 40);
        break;
    default:
        return(false);
        break;
    }
    return(true);
}


//*********************************************************************
//                      dmCast
//*********************************************************************

int dmCast(Player* player, cmd* cmnd) {
    Player  *target=nullptr;
    char    rcast=0, *sp;
    int     splno=0, c=0, fd = player->fd, i=0, silent=0;

    if(cmnd->num < 2) {
        player->print("Globally cast what?\n");
        return(PROMPT);
    }

    if(cmnd->num >2 )   {
        if(!strcmp(cmnd->str[1],"-r"))
            rcast = 1;
        else if(!strcmp(cmnd->str[1],"-s"))
            silent = 1;
        else {
            player->print("Invalid cast flag.\n");
            return(PROMPT);
        }
        sp = cmnd->str[2];
    } else 
        sp = cmnd->str[1];

    do {
        if(!strcmp(sp, get_spell_name(c))) {
            i = 1;
            splno = c;
            break;
        } else if(!strncmp(sp, get_spell_name(c), strlen(sp))) {
            i++;
            splno = c;
        }
        c++;
    } while(get_spell_num(c) != -1);

    if(i == 0) {
        player->print("That spell does not exist.\n");
        return(0);
    } else if(i > 1) {
        player->print("Spell name is not unique.\n");
        return(0);
    }


    if(rcast) {


        if(splno == S_WORD_OF_RECALL) {
            BaseRoom *room = player->getRecallRoom().loadRoom(player);

            if(!room) {
                player->print("Spell failure.\n");
                return(0);
            }
            player->print("You cast %s on everyone in the room.\n", get_spell_name(splno));
            broadcast(player->getSock(), player->getParent(),
                "%M casts %s on everyone in the room.\n", player, get_spell_name(splno));

            log_immort(false, player, "%s casts %s on everyone in room %s.\n", player->getCName(), get_spell_name(splno),
                player->getRoomParent()->fullName().c_str());

            auto pIt = player->getRoomParent()->players.begin();
            while(pIt != player->getRoomParent()->players.end()) {
                target = (*pIt++);
                target->print("%M casts %s on you.\n", player, get_spell_name(splno));

                target->deleteFromRoom();
                target->addToRoom(room);
            }
            return(0);
        }

        if((!dmGlobalSpells(player, splno, true))) {
            player->print("Sorry, you cannot room cast that spell.\n");
            return(0);
        }

        player->print("You cast %s on everyone in the room.\n", get_spell_name(splno));

        for(Player* ply : player->getRoomParent()->players) {
            if(ply->flagIsSet(P_DM_INVIS))
                continue;

            ply->print("%M casts %s on you.\n", player, get_spell_name(splno));
            dmGlobalSpells(ply, splno, false);
        }

        broadcast(player->getSock(), player->getParent(), "%M casts %s on everyone in the room.\n",
            player, get_spell_name(splno));

        log_immort(false, player, "%s casts %s on everyone in room %s.\n", player->getCName(), get_spell_name(splno),
            player->getRoomParent()->fullName().c_str());

    } else {
        if(!dmGlobalSpells(player, splno, true)) {
            player->print("Sorry, you cannot globally cast that spell.\n");
            return(0);
        }


        if(!silent) {
            player->print("You cast %s on everyone.\n", get_spell_name(splno));
        } else {
            player->print("You silently cast %s on everyone.\n", get_spell_name(splno));
        }

        Player* ply;
        for(const auto& p : gServer->players) {
            ply = p.second;

            if(!ply->isConnected())
                continue;
            if(ply->fd == fd)
                continue;
            if(ply->flagIsSet(P_DM_INVIS))
                continue;
            if(!silent)
                ply->print("%M casts %s on you.\n", player, get_spell_name(splno));
            dmGlobalSpells(ply, splno, false);
        }

        if(!silent) {
            broadcast("%M casts %s on everyone.",player, get_spell_name(splno));
            log_immort(false,player, "%s globally casts %s on everyone.\n", player->getCName(), get_spell_name(splno));
        } else {
            broadcast(isCt, "^y%M silently casts %s on everyone.",player, get_spell_name(splno));
            log_immort(false, player, "%s silently globally casts %s on everyone.\n", player->getCName(), get_spell_name(splno));
        }

    }

    return(0);
}


//*********************************************************************
//                      dmSet
//*********************************************************************
// This function allows staff to set a variable within a currently
// existing data structure in the game.

int dmSet(Player* player, cmd* cmnd) {
    if(cmnd->num < 2) {
        player->print("Set what?\n");
        return(0);
    }

    if(!player->builderCanEditRoom("use *set"))
        return(0);

    switch(low(cmnd->str[1][0])) {
    case 'x':
        return(dmSetExit(player, cmnd));
    case 'r':
        if(low(cmnd->str[1][1]) == 'e')
            return(dmSetRecipe(player, cmnd));
        return(dmSetRoom(player, cmnd));
    case 'c':
    case 'p':
    case 'm':
        return(dmSetCrt(player, cmnd));
    case 'i':
    case 'o':
        return(dmSetObj(player, cmnd));
    default:
        player->print("Invalid option.  *set <x|r|c|o> <options>\n");
        return(0);
    }

}

//*********************************************************************
//                      dmLog
//*********************************************************************
// This function allows staff to peruse the log file while in the game.
// If *log r is typed, then the log file is removed (i.e. cleared).

int dmLog(Player* player, cmd* cmnd) {
    char filename[80];

    switch(tolower(cmnd->str[1][0])) {
    case 'a':
        if(!player->isDm())
            return(PROMPT);
        sprintf(filename, "%s/assert.log.txt", Path::Log);
        break;
    case 'i':
        if(!player->isDm())
            return(PROMPT);
        sprintf(filename, "%s/log.imm.txt", Path::Log);
        break;
    case 's':
        sprintf(filename, "%s/log.suicide.txt", Path::Log);
        break;
    case 'w':
        if(!player->isDm())
            return(PROMPT);
        sprintf(filename, "%s/log.passwd.txt", Path::Log);
        break;
    case 'p':
    default:
        sprintf(filename, "%s/log.txt", Path::Log);
        break;
    }


    if( cmnd->num >= 3 ) {
        cmnd->fullstr = getFullstrText(cmnd->fullstr, 2);
        strcpy(player->getSock()->tempstr[3], cmnd->fullstr.c_str());
    } else
        strcpy(player->getSock()->tempstr[3], "\0");

    player->getSock()->viewFileReverse(filename);


    return(DOPROMPT);
}

//*********************************************************************
//                      dmList
//*********************************************************************
// This function allows staff to fork a "list" process. List is the
// utility program that allows one to nicely output a list of monsters,
// objects or rooms in the game. There are many flags provided with
// the command, and they can be entered here in the same way that they
// are entered on the command line.

int dmList(Player* player, cmd* cmnd) {
    if(!player->flagIsSet(P_CT_CAN_DM_LIST) && !player->isDm())
        return(cmdNoAuth(player));

    if(cmnd->num < 2) {
        player->print("List what?\n");
        return(0);
    }

    bstring args = cmnd->fullstr;
    args.trimLeft(" *list");

    gServer->runList(player->getSock(), cmnd);
    return(0);
}

int dmIds(Player* player, cmd* cmnd) {
    player->print("%s", gServer->getRegisteredList().c_str());
    return(0);
}

//*********************************************************************
//                      dmInfo
//*********************************************************************

int dmInfo(Player* player, cmd* cmnd) {
    long            t=0, days=0, hours=0, minutes=0;
    extern short    Random_update_interval;

    player->print("Last compiled " __TIME__ " " __DATE__ ".\n");

    t = time(nullptr);
    days = (t - StartTime) / 86400L;
    hours = (t - StartTime) / 3600L;
    hours %= 24;
    minutes = (t - StartTime) / 60L;
    minutes %= 60;
    if(!days)
        player->print("Uptime: %02ld:%02ld:%02ld\n", hours, minutes, (t - StartTime) % 60L);
    else if(days == 1)
        player->print("Uptime: %ld day %02ld:%02ld:%02ld\n", days, hours, minutes, (t - StartTime) % 60L);
    else
        player->print("Uptime: %ld days %02ld:%02ld:%02ld\n", days, hours, minutes, (t - StartTime) % 60L);
    player->print("\n    Bytes in:  %9ld\n    Bytes out: %9ld(%ld)[%f]\n", InBytes, OutBytes, UnCompressedBytes, (OutBytes*1.0)/(UnCompressedBytes*1.0));
    player->print("\nInternal Cache Queue Sizes:\n");
    player->print("   Rooms: %-5d   Monsters: %-5d   Objects: %-5d\n\n",
            gServer->roomCache.size(), gServer->monsterCache.size(), gServer->objectCache.size());
    player->print("Wander update: %d\n", Random_update_interval);
    if(player->isDm())
        player->print("      Players: %d\n\n", Socket::getNumSockets());

    player->print("Max Ids: P: %ld M: %ld O: %ld\n", gServer->getMaxPlayerId(), gServer->getMaxMonsterId(), gServer->getMaxObjectId());


    return(0);
}

int dmMd5(Player* player, cmd* cmnd) {
    bstring tohash = getFullstrText(cmnd->fullstr, 1, ' ');
    player->print("MD5: '%s' = '%s'\n", tohash.c_str(), md5(tohash).c_str());
    return(0);
}
//*********************************************************************
//                      dmStatDetail
//*********************************************************************
//  This function will allow staff to display detailed stat information

int dmStatDetail(Player* player, cmd* cmnd) {
    Creature* target = nullptr;

    if(cmnd->num < 2)
        target = player;
    else {
        target = player->getParent()->findCreature(player, cmnd, 1);
        cmnd->str[1][0] = up(cmnd->str[1][0]);
        if(!target)
            target = gServer->findPlayer(cmnd->str[1]);
        if(!target) {
            player->print("Unable to locate.\n");
            return(0);
        }
    }
    *player << ColorOn << "Detailed stat information for " << target << ":\n";
    *player << target->hp << "\n";
    *player << target->mp << "\n";
    if(target->isPlayer())
        *player << target->getAsPlayer()->focus << "\n";

    *player << target->strength << "\n";
    *player << target->dexterity << "\n";
    *player << target->constitution << "\n";
    *player << target->intelligence << "\n";
    *player << target->piety << "\n";
    *player << ColorOff;

    return(0);
}

//*********************************************************************
//                      dmStat
//*********************************************************************
//  This function will allow staff to display information on an object
//  creature, player, or room.

int dmStat(Player* player, cmd* cmnd) {
    Object  *object=nullptr;
    Creature* target=nullptr;
    Monster* mTarget=nullptr;
    Creature* player2=nullptr;
    int i=0, j=0;
    CatRef  cr;

    if(!player->checkBuilder(player->getUniqueRoomParent())) {
        player->print("Current room number not in any of your alotted ranges.\n");
        return(0);
    }
    if(!player->builderCanEditRoom("use *status"))
        return(0);
    // Give stats on room staff is in or specified room #

    // *st 1 and *st 1.-10.7 will trigger num < 2,
    // *st misc.100 will not, so we need to make an exception
    bstring str = getFullstrText(cmnd->fullstr, 1);
    bstring txt = getFullstrText(str, 1, '.');

    if(cmnd->num < 2 || (!txt.empty() && txt.getAt(0))) {
        Area        *area=nullptr;
        MapMarker   mapmarker;
        AreaRoom*   aRoom=nullptr;
        UniqueRoom      *uRoom=nullptr;

        // if they're not *st-ing anything in particular
        if(str.empty()) {
            if(player->inUniqueRoom()) {
                uRoom = player->getUniqueRoomParent();
                cr = player->getUniqueRoomParent()->info;
            } else if(player->inAreaRoom()) {
                aRoom = player->getAreaRoomParent();
//              mapmarker = aRoom->mapmarker;
            }
        } else {

            getDestination(str, &mapmarker, &cr, player);

        }

        if(player->getClass() !=  CreatureClass::BUILDER && (mapmarker.getArea() || aRoom)) {

            if(!aRoom) {
                area = gServer->getArea(mapmarker.getArea());
                if(!area) {
                    player->print("Area does not exist.\n");
                    return(0);
                }

                aRoom = area->loadRoom(nullptr, &mapmarker, false);
            }

            stat_rom(player, aRoom);

            if(aRoom->canDelete())
                aRoom->area->remove(aRoom);

        } else if(cr.id) {

            if(!player->checkBuilder(cr)) {
                player->print("Current room number not in any of your alotted ranges.\n");
                return(0);
            }
            if(cr.id < 0 || cr.id >= RMAX) {
                player->print("Error: out of room range.\n");
                return(0);
            }
            if(player->inUniqueRoom() && cr == player->getUniqueRoomParent()->info)
                uRoom = player->getUniqueRoomParent();
            else {
                if(!loadRoom(cr, &uRoom)) {
                    player->print("Error (%s)\n", cr.str().c_str());
                    return(0);
                }
            }

            stat_rom(player, uRoom);

        } else
            player->print("*stat what room?\n");
        return(0);
    }

    //  Use player reference through 2nd parameter or default to staff
    if(cmnd->num < 3)
        player2 = player;
    else {
        player2 = player->getParent()->findCreature(player, cmnd, 2);
        cmnd->str[2][0] = up(cmnd->str[2][0]);
        if(!player2)
            player2 = gServer->findPlayer(cmnd->str[2]);
        if(!player2 || !player->canSee(player2)) {
            player->print("Unable to locate.\n");
            return(0);
        }

        if(player->getClass() == CreatureClass::BUILDER) {
            if(!player->canBuildMonsters()) {
                player->print("Error: you do not have authorization to modify monsters.\n");
                return(PROMPT);
            }
            mTarget = player->getAsMonster();
            if(!mTarget) {
                player->print("Error: you are not allowed to modify players.\n");
                return(0);
            } else if(mTarget->info.id && !player->checkBuilder(mTarget->info)) {
                player->print("Creature number is not in any of your alotted ranges.\n");
                return(0);
            }
        }
    }

    // Give info on object, if found
    object = player2->findObject(player2, cmnd, 1);
    if(!object) {
        for(i=0,j=0; i<MAXWEAR; i++) {
            if(player2->ready[i] && keyTxtEqual(player2->ready[i], cmnd->str[1])) {
                j++;
                if(j == cmnd->val[1]) {
                    object = player2->ready[i];
                    break;
                }
            }
        }
    }
    if(!object)
        object = player->getRoomParent()->findObject(player2, cmnd, 1);

    if(object) {
        stat_obj(player, object);
        return(0);
    }

    // Search for creature or player to get info on
    target = player->getParent()->findCreature(player, cmnd);

    cmnd->str[1][0] = up(cmnd->str[1][0]);
    if(!target)
        target = gServer->findPlayer(cmnd->str[1]);

    if(target && player->canSee(target)) {
        Player  *pTarget = target->getAsPlayer();
        mTarget = target->getAsMonster();

        if(player->getClass() == CreatureClass::BUILDER) {
            if(!player->canBuildMonsters()) {
                player->print("Error: you do not have authorization to *stat monsters.\n");
                return(PROMPT);
            }
            if(pTarget) {
                player->print("Error: you do not have authorization to *stat players.\n");
                return(0);
            } else if(mTarget->info.id && !player->checkBuilder(mTarget->info)) {
                player->print("Creature number is not in any of your alotted ranges.\n");
                return(0);
            }
        }
        if(mTarget && !player->isDm())
            log_immort(false, player, "%s statted %s in room %s.\n", player->getCName(), mTarget->getCName(),
                player->getRoomParent()->fullName().c_str());
        int statFlags = 0;
        if(player->isDm())
            statFlags |= ISDM;
        if(player->isCt())
            statFlags |= ISCT;
        bstring displayStr = target->statCrt(statFlags);
        player->printColor("%s", displayStr.c_str());
    } else
        player->print("Unable to locate.\n");

    return(0);
}

//*********************************************************************
//                      dmCache
//*********************************************************************

int dmCache(Player* player, cmd* cmnd) {
    bstring cacheStr = gServer->getDnsCacheString();
    player->printColor("%s", cacheStr.c_str());
    return(0);
}

//*********************************************************************
//                      dmTxtOnCrash
//*********************************************************************

int dmTxtOnCrash(Player* player, cmd* cmnd) {
    gConfig->toggleTxtOnCrash();
    player->printColor("Setting toggled to %s.\n", gConfig->sendTxtOnCrash() ? "^GYes" : "^RNo");
    player->print("Note that, on reboot, text-on-crash is reset to No!\n");
    return(0);
}
