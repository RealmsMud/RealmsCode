/*
 * carry.cpp
 *	 Carried objects file
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
#include "mud.h"

//*********************************************************************
//						Carry
//*********************************************************************

Carry::Carry() {
	numTrade = 0;
}

Carry& Carry::operator=(const Carry& cry) {
	info = cry.info;
	numTrade = cry.numTrade;
	return(*this);
}

bool Carry::operator==(const Carry& cry) const {
	return(info == cry.info && numTrade == cry.numTrade);
}
bool Carry::operator!=(const Carry& cry) const {
	return(!(*this == cry));
}
//*********************************************************************
//						load
//*********************************************************************

void Carry::load(xmlNodePtr curNode) {
	info.load(curNode);
	numTrade = xml::getIntProp(curNode, "NumTrade");
}

//*********************************************************************
//						save
//*********************************************************************

xmlNodePtr Carry::save(xmlNodePtr curNode, const char* childName, bool saveNonZero, int pos) const {
	curNode = info.save(curNode, childName, saveNonZero, pos);
	if(curNode && numTrade)
		xml::newNumProp(curNode, "NumTrade", numTrade);
	return(curNode);
}

//*********************************************************************
//						str
//*********************************************************************

bstring Carry::str(bstring current, char color) const {
	std::ostringstream oStr;
	oStr << info.str(current, color);
	if(numTrade)
		oStr << " (" << numTrade << ")";
	return(oStr.str());
}

