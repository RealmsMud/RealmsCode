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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <bits/types/struct_tm.h>                   // for tm
#include <fmt/format.h>                             // for format
#include <libxml/parser.h>                          // for xmlCleanupParser
#include <strings.h>                                // for strncasecmp
#include <boost/algorithm/string/case_conv.hpp>     // for to_lower_copy
#include <boost/algorithm/string/predicate.hpp>     // for iequals
#include <boost/algorithm/string/replace.hpp>       // for replace_all
#include <boost/iterator/iterator_facade.hpp>       // for operator!=, opera...
#include <boost/iterator/iterator_traits.hpp>       // for iterator_value<>:...
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <boost/mpl/eval_if.hpp>                    // for eval_if<>::type
#include <boost/token_functions.hpp>                // for char_delimiters_s...
#include <boost/token_iterator.hpp>                 // for token_iterator
#include <boost/tokenizer.hpp>                      // for tokenizer<>::iter...
#include <cctype>                                   // for ispunct, isspace
#include <cstdio>                                   // for sprintf
#include <cstring>                                  // for strlen, strncmp
#include <ctime>                                    // for time, localtime_r
#include <deque>                                    // for _Deque_iterator
#include <limits>                                   // for numeric_limits
#include <list>                                     // for list, operator==
#include <locale>                                   // for locale
#include <map>                                      // for operator==, map
#include <set>                                      // for set, set<>::iterator
#include <sstream>                                  // for operator<<, basic...
#include <stdexcept>                                // for runtime_error
#include <string>                                   // for string, basic_string
#include <string_view>                              // for operator==, strin...
#include <utility>                                  // for pair, make_pair

#include "catRef.hpp"                               // for CatRef
#include "cmd.hpp"                                  // for cmd
#include "commands.hpp"                             // for getFullstrText
#include "config.hpp"                               // for Config, QuestInfoMap
#include "creatureStreams.hpp"                      // for Streamable, ColorOn
#include "factions.hpp"                             // for Faction
#include "flags.hpp"                                // for M_TALKS, P_AFK
#include "global.hpp"                               // for CAP, INV, CAST_RE...
#include "money.hpp"                                // for Money
#include "mudObjects/container.hpp"                 // for ObjectSet, Container
#include "mudObjects/creatures.hpp"                 // for Creature
#include "mudObjects/monsters.hpp"                  // for Monster
#include "mudObjects/objects.hpp"                   // for Object, ObjectType
#include "mudObjects/players.hpp"                   // for Player, Player::Q...
#include "mudObjects/rooms.hpp"                     // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"               // for UniqueRoom
#include "oldquest.hpp"                             // for fulfillQuest
#include "paths.hpp"                                // for Game
#include "proto.hpp"                                // for broadcast, get_la...
#include "quests.hpp"                               // for QuestCompletion
#include "random.hpp"                               // for Random
#include "server.hpp"                               // for GOLD_IN, Server
#include "structs.hpp"                              // for ttag
#include "toNum.hpp"                                // for toNum
#include "utils.hpp"                                // for MIN, MAX
#include "xml.hpp"                                  // for loadObject, loadM...

CatRef QuestInfo::getQuestId(xmlNodePtr curNode) {
    CatRef toReturn;
    toReturn.id = xml::getShortProp(curNode, "ID");
    if(toReturn.id == 0)
        toReturn.id = xml::getShortProp(curNode, "Num");

    xml::copyPropToString(toReturn.area, curNode, "Area");

    return toReturn;
}
CatRef QuestInfo::getQuestId(std::string strId) {
    CatRef toReturn;
    getCatRef(std::move(strId), &toReturn, nullptr);

    return toReturn;
}

void QuestInfo::saveQuestId(xmlNodePtr curNode, const CatRef& questId) {
    xml::newNumProp(curNode, "ID", questId.id);
    xml::newProp(curNode, "Area", questId.area);
}

