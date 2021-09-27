/*
 * bans.cpp
 *  Various functions to ban a person from the mud
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

#include <cctype>                 // for isspace, isgraph, isdigit
#include <cstdio>                 // for sprintf
#include <cstdlib>                // for atoi, free
#include <cstring>                // for strlen, strcpy, strstr, strcat, memcpy
#include <strings.h>              // for strncasecmp
#include <ctime>                  // for time, ctime

#include "bans.hpp"               // for Ban
#include "bstring.hpp"            // for bstring
#include "cmd.hpp"                // for cmd
#include "config.hpp"             // for Config, gConfig
#include "creatures.hpp"          // for Player
#include "global.hpp"             // for CreatureClass, CreatureClass::BUILDER
#include "mud.hpp"                // for questions_to_email
#include "proto.hpp"              // for broadcast, isCt, log_immort, logn
#include "server.hpp"             // for Server, gServer, SocketList
#include "socket.hpp"             // for Socket
#include "utils.hpp"              // for MAX, MIN

Ban::Ban() {
    reset(); 
}

void Ban::reset() {
    site = ""; 
    by = ""; 
    time = ""; 
    reason = ""; 
    password = ""; 
    duration = 0;
    unbanTime = 0;
    isPrefix = isSuffix = false;    
}

bool Ban::matches(std::string_view toMatch) {

    if(site == "*")
        return(true);
    else if(isPrefix && isSuffix && toMatch.find(site) != std::string_view::npos)
        return(true);
    else if(isPrefix && toMatch.starts_with(site))
        return(true);
    else if(isSuffix && toMatch.ends_with(site))
        return(true);
    else if(!toMatch.compare(site))
        return(true);
    
    return(false);
}




int dmListbans(Player* player, cmd* cmnd) {
    int found=0;
    std::ostringstream banStr;
    
    banStr << "Current bans:\n";

    std::list<Ban*>::iterator it;
    Ban* ban;

    for(it = gConfig->bans.begin() ; it != gConfig->bans.end() ; it++) {
        found++;
        ban = (*it);

        banStr << "^c#" << found;

        banStr << "^xSite: ";
        if(ban->site[0] == '\0')
            banStr << "^CEveryone! ";
        else {
            banStr << "^C";
            if(ban->isSuffix)
                banStr << "*";
            banStr << ban->site;
            if(ban->isPrefix)
                banStr << "*";
        }
        banStr << "^XDuration: ";
        if(ban->duration <= 0)
            banStr << "Indefinite ";
        else
            banStr << ban->duration << " Day(s) ";

        banStr << "^wBanned by: ";
        banStr << "^c" << ban->by << "\n";

        banStr << "    ^wBan Time: ";
        banStr << "^c" << ban->time << " ";

        banStr <<  "^wExpires: ";
        if(ban->unbanTime)
            banStr << "^g" << ctime(&ban->unbanTime);
        else
            banStr << "^gNever!\n";

        banStr << "    ^wPassword: ";
        if(ban->password.empty())
            banStr << "None. ";
        else {
            banStr << "^c" << ban->password << " ";
        }

        banStr << "^wBan Reason: ";

        banStr << "^c" << ban->reason << "^x\n\n";
    }
    bstring toPrint = banStr.str();
    player->printColor("%s", toPrint.c_str());
    
    if(!found)
        player->print("No bans!\n");
    else
        player->print("%d Ban(s) found\n", found);
    return(0);
}

int dmBan(Player* player, cmd* cmnd) {
    Player* target=nullptr;
    size_t  i=0, j=0, strLen, len;
    int     dur, iTmp = 0;
    int     isPrefix = 0, isSuffix = 0;
    char    *pass=nullptr;
    char    log[128], log2[128], log3[128], log4[128];
    Ban*    newBan;
    char    str[255], who[255], site[255], comment[255];
    long    t;

    strcpy(log,"");
    strcpy(log2,"");
    strcpy(log3,"");
    strcpy(log4,"");

    strLen = cmnd->fullstr.length();
    // This kills all leading whitespace
    while(i<strLen && isspace(cmnd->fullstr[i]))
        i++;

    // This kills the command itself
    while(i<strLen && !isspace(cmnd->fullstr[i]))
        i++;

    // This kills all the white space before the who
    while(i<strLen && isspace(cmnd->fullstr[i]))
        i++;

    len = strlen(&cmnd->fullstr[i]);
    if(!len) {
        dmListbans(player, nullptr);
        //player->print("Syntax: *ban <player/site> [dur] [comment] [-p [password]]\n");
        return(0);
    }
    // This finds out how long the who is
    while(i+j < strLen && isgraph(cmnd->fullstr[i+j]))
        j++;

    memcpy(who, &cmnd->fullstr[i], j);
    who[j] = '\0';
    strcpy(site, who);
    lowercize(who, 1);
    target = gServer->findPlayer(who);
    if(!target || target == player) {
        if((!strstr(site, "*") && !strstr(site, ".")) || !player->isDm()) {
            player->print("%s is not on.\n", who);
            return(0);
        }
    }
    i = i+j;
    j=0;

    len = strlen(&cmnd->fullstr[i+1]);
    if(!len)
        dur = 0;
    else {
        // See if we have a password
        pass = strstr(&cmnd->fullstr[i], "-p ");
        if(pass)    { // There is a password string then
            iTmp = i;
            while(iTmp < strLen && &cmnd->fullstr[iTmp] != pass)
                iTmp++;
            pass += 3;
            while(isspace(*pass))
                pass++;
            // Kill trailing whitespace
            len = strlen(pass);
            while(isspace(pass[len-1]))
                len--;
            pass[len] = '\0';
        }
        if(iTmp) { // We have a password, everything else stops at the beginging of the -p
            strLen = iTmp;
            cmnd->fullstr[iTmp] = '\0';
        }
        // Kill white space
        while(i < strLen && isspace(cmnd->fullstr[i]))
            i++;

        // Find length of the str
        while(i+j < strLen && isgraph(cmnd->fullstr[i+j]))
            j++;

        memcpy(str, &cmnd->fullstr[i], j);
        str[j] = '\0';
        // If its a digit, assume its the length of the ban, otherwise no length
        // was specified, assume indefinite, and use the rest of the str as the
        // comment.
        if(isdigit(str[0])) {
            dur = atoi(str);
            // Anything less than 0 becomes 0 (indefinite) anything greater than
            // 1825 (5 years) becomes 1825
            dur = MAX(MIN(dur, 1825), 0);
            i=i+j;
            // Kill all whitespace before the comment
            while(i < strLen && isspace(cmnd->fullstr[i]))
                i++;
        } else
            dur = 0;
    }

    strcpy(comment, &cmnd->fullstr[i]);
    // Kill trailling whitespace on comment
    len = strlen(comment);
    while(isspace(comment[len-1]))
        len--;
    comment[len] = '\0';
    if(target) {
        if(target->getClass() > CreatureClass::BUILDER) {
            player->print("You can't ban staff! Dumbass.\n");
            target->printColor("^r%s tried to siteban you! Kill %s!\n", player->getCName(), player->himHer());
            return(0);
        }
    }
    if(!strlen(comment) && !player->isDm()) {
        player->print("You must give a reason for banning someone.\n");
        return(0);
    }
    if(target)
        strncpy(site, target->getSock()->getHostname().data(), target->getSock()->getHostname().length());

    if(strlen(site) > 1) {
        if(site[0] == '*') {
            isSuffix = 1;
            memmove(&site[0], &site[1], strlen(site));
        }
        if(site[strlen(site) - 1] == '*') {
            isPrefix = 1;
            site[strlen(site) - 1] = '\0';
        }
    }
    if(gConfig->isBanned(site)) {
        player->print("The address '%s' is already banned.\n", site);
        return(0);
    }

    newBan = new Ban;

    // The site of the ban
    newBan->site = site;
    // Prefix or Suffix?
    if(isPrefix == 1)
        newBan->isPrefix = true;
    if(isSuffix == 1)
        newBan->isSuffix = true;

    // Who set the ban
    newBan->by = player->getName();
    // Set the ban duration
    newBan->duration = dur;
    // Set the ban reason
    if(strlen(comment))
        newBan->reason = comment;
    else
        newBan->reason = "No reason given";

    // Set the ban time
    t = time(nullptr);
    char *timeStr = strdup((char *)ctime(&t));
    timeStr[strlen(timeStr)-1] = '\0';
    // Get rid of the '\n'
    newBan->time = timeStr;
    
    free(timeStr);
    
    if(newBan->duration > 0)// NOW  +  # of days * # of seconds in a day
        newBan->unbanTime = time(nullptr) + ((long)newBan->duration * 60*60*24L);

    // Set the password
    if(pass)
        newBan->password = pass;

    gConfig->addBan(newBan);
    if(target)
        sprintf(log, "%s (%s) has been banned", who, newBan->site.c_str());
    else
        sprintf(log, "'%s' has been banned", site);

    if(dur > 0)
        sprintf(log2, " for %d day(s) by %s.", newBan->duration, player->getCName());
    else
        sprintf(log2, " indefinitely.");

    if(!newBan->reason.empty())
        sprintf(log3, " Reason: '%s' by %s.", newBan->reason.c_str(), player->getCName());


    if(!newBan->password.empty())
        sprintf(log4, " Password: '%s'.\n", newBan->password.c_str());
    else
        sprintf(log4, "\n");

    strcat(log, log2);
    strcat(log, log3);
    strcat(log, log4);

    log_immort(true, player, "%s", log);
    logn("log.bans", "%s: %s", player->getCName(), log);

    gConfig->saveBans();
    
    if(target) {
        char tmpBuf[1024];
        sprintf(tmpBuf, "\n\rThe watcher just arrived.\n\rThe watcher says, \"Begone from this place!\".\n\rThe watcher banishes your soul from this world.\n\r\n\r\n\r");
        target->getSock()->write(tmpBuf);
        target->getSock()->disconnect();
    } else { // Determine who's effected by the siteban and drop them *excluding staff*
        gServer->checkBans();
    }
    return(0);
}

void Server::checkBans() {
    static const char* banString = "\n\rThe watcher just arrived.\n\rThe watcher says, \"Begone from this place!\".\n\rThe watcher banishes your soul from this world.\n\r\n\r\n\r";

    for(auto sock : sockets) {
        if(sock.getPlayer() && (gConfig->isBanned(sock.getIp()) ||
            gConfig->isBanned(sock.getHostname()))) {
            if(sock.getPlayer()->getClass() <= CreatureClass::BUILDER) {
                sock.write(banString);
                sock.disconnect();
            }
        }
    }
}

int dmUnban(Player* player, cmd* cmnd) {
    int toDel = 0, i;

    // Kill any trailing white space in the fullstr
    i = cmnd->fullstr.length();
    while(isspace(cmnd->fullstr[i-1]))
        i--;
    cmnd->fullstr[i] = '\0';
    // We need a number with the command!
    if(!strncasecmp(cmnd->fullstr.c_str(), "*unban", i))
        toDel = -1;
    else
        toDel = cmnd->val[0];

    if(toDel <= 0) {
        player->print("What ban would you like lifted?\n");
        return(0);
    }

    if(gConfig->deleteBan(toDel)) {
        player->print("Ban #%d successfully deleted.\n", toDel);
        gConfig->saveBans();
    } else
        player->print("Ban #%d could not be deleted\n", toDel);

    return(0);
}
//*********************************************************************
// Misc commands to add new bans, free bans, delete bans and the like
//*********************************************************************

bool Config::addBan(Ban* toAdd) {
    bans.push_back(toAdd);
    return(true);   
}

bool Config::deleteBan(int toDel) {
    std::list<Ban*>::iterator it;
    int count=0;

    for(it = bans.begin() ; it != bans.end() ; it++) {
        count++;
        if(count == toDel) { // We've found the clan to delete, now remove it from the list
            Ban* ban = (*it);
            bans.erase(it);
            delete ban;
            return(true);
        }
    }
    return(false);
}

//***************************
// isLockedOut
//***************************

// This function determines if the player on socket number, fd, is on
// a site that is being locked out.  If the site is password locked,
// then 2 is returned.  If it's completely locked, 1 is returned.  If
// it's not locked out at all, 0 is returned.
int Config::isLockedOut( Socket* sock ) {
    bool match = false;
    int count=0;

    std::list<Ban*>::iterator it;
    Ban* ban = nullptr;

    for(it = bans.begin() ; it != bans.end() ; it++) {
        count++;
        ban = (*it);
        // Better safe than sorry
        if(!ban)
            continue;
        
        if(ban->matches(sock->getHostname()) || ban->matches(sock->getIp())) {
            match = true;
            break;
        }
    }
    if(!match)
        return(0);

    if(ban->unbanTime != 0 && time(nullptr) > ban->unbanTime) {
        // Ban has expired so delete it
        broadcast(isCt, "^y--- Expiring ban for '%s'", ban->site.c_str());
        deleteBan(count);
        saveBans();

        // See if they're effected by another ban
        return(isLockedOut(sock));
    }
    if(!ban->password.empty()) {
        strcpy(sock->tempstr[0], ban->password.c_str());
        return (2);
    } else {
        char buf[1024];
        sprintf(buf, "\n\rYour site is locked out.\n\rSend questions to %s.\n\r", questions_to_email);
        sock->write(buf);
        broadcast(isCt, "^yDenying access to '%s(%s)'", sock->getHostname(), ban->site.c_str());
        return(1);
    }
}

// Returns 1 if the site is on the ban list, 0 otherwise
bool Config::isBanned(std::string_view site) {
    std::list<Ban*>::iterator it;
    Ban* ban;

    for(it = bans.begin() ; it != bans.end() ; it++) {
        ban = (*it);
        // Better safe than sorry
        if(!ban)
            continue;
        if(ban->matches(site))
            return(true);
    }
    return (false);
}

// Clears bans
void Config::clearBanList() {
    Ban* ban;
    
    while(!bans.empty()) {
        ban = bans.front();
        delete ban;
        bans.pop_front();
    }
    bans.clear();
}

