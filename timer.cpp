/*
 * attackTimer.cpp
 *	 Source file for the attack timer class (Handling of sub second attack timers)
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *  
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

//#include "attackTimer.h"
#include "mud.h"

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
	setDelay(tMAX<long>(newDelay, left));
}

void Timer::setDelay(int newDelay) {
	delay = MAX(1, newDelay);
}
void Timer::modifyDelay(int amt) {
	delay = MAX(1, delay + amt);
}

bool Timer::hasExpired() const {
	return(getTimeLeft() == 0);
}
void timeDiff(const struct timeval x, const struct timeval y, struct timeval* result);

long Timer::getTimeLeft() const {
	struct timeval curTime, difference;
	long timePassed = 0;
	
	timerclear(&curTime);
	timerclear(&difference);
	
	gettimeofday(&curTime, 0);
	//timeDiff(endTime,startTime, &timePassed);
	timeDiff(curTime, lastAttacked, &difference);
	//timersub(&lastAttacked, &curTime, &difference);
	
	timePassed += MAX(0, difference.tv_sec)*10;
	timePassed += (long)((difference.tv_usec / 1000000.0)*10);

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
