/*
 * bans.h
 *   Header file for bans
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
#ifndef BANS_H_
#define BANS_H_

#include "common.h"

class Ban {
public:
    Ban();
    Ban(xmlNodePtr curNode);
    void reset();
    bool matches(const char* toMatch);
    
public: // for now
    bstring     site;
    int             duration;
    long            unbanTime;
    bstring     by;
    bstring     time;
    bstring     reason;
    bstring     password;
    bool            isPrefix;
    bool            isSuffix;
};


#endif /*BANS_H_*/
