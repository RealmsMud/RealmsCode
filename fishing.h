/*
 * fishing.h
 *	 Fishing
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

#ifndef _FISHING_H
#define	_FISHING_H


class FishingItem {
public:
	FishingItem();
	void	load(xmlNodePtr rootNode);

	CatRef getFish() const;
	bool isDayOnly() const;
	bool isNightOnly() const;
	short getWeight() const;	// as in: probability
	short getMinQuality() const;
	short getMinSkill() const;
	long getExp() const;

	bool isMonster() const;
	bool willAggro() const;
protected:
	CatRef fish;
	bool dayOnly;
	bool nightOnly;
	short weight;	// as in: probability
	short minQuality;
	short minSkill;
	long exp;

	bool monster;
	bool aggro;
};

class Fishing {
public:
	Fishing();
	void	load(xmlNodePtr rootNode);
	const FishingItem* getItem(short skill, short quality) const;
	bool empty() const;
	bstring id;
	std::list<FishingItem> items;
};


#endif	/* _FISHING_H */

