/*
 * command5.cpp
 *   Command handling/parsing routines.
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

#include <fmt/format.h>                // for format
#include <strings.h>                   // for strcasecmp
#include <unistd.h>                    // for unlink
#include <cstdio>                      // for sprintf, BUFSIZ
#include <cstdlib>                     // for atoi
#include <cstring>                     // for strcpy
#include <ctime>                       // for time
#include <list>                        // for operator==, list, _List_iterator
#include <map>                         // for operator==, _Rb_tree_iterator
#include <ostream>                     // for operator<<, ostringstream, bas...
#include <string>                      // for string, allocator, operator==
#include <utility>                     // for pair
#include <numeric>                     // for accumulate

#include "calendar.hpp"                // for Calendar
#include "catRef.hpp"                  // for CatRef
#include "catRefInfo.hpp"              // for CatRefInfo
#include "cmd.hpp"                     // for cmd
#include "commands.hpp"                // for changingStats, cmdChangeStats
#include "config.hpp"                  // for Config, gConfig
#include "enums/loadType.hpp"          // for LoadType, LoadType::LS_BACKUP
#include "flags.hpp"                   // for P_AFK, P_CAN_CHANGE_STATS, P_C...
#include "global.hpp"                  // for CreatureClass, CreatureClass::...
#include "guilds.hpp"                  // for GuildCreation
#include "lasttime.hpp"                // for lasttime
#include "login.hpp"                   // for CON_PLAYING, CON_CHANGING_STAT...
#include "mud.hpp"                     // for LT, LT_HYPNOTIZE, LT_SMOTHER
#include "mudObjects/creatures.hpp"    // for PetList
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "playerClass.hpp"             // for PlayerClass
#include "raceData.hpp"                // for RaceData
#include "paths.hpp"                   // for Bank, History, Player, Post
#include "proto.hpp"                   // for broadcast, low, isCt, lowercize
#include "server.hpp"                  // for Server, gServer, PlayerMap
#include "socket.hpp"                  // for Socket
#include "stats.hpp"                   // for Stat
#include "structs.hpp"                 // for StatsContainer
#include "web.hpp"                     // for updateRecentActivity, webUnass...

//*********************************************************************
//                      who
//*********************************************************************
// This function outputs a list of all the players who are currently
// logged into the game.

int cmdWho(const std::shared_ptr<Player>& player, cmd* cmnd) {
    CreatureClass cClass = CreatureClass::NONE;

    int     special=0;
    int     chaos=0, clan=0, pledge=0;
    int     race=0, cls=0, law=0, guild=0;
    bool    found=false;

    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    if(player->isBlind()) {
        player->printColor("^CYou're blind!\n");
        return(0);
    }


    if(cmnd->num > 1) {

        lowercize(cmnd->str[1],0);

        switch (cmnd->str[1][0]) {
        case 'a':
            switch (cmnd->str[1][1]) {

            case 'd':
                special = -2;
                break;
            case 's':
                cClass = CreatureClass::ASSASSIN;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;
        case 'b':
            switch (cmnd->str[1][3]) {
            case 'b':
                race = BARBARIAN;
                break;
            case 's':
                cClass = CreatureClass::BERSERKER;
                break;
            case 'd':
                cClass = CreatureClass::BARD;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;
        case 'c':
            switch (cmnd->str[1][1]) {
            case 'a':
                switch(cmnd->str[1][2]) {
                case 'm':
                    race = CAMBION;
                    break;
                case 'r':
                    if(!player->isCt()) {
                        player->print("You do not currently have that privilege.\n");
                        return(0);
                    } else
                        cClass = CreatureClass::CARETAKER;
                    break;
                default:
                    player->print("Parameter not unique.\n");
                    return(0);
                }
                break;
            case 'h':
                chaos = 1;
                break;
            case 'l':
                switch (cmnd->str[1][2]) {
                case 'e':
                    cClass = CreatureClass::CLERIC;
                    break;
                case 'a':
                    switch(cmnd->str[1][3]) {
                    case 's':
                        cClass = player->getClass();
                        break;
                    case 'n':
                        clan = 1;
                        break;

                    }
                    break;
                default:
                    player->print("Parameter not unique.\n");
                    return(0);
                }
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;

        case 'd':
            switch(cmnd->str[1][1]) {
            case 'a':
                race = DARKELF;
                break;
            case 'e':
                cClass = CreatureClass::DEATHKNIGHT;
                break;
            case 'r':
                cClass = CreatureClass::DRUID;
                break;
            case 'w':
                race = DWARF;
                break;
            case 'u':
                if(!player->isDm()) {
                    player->print("You do not currently have that privilege.\n");
                    return(0);
                } else
                    cClass = CreatureClass::DUNGEONMASTER;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;

        case 'e':
            race = ELF;
            break;
        case 'f':
            cClass = CreatureClass::FIGHTER;
            break;
        case 'g':
            switch(cmnd->str[1][1]) {
            case 'n':
                race = GNOME;
                break;
            case 'o':
                race = GOBLIN;
                break;
            case 'u':
                guild = 1;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;

        case 'h':
            switch(cmnd->str[1][1]) {
            case 'u':
                race = HUMAN;
                break;
            case 'a':
                switch(cmnd->str[1][4]) {
                case 'e':
                    race = HALFELF;
                    break;
                case 'g':
                    race = HALFGIANT;
                    break;
                case 'l':
                    race = HALFLING;
                    break;
                case 'o':
                    race = HALFORC;
                    break;
                default:
                    player->print("Parameter not unique.\n");
                    return(0);
                }
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;

        case 'k':
            switch(cmnd->str[1][1]) {
            case 'a':
                race = KATARAN;
                break;
            case 'e':
                race = KENKU;
                break;
            case 'o':
                race = KOBOLD;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;

        case 'l':
            switch(cmnd->str[1][1]) {
            case 'a':
                law = 1;
                break;
            case 'i':
                cClass = CreatureClass::LICH;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;


        case 'm':
            switch(cmnd->str[1][1]) {
            case 'a':
                cClass = CreatureClass::MAGE;
                break;
            case 'i':
                race = MINOTAUR;
                break;
            case 'o':
                cClass = CreatureClass::MONK;
                break;
            default:
                player->print("Unknown parameter.\n");
                return(0);
            }
            break;

        case 'o':
            switch(cmnd->str[1][1]) {
            case 'g':
                race = OGRE;
                break;
            case 'r':
                race = ORC;
                break;
            default:
                player->print("Unknown parameter.\n");
                return(0);
            }
            break;

        case 'p':
            switch(cmnd->str[1][1]) {
            case 'a':
                cClass = CreatureClass::PALADIN;
                break;
            case 'l':
                pledge = 1;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;

        case 'r':
            switch(cmnd->str[1][1]) {
            case 'a':
                cClass = CreatureClass::RANGER;
                break;
            case 'o':
                cClass = CreatureClass::ROGUE;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;

        case 's':
            race = SERAPH;
            break;
        case 't':
            switch(cmnd->str[1][1]) {
            case 'h':
                cClass = CreatureClass::THIEF;
                break;
            case 'r':
                race = TROLL;
                break;
            case 'i':
                race = TIEFLING;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;

        case 'v':
            switch (cmnd->str[1][1]) {
            case 'a':
                cClass = CreatureClass::PUREBLOOD;
                break;
            case 'e':
                special = -3;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;
        case 'w':
            switch(cmnd->str[1][1]) {
            case 'a':
                special = -1;
                break;
            case 'e':
                cClass = CreatureClass::WEREWOLF;
                break;
            default:
                player->print("Parameter not unique.\n");
                return(0);
            }
            break;
        case '.':
            cls = 1;
            break;

        default:
            player->print("Parameter not unique.\n");
            return(0);
        }// end main switch

    } // end if cmnd->num > 1

    std::ostringstream whoStr;
    std::string curStr;

    whoStr << "\n^BPlayers currently online:\n";
    whoStr << "-------------------------------------------------------------------------------^x\n";
    auto cmp = [](const std::shared_ptr<Player>& a, const std::shared_ptr<Player>& b) { return a->getSock()->getHostname() < b->getSock()->getHostname(); };
    std::multiset<std::shared_ptr<Player>, decltype(cmp)> sortedPlayers;
    for(const auto& [pId, ply] : gServer->players) sortedPlayers.insert(ply);

    for(const auto& target : sortedPlayers) {
        if(!target->isConnected()) continue;
        if(target->flagIsSet(P_DM_INVIS) && player->getClass() < target->getClass()) continue;
        if(target->isEffected("incognito") && player->getClass() < target->getClass() && player->getParent() != target->getParent()) continue;

        if(cls == 1) if(target->getClass() != player->getClass()) continue;
        if(clan == 1) if(!target->getClan()) continue;
        if(guild == 1) if(player->getGuild() && target->getGuild() != player->getGuild()) continue;
        if(chaos == 1) if(!target->flagIsSet(P_CHAOTIC)) continue;
        if(law == 1) if(target->flagIsSet(P_CHAOTIC)) continue;
        if(pledge == 1) if((target->getClan() && target->getClan() == player->getClan()) || !target->getClan()) continue;
        if(race > 0) if((player->willIgnoreIllusion() ? target->getRace() : target->getDisplayRace()) != race) continue;
        if(special == -1) if(!target->isPublicWatcher()) continue;
        if(special == -2) if(!target->flagIsSet(P_ADULT)) continue;
        if(special == -3) if(!target->flagIsSet(P_VETERAN)) continue;
        if(cClass != CreatureClass::NONE) if(target->getClass() != cClass) continue;
        if(target->flagIsSet(P_GLOBAL_GAG) && player != target && !player->isCt()) continue;

        found = true;

        curStr = target->getWhoString(false, true, player->willIgnoreIllusion());
        whoStr << curStr;
    }
    if(!found)
        player->print("Nobody found!\n");
    else {
        curStr = whoStr.str();
        player->printColor("%s\n", curStr.c_str());
    }

    return(0);
}

//*********************************************************************
//              classwho
//*********************************************************************
// This function outputs a list of all the players who are currently
// logged into the game which are the same class as the player doing
// the command.

int cmdClasswho(const std::shared_ptr<Player>& player, cmd* cmnd) {
    cmnd->num = 2;
    strcpy(cmnd->str[0], "who");
    strcpy(cmnd->str[1], "class");
    return(cmdWho(player, cmnd));
}


//*********************************************************************
//                      cmdWhois
//*********************************************************************
// The whois function displays a selected player's name, class, level
// title, age and gender

int cmdWhois(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;

    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    if(cmnd->num < 2) {
        player->print("Whois who?\n");
        return(0);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target || !player->canSee(target)) {
        player->print("That player is not logged on.\n");
        return(0);
    }

    player->printColor("%s", target->getWhoString(true, true, player->willIgnoreIllusion()).c_str());
    return(0);
}

//*********************************************************************
//                      cmdSuicide
//*********************************************************************
// This function is called whenever the player explicitly asks to
// commit suicide.

int cmdSuicide(const std::shared_ptr<Player>& player, cmd* cmnd) {

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->getProxyName().empty()) {
        player->print("You are unable to suicide a proxied character.\n");
        return(0);
    }

    if(player->flagIsSet(P_NO_SUICIDE)) {
        player->print("You cannot suicide right now. You are in a 24 hour cooling-off period.\n");
        return(0);
    }
    // Prevents players from suiciding in jail
    if(player->inJail()) {
        player->print("You attempt to kill yourself and fail. It sure hurts though!\n");
        broadcast(player->getSock(), player->getParent(),"%M tries to kill %sself.", player.get(), player->himHer());
        return(0);
    }

    if(cmnd->num < 2 || strcasecmp(cmnd->str[1], "yes")) {
        player->printColor("^rWARNING:^x This will completely erase your character!\n");
        player->print("To prevent accidental suicides you must confirm you want to suicide;\nto do so type 'suicide yes'\n");
        if(player->getGuild() && player->getGuildRank() == GUILD_MASTER)
            player->printColor("^yNote:^x if you want to control who takes over your guild after you suicide,\nbe sure to abdicate leadership first.\n");
        return(0);
    }

    //if(player->getLevel() > 2 && !player->isStaff())
        broadcast("### %s committed suicide! We'll miss %s dearly.", player->getCName(), player->himHer());
    //else
    //  broadcast(isWatcher, "^C### %s committed suicide! We'll miss %s dearly.", player->getCName(), player->himHer());

    logn("log.suicide", fmt::format("{}({})-{} ({})\n", player->getCName(), player->getPassword().c_str(), player->getLevel(), player->getSock()->getHostname()).c_str());

    player->deletePlayer();
    updateRecentActivity();
    return(0);
}


//*********************************************************************
//                      deletePlayer
//*********************************************************************
// Does a true delete of a player and their files

void Player::deletePlayer() {
    bool hardcore = isHardcore();
    // cache the name because we will be deleting the player object
    std::string name = getName();

    auto pThis = Containable::downcasted_shared_from_this<Player>();

    gConfig->deleteUniques(pThis);
    gServer->clearAsEnemy(pThis);

    // remove minions before backup, since this affects other players as well
    clearMinions();

    // they are no longer the owner of any effects they have created
    gServer->removeEffectsOwner(pThis);

    // unassociate
    if(!getForum().empty()) {
        setForum("");
        webUnassociate(name);
    }

    // take the player out of any guilds
    updateGuild(pThis, GUILD_REMOVE);

    // save a backup - this will be the only copy of the player!
    if(getLevel() >= 7)
        save(false, LoadType::LS_BACKUP);

    if(flagIsSet(P_CREATING_GUILD)) {
        // If they are the founder, this will return text, and we
        // need go no further. Otherwise, they are a supporter, and we
        // have to search for them.
        if(!gConfig->removeGuildCreation(name).empty()) {
            std::list<GuildCreation*>::iterator gcIt;
            for(gcIt = gConfig->guildCreations.begin(); gcIt != gConfig->guildCreations.end(); gcIt++) {
                // they were supporting this guild - stop
                if((*gcIt)->removeSupporter(name))
                    break;
            }
            gConfig->saveGuilds();
        }
    }

    // this deletes the player object
    std::shared_ptr<Socket> sock = getSock();
    uninit();
    gServer->clearPlayer(name);
    sock->clearPlayer();

    // get rid of any files the player was using
    fs::remove((Path::Player / name).replace_extension("xml"));
    fs::remove((Path::Bank / name).replace_extension("txt"));
    fs::remove((Path::Post / name).replace_extension("txt"));
    fs::remove((Path::History / name).replace_extension("txt"));

    // handle removing and property this player owned
    gConfig->destroyProperties(name);

    if(hardcore)
        sock->reconnect(true);
    else
        sock->disconnect();
}


//*********************************************************************
//                      cmdQuit
//*********************************************************************
// This function checks to see if a player is allowed to quit yet. It
// checks to make sure the player isn't in the middle of combat, and if
// so, the player is not allowed to quit (and 0 is returned).

int cmdQuit(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    i=0, t=0;

    if(!player->ableToDoCommand())
        return(0);

    t = time(nullptr);
    if(player->inCombat())
        i = player->getLTAttack() + 20;
    else
        i = player->getLTAttack() + 2;
    if(t < i) {
        player->pleaseWait(i-t);
        return(0);
    }
    if(player->inCombat())
        i = LT(player, LT_SPELL) + 20;
    else
        i = LT(player, LT_SPELL) + 2;

    if(t < i) {
        player->pleaseWait(i-t);
        return(0);
    }
    player->update();


    return(DISCONNECT);
}


//*********************************************************************
//                      changeStats
//*********************************************************************

int cmdChangeStats(const std::shared_ptr<Player>& player, cmd* cmnd) {
    player->changeStats();
    return(0);
}

void Player::changeStats() {
    if(!flagIsSet(P_CAN_CHANGE_STATS) && !isCt()) {
        print("You cannot change your stats at this time.\n");
        return;
    } else {
        getSock()->getPlayer()->newStats.st = 0;
        getSock()->getPlayer()->newStats.de = 0;
        getSock()->getPlayer()->newStats.co = 0;
        getSock()->getPlayer()->newStats.in = 0;
        getSock()->getPlayer()->newStats.pi = 0;

        broadcast(::isCt, "^y### %s is choosing new stats.", getCName());
        printColor("^yPlease enter a new set of initial stats (56 points):\n");
        getSock()->setState(CON_CHANGING_STATS);
    }
}


//********************************************************************
//                      changingStats
//********************************************************************
// This function allows a player to change their stats

void changingStats(std::shared_ptr<Socket> sock, const std::string& str) {
    sock->getPlayer()->changingStats(str);
}

void Player::changingStats(std::string str) {
    int sum;
    std::vector<std::string> inputArgs;
    std::vector<int> statInput;
    std::shared_ptr<Socket> sock = getSock();
    std::shared_ptr<Player> player = sock->getPlayer();

    switch(sock->getState()) {
    case CON_CHANGING_STATS:
        // empty input would cause crash here
        if (str.length() < 1) {
            sock->print("Aborted.\n");
            sock->setState(CON_PLAYING);
            return;
        }

        inputArgs = splitString(str, " ");
        try {
            std::transform(inputArgs.begin(), inputArgs.end(), std::back_inserter(statInput), [](const std::string &s) { return std::stoi(s); });
        } catch (std::invalid_argument &) {
            sock->print("Invalid input\n");
            sock->print("Aborted.\n");
            sock->setState(CON_PLAYING);
            return;
        }

        if(statInput.size() < 5) {
            sock->print("Please enter all 5 numbers.\n");
            sock->print("Aborted.\n");
            sock->setState(CON_PLAYING);
            return;
        }

        if (std::any_of(statInput.begin(), statInput.end(), [](int i){ return i < 3 || i > 18; })){
            sock->print("No stats < 3 or > 18 please.\n");
            sock->print("Aborted.\n");
            sock->setState(CON_PLAYING);
            return;
        }

        sum = std::accumulate(statInput.begin(), statInput.end(), 0);
        if(sum != 56) {
            sock->print("Stat total must equal 56 points.\n");
            sock->print("Aborted.\n");
            sock->setState(CON_PLAYING);
            return;
        }

        player->newStats.st = statInput[0];
        player->newStats.de = statInput[1];
        player->newStats.co = statInput[2];
        player->newStats.in = statInput[3];
        player->newStats.pi = statInput[4];

        if(race == HUMAN) {
            sock->print("Raise which stat?:\n[A] Strength, [B] Dexterity, [C] Constitution, [D] Intelligence, or [E] Piety.\n");
            sock->setState(CON_CHANGING_STATS_RAISE);
            return;
        }

        sock->print("Your stats have been calculated.\n");
        sock->print("Please press [ENTER].\n");
        sock->setState(CON_CHANGING_STATS_CALCULATE);
        return;
    case CON_CHANGING_STATS_CALCULATE:
        // keep track of previous stats
        player->oldStats.st = player->strength.getInitial();
        player->oldStats.de = player->dexterity.getInitial();
        player->oldStats.co = player->constitution.getInitial();
        player->oldStats.in = player->intelligence.getInitial();
        player->oldStats.pi = player->piety.getInitial();

        // set new stats
        player->strength.setInitial(player->newStats.st * 10);
        player->dexterity.setInitial(player->newStats.de * 10);
        player->constitution.setInitial(player->newStats.co * 10);
        player->intelligence.setInitial(player->newStats.in * 10);
        player->piety.setInitial(player->newStats.pi * 10);

        // make race adjustments
        player->strength.addInitial( gConfig->getRace(player->getRace())->getStatAdj(STR) );
        player->dexterity.addInitial( gConfig->getRace(player->getRace())->getStatAdj(DEX) );
        player->constitution.addInitial( gConfig->getRace(player->getRace())->getStatAdj(CON) );
        player->intelligence.addInitial( gConfig->getRace(player->getRace())->getStatAdj(INT) );
        player->piety.addInitial( gConfig->getRace(player->getRace())->getStatAdj(PTY) );

        sock->print("Your resulting stats will be as follows:\n");
        sock->print(
            "STR: %d   DEX: %d   CON: %d   INT: %d   PIE: %d\n",
            player->strength.getMax(),
            player->dexterity.getMax(),
            player->constitution.getMax(),
            player->intelligence.getMax(),
            player->piety.getMax()
        );
        sock->print("Hit Points: %d\n", player->hp.getMax());
        sock->print("Magic Points: %d\n", player->mp.getMax());
        sock->print("Are these the stats you want? (Y/N)\n");
        sock->setState(CON_CHANGING_STATS_CONFIRM);
        break;
    case CON_CHANGING_STATS_CONFIRM:
        if(low(str[0]) == 'y') {
            broadcast(::isCt, "^y### %s has chosen %s stats.", getCName(), hisHer());
            sock->print("New stats set.\n");
            clearFlag(P_CAN_CHANGE_STATS);
            sock->setState(CON_PLAYING);
            return;
        } else {
            // revert to old stats
            player->strength.setInitial(player->oldStats.st);
            player->dexterity.setInitial(player->oldStats.de);
            player->constitution.setInitial(player->oldStats.co);
            player->intelligence.setInitial(player->oldStats.in);
            player->piety.setInitial(player->oldStats.pi);

            sock->print("Keeping old stats:\n");
            sock->print(
                "STR: %d   DEX: %d   CON: %d   INT: %d   PIE: %d\n",
                player->strength.getMax(),
                player->dexterity.getMax(),
                player->constitution.getMax(),
                player->intelligence.getMax(),
                player->piety.getMax()
            );

            broadcast(::isCt,"^y### %s aborted choosing new stats.", getCName());
            sock->print("Aborted.\n");
            sock->setState(CON_PLAYING);
            return;
        }
    case CON_CHANGING_STATS_RAISE:
        switch (low(str[0])) {
        case 'a':
            player->newStats.st++;
            break;
        case 'b':
            player->newStats.de++;
            break;
        case 'c':
            player->newStats.co++;
            break;
        case 'd':
            player->newStats.in++;
            break;
        case 'e':
            player->newStats.pi++;
            break;
        default:
            sock->print("\nPlease choose one.\n");
            return;
        }

        sock->print("Lower which stat?:\n[A] Strength, [B] Dexterity, [C] Constitution, [D] Intelligence, or [E] Piety.\n");
        sock->setState(CON_CHANGING_STATS_LOWER);
        return;
    case CON_CHANGING_STATS_LOWER:
        switch (low(str[0])) {
        case 'a':
            player->newStats.st = std::max(1, newStats.st-1);
            break;
        case 'b':
            player->newStats.de = std::max(1, newStats.de-1);
            break;
        case 'c':
            player->newStats.co = std::max(1, newStats.co-1);
            break;
        case 'd':
            player->newStats.in = std::max(1, newStats.in-1);
            break;
        case 'e':
            player->newStats.pi = std::max(1, newStats.pi-1);
            break;
        default:
            sock->print("\nPlease choose one.");
            return;
        }

        sock->print("Your stats have been calculated.\n");
        sock->print("Please press [ENTER].\n");
        sock->setState(CON_CHANGING_STATS_CALCULATE);
        break;
    }
}



//*********************************************************************
//                      timestr
//*********************************************************************

char *timestr(long t) {
    static char buf[BUFSIZ];
    if(t >= 60) {
        sprintf(buf, "%01ld:%02ld", t / 60, t % 60);
    } else {
        sprintf(buf, "%ld seconds", t);
    }
    return(buf);
}

#define TIMELEFT(name,ltflag) \
    tmp = std::max<long>(0,(LT(player, (ltflag)) - t)); \
    player->print("Time left on current %s: %s.\n", name, timestr(tmp));
#define TIMEUNTIL(name,ltflag,time) \
    u = (time) - t + player->lasttime[(ltflag)].ltime;\
    if(u < 0) u = 0;\
    player->print("Time left before next %s: %s.\n", name, timestr(u));

//*********************************************************************
//                      showAbility
//*********************************************************************

void showAbility(const std::shared_ptr<Player>& player, const char *skill, const char *display, int lt, int tu, int flag = -1) {
    long    t = time(nullptr), u=0, tmp=0;

    if(player->knowsSkill(skill)) {
        if(flag != -1 && player->flagIsSet(flag)) {
            TIMELEFT(display, lt);
        } else {
            TIMEUNTIL(display, lt, tu);
        }
    }
}


//*********************************************************************
//                      cmdTime
//*********************************************************************
// This function outputs the time of day (realtime) to the player.

int cmdTime(const std::shared_ptr<Player>& player, cmd* cmnd) {
    long    t = time(nullptr), u=0, tmp=0, i=0;
    const CatRefInfo* cri = gConfig->getCatRefInfo(player->getRoomParent(), 1);
    std::string world;

    if(cri && !cri->getWorldName().empty()) {
        world += " of ";
        world += cri->getWorldName();
    }

    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        *player << "You are brain-dead. You can't do that.\n";
        return(0);
    }

    *player << ColorOn << "^cThe current time in the world" << world.c_str() << ":\n" << ColorOff;

    gConfig->getCalendar()->printtime(player);

    if(gConfig->getCalendar()->isBirthday(player))
        *player << ColorOn << "^yToday is your birthday!\n" << ColorOff;

    if(player->flagIsSet(P_PTESTER))
        *player << "Tick Interval: " << player->lasttime[LT_TICK].interval << ".\n";

    if(cmnd->num < 2) {

        // for ceris/aramon when they resurrect dead people
        tmp = std::max<long>(0,(LT(player, LT_NOMPTICK) - t));
        if(tmp > 0)
            *player << "Time until vitality is restored: " << timestr(tmp) << ".\n";

        *player << "\n";

        // CT+ doesn't need to see all this info
        if(!player->isCt()) {
            if(player->getClass() == CreatureClass::BARD && player->getLevel() >= 4) {
                TIMEUNTIL("bard song", LT_SING, player->lasttime[LT_SING].interval);
            }
            showAbility(player, "barkskin", "barkskin", LT_BARKSKIN, 600);
            showAbility(player, "berserk", "berserk", LT_BERSERK, player->lasttime[LT_BERSERK].interval);
            showAbility(player, "bite", "bite", LT_PLAYER_BITE, player->lasttime[LT_PLAYER_BITE].interval);
            showAbility(player, "bloodsac", "blood sacrifice", LT_BLOOD_SACRIFICE, 600);
            showAbility(player, "commune", "commune", LT_PRAY, 45);
            showAbility(player, "charm", "charm", LT_HYPNOTIZE, player->lasttime[LT_HYPNOTIZE].interval);
            showAbility(player, "creeping-doom", "creeping-doom", LT_SMOTHER, player->lasttime[LT_SMOTHER].interval);
            showAbility(player, "disarm", "disarm", LT_DISARM, player->lasttime[LT_DISARM].interval);
            showAbility(player, "drain", "drain life", LT_DRAIN_LIFE, player->lasttime[LT_DRAIN_LIFE].interval);
            showAbility(player, "smother", "earth-smother", LT_SMOTHER, player->lasttime[LT_SMOTHER].interval);
            showAbility(player, "enthrall", "enthrall", LT_HYPNOTIZE, player->lasttime[LT_HYPNOTIZE].interval);
            showAbility(player, "focus", "focus", LT_FOCUS, 600, P_FOCUSED);
            showAbility(player, "frenzy", "frenzy", LT_FRENZY, 600);
            showAbility(player, "harm", "harm touch", LT_LAY_HANDS, player->lasttime[LT_LAY_HANDS].interval);
            showAbility(player, "holyword", "holyword", LT_SMOTHER, player->lasttime[LT_SMOTHER].interval);
            showAbility(player, "howl", "howl", LT_HOWLS, 240);
            showAbility(player, "hypnotize", "hypnotize", LT_HYPNOTIZE, player->lasttime[LT_HYPNOTIZE].interval);
            if( player->knowsSkill("identify") ||
                player->canBuildObjects()
            ) {
                TIMEUNTIL("identify", LT_IDENTIFY, player->lasttime[LT_IDENTIFY].interval);
            }
           
            showAbility(player, "hands", "lay on hands", LT_LAY_HANDS, player->lasttime[LT_LAY_HANDS].interval);
            showAbility(player, "maul", "maul", LT_KICK, player->lasttime[LT_KICK].interval);
            showAbility(player, "meditate", "meditate", LT_MEDITATE, player->lasttime[LT_MEDITATE].interval);
            showAbility(player, "mistbane", "mistbane", LT_FOCUS, 600, P_MISTBANE);
            showAbility(player, "poison", "poison", LT_DRAIN_LIFE, player->lasttime[LT_DRAIN_LIFE].interval);
            showAbility(player, "pray", "pray", LT_PRAY, 600);
            showAbility(player, "regenerate", "regenerate", LT_REGENERATE, player->lasttime[LT_REGENERATE].interval);
            showAbility(player, "renounce", "renounce", LT_RENOUNCE, player->lasttime[LT_RENOUNCE].interval);
            showAbility(player, "touch", "touch of death", LT_TOUCH_OF_DEATH, player->lasttime[LT_TOUCH_OF_DEATH].interval);
            showAbility(player, "turn", "turn undead", LT_TURN, player->lasttime[LT_TURN].interval);
            showAbility(player, "scout", "scout", LT_SCOUT, player->lasttime[LT_SCOUT].interval);
        }

        if(player->flagIsSet(P_OUTLAW)) {
            i = LT(player, LT_OUTLAW);
            *player << "Outlaw time remaining: ";
            if(i - t > 3600)
                *player << ((i-t)/3600L) << ":" << (((i - t) % 3600L) / 60L) << ":" << ((i - t) % 60L) << " more hours.\n";
            else if((i - t > 60) && (i - t < 3600))
                *player << ((i - t) / 60L) << ":" << ((i - t) % 60L) << " more minutes.\n";
            else
                *player << (std::max<int>((i - t),0)) << " more seconds.\n";
        }

        if(player->flagIsSet(P_JAILED)) {
            if( player->inUniqueRoom() &&
                player->getUniqueRoomParent()->flagIsSet(R_MOB_JAIL)
            )
                i = LT(player, LT_MOB_JAILED);
            else
                i = LT(player, LT_JAILED);

            *player << "NOTE: 0 Seconds means you're out any second.\n";
            *player << "Jailtime remaining(Approx): ";
            if(i - t > 3600)    
                *player << ((i-t)/3600L) << ":" << (((i - t) % 3600L) / 60L) << ":" << ((i - t) % 60L) << " more hours.\n";
            else if((i - t > 60) && (i - t < 3600))  
                *player << ((i - t) / 60L) << ":" << ((i - t) % 60L) << " more minutes.\n";
            else
                *player << (std::max<int>((i - t),0)) << " more seconds.\n";
        }

        i = 0;
        for(const auto& pet : player->pets) {
            if(pet->isMonster() && pet->isPet()) {
                i = 1;
                if(pet->isUndead())
                    *player << "Time left before creature following you leaves/fades: " << (timestr(pet->lasttime[LT_ANIMATE].ltime+pet->lasttime[LT_ANIMATE].interval-t)) << "\n";
                else
                    *player << "Time left before creature following you leaves/fades: " << (timestr(pet->lasttime[LT_INVOKE].ltime+pet->lasttime[LT_INVOKE].interval-t)) << "\n";

            }
        }

        // only show how long until next if they don't have a pet already
        if(!i) {
            if( player->getClass() == CreatureClass::DRUID ||
                (player->getClass() == CreatureClass::CLERIC && player->getDeity() == GRADIUS))
            {
                TIMEUNTIL("conjure", LT_INVOKE, player->lasttime[LT_INVOKE].interval);
            }
            if( player->getClass() == CreatureClass::CLERIC &&
                player->getDeity() == ARAMON &&
                player->getAdjustedAlignment() <= REDDISH)
            {
                TIMEUNTIL("animate dead", LT_ANIMATE, player->lasttime[LT_ANIMATE].interval);
            }
        }
    }
    return(0);
}


//*********************************************************************
//                      cmdSave
//*********************************************************************
// This is the function that is called when a players types save
// It calls save() (the real save function) and then tells the user
// the player was saved.  See save() for more details.

int cmdSave(const std::shared_ptr<Player>& player, cmd* cmnd) {
    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    player->save(true);
    // Save storage rooms
    if(player->inUniqueRoom() && player->getUniqueRoomParent()->info.isArea("stor"))
        gServer->resaveRoom(player->getUniqueRoomParent()->info);

    player->print("Player saved.\n");
    return(0);
}
