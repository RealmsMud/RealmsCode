/*
 * command4.cpp
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
#include <algorithm>               // for std::min
#include <cstdio>                  // for sprintf
#include <cstdlib>                 // for abs
#include <cstring>                 // for strchr, strncasecmp
#include <ostream>                 // for operator<<, basic_ostream, ostring...
#include <string>                  // for allocator, string, char_traits

#include <boost/algorithm/string/case_conv.hpp>  // for to_lower

#include "account.hpp"             // for Account
#include "accountUpgrades.hpp"     // for AccountUpgradeDefinition
#include "calendar.hpp"            // for cDay, Calendar, cMonth
#include "cmd.hpp"                 // for cmd
#include "commands.hpp"            // for cmdAge, cmdHelp, cmdInfo, cmdVersion
#include "config.hpp"              // for Config, gConfig
#include "flags.hpp"               // for P_AFK
#include "global.hpp"              // for DOPROMPT
#include "lasttime.hpp"            // for lasttime
#include "mud.hpp"                 // for LT_AGE
#include "mudObjects/players.hpp"  // for Player
#include "paths.hpp"               // for Help
#include "proto.hpp"               // for getOrdinal, up
#include "server.hpp"              // for Server, gServer
#include "socket.hpp"              // for Socket
#include "version.hpp"             // for VERSION

//*********************************************************************
//                      cmdHelp
//*********************************************************************
// This function allows a player to get help in general, or help for a
// specific command. If help is typed by itself, a list of commands
// is produced. Otherwise, help is supplied for the command specified

int cmdHelp(const std::shared_ptr<Player>& player, cmd* cmnd) {
    char    file[80];
    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    if(cmnd->num < 2) {
        sprintf(file, "%s/helpfile.txt", Path::Help.c_str());
        player->getSock()->viewFile(file, true);
        return(DOPROMPT);
    }
    if(strchr(cmnd->str[1], '/')!=nullptr) {
        player->print("You may not use backslashes.\n");
        return(0);
    }
    sprintf(file, "%s/%s.txt", Path::Help.c_str(), cmnd->str[1]);
    player->getSock()->viewFile(file, true);
    return(DOPROMPT);
}

//*********************************************************************
//                      cmdWelcome
//*********************************************************************
// Outputs welcome file to user, giving them info on how to play
// the game

int cmdWelcome(const std::shared_ptr<Player>& player, cmd* cmnd) {
    char    file[80];
    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    sprintf(file, "%s/welcomerealms.txt", Path::Help.c_str());

    player->getSock()->viewFile(file, true);
    return(0);
}

//*********************************************************************
//                      getTimePlayed
//*********************************************************************

std::string Player::getTimePlayed() const {
    std::ostringstream oStr;
    long    played = lasttime[LT_AGE].interval;

    if(played > 86400L)
        oStr << (played / 86400) << " Day" << ((played / 86400 == 1) ? ", " : "s, ");
    if(played > 3600L)
        oStr << (played % 86400L) / 3600L << " Hour" << ((played % 86400L) / 3600L == 1 ? ", " : "s, ");
    oStr << (played % 3600L) / 60L << " Minute" << ((played % 3600L) / 60L == 1 ? "" : "s");

    return(oStr.str());
}

//*********************************************************************
//                      showAge
//*********************************************************************

void Player::showAge(const std::shared_ptr<Player> viewer) const {
    if(birthday) {
        viewer->printColor("^gAge:^x  %d\n^gBorn:^x the %s of %s, the %s month of the year,\n      ",
            getAge(), getOrdinal(birthday->getDay()).c_str(),
            gConfig->getCalendar()->getMonth(birthday->getMonth())->getName().c_str(),
            getOrdinal(birthday->getMonth()).c_str());
        if(birthday->getYear()==0)
            viewer->print("the year the Kingdom of Bordia fell.\n");
        else
            viewer->print("%d year%s %s the fall of the Kingdom of Bordia.\n",
                abs(birthday->getYear()), abs(birthday->getYear())==1 ? "" : "s",
                birthday->getYear() > 0 ? "after" : "before");


        if(gConfig->getCalendar()->isBirthday(Containable::downcasted_shared_from_this<Player>())) {
            if(viewer.get() == this)
                viewer->printColor("^yToday is your birthday!\n");
            else
                viewer->printColor("^yToday is %s's birthday!\n", getCName());
        }
    } else
        viewer->printColor("^gAge:^x  unknown\n");

    viewer->print("\n");

    std::string str = getCreatedStr();
    if(!str.empty())
        viewer->printColor("^gCharacter Created:^x %s\n", str.c_str());

    viewer->printColor("^gTime Played:^x %s\n\n", getTimePlayed().c_str());
}


//*********************************************************************
//                      cmdAge
//*********************************************************************

int cmdAge(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target = player;

    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    if(player->isCt()) {
        if(cmnd->num > 1) {
            cmnd->str[1][0] = up(cmnd->str[1][0]);
            target = gServer->findPlayer(cmnd->str[1]);
            if(!target || (target->isDm() && !player->isDm())) {
                player->print("That player is not logged on.\n");
                return(0);
            }
        }
    }

    player->print("%s the %s (level %d)\n", target->getCName(), target->getTitle().c_str(), target->getLevel());
    player->print("\n");

    target->showAge(player);
    return(0);
}

//*********************************************************************
//                      cmdVersion
//*********************************************************************
// Shows the players the version of the mud and last compile time

int cmdVersion(const std::shared_ptr<Player>& player, cmd* cmnd) {
    player->print("Mud Version: " VERSION "\nLast compiled " __TIME__ " on " __DATE__ ".\n");
    return(0);
}

//*********************************************************************
//                      cmdInfo
//*********************************************************************

int cmdInfo(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target = player;

    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    if(player->isCt()) {
        if(cmnd->num > 1) {
            cmnd->str[1][0] = up(cmnd->str[1][0]);
            target = gServer->findPlayer(cmnd->str[1]);
            if(!target || (target->isDm() && !player->isDm())) {
                player->print("That player is not logged on.\n");
                return(0);
            }
            target->information(player);
            return(0);
        }
    }

    target->information();
    return(0);
}

//*********************************************************************
//                      cmdAccount
//*********************************************************************

int cmdAccount(const std::shared_ptr<Player>& player, cmd* cmnd) {
    if(!player->hasAccount()) {
        player->print("You do not have an account.\n");
        return(0);
    }

    if(cmnd->num < 2) {
        player->print("Account command options:\n");
        player->print("  ^Waccount (i)nfo^x - Display account information\n");
        player->print("  ^Waccount (c)haracters^x - List your characters\n");
        player->print("  ^Waccount (u)pgrade^x - View account upgrade options\n");
        player->print("  ^Waccount (u)pgrade (b)uy <name>^x - Purchase an upgrade\n");
        return(0);
    }

    auto account = gServer->getOrLoadAccount(player->getAccountName());
    if(!account) {
        player->print("Unable to load your account information.\n");
        return(0);
    }

    std::string subcommand = cmnd->str[1];
    boost::to_lower(subcommand);

    // Handle "info" command with partial matching
    if(partialMatch(subcommand, "info", 4)) {
        player->print("\n^W~~~~~~~ Account Information ~~~~~~~^x\n\n");
        account->printInfoFields(player);
        
        return(0);
    }

    // Handle "characters" with partial matching
    if(partialMatch(subcommand, "characters", 10)) {
        account->printCharacterList(player);
        return(0);
    }

    if(partialMatch(subcommand, "upgrade", 7) && cmnd->num == 2) {
        account->printUpgradeSummary(player);
        player->print("\nUse '^Waccount upgrade buy <name>^x' to purchase upgrades.\n");
        return(0);
    }

    if(partialMatch(subcommand, "upgrade", 7) && cmnd->num > 2) {
        if(cmnd->num < 3) {
            player->print("Usage: ^Waccount upgrade buy <upgradeName>^x\n");
            return(0);
        }

        std::string action = cmnd->str[2];
        boost::to_lower(action);

        if(!partialMatch(action, "buy", 3)) {
            player->print("Unknown account upgrade action '%s'. Try 'buy'.\n", action.c_str());
            return(0);
        }

        if(cmnd->num < 4) {
            player->print("Usage: ^Waccount upgrade buy <upgradeName>^x\n");
            return(0);
        }

        std::string upgradeName = cmnd->str[3];
        const AccountUpgradeDefinition* def = matchAccountUpgrade(upgradeName);
        if(!def) {
            player->print("Unknown upgrade '%s'. Type '^Waccount upgrade^x' for a list.\n", upgradeName.c_str());
            return(0);
        }

        std::string name(def->displayName);
        unsigned short currentRank = account->getUpgradeLevel(def->id);
        if(currentRank >= def->maxRank) {
            player->print("^W%s^x is already at maximum rank (%u).\n", name.c_str(), def->maxRank);
            return(0);
        }

        unsigned long cost = def->costPerRank;
        unsigned long availableExp = account->getAvailableExp();
        if(availableExp < cost) {
            player->print("You need ^G%lu^x more account experience to purchase %s.\n",
                          cost - availableExp, name.c_str());
            return(0);
        }

        if(!account->spendExp(cost)) {
            player->print("Unable to spend your account experience right now. Please try again.\n");
            return(0);
        }

        account->setUpgradeLevel(def->id, currentRank + 1);
        if(!account->save()) {
            player->print("^RWarning:^x failed to save your account. Please contact staff.\n");
        }

        player->applyAccountUpgradeBonuses();

        unsigned short newRank = account->getUpgradeLevel(def->id);
        auto bonusDesc = describeAccountUpgradeBonus(*def, account->getUpgradeValue(def->id));
        player->print("Purchased ^W%s^x rank ^G%u/%u^x. Bonus is now %s.\n",
                      name.c_str(), newRank, def->maxRank, bonusDesc.c_str());
        return(0);
    }

    player->print("Unknown account option '%s'.\n", subcommand.c_str());
    player->print("Type '^Waccount^x' for a list of available options.\n");
    return(0);
}

