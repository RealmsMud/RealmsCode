/*
 * bans.h
 *	 Header file for bans
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
#ifndef BANS_H_
#define BANS_H_


class Ban {
public:
	Ban();
	Ban(xmlNodePtr curNode);
	void reset();
	bool matches(const char* toMatch);
	
public: // for now
	bstring		site;
	int				duration;
	long			unbanTime;
	bstring		by;
	bstring		time;
	bstring		reason;
	bstring		password;
	bool			isPrefix;
	bool			isSuffix;
};


#endif /*BANS_H_*/
