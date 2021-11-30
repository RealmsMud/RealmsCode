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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <libxml/parser.h>                          // for xmlFreeDoc, xmlCl...
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <cstring>                                  // for strcpy
#include <ctime>                                    // for time
#include <libxml/xmlstring.h>                       // for BAD_CAST
#include <list>                                     // for list, operator==
#include <ostream>                                  // for basic_ostream::op...
#include <string>                                   // for allocator, operat...

#include "carry.hpp"                                // for Carry
#include "catRef.hpp"                               // for CatRef
#include "config.hpp"                               // for Config, gConfig
#include "enums/loadType.hpp"                       // for LoadType, LoadTyp...
#include "flags.hpp"                                // for M_TALKS
#include "global.hpp"                               // for ALLITEMS, FATAL
#include "lasttime.hpp"                             // for lasttime
#include "mud.hpp"                                  // for LT_TICK, LT_TICK_...
#include "mudObjects/creatures.hpp"                 // for NUM_ASSIST_MOB
#include "mudObjects/monsters.hpp"                  // for Monster
#include "os.hpp"                                   // for merror
#include "paths.hpp"                                // for checkDirExists
#include "proto.hpp"                                // for monsterPath, load...
#include "quests.hpp"                               // for TalkResponse, Que...
#include "server.hpp"                               // for Server, gServer
#include "xml.hpp"                                  // for toNum, NODE_NAME

//*********************************************************************
//                      loadMonster
//*********************************************************************

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

bool loadMonsterFromFile(const CatRef& cr, Monster **pMonster, std::string filename, bool offline) {
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

    if(NODE_NAME(curNode, "Plural")) xml::copyToString(plural, curNode);
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
    else if(NODE_NAME(curNode, "Talk")) xml::copyToString(talk, curNode);
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
        xml::copyToString(primeFaction, curNode);
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


//*********************************************************************
//                      saveToFile
//*********************************************************************
// This function will write the supplied creature to the proper
// m##.xml file - To accomplish this it parse the proper m##.xml file if
// available and walk down the tree to the proper place in numerical order
// if the file does not exist it will create it with only that monster in it.
// It will most likely only be called from *save c

int Monster::saveToFile() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char        filename[256];

    // If we can't clean the monster properly, don't save anything
    if(cleanMobForSaving() != 1)
        return(-1);

    // Invalid Number
    if(info.id < 0)
        return(-1);
    Path::checkDirExists(info.area, monsterPath);

    gServer->saveIds();

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Creature", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    escapeText();
    std::string idTemp = id;
    id = "-1";
    saveToXml(rootNode, ALLITEMS, LoadType::LS_FULL);
    id = idTemp;

    strcpy(filename, monsterPath(info));
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
    return(0);
}



//*********************************************************************
//                      saveXml
//*********************************************************************

void Monster::saveXml(xmlNodePtr curNode) const {
    xmlNodePtr childNode, subNode;

    // record monsters saved during swap
    if(gConfig->swapIsInteresting(this))
        gConfig->swapLog((std::string)"m" + info.rstr(), false);

    xml::saveNonNullString(curNode, "Plural", plural);
    xml::saveNonZeroNum(curNode, "SkillLevel", skillLevel);
    xml::saveNonZeroNum(curNode, "UpdateAggro", updateAggro);
    xml::saveNonZeroNum(curNode, "LoadAggro", loadAggro);
    // Attacks
    childNode = xml::newStringChild(curNode, "Attacks");
    for(int i=0; i<3; i++) {
        if(attack[i][0] == 0)
            continue;
        subNode = xml::newStringChild(childNode, "Attack", attack[i]);
        xml::newNumProp(subNode, "Num", i);
    }
    xml::saveNonNullString(curNode, "LastMod", last_mod);
    xml::saveNonNullString(curNode, "Talk", talk);
    xmlNodePtr talkNode = xml::newStringChild(curNode, "TalkResponses");
    for(TalkResponse * talkResponse : responses)
        talkResponse->saveToXml(talkNode);

    xml::saveNonNullString(curNode, "TradeTalk", ttalk);
    xml::saveNonZeroNum(curNode, "NumWander", numwander);
    xml::saveNonZeroNum(curNode, "MagicResistance", magicResistance);
    xml::saveNonZeroNum(curNode, "DefenseSkill", defenseSkill);
    xml::saveNonZeroNum(curNode, "AttackPower", attackPower);
    xml::saveNonZeroNum(curNode, "WeaponSkill", weaponSkill);
    saveCarryArray(curNode, "CarriedItems", "Carry", carry, 10);
    saveCatRefArray(curNode, "AssistMobs", "Mob", assist_mob, NUM_ASSIST_MOB);
    saveCatRefArray(curNode, "EnemyMobs", "Mob", enemy_mob, NUM_ENEMY_MOB);

    xml::saveNonNullString(curNode, "PrimeFaction", primeFaction);
    xml::saveNonNullString(curNode, "AggroString", aggroString);

    jail.save(curNode, "Jail", false);
    xml::saveNonZeroNum(curNode, "WeaponSkill", maxLevel);
    xml::saveNonZeroNum(curNode, "Cast", cast);
    saveCatRefArray(curNode, "Rescue", "Mob", rescue, NUM_RESCUE);

    saveBits(curNode, "ClassAggro", 32, cClassAggro);
    saveBits(curNode, "RaceAggro", 32, raceAggro);
    saveBits(curNode, "DeityAggro", 32, deityAggro);
}

//*********************************************************************
//                      saveCarryArray
//*********************************************************************

xmlNodePtr saveCarryArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const Carry array[], int arraySize) {
    xmlNodePtr curNode=nullptr;
    // this nested loop means we won't create an xml node if we don't have to
    for(int i=0; i<arraySize; i++) {
        if(array[i].info.id) {
            curNode = xml::newStringChild(parentNode, rootName);
            for(; i<arraySize; i++) {
                if(array[i].info.id)
                    array[i].save(curNode, childName, false, i);
            }
            return(curNode);
        }
    }
    return(curNode);
}

