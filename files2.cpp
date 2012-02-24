/*
 * files2.cpp
 *   Additional file routines, including memory management.
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
//						reloadRoom
//*********************************************************************
// This function reloads a room from disk, if it's already loaded. This
// allows you to make changes to a room, and then reload it, even if it's
// already in the memory room queue.

bool Config::reloadRoom(BaseRoom* room) {
	UniqueRoom*	uRoom = room->getAsUniqueRoom();

	if(uRoom) {
		bstring str = uRoom->info.str();
		if(	roomQueue.find(str) != roomQueue.end() &&
			roomQueue[str].rom &&
			reloadRoom(uRoom->info) >= 0
		) {
			roomQueue[str].rom->addPermCrt();
			return(true);
		}
	} else {

		AreaRoom* aRoom = room->getAsAreaRoom();
		char	filename[256];
		sprintf(filename, "%s/%d/%s", Path::AreaRoom, aRoom->area->id,
			aRoom->mapmarker.filename().c_str());

		if(file_exists(filename)) {
			xmlDocPtr	xmlDoc;
			xmlNodePtr	rootNode;

			if((xmlDoc = xml::loadFile(filename, "AreaRoom")) == NULL)
				merror("Unable to read arearoom file", FATAL);

			rootNode = xmlDocGetRootElement(xmlDoc);

			Area *area = aRoom->area;
			aRoom->reset();
			aRoom->area = area;
			aRoom->load(rootNode);
			return(true);
		}
	}
	return(false);
}

int Config::reloadRoom(CatRef cr) {
	UniqueRoom	*room=0;
	otag	*op=0;

	bstring str = cr.str();
	if(roomQueue.find(str) == roomQueue.end())
		return(0);
	if(!roomQueue[str].rom)
		return(0);

	// if(read_rom(fd, room) < 0) {
	if(!loadRoomFromFile(cr, &room))
		return(-1);
	// Move any current players & monsters into the new room
	for(Player* ply : roomQueue[str].rom->players) {
		room->players.insert(ply);
	}
	roomQueue[str].rom->players.clear();

	// Only Monsters copy if target room is empty
	if(room->monsters.empty()) {
		for(Monster* mons : roomQueue[str].rom->monsters) {
			room->monsters.insert(mons);
		}
		roomQueue[str].rom->monsters.clear();
	}
	if(!room->first_obj) {
		room->first_obj = roomQueue[str].rom->first_obj;
		roomQueue[str].rom->first_obj = 0;
	}

	delete roomQueue[str].rom;
	roomQueue[str].rom = room;

	// Make sure we have the right parent set on everyone
	op = room->first_obj;
	while(op) {
		op->obj->parent_room = room;
		op = op->next_tag;
	}
	for(Player* ply : room->players) {
		ply->setParent(room);
	}
	for(Monster* mons : room->monsters) {
		mons->setParent(room);
	}

	return(0);
}

//*********************************************************************
//						resaveRoom
//*********************************************************************
// This function saves an already-loaded room back to memory without
// altering its position on the queue.

int saveRoomToFile(UniqueRoom* pRoom, int permOnly);

int Config::resaveRoom(CatRef cr) {
	bstring str = cr.str();
	if(roomQueue[str].rom)
		roomQueue[str].rom->saveToFile(ALLITEMS);
	return(0);
}


int Config::saveStorage(UniqueRoom* uRoom) {
	if(uRoom->flagIsSet(R_SHOP))
		saveStorage(shopStorageRoom(uRoom));
	return(saveStorage(uRoom->info));
}
int Config::saveStorage(CatRef cr) {
	bstring str = cr.str();
	if(!roomQueue[str].rom)
		return(0);

	if(roomQueue[str].rom->saveToFile(ALLITEMS) < 0)
		return(-1);

	return(0);
}

//********************************************************************
//						resaveAllRooms
//********************************************************************
// This function saves all memory-resident rooms back to disk.  If the
// permonly parameter is non-zero, then only permanent items in those
// rooms are saved back.

void Config::resaveAllRooms(char permonly) {
	qtag 	*qt = roomHead;
	while(qt) {
		if(roomQueue[qt->str].rom) {
			if(roomQueue[qt->str].rom->saveToFile(permonly) < 0)
				return;
		}
		qt = qt->next;
	}
}

//*********************************************************************
//						replaceMonsterInQueue
//*********************************************************************
void Config::replaceMonsterInQueue(CatRef cr, Monster *monster) {
	if(this->monsterInQueue(cr))
		*monsterQueue[cr.str()].mob = *monster;
	else
		addMonsterQueue(cr, &monster);
}

//*********************************************************************
//						replaceObjectInQueue
//*********************************************************************
void Config::replaceObjectInQueue(CatRef cr, Object* object) {
	if(objectInQueue(cr))
		*objectQueue[cr.str()].obj = *object;
	else
		addObjectQueue(cr, &object);
}

//*********************************************************************
//						save_all_ply
//*********************************************************************
// This function saves all players currently in memory.

void save_all_ply() {
	for(std::pair<bstring, Player*> p : gServer->players) {
		if(!p.second->isConnected())
			continue;
		p.second->save(true);
	}
}
