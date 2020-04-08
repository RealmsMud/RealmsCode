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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include <iomanip>

#include "calendar.hpp"
#include "clans.hpp"
#include "commands.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "deityData.hpp"
#include "mud.hpp"
#include "raceData.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "version.hpp"
#include "xml.hpp"


//*********************************************************************
//                      cmdHelp
//*********************************************************************
// This function allows a player to get help in general, or help for a
// specific command. If help is typed by itself, a list of commands
// is produced. Otherwise, help is supplied for the command specified

int cmdHelp(Player* player, cmd* cmnd) {
    char    file[80];
    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    if(cmnd->num < 2) {
        sprintf(file, "%s/helpfile.txt", Path::Help);
        viewFile(player->getSock(), file);
        return(DOPROMPT);
    }
    if(strchr(cmnd->str[1], '/')!=nullptr) {
        player->print("You may not use backslashes.\n");
        return(0);
    }
    sprintf(file, "%s/%s.txt", Path::Help, cmnd->str[1]);
    viewFile(player->getSock(), file);
    return(DOPROMPT);
}

//*********************************************************************
//                      cmdWelcome
//*********************************************************************
// Outputs welcome file to user, giving them info on how to play
// the game

int cmdWelcome(Player* player, cmd* cmnd) {
    char    file[80];
    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    sprintf(file, "%s/welcomerealms.txt", Path::Help);

    viewFile(player->getSock(), file);
    return(0);
}

//*********************************************************************
//                      getTimePlayed
//*********************************************************************

bstring Player::getTimePlayed() const {
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

void Player::showAge(const Player* viewer) const {
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


        if(gConfig->getCalendar()->isBirthday(this)) {
            if(viewer == this)
                viewer->printColor("^yToday is your birthday!\n");
            else
                viewer->printColor("^yToday is %s's birthday!\n", getCName());
        }
    } else
        viewer->printColor("^gAge:^x  unknown\n");

    viewer->print("\n");

    bstring str = getCreatedStr();
    if(str != "")
        viewer->printColor("^gCharacter Created:^x %s\n", str.c_str());

    viewer->printColor("^gTime Played:^x %s\n\n", getTimePlayed().c_str());
}


//*********************************************************************
//                      cmdAge
//*********************************************************************

int cmdAge(Player* player, cmd* cmnd) {
    Player  *target = player;

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

int cmdVersion(Player* player, cmd* cmnd) {
    player->print("Mud Version: " VERSION "\nLast compiled " __TIME__ " on " __DATE__ ".\n");
    return(0);
}

//*********************************************************************
//                      cmdInfo
//*********************************************************************

int cmdInfo(Player* player, cmd* cmnd) {
    Player  *target = player;

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

