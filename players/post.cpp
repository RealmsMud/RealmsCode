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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "commands.hpp"
#include "creatures.hpp"
#include "login.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "xml.hpp"

//*********************************************************************
//                      hasNewMudmail
//*********************************************************************

void Player::hasNewMudmail() const {
    char    filename[250];

    if(!flagIsSet(P_UNREAD_MAIL))
        return;

    sprintf(filename, "%s/%s.txt", Path::Post, getCName());
    if(file_exists(filename))
        print("\n*** You have new mudmail in the post office.\n");
}

//*********************************************************************
//                      canPost
//*********************************************************************

bool canPost(Player* player) {
    if(!player->isStaff()) {
        if( !player->getRoomParent()->flagIsSet(R_POST_OFFICE) &&
            !player->getRoomParent()->flagIsSet(R_LIMBO)
        ) {
            player->print("This is not a post office.\n");
            return(false);
        }
    }
    return(true);
}

//*********************************************************************
//                      postText
//*********************************************************************
// format text for writing to file

bstring postText(bstring str) {
    bstring outstr = "";

    if(Pueblo::is(str))
        outstr = " ";
    outstr += str;
    outstr = outstr.left(158);
    outstr += "\n";

    return(outstr);
}


//*********************************************************************
//                      cmdSendmail
//*********************************************************************
// This function allows a player to send a letter to another player if
// he/she is in a post office.

int cmdSendMail(Player* player, cmd* cmnd) {
    Player  *target=0;
    char    file[80];

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
        if(!loadPlayer(cmnd->str[1], &target)) {
            //player->print("Player does not exist.\n");
            return(0);
        }
    } else
        online = true;

    if(!target->isStaff() && player->getClass() == CreatureClass::BUILDER) {
        player->print("You may not mudmail players at this time.\n");
        if(!online)
            free_crt(target, false);
        return(0);
    }

    if(!player->isStaff() && target->getClass() == CreatureClass::BUILDER) {
        player->print("You may not mudmail that character at this time.\n");
        if(!online)
            free_crt(target, false);
        return(0);
    }

    if(target->isCt() && !player->isWatcher() && !player->isStaff() && !player->flagIsSet(P_CAN_MUDMAIL_STAFF)) {
        player->print("Please do not mudmail Caretakers and DMs directly.\n");
        player->print("You need to contact a watcher.\n");
        if(!online)
            free_crt(target, false);
        return(0);
    }

    if(!player->isDm() && !target->isDm())
        broadcast(isDm, "^g### %s is sending mudmail to %s.", player->getCName(), target->getCName());

    target->setFlag(P_UNREAD_MAIL);
    target->save(online);
    if(!online)
        free_crt(target, false);

    player->print("Enter your message now. Type '.' or '*' on a line by itself to finish or '\\' to\ncancel. Each line should be NO LONGER THAN 80 CHARACTERS.\n-: ");
    sprintf(player->getSock()->tempstr[0], "%s", cmnd->str[1]);

    sprintf(file, "%s/%s_to_%s.txt", Path::Post, player->getCName(), player->getSock()->tempstr[0]);
    unlink(file);

    player->setFlag(P_READING_FILE);
    gServer->processOutput();
    player->getSock()->setState(CON_SENDING_MAIL);
    player->getSock()->intrpt &= ~1;
//  player->getSock()->fn = postedit;
//  player->getSock()->fnparam = 1;

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

void sendMail(const bstring& target, const bstring& message) {
    Player  *player=0;
    char    outcstr[158], postfile[80], datestr[40];
    long    t=0;
    int     ff=0;
    bool    online=true;

    if(target == "" || message == "")
        return;

    player = gServer->findPlayer(target.c_str());
    if(!player) {
        if(!loadPlayer(target.c_str(), &player))
            return;
        online = false;
    }

    sprintf(postfile, "%s/%s.txt", Path::Post, target.c_str());
    ff = open(postfile, O_CREAT | O_APPEND | O_RDWR, ACC);
    if(ff < 0)
        merror("sendMail", FATAL);

    time(&t);
    strcpy(datestr, (char *) ctime(&t));
    datestr[strlen(datestr) - 1] = 0;
    sprintf(outcstr, "\n--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--\n\nMail from System (%s):\n\n",
        datestr);
    
    write(ff, outcstr, strlen(outcstr));
    write(ff, message.c_str(), message.getLength());

    close(ff);

    
    player->setFlag(P_UNREAD_MAIL);
    player->save(online);

    if(online)
        player->printColor("^c### You have new mudmail.\n");

    if(!online)
        free_crt(player);
}


//*********************************************************************
//                      postedit
//*********************************************************************
// This function is called when a player is editing a message to send
// to another player.

