/*
 * post.cpp
 *   Handles in-game post office.
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

#include <fcntl.h>                 // for open, O_APPEND, O_CREAT, O_RDWR
#include <fmt/format.h>            // for format
#include <unistd.h>                // for unlink, write, close
#include <cstdio>                  // for sprintf, fclose, feof, fgets, fopen
#include <cstring>                 // for strcpy, strlen, strcmp
#include <ctime>                   // for ctime, time
#include <string>                  // for string, allocator, basic_string

#include "cmd.hpp"                 // for cmd
#include "flags.hpp"               // for P_UNREAD_MAIL, P_READING_FILE, P_C...
#include "global.hpp"              // for DOPROMPT, FATAL, PROMPT, CreatureC...
#include "login.hpp"               // for CON_EDIT_HISTORY, CON_SENDING_MAIL
#include "mud.hpp"                 // for ACC
#include "mudObjects/players.hpp"  // for Player
#include "mudObjects/rooms.hpp"    // for BaseRoom
#include "paths.hpp"               // for Post, History
#include "proto.hpp"               // for up, broadcast, isCt, is
#include "server.hpp"              // for Server, gServer
#include "socket.hpp"              // for Socket
#include "xml.hpp"                 // for loadPlayer

//*********************************************************************
//                      hasNewMudmail
//*********************************************************************

void Player::hasNewMudmail() const {
    if(!flagIsSet(P_UNREAD_MAIL))
        return;

    if(fs::exists(Path::Post / getName() / ".txt"))
        print("\n*** You have new mudmail in the post office.\n");
}

//*********************************************************************
//                      canPost
//*********************************************************************

bool canPost(std::shared_ptr<Player> player) {
    if(!player->isStaff()) {
        if( !player->getRoomParent()->flagIsSet(R_POST_OFFICE) &&
            !player->getRoomParent()->flagIsSet(R_LIMBO) &&
            !(player->getRoomParent()->flagIsSet(R_FAST_HEAL) && player->getRoomParent()->isPkSafe())
        ) {
            *player << "You cannot do that here.\nYou need to find a post office, or you need to be in a tick room that is pkill safe.\n";
            return(false);
        }
    }
    return(true);
}

//*********************************************************************
//                      postText
//*********************************************************************
// format text for writing to file

std::string postText(const std::string &str) {
    std::string outstr = "";

    if(Pueblo::is(str))
        outstr = " ";
    outstr += str;
    outstr = outstr.substr(0, 158);
    outstr += "\n";

    return(outstr);
}


//*********************************************************************
//                      cmdSendmail
//*********************************************************************
// This function allows a player to send a letter to another player if
// he/she is in a post office.

int cmdSendMail(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;

    bool    online=false;

    if(!canPost(player))
        return(0);

    if(cmnd->num < 2) {
        player->print("Mail whom?\n");
        return(0);
    }

    cmnd->str[1][0] = up(cmnd->str[1][0]);

    if(!Player::exists(cmnd->str[1])) {
        player->print("That player does not exist.\n");
        return(0);
    }

    target = gServer->findPlayer(cmnd->str[1]);
    if(!target) {
        if(!loadPlayer(cmnd->str[1], target)) {
            //player->print("Player does not exist.\n");
            return(0);
        }
    } else
        online = true;

    if(!target->isStaff() && player->getClass() == CreatureClass::BUILDER) {
        player->print("You may not mudmail players at this time.\n");
        return(0);
    }

    if(!player->isStaff() && target->getClass() == CreatureClass::BUILDER) {
        player->print("You may not mudmail that character at this time.\n");
        return(0);
    }


    if(!player->isDm() && !target->isDm())
        broadcast(isDm, "^g### %s is sending mudmail to %s.", player->getCName(), target->getCName());

    target->setFlag(P_UNREAD_MAIL);
    target->save(online);

    player->print("Enter your message now. Type '.' or '*' on a line by itself to finish or '\\' to\ncancel. Each line should be NO LONGER THAN 80 CHARACTERS.\n-: ");
    sprintf(player->getSock()->tempstr[0], "%s", cmnd->str[1]);

    std::error_code ec;
    fs::remove(Path::Post / (player->getName() + "_to_" + player->getSock()->tempstr[0] + ".txt"), ec );

    player->setFlag(P_READING_FILE);
    gServer->processOutput();
    player->getSock()->setState(CON_SENDING_MAIL);
    player->getSock()->intrpt &= ~1;

    if(online) {
        if( target != player &&
            !target->flagIsSet(P_NO_SHOW_MAIL) &&
            (   !player->isStaff() ||
                (   player->isCt() &&
                    (   !strcmp(cmnd->str[2], "-s") ||
                        target->isStaff()
                    )
                )
            )
        ) {
            target->printColor("^c### You are receiving new mudmail from %s.\n", player->getCName());
        }
    }

    return(DOPROMPT);
}

//*********************************************************************
//                      sendMail
//*********************************************************************
// Sends a mudmail from System to the target. Include a trailing \n yourself.

void sendMail(const std::string &target, const std::string &message) {
    std::shared_ptr<Player> player=nullptr;
    char    datestr[40];
    long    t=0;
    int     ff=0;
    bool    online=true;

    if(target.empty() || message.empty())
        return;

    player = gServer->findPlayer(target);
    if(!player) {
        if(!loadPlayer(target, player))
            return;
        online = false;
    }


    ff = open(((Path::Post / target).replace_extension("txt")).c_str(), O_CREAT | O_APPEND | O_RDWR, ACC);
    if(ff < 0)
        throw std::runtime_error("sendMail");

    time(&t);
    strcpy(datestr, (char *) ctime(&t));
    datestr[strlen(datestr) - 1] = 0;
    auto header = fmt::format("\n--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--\n\nMail from System ({}):\n\n", datestr);
    
    write(ff, header.c_str(), header.length());
    write(ff, message.data(), message.length());

    close(ff);

    
    player->setFlag(P_UNREAD_MAIL);
    player->save(online);

    if(online)
        player->printColor("^c### You have new mudmail.\n");

}


//*********************************************************************
//                      postedit
//*********************************************************************
// This function is called when a player is editing a message to send
// to another player.

void postedit(std::shared_ptr<Socket> sock, const std::string& str) {
    char    outcstr[158], datestr[40], filename[80], postfile[80];
    long    t=0;
    int     ff=0;
    FILE    *fp=nullptr;
    std::string outstr = "";

    std::shared_ptr<Player> ply = sock->getPlayer();
    
    // use a temp file while we're editting it
    sprintf(filename, "%s/%s_to_%s.txt", Path::Post.c_str(), ply->getCName(), sock->tempstr[0]);

    if((str[0] == '.' || str[0] == '*') && !str[1]) {
        // time to copy the temp file to the real file!
        fp = fopen(filename, "r");

        // BUGFIX: Make sure fp is valid!!!!
        if(!fp) {
            ply->clearFlag(P_READING_FILE);
            sock->print("Error sending mail.\n");
            sock->restoreState();
            return;
        }

        sprintf(postfile, "%s/%s.txt", Path::Post.c_str(), sock->tempstr[0]);
        ff = open(postfile, O_CREAT | O_APPEND | O_RDWR, ACC);
        if(ff < 0)
            throw std::runtime_error("postEdit");

        time(&t);
        strcpy(datestr, (char *) ctime(&t));
        datestr[strlen(datestr) - 1] = 0;
        sprintf(outcstr, "\n--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--\n\nMail from %s (%s):\n\n",
            ply->getCName(), datestr);
        write(ff, outcstr, strlen(outcstr));

        while(!feof(fp)) {
            fgets(outcstr, sizeof(outcstr), fp);
            write(ff, outcstr, strlen(outcstr));
            strcpy(outcstr, "");
        }

        close(ff);
        fclose(fp);
        unlink(filename);

        ply->clearFlag(P_READING_FILE);
        sock->print("Mail sent.\n");
        sock->restoreState();
        return;
    }

    if(str[0] == '\\' && !str[1]) {
        unlink(filename);
        ply->clearFlag(P_READING_FILE);
        sock->print("Mail cancelled.\n");
        sock->restoreState();
        return;
    }

    ff = open(filename, O_CREAT | O_APPEND | O_RDWR, ACC);
    if(ff < 0)
        throw std::runtime_error("postEdit");

    outstr = postText(str);
    write(ff, outstr.c_str(), outstr.length());
    close(ff);
    sock->print("-: ");

    gServer->processOutput();
    sock->intrpt &= ~1;
}

//*********************************************************************
//                      cmdReadMail
//*********************************************************************
// This function allows a player to read their mail.

int cmdReadMail(const std::shared_ptr<Player>& player, cmd* cmnd) {

    if(!canPost(player))
        return(0);

    player->clearFlag(P_UNREAD_MAIL);
    const auto filename = (Path::Post / player->getName()).replace_extension("txt");

    if(!fs::exists(filename)) {
        player->print("You have no mail.\n");
        return(0);
    }

    player->getSock()->viewFile(filename, true);
    return(DOPROMPT);
}

//*********************************************************************
//                      dmReadmail
//*********************************************************************
// This function allows a DM to read other peoples' mudmail.

int dmReadmail(const std::shared_ptr<Player>& player, cmd* cmnd) {
    cmnd->str[1][0] = up(cmnd->str[1][0]);

    if(!Player::exists(cmnd->str[1])) {
        player->print("That player does not exist.\n");
        return(0);
    }

    const auto filename = (Path::Post / cmnd->str[1]).replace_extension("txt");
    if(!fs::exists(filename)) {
        player->print("%s currently has no mudmail.\n", cmnd->str[1]);
        return(PROMPT);
    }

    player->print("%s's current mudmail:\n", cmnd->str[1]);
    player->getSock()->viewFile(filename, true);
    return(DOPROMPT);
}


//*********************************************************************
//                      cmdDeleteMail
//*********************************************************************
// This function allows a player to delete their mail.

int cmdDeleteMail(const std::shared_ptr<Player>& player, cmd* cmnd) {
    if(!canPost(player))
        return(0);
    std::error_code ec;
    fs::remove((Path::Post / player->getName()).replace_extension("txt"), ec);
    player->print("Mail deleted.\n");

    player->clearFlag(P_UNREAD_MAIL);

    return(0);
}

//*********************************************************************
//                      dmDeletemail
//*********************************************************************
// This function allows a DM to delete a player's mail file.

int dmDeletemail(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> creature=nullptr;

    cmnd->str[1][0] = up(cmnd->str[1][0]);

    if(!Player::exists(cmnd->str[1])) {
        player->print("That player does not exist.\n");
        return(0);
    }

    const auto filename = (Path::Post / cmnd->str[1]).replace_extension("txt");

    if(!fs::exists(filename)) {
        player->print("%s has no mail file to delete.\n", cmnd->str[1]);
        return(PROMPT);
    }
    std::error_code ec;
    fs::remove(filename, ec);
    player->print("%s's mail deleted.\n", cmnd->str[1]);

    creature = gServer->findPlayer(cmnd->str[1]);

    if(!creature) {
        if(!loadPlayer(cmnd->str[1], creature)) {
            //player->print("Player does not exist.\n");
            return(0);
        }
    }

    creature->clearFlag(P_UNREAD_MAIL);
    creature->save(true);

    return(0);
}


//*********************************************************************
//                      cmdEditHistory
//*********************************************************************
// This function allows a player to define his/her own character history
// for roleplaying purposes. -- TC

int cmdEditHistory(const std::shared_ptr<Player>& player, cmd* cmnd) {
    char    file[80];
    int     ff=0;


    if(!player->getRoomParent()->isPkSafe() && !player->isStaff()) {
        player->print("You must be in a no pkill room in order to write your history.\n");
        return(0);
    }

    sprintf(file, "%s/%s.txt", Path::History.c_str(), player->getCName());

    ff = open(file, O_RDONLY, 0);
    close(ff);

    if(fs::exists(file)) {
        player->print("%s's history so far:\n\n", player->getCName());
        player->getSock()->viewFile(file);
        player->print("\n\n");
    }

    broadcast(isCt, "^y### %s is editing %s character history.", player->getCName(), player->hisHer());

    player->print("You may append your character's history now.\nType '.' or '*' on a line by itself to finish.\nEach line should be NO LONGER THAN 80 CHARACTERS.\n-: ");

    strcpy(player->getSock()->tempstr[0], file);

    player->setFlag(P_READING_FILE);
    gServer->processOutput();
    player->getSock()->setState(CON_EDIT_HISTORY);
    player->getSock()->intrpt &= ~1;

    return(DOPROMPT);
}

//*********************************************************************
//                      histedit
//*********************************************************************

void histedit(std::shared_ptr<Socket> sock, const std::string& str) {
    std::string outstr = "";
    int     ff=0;

    if((str[0] == '.' || str[0] == '*') && !str[1]) {
        sock->getPlayer()->clearFlag(P_READING_FILE);
        sock->print("History appended.\n");
        sock->restoreState();
        return;
    }

    ff = open(sock->tempstr[0], O_CREAT | O_APPEND | O_RDWR, ACC);
    if(ff < 0)
        throw std::runtime_error("histEdit");

    outstr = postText(str);
    write(ff, outstr.c_str(), outstr.length());
    close(ff);
    sock->print("-: ");

    gServer->processOutput();
    sock->intrpt &= ~1;
}

//*********************************************************************
//                      cmdHistory
//*********************************************************************

int cmdHistory(const std::shared_ptr<Player>& player, cmd* cmnd) {
    bool    self = false;


    if(!player->getRoomParent()->isPkSafe() && !player->isStaff()) {
        player->print("You must be in a no pkill room in order to write your history.\n");
        return(0);
    }

    if(cmnd->num < 2)
        self = true;
    else {
        cmnd->str[1][0] = up(cmnd->str[1][0]);

        if(!Player::exists(cmnd->str[1])) {
            player->print("That player does not exist.\n");
            return(0);
        }
    }

    const auto filename = (Path::History / (self ? player->getName() : cmnd->str[1])).replace_extension("txt");

    if(!fs::exists(filename)) {
        if(self)
            player->print("You haven't written a character history.\n");
        else
            player->print("%s hasn't written a character history.\n", cmnd->str[1]);
        return(0);
    }

    if(self)
        player->print("Your history:\n\n");
    else
        player->print("Current History of %s:\n\n", cmnd->str[1]);

    player->getSock()->viewFile(filename, true);
    return(0);
}

//*********************************************************************
//                      cmdDeleteHistory
//*********************************************************************
// This function allows a player to delete their history.

int cmdDeleteHistory(const std::shared_ptr<Player>& player, cmd* cmnd) {

    const auto filename = (Path::History / player->getName()).replace_extension("txt");
    std::error_code ec;
    fs::remove(filename, ec);
    player->print("History deleted.\n");

    broadcast(isCt, "^y%s just deleted %s history.", player->getCName(), player->hisHer());
    return(0);
}

//*********************************************************************
//                      dmDeletemail
//*********************************************************************
// This function allows a DM to delete a player's mail file.

int dmDeletehist(const std::shared_ptr<Player>& player, cmd* cmnd) {
    cmnd->str[1][0] = up(cmnd->str[1][0]);

    if(!Player::exists(cmnd->str[1])) {
        player->print("That player does not exist.\n");
        return(0);
    }

    const auto filename = (Path::History / cmnd->str[1]).replace_extension("txt");

    if(!fs::exists(filename)) {
        player->print("%s has no history to delete.\n", cmnd->str[1]);
        return(PROMPT);
    }
    std::error_code ec;
    fs::remove(filename, ec);
    player->print("%s's history deleted.\n", cmnd->str[1]);

    return(0);
}

