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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
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

#include "creatures.hpp"
#include "calendar.hpp"
#include "paths.hpp"
#include "xml.hpp"

bool loadPlayer(const bstring& name, Player** player, enum LoadType loadType) {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char        filename[256];
    bstring     pass = "", loadName = "";

    if(loadType == LoadType::LS_BACKUP)
        sprintf(filename, "%s/%s.bak.xml", Path::PlayerBackup, name.c_str());
    else if(loadType == LoadType::LS_CONVERT)
        sprintf(filename, "%s/convert/%s.xml", Path::Player, name.c_str());
    else // LoadType::LS_NORMAL
        sprintf(filename, "%s/%s.xml", Path::Player, name.c_str());

    if((xmlDoc = xml::loadFile(filename, "Player")) == nullptr)
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
    xml::copyPropToBString(pass, rootNode, "Password");
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
    else if(NODE_NAME(curNode, "Surname")) xml::copyToBString(surname, curNode);
    else if(NODE_NAME(curNode, "Wrap")) xml::copyToNum(wrap, curNode);
    else if(NODE_NAME(curNode, "Forum")) xml::copyToBString(forum, curNode);
    else if(NODE_NAME(curNode, "Ranges"))
        loadRanges(curNode, this);
    else if(NODE_NAME(curNode, "Wimpy")) setWimpy(xml::toNum<unsigned short>(curNode));

    else if(NODE_NAME(curNode, "Songs")) {
        loadBits(curNode, songs);
    }
    else if(NODE_NAME(curNode, "Anchors")) {
        loadAnchors(curNode);
    }

    else if(NODE_NAME(curNode, "LastPassword")) xml::copyToBString(lastPassword, curNode);
    else if(NODE_NAME(curNode, "PoisonedBy")) xml::copyToBString(poisonedBy, curNode);
    else if(NODE_NAME(curNode, "AfflictedBy")) xml::copyToBString(afflictedBy, curNode);
    else if(NODE_NAME(curNode, "ActualLevel")) setActualLevel(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "WeaponTrains")) setWeaponTrains(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "LastMod")) xml::copyToBString(oldCreated, curNode);
    else if(NODE_NAME(curNode, "Created")) {
        if(getVersion() < "2.43c")
            xml::copyToBString(oldCreated, curNode);
        else
            xml::copyToNum(created, curNode);
    }
    else if(NODE_NAME(curNode, "Guild")) setGuild(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "GuildRank")) setGuildRank(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "TickDmg")) setTickDamage(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "NegativeLevels")) setNegativeLevels(xml::toNum<unsigned short>(curNode));
    else if(NODE_NAME(curNode, "LastInterest")) setLastInterest(xml::toNum<long>(curNode));
    else if(NODE_NAME(curNode, "Title")) setTitle(xml::getBString(curNode));
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
                minions.push_back(xml::getBString(childNode));
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
                        knownAlchemyEffects.insert(std::make_pair(xml::getBString(subNode), true));
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



