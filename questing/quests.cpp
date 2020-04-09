/*
 * Quests.cpp
 *   New questing system
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
#include "commands.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "factions.hpp"
#include "mud.hpp"
#include "quests.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "tokenizer.hpp"
#include "xml.hpp"
#include "objects.hpp"

QuestCatRef::QuestCatRef() {
    area = gConfig->defaultArea;
    reqNum = 1;
    curNum = 0;
    id = 0;
}

QuestCatRef::QuestCatRef(xmlNodePtr rootNode) {
    // Set up the defaults
    reqNum = 1;
    curNum = 0;
    area = gConfig->defaultArea;

    // And then read in the XML file
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
            if(NODE_NAME(curNode, "Area")) xml::copyToBString(area, curNode);
        else if(NODE_NAME(curNode, "Id")) xml::copyToNum(id, curNode);
        else if(NODE_NAME(curNode, "ReqAmt")) xml::copyToNum(reqNum, curNode);
        else if(NODE_NAME(curNode, "CurAmt")) xml::copyToNum(curNum, curNode);

        curNode = curNode->next;
    }
}
xmlNodePtr QuestCatRef::save(xmlNodePtr rootNode, bstring saveName) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, saveName.c_str());

    xml::newStringChild(curNode, "Area", area);
    xml::newNumChild(curNode, "Id", id);
    xml::newNumChild(curNode, "ReqAmt", reqNum);
    xml::newNumChild(curNode, "CurAmt", curNum);
    return(curNode);
}

QuestInfo::QuestInfo(xmlNodePtr rootNode) {
    bstring faction = "";

    questId = xml::getIntProp(rootNode, "Num");
    repeatable = sharable = false;
    expReward = minLevel = minFaction = level = 0;
    repeatFrequency = QuestRepeatFrequency::REPEAT_NEVER;

    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
            if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
        else if(NODE_NAME(curNode, "Revision")) xml::copyToBString(revision, curNode);
        else if(NODE_NAME(curNode, "Description")) xml::copyToBString(description, curNode);
        else if(NODE_NAME(curNode, "ReceiveString")) xml::copyToBString(receiveString, curNode);
        else if(NODE_NAME(curNode, "CompletionString")) xml::copyToBString(completionString, curNode);
        else if(NODE_NAME(curNode, "TimesRepeatable")) xml::copyToNum(timesRepeatable, curNode);
        else if(NODE_NAME(curNode, "RepeatFrequency")) xml::copyToNum<QuestRepeatFrequency>(repeatFrequency, curNode);
        else if(NODE_NAME(curNode, "Sharable")) xml::copyToBool(sharable, curNode);
        else if(NODE_NAME(curNode, "TurnIn")) turnInMob = QuestCatRef(curNode);
        else if(NODE_NAME(curNode, "Level")) xml::copyToNum(level, curNode);
        else if(NODE_NAME(curNode, "MinLevel")) xml::copyToNum(minLevel, curNode);
        else if(NODE_NAME(curNode, "MinFaction")) xml::copyToNum(minFaction, curNode);
        else if(NODE_NAME(curNode, "Prerequisites")) {
            xmlNodePtr childNode = curNode->children;
            int preReq=0;
            while(childNode) {
                if(NODE_NAME(childNode, "Prerequisite"))
                    if((preReq = xml::toNum<int>(childNode)) != 0)
                        preRequisites.push_back(preReq);

                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "Initial")) {
            xmlNodePtr childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Object")) initialItems.push_back(QuestCatRef(childNode));

                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "Requirements")) {
            xmlNodePtr childNode = curNode->children;
            while(childNode) {
                    if(NODE_NAME(childNode, "Object")) itemsToGet.push_back(QuestCatRef(childNode));
                else if(NODE_NAME(childNode, "Monster")) mobsToKill.push_back(QuestCatRef(childNode));
                else if(NODE_NAME(childNode, "Room")) roomsToVisit.push_back(QuestCatRef(childNode));

                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "Rewards")) {
            xmlNodePtr childNode = curNode->children;
            while(childNode) {
                    if(NODE_NAME(childNode, "Coins")) cashReward.load(childNode);
                else if(NODE_NAME(childNode, "Experience")) xml::copyToNum(expReward, childNode);
                else if(NODE_NAME(childNode, "Object")) itemRewards.push_back(QuestCatRef(childNode));
                else if(NODE_NAME(childNode, "Faction")) {
                    xml::copyPropToBString(faction, childNode, "id");
                    if(faction != "")
                        factionRewards[faction] = xml::toNum<long>(childNode) * -1;
                }

                childNode = childNode->next;
            }
        }
        curNode = curNode->next;
    }

    if (repeatFrequency != QuestRepeatFrequency::REPEAT_NEVER)
        repeatable = true;

}


QuestCompletion::QuestCompletion(QuestInfo* parent, Player* player) {
    questId = parent->getId();
    parentQuest = parent;
    parentPlayer = player;
    revision = parent->revision;

    for(QuestCatRef & qcr : parent->mobsToKill)
        mobsKilled.push_back(qcr);

    for(QuestCatRef & qcr : parent->roomsToVisit)
        roomsVisited.push_back(qcr);
    mobsCompleted = false;
    itemsCompleted = false;
    roomsCompleted = false;
}


QuestCompletion::QuestCompletion(xmlNodePtr rootNode, Player* player) {
    questId = xml::getIntProp(rootNode, "ID");
    resetParentQuest();

    parentPlayer = player;

    mobsCompleted = false;
    itemsCompleted = false;
    roomsCompleted = false;

    xmlNodePtr curNode = rootNode->children;
    xmlNodePtr childNode;
    while(curNode) {
        if(NODE_NAME(curNode, "Revision")) xml::copyToBString(revision, curNode);
        else if(NODE_NAME(curNode, "MobsKilled")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "QuestCatRef")) {
                    mobsKilled.push_back(QuestCatRef(childNode));
                }
                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "RoomsVisited")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "QuestCatRef")) {
                    roomsVisited.push_back(QuestCatRef(childNode));
                }
                childNode = childNode->next;
            }
        }
        curNode = curNode->next;
    }

    checkQuestCompletion(false);
}

xmlNodePtr QuestCompletion::save(xmlNodePtr rootNode) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "QuestCompletion");
    xml::newNumProp(curNode, "ID", questId);

    // We don't actually read this in, but we'll save it for reference incase
    // someone edits the file and wants to know this info
    xml::newStringChild(curNode, "QuestName", parentQuest->getName());
    xml::newStringChild(curNode, "Revision", revision);
    if(!mobsKilled.empty()) {
        xmlNodePtr mobsNode = xml::newStringChild(curNode, "MobsKilled");
        for(const QuestCatRef & mob : mobsKilled) {
            mob.save(mobsNode);
        }
    }
    if(!roomsVisited.empty()) {
        xmlNodePtr roomsNode = xml::newStringChild(curNode, "RoomsVisited");
        for(const QuestCatRef & rom : roomsVisited) {
            rom.save(roomsNode);
        }
    }
    return(curNode);
}

QuestCompleted::QuestCompleted(const QuestCompleted &qc) {
    times = qc.times;
    lastCompleted = qc.lastCompleted;
}

QuestCompleted::QuestCompleted(xmlNodePtr rootNode) {
    init();

    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "Times")) xml::copyToNum(times, curNode);
        else if(NODE_NAME(curNode, "LastCompleted")) xml::copyToNum(lastCompleted, curNode);

        curNode = curNode->next;
    }
}

xmlNodePtr QuestCompleted::save(xmlNodePtr rootNode, int id) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "QuestCompleted");
    xml::newNumProp(curNode, "ID", id);
    xml::newNumChild(curNode, "Times", times);
    xml::newNumChild(curNode, "LastCompleted", lastCompleted);

    return(curNode);
}


void QuestCompletion::resetParentQuest() {
    if((parentQuest = gConfig->getQuest(questId)) == nullptr) {
        throw(std::runtime_error("Unable to find parent quest - " + bstring(questId)));
    }
}

QuestInfo* QuestCompletion::getParentQuest() const {
    return(parentQuest);
}

TalkResponse::TalkResponse() {
    quest = nullptr;
}

TalkResponse::TalkResponse(xmlNodePtr rootNode) {
    quest = nullptr;

    // And then read in the XML file
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
            if(NODE_NAME(curNode, "Keyword")) keywords.push_back(xml::getBString(curNode).toLower());
        else if(NODE_NAME(curNode, "Response")) xml::copyToBString(response, curNode);
        else if(NODE_NAME(curNode, "Action")) {
                xml::copyToBString(action, curNode);
                parseQuest();
            }

        curNode = curNode->next;
    }
}

xmlNodePtr TalkResponse::saveToXml(xmlNodePtr rootNode) const {
    xmlNodePtr talkNode = xml::newStringChild(rootNode, "TalkResponse");

    for(const bstring&  keyword : keywords) {
        xml::newStringChild(talkNode, "Keyword", keyword);
    }
    xml::newStringChild(talkNode, "Response", response);
    xml::newStringChild(talkNode, "Action", action);
    return(talkNode);
}

//*****************************************************************************
//                      parseQuest
//*****************************************************************************

void TalkResponse::parseQuest() {
    boost::tokenizer<> tok(action);
    auto it = tok.begin();

    if(it == tok.end())
        return;

    bstring cmd = *it++;

    if(cmd.equals("quest", false)) {
        if(it == tok.end())
            return;

        // Find the quest number
        bstring num = *it++;
        int questNum = num.toInt();

        if(questNum < 1)
            return;

        QuestInfo* questInfo = gConfig->getQuest(questNum);
        if(!questInfo)
            return;

        quest = questInfo;

    }

    return;
}

bool QuestInfo::isRepeatable() const {
    return(repeatable);
}
int QuestInfo::getTimesRepeatable() const {
    return(timesRepeatable);
}

const QuestCatRef& QuestInfo::getTurnInMob() const {
    return(turnInMob);
}
bstring QuestInfo::getName() const {
    return(name);
}
int QuestInfo::getId() const {
    return(questId);
}
bstring QuestInfo::getDisplayName() const {
    std::ostringstream displayStr;
    displayStr << "^Y#" << questId << " - " << name << "^x";
    return(displayStr.str());
}

bstring QuestInfo::getDisplayString() const {
    std::ostringstream displayStr;
    std::map<bstring, long>::const_iterator it;
    int i = 0;

    displayStr << getDisplayName();
    if(repeatable)
        displayStr << " ^G*Repeatable*^x";
    if(sharable)
        displayStr << " ^Y*Sharable*^x";
    displayStr << std::endl;
    displayStr << "^WDescription: ^x" << description << "^x\n";
    bstring temp = "";

    temp = receiveString;
    temp.Replace("*CR", "\n");
    displayStr << "^WReceiveString: ^x" << temp << "\n";

    temp = completionString;
    temp.Replace("*CR", "\n");
    displayStr << "^WCompletionString: ^x" << temp << "\n";

    if(!preRequisites.empty()) {
        QuestInfo* q=0;
        int t = 0;
        displayStr << "^WPrerequisites:^x";
        for(const int & preReq : preRequisites) {
            q = gConfig->getQuest(preReq);
            if(q) {
                if(t++ > 0)
                    displayStr << ", ";
                else displayStr << " ";
                displayStr << q->getDisplayName();
            }
        }
        displayStr << ".\n";
    }
    if(minLevel)
        displayStr << "^WMinimum Level:^x " << minLevel << "\n";

    if(minFaction)
        displayStr << "^WMinimum Faction:^x " << minFaction << "\n";

    Monster* endMonster = 0;
    displayStr << "^RTurn in Monster:^x ";
    if(loadMonster(turnInMob, &endMonster)) {
         displayStr << endMonster->getName();
         delete endMonster;
    } else {
        displayStr << "ERROR: Monster doesn't exit!";
    }
    displayStr << "(" << turnInMob.str() << ")\n";


    if(!initialItems.empty()) {
        Object* object;
        displayStr << "\t^WInitial Items:^x\n";
        i = 1;
        for(const QuestCatRef & obj : initialItems) {
            displayStr << "\t\t^W" << i++ << ")^x #" << obj.str() << " - ";
            if(loadObject(obj, &object)) {
                 displayStr << object->getName();
                 delete object;
            } else {
                displayStr << "ERROR: Object doesn't exist!";
            }
            displayStr << "(" << obj.reqNum << ")" << std::endl;
        }
    }


    if(!mobsToKill.empty()) {
        Monster* monster;
        displayStr << "\t^WMonsters To Kill:^x\n";
        i = 1;
        for(const QuestCatRef & mob : mobsToKill) {
            displayStr << "\t\t^W" << i++ << ")^x #" << mob.str() << " - ";
            if(loadMonster(mob, &monster)) {
                displayStr << monster->getName();
                delete monster;
            } else {
                displayStr << "ERROR: Monster doesn't exist!";
            }
            displayStr << "(" << mob.reqNum << ")" << std::endl;
        }
    }
    if(!itemsToGet.empty()) {
        Object* object;
        displayStr << "\t^WObjects To Obtain:^x\n";
        i = 1;
        for(const QuestCatRef & obj : itemsToGet) {
            displayStr << "\t\t^W" << i++ << ")^x #" << obj.str() << " - ";
            if(loadObject(obj, &object)) {
                 displayStr << object->getName();
                 delete object;
            } else {
                displayStr << "ERROR: Object doesn't exist!";
            }
            displayStr << "(" << obj.reqNum << ")" << std::endl;
        }
    }
    if(!roomsToVisit.empty()) {
        UniqueRoom* room;
        displayStr << "\t^WRooms To Visit:^x\n";
        i = 1;
        for(const QuestCatRef & rom : roomsToVisit) {
            displayStr << "\t\t^W" << i++ << ")^x #" << rom.str() << " - ";
            if(loadRoom(rom, &room)) {
                displayStr << room->getName() << std::endl;
            } else {
                 displayStr << "ERROR: Room doesn't exist!" << std::endl;
            }
        }
    }

    displayStr << "\t^WRewards:^x\n";
    displayStr.imbue(std::locale(""));
    if(!cashReward.isZero())
        displayStr << "\t\t^WCash:^x " << cashReward.str() << std::endl;
    if(expReward)
        displayStr << "\t\t^WExp:^x " << expReward << std::endl;
    if(!itemRewards.empty()) {
        Object* object;
        i = 1;
        displayStr << "\t\t^WItems:^x" << std::endl;
        for(const QuestCatRef & obj : itemRewards) {
            displayStr << "\t\t\t^W" << i++ << ")^x #" << obj.str() << " - ";
            if(loadObject(obj, &object)) {
                 displayStr << object->getName();
                 delete object;
            } else {
                displayStr << "ERROR: Object doesn't exist!";
            }
            displayStr << "(" << obj.reqNum << ")";
            if(obj.curNum == -1)
                displayStr << " ^Y*Hidden Reward^x";
            displayStr << std::endl;
        }
    }
    if(!factionRewards.empty()) {
        displayStr << "\t\t^WFactions:^x" << std::endl;
        for(it = factionRewards.begin(); it != factionRewards.end(); it++) {
            displayStr << "\t\t   " << (*it).first << ": " << (*it).second << std::endl;
        }
    }

    return(displayStr.str());
}


void Config::clearQuests() {
    // Only to be used on cleanup
    std::map<int, QuestInfo*>::iterator it;
    QuestInfo* quest;

    for(it = quests.begin(); it != quests.end(); it++) {
        quest = (*it).second;
        delete quest;
    }
    quests.clear();
}

bool Config::loadQuests() {
    // You can update quests, but you can't delete them!
    xmlDocPtr   xmlDoc;
    char filename[256];

    sprintf(filename, "%s/quests.xml", Path::Game);
    if((xmlDoc = xml::loadFile(filename, "Quests")) == nullptr)
        return(false);

    xmlNodePtr rootNode = xmlDocGetRootElement(xmlDoc);
    xmlNodePtr curNode = rootNode->children;
    int questId = 0;

    while(curNode) {
        if(NODE_NAME(curNode, "QuestInfo")) {
            questId = xml::getIntProp(curNode, "Num");
            if(questId > 0) {
                QuestInfo* tmpQuest = new QuestInfo(curNode);

                if(quests[questId] == nullptr){
                    quests[questId] = tmpQuest;
                } else {
                    (*quests[questId]) = *tmpQuest;
                    delete tmpQuest;
                }
            }
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

QuestInfo* Config::getQuest(int questNum) {
    auto questIt = quests.find(questNum);
    if(questIt != quests.end())
        return(questIt->second);
    else
        return(nullptr);

}

bool Player::addQuest(QuestInfo* toAdd) {
    if(hasQuest(toAdd))
        return(false);
    questsInProgress[toAdd->getId()] = new QuestCompletion(toAdd, this);
    printColor("^W%s^x has been added to your quest book.\n", toAdd->getName().c_str());
    return(true);
}
bool Player::hasQuest(int questId) const {
    return(questsInProgress.find(questId) != questsInProgress.end());
}

bool Player::hasQuest(const QuestInfo *quest) const {
    return(quest && hasQuest(quest->getId()));
}

bool Player::hasDoneQuest(int questId) const {
    return(questsCompleted.find(questId) != questsCompleted.end());
}

bool Monster::hasQuests() const {
    return quests.size() > 0;
}

QuestEligibility Monster::getEligibleQuestDisplay(const Creature* viewer) const {
    if (!viewer)
        return QuestEligibility::INELIGIBLE;

    const Player *pViewer = viewer->getAsConstPlayer();
    bool hasRepetable = false;
    bool needsMoreTime = false;
    bool isLowLevel = false;

    if (!pViewer)
        return QuestEligibility::INELIGIBLE;

    for(auto quest : quests) {
        QuestEligibility eligible = quest->getEligibility(pViewer, this);
        if (eligible == QuestEligibility::ELIGIBLE)
            return QuestEligibility::ELIGIBLE;
        else if (eligible == QuestEligibility::ELIGIBLE_DAILY)
            hasRepetable = true;
        else if (eligible == QuestEligibility::ELIGIBLE_WEEKLY)
            hasRepetable = true;
        else if (eligible == QuestEligibility::INELIGIBLE_LEVEL)
            isLowLevel = true;
        else if (eligible == QuestEligibility::INELIGIBLE_DAILY_NOT_EXPIRED)
            needsMoreTime = true;
        else if (eligible == QuestEligibility::INELIGIBLE_WEEKLY_NOT_EXPIRED)
            needsMoreTime = true;
    }

    if (hasRepetable)
        return QuestEligibility::ELIGIBLE_DAILY;

    if (needsMoreTime)
        return QuestEligibility::INELIGIBLE_DAILY_NOT_EXPIRED;

    if (isLowLevel)
        return QuestEligibility::INELIGIBLE_LEVEL;

    return QuestEligibility::INELIGIBLE;
}

QuestTurninStatus Monster::checkQuestTurninStatus(const Creature* viewer) const {
    if (!viewer)
        return QuestTurninStatus::NO_TURNINS;

    const Player *pViewer = viewer->getAsConstPlayer();
    bool hasUncompletedTurnin = false;
    bool hasRepeatableTurnin = false;

    QuestCompletion* quest;
    for(auto p : pViewer->questsInProgress) {
        quest = p.second;
        if (info == quest->getParentQuest()->getTurnInMob()) {
            // We're the turnin for this quest
            hasUncompletedTurnin = true;
            if(quest->checkQuestCompletion(false)) {

                QuestInfo* parentQuest = quest->getParentQuest();
                if(parentQuest->isRepeatable()) {
                    QuestCompleted* completed = pViewer->getQuestCompleted(parentQuest->getId());
                    if(completed) {
                        // We've aready done this at least once
                        hasRepeatableTurnin = true;
                        continue;
                    }
                }
                // If Daily && !first
                // hasRpeatableTurnin = true
                return QuestTurninStatus::COMPLETED_TURNINS;
            }
        }
    }
    if (hasRepeatableTurnin)
        return QuestTurninStatus::COMPLETED_DAILY_TURNINS;

    if (hasUncompletedTurnin)
        return QuestTurninStatus::UNCOMPLETED_TURNINS;

    return QuestTurninStatus::NO_TURNINS;
}

QuestCompleted* Player::getQuestCompleted(const int questId) const {
    auto completion = questsCompleted.find(questId);
    if (completion != questsCompleted.end()) {
        return completion->second;
    } else {
        return nullptr;
    }
}

void Player::updateMobKills(Monster* monster) {
    for(std::pair<int, QuestCompletion*> p : questsInProgress) {
        QuestCompletion* quest = p.second;
        quest->updateMobKills(monster);
    }
}

void Player::updateItems(Object* object) {
    for(std::pair<int, QuestCompletion*> p : questsInProgress) {
        QuestCompletion* quest = p.second;
        quest->updateItems(object);
    }
}
void Player::updateRooms(UniqueRoom* room) {
    for(std::pair<int, QuestCompletion*> p : questsInProgress) {
        QuestCompletion* quest = p.second;
        quest->updateRooms(room);
    }
}
void QuestCompletion::updateMobKills(Monster* monster) {
    //std::list<QuestCatRef> mobsKilled;
    for(QuestCatRef & qcr : mobsKilled) {
        if(qcr == monster->info) {
            if(qcr.reqNum != qcr.curNum) {
                if(++(qcr.curNum) == qcr.reqNum) {
                    parentPlayer->printColor("Quest Update: %s - Required number of ^W%s^x have been killed.\n",
                            parentQuest->getName().c_str(), monster->getCName());
                } else {
                    parentPlayer->printColor("Quest Update: %s - Killed ^W %s / %s^x.\n",
                            parentQuest->getName().c_str(), intToText(qcr.curNum).c_str(),
                            monster->getCrtStr(parentPlayer, INV, qcr.reqNum).c_str());
                }
            }
        }
    }
    checkQuestCompletion();
}

// Called when adding an item to inventory, but before it is put in the list of items
void QuestCompletion::updateItems(Object* object) {
    for(QuestCatRef & qcr : parentQuest->itemsToGet) {
        if(qcr == object->info && object->isQuestOwner(parentPlayer)) {
            int curNum = parentPlayer->countItems(qcr)-1;

            if(curNum < qcr.reqNum) {
                itemsCompleted = false;
                if(++curNum == qcr.reqNum) {
                    parentPlayer->printColor("Quest Update: %s - Required number of ^W%s^x have been obtained.\n",
                        parentQuest->getName().c_str(), object->getCName());
                } else {
                    parentPlayer->printColor("Quest Update: %s - Obtained ^W%s / %s^x.\n",
                        parentQuest->getName().c_str(), intToText(curNum).c_str(),
                        object->getObjStr(parentPlayer, INV, qcr.reqNum).c_str());
                }
            }
        }
    }
    checkQuestCompletion();
}
void QuestCompletion::updateRooms(UniqueRoom* room) {
    if(roomsCompleted)
        return;
    for(QuestCatRef & qcr : roomsVisited) {
        if(qcr == room->info) {
            if(qcr.curNum != 1) {
                if(++qcr.curNum == 1) {
                    // ReqNum should always be one
                    parentPlayer->printColor("Quest Update: %s - Visited ^W%s^x.\n",
                            parentQuest->getName().c_str(), room->getCName());
                }
            }
        }
    }
    checkQuestCompletion();
}
bool QuestCompletion::checkQuestCompletion(bool showMessage) {
    bool alreadyCompleted = mobsCompleted && itemsCompleted && roomsCompleted;

    // First see if we have killed all required monsters
    if(mobsCompleted == false) {
        // Variable is false, so check the function
        if(!hasRequiredMobs())
            mobsCompleted = false;
        else
            mobsCompleted = true;
    }
    // Now check to see if we've been to all of the rooms we should have
    if(roomsCompleted == false) {
        // Nope, so check the function
        if(!hasRequiredRooms())
            roomsCompleted = false;
        else
            roomsCompleted = true;
    }

    // Now see if we still have the required items
    bool itemsDone = hasRequiredItems();
    if(itemsCompleted == true && itemsDone == true) {
        itemsCompleted = true;
    } else if(itemsDone == false) {
        itemsCompleted = false;
        return(false);
    } else { //if(itemsDone == true) {
        itemsCompleted = true;
    }

    if(mobsCompleted && roomsCompleted && itemsCompleted) {
        if(showMessage && !alreadyCompleted) {
            Monster* endMonster = 0;

            parentPlayer->printColor("You have fufilled all of the requirements for ^W%s^x!\n", parentQuest->getName().c_str());
            if(loadMonster(parentQuest->turnInMob, &endMonster)) {
                parentPlayer->printColor("Return to ^W%s^x to claim your reward.\n", endMonster->getCName());
                delete endMonster;
            }
        }
        return(true);
    }
    return(false);

}

bool QuestCompletion::hasRequiredMobs() const {
    // Assume completion, and test for non completion
    for(const QuestCatRef & mob : mobsKilled) {
        if(mob.curNum != mob.reqNum) {
            return(false);
        }
    }
    return(true);
}
bool QuestCompletion::hasRequiredItems() const {
    int curNum;
    for(const QuestCatRef & obj : parentQuest->itemsToGet) {
        curNum = parentPlayer->countItems(obj);
        curNum = MIN(curNum, obj.reqNum);
        if(curNum != obj.reqNum) {
            return(false);
        }
    }
    return(true);
}
bool QuestCompletion::hasRequiredRooms() const {
    // Assume completion, and test for non completion
    for(const QuestCatRef & rom : roomsVisited) {
        if(rom.curNum != 1) {
            return(false);
        }
    }
    return(true);
}
bstring QuestCompletion::getStatusDisplay() {
    std::ostringstream displayStr;
    int i;

    checkQuestCompletion(false);

    if(mobsCompleted && itemsCompleted && roomsCompleted)
        displayStr << "^Y*";
    else
        displayStr << "^y";
    displayStr << parentQuest->name << "^x\n";
    displayStr << parentQuest->name << "^x\n";


    displayStr << "^WDescription: ^w" << parentQuest->description << "^x\n";

    Monster* endMonster = 0;
    displayStr << "^RWhen finished, return to: ^x";
    if(loadMonster(parentQuest->turnInMob, &endMonster)) {
         displayStr << endMonster->getName();
         delete endMonster;
    } else {
        displayStr << "ERROR: Monster doesn't exit!";
    }
    displayStr << "^x\n";

    if(!mobsKilled.empty()) {
        displayStr << "     ";
        if(mobsCompleted)
            displayStr << "^Y*";
        else
            displayStr << "^y";
        displayStr << "Monsters to Kill.^x\n";
        Monster* monster;
        i = 1;
        for(QuestCatRef & mob : mobsKilled) {
            displayStr << "\t";
            if(mob.curNum == mob.reqNum)
                displayStr << "^C*";
            else
                displayStr << "^c";

            displayStr << i++ << ") ";
            if(loadMonster(mob, &monster)) {
                displayStr << monster->getName();
                delete monster;
            } else {
                displayStr << "ERROR: Unable to load monster";
            }
            displayStr << " " << mob.curNum << "/" << mob.reqNum << "^x\n";

        }
    }
    if(!parentQuest->itemsToGet.empty()) {
        displayStr << "     ";
        if(itemsCompleted)
            displayStr << "^Y*";
        else
            displayStr << "^y";

        displayStr << "Objects to Get.^x\n";
        Object* object;
        i = 1;
        for(QuestCatRef & obj : parentQuest->itemsToGet) {
            int curNum = parentPlayer->countItems(obj);
            curNum = MIN(curNum, obj.reqNum);
            displayStr << "\t";
            if(curNum == obj.reqNum)
                displayStr << "^C*";
            else
                displayStr << "^c";

            displayStr << i++ << ") ";
            if(loadObject(obj, &object)) {
                displayStr << object->getName();
                delete object;
            } else {
                displayStr << "ERROR: Unable to load object";
            }
            displayStr << " " << curNum << "/" << obj.reqNum << "^x\n";
        }
    }
    if(!roomsVisited.empty()) {
        displayStr << "     ";
        if(roomsCompleted)
            displayStr << "^Y*";
        else
            displayStr << "^y";

        displayStr << "Rooms to Visit.^x\n";
        UniqueRoom* room;
        i = 1;
        for(QuestCatRef & rom : roomsVisited) {
            displayStr << "\t";
            if(rom.curNum == rom.reqNum)
                displayStr << "^C*";
            else
                displayStr << "^c";

            displayStr << i++ << ") ";
            if(loadRoom(rom, &room)) {
                displayStr << room->getName();
            } else {
                displayStr << "ERROR: Unable to load room";
            }
            displayStr << std::endl;
        }
    }

    displayStr << "^WRewards:^x\n";
    displayStr.imbue(std::locale(""));
    if(!parentQuest->cashReward.isZero())
        displayStr << "     ^WCash:^x " << parentQuest->cashReward.str() << std::endl;
    if(!parentQuest->itemRewards.empty()) {
        Object* object;
        i = 1;
        displayStr << "     ^WItems:^x" << std::endl;
        for(QuestCatRef & obj : parentQuest->itemRewards) {
            // Hidden Reward
            if(obj.curNum == -1)
                continue;
            displayStr << "          ^W" << i++ << ")^x ";
            if(loadObject(obj, &object)) {
                 displayStr << object->getName();
                 delete object;
            } else {
                displayStr << "ERROR: Object doesn't exist!";
            }
            if(obj.reqNum > 1)
                displayStr << "(" << obj.reqNum << ")";
            displayStr << std::endl;
        }
    }

    return(displayStr.str());

}

int cmdQuestStatus(Player* player, cmd* cmnd) {
    Player* target = player;
    int i = 1;

    for(std::pair<int, QuestCompletion*> p : target->questsInProgress) {
        QuestCompletion* quest = p.second;
        player->print("%d) %s\n", i++, quest->getStatusDisplay().c_str());
    }
    return(1);
}
bool Object::isQuestValid() const {
    return((type == ObjectType::CONTAINER && shotsCur == 0) ||
     (type != ObjectType::CONTAINER && (shotsCur != 0 || shotsMax == 0)) );

}
// Count how many of a given item this player has that are non-broken
int Player::countItems(const QuestCatRef& obj) {
    int total=0;
    for(Object* object : objects) {
        // Items only count if they're a bag and have 0 shots, or if
        // they're not a bag, and don't have 0 shots (unless shotsmax is 0)
        if(object && object->info == obj && object->isQuestValid())
            total++;

        if(object && object->getType() == ObjectType::CONTAINER) {
            for(Object* subObj : object->objects) {
                if(subObj->info == obj  && subObj->isQuestValid())
                    total++;
            }
        }
    }
    return(total);
}

bool prepareItemList(const Player* player, std::list<Object*> &objects, Object* object, const Monster* monster, bool isTrade, bool setTradeOwner, int totalBulk);

bool QuestCompletion::complete(Monster* monster) {
    std::map<bstring, long>::const_iterator it;
    std::list<Object*> objects;
    std::list<Object*>::iterator oIt;

    if(!checkQuestCompletion()) {
        parentPlayer->print("You have not fulfilled all the requirements to complete this quest.\n");
        return(false);
    }

    // if they can't get all of the item rewards, they can't complete the quest
    for(QuestCatRef & obj : parentQuest->itemRewards) {
        int num = obj.reqNum, cur = 0;
        Object* object=0;
        if(num <= 0)
            continue;

        while(cur++ < num) {
            if(loadObject(obj, &object)) {
                if(!prepareItemList(parentPlayer, objects, object, monster, false, true, 0))
                    return(false);
            }
        }
    }

    // Remove this quest from the list of quests so when we add items to the player
    // it doesn't try to update this quest.  We will delete this quest at the end of the
    // function right before we return

    parentPlayer->questsInProgress.erase(parentQuest->questId);
    parentQuest->printCompletionString(parentPlayer, monster);

    // First, remove all of the items from the player
    for(QuestCatRef & obj : parentQuest->itemsToGet) {
        bstring oName;
        Object *object;
        ObjectSet::iterator it;
        int num = obj.reqNum;

        for( it = parentPlayer->objects.begin() ; it != parentPlayer->objects.end() && num > 0 ; ) {
            object = (*it++);
            if(object->info == obj && object->isQuestValid()) {
                oName = object->getName();
                parentPlayer->delObj(object, true, false, true, false);
                delete object;
                num--;
                continue;
            }
            if(object->getType() == ObjectType::CONTAINER) {
                Object *subObject;
                ObjectSet::iterator sIt;
                for(sIt = object->objects.begin() ; sIt != object->objects.end() && num > 0 ; ) {
                    subObject = (*sIt++);
                    if(subObject->info == obj && subObject->isQuestValid()) {
                        oName = subObject->getName();
                        parentPlayer->delObj(subObject, true, false, true, false);
                        delete subObject;
                        num--;
                        continue;
                    }
                }
            }
        }
        if(loadObject(obj, &object)) {
            parentPlayer->printColor("%M takes ^W%s^x from you.\n", monster,
                object->getObjStr(parentPlayer, 0, obj.reqNum).c_str());
            delete object;
        }
    }

    for(oIt = objects.begin(); oIt != objects.end(); oIt++) {
        parentPlayer->printColor("%M gives you ^C%P^x.\n", monster, *oIt);
        (*oIt)->setDroppedBy(monster, "QuestCompletion");
        doGetObject(*oIt, parentPlayer);
    }

    parentPlayer->checkDarkness();

    //float multiplier = 1.0;
    // TODO: Adjust multiplier based on level difference
    if(parentQuest->level != 0) {
        // adjust multiplier here
    }

    if(!parentQuest->cashReward.isZero()) {
        parentPlayer->coins.add(parentQuest->cashReward);
        gServer->logGold(GOLD_IN, parentPlayer, parentQuest->cashReward, nullptr, "QuestCompletion");
        parentPlayer->printColor("%M gives you ^C%s^x. You now have %s.\n",
            monster, parentQuest->cashReward.str().c_str(), parentPlayer->coins.str().c_str());
    }
    if(parentQuest->expReward) {
        if(!parentPlayer->halftolevel()) {
            parentPlayer->printColor("You %s ^C%ld^x experience.\n", gConfig->isAprilFools() ? "lose" : "gain", parentQuest->expReward);
            parentPlayer->addExperience(parentQuest->expReward);
        }
    }
    if(!parentQuest->factionRewards.empty())
        parentPlayer->adjustFactionStanding(parentQuest->factionRewards);

    auto completionPtr = parentPlayer->questsCompleted.find(parentQuest->questId);
    QuestCompleted *questCompleted = nullptr;

    if (completionPtr == parentPlayer->questsCompleted.end()) {
        questCompleted = new QuestCompleted();
        parentPlayer->questsCompleted.insert(std::make_pair(parentQuest->questId, questCompleted));
    } else {
        questCompleted = completionPtr->second;
    }
    questCompleted->complete();

    // INVALID AFTER THIS...DO NOT ACCESS ANY MEMBERS OR THIS QUEST FROM ANYWHERE
    delete this;

    return(true);
}


//*****************************************************************************
//                      cmdTalk
//*****************************************************************************

int cmdTalk(Player* player, cmd* cmnd) {
    Monster *target=0;

    QuestInfo* quest = nullptr;

    bstring question;
    bstring response;
    bstring action;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num < 2) {
        player->print("Talk to whom?\n");
        return(0);
    }

    target = player->getParent()->findMonster(player, cmnd);
    if(!target) {
        player->print("You don't see that here.\n");
        return(0);
    }

    player->unhide();
    if(player->checkForSpam())
        return(0);

    if(target->flagIsSet(M_AGGRESSIVE_AFTER_TALK))
        target->addEnemy(player);

    if(target->current_language && !target->languageIsKnown(player->current_language)) {
        if(!player->checkStaff("%M doesn't seem to understand anything in %s.\n", target, get_language_adj(player->current_language)))
            return(0);
    }

    if(!target->canSpeak()) {
        player->print("%M is unable to speak right now.\n", target);
        return(0);
    }

    if(target->getTalk() != "" && !Faction::willSpeakWith(player, target->getPrimeFaction())) {
        player->print("%M refuses to speak with you.\n", target);
        return(0);
    }

    if(cmnd->num == 2 || target->responses.empty()) {
        response = target->getTalk();
        if(response == "$random") {
            std::list<bstring> randomResponses;
            std::list<bstring> randomActions;
            int numResponses=0;
            for(TalkResponse * talkResponse : target->responses) {
                for(const bstring&  keyword : talkResponse->keywords) {
                    if(keyword == "$random") {
                        randomResponses.push_back(talkResponse->response);
                        randomActions.push_back(talkResponse->action);
                        numResponses++;
                        break;
                    }
                }
            }
            if(!numResponses)
                response = "";
            else {
                numResponses = Random::get(1,numResponses);
                while(numResponses > 1) {
                    randomResponses.pop_front();
                    randomActions.pop_front();
                    numResponses--;
                }
                response = randomResponses.front();
                action = randomActions.front();
            }
        }
        if(response != "")
            broadcast(player->getSock(), player->getParent(), "%M speaks to %N in %s.",
                player, target, get_language_adj(player->current_language));
    } else {
        question = keyTxtConvert(getFullstrText(cmnd->fullstr, 2).toLower());
        broadcast_rom_LangWc(target->current_language, player->getSock(), player->currentLocation, "%M asks %N \"%s\".^x",
            player, target, question.c_str());
        bstring key = "", keyword = "";
        for(TalkResponse * talkResponse : target->responses) {
            for(const bstring& keyWrd : talkResponse->keywords) {
                keyword = keyTxtConvert(keyWrd).toLower();

                if(keyword[0] == '@') {
                    // We're looking for an exact match of the entire string
                    const bstring& toMatch = keyword.substr(1);
                    if(question == toMatch) {
                        // First let's copy over the information
                        key = keyword;
                        response = talkResponse->response;
                        action = talkResponse->action;
                        quest = talkResponse->quest;
                        // Next...lets get the hell out of here!
                        goto foundresponse;
                    }
                } else if(keyword[0] == '%') {
                    // Now we're looking for a match of the keyword surrounded by either white space,
                    // punctuation, or the end/start of the string
                    const bstring& toMatch = keyword.substr(1);
                    bstring::size_type idx = question.find(toMatch,0);
                    if(idx != bstring::npos) {
                        // Possible match
                        bstring::size_type keyLen = toMatch.length();
                        bstring::size_type questLen = question.length();
                        bstring::size_type startIdx = idx-1;
                        if(idx == 0 || (isspace(question[startIdx]) || ispunct(question[startIdx]))) {
                            // The start of the string is good, now let's check the end
                            bstring::size_type endIdx = idx + keyLen;
                            //char ch = question[endIdx];

                            if(endIdx >= questLen || (isspace(question[endIdx]) || ispunct(question[endIdx]))) {
                                // We've got a good match
                                // First let's copy over the information
                                key = keyword;
                                response = talkResponse->response;
                                action = talkResponse->action;
                                quest = talkResponse->quest;
                                // Next...lets get the hell out of here!
                                goto foundresponse;
                            }
                        }
                    }
                } else if(question.find(keyword, 0) != bstring::npos) {
                    // We found one of the keywords!
                    // First let's copy over the information
                    key = keyword;
                    response = talkResponse->response;
                    action = talkResponse->action;
                    quest = talkResponse->quest;
                    // Next...lets get the hell out of here!
                    goto foundresponse;
                }
            }
        }
        foundresponse:

        // they can't trigger special responses by talking to them
        if(key == "" || key.getAt(0) == '$') {
            response = "";
            action = "";
        }

        if(response == "" && action == "") {
            broadcast(nullptr, player->getRoomParent(), "%M shrugs.", target);
            return(0);
        }
    }


    if(response != "")
        target->sayTo(player, response);

    if(action != "")
        target->doTalkAction(player, action, quest);

    if(response == "" && action == "")
        broadcast(nullptr, player->getRoomParent(), "%M doesn't say anything.", target);

    return(0);
}

//*****************************************************************************
//                      doTalkAction
//*****************************************************************************

bool Monster::doTalkAction(Player* target, bstring action, QuestInfo* quest) {
    if(action.empty())
        return(false);

    boost::tokenizer<> tok(action);
    boost::tokenizer<>::iterator it = tok.begin();

    if(it == tok.end())
        return(false);

    bstring cmd = *it++;

    if(quest != nullptr) {
        if(isEnemy(target) && !target->checkStaff("%M refuses to talk to you about that.\n", this))
            return(false);

        if(!canSee(target) && !target->checkStaff("^m%M says, \"I'm not telling you about that unless I can see you!\"\n", this))
            return(false);

        if(!quest->canGetQuest(target, this))
            return(false);

        target->printColor("^m%M shares ^W%s^m with you.\n",  this, quest->getName().c_str());
        broadcast(target->getSock(), target->getRoomParent(), "^x%M shares ^W%s^x with %N.\n",
                  this, quest->getName().c_str(), target);
        quest->printReceiveString(target, this);
        target->addQuest(quest);
        quest->giveInitialitems(this, target);
        return(true);
    }
    else if(cmd.equals("attack", false)) {
        addEnemy(target, true);
        return(true);
    }
    else if(cmd.equals("action", false)) {
        if(it != tok.end()) {
            // Need to use global namespace cmd, overriding local variable
            ::cmd cm;

            bstring actionCmd = *it++;
            cm.fullstr = actionCmd;
            //strncpy(cm.fullstr, actionCmd.c_str(), 100);

            if(it != tok.end()) {
                bstring actTarget = *it++;
                if(actTarget.equals("player", false)) {
                    cm.fullstr += bstring(" ") + target->getName();
                }
            }

            stripBadChars(cm.fullstr); // removes '.' and '/'
            lowercize(cm.fullstr, 0);
            parse(cm.fullstr, &cm);

            cmdProcess(this, &cm);
            return(true);
        }
    }
    else if(cmd.equals("cast", false)) {
        if(it != tok.end()) {
            // Need to use global namespace cmd, overriding local variable
            ::cmd cm;

            action.Replace("PLAYER", target->getCName());
            cm.fullstr = action;

            stripBadChars(cm.fullstr); // removes '.' and '/'
            lowercize(cm.fullstr, 0);
            parse(cm.fullstr, &cm);

            CastResult result = doCast(this, &cm);

            if(result == CAST_RESULT_FAILURE) {
                target->print("%M apologizes that %s cannot cast that spell.\n", this, heShe());
                return(false);
            } else if(result == CAST_RESULT_CURRENT_FAILURE || result == CAST_RESULT_SPELL_FAILURE) {
                target->print("%M apologizes that %s cannot currently cast that spell.\n", this, heShe());
                return(false);
            }

            return(true);

        }
    }
    else if(cmd.equals("give", false)) {
        if(it != tok.end()) {
            CatRef toGive;
            // Find the area/number
            bstring area = *it++;
            int objNum = area.toInt();
            if(objNum <= 0) {
                toGive.setArea(area);

                // We didn't find a number, must be an area, look for the number now
                bstring num;

                if(it == tok.end())
                    return(false);
                num = *it++;
                objNum = num.toInt();
                if(objNum <= 0)
                    return(false);
            }
            toGive.id = objNum;

            Object* object = 0;
            if(loadObject(toGive, &object)) {
                if(object->flagIsSet(O_RANDOM_ENCHANT))
                    object->randomEnchant();

                if( (target->getWeight() + object->getActualWeight() > target->maxWeight()) &&
                    !target->checkStaff("You can't hold anymore.\n")
                ) {
                    delete object;
                    return(false);
                }

                if( object->getQuestnum() && target->questIsSet(object->getQuestnum()) &&
                    !target->checkStaff("You may not get that. You have already fulfilled that quest.\n")
                ) {
                    delete object;
                    return(false);
                }

                fulfillQuest(target, object);

                target->addObj(object);
                target->printColor("%M gives you %1P\n", this, object);
                broadcast(target->getSock(), target->getRoomParent(), "%M gives %N %1P\n", this, target, object);
            } else
                target->print("%M has nothing to give you.\n", this);

            return(true);
        }
    }

    return(false);
}

//*****************************************************************************
//                      giveInitialitems
//*****************************************************************************

void QuestInfo::giveInitialitems(const Monster* giver, Player* player) const {
    Object* object=0;
    std::list<QuestCatRef>::const_iterator it;
    int num=0, cur=0;

    for(it = initialItems.begin(); it != initialItems.end(); it++) {
        num = (*it).reqNum, cur = 0;
        if(num <= 0)
            continue;

        while(cur++ < num) {
            if(loadObject(*it, &object)) {
                object->init();
                // quest results set player ownership
                object->setQuestOwner(player);
                player->addObj(object);
            }
        }
        player->printColor("%M gives you ^C%s^x.\n", giver, object->getObjStr(player, 0, num).c_str());
    }
}

QuestEligibility QuestInfo::getEligibility(const Player *player, const Monster *giver) const {
    QuestEligibility eligibleRet = QuestEligibility::ELIGIBLE;

    if(player->hasQuest(this)) {
        return QuestEligibility::INELIGIBLE_HAS_QUEST;
    }

    QuestCompleted* completed = player->getQuestCompleted(questId);

    if(completed != nullptr) {
        if (!isRepeatable()) {
            return QuestEligibility::INELIGIBLE_NOT_REPETABLE;
        }
        else {
            if (completed->getTimesCompleted() >= getTimesRepeatable()) {
                return QuestEligibility::INELIGIBLE_NOT_REPETABLE;
            }

            time_t lastCompletion = completed->getLastCompleted();
            if (repeatFrequency == QuestRepeatFrequency::REPEAT_DAILY) {

                if (lastCompletion > getDailyReset()) {
                    return QuestEligibility::INELIGIBLE_DAILY_NOT_EXPIRED;
                }
                eligibleRet = QuestEligibility::ELIGIBLE_DAILY;
            }
            else if ( repeatFrequency == QuestRepeatFrequency::REPEAT_WEEKLY) {
                // else if weekly is the completion time earlier than last wednesday, midnight
                // if not, return
                if (lastCompletion > getWeeklyReset()) {
                    return QuestEligibility::INELIGIBLE_WEEKLY_NOT_EXPIRED;
                }

                eligibleRet = QuestEligibility::ELIGIBLE_WEEKLY;
            }
        }

    }
    if(minLevel != 0 && player->getLevel() < minLevel) {
        return QuestEligibility::INELIGIBLE_LEVEL;
    }

    if(minFaction != 0 && !giver->getPrimeFaction().empty()) {
        if(player->getFactionStanding(giver->getPrimeFaction()) < minFaction) {
            return QuestEligibility::INELIGIBLE_FACTION;
        }
    }

    if(!preRequisites.empty()) {
        for(const int & preReq : preRequisites) {
            if(!player->hasDoneQuest(preReq)){
                return QuestEligibility::INELIGIBLE_UNCOMPLETED_PREREQUISITES;
            }
        }
    }

    return eligibleRet;
}


//*****************************************************************************
//                      canGetQuest
//*****************************************************************************


bool QuestInfo::canGetQuest(const Player* player, const Monster* giver) const {
    QuestEligibility eligibility = getEligibility(player, giver);

    if(eligibility == QuestEligibility::INELIGIBLE_HAS_QUEST) {
        // Staff still can't have it in their quest book twice
        player->printColor("^m%M says, \"I'd tell you about ^W%s^m, but you already know about it!\"\n",
            giver, getName().c_str());
        return(false);
    }

    if(eligibility == QuestEligibility::INELIGIBLE_NOT_REPETABLE &&
            !player->checkStaff("^m%M says, \"I'd tell you about ^W%s^m, but you've already done that quest!\"\n",
                                 giver, getName().c_str()))
    {
        return (false);
    }

    if (eligibility == QuestEligibility::INELIGIBLE_DAILY_NOT_EXPIRED &&
            !player->checkStaff("^m%M says, \"Come back tomorrow and I'll tell you more about ^W%s^m.\"\n",
                            giver, getName().c_str())) {
        return (false);
    }

    if (eligibility == QuestEligibility::INELIGIBLE_WEEKLY_NOT_EXPIRED &&
            !player->checkStaff("^m%M says, \"Come back next week and I'll tell you more about ^W%s^m.\"\n",
                            giver, getName().c_str())) {
        return (false);
    }

    if (eligibility == QuestEligibility::INELIGIBLE_LEVEL &&
        !player->checkStaff("^m%M says, \"You're not ready for that information yet! Return when you have gained more experience.\"\n", giver))
        return(false);

    if (eligibility == QuestEligibility::INELIGIBLE_FACTION &&
        !player->checkStaff("^m%M says, \"I'm sorry, I don't trust you enough to tell you about that.\"\n", giver)) {
        return(false);
    }

    if(eligibility == QuestEligibility::INELIGIBLE_UNCOMPLETED_PREREQUISITES) {
        for(const int & preReq : preRequisites) {
            if(!player->hasDoneQuest(preReq) &&
                !player->checkStaff("^m%M says, \"You're not ready for that information yet! Return when you have finished ^W%s^m.\"\n",
                    giver, gConfig->getQuest(preReq)->getName().c_str())
            ) {
                return(false);
            }
        }
    }

    return(true);
}

//*****************************************************************************
//                      printReceiveString
//*****************************************************************************

void QuestInfo::printReceiveString(const Player* player, const Monster* giver) const {
    if(receiveString.empty())
        return;

    bstring toPrint = receiveString;

    toPrint.Replace("*GIVER*", giver->getCrtStr(player, CAP).c_str());
    toPrint.Replace("*CR*", "\n");

    player->printColor("%s\n", toPrint.c_str());
    return;
}

//*****************************************************************************
//                      printCompletionString
//*****************************************************************************

void QuestInfo::printCompletionString(const Player* player, const Monster* giver) const {
    if(receiveString.empty())
        return;

    bstring toPrint = completionString;

    toPrint.Replace("*GIVER*", giver->getCrtStr(player, CAP).c_str());
    toPrint.Replace("*CR*", "\n");

    player->printColor("%s\n", toPrint.c_str());
    return;
}



//*****************************************************************************
//                      convertOldTalks
//*****************************************************************************

void Monster::convertOldTalks() {
    ttag    *tp=0, *next=0;

    if(!flagIsSet(M_TALKS))
        return;
    clearFlag(M_TALKS);
    tp = first_tlk;
    while(tp) {
        TalkResponse* newResponse = new TalkResponse();
        newResponse->keywords.push_back(tp->key);
        newResponse->response = tp->response;
        switch(tp->type) {
        case 1:
            newResponse->action = "attack";
            break;
        case 2:
            newResponse->action = bstring("action ") + tp->action + " " + tp->target;
            break;
        case 3:
            newResponse->action = bstring("cast ") + tp->action + " " + tp->target;
            break;
        case 4:
            newResponse->action = bstring("give ") + tp->action;
            break;
        }

        responses.push_back(newResponse);
        tp = tp->next_tag;
    }

    tp = first_tlk;
    first_tlk = 0;
    while(tp) {
        next = tp->next_tag;
        if(tp->key)
            delete[] tp->key;
        if(tp->response)
            delete[] tp->response;
        if(tp->action)
            delete[] tp->action;
        if(tp->target)
            delete[] tp->target;
        delete tp;
        tp = next;
    }
    saveToFile();
}

//*********************************************************************
//                      cmdQuests
//*********************************************************************

int cmdQuests(Player* player, cmd* cmnd) {
    int j,i;
    char str[2048], str2[80];

    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    if(cmnd->num > 1 && (!strncmp(cmnd->str[1], "complete", strlen(cmnd->str[1])) ||
                         !strncmp(cmnd->str[1], "finish", strlen(cmnd->str[1]))) )
    {
        // We're trying to complete a quest here, lets see if we have one that matches
        // the user's input
        bstring questName = getFullstrText(cmnd->fullstr, 2);
        QuestCompletion* quest;
        for(std::pair<int, QuestCompletion*> p : player->questsInProgress) {
            quest = p.second;
            if(questName.empty() || !strncasecmp(quest->getParentQuest()->getName().c_str(), questName.c_str(), questName.length())) {

                // Make sure the quest is completed!!
                if(!quest->checkQuestCompletion(false)) {

                    // No name was specified, so continue to next quest and try to complete that
                    if(questName.empty()) continue;

                    player->printColor("But you haven't met all of the requirements for ^W%s^x yet!\n",
                        quest->getParentQuest()->getName().c_str());
                    return(0);
                }

                // We've found the quest that matches the string the user put in, now lets see if we can
                // find the finishing monster
                for(Monster* mons : player->getRoomParent()->monsters) {
                    if( mons->info == quest->getParentQuest()->getTurnInMob() ) {
                        // We have a turn in monster, lets complete the quest
                        if(mons->isEnemy(player)) {
                            player->print("%M refuses to deal with you right now!\n", mons);
                            return(0);
                        }

                        bstring name = quest->getParentQuest()->getName();
                        player->printColor("Completing quest ^W%s^x.\n", name.c_str());

                        // NOTE: After quest->complete, quest is INVALID, do not attempt to access it
                        if(quest->complete(mons)) {
                            broadcast(player->getSock(), player->getParent(), "%M just completed ^W%s^x.",
                                player, name.c_str());
                        } else {
                            //player->print("Quest completion failed.\n");
                            broadcast(player->getSock(), player->getParent(), "%M tried to complete ^W%s^x.",
                                player, name.c_str());
                        }

                        return(0);
                    }

                }

                if(!questName.empty()) {
                    // Name was specified, so stop
                    player->printColor("Could not find a turn in monster!\n");
                    return(0);
                }
                // No name was specified, so continue to next quest and try to complete that
            }
        }
        player->printColor("No quests were found that could be completed right now.\n");
        return(0);
    } else if(cmnd->num > 1 && (!strncmp(cmnd->str[1], "quit", strlen(cmnd->str[1])) ||
             !strncmp(cmnd->str[1], "abandon", strlen(cmnd->str[1]))) )
    {
        // We're trying to abandon a quest here, lets see if we have one that matches
        // the user's input
        bstring questName = getFullstrText(cmnd->fullstr, 2);
        QuestCompletion* quest;
        if(questName.empty()) {
            player->print("Abandon which quest?\n");
            return(0);
        }
        for(std::pair<int, QuestCompletion*> p : player->questsInProgress) {
            quest = p.second;
            if(!strncasecmp(quest->getParentQuest()->getName().c_str(), questName.c_str(), questName.length())) {
                player->printColor("Abandoning ^W%s^x.\n", quest->getParentQuest()->getName().c_str());
                player->questsInProgress.erase(quest->getParentQuest()->getId());
                delete quest;
                return(0);
            }
        }
        player->printColor("Could not find any quests that matched the name ^W%s^x.\n", questName.c_str());
        return(0);
    }
    sprintf(str, "^WOld Quests Completed:^x\n");
    for(i=1, j=0; i<MAX_QUEST; i++)
        if(player->questIsSet(i)) {
            sprintf(str2, "%s, ", get_quest_name(i));
            strcat(str, str2);
            j++;
        }
    if(!j)
        strcat(str, "None.\n");
    else {
        str[strlen(str)-2] = '.';
        strcat(str, "\n");
    }

    player->printColor("%s", str);

    if(!player->questsCompleted.empty()) {

        std::ostringstream displayStr;

        displayStr << "^WQuests Completed:^x\n";
        for(auto qc : player->questsCompleted) {
            displayStr << gConfig->getQuest(qc.first)->getName();
            if(qc.second->getTimesCompleted() > 1)
                displayStr << " (" << qc.second << ")";
            displayStr << "\n";
        }

        player->printColor("%s", displayStr.str().c_str());
    }


    Player* target = player;
    i = 1;
    player->printColor("^WQuests in Progress:\n");

    if(target->questsInProgress.empty())
        player->printColor("None!\n");

    for(std::pair<int, QuestCompletion*> p : target->questsInProgress) {
        QuestCompletion* quest = p.second;
        player->printColor("%d) %s\n", i++, quest->getStatusDisplay().c_str());
    }

    return(0);
}


void QuestCompleted::complete() {
    lastCompleted = time(0);
    times++;
}

time_t QuestCompleted::getLastCompleted() {
    return lastCompleted;
}

int QuestCompleted::getTimesCompleted() {
    return times;
}

QuestCompleted::QuestCompleted(int pTimes) {
    lastCompleted = time(0);
    times = pTimes;
}

void QuestCompleted::init() {
    lastCompleted = 0;
    times = 0;
}

QuestCompleted::QuestCompleted() {
    init();
}


time_t getDailyReset() {
    struct tm t;
    time_t now = time(0);
    localtime_r(&now, &t);
    t.tm_sec = t.tm_min = t.tm_hour = 0;
    return (mktime(&t) - 1);


}

time_t getWeeklyReset() {
    time_t base_t = time(0);
    struct tm base_tm;
    localtime_r(&base_t, &base_tm);

    int dow = base_tm.tm_wday;
    int ddiff = 0;

    if (dow > 2) {
        ddiff = 2 - dow;
    } else if (dow < 2 ) {
        ddiff = -(dow + 5);
    }

    struct tm new_tm;
    new_tm = base_tm;

    new_tm.tm_yday += ddiff;
    new_tm.tm_wday += ddiff;
    new_tm.tm_mday += ddiff;

    // Midnight
    new_tm.tm_min = new_tm.tm_sec = new_tm.tm_hour = 0;
    
    time_t new_time = mktime(&new_tm);
    struct tm new_t;
    localtime_r(&new_time, &new_t);

    return new_time;
}
