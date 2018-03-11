/*
 * lasttime.h
 *   Lasttime
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

#ifndef REALMSCODE_LASTTIME_H
#define REALMSCODE_LASTTIME_H

#include "catRef.hpp"

// Timed operation struct
typedef struct lasttime {
public:
    lasttime() { interval = ltime = misc = 0; };
    long        interval;
    long        ltime;
    short       misc;
} lasttime;

typedef struct crlasttime {
public:
    friend std::ostream& operator<<(std::ostream& out, const crlasttime& crl);
    crlasttime() { interval = ltime = 0; };
    long        interval;
    long        ltime;
    CatRef      cr;
} crlasttime;


#endif //REALMSCODE_LASTTIME_H
