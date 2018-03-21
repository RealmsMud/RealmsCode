/* 
 * queue.c
 *   Deals with the monster, object, and room queues
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "creatures.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "server.hpp"


//*********************************************************************
//                      flushRoom
//*********************************************************************
// This function flushes out the room queue and clears the room sparse
// pointer array, without saving anything to file. It also clears all
// memory used by loaded rooms. Call this function before leaving the
// program.

void Server::flushRoom() {
	roomCache.clear();
}

//*********************************************************************
//                      flushMonster
//*********************************************************************
// This function flushes out the monster queue and clears the monster
// sparse pointer array without saving anything to file. It also
// clears all memory used by loaded creatures. Call this function
// before leaving the program.

void Server::flushMonster() {
	monsterCache.clear();
}

//********************************************************************
//                      flushObject
//********************************************************************
// This function flushes out the object queue and clears the object
// sparse pointer array without saving anything to file. It also
// clears all memory used by loaded objects. Call this function
// leaving the program.

void Server::flushObject() {
	objectCache.clear();
}

//*********************************************************************
//                      killMortalObjects
//*********************************************************************
// called on sunrise - runs through active room list and kills darkmetal
// called when a unique group is full - destroys any extra unique items
// in game that can't be kept around anymore

void Server::killMortalObjects() {
    std::list<Area*>::iterator it;
    std::map<bstring, AreaRoom*>::iterator rt;
    std::list<AreaRoom*> toDelete;
    Area    *area=nullptr;
    AreaRoom* room=nullptr;
    UniqueRoom* r=nullptr;

    for(auto it : roomCache) {
        r = it.second->second;
        if(!r)
            continue;
	    r->killMortalObjects();
    }

    for(it = gServer->areas.begin() ; it != gServer->areas.end() ; it++) {
        area = (*it);
        for(rt = area->rooms.begin() ; rt != area->rooms.end() ; rt++) {
            room = (*rt).second;
            room->killMortalObjects();

            if(room->canDelete())
                toDelete.push_back(room);
        }

        while(!toDelete.empty()) {
            area->remove(toDelete.front());
            toDelete.pop_front();
        }
        toDelete.clear();
    }
}


//********************************************************************
//                      resaveAllRooms
//********************************************************************
// This function saves all memory-resident rooms back to disk.  If the
// permonly parameter is non-zero, then only permanent items in those
// rooms are saved back.

void Server::resaveAllRooms(char permonly) {
	UniqueRoom *r;

    for(auto it : roomCache) {
        r = it.second->second;
        if(!r)
            continue;
		r->saveToFile(permonly);
	}
}
