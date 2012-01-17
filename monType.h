/*
 * monType.h
 *	 Monster Type functions
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

#ifndef _MONTYPE_H
#define	_MONTYPE_H


namespace monType {
	bool isIntelligent(mType type);
	char *getName(mType type);
	int getHitdice(mType type);
	bool isUndead(mType type);
	Size size(mType type);
	bool immuneCriticals(mType type);
	bool noLivingVulnerabilities(mType type);
}


#endif	/* _MONTYPE_H */

