/*
 * staff.cpp
 *   Functions dealing with staff.
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

#include <cstdarg>                   // for va_end, va_list, va_start
#include <cstdio>                    // for sprintf, vasprintf
#include <cstdlib>                   // for free
#include <ostream>                   // for operator<<, ostream
#include <string>                    // for string, operator==, allocator

#include "flags.hpp"                 // for P_EAVESDROPPER, P_LOG_WATCH, P_P...
#include "global.hpp"                // for CreatureClass, CreatureClass::BU...
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/players.hpp"    // for Player
#include "proto.hpp"                 // for broadcast, stripLineFeeds, logn
#include "socket.hpp"                // for Socket

// *********************************************************
// log_immort
// *********************************************************
// Parameters:  <broad> Broadcast or not?
//              <player> Who are we logging?

// Log something done by an immortal, and optionally broadcast it
int log_immort(int broad, Player* player, const char *fmt,...) {
    // broad==0 - no broadcast
    // broad==1 - broadcast
    // broad==2 - more needs to be done
    char    *str, name[42];
    va_list ap;
    std::string txt = "";

    if(player->isMonster() || !player->isStaff())
        return(0);

    va_start(ap, fmt);
    if(vasprintf(&str, fmt, ap) == -1) {
        std::clog << "Error in log_immort\n";
        return(0);
    }
    va_end(ap);

    txt = str;
    // broad is 2 when we need to remove the last ^g in the string.
    // this is the case when we broadcast an object name and manually
    // return the string to the proper color
    if(broad == 2) {
        std::string::size_type idx = txt.find_last_of('^');
        if(idx != std::string::npos && (txt.at(idx+1) == 'g' || txt.at(idx+1) == 'G'))
            txt.erase(idx, 2);
    }

    // trick logn to use Path::StaffLog
    sprintf(name, "staff/%s", player->getCName());  // Path::StaffLog
    logn(name, "%s\n", txt.c_str());

    if(broad) {
        if(player->isDm())
            broadcast(isDm, watchingLog, "^g*** %s", stripLineFeeds(str));
        else if(player->isCt())
            broadcast(isCt, watchingLog, "^g*** %s", stripLineFeeds(str));
        else
            broadcast(isStaff, watchingLog, "^g*** %s", stripLineFeeds(str));
    }

    free(str);
    return(1);
}


// *********************************************************
// CheckStaff
// *********************************************************
// Parameters:  <failStr> String to output on failure

// If player isn't staff print fail str and return false
// If player is staff print fail str but return true

bool Creature::checkStaff(const char *failStr,...) const {
    bool    ret=false;
    va_list ap;

    va_start(ap, failStr);
    if(!isStaff()) {
        ret = false;
    } else {
        print("*STAFF* ");
        ret = true;
    }
    vprint(failStr, ap);
    va_end(ap);
    return(ret);
}

bool isPtester(const Creature* player) {
    if(player->isMonster())
        return(false);
    return(player->isCt() || player->flagIsSet(P_PTESTER));
}
bool isPtester(Socket* sock) {
    if(sock->getPlayer())
        return(isPtester(sock->getPlayer()));
    return(false);
}
bool isWatcher(const Creature* player) {
    if(player->isMonster())
        return(false);
    return(player->isCt() || player->isWatcher());
}

bool isWatcher(Socket* sock) {
    if(sock->getPlayer())
        return(isWatcher(sock->getPlayer()));
    return(false);
}

bool isStaff(const Creature* player) {
    if(player->isMonster())
        return(false);
    return(player->getClass() >= CreatureClass::BUILDER);
}

bool isStaff(Socket* sock) {
    if(sock->getPlayer())
        return(sock->getPlayer()->isStaff());
    return(false);
}

bool isCt(const Creature* player) {
    if(player->isMonster())
        return(false);
    return(player->getClass() >= CreatureClass::CARETAKER);
}

bool isCt(Socket* sock) {
    if(sock->getPlayer())
        return(sock->getPlayer()->isCt());

    return(false);
}

bool isDm(const Creature* player) {
    if(player->isMonster())
        return(false);
    return(player->getClass() == CreatureClass::DUNGEONMASTER);
}

bool isDm(Socket* sock) {
    if(sock->getPlayer())
        return(sock->getPlayer()->isDm());
    return(false);
}

bool isAdm(const Creature* player) {
    if(player->isMonster())
        return(false);
    return(player->getName() == "Bane" || player->getName() == "Dominus" || player->getName() == "Ocelot");
}

bool isAdm(Socket* sock) {
    if(sock->getPlayer())
        return(isAdm(sock->getPlayer()));
    return(false);
}

bool watchingLog(Socket* sock) {
    if(sock->getPlayer())
        if(sock->getPlayer()->flagIsSet(P_LOG_WATCH))
            return(true);
    return(false);
}

bool watchingEaves(Socket* sock) {
    if(sock->getPlayer())
        if(isCt(sock) && sock->getPlayer()->flagIsSet(P_EAVESDROPPER))
            return(true);
    return(false);
}

bool watchingSuperEaves(Socket* sock) {
    if(sock->getPlayer())
        if(isCt(sock) && sock->getPlayer()->flagIsSet(P_SUPER_EAVESDROPPER))
            return(true);
    return(false);
}

