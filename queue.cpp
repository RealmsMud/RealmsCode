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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "mud.h"
//#include "xml.h"


//*********************************************************************
//                      flushRoom
//*********************************************************************
// This function flushes out the room queue and clears the room sparse
// pointer array, without saving anything to file. It also clears all
// memory used by loaded rooms. Call this function before leaving the
// program.

void Config::flushRoom() {
    qtag *qt=0;

    while(1) {
        pullQueue(&qt, &roomHead, &roomTail);
        if(!qt)
            break;
        delete roomQueue[qt->str].rom;
        roomQueue.erase(qt->str);
        delete qt;
    }
}

//*********************************************************************
//                      flushMonster
//*********************************************************************
// This function flushes out the monster queue and clears the monster
// sparse pointer array without saving anything to file. It also
// clears all memory used by loaded creatures. Call this function
// before leaving the program.

void Config::flushMonster() {
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

void Config::flushObject() {
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

void Config::putQueue(qtag **qt, qtag **headptr, qtag **tailptr) {
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

void Config::pullQueue(qtag **qt, qtag **headptr, qtag **tailptr) {
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

void Config::delQueue(qtag **qt, qtag **headptr, qtag **tailptr) {
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

void Config::frontQueue(qtag **qt, qtag **headptr, qtag **tailptr) {
    delQueue(qt, headptr, tailptr);
    putQueue(qt, headptr, tailptr);
}


//*********************************************************************
//                      monsterInQueue
//*********************************************************************

bool Config::monsterInQueue(const CatRef cr) {
    return(monsterQueue.find(cr.str()) != monsterQueue.end());
}

//**********************************************************************
//                      frontMonsterQueue
//**********************************************************************

void Config::frontMonsterQueue(const CatRef cr) {
    frontQueue(&monsterQueue[cr.str()].q_mob, &monsterHead, &monsterTail);
}

//**********************************************************************
//                      addMonsterQueue
//**********************************************************************

void Config::addMonsterQueue(const CatRef cr, Monster** pMonster) {
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

void Config::getMonsterQueue(const CatRef cr, Monster** pMonster) {
    **pMonster = *monsterQueue[cr.str()].mob;
}

//**********************************************************************
//                      objectInQueue
//**********************************************************************

bool Config::objectInQueue(const CatRef cr) {
    return(objectQueue.find(cr.str()) != objectQueue.end());
}

//**********************************************************************
//                      frontObjectQueue
//**********************************************************************

void Config::frontObjectQueue(const CatRef cr) {
    frontQueue(&objectQueue[cr.str()].q_obj, &objectHead, &objectTail);
}

//**********************************************************************
//                      addObjectQueue
//**********************************************************************

void Config::addObjectQueue(const CatRef cr, Object** pObject) {
    qtag    *qt;
    
    if(objectInQueue(cr))
        return;
    
    qt = new qtag;
    if(!qt)
        merror("addObjectQueue", FATAL);
    qt->str = cr.str();
    objectQueue[qt->str].obj = new Object;
    *objectQueue[qt->str].obj = **pObject;
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

void Config::getObjectQueue(const CatRef cr, Object** pObject) {
    **pObject = *objectQueue[cr.str()].obj;
}

//**********************************************************************
//                      roomInQueue
//**********************************************************************

bool Config::roomInQueue(const CatRef cr) {
    return(roomQueue.find(cr.str()) != roomQueue.end());
}

//**********************************************************************
//                      frontRoomQueue
//**********************************************************************

void Config::frontRoomQueue(const CatRef cr) {
    frontQueue(&roomQueue[cr.str()].q_rom, &roomHead, &roomTail);
}

//**********************************************************************
//                      addRoomQueue
//**********************************************************************

void Config::addRoomQueue(const CatRef cr, UniqueRoom** pRoom) {
    qtag    *qt;

    if(roomInQueue(cr))
        return;

    qt = new qtag;
    if(!qt)
        merror("addRoomQueue", FATAL);
    qt->str = cr.str();
    roomQueue[qt->str].rom = *pRoom;
    roomQueue[qt->str].q_rom = qt;
    putQueue(&qt, &roomHead, &roomTail);

    while(roomQueue.size() > RQMAX) {
        pullQueue(&qt, &roomHead, &roomTail);
        if(!roomQueue[qt->str].rom->players.empty()) {
            putQueue(&qt, &roomHead, &roomTail);
            continue;
        }
        if(!roomQueue[qt->str].rom)
            merror("ERROR - loadRoom", NONFATAL);
        // Save Room
        roomQueue[qt->str].rom->saveToFile(PERMONLY);
        delete roomQueue[qt->str].rom;
        roomQueue.erase(qt->str);
        delete qt;
    }
}

//**********************************************************************
//                      delRoomQueue
//**********************************************************************

void Config::delRoomQueue(const CatRef cr) {
    if(!roomInQueue(cr))
        return;

    qtag    *qt = roomQueue[cr.str()].q_rom;
    delQueue(&qt, &roomHead, &roomTail);
    delete qt;
    roomQueue.erase(cr.str());
}

//**********************************************************************
//                      getRoomQueue
//**********************************************************************

void Config::getRoomQueue(const CatRef cr, UniqueRoom** pRoom) {
    *pRoom = roomQueue[cr.str()].rom;
}

//**********************************************************************
//                      queue size
//**********************************************************************

int Config::roomQueueSize() {
    return(roomQueue.size());
}
int Config::monsterQueueSize() {
    return(monsterQueue.size());
}
int Config::objectQueueSize() {
    return(objectQueue.size());
}

//*********************************************************************
//                      killMortalObjects
//*********************************************************************
// called on sunrise - runs through active room list and kills darkmetal
// called when a unique group is full - destroys any extra unique items
// in game that can't be kept around anymore

void Config::killMortalObjects() {
    std::list<Area*>::iterator it;
    std::map<bstring, AreaRoom*>::iterator rt;
    std::list<AreaRoom*> toDelete;
    Area    *area=0;
    AreaRoom* room=0;
    qtag    *qt=0;

    qt = roomHead;
    while(qt) {
        if(roomQueue[qt->str].rom)
            roomQueue[qt->str].rom->killMortalObjects();
        qt = qt->next;
    }

    for(it = gConfig->areas.begin() ; it != gConfig->areas.end() ; it++) {
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
