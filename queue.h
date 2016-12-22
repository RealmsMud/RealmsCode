//
// Created by jason on 12/11/16.
//

#ifndef REALMSCODE_QUEUE_H
#define REALMSCODE_QUEUE_H

#include "bstring.h"

class Monster;
class Object;
class UniqueRoom;

// General queue tag data struct
typedef struct queue_tag {
public:
    queue_tag() { next = prev = 0; };
    bstring             str;
    struct queue_tag    *next;
    struct queue_tag    *prev;
} qtag;


// Sparse pointer array for rooms
typedef struct rsparse {
public:
    rsparse() { rom = 0; q_rom = 0; };
    UniqueRoom *rom;
    qtag            *q_rom;
} rsparse;


// Sparse pointer array for creatures
typedef struct msparse {
public:
    msparse() { mob = 0; q_mob = 0; };
    Monster *mob;
    qtag            *q_mob;
} msparse;


// Sparse pointer array for objects
typedef struct osparse {
public:
    osparse() {obj = 0; q_obj = 0; };
    Object          *obj;
    qtag            *q_obj;
} osparse;

#endif //REALMSCODE_QUEUE_H
