/*
 * players-xml.cpp
 *   Player xml
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

// TODO: Make a function that will use an xml reader to only parse the player to find
// name and password

//*********************************************************************
//                      loadPlayer
//*********************************************************************
// Attempt to load the player named 'name' into the address given
// return 0 on success, -1 on failure

#include <libxml/parser.h>          // for xmlNode, xmlNodePtr
#include <ostream>                  // for operator<<, basic...

#include "anchor.hpp"               // for Anchor
#include "calendar.hpp"             // for cDay
#include "catRef.hpp"               // for CatRef
#include "config.hpp"               // for Config, gConfig
#include "creatures.hpp"            // for Player, Player::Q...
#include "enums/loadType.hpp"       // for LoadType, LoadTyp...
#include "global.hpp"               // for MAX_DIMEN_ANCHORS
#include "levelGain.hpp"            // for LevelGain
#include "paths.hpp"                // for Player, PlayerBackup
#include "playerClass.hpp"          // for PlayerClass
#include "playerTitle.hpp"          // for PlayerTitle
#include "proxy.hpp"                // for ProxyAccess, ProxyManager
#include "proto.hpp"                // file_exists
#include "quests.hpp"               // for QuestCompleted
#include "server.hpp"               // for Server, gServer
#include "skillGain.hpp"            // for SkillGain
#include "xml.hpp"                  // for NODE_NAME, newStr...


bool loadPlayer(std::string_view name, Player** player, enum LoadType loadType) {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    std::string     filename;
    std::string     pass = "", loadName = "";

    if(loadType == LoadType::LS_BACKUP)
        filename = fmt::format("{}/{}.bak.xml", Path::PlayerBackup, name);
    else if(loadType == LoadType::LS_CONVERT)
        filename = fmt::format("{}/convert/{}.xml", Path::Player, name);
    else // LoadType::LS_NORMAL
        filename = fmt::format("{}/{}.xml", Path::Player, name);

    if((xmlDoc = xml::loadFile(filename.c_str(), "Player")) == nullptr)
        return(false);

    rootNode = xmlDocGetRootElement(xmlDoc);
    loadName = xml::getProp(rootNode, "Name");
    if(loadName != name) {
        std::clog << "Error loading " << name << ", found " << loadName << " instead!\n";
        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();
        return(false);
    }

    *player = new Player;

    (*player)->setName(xml::getProp(rootNode, "Name"));
    xml::copyPropToString(pass, rootNode, "Password");
    (*player)->setPassword(pass);
    (*player)->setLastLogin(xml::getIntProp(rootNode, "LastLogin"));

    // If we get here, we should be loading the correct player, so start reading them in
    (*player)->readFromXml(rootNode);

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

//*********************************************************************
//                      readXml
//*********************************************************************

void Player::readXml(xmlNodePtr curNode, bool offline) {
    xmlNodePtr childNode;

    if(NODE_NAME(curNode, "Birthday")) {
        birthday = new cDay;
        birthday->load(curNode);
    }
    else if(NODE_NAME(curNode, "BoundRoom")) {
        if(getVersion() < "2.42h")
            bound.room.load(curNode);
        else
            bound.load(curNode);
    }
    else if(NODE_NAME(curNode, "PreviousRoom")) previousRoom.load(curNode); // monster do not save PreviousRoom
    else if(NODE_NAME(curNode, "Statistics")) statistics.load(curNode);

    else if(NODE_NAME(curNode, "Bank")) bank.load(curNode);
    else if(NODE_NAME(curNode, "Surname")) xml::copyToString(surname, curNode);
    else if(NODE_NAME(curNode, "Wrap")) xml::copyToNum(wrap, curNode);
    else if(NODE_NAME(curNode, "Forum")) xml::copyToString(forum, curNode);
    else if(NODE_NAME(curNode, "Ranges"))
        loadRanges(curNode, this);
    else if(NODE_NAME(curNode, "Wimpy")) setWimpy(xml::toNum<unsigned short>(curNode));

    else if(NODE_NAME(curNode, "Songs")) {
        loadBits(curNode, songs);
    }
    else if(NODE_NAME(curNode, "Anchors")) {
        loadAnchors(curNode);
    }

    else if(NODE_NAME(curNode, "LastPassword")) xml::copyToString(lastPassword, curNode);
    else if(NODE_NAME(curNode, "PoisonedBy")) xml::copyToString(poisonedBy, curNode);
    else if(NODE_NAME(curNode, "AfflictedBy")) xml::copyToString(afflictedBy, curNode);
    else if(NODE_NAME(curNode, "ActualLevel")) setActualLevel(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "WeaponTrains")) setWeaponTrains(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "LastMod")) xml::copyToString(oldCreated, curNode);
    else if(NODE_NAME(curNode, "Created")) {
        if(getVersion() < "2.43c")
            xml::copyToString(oldCreated, curNode);
        else
            xml::copyToNum(created, curNode);
    }
    else if(NODE_NAME(curNode, "Guild")) setGuild(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "GuildRank")) setGuildRank(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "TickDmg")) setTickDamage(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "NegativeLevels")) setNegativeLevels(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "LastInterest")) setLastInterest(xml::toNum<long>(curNode));
    else if(NODE_NAME(curNode, "Title")) setTitle(xml::getString(curNode));
    else if(NODE_NAME(curNode, "CustomColors")) xml::copyToCString(customColors, curNode);
    else if(NODE_NAME(curNode, "Thirst")) setThirst(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "RoomExp")) {
        childNode = curNode->children;
        while(childNode) {
            if(NODE_NAME(childNode, "Room")) {
                CatRef cr;
                cr.load(childNode);
                roomExp.push_back(cr);
            }
            childNode = childNode->next;
        }
    }
    else if(NODE_NAME(curNode, "ObjIncrease")) {
        childNode = curNode->children;
        while(childNode) {
            if(NODE_NAME(childNode, "Object")) {
                CatRef cr;
                cr.load(childNode);
                objIncrease.push_back(cr);
            }
            childNode = childNode->next;
        }
    }
    else if(NODE_NAME(curNode, "StoresRefunded")) {\
        childNode = curNode->children;
        while(childNode) {
            if(NODE_NAME(childNode, "Store")) {
                CatRef cr;
                cr.load(childNode);
                storesRefunded.push_back(cr);
            }
            childNode = childNode->next;
        }
    }
    else if(NODE_NAME(curNode, "Lore")) {
        childNode = curNode->children;
        while(childNode) {
            if(NODE_NAME(childNode, "Info")) {
                CatRef cr;
                cr.load(childNode);
                lore.push_back(cr);
            }
            childNode = childNode->next;
        }
    }
    else if(NODE_NAME(curNode, "Recipes")) {
        childNode = curNode->children;
        while(childNode) {
            if(NODE_NAME(childNode, "Recipe")) {
                int i=0;
                xml::copyToNum(i, childNode);
                recipes.push_back(i);
            }
            childNode = childNode->next;
        }
    }
    else if(NODE_NAME(curNode, "QuestsInProgress")) {
        childNode = curNode->children;
        QuestCompletion* qc;
        while(childNode) {
            if(NODE_NAME(childNode, "QuestCompletion")) {
                qc = new QuestCompletion(childNode, this);
                questsInProgress[qc->getParentQuest()->getId()] = qc;
            }
            childNode = childNode->next;
        }
    }
    else if(NODE_NAME(curNode, "QuestsCompleted")) {
        childNode = curNode->children;
        if(getVersion() < "2.45") {
            // Old format
            while(childNode) {
                if(NODE_NAME(childNode, "Quest")) {
                    int questNum = xml::toNum<int>(childNode);
                    questsCompleted.insert(std::make_pair(questNum, new QuestCompleted(1)));
                }
                childNode = childNode->next;
            }
        } else if (getVersion() < "2.47i") {
            // Old - New Format
            xmlNodePtr subNode;
            while(childNode) {
                if(NODE_NAME(childNode, "Quest")) {
                    int questNum = -1;
                    int numCompletions = 1;
                    subNode = childNode->children;
                    while(subNode) {
                        if(NODE_NAME(subNode, "Id")) {
                            questNum = xml::toNum<int>(subNode);
                        } else if(NODE_NAME(subNode, "Num")) {
                            numCompletions = xml::toNum<int>(subNode);
                        }
                        if(questNum != -1) {
                            questsCompleted.insert(std::make_pair(questNum, new QuestCompleted(numCompletions)));
                        }
                        subNode = subNode->next;
                    }
                }
                childNode = childNode->next;
            }
        } else {
            // New - New format!
            int id;
            while(childNode) {
                if(NODE_NAME(childNode, "QuestCompleted")) {
                    id = xml::getIntProp(childNode, "ID");

                    questsCompleted.insert(std::make_pair(id, new QuestCompleted(childNode)));

                }
                childNode = childNode->next;
            }
        }
    }
    else if(NODE_NAME(curNode, "Minions")) {
        childNode = curNode->children;
        while(childNode) {
            if(NODE_NAME(childNode, "Minion")) {
                minions.push_back(xml::getString(childNode));
            }
            childNode = childNode->next;
        }
    }
    else if(NODE_NAME(curNode, "Alchemy")) {
        xmlNodePtr subNode;
        childNode = curNode->children;
        while(childNode) {
            if(NODE_NAME(childNode, "KnownAlchemyEffects")) {
                subNode = childNode->children;
                while(subNode) {
                    if(NODE_NAME(subNode, "ObjectEffect")) {
                        knownAlchemyEffects.insert(std::make_pair(xml::getString(subNode), true));
                    }
                    subNode = subNode->next;
                }
            }
            childNode = childNode->next;
        }
    }
    else if(getVersion() < "2.42i") {
        if(NODE_NAME(curNode, "PkWon")) statistics.setPkwon(xml::toNum<unsigned long>(curNode));
        else if(NODE_NAME(curNode, "PkIn")) statistics.setPkin(xml::toNum<unsigned long>(curNode));
    } else if(getVersion() < "2.46l") {
        if(NODE_NAME(curNode, "LostExperience")) statistics.setExperienceLost(xml::toNum<unsigned long>(curNode));
    }

}


//*********************************************************************
//                      loadAnchors
//*********************************************************************
// Loads all anchors into the given anchors array

void Player::loadAnchors(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    int i=0;

    while(childNode) {
        if(NODE_NAME(childNode, "Anchor")) {
            i = xml::getIntProp(childNode, "Num");
            if(i >= 0 && i < MAX_DIMEN_ANCHORS && !anchor[i]) {
                anchor[i] = new Anchor;
                anchor[i]->load(childNode);
            }
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      loadRanges
//*********************************************************************
// Loads builder ranges into the provided creature

void loadRanges(xmlNodePtr curNode, Player *pPlayer) {
    xmlNodePtr childNode = curNode->children;
    int i=0;

    while(childNode) {
        if(NODE_NAME(childNode, "Range")) {
            i = xml::getIntProp(childNode, "Num");
            if(i >= 0 && i < MAX_BUILDER_RANGE)
                pPlayer->bRange[i].load(childNode);
        }
        childNode = childNode->next;
    }
}




//*********************************************************************
//                      saveToFile
//*********************************************************************
// This function will write the supplied player to Player.xml
// NOTE: For now, it will ignore equiped equipment, so be sure to
// remove the equiped equipment and put it in the inventory before
// calling this function otherwise it will be lost

int Player::saveToFile(LoadType saveType) {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char        filename[256];

    if( getName().empty() || !isPlayer())
        return(-1);

    if(getName()[0] == '\0') {
        std::clog << "Invalid player passed to save\n";
        return(-1);
    }

    gServer->saveIds();

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Player", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    escapeText();
    saveToXml(rootNode, ALLITEMS, LoadType::LS_FULL);

    if(saveType == LoadType::LS_BACKUP) {
        sprintf(filename, "%s/%s.bak.xml", Path::PlayerBackup, getCName());
    } else {
        sprintf(filename, "%s/%s.xml", Path::Player, getCName());
    }

    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
    return(0);
}


//*********************************************************************
//                      saveXml
//*********************************************************************

void Player::saveXml(xmlNodePtr curNode) const {
    xmlNodePtr childNode;
    int i;

    // record people logging off during swap
    if(gConfig->swapIsInteresting(this))
        gConfig->swapLog((std::string)"p" + getName(), false);

    bank.save("Bank", curNode);
    xml::saveNonZeroNum(curNode, "WeaponTrains", weaponTrains);
    xml::saveNonNullString(curNode, "Surname", surname);
    xml::saveNonNullString(curNode, "Forum", forum);
    xml::newNumChild(curNode, "Wrap", wrap);
    bound.save(curNode, "BoundRoom");
    previousRoom.save(curNode, "PreviousRoom"); // monster do not save PreviousRoom
    statistics.save(curNode, "Statistics");
    xml::saveNonZeroNum(curNode, "ActualLevel", actual_level);
    xml::saveNonZeroNum(curNode, "Created", created);
    xml::saveNonNullString(curNode, "OldCreated", oldCreated);
    xml::saveNonZeroNum(curNode, "Guild", guild);

    if(title != " ")
        xml::saveNonNullString(curNode, "Title", title);

    xml::saveNonZeroNum(curNode, "GuildRank", guildRank);

    if(isStaff()) {
        childNode = xml::newStringChild(curNode, "Ranges");
        for(i=0; i<MAX_BUILDER_RANGE; i++) {
            // empty range, skip it
            if(!bRange[i].low.id && !bRange[i].high)
                continue;
            bRange[i].save(childNode, "Range", i);
        }
    }

    xml::saveNonZeroNum(curNode, "NegativeLevels", negativeLevels);
    xml::saveNonZeroNum(curNode, "LastInterest", lastInterest);
    xml::saveNonZeroNum(curNode, "TickDmg", tickDmg);

    xml::saveNonNullString(curNode, "CustomColors", customColors);
    xml::saveNonZeroNum(curNode, "Thirst", thirst);
    saveBits(curNode, "Songs", gConfig->getMaxSong(), songs);

    std::list<CatRef>::const_iterator it;

    if(!storesRefunded.empty()) {
        childNode = xml::newStringChild(curNode, "StoresRefunded");
        for(it = storesRefunded.begin() ; it != storesRefunded.end() ; it++) {
            (*it).save(childNode, "Store", true);
        }
    }

    if(!roomExp.empty()) {
        childNode = xml::newStringChild(curNode, "RoomExp");
        for(it = roomExp.begin() ; it != roomExp.end() ; it++) {
            (*it).save(childNode, "Room", true);
        }
    }
    if(!objIncrease.empty()) {
        childNode = xml::newStringChild(curNode, "ObjIncrease");
        for(it = objIncrease.begin() ; it != objIncrease.end() ; it++) {
            (*it).save(childNode, "Object", true);
        }
    }
    if(!lore.empty()) {
        childNode = xml::newStringChild(curNode, "Lore");
        for(it = lore.begin() ; it != lore.end() ; it++) {
            (*it).save(childNode, "Info", true);
        }
    }
    std::list<int>::const_iterator rt;
    if(!recipes.empty()) {
        childNode = xml::newStringChild(curNode, "Recipes");
        for(rt = recipes.begin() ; rt != recipes.end() ; rt++) {
            xml::newNumChild(childNode, "Recipe", (*rt));
        }
    }

    // Save any Anchors
    for(i=0; i<MAX_DIMEN_ANCHORS; i++) {
        if(anchor[i]) {
            childNode = xml::newStringChild(curNode, "Anchors");
            for(; i<MAX_DIMEN_ANCHORS; i++)
                if(anchor[i]) {
                    xmlNodePtr subNode = xml::newStringChild(childNode, "Anchor");
                    xml::newNumProp(subNode, "Num", i);
                    anchor[i]->save(subNode);
                }
            break;
        }
    }

    xml::saveNonZeroNum(curNode, "Wimpy", wimpy);

    childNode = xml::newStringChild(curNode, "Pets");
    savePets(childNode);

    if(birthday) {
        childNode = xml::newStringChild(curNode, "Birthday");
        birthday->save(childNode);
    }

    xml::saveNonNullString(curNode, "LastPassword", lastPassword);
    xml::saveNonNullString(curNode, "PoisonedBy", poisonedBy);
    xml::saveNonNullString(curNode, "AfflictedBy", afflictedBy);

    xmlNodePtr alchemyNode = xml::newStringChild(curNode, "Alchemy");
    childNode = xml::newStringChild(alchemyNode, "KnownAlchemyEffects");
    for(const auto& p : knownAlchemyEffects) {
        if(p.second) {
            xml::newStringChild(childNode, "ObjectEffect", p.first);
        }
    }

}




//*********************************************************************
//                      saveQuests
//*********************************************************************

void Player::saveQuests(xmlNodePtr rootNode) const {
    xmlNodePtr questNode;

    questNode = xml::newStringChild(rootNode, "QuestsInProgress");
    for(std::pair<int, QuestCompletion*> p : questsInProgress) {
        p.second->save(questNode);
    }

    questNode = xml::newStringChild(rootNode, "QuestsCompleted");
    if(!questsCompleted.empty()) {
        for(auto qp : questsCompleted) {
            qp.second->save(questNode, qp.first);
        }
    }
}


//*********************************************************************
//                      load
//*********************************************************************

void PlayerClass::load(xmlNodePtr rootNode) {
//  // Stuff gained on each additional level
//  std::map<int, LevelGain*> levels;

    id = xml::getIntProp(rootNode, "id");
    xml::copyPropToString(name, rootNode, "Name");

    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "Title")) {
            titles[1] = new PlayerTitle;
            titles[1]->load(curNode);
        }
        else if(NODE_NAME(curNode, "UnarmedWeaponSkill")) xml::copyToString(unarmedWeaponSkill, curNode);
        else if(NODE_NAME(curNode, "NumProfs")) xml::copyToNum(numProf, curNode);
        else if(NODE_NAME(curNode, "NeedsDeity")) {
            int i=0;
            xml::copyToNum(i, curNode);
            if(i)
                needDeity = true;
        } else if(NODE_NAME(curNode, "Base")) {
            xmlNodePtr baseNode = curNode->children;
            while(baseNode) {
                if(NODE_NAME(baseNode, "Stats")) {
                    xmlNodePtr statNode = baseNode->children;
                    while(statNode) {
                        if(NODE_NAME(statNode, "Hp")) {
                            xml::copyToNum(baseHp, statNode);
                        } else if(NODE_NAME(statNode, "Mp")) {
                            xml::copyToNum(baseMp, statNode);
                        } else if(NODE_NAME(statNode, "Strength")) {
                            xml::copyToNum(baseStrength, statNode);
                            checkAutomaticStats();
                        } else if(NODE_NAME(statNode, "Dexterity")) {
                            xml::copyToNum(baseDexterity, statNode);
                            checkAutomaticStats();
                        } else if(NODE_NAME(statNode, "Constitution")) {
                            xml::copyToNum(baseConstitution, statNode);
                            checkAutomaticStats();
                        } else if(NODE_NAME(statNode, "Intelligence")) {
                            xml::copyToNum(baseIntelligence, statNode);
                            checkAutomaticStats();
                        } else if(NODE_NAME(statNode, "Piety")) {
                            xml::copyToNum(basePiety, statNode);
                            checkAutomaticStats();
                        }
                        statNode = statNode->next;
                    }
                } else if(NODE_NAME(baseNode, "Dice")) damage.load(baseNode);
                else if(NODE_NAME(baseNode, "Skills")) {
                    xmlNodePtr skillNode = baseNode->children;
                    while(skillNode) {
                        if(NODE_NAME(skillNode, "Skill")) {
                            auto* skillGain = new SkillGain(skillNode);
                            baseSkills.push_back(skillGain);
                        }
                        skillNode = skillNode->next;
                    }
                }
                baseNode = baseNode->next;
            }
        } else if(NODE_NAME(curNode, "Levels")) {
            xmlNodePtr levelNode = curNode->children;
            int lvl;
            while(levelNode) {
                if(NODE_NAME(levelNode, "Level")) {
                    lvl = xml::getIntProp(levelNode, "Num");
                    if(lvl > 0) {
                        levels[lvl] = new LevelGain(levelNode);
                    }

                    xmlNodePtr childNode = levelNode->children;
                    while(childNode) {
                        if(NODE_NAME(childNode, "Title")) {
                            titles[lvl] = new PlayerTitle;
                            titles[lvl]->load(childNode);
                        }
                        childNode = childNode->next;
                    }
                }
                levelNode = levelNode->next;
            }
        }

        curNode = curNode->next;

    }
}

//*********************************************************************
//                      loadClasses
//*********************************************************************

bool Config::loadClasses() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;

    char filename[80];
    snprintf(filename, 80, "%s/classes.xml", Path::Game);
    xmlDoc = xml::loadFile(filename, "Classes");

    if(xmlDoc == nullptr)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode))
        curNode = curNode->next;

    if(curNode == nullptr) {
        xmlFreeDoc(xmlDoc);
        return(false);
    }

    clearClasses();
    std::string className = "";
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Class")) {
            xml::copyPropToString(className, curNode, "Name");
            if(!className.empty()) {
                if(classes.find(className) == classes.end()) {
                    //printf("\n\tLoaded %s", className.c_str());
                    classes[className] = new PlayerClass(curNode);
                } else {
                    std::clog << "Error: Duplicate class: " << className.c_str() << std::endl;
                }
            }
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    //printf("\n");
    return(true);
}


//*********************************************************************
//                ProxyManager
//*********************************************************************

void ProxyManager::save() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr      rootNode;
    char            filename[80];


    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Proxies", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    for(ProxyMultiMap::value_type p : proxies) {
        p.second.save(rootNode);
    }

    sprintf(filename, "%s/proxies.xml", Path::PlayerData);

    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
}

void ProxyManager::loadProxies() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr  curNode;
    char        filename[80];

    sprintf(filename, "%s/proxies.xml", Path::PlayerData);

    if(!file_exists(filename))
        return;

    if((xmlDoc = xml::loadFile(filename, "Proxies")) == nullptr)
        return;

    rootNode = xmlDocGetRootElement(xmlDoc);
    curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "ProxyAccess")) {
            ProxyAccess access(curNode);
            proxies.insert(ProxyMultiMap::value_type(access.getProxiedId(), access));
        }
        curNode = curNode->next;
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
}


//*********************************************************************
//                      load
//*********************************************************************

void PlayerTitle::load(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Female")) xml::copyToString(female, curNode);
        else if(NODE_NAME(curNode, "Male")) xml::copyToString(male, curNode);
        curNode = curNode->next;
    }
}

ProxyAccess::ProxyAccess(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "ProxyId")) xml::copyToString(proxyId, curNode);
        else if (NODE_NAME(curNode, "ProxyName")) xml::copyToString(proxyName, curNode);
        else if (NODE_NAME(curNode, "ProxiedId")) xml::copyToString(proxiedId, curNode);
        else if (NODE_NAME(curNode, "ProxiedName")) xml::copyToString(proxiedName, curNode);

        curNode = curNode->next;
    }
}


void ProxyAccess::save(xmlNodePtr rootNode) {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "ProxyAccess");
    xml::newStringChild(curNode, "ProxiedId", proxiedId);
    xml::newStringChild(curNode, "ProxiedName", proxiedName);
    xml::newStringChild(curNode, "ProxyId", proxyId);
    xml::newStringChild(curNode, "ProxyName", proxyName);

}

