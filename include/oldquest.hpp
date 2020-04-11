//
// Created by jason on 12/11/16.
//

#ifndef REALMSCODE_OLDQUEST_H
#define REALMSCODE_OLDQUEST_H

class Player;
class Object;

typedef struct quest {
public:
    quest() { num = exp = 0; name = 0; };
    int     num;
    int     exp;
    char    *name;
} quest, *questPtr;

// quests.cpp
void freeQuest(questPtr toFree);
void fulfillQuest(Player* player, Object* object);

extern int numQuests;


#endif //REALMSCODE_OLDQUEST_H
