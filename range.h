/*
 * range.h
 *	 CatRef range object
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _RANGE_H
#define	_RANGE_H


class Range {
public:
	Range();

	void	load(xmlNodePtr curNode);
	xmlNodePtr save(xmlNodePtr curNode, const char* childName, int pos=0) const;
	Range&	operator=(const Range& r);
	bool operator==(const Range& r) const;
	bool operator!=(const Range& r) const;
	bool	belongs(CatRef cr) const;
	bstring	str() const;
	bool	isArea(bstring c) const;

	CatRef	low;
	short	high;
};


#endif	/* _RANGE_H */

