/*
 * catRef.cpp
 *	 CatRef object
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
//						CatRef
//*********************************************************************

CatRef::CatRef() {
	clear();
	if(gConfig)
		setArea(gConfig->defaultArea);
	else
		setArea("misc");
}

//*********************************************************************
//						setDefault
//*********************************************************************

void CatRef::setDefault(const Creature* target) {
	clear();
	setArea(gConfig->defaultArea);

	if(target) {
		if(target->inUniqueRoom()) {
			setArea(target->getConstUniqueRoomParent()->info.area);
		} else if(target->inAreaRoom()) {
			const CatRefInfo* cri = gConfig->getCatRefInfo("area", target->getConstAreaRoomParent()->area->id, true);
			if(cri)
				setArea(cri->getArea());
		}
	}
}

//*********************************************************************
//						clear
//*********************************************************************

void CatRef::clear() {
	area = "";
	id = 0;
}

//*********************************************************************
//						operators
//*********************************************************************

CatRef& CatRef::operator=(const CatRef& cr) {
	area = cr.area;
	id = cr.id;
	return(*this);
}

bool CatRef::operator==(const CatRef& cr) const {
	return(area == cr.area && id == cr.id);
}
bool CatRef::operator!=(const CatRef& cr) const {
	return(!(*this == cr));
}

//*********************************************************************
//						rstr
//*********************************************************************

// version of string for reloading
bstring CatRef::rstr() const {
	std::ostringstream oStr;
	oStr << area << "." << id;
	return(oStr.str());
}

//*********************************************************************
//						str
//*********************************************************************

bstring CatRef::str(bstring current, char color) const {
	std::ostringstream oStr;
	// if we're in an area already, we can chop off some text because they
	// already know what the area is
	if(	id &&
		area != "" &&
		(current == "" || area != current)
	) {
		if(color)
			oStr << "^" << color << area << ":^x" << id;
		else
			oStr << id << " - " << area;
	} else
		oStr << id;
	return(oStr.str());
}

//*********************************************************************
//						setArea
//*********************************************************************

void CatRef::setArea(bstring c) {
	if(c.getLength() > 20)
		c = c.left(20);
	area = c.toLower();
}

//*********************************************************************
//						isArea
//*********************************************************************

bool CatRef::isArea(bstring c) const {
	return(area == c);
}

//*********************************************************************
//						load
//*********************************************************************

void CatRef::load(xmlNodePtr curNode) {
	xml::copyPropToBString(area, curNode, "Area");
	xml::copyToNum(id, curNode);
}

//*********************************************************************
//						save
//*********************************************************************

xmlNodePtr CatRef::save(xmlNodePtr curNode, const char* childName, bool saveNonZero, int pos) const {
	if(!saveNonZero && !id)
		return(NULL);
	curNode = xml::newNumChild(curNode, childName, id);
	xml::newProp(curNode, "Area", area.c_str());
	if(pos)
		xml::newNumProp(curNode, "Num", pos);
	return(curNode);
}
