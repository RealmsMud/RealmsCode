//
// Created by jason on 12/11/16.
//

#pragma once

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
void fulfillQuest(std::shared_ptr<Player> player, std::shared_ptr<Object>  object);

extern int numQuests;

