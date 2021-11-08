/*
 * attackTimer.h
 *   Header file for the timer class (Handling of sub second timers)
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

#ifndef TIMER_H_
#define TIMER_H_

#include <sys/time.h>

class Timer {
public:
    Timer();
    
private:
    struct timeval lastAttacked;
    int delay; // Delay between attacks
public:
    void update(int newDelay = 0);
    void modifyDelay(int amt);
    void setDelay(int newDelay);
    bool hasExpired() const;
    long getTimeLeft() const;
    int getDelay() const;
    time_t getLT() const;
};

#endif /*TIMER_H_*/
