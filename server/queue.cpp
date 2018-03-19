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
    qtag *qt=0;

    while(1) {
        pullQueue(&qt, &monsterHead, &monsterTail);
        if(!qt)
            break;
        free_crt(monsterQueue[qt->str].mob);
        monsterQueue.erase(qt->str);
        delete qt;
    }
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
    qtag *qt=0;

    while(1) {
        pullQueue(&qt, &objectHead, &objectTail);
        if(!qt)
            break;
        delete objectQueue[qt->str].obj;
        objectQueue.erase(qt->str);
        delete qt;
    }
}

//*********************************************************************
//                      putQueue
//*********************************************************************
// putQueue places the queue tag pointed to by the first paramater onto
// a queue whose head and tail tag pointers are the second and third
// parameters.  If parameters 2 & 3 are 0, then a new queue is created.
// The fourth parameter points to a queue size counter which is
// incremented. 

void Server::putQueue(qtag **qt, qtag **headptr, qtag **tailptr) {
    if(!*headptr) {
        *headptr = *qt;
        *tailptr = *qt;
        (*qt)->next = 0;
        (*qt)->prev = 0;
    } else {
        (*headptr)->prev = *qt;
        (*qt)->next = *headptr;
        (*qt)->prev = 0;
        *headptr = *qt;
    }
}

//*********************************************************************
//                      pullQueue
//*********************************************************************
// pullQueue removes the last queue tag on the queue specified by the
// second and third parameters and returns that tag in the first
// parameter.  The fourth parameter points to a queue size counter
// which is decremented.

void Server::pullQueue(qtag **qt, qtag **headptr, qtag **tailptr) {
    if(!*tailptr)
        *qt = 0;
    else {
        *qt = *tailptr;
        if((*qt)->prev) {
            (*qt)->prev->next = 0;
            *tailptr = (*qt)->prev;
        } else {
            *headptr = 0;
            *tailptr = 0;
        }
    }
}

//*********************************************************************
//                      delQueue
//*********************************************************************

void Server::delQueue(qtag **qt, qtag **headptr, qtag **tailptr) {
    if((*qt)->prev) {
        ((*qt)->prev)->next = (*qt)->next;
        if(*qt == *tailptr)
            *tailptr = (*qt)->prev;
    }
    if((*qt)->next) {
        ((*qt)->next)->prev = (*qt)->prev;
        if(*qt == *headptr)
            *headptr = (*qt)->next;
    }
    if(!(*qt)->prev && !(*qt)->next) {
        *headptr = 0;
        *tailptr = 0;
    }
    (*qt)->next = 0;
    (*qt)->prev = 0;
}

//*********************************************************************
//                      frontQueue
//*********************************************************************
// frontQueue removes the queue tag pointed to by the first parameter
// from the queue (specified by the second and third parameters) and 
// places it back at the head of the queue.  The fourth parameter is a 
// pointer to a queue size counter, and it remains unchanged.

void Server::frontQueue(qtag **qt, qtag **headptr, qtag **tailptr) {
    delQueue(qt, headptr, tailptr);
    putQueue(qt, headptr, tailptr);
}


//*********************************************************************
//                      monsterInQueue
//*********************************************************************

bool Server::monsterInQueue(const CatRef cr) {
    return(monsterQueue.find(cr.str()) != monsterQueue.end());
}

//**********************************************************************
//                      frontMonsterQueue
//**********************************************************************

void Server::frontMonsterQueue(const CatRef cr) {
    frontQueue(&monsterQueue[cr.str()].q_mob, &monsterHead, &monsterTail);
}

//**********************************************************************
//                      addMonsterQueue
//**********************************************************************

void Server::addMonsterQueue(const CatRef cr, Monster** pMonster) {
    qtag    *qt;

    if(monsterInQueue(cr))
        return;

    qt = new qtag;
    if(!qt)
        merror("addMonsterQueue", FATAL);
    qt->str = cr.str();
    monsterQueue[qt->str].mob = new Monster;
    *monsterQueue[qt->str].mob = **pMonster;
    monsterQueue[qt->str].q_mob = qt;
    putQueue(&qt, &monsterHead, &monsterTail);

    while(monsterQueue.size() > MQMAX) {
        pullQueue(&qt, &monsterHead, &monsterTail);
        free_crt(monsterQueue[qt->str].mob);
        monsterQueue.erase(qt->str);
        delete qt;
    }
}

//**********************************************************************
//                      getMonsterQueue
//**********************************************************************

void Server::getMonsterQueue(const CatRef cr, Monster** pMonster) {
    **pMonster = *monsterQueue[cr.str()].mob;
}

//**********************************************************************
//                      objectInQueue
//**********************************************************************

bool Server::objectInQueue(const CatRef cr) {
    return(objectQueue.find(cr.str()) != objectQueue.end());
}

//**********************************************************************
//                      frontObjectQueue
//**********************************************************************

void Server::frontObjectQueue(const CatRef cr) {
    frontQueue(&objectQueue[cr.str()].q_obj, &objectHead, &objectTail);
}

//**********************************************************************
//                      addObjectQueue
//**********************************************************************

void Server::addObjectQueue(const CatRef cr, Object** pObject) {
    qtag    *qt;
    
    if(objectInQueue(cr))
        return;
    
    qt = new qtag;
    if(!qt)
        merror("addObjectQueue", FATAL);
    qt->str = cr.str();
    objectQueue[qt->str].obj = new Object;
    *objectQueue[qt->str].obj = **pObject;
    objectQueue[qt->str].obj->id = "-1";

    objectQueue[qt->str].q_obj = qt;
    putQueue(&qt, &objectHead, &objectTail);
    
    while(objectQueue.size() > OQMAX) {
        pullQueue(&qt, &objectHead, &objectTail);
        delete objectQueue[qt->str].obj;
        objectQueue.erase(qt->str);
        delete qt;
    }
}

//**********************************************************************
//                      getObjectQueue
//**********************************************************************

void Server::getObjectQueue(const CatRef cr, Object** pObject) {
    **pObject = *objectQueue[cr.str()].obj;
}

//**********************************************************************
//                      queue size
//**********************************************************************

int Server::monsterQueueSize() {
    return(monsterQueue.size());
}
int Server::objectQueueSize() {
    return(objectQueue.size());
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

//*********************************************************************
//                      replaceMonsterInQueue
//*********************************************************************
void Server::replaceMonsterInQueue(CatRef cr, Monster *monster) {
    if(this->monsterInQueue(cr))
        *monsterQueue[cr.str()].mob = *monster;
    else
        addMonsterQueue(cr, &monster);
}

//*********************************************************************
//                      replaceObjectInQueue
//*********************************************************************
void Server::replaceObjectInQueue(CatRef cr, Object* object) {
    if(objectInQueue(cr))
        *objectQueue[cr.str()].obj = *object;
    else
        addObjectQueue(cr, &object);
}


