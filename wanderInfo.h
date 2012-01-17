/*
 * wanderInfo.h
 *	 Random monster wandering info
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercia4 - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _WANDERINFO_H
#define	_WANDERINFO_H


class WanderInfo {
public:
	WanderInfo();
	CatRef	getRandom() const;
	void	load(xmlNodePtr curNode);
	void	save(xmlNodePtr curNode) const;
	void	show(const Player* player, bstring area="") const;

	std::map<int, CatRef> random;

	short getTraffic() const;
	void setTraffic(short t);
protected:
	short	traffic;
};


#endif	/* _WANDERINFO_H */

