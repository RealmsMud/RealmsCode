/*
 * dice.h
 *   Dice
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

#ifndef _DICE_H
#define	_DICE_H


class Dice {
public:
	Dice();
	Dice(unsigned short n, unsigned short s, short p);
	bool operator==(const Dice& d) const;
	bool operator!=(const Dice& d) const;
	void clear();
	void load(xmlNodePtr curNode);
	void save(xmlNodePtr curNode, const char* name) const;

	int roll() const;
	int average() const;
	int low() const;
	int high() const;
	bstring str() const;

	double getMean() const;
	unsigned short getNumber() const;
	unsigned short getSides() const;
	short getPlus() const;

	void setMean(double m);
	void setNumber(unsigned short n);
	void setSides(unsigned short s);
	void setPlus(short p);
protected:
	double mean;	    // If set, replaces number/sides/plus
	unsigned short number;
	unsigned short sides;
	short plus;
};


#endif	/* _DICE_H */

