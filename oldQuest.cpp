/*
 * oldQuest.cpp
 *   Xml parsing and various quest related functions (Including monster interaction)
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
#include "commands.h"
#include "factions.h"
#include "quests.h"
#include "tokenizer.h"

// Function prototypes
static questPtr parseQuest(xmlDocPtr doc, xmlNodePtr cur);

// Loads the quests file
bool Config::loadQuestTable() {
    xmlDocPtr       doc;
    questPtr        curquest;
    xmlNodePtr      cur;
    char    filename[80];

    // build an XML tree from a the file
    sprintf(filename, "%s/questTable.xml", Path::Game);
    doc = xml::loadFile(filename, "Quests");
    if(doc == nullptr)
        return(false);

    cur = xmlDocGetRootElement(doc);
    numQuests = 0;
    memset(questTable, 0, sizeof(questTable));

    // First level we expect a Quest
    cur = cur->children;
    while(cur && xmlIsBlankNode(cur))
        cur = cur->next;

    if(cur == 0) {
        xmlFreeDoc(doc);
        return(false);
    }

    clearQuestTable();
    // Go through the nodes and grab off all Quests
    while(cur != nullptr) {
        if((!strcmp((char *)cur->name, "Quest"))) {
            // Parse the quest
            curquest = parseQuest(doc, cur);
            // If we parsed a valid quest, add it to the table
            if(curquest != nullptr)
                questTable[numQuests++] = curquest;
            // No more than 128 quests currently allowed
            if(numQuests >= 128)
                break;
        }
        cur = cur->next;
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return(true);
}

static questPtr parseQuest(xmlDocPtr doc, xmlNodePtr cur) {
    questPtr ret = nullptr;
    ret = new quest;
    if(ret == nullptr) {
        loge("Quest_Load: out of memory\n");
        return(nullptr);
    }

    cur = cur->children;
    // Note: (char *)xmlNodeListGetString returns a string which MUST be freed
    //       thus we can't use atoi...use toInt or toLong

    while(cur != nullptr) {
        if((!strcmp((char *)cur->name, "ID")))
            ret->num = xml::toNum<int>((char *)xmlNodeListGetString(doc, cur->children, 1));
        if((!strcmp((char *)cur->name, "name")))
            ret->name = (char *)xmlNodeListGetString(doc, cur->children, 1);
        if((!strcmp((char *)cur->name, "exp")))
            ret->exp = xml::toNum<int>((char *)xmlNodeListGetString(doc, cur->children, 1));
        cur = cur->next;
    }

    return (ret);
}


// Free the memory being used by the quest table
void Config::clearQuestTable() {
    int i;
    for(i=0;i<numQuests;i++)
        freeQuest(questTable[i]);
}

// Free the memory being used by a quest
void freeQuest(questPtr toFree) {
    if(toFree->name)
        free(toFree->name);
    delete toFree;
}

void fulfillQuest(Player* player, Object* object) {
    if(object->getQuestnum()) {
        player->print("Quest fulfilled!");
        if(object->getType() != MONEY) {
            player->printColor(" Don't drop %P.\n", object);
            player->print("You won't be able to pick it up again.");
        }
        player->print("\n");
        player->setQuest(object->getQuestnum()-1);
        player->addExperience(get_quest_exp(object->getQuestnum()));
        if(!player->halftolevel())
            player->print("%ld experience granted.\n", get_quest_exp(object->getQuestnum()));
    }
    for(Object *obj : object->objects) {
        fulfillQuest(player, obj);
    }
}