void postedit(Socket* sock, bstring str) {
    char    outcstr[158], datestr[40], filename[80], postfile[80];
    long    t=0;
    int     ff=0;
    FILE    *fp=0;
    bstring outstr = "";

    Player* ply = sock->getPlayer();
    
    // use a temp file while we're editting it
    sprintf(filename, "%s/%s_to_%s.txt", Path::Post, ply->getCName(), sock->tempstr[0]);

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

        sprintf(postfile, "%s/%s.txt", Path::Post, sock->tempstr[0]);
        ff = open(postfile, O_CREAT | O_APPEND | O_RDWR, ACC);
        if(ff < 0)
            merror("postedit", FATAL);

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
        merror("postedit", FATAL);

    outstr = postText(str);
    write(ff, outstr.c_str(), outstr.getLength());
    close(ff);
    sock->print("-: ");

    gServer->processOutput();
    sock->intrpt &= ~1;
}

//*********************************************************************
//                      cmdReadMail
//*********************************************************************
// This function allows a player to read their mail.

int cmdReadMail(Player* player, cmd* cmnd) {
    char    filename[80];

    if(!canPost(player))
        return(0);

    player->clearFlag(P_UNREAD_MAIL);
    sprintf(filename, "%s/%s.txt", Path::Post, player->getCName());

    if(open(filename, O_RDONLY, 0) < 0) {
        player->print("You have no mail.\n");
        return(0);
    }

    viewFile(player->getSock(), filename);
    return(DOPROMPT);
}

//*********************************************************************
//                      dmReadmail
//*********************************************************************
// This function allows a DM to read other peoples' mudmail.

int dmReadmail(Player* player, cmd* cmnd) {
    char    filename[80];

    cmnd->str[1][0] = up(cmnd->str[1][0]);

    if(!Player::exists(cmnd->str[1])) {
        player->print("That player does not exist.\n");
        return(0);
    }

    sprintf(filename, "%s/%s.txt", Path::Post, cmnd->str[1]);
    if(!file_exists(filename)) {
        player->print("%s currently has no mudmail.\n", cmnd->str[1]);
        return(PROMPT);
    }

    player->print("%s's current mudmail:\n", cmnd->str[1]);
    viewFile(player->getSock(), filename);
    return(DOPROMPT);
}


//*********************************************************************
//                      cmdDeleteMail
//*********************************************************************
// This function allows a player to delete their mail.

int cmdDeleteMail(Player* player, cmd* cmnd) {
    char    file[80];

    if(!canPost(player))
        return(0);

    sprintf(file, "%s/%s.txt", Path::Post, player->getCName());
    unlink(file);
    player->print("Mail deleted.\n");

    player->clearFlag(P_UNREAD_MAIL);

    return(0);
}

//*********************************************************************
//                      dmDeletemail
//*********************************************************************
// This function allows a DM to delete a player's mail file.

int dmDeletemail(Player* player, cmd* cmnd) {
    Player  *creature=0;
    char    str[50];

    cmnd->str[1][0] = up(cmnd->str[1][0]);

    if(!Player::exists(cmnd->str[1])) {
        player->print("That player does not exist.\n");
        return(0);
    }

    sprintf(str, "%s/%s.txt", Path::Post, cmnd->str[1]);

    if(!file_exists(str)) {
        player->print("%s has no mail file to delete.\n", cmnd->str[1]);
        return(PROMPT);
    }

    unlink(str);
    player->print("%s's mail deleted.\n", cmnd->str[1]);

    creature = gServer->findPlayer(cmnd->str[1]);

    if(!creature) {
        if(!loadPlayer(cmnd->str[1], &creature)) {
            //player->print("Player does not exist.\n");
            return(0);
        } else {
            creature->clearFlag(P_UNREAD_MAIL);
            creature->save();
            free_crt(creature);
        }
    } else {
        creature->clearFlag(P_UNREAD_MAIL);
        creature->save(true);
    }

    return(0);
}



//*********************************************************************
//                      notepad
//*********************************************************************

int notepad(Player* player, cmd* cmnd) {
    char    file[80];

    if(cmnd->num == 3 && 0 == strcmp(cmnd->str[2], "all")) {
        sprintf(file, "%s/all_pad.txt", Path::Post);
        cmnd->num = 2;
    } else {
        sprintf(file, "%s/%s_pad.txt", Path::Post, player->getCName());
    }

    if(cmnd->num == 2) {
        if(low(cmnd->str[1][0]) == 'a') {
            strcpy(player->getSock()->tempstr[0], file);
            player->print("Staff notepad:\n->");
            player->setFlag(P_READING_FILE);
            gServer->processOutput();
            player->getSock()->setState(CON_EDIT_NOTE);
            player->getSock()->intrpt &= ~1;
            //player->getSock()->fn = noteedit;
            //player->getSock()->fnparam = 1;
            return(DOPROMPT);
        } else if(low(cmnd->str[1][0]) == 'd') {
            unlink(file);
            player->print("Clearing your notepad.\n");
            return(PROMPT);
        } else if(low(cmnd->str[1][0]) == 'v') {
            viewFile(player->getSock(), file);
            return(DOPROMPT);
        } else {
            player->print("invalid option.\n");
            return(PROMPT);
        }
    } else {
        viewFile(player->getSock(), file);
        return(DOPROMPT);
    }
}

