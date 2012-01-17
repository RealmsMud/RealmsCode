/*
 * stats.h
 *	 Header file for stats
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
#ifndef STAT_H_
#define STAT_H_

class Stat
{
public:
	Stat();
	virtual ~Stat();
	
	bool load(xmlNodePtr curNode);
	void save(xmlNodePtr parentNode, const char* statName) const;
	
	int increaseMax(int amt);
	int decreaseMax(int amt);
	int adjustMax(int amt);
	
	int increase(int amt, bool overMaxOk = false);
	int decrease(int amt);
	int adjust(int amt, bool overMaxOk = false);
	
	short getCur() const;
	short getMax() const;
	short getInitial() const;

	void addCur(short a);
	void addMax(short a);
	int setMax(short newMax, bool allowZero=false);
	int setCur(short newCur);
	void setInitial(short i);

	int restore(); // Set a stat to it's maximum value
protected:
	short	cur;
	short	max;
	short	tmpMax;
	short	initial;
};

#endif /*STAT_H_*/
