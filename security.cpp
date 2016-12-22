/*
 * security.cpp
 *   Security related functions
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "creatures.h"
#include "mud.h"
#include "login.h"
#include "server.h"
#include "socket.h"

bstring Player::hashPassword(bstring pass) {
    // implement md5 or sha1 here if you want 
    return(pass);
}

bstring Player::getPassword() const {
    return(password);
}

bool Player::isPassword(bstring pass) const {
    return(password == hashPassword(pass));
}

void Player::setPassword(bstring pass) {
    lastPassword = password;
    password = hashPassword(pass);
}

const char *passCriteria =
    "Passwords must meet the following criteria:\n"
    "1) Must be from %d to %d characters in length.\n"
    "2) Must contain at least 2 letters.\n"
    "3) Must contain at least 2 numbers or special characters.\n"
    "4) Must contain no leading or trailing spaces.\n";


bool isValidPassword(Socket* sock, bstring pass) {
    int         len=0, alpha=0, i=0, digits=0, special=0;

    if(!sock)
        return(false);

    len = pass.length();

    if(pass[0] == ' ') {
        sock->print("No leading spaces allowed.\n\n");
        sock->print(passCriteria, PASSWORD_MIN_LENGTH, PASSWORD_MAX_LENGTH);
        return(false);
    }

    if(pass[len-1] == ' ') {
        sock->print("No trailing spaces allowed.\n\n");
        sock->print(passCriteria, PASSWORD_MIN_LENGTH, PASSWORD_MAX_LENGTH);
        return(false);
    }

    if(len < PASSWORD_MIN_LENGTH) {
        sock->print("Too short.\n\n");
        sock->print(passCriteria, PASSWORD_MIN_LENGTH, PASSWORD_MAX_LENGTH);
        return(false);
    }

    if(len > PASSWORD_MAX_LENGTH) {
        sock->print("Too long.\n\n");
        sock->print(passCriteria, PASSWORD_MIN_LENGTH, PASSWORD_MAX_LENGTH);
        return(false);
    }

    for(i=0; i<len; i++) {
        if(isalpha(pass[i]))
            alpha++;
        if(isdigit(pass[i]))
            digits++;
        if(!isalpha(pass[i]) && !isdigit(pass[i]))
            special++;
    }

    if(alpha < 2) {
        sock->print("Not enough letters.\n\n");
        sock->print(passCriteria, PASSWORD_MIN_LENGTH, PASSWORD_MAX_LENGTH);
        return(false);
    }

    if(digits + special < 2) {
        sock->print("Not enough numbers or special characters.\n\n");
        sock->print(passCriteria, PASSWORD_MIN_LENGTH, PASSWORD_MAX_LENGTH);
        return(false);
    }

    return(true);
}

//*********************************************************************
//                      cmdPassword
//*********************************************************************
// This function cllls the function to allow
// a player to change their password.

int cmdPassword(Player* player, cmd* cmnd) {
    player->clearFlag(P_AFK);
    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }
    if(player->getProxyName() != "") {
        player->print("You are unable to change the password of a proxied character.\n");
        return(0);
    }

    // do not flash output until player hits return
    player->setFlag(P_READING_FILE);
    player->print("%c%c%c\n\r", 255, 251, 1);
    player->print("Current password: ");
    gServer->processOutput();
    //    sock->intrpt &= ~1;
    player->getSock()->setState(CON_CHANGE_PASSWORD);
    return(0);
}

//*********************************************************************
//                      changePassword
//*********************************************************************
// This command handles the procedure involved in
// changing a player's password. A player first must enter the 
// correct current password, then the new password, and re enter
// the new password to comfirm it.If the player enters the
// wrong password  or an invalid password (too short or long),
// the password will not be changed and the procedure is aborted.

void changePassword(Socket* sock, bstring str) {
    Player* player = sock->getPlayer();
    gServer->processOutput();

    switch (sock->getState()) {
    case CON_CHANGE_PASSWORD:
        if(player->isPassword(str)) {
            sock->print("%c%c%c\n\r", 255, 251, 1);
            sock->print("New password: ");
            gServer->processOutput();
            sock->intrpt &= ~1;
            player->getSock()->setState(CON_CHANGE_PASSWORD_GET_NEW);
            return;
            //          (fd,changePassword,2);
        } else {
            sock->print("%c%c%c\n\r", 255, 252, 1);
            sock->print("Incorrect password.\n");
            sock->print("Aborting.\n");
            player->clearFlag(P_READING_FILE);
            player->getSock()->setState(CON_PLAYING);
            return;
        }
    case CON_CHANGE_PASSWORD_GET_NEW:
        if(!isValidPassword(sock, str)) {
            sock->print("%c%c%c\n\r", 255, 252, 1);
            sock->print("Aborting.\n");
            player->clearFlag(P_READING_FILE);
            player->getSock()->setState(CON_PLAYING);
            return;
        } else {
            strncpy(sock->tempstr[1], str.c_str(), 255);
            sock->tempstr[1][255] = '\0';
            sock->print("%c%c%c\n\r", 255, 251, 1);
            sock->print("Re-enter password: ");
            gServer->processOutput();
            sock->intrpt &= ~1;
            player->getSock()->setState(CON_CHANGE_PASSWORD_FINISH);
            return;
        }
        break;
    case CON_CHANGE_PASSWORD_FINISH:
        sock->print("%c%c%c\n\r", 255, 252, 1);
        if(str.equals(sock->tempstr[1])) {

            if(!player->isCt())
                logn("log.passwd", "(%s)\n   %s changed %s password. Old: %s New: %s\n", player->getSock()->getHostname().c_str(),
                     player->getCName(), player->hisHer(), player->getPassword().c_str(), sock->tempstr[1]);
            player->setPassword(str);

            sock->print("Password changed.\n");
            broadcast(isDm, "^g%s just changed %s password.", player->getCName(), player->hisHer());

            player->clearFlag(P_READING_FILE);
            player->setFlag(P_PASSWORD_CURRENT);
            player->save(true);
        } else {
            sock->print("Different passwords given.\n");
            sock->print("Aborting.\n");
            player->clearFlag(P_READING_FILE);
        }
        player->getSock()->setState(CON_PLAYING);
        return;
    }
}

