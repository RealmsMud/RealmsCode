/*
 * timer.cpp
 *   Used to assist in a high precision cpu timer
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

#include <sys/select.h>     // for select
#include <sys/time.h>       // for timeval, gettimeofday

#include "serverTimer.hpp"  // for ServerTimer

//*********************************************************************
//                          timeZero
//*********************************************************************

void timeZero(struct timeval& toZero) {
    toZero.tv_sec = 0;
    toZero.tv_usec = 0;
}

//*********************************************************************
//                          timeDiff
//*********************************************************************

void timeDiff(const struct timeval& x, const struct timeval& y, struct timeval& result) {

    timeZero(result);

    if(x.tv_sec < y.tv_sec)
        return;
    else if(x.tv_sec == y.tv_sec && x.tv_usec > y.tv_usec) {
        result.tv_sec = 0;
        result.tv_usec = x.tv_usec - y.tv_usec;
    } else {
        result.tv_sec = x.tv_sec - y.tv_sec;
        if(x.tv_usec < y.tv_usec) {
            result.tv_usec = x.tv_usec + 1000000 - y.tv_usec;
            result.tv_sec--;
        } else
            result.tv_usec = x.tv_usec - y.tv_usec;
    }
}

//*********************************************************************
//                          reset
//*********************************************************************

void ServerTimer::reset() {
    timeZero(timePassed);
    timeZero(startTime);
    timeZero(endTime);
    running = 0;
}

//*********************************************************************
//                          start
//*********************************************************************

void ServerTimer::start() {
    reset();
    running = 1;
    gettimeofday(&startTime, nullptr);
}

//*********************************************************************
//                          end
//*********************************************************************

void ServerTimer::end() {
    gettimeofday(&endTime, nullptr);
    timeDiff(endTime,startTime, timePassed);
    running = false;
}

//*********************************************************************
//                          sleep
//*********************************************************************

void ServerTimer::sleep() {
    // Only sleep if we're not running!
    if(running)
        return;
    if(timePassed.tv_sec > 0 || timePassed.tv_usec > 100000) {
        //Do nothing
    } else {
        struct timeval toSleep{};
        timeZero(toSleep);
        toSleep.tv_usec = 100000 - timePassed.tv_usec;
        select(0,nullptr,nullptr,nullptr,&toSleep);
    }
}
