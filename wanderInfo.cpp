/*
 * wanderInfo.cpp
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
#include "mud.h"

//*********************************************************************
//						WanderInfo
//*********************************************************************

WanderInfo::WanderInfo() {
	traffic = 0;
}

short WanderInfo::getTraffic() const { return(traffic); }
void WanderInfo::setTraffic(short t) { traffic = MAX(0, MIN(100, t)); }

//*********************************************************************
//						getRandom
//*********************************************************************
// This function used to be weighted - it would check 0-9 and if an entry didn't
// exist, no monster would come. If we pick a random entry and the list size is
// less than 10, then we have increased traffic.

CatRef WanderInfo::getRandom() const {
	std::map<int, CatRef>::const_iterator it;
	CatRef cr;
	int	i=0;

	if(!random.size())
		return(cr);

	// past the end
	it = random.end();
	// the last element
	it--;

	if(it == random.begin())
		return(cr);

	// pick a random element, adding weight if <9
	i = (*it).first;
	i = mrand(0, i < 9 ? 9 : i);

	it = random.find(i);
	if(it != random.end())
		cr = (*it).second;
	return(cr);
}

//*********************************************************************
//						load
//*********************************************************************

void WanderInfo::load(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
			 if(NODE_NAME(childNode, "Traffic")) xml::copyToNum(traffic, childNode);
		else if(NODE_NAME(childNode, "RandomMonsters")) {
			loadCatRefArray(childNode, random, "Mob", NUM_RANDOM_SLOTS);
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						save
//*********************************************************************

void WanderInfo::save(xmlNodePtr curNode) const {
	xml::saveNonZeroNum(curNode, "Traffic", traffic);
	saveCatRefArray(curNode, "RandomMonsters", "Mob", random, NUM_RANDOM_SLOTS);
}


//*********************************************************************
//						show
//*********************************************************************

void WanderInfo::show(const Player* player, bstring area) const {
	std::map<int, CatRef>::const_iterator it;
	Monster* monster=0;

	player->print("Traffic: %d%%\n", traffic);
	player->print("Random Monsters:\n");
	for(it = random.begin(); it != random.end() ; it++) {
		loadMonster((*it).second, &monster);

		player->printColor("^c%2d) ^x%14s ^c::^x %s\n", (*it).first+1,
			(*it).second.str("", 'c').c_str(), monster ? monster->getCName() : "");

		if(monster) {
			free_crt(monster);
			monster = 0;
		}
	}
}
