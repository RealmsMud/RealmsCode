/*
 * dice.cpp
 *   Dice
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
#include "mud.h"

//*********************************************************************
//						Dice
//*********************************************************************

Dice::Dice() {
	clear();
}
Dice::Dice(unsigned short n, unsigned short s, short p) {
	clear();
	setNumber(n);
	setSides(s);
	setPlus(p);
}

//*********************************************************************
//						clear
//*********************************************************************

void Dice::clear() {
	number = sides = plus = 0;
	mean = 0.0;
}

//*********************************************************************
//						operators
//*********************************************************************

bool Dice::operator==(const Dice& d) const {
	if(	number != d.getNumber() ||
		plus != d.getPlus() ||
		sides != d.getSides() ||
		mean != d.getMean()
	)
		return(false);
	return(true);
}

bool Dice::operator!=(const Dice& d) const {
	return(!(*this==d));
}

//*********************************************************************
//						load
//*********************************************************************

void Dice::load(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	clear();

	while(childNode) {
		if(NODE_NAME(childNode, "Number")) setNumber(xml::toNum<unsigned short>(childNode));
		else if(NODE_NAME(childNode, "Sides")) setSides(xml::toNum<unsigned short>(childNode));
		else if(NODE_NAME(childNode, "Plus")) setPlus(xml::toNum<short>(childNode));
		else if(NODE_NAME(childNode, "Mean")) setMean(xml::toNum<double>(childNode));

		childNode = childNode->next;
	}
}

//*********************************************************************
//						save
//*********************************************************************

void Dice::save(xmlNodePtr curNode, const char* name) const {
	if(!number && !sides && !plus && !mean)
		return;
	xmlNodePtr childNode = xml::newStringChild(curNode, name);

	xml::saveNonZeroNum(childNode, "Number", number);
	xml::saveNonZeroNum(childNode, "Sides", sides);
	xml::saveNonZeroNum(childNode, "Plus", plus);
	xml::saveNonZeroNum(childNode, "Mean", mean);
}

//*********************************************************************
//						roll
//*********************************************************************

int Dice::roll() const {
	if(mean)
		return(mrand(low(), high()));
	return(dice(number, sides, plus));
}

//*********************************************************************
//						average
//*********************************************************************

int Dice::average() const {
	if(mean)
		return((high() - low())/2 + low());
	return((number + number * sides) / 2 + plus);
}

//*********************************************************************
//						low
//*********************************************************************

int Dice::low() const {
	if(mean)
		return(MAX(1,mean*2/3));
	return(number + plus);
}

//*********************************************************************
//						high
//*********************************************************************

int Dice::high() const {
	if(mean)
		return(MAX(1,mean*1.35));
	return(number * sides + plus);
}

//*********************************************************************
//						str
//*********************************************************************

bstring Dice::str() const {
	std::ostringstream oStr;
	if(mean)
	{
		oStr << low() << "-" << high();
	}
	else
	{
		oStr << number << "d" << sides;
		if(plus)
			oStr << (plus > 0 ? "+" : "-") << plus;
	}
	return(oStr.str());
}

//*********************************************************************
//						getMean
//*********************************************************************

double Dice::getMean() const { return(mean); }

//*********************************************************************
//						getNumber
//*********************************************************************

unsigned short Dice::getNumber() const { return(number); }

//*********************************************************************
//						getSides
//*********************************************************************

unsigned short Dice::getSides() const { return(sides); }

//*********************************************************************
//						getPlus
//*********************************************************************

short Dice::getPlus() const { return(plus); }

//*********************************************************************
//						setMean
//*********************************************************************

void Dice::setMean(double m) { mean = m; }

//*********************************************************************
//						setNumber
//*********************************************************************

void Dice::setNumber(unsigned short n) { number = n; }

//*********************************************************************
//						setSides
//*********************************************************************

void Dice::setSides(unsigned short s) { sides = s; }

//*********************************************************************
//						setPlus
//*********************************************************************

void Dice::setPlus(short p) { plus = p; }