QuestCatRef::QuestCatRef() {
    area = gConfig->getDefaultArea();
    reqNum = 1;
    curNum = 0;
    id = 0;
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
    questId = QuestInfo::getQuestId(rootNode);
    resetParentQuest();

    parentPlayer = player;

    mobsCompleted = false;
    itemsCompleted = false;
    roomsCompleted = false;

    xmlNodePtr curNode = rootNode->children;
    xmlNodePtr childNode;
    while(curNode) {
        if(NODE_NAME(curNode, "Revision")) xml::copyToString(revision, curNode);
        else if(NODE_NAME(curNode, "MobsKilled")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "QuestCatRef")) {
                    mobsKilled.emplace_back(childNode);
                }
                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "RoomsVisited")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "QuestCatRef")) {
                    roomsVisited.emplace_back(childNode);
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
    QuestInfo::saveQuestId(curNode, questId);

    // We don't actually read this in, but we'll save it for reference in case
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


void QuestCompletion::resetParentQuest() {
    if((parentQuest = gConfig->getQuest(questId)) == nullptr) {
        throw(std::runtime_error("Unable to find parent quest - " + questId.str()));
    }
}

QuestInfo* QuestCompletion::getParentQuest() const {
    return(parentQuest);
}

TalkResponse::TalkResponse() {
    quest = nullptr;
}

//*****************************************************************************
//                      parseQuest
//*****************************************************************************

void TalkResponse::parseQuest() {
    boost::tokenizer<> tok(action);
    auto it = tok.begin();

    if(it == tok.end())
        return;

    std::string cmd = *it++;

    if(boost::iequals(cmd, "quest")) {
        if(it == tok.end())
            return;

        // Find the quest id (area.##)
        std::string questIdStr = *it++;
        if(questIdStr.empty())
            return;

        CatRef questId = QuestInfo::getQuestId(questIdStr);

        QuestInfo* questInfo = gConfig->getQuest(questId);
        if(!questInfo)
            return;

        quest = questInfo;

    }

}

bool QuestInfo::isRepeatable() const {
    return(repeatable);
}
int QuestInfo::getTimesRepeatable() const {
    if (timesRepeatable == 0)
        return(std::numeric_limits<int>::max());
    else
        return(timesRepeatable);
}

const QuestCatRef& QuestInfo::getTurnInMob() const {
    return(turnInMob);
}
std::string QuestInfo::getName() const {
    return(name);
}
CatRef QuestInfo::getId() const {
    return(questId);
}
std::string QuestInfo::getDisplayName() const {
    std::ostringstream displayStr;
    displayStr << "^Y#" << questId.str() << " - " << name << "^x";
    return(displayStr.str());
}

std::string QuestInfo::getDisplayString() const {
    std::ostringstream displayStr;
    std::map<std::string, long>::const_iterator it;
    int i = 0;

    displayStr << getDisplayName();

    if(timesRepeatable)
            displayStr << " ^G*Repeatable " << timesRepeatable << " times*^x\n";
    if (isRepeatable()) {
        if(repeatFrequency == QuestRepeatFrequency::REPEAT_DAILY)
            displayStr << " ^G*Offered Daily*^x";
        else if (repeatFrequency == QuestRepeatFrequency::REPEAT_WEEKLY)
            displayStr << " ^G*Offered Weekly*^x";
        else if (repeatFrequency == QuestRepeatFrequency::REPEAT_UNLIMITED)
            displayStr << " ^G*Offered Always*^x";
   }
    


    if(sharable)
        displayStr << " ^Y*Sharable*^x";
    displayStr << std::endl;
    displayStr << "^WDescription: ^x" << description << "^x\n";
    std::string temp = "";

    temp = receiveString;
    boost::replace_all(temp, "*CR", "\n");
    displayStr << "^WReceiveString: ^x" << temp << "\n";

    temp = completionString;
    boost::replace_all(temp, "*CR", "\n");
    displayStr << "^WCompletionString: ^x" << temp << "\n";

    if(!preRequisites.empty()) {
        QuestInfo* q=nullptr;
        int t = 0;
        displayStr << "^WPrerequisites:^x";
        for(const CatRef & preReq : preRequisites) {
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

    Monster* endMonster = nullptr;
    displayStr << "^RTurn in Monster:^x ";
    if(loadMonster(turnInMob, &endMonster)) {
         displayStr << endMonster->getName();
         delete endMonster;
    } else {
        displayStr << "ERROR: Monster doesn't exit!";
    }
    displayStr << "(" << turnInMob.displayStr() << ")\n";


    if(!initialItems.empty()) {
        Object* object;
        displayStr << "\t^WInitial Items:^x\n";
        i = 1;
        for(const QuestCatRef & obj : initialItems) {
            displayStr << "\t\t^W" << i++ << ")^x #" << obj.displayStr() << " - ";
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
            displayStr << "\t\t^W" << i++ << ")^x #" << mob.displayStr() << " - ";
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
            displayStr << "\t\t^W" << i++ << ")^x #" << obj.displayStr() << " - ";
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
            displayStr << "\t\t^W" << i++ << ")^x #" << rom.displayStr() << " - ";
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
            displayStr << "\t\t\t^W" << i++ << ")^x #" << obj.displayStr() << " - ";
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
    for (auto const& [questId, quest] : quests) {
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

    while(curNode) {
        if(NODE_NAME(curNode, "QuestInfo")) {
            CatRef questId = QuestInfo::getQuestId(curNode);
            auto* tmpQuest = new QuestInfo(curNode);

            if(quests[questId] == nullptr){
                quests[questId] = tmpQuest;
            } else {
                (*quests[questId]) = *tmpQuest;
                delete tmpQuest;
            }
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

QuestInfo* Config::getQuest(const CatRef& questId) {
    auto questIt = quests.find(questId);
    if(questIt != quests.end())
        return(questIt->second);
    else
        return(nullptr);

}

bool Player::addQuest(QuestInfo* toAdd) {
    if(hasQuest(toAdd))
        return(false);
    questsInProgress[toAdd->getId()] = new QuestCompletion(toAdd, this);
    *this << ColorOn << fmt::format("^W{}^x has been added to your quest book.\n", toAdd->getName()) << ColorOff;
    return(true);
}
bool Player::hasQuest(const CatRef& questId) const {
    return(questsInProgress.find(questId) != questsInProgress.end());
}

bool Player::hasQuest(const QuestInfo *quest) const {
    return(quest && hasQuest(quest->getId()));
}

bool Player::hasDoneQuest(const CatRef& questId) const {
    return(questsCompleted.find(questId) != questsCompleted.end());
}

bool Monster::hasQuests() const {
    return !quests.empty();
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
        else if (eligible == QuestEligibility::ELIGIBLE_DAILY || eligible == QuestEligibility::ELIGIBLE_WEEKLY)
            hasRepetable = true;
        else if (eligible == QuestEligibility::INELIGIBLE_LEVEL)
            isLowLevel = true;
        else if (eligible == QuestEligibility::INELIGIBLE_DAILY_NOT_EXPIRED || eligible == QuestEligibility::INELIGIBLE_WEEKLY_NOT_EXPIRED)
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

QuestCompleted* Player::getQuestCompleted(const CatRef& questId) const {
    auto completion = questsCompleted.find(questId);
    if (completion != questsCompleted.end()) {
        return completion->second;
    } else {
        return nullptr;
    }
}

void Player::updateMobKills(Monster* monster) {
    for(const auto&[questId, quest] : questsInProgress) {
        quest->updateMobKills(monster);
    }
}

void Player::updateItems(Object* object) {
    for(const auto&[questId, quest] : questsInProgress) {
        quest->updateItems(object);
    }
}
void Player::updateRooms(UniqueRoom* room) {
    for(const auto&[questId, quest] : questsInProgress) {
        quest->updateRooms(room);
    }
}
void QuestCompletion::updateMobKills(Monster* monster) {
    //std::list<QuestCatRef> mobsKilled;
    for(QuestCatRef & qcr : mobsKilled) {
        if(qcr == monster->info) {
            if(qcr.reqNum != qcr.curNum) {
                if(++(qcr.curNum) == qcr.reqNum) {
                    *parentPlayer << ColorOn << fmt::format(
                            "Quest Update: {} - Required number of ^W{}^x have been killed.\n",
                            parentQuest->getName(), monster->getName()) << ColorOff;
                } else {
                    *parentPlayer << ColorOn << fmt::format(
                            "Quest Update: {} - Killed ^W {} / {}^x.\n",
                            parentQuest->getName(), intToText(qcr.curNum),
                            monster->getCrtStr(parentPlayer, INV, qcr.reqNum))  << ColorOff;
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
                    *parentPlayer << ColorOn << fmt::format(
                        "Quest Update: {} - Required number of ^W{}^x have been obtained.\n",
                        parentQuest->getName(), object->getName()) << ColorOff;
                } else {
                    *parentPlayer << ColorOn << fmt::format(
                        "Quest Update: {} - Obtained ^W{} / {}^x.\n",
                        parentQuest->getName(), intToText(curNum),
                        object->getObjStr(parentPlayer, INV, qcr.reqNum)) << ColorOff;
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
                    *parentPlayer << ColorOn << fmt::format("Quest Update: {} - Visited ^W{}^x.\n",
                            parentQuest->getName(), room->getName());
                }
            }
        }
    }
    checkQuestCompletion();
}
bool QuestCompletion::checkQuestCompletion(bool showMessage) {
    bool alreadyCompleted = mobsCompleted && itemsCompleted && roomsCompleted;

    // First see if we have killed all required monsters
    if(!mobsCompleted) {
        // Variable is false, so check the function
        mobsCompleted = hasRequiredMobs();
    }
    // Now check to see if we've been to all of the rooms we should have
    if(!roomsCompleted) {
        // Nope, so check the function
        roomsCompleted = hasRequiredRooms();
    }

    // Now see if we still have the required items
    bool itemsDone = hasRequiredItems();
    if(itemsCompleted && itemsDone) {
        itemsCompleted = true;
    } else if(!itemsDone) {
        itemsCompleted = false;
        return(false);
    } else { //if(itemsDone == true) {
        itemsCompleted = true;
    }

    if(mobsCompleted && roomsCompleted && itemsCompleted) {
        if(showMessage && !alreadyCompleted) {
            Monster* endMonster = nullptr;

            *parentPlayer << ColorOn << fmt::format("You have fufilled all of the requirements for ^W{}^x!\n", parentQuest->getName()) << ColorOff;
            if(loadMonster(parentQuest->turnInMob, &endMonster)) {
                *parentPlayer << ColorOn << fmt::format("Return to ^W{}^x to claim your reward.\n", endMonster->getName()) << ColorOff;
                delete endMonster;
            }
        }
        return(true);
    }
    return(false);

}

bool QuestCompletion::hasRequiredMobs() const {
    // Assume completion, and test for non completion
    for(const QuestCatRef& mob : mobsKilled) {
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
std::string QuestCompletion::getStatusDisplay() {
    std::ostringstream displayStr;
    int i;

    checkQuestCompletion(false);

    if(mobsCompleted && itemsCompleted && roomsCompleted)
        displayStr << "^Y*";
    else
        displayStr << "^y";
    displayStr << parentQuest->name << "^x\n";
    displayStr << parentQuest->name << "^x";
    
    if(parentQuest->timesRepeatable)
        displayStr << " ^G*Repeatable Limited*^x";
    if (parentQuest->isRepeatable()) {
        if(parentQuest->repeatFrequency == QuestRepeatFrequency::REPEAT_DAILY)
            displayStr << " ^G*Offered Daily*^x";
        else if (parentQuest->repeatFrequency == QuestRepeatFrequency::REPEAT_WEEKLY)
            displayStr << " ^G*Offered Weekly*^x";
        else if (parentQuest->repeatFrequency == QuestRepeatFrequency::REPEAT_UNLIMITED)
            displayStr << " ^G*Offered Always*^x";
    }



    displayStr << std::endl;

    displayStr << "^WDescription: ^w" << parentQuest->description << "^x\n";

    Monster* endMonster = nullptr;
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
    std::map<std::string, long>::const_iterator it;
    std::list<Object*> objects;
    std::list<Object*>::iterator oIt;

    if(!checkQuestCompletion()) {
        *parentPlayer <<"You have not fulfilled all the requirements to complete this quest.\n";
        return(false);
    }

    // if they can't get all of the item rewards, they can't complete the quest
    for(QuestCatRef & obj : parentQuest->itemRewards) {
        int num = obj.reqNum, cur = 0;
        Object* object=nullptr;
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
        std::string oName;
        Object *object;
        ObjectSet::iterator osIt;
        int num = obj.reqNum;

        for(osIt = parentPlayer->objects.begin() ; osIt != parentPlayer->objects.end() && num > 0 ; ) {
            object = (*osIt++);
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
            *parentPlayer << ColorOn << setf(CAP) << monster << " takes ^W" << setn(obj.reqNum) << object << "^x from you.\n" << ColorOff;
            delete object;
        }
    }

    for(oIt = objects.begin(); oIt != objects.end(); oIt++) {
        *parentPlayer << ColorOn << setf(CAP) << monster << " gives you ^C" << *oIt << "^x.\n" << ColorOff;
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
        Server::logGold(GOLD_IN, parentPlayer, parentQuest->cashReward, nullptr, "QuestCompletion");
        *parentPlayer << ColorOn << setf(CAP) << monster << " gives you ^C" << parentQuest->cashReward.str() << "^x. You now have " << parentPlayer->coins.str() << ".\n" << ColorOff;
    }
    if(parentQuest->expReward) {
        if(!parentPlayer->halftolevel()) {
            *parentPlayer << ColorOn << fmt::format("You {} ^C{}^x experience.\n", gConfig->isAprilFools() ? "lose" : "gain", parentQuest->expReward);
            parentPlayer->addExperience(parentQuest->expReward);
        }
    }
    if(parentQuest->alignmentChange) {
        std::ostringstream oStr;
        oStr << (parentQuest->alignmentChange < 0 ? "^r" : "^b") << "Your alignment has shifted towards " << (parentQuest->alignmentChange < 0 ? "^Revil" : "^Bgood") << ".";
        if (parentPlayer->isStaff())
            oStr << " (" << parentQuest->alignmentChange << ")";
        *parentPlayer << ColorOn << oStr.str() << ColorOff << "\n";

        parentPlayer->setAlignment(MAX<short>(-1000, MIN<short>(1000,(parentPlayer->getAlignment()+parentQuest->alignmentChange))));
        parentPlayer->alignAdjustAcThaco();
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
    Monster *target=nullptr;

    QuestInfo* quest = nullptr;

    std::string question;
    std::string response;
    std::string action;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num < 2) {
        *player << "Talk to whom?\n";
        return(0);
    }

    target = player->getParent()->findMonster(player, cmnd);
    if(!target) {
        *player << "You don't see that here.\n";
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
        *player << setf(CAP) << target << " is unable to speak right now.\n";
        return(0);
    }

    if(!target->getTalk().empty() && !Faction::willSpeakWith(player, target->getPrimeFaction())) {
        *player << setf(CAP) << target << " irefuses to speak with you.\n";
        return(0);
    }

    if(cmnd->num == 2 || target->responses.empty()) {
        response = target->getTalk();
        if(response == "$random") {
            std::list<std::string> randomResponses;
            std::list<std::string> randomActions;
            int numResponses=0;
            for(TalkResponse * talkResponse : target->responses) {
                for(std::string_view  keyword : talkResponse->keywords) {
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
        if(!response.empty())
            broadcast(player->getSock(), player->getParent(), "%M speaks to %N in %s.",
                player, target, get_language_adj(player->current_language));
    } else {
        question = keyTxtConvert(boost::to_lower_copy(getFullstrText(cmnd->fullstr, 2)));
        broadcast_rom_LangWc(target->current_language, player->getSock(), player->currentLocation, "%M asks %N \"%s\".^x",
            player, target, question.c_str());
        std::string key, keyword;
        for(TalkResponse * talkResponse : target->responses) {
            for(std::string_view keyWrd : talkResponse->keywords) {
                keyword = boost::to_lower_copy(keyTxtConvert(keyWrd));

                if(keyword[0] == '@') {
                    // We're looking for an exact match of the entire string
                    const std::string& toMatch = keyword.substr(1);
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
                    const std::string& toMatch = keyword.substr(1);
                    std::string::size_type idx = question.find(toMatch,0);
                    if(idx != std::string::npos) {
                        // Possible match
                        std::string::size_type keyLen = toMatch.length();
                        std::string::size_type questLen = question.length();
                        std::string::size_type startIdx = idx-1;
                        if(idx == 0 || (isspace(question[startIdx]) || ispunct(question[startIdx]))) {
                            // The start of the string is good, now let's check the end
                            std::string::size_type endIdx = idx + keyLen;
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
                } else if(question.find(keyword, 0) != std::string::npos) {
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
        if(key.empty() || key.at(0) == '$') {
            response = "";
            action = "";
        }

        if(response.empty() && action.empty()) {
            broadcast(nullptr, player->getRoomParent(), "%M shrugs.", target);
            return(0);
        }
    }


    if(!response.empty())
        target->sayTo(player, response);

    if(!action.empty())
        target->doTalkAction(player, action, quest);

    if(response.empty() && action.empty())
        broadcast(nullptr, player->getRoomParent(), "%M doesn't say anything.", target);

    return(0);
}

//*****************************************************************************
//                      doTalkAction
//*****************************************************************************

bool Monster::doTalkAction(Player* target, std::string action, QuestInfo* quest) {
    if(action.empty())
        return(false);

    boost::tokenizer<> tok(action);
    boost::tokenizer<>::iterator it = tok.begin();

    if(it == tok.end())
        return(false);

    std::string cmd = *it++;

    if(quest != nullptr) {
        if(isEnemy(target) && !target->checkStaff("%M refuses to talk to you about that.\n", this))
            return(false);

        if(!canSee(target) && !target->checkStaff("^m%M says, \"I'm not telling you anything else useful until I can see you!\"\n", this))
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
    else if(boost::iequals(cmd, "attack")) {
        addEnemy(target, true);
        return(true);
    }
    else if(boost::iequals(cmd, "action")) {
        if(it != tok.end()) {
            // Need to use global namespace cmd, overriding local variable
            ::cmd cm;

            std::string actionCmd = *it++;
            cm.fullstr = actionCmd;
            //strncpy(cm.fullstr, actionCmd.c_str(), 100);

            if(it != tok.end()) {
                std::string actTarget = *it++;
                if(boost::iequals(actTarget, "player")) {
                    cm.fullstr += std::string(" ") + target->getName();
                }
            }

            stripBadChars(cm.fullstr); // removes '.' and '/'
            lowercize(cm.fullstr, 0);
            parse(cm.fullstr, &cm);

            cmdProcess(this, &cm);
            return(true);
        }
    }
    else if(boost::iequals(cmd, "cast")) {
        if(it != tok.end()) {
            // Need to use global namespace cmd, overriding local variable
            ::cmd cm;

            boost::replace_all(action, "PLAYER", target->getCName());
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
    else if(boost::iequals(cmd, "give")) {
        if(it != tok.end()) {
            CatRef toGive;
            // Find the area/number
            std::string area = *it++;
            int objNum = toNum<int>(area);
            if(objNum <= 0) {
                toGive.setArea(area);

                // We didn't find a number, must be an area, look for the number now
                std::string num;

                if(it == tok.end())
                    return(false);
                num = *it++;
                objNum = toNum<int>(area);
                if(objNum <= 0)
                    return(false);
            }
            toGive.id = objNum;

            Object* object = nullptr;
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
    Object* object=nullptr;
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
        for(const CatRef& preReq : preRequisites) {
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
        for(const CatRef & preReq : preRequisites) {
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

void QuestInfo::printReceiveString(Player* player, const Monster* giver) const {
    if(receiveString.empty())
        return;

    std::string toPrint = receiveString;

    boost::replace_all(toPrint, "*GIVER*", giver->getCrtStr(player, CAP).c_str());
    boost::replace_all(toPrint, "*CR*", "\n");
    *player << ColorOn << toPrint << ColorOff << "\n";
}

//*****************************************************************************
//                      printCompletionString
//*****************************************************************************

void QuestInfo::printCompletionString(Player* player, const Monster* giver) const {
    if(receiveString.empty())
        return;

    std::string toPrint = completionString;

    boost::replace_all(toPrint, "*GIVER*", giver->getCrtStr(player, CAP).c_str());
    boost::replace_all(toPrint, "*CR*", "\n");
    *player << ColorOn << toPrint << ColorOff << "\n";
}



//*****************************************************************************
//                      convertOldTalks
//*****************************************************************************

void Monster::convertOldTalks() {
    ttag    *tp=nullptr, *next=nullptr;

    if(!flagIsSet(M_TALKS))
        return;
    clearFlag(M_TALKS);
    tp = first_tlk;
    while(tp) {
        auto* newResponse = new TalkResponse();
        newResponse->keywords.emplace_back(tp->key);
        newResponse->response = tp->response;
        switch(tp->type) {
        case 1:
            newResponse->action = "attack";
            break;
        case 2:
            newResponse->action = std::string("action ") + tp->action + " " + tp->target;
            break;
        case 3:
            newResponse->action = std::string("cast ") + tp->action + " " + tp->target;
            break;
        case 4:
            newResponse->action = std::string("give ") + tp->action;
            break;
        }

        responses.push_back(newResponse);
        tp = tp->next_tag;
    }

    tp = first_tlk;
    first_tlk = nullptr;
    while(tp) {
        next = tp->next_tag;
        delete[] tp->key;
        delete[] tp->response;
        delete[] tp->action;
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
        *player << "You are brain-dead. You can't do that.\n";
        return(0);
    }

    if(cmnd->num > 1 && (!strncmp(cmnd->str[1], "complete", strlen(cmnd->str[1])) ||
                         !strncmp(cmnd->str[1], "finish", strlen(cmnd->str[1]))) )
    {
        // We're trying to complete a quest here, lets see if we have one that matches
        // the user's input
        std::string questName = getFullstrText(cmnd->fullstr, 2);
        for(const auto& [questId, quest] : player->questsInProgress) {
            if(questName.empty() || !strncasecmp(quest->getParentQuest()->getName().c_str(), questName.c_str(), questName.length())) {
                // Make sure the quest is completed!!
                if(!quest->checkQuestCompletion(false)) {
                    // No name was specified, so continue to next quest and try to complete that
                    if(questName.empty()) continue;

                    *player << ColorOn <<"But you haven't met all of the requirements for ^W" << quest->getParentQuest()->getName() << "^x yet!\n" << ColorOff;
                    return(0);
                }

                // We've found the quest that matches the string the user put in, now lets see if we can
                // find the finishing monster
                for(Monster* mons : player->getRoomParent()->monsters) {
                    if( mons->info == quest->getParentQuest()->getTurnInMob() ) {
                        // We have a turn in monster, lets complete the quest
                        if(mons->isEnemy(player)) {
                            *player << setf(CAP) << mons << " refuses to deal with you right now!\n";
                            return(0);
                        }

                        std::string name = quest->getParentQuest()->getName();
                        *player << ColorOn << "Completing quest ^W" << name << "^x.\n";

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
                    *player << "Could not find a turn in monster!\n";
                    return(0);
                }
                // No name was specified, so continue to next quest and try to complete that
            }
        }
        *player <<"No quests were found that could be completed right now.\n";
        return(0);
    } else if(cmnd->num > 1 && (!strncmp(cmnd->str[1], "quit", strlen(cmnd->str[1])) ||
             !strncmp(cmnd->str[1], "abandon", strlen(cmnd->str[1]))) )
    {
        // We're trying to abandon a quest here, lets see if we have one that matches
        // the user's input
        std::string questName = getFullstrText(cmnd->fullstr, 2);
        QuestCompletion* quest;
        if(questName.empty()) {
            *player << "Abandon which quest?\n";
            return(0);
        }
        for(std::pair<CatRef, QuestCompletion*> p : player->questsInProgress) {
            quest = p.second;
            if(!strncasecmp(quest->getParentQuest()->getName().c_str(), questName.c_str(), questName.length())) {
                *player << ColorOn << "Abandoning ^W" << quest->getParentQuest()->getName() << "^x.\n";
                player->questsInProgress.erase(quest->getParentQuest()->getId());
                delete quest;
                return(0);
            }
        }
        *player << "Could not find any quests that matched the name ^W" << questName << "^x.\n";
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

    *player << PagerOn << ColorOn << str << ColorOff;

    if(!player->questsCompleted.empty()) {

        std::ostringstream displayStr;

        displayStr << "^WQuests Completed:^x\n";
        for(auto qc : player->questsCompleted) {
            displayStr << gConfig->getQuest(qc.first)->getName();

            if(qc.second->getTimesCompleted() > 1)
                displayStr << " (" << qc.second << ")";

            displayStr << "\n";
        }

        *player << ColorOn << displayStr.str() << ColorOff;
    }


    Player* target = player;
    i = 1;
    *player << ColorOn << "^WQuests in Progress:\n" << ColorOff;

    if(target->questsInProgress.empty())
        player->printColor("None!\n");

    for(const auto& [questId, quest] : target->questsInProgress) {
        *player << i++ << ") " << ColorOn << quest->getStatusDisplay() << ColorOff;
    }

    *player << PagerOff;

    return(0);
}


void QuestCompleted::complete() {
    lastCompleted = time(nullptr);
    times++;
}

time_t QuestCompleted::getLastCompleted() const {
    return lastCompleted;
}

int QuestCompleted::getTimesCompleted() const {
    return times;
}

QuestCompleted::QuestCompleted(int pTimes) {
    lastCompleted = time(nullptr);
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
    struct tm t{};
    time_t now = time(nullptr);
    localtime_r(&now, &t);
    t.tm_sec = t.tm_min = t.tm_hour = 0;
    return (mktime(&t) - 1);


}

time_t getWeeklyReset() {
    time_t base_t = time(nullptr);
    struct tm base_tm{};
    localtime_r(&base_t, &base_tm);

    int dow = base_tm.tm_wday;
    int ddiff = 0;

    if (dow > 2) {
        ddiff = 2 - dow;
    } else if (dow < 2 ) {
        ddiff = -(dow + 5);
    }

    struct tm new_tm{};
    new_tm = base_tm;

    new_tm.tm_yday += ddiff;
    new_tm.tm_wday += ddiff;
    new_tm.tm_mday += ddiff;

    // Midnight
    new_tm.tm_min = new_tm.tm_sec = new_tm.tm_hour = 0;
    
    time_t new_time = mktime(&new_tm);
    struct tm new_t{};
    localtime_r(&new_time, &new_t);

    return new_time;
}
