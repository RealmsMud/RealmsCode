/*
 * money.h
 *   Money
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

#ifndef _MONEY_H
#define	_MONEY_H


class Money {
public:
	Money();
	Money(unsigned long n, Coin c);
	void load(xmlNodePtr curNode);
	void save(const char* name, xmlNodePtr curNode) const;

	bool isZero() const;
	void zero();

	bool operator==(const Money& mn) const;
	bool operator!=(const Money& mn) const;
	unsigned long operator[](Coin c) const;
	unsigned long get(Coin c) const;

	void set(unsigned long n, Coin c);
	void add(unsigned long n, Coin c);
	void sub(unsigned long n, Coin c);
	void set(Money mn);
	void add(Money mn);
	void sub(Money mn);

	bstring str() const;

	static bstring coinNames(Coin c);
protected:
	unsigned long m[MAX_COINS+1];
};


#endif	/* _MONEY_H */

