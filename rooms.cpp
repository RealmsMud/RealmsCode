/*
 * rooms.cpp
 *	 Room routines.
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
#include "effects.h"
#include "move.h"
#include <sstream>


BaseRoom::BaseRoom() {
	first_ext = 0;
	first_obj = 0;
	first_mon = 0;
	first_ply = 0;

	tempNoKillDarkmetal = false;
	memset(misc, 0, sizeof(misc));
	hooks.setParent(this);
}


bstring BaseRoom::getVersion() const { return(version); }
void BaseRoom::setVersion(bstring v) { version = v; }



bstring BaseRoom::fullName() const {
	const UniqueRoom* uRoom = getConstUniqueRoom();
	const AreaRoom* aRoom = getConstAreaRoom();
	std::ostringstream oStr;

	if(uRoom) {
		oStr << uRoom->info.str() << "(" << uRoom->name << ")";
	} else if(aRoom) {
		oStr << aRoom->mapmarker.str();
	} else
		oStr << "invalid room";
	return(oStr.str());
}


//*********************************************************************
//						Room
//*********************************************************************

UniqueRoom::UniqueRoom() {
	memset(name, 0, sizeof(name));
	short_desc = "";
	long_desc = "";
	lowLevel = highLevel = maxmobs = trap = trapweight = trapstrength = 0;
	memset(flags, 0, sizeof(flags));
	roomExp = 0;
	size = NO_SIZE;
	beenhere = 0;
	memset(last_mod, 0, sizeof(last_mod));

	memset(lastModTime, 0, sizeof(lastModTime));

	memset(lastPly, 0, sizeof(lastPly));
	memset(lastPlyTime, 0, sizeof(lastPlyTime));
}

bstring UniqueRoom::getShortDescription() const { return(short_desc); }
bstring UniqueRoom::getLongDescription() const { return(long_desc); }
short UniqueRoom::getLowLevel() const { return(lowLevel); }
short UniqueRoom::getHighLevel() const { return(highLevel); }
short UniqueRoom::getMaxMobs() const { return(maxmobs); }
short UniqueRoom::getTrap() const { return(trap); }
CatRef UniqueRoom::getTrapExit() const { return(trapexit); }
short UniqueRoom::getTrapWeight() const { return(trapweight); }
short UniqueRoom::getTrapStrength() const { return(trapstrength); }
bstring UniqueRoom::getFaction() const { return(faction); }
long UniqueRoom::getBeenHere() const { return(beenhere); }
int UniqueRoom::getRoomExperience() const { return(roomExp); }
Size UniqueRoom::getSize() const { return(size); }

void UniqueRoom::setShortDescription(const bstring& desc) { short_desc = desc; }
void UniqueRoom::setLongDescription(const bstring& desc) { long_desc = desc; }
void UniqueRoom::appendShortDescription(const bstring& desc) { short_desc += desc; }
void UniqueRoom::appendLongDescription(const bstring& desc) { long_desc += desc; }
void UniqueRoom::setLowLevel(short lvl) { lowLevel = MAX(0, lvl); }
void UniqueRoom::setHighLevel(short lvl) { highLevel = MAX(0, lvl); }
void UniqueRoom::setMaxMobs(short m) { maxmobs = MAX(0, m); }
void UniqueRoom::setTrap(short t) { trap = t; }
void UniqueRoom::setTrapExit(CatRef t) { trapexit = t; }
void UniqueRoom::setTrapWeight(short weight) { trapweight = MAX(0, weight); }
void UniqueRoom::setTrapStrength(short strength) { trapstrength = MAX(0, strength); }
void UniqueRoom::setFaction(bstring f) { faction = f; }
void UniqueRoom::incBeenHere() { beenhere++; }
void UniqueRoom::setRoomExperience(int exp) { roomExp = MAX(0, exp); }
void UniqueRoom::setSize(Size s) { size = s; }

//*********************************************************************
//						deconstructors
//*********************************************************************

void BaseRoom::BaseDestroy() {
	xtag 	*xp = first_ext, *xtemp=0;
	while(xp) {
		if(xp->ext->flagIsSet(X_PORTAL)) {
			BaseRoom* target = xp->ext->target.loadRoom();
			// BUG: argument 3 is wrong, crash waiting to happen
			Move::deletePortal(target, xp->ext->getPassPhrase());
		}
		xtemp = xp->next_tag;
		delete xp->ext;
		delete xp;
		xp = xtemp;
	}
	first_ext = 0;

	otag	*op = first_obj, *otemp=0;
	while(op) {
		otemp = op->next_tag;
		delete op->obj;
		delete op;
		op = otemp;
	}
	first_obj = 0;

	ctag	*cp = first_mon, *ctemp=0;
	while(cp) {
		ctemp = cp->next_tag;
		free_crt(cp->crt);
		delete cp;
		cp = ctemp;
	}
	first_mon = 0;

	cp = first_ply;
	while(cp) {
		ctemp = cp->next_tag;
		delete cp;
		cp = ctemp;
	}
	first_ply = 0;

	effects.removeAll();
	removeEffectsIndex();
	gServer->removeDelayedActions(this);
}

UniqueRoom::~UniqueRoom() {
	BaseDestroy();
}

AreaRoom::~AreaRoom() {
	BaseDestroy();
	reset();
}

//*********************************************************************
//						reset
//*********************************************************************

void AreaRoom::reset() {
	xtag 	*xp = first_ext, *xtemp=0;

	area = 0;
	decCompass = needsCompass = stayInMemory = false;

	while(xp) {
		xtemp = xp->next_tag;
		delete xp->ext;
		delete xp;
		xp = xtemp;
	}
	first_ext = 0;
}

//*********************************************************************
//						AreaRoom
//*********************************************************************

AreaRoom::AreaRoom(Area *a, const MapMarker *m) {
	reset();

	area = a;
	if(m) {
		setMapMarker(m);
		recycle();
	}
}

Size AreaRoom::getSize() const { return(SIZE_COLOSSAL); }
bool AreaRoom::getNeedsCompass() const { return(needsCompass); }
bool AreaRoom::getDecCompass() const { return(decCompass); }
bool AreaRoom::getStayInMemory() const { return(stayInMemory); }

void AreaRoom::setNeedsCompass(bool need) { needsCompass = need; }
void AreaRoom::setDecCompass(bool dec) { decCompass = dec; }
void AreaRoom::setStayInMemory(bool stay) { stayInMemory = stay; }

//*********************************************************************
//						recycle
//*********************************************************************

void AreaRoom::recycle() {
	otag*   op = first_obj, *otemp=0;

	while(op) {
		otemp = op->next_tag;
		delete op->obj;
		delete op;
		op = otemp;
	}
	first_obj = 0;

	updateExits();
}

//*********************************************************************
//						setMapMarker
//*********************************************************************

void AreaRoom::setMapMarker(const MapMarker* m) {
	bstring str = mapmarker.str();
	area->rooms.erase(str);
	*&mapmarker = *m;
	str = mapmarker.str();
	area->rooms[str] = this;
}

//*********************************************************************
//						updateExits
//*********************************************************************

bool AreaRoom::updateExit(bstring dir) {
	if(dir == "north") {
		mapmarker.add(0, 1, 0);
		link_rom(this, &mapmarker, dir);
		mapmarker.add(0, -1, 0);

	} else if(dir == "east") {
		mapmarker.add(1, 0, 0);
		link_rom(this, &mapmarker, dir);
		mapmarker.add(-1, 0, 0);

	} else if(dir == "south") {
		mapmarker.add(0, -1, 0);
		link_rom(this, &mapmarker, dir);
		mapmarker.add(0, 1, 0);

	} else if(dir == "west") {
		mapmarker.add(-1, 0, 0);
		link_rom(this, &mapmarker, dir);
		mapmarker.add(1, 0, 0);

	} else if(dir == "northeast") {
		mapmarker.add(1, 1, 0);
		link_rom(this, &mapmarker, dir);
		mapmarker.add(-1, -1, 0);

	} else if(dir == "northwest") {
		mapmarker.add(-1, 1, 0);
		link_rom(this, &mapmarker, dir);
		mapmarker.add(1, -1, 0);

	} else if(dir == "southeast") {
		mapmarker.add(1, -1, 0);
		link_rom(this, &mapmarker, dir);
		mapmarker.add(-1, 1, 0);

	} else if(dir == "southwest") {
		mapmarker.add(-1, -1, 0);
		link_rom(this, &mapmarker, dir);
		mapmarker.add(1, 1, 0);

	} else
		return(false);
	return(true);
}

void AreaRoom::updateExits() {
	updateExit("north");
	updateExit("east");
	updateExit("south");
	updateExit("west");
	updateExit("northeast");
	updateExit("northwest");
	updateExit("southeast");
	updateExit("southwest");
}


//*********************************************************************
//						exitNameByOrder
//*********************************************************************

const char *exitNameByOrder(int i) {
	if(i==0) return("north");
	if(i==1) return("east");
	if(i==2) return("south");
	if(i==3) return("west");
	if(i==4) return("northeast");
	if(i==5) return("northwest");
	if(i==6) return("southeast");
	if(i==7) return("southwest");
	return("");
}

//*********************************************************************
//						canSave
//*********************************************************************

bool AreaRoom::canSave() const {
	xtag	*xp=0;
	int		i=0;

	if(unique.id)
		return(true);
	
	if(needsEffectsIndex())
		return(true);

	if(first_ext) {
		xp = first_ext;
		while(xp) {
			if(strcmp(xp->ext->name, exitNameByOrder(i++)))
				return(true);
			// doesnt check rooms leading to other area rooms
			if(xp->ext->target.room.id)
				return(true);
			xp = xp->next_tag;
		}
		if(i != 8)
			return(true);
	}

	return(false);
}


//*********************************************************************
//						getRandomWanderInfo
//*********************************************************************

WanderInfo* AreaRoom::getRandomWanderInfo() {
	std::list<AreaZone*>::iterator it;
	AreaZone *zone=0;
	TileInfo* tile=0;
	std::map<int, WanderInfo*> w;
	int i=0;

	// we want to randomly pick one of the zones
	for(it = area->zones.begin() ; it != area->zones.end() ; it++) {
		zone = (*it);
		if(zone->wander.getTraffic() && zone->inside(area, &mapmarker)) {
			w[i++] = &zone->wander;
		}
	}

	tile = area->getTile(area->getTerrain(0, &mapmarker, 0, 0, 0, true), area->getSeasonFlags(&mapmarker));
	if(tile && tile->wander.getTraffic())
		w[i++] = &tile->wander;

	if(!i)
		return(0);
	return(w[mrand(0,i-1)]);
}


//*********************************************************************
//						getWanderInfo
//*********************************************************************

WanderInfo* BaseRoom::getWanderInfo() {
	UniqueRoom*	uRoom = getUniqueRoom();
	if(uRoom)
		return(&uRoom->wander);
	AreaRoom* aRoom = getAreaRoom();
	if(aRoom)
		return(aRoom->getRandomWanderInfo());
	return(0);
}

//*********************************************************************
//						save
//*********************************************************************

void AreaRoom::save(Player* player) const {
	char			filename[256];

	sprintf(filename, "%s/%d/", Path::AreaRoom, area->id);
	Path::checkDirExists(filename);
	strcat(filename, mapmarker.filename().c_str());

	if(!canSave()) {
		if(file_exists(filename)) {
			if(player)
				player->print("Restoring this room to generic status.\n");
			unlink(filename);
		} else {
			if(player)
				player->print("There is no reason to save this room!\n\n");
		}
		return;
	}

	// record rooms saved during swap
	if(gConfig->swapIsInteresting(this))
		gConfig->swapLog((bstring)"a" + mapmarker.str(), false);

	xmlDocPtr	xmlDoc;
	xmlNodePtr		rootNode, curNode;

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "AreaRoom", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	unique.save(rootNode, "Unique", false);
	xml::saveNonZeroNum(rootNode, "NeedsCompass", needsCompass);
	xml::saveNonZeroNum(rootNode, "DecCompass", decCompass);
	effects.save(rootNode, "Effects");
	hooks.save(rootNode, "Hooks");

	curNode = xml::newStringChild(rootNode, "MapMarker");
	mapmarker.save(curNode);

	curNode = xml::newStringChild(rootNode, "Exits");
	saveExitsXml(curNode, first_ext);

	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);

	if(player)
		player->print("Room saved.\n");
}


//*********************************************************************
//						load
//*********************************************************************

void AreaRoom::load(xmlNodePtr rootNode) {
	xmlNodePtr childNode = rootNode->children;

	if(first_ext) {
		xtag	*prev=0, *xp = first_ext;

		while(xp) {
			prev = xp;
			xp = xp->next_tag;
			delete prev;
		}
		first_ext = 0;
	}

	while(childNode) {
		if(NODE_NAME(childNode, "Exits"))
			readExitsXml(childNode);
		else if(NODE_NAME(childNode, "MapMarker")) {
			mapmarker.load(childNode);
			area->rooms[mapmarker.str()] = this;
		}
		else if(NODE_NAME(childNode, "Unique")) unique.load(childNode);
		else if(NODE_NAME(childNode, "NeedsCompass")) xml::copyToBool(needsCompass, childNode);
		else if(NODE_NAME(childNode, "DecCompass")) xml::copyToBool(decCompass, childNode);
		else if(NODE_NAME(childNode, "Effects")) effects.load(childNode);
		else if(NODE_NAME(childNode, "Hooks")) hooks.load(childNode);

		childNode = childNode->next;
	}
	addEffectsIndex();
}

//*********************************************************************
//						canDelete
//*********************************************************************

bool AreaRoom::canDelete() {
	// don't delete unique rooms
	if(stayInMemory || unique.id)
		return(false);
	if(first_mon)
		return(false);
	if(first_ply)
		return(false);
	if(first_obj) {
		otag	*op = first_obj;
		while(op) {
			if(!op->obj->flagIsSet(O_DISPOSABLE))
				return(false);
			op = op->next_tag;
		}
	}
	// any room effects?
	if(needsEffectsIndex())
		return(false);
	if(first_ext) {
		int		i=0;
		xtag	*xp = first_ext;
		while(xp) {
			if(strcmp(xp->ext->name, exitNameByOrder(i++)))
				return(false);
			// doesnt check rooms leading to other area rooms
			if(xp->ext->target.room.id)
				return(false);
			xp = xp->next_tag;
		}
		if(i != 8)
			return(false);
	}
	return(true);
}

//*********************************************************************
//					isInteresting
//*********************************************************************
// This will determine whether the looker will see anything of
// interest in the room. This is used by cmdScout.

bool AreaRoom::isInteresting(const Player *viewer) const {
	ctag	*cp=0;
	xtag	*xp=0;
	int		i=0;

	if(unique.id)
		return(true);

	cp = first_ply;
	while(cp) {
		if(!cp->crt->flagIsSet(P_HIDDEN) && viewer->canSee(cp->crt))
			return(true);
		cp = cp->next_tag;
	}

	cp = first_mon;
	while(cp) {
		if(!cp->crt->flagIsSet(M_HIDDEN) && viewer->canSee(cp->crt))
			return(true);
		cp = cp->next_tag;
	}

	xp = first_ext;
	for(i=0; i<8; i++) {
		if(!xp)
			return(true);
		if(strcmp(xp->ext->name, exitNameByOrder(i)))
			return(true);
		xp = xp->next_tag;
	}

	// check out the remaining exits
	while(xp) {
		if(viewer->showExit(xp->ext))
			return(true);
		xp = xp->next_tag;
	}

	return(false);
}


bool AreaRoom::flagIsSet(int flag) const {
	MapMarker m = mapmarker;
	return(area->flagIsSet(flag, &m));
}

bool UniqueRoom::flagIsSet(int flag) const {
	return(flags[flag/8] & 1<<(flag%8));
}
void UniqueRoom::setFlag(int flag) {
	flags[flag/8] |= 1<<(flag%8);
}
void UniqueRoom::clearFlag(int flag) {
	flags[flag/8] &= ~(1<<(flag%8));
}
bool UniqueRoom::toggleFlag(int flag) {
	if(flagIsSet(flag))
		clearFlag(flag);
	else
		setFlag(flag);
	return(flagIsSet(flag));
}


//*********************************************************************
//						isMagicDark
//*********************************************************************

bool BaseRoom::isMagicDark() const {
	ctag	*cp=0;
	otag	*op=0;

	if(flagIsSet(R_MAGIC_DARKNESS))
		return(true);

	// check for darkness spell
	cp = first_ply;
	while(cp) {
		// darkness spell on staff does nothing
		if(cp->crt->isEffected("darkness") && !cp->crt->isStaff())
			return(true);
		if(cp->crt->flagIsSet(P_DARKNESS) && !cp->crt->flagIsSet(P_DM_INVIS))
			return(true);
		cp = cp->next_tag;
	}
	cp = first_mon;
	while(cp) {
		if(cp->crt->isEffected("darkness"))
			return(true);
		if(cp->crt->flagIsSet(M_DARKNESS))
			return(true);
		cp = cp->next_tag;
	}

	op = first_obj;
	while(op) {
		if(op->obj->flagIsSet(O_DARKNESS))
			return(true);
		op = op->next_tag;
	}
	return(false);
}


//*********************************************************************
//						isNormalDark
//*********************************************************************

bool BaseRoom::isNormalDark() const {
	if(flagIsSet(R_DARK_ALWAYS))
		return(true);

	if(!isDay() && flagIsSet(R_DARK_AT_NIGHT))
		return(true);

	return(false);
}


void BaseRoom::addExit(Exit *ext) {
	xtag	*xp, *temp, *prev;

	xp = 0;
	xp = new xtag;
	if(!xp) merror("addExit", FATAL);

	xp->ext = ext;
    
    ext->setRoom(this);

	if(!first_ext) {
		first_ext = xp;
		return;
	}

	temp = first_ext;

	while(temp) {
		prev = temp;
		temp = temp->next_tag;
	}
	prev->next_tag = xp;
}

//
// This function checks the status of the exits in a room.  If any of
// the exits are closable or lockable, and the correct time interval
// has occurred since the last opening/unlocking, then the doors are
// re-shut/re-closed.
//
void BaseRoom::checkExits() {
	xtag	*xp=0;
	long	t = time(0);

	xp = first_ext;
	while(xp) {
		if(!xp->ext) {
			broadcast(isDm, "^G*** Room %s has an exit tag with a null exit!", this->fullName().c_str());
			xp = xp->next_tag;
			continue;
		}
		if(	xp->ext->flagIsSet(X_LOCKABLE) &&
			(xp->ext->ltime.ltime + xp->ext->ltime.interval) < t)
		{
			xp->ext->setFlag(X_LOCKED);
			xp->ext->setFlag(X_CLOSED);
		} else if(	xp->ext->flagIsSet(X_CLOSABLE) &&
					(xp->ext->ltime.ltime + xp->ext->ltime.interval) < t)
			xp->ext->setFlag(X_CLOSED);

		xp = xp->next_tag;
	}
}

//*********************************************************************
//						deityRestrict
//*********************************************************************
// if our particular flag is set, fail = 0
// else if ANY OTHER flags are set, fail = 1

bool BaseRoom::deityRestrict(const Creature* creature) const {

	// if no flags are set
	if(	!flagIsSet(R_ARAMON) &&
		!flagIsSet(R_CERIS) &&
		!flagIsSet(R_ENOCH) &&
		!flagIsSet(R_GRADIUS) &&
		!flagIsSet(R_KAMIRA) &&
		!flagIsSet(R_ARES) &&
		!flagIsSet(R_ARACHNUS) &&
		!flagIsSet(R_LINOTHAN) )
		return(false);

	// if the deity flag is set and they match, they pass
	if(	(flagIsSet(R_ARAMON) && creature->getDeity() == ARAMON) ||
		(flagIsSet(R_CERIS) && creature->getDeity() == CERIS) ||
		(flagIsSet(R_ENOCH) && creature->getDeity() == ENOCH) ||
		(flagIsSet(R_GRADIUS) && creature->getDeity() == GRADIUS) ||
		(flagIsSet(R_KAMIRA) && creature->getDeity() == KAMIRA) ||
		(flagIsSet(R_ARES) && creature->getDeity() == ARES) ||
		(flagIsSet(R_ARACHNUS) && creature->getDeity() == ARACHNUS) ||
		(flagIsSet(R_LINOTHAN) && creature->getDeity() == LINOTHAN) )
		return(false);

	return(true);
}



//*********************************************************************
//							maxCapacity
//*********************************************************************
// 0 means no limit

int BaseRoom::maxCapacity() const {
	if(flagIsSet(R_ONE_PERSON_ONLY))
		return(1);
	if(flagIsSet(R_TWO_PEOPLE_ONLY))
		return(2);
	if(flagIsSet(R_THREE_PEOPLE_ONLY))
		return(3);
	return(0);
}

//*********************************************************************
//							isFull
//*********************************************************************
// Can the player fit in the room?

bool BaseRoom::isFull() const {
	return(maxCapacity() && countVisPly() >= maxCapacity());
}

//*********************************************************************
//						countVisPly
//*********************************************************************
// This function counts the number of (non-DM-invisible) players in a
// room and returns that number.

int BaseRoom::countVisPly() const {
	ctag	*cp;
	int		num = 0;

	cp = first_ply;
	while(cp) {
		if(!cp->crt->flagIsSet(P_DM_INVIS))
			num++;
		cp = cp->next_tag;
	}

	return(num);
}


//*********************************************************************
//						countCrt
//*********************************************************************
// This function counts the number of creatures in a
// room and returns that number.

int BaseRoom::countCrt() const {
	ctag	*cp;
	int	num = 0;

	cp = first_mon;
	while(cp) {
		if(!cp->crt->isPet())
			num++;
		cp = cp->next_tag;
	}

	return(num);
}


//*********************************************************************
//						getMaxMobs
//*********************************************************************

int BaseRoom::getMaxMobs() const {
	const UniqueRoom* room = getConstUniqueRoom();
	if(!room)
		return(MAX_MOBS_IN_ROOM);
	return(room->getMaxMobs() ? room->getMaxMobs() : MAX_MOBS_IN_ROOM);
}

//*********************************************************************
//						wake
//*********************************************************************

void BaseRoom::wake(bstring str, bool noise) const {
	ctag *cp = first_ply;

	while(cp) {
		cp->crt->wake(str, noise);
		cp = cp->next_tag;
	}
}

//*********************************************************************
//						vampCanSleep
//*********************************************************************

bool BaseRoom::vampCanSleep(Socket* sock) const {
	// not at night
	if(!isDay()) {
		sock->print("Your thirst for blood keeps you from sleeping.\n");
		return(false);
	}
	// during the day, under certain circumstances
	if(isMagicDark())
		return(true);
	if(	flagIsSet(R_DARK_ALWAYS) ||
		flagIsSet(R_INDOORS) ||
		flagIsSet(R_LIMBO) ||
		flagIsSet(R_VAMPIRE_COVEN) ||
		flagIsSet(R_UNDERGROUND) )
		return(true);
	sock->print("The sunlight prevents you from sleeping.\n");
	return(false);
}


//*********************************************************************
//						isCombat
//*********************************************************************
// checks to see if there is any time of combat going on in this room

bool BaseRoom::isCombat() const {
	ctag	*cp=0;

	cp = first_ply;
	while(cp) {
		if(cp->crt->inCombat(true))
			return(true);
		cp = cp->next_tag;
	}

	cp = first_mon;
	while(cp) {
		if(cp->crt->inCombat(true))
			return(true);
		cp = cp->next_tag;
	}

	return(false);
}


//*********************************************************************
//						isUnderwater
//*********************************************************************

bool BaseRoom::isUnderwater() const {
	return(flagIsSet(R_UNDER_WATER_BONUS));
}


//*********************************************************************
//						isOutdoors
//*********************************************************************

bool BaseRoom::isOutdoors() const {
	return(!flagIsSet(R_INDOORS) && !flagIsSet(R_VAMPIRE_COVEN) && !flagIsSet(R_SHOP_STORAGE) && !flagIsSet(R_LIMBO));
}


//*********************************************************************
//						magicBonus
//*********************************************************************

bool BaseRoom::magicBonus() const {
	return(flagIsSet(R_MAGIC_BONUS));
}


//*********************************************************************
//						isForest
//*********************************************************************

bool BaseRoom::isForest() const {
	return(flagIsSet(R_FOREST_OR_JUNGLE));
}

//*********************************************************************
//						isWater
//*********************************************************************
// drowning in area rooms is different than drowning in unique water rooms,
// therefore we need a different isWater function

bool AreaRoom::isWater() const {
	return(area->isWater(mapmarker.getX(), mapmarker.getY(), mapmarker.getZ(), true));
}

//*********************************************************************
//						isRoad
//*********************************************************************

bool AreaRoom::isRoad() const {
	return(area->isRoad(mapmarker.getX(), mapmarker.getY(), mapmarker.getZ(), true));
}

//*********************************************************************
//						getUnique
//*********************************************************************
// creature is allowed to be null

CatRef AreaRoom::getUnique(Creature *creature, bool skipDec) {
	if(!needsCompass)
		return(unique);
	CatRef	cr;

	if(!creature)
		return(cr);

	bool pet = creature->isPet();
	Player*	player = creature->getPlayerMaster();

	if(	!player ||
		!player->ready[HELD-1] ||
		player->ready[HELD-1]->getShotsCur() < 1 ||
		!player->ready[HELD-1]->compass ||
		*player->ready[HELD-1]->compass != *&mapmarker
	)
		return(cr);

	// no need to decrement twice if their pet is going through
	if(!skipDec && decCompass && !pet) {
		player->ready[HELD-1]->decShotsCur();
		player->breakObject(player->ready[HELD-1], HELD);
	}
	return(unique);
}

bool BaseRoom::isDropDestroy() const {
	if(!flagIsSet(R_DESTROYS_ITEMS))
		return(false);
	const AreaRoom* aRoom = getConstAreaRoom();
	if(aRoom && aRoom->area->isRoad(aRoom->mapmarker.getX(), aRoom->mapmarker.getY(), aRoom->mapmarker.getZ(), true))
		return(false);
	return(true);
}
bool BaseRoom::hasRealmBonus(Realm realm) const {
	return(flagIsSet(R_ROOM_REALM_BONUS) && flagIsSet(getRealmRoomBonusFlag(realm)));
}
bool BaseRoom::hasOppositeRealmBonus(Realm realm) const {
	return(flagIsSet(R_OPPOSITE_REALM_BONUS) && flagIsSet(getRealmRoomBonusFlag(getOppositeRealm(realm ))));
}


Monster* BaseRoom::getTollkeeper() {
	ctag	*cp = first_mon;
	while(cp) {
		if(!cp->crt->isPet() && cp->crt->flagIsSet(M_TOLLKEEPER))
			return(cp->crt->getMonster());
		cp = cp->next_tag;
	}
	return(0);
}
MudObject* BaseRoom::findTarget(Creature* searcher, const cmd* cmnd, int num) {
	return(findTarget(searcher, cmnd->str[num], cmnd->val[num]));
}

MudObject* BaseRoom::findTarget(Creature* searcher, const bstring& name, const int num,  bool monFirst, bool firstAggro, bool exactMatch) {
	int match=0;
	return(findTarget(searcher, name, num, monFirst, firstAggro, exactMatch, match));
}

MudObject* BaseRoom::findTarget(Creature* searcher, const bstring& name, const int num,  bool monFirst, bool firstAggro, bool exactMatch, int& match) {
	MudObject* toReturn = 0;

	if((toReturn = findCreature(searcher, name, num, monFirst, firstAggro, exactMatch, match))) {
		return(toReturn);
	}
	// TODO: Search the room's objects
	return(toReturn);
}


// Wrapper for the real findCreature to support legacy callers
Creature* BaseRoom::findCreature(Creature* searcher, const cmd* cmnd, int num) {
	return(findCreature(searcher, cmnd->str[num], cmnd->val[num]));
}

Creature* BaseRoom::findCreaturePython(Creature* searcher, const bstring& name, bool monFirst, bool firstAggro, bool exactMatch ) {
	int ignored=0;
	int num = 1;

	bstring newName = name;
	newName.trim();
	unsigned int sLoc = newName.ReverseFind(" ");
	if(sLoc != bstring::npos) {
		num = newName.right(newName.length() - sLoc).toInt();
		if(num != 0) {
			newName = newName.left(sLoc);
		} else {
			num = 1;
		}
	}

	std::cout << "Looking for '" << newName << "' #" << num << "\n";

	return(findCreature(searcher, newName, num, monFirst, firstAggro, exactMatch, ignored));
}

// Wrapper for the real findCreature for callers that don't care about the value of match
Creature* BaseRoom::findCreature(Creature* searcher, const bstring& name, const int num, bool monFirst, bool firstAggro, bool exactMatch ) {
	int ignored=0;
	return(findCreature(searcher, name, num, monFirst, firstAggro, exactMatch, ignored));
}

// The real findCreature, will take and return the value of match as to allow for findTarget to find the 3rd thing named
// gold with a monster name goldfish, player named goldmine, and some gold in the room!
Creature* BaseRoom::findCreature(Creature* searcher, const bstring& name, const int num, bool monFirst, bool firstAggro, bool exactMatch, int& match) {

	if(!searcher || name.empty())
		return(NULL);

	Creature* target=0;
	ctag	*cp;
	//match=0,
	int		found=0;
	bool	firstSearchDone=false;

	// check for monsters first
	if(monFirst) {
		cp = first_mon;
	} else {
		cp = first_ply;
	}

	// Incase we start off with a null first ply/mon list
	if(!cp) {
		firstSearchDone = true;
		if(monFirst)
			cp = first_ply;
		else
			cp = first_mon;
	}

	while(cp) {
		target = cp->crt;
		cp = cp->next_tag;
		if(cp == NULL && firstSearchDone == false) {
			firstSearchDone = true;
			// We've run to the end of the first search, switch to the next list now
			if(!monFirst) {
				// checking for monster now
				cp = first_mon;
			} else {
				// checking for player now
				cp = first_ply;
			}
		}
		if(!target)
			continue;
		if(!searcher->canSee(target))
			continue;

		if(exactMatch) {
			if(!strcmp(target->name, name.c_str())) {
				match++;
				if(match == num)
					return(target);
			}

		} else {
			if(keyTxtEqual(target, name.c_str())) {
				match++;
				if(match == num) {
					found = 1;
					break;
				}
			}
		}

	}

	if(firstAggro && found) {
		if(num < 2 && searcher->pFlagIsSet(P_KILL_AGGROS))
			return(getFirstAggro(target, searcher));
		else
			return(target);
	} else if(found) {
		return(target);
	} else {
		return(NULL);
	}
}


Monster* BaseRoom::findMonster(Creature* searcher, const cmd* cmnd, int num) {
	return(findMonster(searcher, cmnd->str[num], cmnd->val[num]));
}
Monster* BaseRoom::findMonster(Creature* searcher, const bstring& name, const int num, bool firstAggro, bool exactMatch) {
	Creature* creature = findCreature(searcher, name, num, true, firstAggro, exactMatch);
	if(creature)
		return(creature->getMonster());
	return(NULL);
}

Player* BaseRoom::findPlayer(Creature* searcher, const cmd* cmnd, int num) {
	return(findPlayer(searcher, cmnd->str[num], cmnd->val[num]));
}
Player* BaseRoom::findPlayer(Creature* searcher, const bstring& name, const int num, bool exactMatch) {
	Creature* creature = findCreature(searcher, name, num, false, false, exactMatch);
	if(creature)
		return(creature->getPlayer());
	return(NULL);
}

//*********************************************************************
//						isSunlight
//*********************************************************************
// the first version is isSunlight is only called when an AreaRoom
// does not exist - when rogues scout, for example

bool Area::isSunlight(const MapMarker* mapmarker) const {
	if(!isDay())
		return(false);

	// if their room has any of the following flags, no kill
	if(	flagIsSet(R_DARK_ALWAYS, mapmarker) ||
		flagIsSet(R_UNDERGROUND, mapmarker) ||
		flagIsSet(R_INDOORS, mapmarker) ||
		flagIsSet(R_LIMBO, mapmarker) ||
		flagIsSet(R_VAMPIRE_COVEN, mapmarker) ||
		flagIsSet(R_ETHEREAL_PLANE, mapmarker) ||
		flagIsSet(R_IS_STORAGE_ROOM, mapmarker) ||
		flagIsSet(R_MAGIC_DARKNESS, mapmarker)
	)
		return(false);

	return(true);
}

// this version of isSunlight is called when a room exists

bool BaseRoom::isSunlight() const {

	// !this - for offline players. make it not sunny so darkmetal isnt destroyed
	if(!this)
		return(false);

	if(!isDay() || !isOutdoors() || isMagicDark())
		return(false);

	const UniqueRoom* uRoom = getConstUniqueRoom();
	const CatRefInfo* cri=0;
	if(uRoom)
		cri = gConfig->getCatRefInfo(uRoom->info.area);

	// be nice - no killing darkmetal in special rooms
	if(cri && (
		!uRoom->info.id ||
		uRoom->info.id == cri->getRecall() ||
		uRoom->info.id == cri->getLimbo()
	) )
		return(false);

	// if their room has any of the following flags, no kill
	if(	flagIsSet(R_DARK_ALWAYS) ||
		flagIsSet(R_UNDERGROUND) ||
		flagIsSet(R_ETHEREAL_PLANE) ||
		flagIsSet(R_IS_STORAGE_ROOM)
	)
		return(false);

	return(true);
}

//*********************************************************************
//						destroy
//*********************************************************************

void UniqueRoom::destroy() {
	saveToFile(0, LS_BACKUP);
	char	filename[256];
	strcpy(filename, roomPath(info));
	unlink(filename);
	expelPlayers(true, true, true);
	gConfig->delRoomQueue(info);
	delete this;
}

//*********************************************************************
//						expelPlayers
//*********************************************************************

void BaseRoom::expelPlayers(bool useTrapExit, bool expulsionMessage, bool expelStaff) {
	Player* target=0;
	ctag*	cp = first_ply;
	BaseRoom* newRoom=0;
	UniqueRoom* uRoom=0;

	// allow them to be sent to the room's trap exit
	if(useTrapExit) {
		uRoom = getUniqueRoom();
		if(uRoom) {
			CatRef cr = uRoom->getTrapExit();
			uRoom = 0;
			if(cr.id && loadRoom(cr, &uRoom))
				newRoom = uRoom;
		}
	}

	while(cp) {
		target = cp->crt->getPlayer();
		cp = cp->next_tag;

		if(!expelStaff && target->isStaff())
			continue;

		if(expulsionMessage) {
			target->printColor("^rYou are expelled from this room as it collapses in on itself!\n");
			broadcast(target->getSock(), this, "^r%M is expelled from the room as it collapses in on itself!", target);
		} else {
			target->printColor("^CShifting dimensional forces displace you from this room.\n");
			broadcast(target->getSock(), this, "^CShifting dimensional forces displace %N from this room.", target);
		}

		if(!useTrapExit || !newRoom) {
			newRoom = target->bound.loadRoom(target);
			if(!newRoom || newRoom == this)
				newRoom = abortFindRoom(target, "expelPlayers");
		}

		target->deleteFromRoom();
		target->addToRoom(newRoom);
		target->doPetFollow();
	}
}


//*********************************************************************
//						getSpecialArea
//*********************************************************************

Location getSpecialArea(int (CatRefInfo::*toCheck), CatRef cr) {
	return(getSpecialArea(toCheck, 0, cr.area, cr.id));
}

Location getSpecialArea(int (CatRefInfo::*toCheck), const Creature* creature, bstring area, short id) {
	Location l;

	if(creature) {
		if(creature->area_room) {
			l.room.setArea("area");
			l.room.id = creature->area_room->area->id;
		} else {
			l.room.setArea(creature->room.area);
		}
	}
	if(area != "")
		l.room.setArea(area);
	if(id)
		l.room.id = id;


	const CatRefInfo* cri = gConfig->getCatRefInfo(l.room.area, l.room.id, true);

	if(!cri)
		cri = gConfig->getCatRefInfo(gConfig->defaultArea);
	if(cri) {
		l.room.setArea(cri->getArea());
		l.room.id = (cri->*toCheck);
	} else {
		// failure!
		l.room.setArea(gConfig->defaultArea);
		l.room.id = 0;
	}

	return(l);
}

//*********************************************************************
//						getLimboRoom
//*********************************************************************

Location Creature::getLimboRoom() const {
	return(getSpecialArea(&CatRefInfo::limbo, this, "", 0));
}

//*********************************************************************
//						getRecallRoom
//*********************************************************************

Location Creature::getRecallRoom() const {
	const Player* player = getConstPlayer();
	if(player && (player->flagIsSet(P_T_TO_BOUND) || player->getClass() == BUILDER))
		return(player->bound);
	return(getSpecialArea(&CatRefInfo::recall, this, "", 0));
}


//*********************************************************************
//						print
//*********************************************************************

bool hearBroadcast(Creature* target, Socket* ignore1, Socket* ignore2, bool showTo(Socket*));

void BaseRoom::print(Socket* ignore, const char *fmt, ...) {
    va_list ap;
	va_start(ap, fmt);
	doPrint(0, ignore, NULL, fmt, ap);
	va_end(ap);
}
void BaseRoom::print(Socket* ignore1, Socket* ignore2, const char *fmt, ...) {
    va_list ap;
	va_start(ap, fmt);
	doPrint(0, ignore1, ignore2, fmt, ap);
	va_end(ap);
}

//*********************************************************************
//						doPrint
//*********************************************************************

void BaseRoom::doPrint(bool showTo(Socket*), Socket* ignore1, Socket* ignore2, const char *fmt, va_list ap) {
	Player* target=0;
	ctag	*cp=0;

	cp = first_ply;
	while(cp) {
		target = cp->crt->getPlayer();
		cp = cp->next_tag;

		if(!hearBroadcast(target, ignore1, ignore2, showTo))
			continue;
		if(target->flagIsSet(P_UNCONSCIOUS))
			continue;

		target->vprint(target->customColorize(fmt).c_str(), ap, true);

	}
}


//*********************************************************************
//						isOutlawSafe
//*********************************************************************

bool BaseRoom::isOutlawSafe() const {
	return(	flagIsSet(R_OUTLAW_SAFE) ||
			flagIsSet(R_VAMPIRE_COVEN) ||
			flagIsSet(R_LIMBO) ||
			whatTraining()
	);
}

//*********************************************************************
//						isOutlawSafe
//*********************************************************************

bool BaseRoom::isPkSafe() const {
	return(	flagIsSet(R_SAFE_ROOM) ||
			flagIsSet(R_VAMPIRE_COVEN) ||
			flagIsSet(R_LIMBO)
	);
}

//*********************************************************************
//						isOutlawSafe
//*********************************************************************

bool BaseRoom::isFastTick() const {
	return(	flagIsSet(R_FAST_HEAL) ||
			flagIsSet(R_LIMBO)
	);
}
