/*
 * timer.cpp
 *	 Used to assist in a high precision cpu timer
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

#include <sys/time.h>
#include <stdio.h>

#include "serverTimer.h"

//*********************************************************************
//							timeZero
//*********************************************************************

void timeZero(struct timeval* toZero) {
	toZero->tv_sec = 0;
	toZero->tv_usec = 0;
}

//*********************************************************************
//							timeDiff
//*********************************************************************

void timeDiff(const struct timeval x, const struct timeval y, struct timeval* result) {

	timeZero(result);

	if(x.tv_sec < y.tv_sec)
		return;
	else if(x.tv_sec == y.tv_sec && x.tv_usec > y.tv_usec) {
		result->tv_sec = 0;
		result->tv_usec = x.tv_usec - y.tv_usec;
	} else {
		result->tv_sec = x.tv_sec - y.tv_sec;
		if(x.tv_usec < y.tv_usec) {
			result->tv_usec = x.tv_usec + 1000000 - y.tv_usec;
			result->tv_sec--;
		} else
			result->tv_usec = x.tv_usec - y.tv_usec;
	}
}

//*********************************************************************
//							reset
//*********************************************************************

void ServerTimer::reset() {
	timeZero(&timePassed);
	timeZero(&startTime);
	timeZero(&endTime);
	running = 0;
}

//*********************************************************************
//							start
//*********************************************************************

void ServerTimer::start() {
	reset();
	running = 1;
	gettimeofday(&startTime, 0);
}

//*********************************************************************
//							end
//*********************************************************************

void ServerTimer::end() {
	gettimeofday(&endTime, 0);
	timeDiff(endTime,startTime, &timePassed);
	running = 0;
}

//*********************************************************************
//							sleep
//*********************************************************************

void ServerTimer::sleep() {
	// Only sleep if we're not running!
	if(running == true)
		return;
	if(timePassed.tv_sec > 0 || timePassed.tv_usec > 100000) {
		//printf("Not sleeping, Time passed was %ld sec %ld usec\n", timePassed.tv_sec, timePassed.tv_usec);
		//Do nothing
	} else {
		struct timeval toSleep;
		timeZero(&toSleep);
		toSleep.tv_usec = 100000 - timePassed.tv_usec;
//		if(toSleep.tv_usec < 50000) {
//			printf("%ld sec %ld usec\n", timePassed.tv_sec, timePassed.tv_usec);
//			printf("Sleeping for %ld usec\n", toSleep.tv_usec);
//		}
		select(0,0,0,0,&toSleep);
	}
}
