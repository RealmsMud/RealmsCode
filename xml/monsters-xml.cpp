/*
 * monsters-xml.cpp
 *   Monsters xml
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


//*********************************************************************
//                      loadMonster
//*********************************************************************

#include <creatures.hpp>
#include <proto.hpp>
#include <server.hpp>
#include <mud.hpp>
#include <xml.hpp>

bool loadMonster(int index, Monster ** pMonster, bool offline) {
    CatRef cr;
    cr.id = index;
    return(loadMonster(cr, pMonster, offline));
}

bool loadMonster(const CatRef& cr, Monster ** pMonster, bool offline) {
    if(!validMobId(cr))
        return(false);

    // Check if monster is already loaded, and if so return pointer
    if(gServer->monsterCache.contains(cr)) {
        *pMonster = new Monster;
        gServer->monsterCache.fetch(cr, pMonster, false);
    } else {
        // Otherwise load the monster and return a pointer to the newly loaded monster
        // Load the creature from it's file
        if(!loadMonsterFromFile(cr, pMonster, "", offline))
            return(false);
        gServer->monsterCache.insert(cr, pMonster);
    }

    (*pMonster)->fd = -1;
    (*pMonster)->lasttime[LT_TICK].ltime =
    (*pMonster)->lasttime[LT_TICK_SECONDARY].ltime =
    (*pMonster)->lasttime[LT_TICK_HARMFUL].ltime = time(nullptr);

    (*pMonster)->lasttime[LT_TICK].interval  =
    (*pMonster)->lasttime[LT_TICK_SECONDARY].interval = 60;
    (*pMonster)->lasttime[LT_TICK_HARMFUL].interval = 30;
    (*pMonster)->getMobSave();

    return(true);
}

//*********************************************************************
//                      loadMonsterFromFile
//*********************************************************************

bool loadMonsterFromFile(const CatRef& cr, Monster **pMonster, bstring filename, bool offline) {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    int     num=0;

    if(filename.empty())
        filename = monsterPath(cr);

    if((xmlDoc = xml::loadFile(filename.c_str(), "Creature")) == nullptr)
        return(false);

    if(xmlDoc == nullptr) {
        std::clog << "Error parsing file " << filename;
        return(false);
    }
    rootNode = xmlDocGetRootElement(xmlDoc);
    num = xml::getIntProp(rootNode, "Num");

    if(cr.id == -1 || num == cr.id) {
        *pMonster = new Monster;
        if(!*pMonster)
            merror("loadMonsterFromFile", FATAL);
        (*pMonster)->setVersion(rootNode);

        (*pMonster)->readFromXml(rootNode, offline);
        (*pMonster)->setId("-1");

        if((*pMonster)->flagIsSet(M_TALKS)) {
            loadCreature_tlk((*pMonster));
            (*pMonster)->convertOldTalks();
        }
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}


//*********************************************************************
//                      readXml
//*********************************************************************

void Monster::readXml(xmlNodePtr curNode, bool offline) {
    xmlNodePtr childNode;

    if(NODE_NAME(curNode, "Plural")) xml::copyToBString(plural, curNode);
    else if(NODE_NAME(curNode, "CarriedItems")) {
        loadCarryArray(curNode, carry, "Carry", 10);
    }
    else if(NODE_NAME(curNode, "LoadAggro")) setLoadAggro(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "LastMod")) xml::copyToCString(last_mod, curNode);

        // get rid of these after conversion
    else if(NODE_NAME(curNode, "StorageRoom") && getVersion() < "2.34")
        setLoadAggro(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "BoundRoom") && getVersion() < "2.34")
        setSkillLevel(xml::toNum<int>(curNode));


    else if(NODE_NAME(curNode, "SkillLevel")) setSkillLevel(xml::toNum<int>(curNode));
    else if(NODE_NAME(curNode, "ClassAggro")) {
        loadBits(curNode, cClassAggro);
    }
    else if(NODE_NAME(curNode, "RaceAggro")) {
        loadBits(curNode, raceAggro);
    }
    else if(NODE_NAME(curNode, "DeityAggro")) {
        loadBits(curNode, deityAggro);
    }
    else if(NODE_NAME(curNode, "Attacks")) {
        loadStringArray(curNode, attack, CRT_ATTACK_LENGTH, "Attack", 3);
    }
    else if(NODE_NAME(curNode, "Talk")) xml::copyToBString(talk, curNode);
    else if(NODE_NAME(curNode, "TalkResponses")) {
        childNode = curNode->children;
        TalkResponse* newTalk;
        while(childNode) {
            if(NODE_NAME(childNode, "TalkResponse")) {
                if((newTalk = new TalkResponse(childNode)) != nullptr) {
                    responses.push_back(newTalk);
                    if (newTalk->quest != nullptr) {
                        quests.push_back(newTalk->quest);
                    }
                }
            }
            childNode = childNode->next;
        }

    }
    else if(NODE_NAME(curNode, "TradeTalk")) xml::copyToCString(ttalk, curNode);
    else if(NODE_NAME(curNode, "NumWander")) setNumWander(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "MagicResistance")) setMagicResistance(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "MobTrade")) setMobTrade(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "AssistMobs")) {
        loadCatRefArray(curNode, assist_mob, "Mob", NUM_ASSIST_MOB);
    }
    else if(NODE_NAME(curNode, "EnemyMobs")) {
        loadCatRefArray(curNode, enemy_mob, "Mob", NUM_ENEMY_MOB);
    }
    else if(NODE_NAME(curNode, "PrimeFaction")) {
        xml::copyToBString(primeFaction, curNode);
    }
    else if(NODE_NAME(curNode, "AggroString")) xml::copyToCString(aggroString, curNode);
    else if(NODE_NAME(curNode, "MaxLevel")) setMaxLevel(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "Cast")) setCastChance(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "Jail")) jail.load(curNode);
    else if(NODE_NAME(curNode, "Rescue")) loadCatRefArray(curNode, rescue, "Mob", NUM_RESCUE);

    else if(NODE_NAME(curNode, "UpdateAggro")) setUpdateAggro(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "PkIn") && getVersion() < "2.42b") setUpdateAggro(xml::toNum<unsigned short>(curNode));

        // Now handle version changes

    else if(getVersion() < "2.21") {
        //std::clog << "Loading mob pre version 2.21" << std::endl;
        // Title was changed to AggroString as of 2.21
        if(NODE_NAME(curNode, "Title")) xml::copyToCString(aggroString, curNode);
    }

}

//*********************************************************************
//                      loadCarryArray
//*********************************************************************

void loadCarryArray(xmlNodePtr curNode, Carry array[], const char* name, int maxProp) {
    xmlNodePtr childNode = curNode->children;
    int i=0;

    while(childNode) {
        if(NODE_NAME(childNode , name)) {
            i = xml::getIntProp(childNode, "Num");
            if(i >= 0 && i < maxProp)
                array[i].load(childNode);
        }
        childNode = childNode->next;
    }
}