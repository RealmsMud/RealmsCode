/*
 * timer.h
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

#ifndef SERVERTIMER_H_
#define SERVERTIMER_H_

class ServerTimer {
protected:
	struct timeval timePassed; // How much time passed
	struct timeval startTime; // Time we started the timer
	struct timeval endTime;  // Time we finished the timer
	bool running; // Is the timer running? If so timePassed not valid
	void reset();
public:
	void start();
	void end();
	void sleep();
};


#endif /*SERVERTIMER_H_*/