//*********************************************************************
//                      noteedit
//*********************************************************************

void noteedit(Socket* sock, bstring str) {
    char    tmpstr[40];
    int     ff=0;
    bstring outstr = "";
    Player* ply = sock->getPlayer();

    if(str[0] == '.') {
        ply->clearFlag(P_READING_FILE);
        ply->print("Message appended.\n");
        sock->restoreState();
        return;
    }

    ff = open(sock->tempstr[0], O_RDONLY, 0);
    if(ff < 0) {
        ff = open(sock->tempstr[0], O_CREAT | O_RDWR, ACC);
        sprintf(tmpstr, "            %s\n\n", "=== Staff Notepad ===");
        write(ff, tmpstr, strlen(tmpstr));
    }
    close(ff);

    ff = open(sock->tempstr[0], O_CREAT | O_APPEND | O_RDWR, ACC);
    if(ff < 0)
        merror("noteedit", FATAL);

    outstr = postText(str);
    write(ff, outstr.c_str(), outstr.getLength());
    close(ff);

    sock->print("->");

    gServer->processOutput();
    sock->intrpt &= ~1;
    return;
}


//*********************************************************************
//                      cmdEditHistory
//*********************************************************************
// This function allows a player to define his/her own character history
// for roleplaying purposes. -- TC

int cmdEditHistory(Player* player, cmd* cmnd) {
    char    file[80];
    int     ff=0;


    if(!player->getRoomParent()->isPkSafe() && !player->isStaff()) {
        player->print("You must be in a no pkill room in order to write your history.\n");
        return(0);
    }

    sprintf(file, "%s/%s.txt", Path::History, player->getCName());

    ff = open(file, O_RDONLY, 0);
    close(ff);

    if(file_exists(file)) {
        player->print("%s's history so far:\n\n", player->getCName());
        viewLoginFile(player->getSock(), file);
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

void histedit(Socket* sock, bstring str) {
    bstring outstr = "";
    int     ff=0;

    if((str[0] == '.' || str[0] == '*') && !str[1]) {
        sock->getPlayer()->clearFlag(P_READING_FILE);
        sock->print("History appended.\n");
        sock->restoreState();
        return;
    }

    ff = open(sock->tempstr[0], O_CREAT | O_APPEND | O_RDWR, ACC);
    if(ff < 0)
        merror("histedit", FATAL);

    outstr = postText(str);
    write(ff, outstr.c_str(), outstr.getLength());
    close(ff);
    sock->print("-: ");

    gServer->processOutput();
    sock->intrpt &= ~1;
}

//*********************************************************************
//                      cmdHistory
//*********************************************************************

int cmdHistory(Player* player, cmd* cmnd) {
    char    file[80];
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

    sprintf(file, "%s/%s.txt", Path::History, self ? player->getCName() : cmnd->str[1]);

    if(!file_exists(file)) {
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

    viewFile(player->getSock(), file);
    return(0);
}

//*********************************************************************
//                      cmdDeleteHistory
//*********************************************************************
// This function allows a player to delete their history.

int cmdDeleteHistory(Player* player, cmd* cmnd) {
    char    file[80];

    sprintf(file, "%s/%s.txt", Path::History, player->getCName());
    unlink(file);
    player->print("History deleted.\n");

    broadcast(isCt, "^y%s just deleted %s history.", player->getCName(), player->hisHer());
    return(0);
}

//*********************************************************************
//                      dmDeletemail
//*********************************************************************
// This function allows a DM to delete a player's mail file.

int dmDeletehist(Player* player, cmd* cmnd) {
    char    file[80];

    cmnd->str[1][0] = up(cmnd->str[1][0]);

    if(!Player::exists(cmnd->str[1])) {
        player->print("That player does not exist.\n");
        return(0);
    }

    sprintf(file, "%s/%s.txt", Path::History, cmnd->str[1]);

    if(!file_exists(file)) {
        player->print("%s has no history to delete.\n", cmnd->str[1]);
        return(PROMPT);
    }

    unlink(file);
    player->print("%s's history deleted.\n", cmnd->str[1]);

    return(0);
}

