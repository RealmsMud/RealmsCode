/*
 * ships.c
 *   Moving exits and such
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
#include "move.h"
#include "ships.h"
#include "calendar.h"
#include "commands.h"
#include <sstream>


#define PIRATE_QUEST 54 - 1



// Notes:
// If you mess up the data up, you'll never know what time the ship
// will be at the docks. You will need to check that ALL of the time
// spent at stops / time spent traveling adds up to some 24 hour
// increment or divides into 24.
// For example, if it adds up to 25, you'll get:
//		stop 1, day 1: 8am
//		stop 1, day 2: 9am
//		stop 1, day 3: 10am


//*********************************************************************
//						shipBroadcastRange
//*********************************************************************
// given a ShipStop and a message, we'll announce it to the
// mud (appropriate rooms)

bool ShipStop::belongs(CatRef cr) {
	std::list<Range>::iterator rt;
	for(rt = announce.begin() ; rt != announce.end() ; rt++)
		if((*rt).belongs(cr))
			return(true);
	return(false);
}

bool Ship::belongs(CatRef cr) {
	std::list<Range>::iterator rt;
	for(rt = ranges.begin() ; rt != ranges.end() ; rt++)
		if((*rt).belongs(cr))
			return(true);
	return(false);
}

void shipBroadcastRange(Ship *ship, ShipStop *stop, bstring message) {

	if(message == "")
		return;

	// if our announce range isnt there, announce to everyone and quit
	if(stop->announce.empty()) {
		broadcast("%s", message.c_str());
		return;
	}

	Player* target=0;
	// otherwise go through each player
	for(std::pair<bstring, Player*> p : gServer->players) {
		target = p.second;

		if(!target->isConnected())
			continue;
		if(!target->inUniqueRoom())
			continue;

		if(	ship->belongs(target->getUniqueRoomParent()->info) ||
			stop->belongs(target->getUniqueRoomParent()->info)
		)
			target->print("%s\n", message.c_str());
	}
}



//*********************************************************************
//						spawnRaiders
//*********************************************************************
// run through the exits declared as Raid and spawn some raiders!

void ShipExit::spawnRaiders(ShipRaid* sRaid) {
	Monster *raider=0;
	BaseRoom *room=0;
	int		l=0, total=0;

	if(name == "" || !raid || !origin.getId() || !target.getId())
		return;

	if(!loadMonster(sRaid->getSearchMob(), &raider))
		return;

	room = getRoom(false);
	if(!room) {
		free_crt(raider);
		return;
	}

	if(!raider->flagIsSet(M_CUSTOM))
		raider->validateAc();

	if(!raider->name[0] || raider->name[0] == ' ') {
		free_crt(raider);
		return;
	}

	total = mrand(sRaid->getMinSpawnNum(), sRaid->getMaxSpawnNum());

	for(l=0; l<total;) {
		raider->initMonster(true, false);

		if(!l)
			raider->addToRoom(room, total);
		else
			raider->addToRoom(room, 0);

		raider->setFlag(M_PERMENANT_MONSTER);
		raider->setFlag(M_RAIDING);
		gServer->addActive(raider);
		l++;
		if(l < total)
			loadMonster(sRaid->getSearchMob(), &raider);
	}
}


//*********************************************************************
//						getRoom
//*********************************************************************

BaseRoom* ShipExit::getRoom(bool useOrigin) {
	if(useOrigin)
		return(origin.loadRoom());
	return(target.loadRoom());
}


//*********************************************************************
//						spawnRaiders
//*********************************************************************
// run through the exits declared as Raid and spawn some raiders!

void ShipStop::spawnRaiders() {
	std::list<ShipExit*>::iterator xt;

	if(!raid || !raid->getSearchMob())
		return;

	for(xt = exits.begin() ; xt != exits.end() ; xt++)
		(*xt)->spawnRaiders(raid);
}


//*********************************************************************
//						createExit
//*********************************************************************
// creates the exits for the ShipExit it's given
// it also takes care of announcing the ship's arrival

bool ShipExit::createExit() {
	BaseRoom* newRoom=0;
	int		i=0;

	if(name == "" || !origin.getId() || !target.getId())
		return(false);

	newRoom = getRoom(true);
	if(!newRoom)
		return(false);

	link_rom(newRoom, target, name);

	for(Exit* ext : newRoom->exits) {
		if(!strcmp(ext->name, name.c_str())) {
			zero(ext->flags, sizeof(ext->flags));

			// TODO: Dom: needs to use getFlags() and setFlags()
			for(i=sizeof(ext->flags)-1; i>=0; i--)
				ext->flags[i] = flags[i];
			ext->setFlag(X_MOVING);

			if(arrives != "")
				broadcast(NULL, newRoom, "%s", arrives.c_str());
			return(true);
		}
	}

	return(false);
}

// hand on that responsibility!
// but do broadcasting and raider-spawning
int shipSetExits(Ship *ship, ShipStop *stop) {
	std::list<ShipExit*>::iterator exit;
	bool		broad = false;

	for(exit = stop->exits.begin() ; exit != stop->exits.end() ; exit++)
		broad = (*exit)->createExit() || broad;

	if(broad)
		shipBroadcastRange(ship, stop, stop->arrives);
	stop->spawnRaiders();
	return(0);
}



//*********************************************************************
//						removeExit
//*********************************************************************
// Given a ship exits, it will delete the appropriate moving exits.
// Will also cause raiders to wander away.

void ShipExit::removeExit() {
	Monster *raider=0;
	BaseRoom* newRoom=0;
	AreaRoom* aRoom=0;
	int		i=0, n=0;

	newRoom = getRoom(true);
	if(!newRoom)
		return;

	// kill all raiders
	MonsterSet::iterator mIt = newRoom->monsters.begin();
	while(mIt != newRoom->monsters.end()) {
		raider = (*mIt++);

		if(raider && raider->flagIsSet(M_RAIDING)) {
			broadcast(NULL, newRoom, "%1M just %s away.", raider, Move::getString(raider).c_str());
			gServer->delActive(raider);
			raider->deleteFromRoom();
			raider->clearAsEnemy();
			free_crt(raider);
		}
	}


	if(departs != "")
		broadcast(NULL, newRoom, "%s", departs.c_str());

	aRoom = newRoom->getAsAreaRoom();
	ExitList::iterator xit;
	for(xit = newRoom->exits.begin() ; xit != newRoom->exits.end() ; ) {
		Exit* ext = (*xit++);
		// if we're given an exit, delete it
		// otherwise delete all exits
		if(ext->flagIsSet(X_MOVING) && !strcmp(ext->name, name.c_str()) ) {
			//oldPrintColor(4, "^rDeleting^w exit %s in room %d.\n", xt->name, room);
			if(i < 8 && aRoom) {
				// it's a cardinal direction, clear all flags
				for(n=0; n<MAX_EXIT_FLAGS; n++)
					ext->clearFlag(n);
				ext->target.room.id = 0;
				aRoom->updateExits();
			} else
				newRoom->delExit(ext);
		}
		i++;
	}
}

// hand on that responsibility!
// but do broadcasting and kicking people off a raiding vessel
int shipDeleteExits(Ship *ship, ShipStop *stop) {
	std::list<ShipExit*>::iterator exit;
	Monster		*raider=0;
	UniqueRoom		*room=0, *newRoom=0;
	int			found=1;
	char		output[160];


	if(stop->raid) {
		// record when the last pirate raid was
		if(stop->raid->getRecord())
			gConfig->calendar->setLastPirate(ship->name);

		// kick people off the ship!
		if(	stop->raid->getSearchMob() &&
			(stop->raid->getDump().id || stop->raid->getPrison().id) &&
			loadMonster(stop->raid->getSearchMob(), &raider)
		) {
			Player* ply;
			// otherwise go through each player
			for(std::pair<bstring, Player*> p : gServer->players) {
				ply = p.second;

				if(!ply->isConnected())
					continue;
				if(ply->isStaff())
					continue;
				if(!ply->inUniqueRoom())
					continue;

				if(!ship->belongs(ply->getUniqueRoomParent()->info))
					continue;


				if(!loadRoom(ply->getUniqueRoomParent()->info, &room))
					continue;
				if(!room->wander.getTraffic())
					continue;

				found = 1;


				ply->bPrint(raider->getCrtStr(NULL, CAP, 1) + " just arrived\n");
				ply->sendPrompt();

				// you can't see me!
				if(!raider->canSee(ply))
					found = 0;

				// if they're hidden - look for them!
				if(found && ply->flagIsSet(P_HIDDEN)) {
					found = 0;

					ply->bPrint(raider->getCrtStr(NULL, CAP | NONUM, 0) + " searches the room.\n");

					if(mrand(1,100) <= SHIP_SEARCH_CHANCE) {
						found = 1;
						ply->unhide();

						sprintf(output, "%s found something!\n", raider->upHeShe());
						ply->print(output);
					}
					ply->sendPrompt();
				}

				if(!found) {
				    ply->bPrint(raider->getCrtStr(NULL, CAP|NONUM, 0) + " wanders away.\n");
					ply->sendPrompt();
					continue;
				}

				ply->wake("You awaken suddenly!");

				if(	stop->raid->getDump().id &&
					!(ply->isUnconscious() && stop->raid->getUnconInPrison()) &&
					loadRoom(stop->raid->getDump(), &newRoom)
				) {


					if(stop->raid->getDumpTalk() != "") {
					    ply->bPrint(raider->getCrtStr(NULL, CAP|NONUM, 0) + " says to you, \"" + stop->raid->getDumpTalk() + "\".\n");
					}
					if(stop->raid->getDumpAction() != "") {
						if(stop->raid->getDumpTalk() != "")
							ply->sendPrompt();
						bstring tmp = stop->raid->getDumpAction();
						bstring tmp2 = raider->getCrtStr(NULL, CAP|NONUM, 0);
						tmp.Replace("*ACTOR*", tmp2.c_str());
						ply->bPrint(tmp + "\n");
					}
					ply->deleteFromRoom();
					ply->addToRoom(newRoom);
					ply->doPetFollow();

				} else if(stop->raid->getPrison().id &&
					loadRoom(stop->raid->getPrison(), &newRoom)
				) {

					if(stop->raid->getPrisonTalk() != "") {
					    ply->bPrint(raider->getCrtStr(NULL, CAP|NONUM, 0) + "says to you \"" + stop->raid->getPrisonTalk() + "\".\n");
					}
					if(stop->raid->getPrisonAction() != "") {
						if(stop->raid->getPrisonTalk() != "")
							ply->sendPrompt();
                        bstring tmp = stop->raid->getPrisonAction();
                        bstring tmp2 = raider->getCrtStr(NULL, CAP|NONUM, 0);
                        tmp.Replace("*ACTOR*", tmp2.c_str());
                        ply->bPrint(tmp + "\n");
					}
					ply->deleteFromRoom();
					ply->addToRoom(newRoom);
					ply->doPetFollow();
				}

				broadcast(NULL, room, "%M was hauled off by %N.", ply, raider);
			}
			free_crt(raider);
		}
	}

	for(exit = stop->exits.begin() ; exit != stop->exits.end() ; exit++)
		(*exit)->removeExit();

	shipBroadcastRange(ship, stop, stop->departs);
	return(0);
}




// ___...---...___...---...___...---
//		ShipRaid
// ___...---...___...---...___...---

ShipRaid::ShipRaid() {
	searchMob = raidMob = minSpawnNum = maxSpawnNum = record = 0;
	dumpTalk = prisonTalk = dumpAction = prisonAction = "";
	unconInPrison = false;
}

void ShipRaid::load(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Record")) xml::copyToBool(record, childNode);
		else if(NODE_NAME(childNode, "SearchMob")) xml::copyToNum(searchMob, childNode);
		else if(NODE_NAME(childNode, "RaidMob")) xml::copyToNum(raidMob, childNode);
		else if(NODE_NAME(childNode, "MinSpawnNum")) xml::copyToNum(minSpawnNum, childNode);
		else if(NODE_NAME(childNode, "MaxSpawnNum")) xml::copyToNum(maxSpawnNum, childNode);
		else if(NODE_NAME(childNode, "Dump")) dump.load(childNode);
		else if(NODE_NAME(childNode, "Prison")) prison.load(childNode);
		else if(NODE_NAME(childNode, "DumpTalk")) xml::copyToBString(dumpTalk, childNode);
		else if(NODE_NAME(childNode, "PrisonTalk")) xml::copyToBString(prisonTalk, childNode);
		else if(NODE_NAME(childNode, "DumpAction")) xml::copyToBString(dumpAction, childNode);
		else if(NODE_NAME(childNode, "PrisonAction")) xml::copyToBString(prisonAction, childNode);
		else if(NODE_NAME(childNode, "UnconInPrison")) xml::copyToBool(unconInPrison, childNode);

		childNode = childNode->next;
	}
	minSpawnNum = MAX(0, minSpawnNum);
	if(maxSpawnNum < minSpawnNum)
		maxSpawnNum = minSpawnNum;
}

void ShipRaid::save(xmlNodePtr curNode) const {
	xml::saveNonZeroNum(curNode, "Record", record);
	xml::saveNonZeroNum(curNode, "SearchMob", searchMob);
	xml::saveNonZeroNum(curNode, "RaidMob", raidMob);
	xml::saveNonZeroNum(curNode, "MinSpawnNum", minSpawnNum);
	xml::saveNonZeroNum(curNode, "MaxSpawnNum", maxSpawnNum);

	dump.save(curNode, "Dump", false);
	prison.save(curNode, "Prison", false);
	xml::saveNonNullString(curNode, "DumpTalk", dumpTalk);
	xml::saveNonNullString(curNode, "PrisonTalk", prisonTalk);
	xml::saveNonNullString(curNode, "DumpAction", dumpAction);
	xml::saveNonNullString(curNode, "PrisonAction", prisonAction);

	xml::saveNonZeroNum(curNode, "UnconInPrison", unconInPrison);
}

// get functions
int ShipRaid::getRecord() const {
	return(record);
}
int ShipRaid::getSearchMob() const {
	return(searchMob);
}
int ShipRaid::getRaidMob() const {
	return(raidMob);
}
int ShipRaid::getMinSpawnNum() const {
	return(minSpawnNum);
}
int ShipRaid::getMaxSpawnNum() const {
	return(maxSpawnNum);
}
CatRef ShipRaid::getDump() const {
	return(dump);
}
CatRef ShipRaid::getPrison() const {
	return(prison);
}
bstring ShipRaid::getDumpTalk() const {
	return(dumpTalk);
}
bstring ShipRaid::getPrisonTalk() const {
	return(prisonTalk);
}
bstring ShipRaid::getDumpAction() const {
	return(dumpAction);
}
bstring ShipRaid::getPrisonAction() const {
	return(prisonAction);
}
bool ShipRaid::getUnconInPrison() const {
	return(unconInPrison);
}


// ___...---...___...---...___...---
//		ShipExit
// ___...---...___...---...___...---

ShipExit::ShipExit() {
	raid = 0;
	zero(flags, sizeof(flags));
	name = "";
	arrives = "";
	departs = "";
}

bstring ShipExit::getName() const { return(name); }
bool ShipExit::getRaid() const { return(raid); }
bstring ShipExit::getArrives() const { return(arrives); }
bstring ShipExit::getDeparts() const { return(departs); }
const char *ShipExit::getFlags() const { return(flags); }

void ShipExit::setFlags(char f) {
	int i = sizeof(flags);
	char tmp[i];
	zero(tmp, i);
	*(tmp) = f;
	for(i--; i>=0; i--)
		flags[i] = tmp[i];
}


void ShipExit::save(xmlNodePtr curNode) const {
	xml::saveNonNullString(curNode, "Name", name);
	origin.save(curNode, "Origin");
	target.save(curNode, "Target");
	saveBits(curNode, "Flags", MAX_EXIT_FLAGS, flags);
	xml::saveNonNullString(curNode, "Arrives", arrives);
	xml::saveNonNullString(curNode, "Departs", departs);
	xml::saveNonZeroNum(curNode, "Raid", raid);
}

void ShipExit::load(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Name")) { xml::copyToBString(name, childNode); }
		else if(NODE_NAME(childNode, "Origin")) origin.load(childNode);
		else if(NODE_NAME(childNode, "Target")) target.load(childNode);
		else if(NODE_NAME(childNode, "Arrives")) xml::copyToBString(arrives, childNode);
		else if(NODE_NAME(childNode, "Departs")) xml::copyToBString(departs, childNode);
		else if(NODE_NAME(childNode, "Flags")) loadBits(childNode, flags);
		else if(NODE_NAME(childNode, "Raid")) raid = true;
		childNode = childNode->next;
	}
}

// ___...---...___...---...___...---
//		ShipStop
// ___...---...___...---...___...---

ShipStop::ShipStop() {
	name = arrives = lastcall = departs = "";
	to_next = in_dock = 0;
	raid = 0;
}

ShipStop::~ShipStop() {
	ShipExit* exit=0;

	if(raid)
		delete raid;

	while(!exits.empty()) {
		exit = exits.front();
		delete exit;
		exits.pop_front();
	}
	exits.clear();
}

void ShipStop::load(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Name")) { xml::copyToBString(name, childNode); }
		else if(NODE_NAME(childNode, "Arrives")) { xml::copyToBString(arrives, childNode); }
		else if(NODE_NAME(childNode, "LastCall")) { xml::copyToBString(lastcall, childNode); }
		else if(NODE_NAME(childNode, "Departs")) { xml::copyToBString(departs, childNode); }

		else if(NODE_NAME(childNode, "ToNext")) xml::copyToNum(to_next, childNode);
		else if(NODE_NAME(childNode, "InDock")) xml::copyToNum(in_dock, childNode);

		else if(NODE_NAME(childNode, "Raid")) {
			raid = new ShipRaid;
			raid->load(childNode);
		}
		else if(NODE_NAME(childNode, "Exits"))
			loadExits(childNode);

		else if(NODE_NAME(childNode, "AnnounceRange")) {
			xmlNodePtr subNode = childNode->children;

			while(subNode) {
				if(NODE_NAME(subNode, "Range")) {
					Range r;
					r.load(subNode);
					announce.push_back(r);
				}
				subNode = subNode->next;
			}
		}
		childNode = childNode->next;
	}
}

void ShipStop::loadExits(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	ShipExit	*exit=0;

	while(childNode) {
		if(NODE_NAME(childNode, "Exit")) {
			exit = new ShipExit;
			exit->load(childNode);
			if(exit->origin.room.id || exit->origin.mapmarker.getArea())
				exit->removeExit();
			exits.push_back(exit);
		}
		childNode = childNode->next;
	}
}

void ShipStop::save(xmlNodePtr rootNode) const {
	xmlNodePtr	curNode, childNode, xchildNode;

	curNode = xml::newStringChild(rootNode, "Stop");
	xml::saveNonNullString(curNode, "Name", name);
	xml::saveNonZeroNum(curNode, "InDock", in_dock);
	xml::saveNonZeroNum(curNode, "ToNext", to_next);

	xml::saveNonNullString(curNode, "Arrives", arrives);
	xml::saveNonNullString(curNode, "LastCall", lastcall);
	xml::saveNonNullString(curNode, "Departs", departs);

	if(raid) {
		childNode = xml::newStringChild(curNode, "Raid");
		raid->save(childNode);
	}

	childNode = xml::newStringChild(curNode, "Exits");

	std::list<ShipExit*>::const_iterator exit;
	for(exit = exits.begin() ; exit != exits.end() ; exit++) {
		xchildNode = xml::newStringChild(childNode, "Exit");
		(*exit)->save(xchildNode);
	}

	if(!announce.empty()) {
		childNode = xml::newStringChild(curNode, "AnnounceRange");
		std::list<Range>::const_iterator rt;
		for(rt = announce.begin() ; rt != announce.end() ; rt++)
			(*rt).save(childNode, "Range");
	}
}

// ___...---...___...---...___...---
//		Ship
// ___...---...___...---...___...---

Ship::Ship() {
	timeLeft = clan = 0;
	name = transit = movement = docked = "";
	inPort = canQuery = false;
}

Ship::~Ship() {
	ShipStop* stop=0;

	while(!stops.empty()) {
		stop = stops.front();
		delete stop;
		stops.pop_front();
	}
	stops.clear();
}

void Ship::load(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	int		at_stop=0, stop=0;

	while(childNode) {
			 if(NODE_NAME(childNode, "Name"))  xml::copyToBString(name, childNode);
		else if(NODE_NAME(childNode, "Clan")) xml::copyToNum(clan, childNode);
		else if(NODE_NAME(childNode, "Transit"))  xml::copyToBString(transit, childNode);
		else if(NODE_NAME(childNode, "Movement"))  xml::copyToBString(movement, childNode);
		else if(NODE_NAME(childNode, "Docked"))  xml::copyToBString(docked, childNode);

		else if(NODE_NAME(childNode, "CanQuery")) xml::copyToBool(canQuery, childNode);
		else if(NODE_NAME(childNode, "InPort")) xml::copyToBool(inPort, childNode);
		else if(NODE_NAME(childNode, "TimeLeft")) xml::copyToNum(timeLeft, childNode);
		// at_stop is only used when loading - the cycling of the stops below
		// will put the stop pointed to by at_stop at the front. when it's saved,
		// it will load first
		else if(NODE_NAME(childNode, "AtStop")) xml::copyToNum(at_stop, childNode);

		else if(NODE_NAME(childNode, "BoatRange")) {
			xmlNodePtr subNode = childNode->children;

			while(subNode) {
				if(NODE_NAME(subNode, "Range")) {
					Range r;
					r.load(subNode);
					ranges.push_back(r);
				}
				subNode = subNode->next;
			}
		} else if(NODE_NAME(childNode, "Stops"))
			loadStops(childNode);
		childNode = childNode->next;
	}

	// now move what stop we're at to the front of the list
	while(stop < at_stop) {
		stop++;
		stops.push_back(stops.front());
		stops.pop_front();
	}
}


void Ship::loadStops(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	ShipStop	*stop=0;

	while(childNode) {
		if(NODE_NAME(childNode, "Stop")) {
			stop = new ShipStop;
			stop->load(childNode);
			stops.push_back(stop);
		}
		childNode = childNode->next;
	}
}


void Ship::save(xmlNodePtr rootNode) const {
	xmlNodePtr	curNode, childNode;

	curNode = xml::newStringChild(rootNode, "Ship");
	xml::saveNonNullString(curNode, "Name", name);
	xml::saveNonZeroNum(curNode, "Clan", clan);
	xml::saveNonZeroNum(curNode, "AtStop", 0);
	xml::saveNonZeroNum(curNode, "InPort", inPort);
	xml::saveNonZeroNum(curNode, "TimeLeft", timeLeft);

	xml::saveNonZeroNum(curNode, "CanQuery", canQuery);
	xml::saveNonNullString(curNode, "Transit", transit);
	xml::saveNonNullString(curNode, "Movement", movement);
	xml::saveNonNullString(curNode, "Docked", docked);

	if(!ranges.empty()) {
		childNode = xml::newStringChild(curNode, "BoatRange");
		std::list<Range>::const_iterator rt;
		for(rt = ranges.begin() ; rt != ranges.end() ; rt++)
			(*rt).save(childNode, "Range");
	}

	childNode = xml::newStringChild(curNode, "Stops");
	std::list<ShipStop*>::const_iterator st;
	for(st = stops.begin() ; st != stops.end() ; st++)
		(*st)->save(childNode);
}


//*********************************************************************
//						shipPrintRange
//*********************************************************************

// used in dmQueryShips to print out the ranges

void shipPrintRange(Player* player, std::list<Range>* list) {
	std::list<Range>::iterator rt;
	for(rt = list->begin() ; rt != list->end() ; rt++)
		player->print("  %s\n", (*rt).str().c_str());
}

//*********************************************************************
//						dmQueryShips
//*********************************************************************
// dm command to show all information about ships

int dmQueryShips(Player* player, cmd* cmnd) {
	int			mod=0, id=0;
	std::list<Ship*>::iterator it;
	std::list<ShipStop*>::iterator st;
	std::list<ShipExit*>::iterator xt;
	Ship		*ship=0;
	ShipStop	*stop=0;
	ShipExit	*exit=0;
	int shipID = atoi(getFullstrText(cmnd->fullstr, 1).c_str());
	int stopID = atoi(getFullstrText(cmnd->fullstr, 2).c_str());
	const Calendar* calendar = 0;

	if(!shipID) {
		calendar = gConfig->getCalendar();
		player->print("Location of All Ships:\n");
		player->printColor("^b-------------------------------------------------------------------------------\n");

		id = 1;
		for(it = gConfig->ships.begin() ; it != gConfig->ships.end() ; it++) {
			ship = (*it);
			player->print("  %-25s %s", ship->name.c_str(), ship->inPort ? "Stopped at " : "In transit, going to ");

			st = ship->stops.begin();
			stop = (*st);
			player->print("%s.\n     #%-25dWill ", stop->name.c_str(), id);
			if(ship->inPort) {
				st++;
				stop = (*st);
				player->print("depart for %s", stop->name.c_str());
			} else {
				player->print("arrive");
			}
			player->print(" in ");
			mod = ship->timeLeft / 60;
			if(mod)
				player->print("%d hour%s%s", mod, mod==1 ? "" : "s", ship->timeLeft % 60 ? " " : "");
			mod = ship->timeLeft % 60;
			if(mod)
				player->print("%d minute%s", mod, mod==1 ? "" : "s");
			player->print(".\n");

			id++;
		}
		player->print("\nType \"*ship [ship id]\" to view the details of a specific ship.\n");
		player->printColor("Ship Updates:  ^c%d^x / ^c%d^x    Hour: ^c%d^x / ^c%d^x.\n",
			calendar->shipUpdates, gConfig->expectedShipUpdates(),
			calendar->shipUpdates % 60, gConfig->expectedShipUpdates() % 60);
		player->print("The last pirate raid was: %s\n\n", calendar->getLastPirate().c_str());
		return(0);
	}


	mod = 1;
	// find the ship we're looking at
	for(it = gConfig->ships.begin() ; it != gConfig->ships.end() && mod != shipID ; it++)
		mod++;
	ship = (*it);

	if(!ship) {
		player->print("Invalid ship!\n");
		return(0);
	}

	player->print("Information on %s:\n", ship->name.c_str());
	player->printColor("^b-------------------------------------------------------------------------------\n");

	stop = ship->stops.front();
	player->print("In Port:   %s\n", ship->inPort ? "Yes" : "No");
	player->print("Time Left: %d minutes\n", ship->timeLeft);

	if(!stopID) {

		player->print("Can Query: %s\n", ship->canQuery ? "Yes" : "No");
		if(ship->transit != "")
			player->print("Transit:   %s\n", ship->transit.c_str());
		if(ship->movement != "")
			player->print("Movement:  %s\n", ship->movement.c_str());
		if(ship->docked != "")
			player->print("Docked:    %s\n", ship->docked.c_str());
		player->print("\n");

		// boat range
		player->print("Boat Ranges:\n");
		shipPrintRange(player, &ship->ranges);
		player->print("\n");

		player->print("Stops:\n");

		id = 1;

		for(st = ship->stops.begin() ; st != ship->stops.end() ; st++) {
			stop = (*st);
			player->print("-----------------------\n");
			player->print("  Stop: %d - %s\n", id, stop->name.c_str());
			player->print("  Raid: %s\n", stop->raid ? "Yes" : "No");

			player->print("  Exits:\n");
			mod = 1;
			for(xt = stop->exits.begin() ; xt != stop->exits.end() ; xt++) {
				exit = (*xt);
				player->print("     Exit: %d - %s, room %s %s\n", mod, exit->getName().c_str(),
					exit->origin.room.str().c_str(), exit->origin.mapmarker.getArea() ? exit->origin.mapmarker.str().c_str() : "");
				mod++;
			}

			player->print("\n");
			id++;
		}

		player->print("Type \"*ship [ship id] [stop id]\" to view the details of a specific stop.\n\n");

	} else {

		mod = 1;
		// find the stop we're looking at
		for(st = ship->stops.begin() ; st != ship->stops.end() && mod != stopID ; st++)
			mod++;

		stop = (*st);

		if(!stop) {
			player->print("Invalid stop!\n");
			return(0);
		}

		player->print("Stop: %d - %s\n", id, stop->name.c_str());
 		player->print("In Dock:   %d minutes\n", stop->in_dock);
		player->print("To Next:   %d minutes\n\n", stop->to_next);

		if(stop->arrives != "")
			player->print("Arrives:   %s\n", stop->arrives.c_str());
		if(stop->lastcall != "")
			player->print("Last Call: %s\n", stop->lastcall.c_str());
		if(stop->departs != "")
			player->print("Departs:   %s\n", stop->departs.c_str());
		if(stop->arrives != "" || stop->lastcall != "" || stop->departs != "")
			player->print("\n");

		if(stop->raid) {
 			player->print("Raid Info:\n-----------------------\n");
 			player->print("Record:          %s\n", stop->raid->getRecord() ? "Yes" : "No");
 			player->print("Search Mob:      %d\n", stop->raid->getSearchMob());
 			player->print("Dump Room:       %s\n", stop->raid->getDump().str().c_str());
 			player->print("Prison Room:     %s\n", stop->raid->getPrison().str().c_str());
 			if(stop->raid->getDumpTalk() != "")
 				player->print("Dump Talk:       %s\n", stop->raid->getDumpTalk().c_str());
 			if(stop->raid->getPrisonTalk() != "")
 				player->print("Prison Talk:     %s\n", stop->raid->getPrisonTalk().c_str());
 			if(stop->raid->getDumpAction() != "")
 				player->print("Dump Action:     %s\n", stop->raid->getDumpAction().c_str());
 			if(stop->raid->getPrisonAction() != "")
 				player->print("Prison Action:   %s\n", stop->raid->getPrisonAction().c_str());
			player->print("Uncon In Prison: %s\n\n", stop->raid->getUnconInPrison() ? "Yes" : "No");
		}

		// announce range
		player->print("Announce Ranges:\n");
		shipPrintRange(player, &stop->announce);
		player->print("\n");

		player->print("Exits:\n");
		player->print("-----------------------\n");

		mod = 1;
		for(xt = stop->exits.begin() ; xt != stop->exits.end() ; xt++) {
			exit = (*xt);
			player->print("  Exit:    %d - %s\n", mod, exit->getName().c_str());
			player->print("  uOrigin: %s   aOrigin: %s\n", exit->origin.room.str().c_str(),
				exit->origin.mapmarker.str().c_str());
			player->print("  uTarget: %s   aTarget: %s\n", exit->target.room.str().c_str(),
				exit->target.mapmarker.str().c_str());
			player->print("  Raid:    %s\n", exit->getRaid() ? "Yes" : "No");
			if(exit->getArrives() != "")
				player->print("  Arrives: %s\n", exit->getArrives().c_str());
			if(exit->getDeparts() != "")
				player->print("  Departs: %s\n", exit->getDeparts().c_str());

			player->print("\n");
			mod++;
		}
	}

	return(0);
}


//*********************************************************************
//						cmdQueryShips
//*********************************************************************
// let the player see where ships are

int cmdQueryShips(Player* player, cmd* cmnd) {
	int		mod=0;
	std::list<Ship*>::iterator it;
	std::list<ShipStop*>::iterator st;
	Ship	*ship=0;
	ShipStop *stop=0;

	if(!player->ableToDoCommand())
		return(0);

	player->print("Location of Chartered Ships:\n");
	player->printColor("^b-------------------------------------------------------------------------------\n");

	for(it = gConfig->ships.begin() ; it != gConfig->ships.end() ; it++) {
		ship = (*it);
		// the wizard's eye tower can be viewed by clan members
		if(ship->canQuery || (ship->clan && ship->clan == player->getClan())) {
			player->print("  %-25s ", ship->name.c_str());
			if(ship->inPort)
				player->print("%s at ", ship->docked.c_str());
			else
				player->print("%s, %s to ", ship->transit.c_str(), ship->movement.c_str());

			st = ship->stops.begin();
			stop = (*st);
			player->print("%s.\n%-31sWill ", stop->name.c_str(), " ");
			if(ship->inPort) {
				st++;
				stop = (*st);
				player->print("depart for %s", stop->name.c_str());
			} else
				player->print("arrive");

			player->print(" in ");
			mod = ship->timeLeft / 60;
			if(mod)
				player->print("%d hour%s%s", mod, mod==1 ? "" : "s", ship->timeLeft % 60 ? " " : "");
			mod = ship->timeLeft % 60;
			if(mod)
				player->print("%d minute%s", mod, mod==1 ? "" : "s");
			player->print(".\n");
		}
	}

	// Dont let staff cheat and set the quest on themselves
	// to see where the Marauder will be.
	if(player->isStaff() && !player->isDm())
		return(0);

	if(player->questIsSet(PIRATE_QUEST) || player->isDm())
		player->print("\nThe last pirate raid was: %s\n", gConfig->getCalendar()->getLastPirate().c_str());

	return(0);
}


//*********************************************************************
//						saveShips
//*********************************************************************

void Config::saveShips() const {
	std::list<Ship*>::const_iterator it;
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	char			filename[80];

	// we only save if we're up on the main port
	if(getPortNum() != 3333)
		return;

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Ships", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	for(it = ships.begin() ; it != ships.end() ; it++)
		(*it)->save(rootNode);

	sprintf(filename, "%s/ships.xml", Path::Game);
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
}


//*********************************************************************
//						loadShips
//*********************************************************************

bool Config::loadShips() {
	char filename[80];
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	xmlNodePtr	curNode;
	Ship	*ship=0;
	std::list<Ship*>::iterator it;

	sprintf(filename, "%s/ships.xml", Path::Game);

	if(!file_exists(filename))
		return(false);

	if((xmlDoc = xml::loadFile(filename, "Ships")) == NULL)
		return(false);

	rootNode = xmlDocGetRootElement(xmlDoc);
	curNode = rootNode->children;

	clearShips();
	while(curNode) {
		if(NODE_NAME(curNode, "Ship")) {
			ship = new Ship;
			ship->load(curNode);
			ships.push_back(ship);
		}
		curNode = curNode->next;
	}

	// exits have all been deleted - go and set the current ones
	for(it = ships.begin() ; it != ships.end() ; it++) {
		ship = (*it);
		// if at a stop, make the exits
		if(ship->inPort)
			shipSetExits(ship, ship->stops.front());
	}

	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	return(true);
}

//*********************************************************************
//						clearShips
//*********************************************************************

void Config::clearShips() {
	Ship	*ship=0;

	while(!ships.empty()) {
		ship = ships.front();
		delete ship;
		ships.pop_front();
	}
	ships.clear();
}
