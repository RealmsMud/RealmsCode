/*
 * attackTimer.cpp
 *   Source file for the attack timer class (Handling of sub second attack timers)
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

#include <algorithm>

#include "timer.hpp"
#include "global.hpp"

Timer::Timer() {
    delay = DEFAULT_WEAPON_DELAY;
    gettimeofday(&lastAttacked, 0);
}

void Timer::update(int newDelay) {
    long left = getTimeLeft();

    timerclear(&lastAttacked);
    gettimeofday(&lastAttacked, 0);

    // The new delay is either the parameter delay, or however much time was
    // left on the timer whichever was higher
    setDelay(std::max<long>(newDelay, left));
}

void Timer::setDelay(int newDelay) {
    delay = std::max(1, newDelay);
}
void Timer::modifyDelay(int amt) {
    delay = std::max(1, delay + amt);
}

bool Timer::hasExpired() const {
    return(getTimeLeft() == 0);
}
void timeDiff(const struct timeval& x, const struct timeval& y, struct timeval& result);

long Timer::getTimeLeft() const {
    struct timeval curTime, difference;
    long timePassed = 0;
    
    timerclear(&curTime);
    timerclear(&difference);
    
    gettimeofday(&curTime, 0);
    timeDiff(curTime, lastAttacked, difference);
    
    timePassed += std::max<long>(0, difference.tv_sec)*10;
    timePassed += (long)((difference.tv_usec / 100000.0));

    if(timePassed >= delay)
        return(0);
    else
        return(delay-timePassed);
}

int Timer::getDelay() const {
    return(delay);
}
time_t Timer::getLT() const {
    return(lastAttacked.tv_sec);
}
