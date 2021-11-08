/*
 * Quests-xml.cpp
 *   New questing system - XML
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

#include <boost/algorithm/string/case_conv.hpp>
#include "config.hpp"                               // for Config, gConfig
#include "quests.hpp"                               // for QuestCompletion
#include "xml.hpp"                                  // for NODE_NAME, newStr...

QuestCatRef::QuestCatRef(xmlNodePtr rootNode) {
    // Set up the defaults
    reqNum = 1;
    curNum = 0;
    area = gConfig->getDefaultArea();

    // And then read in the XML file
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Area")) xml::copyToString(area, curNode);
        else if(NODE_NAME(curNode, "Id")) xml::copyToNum(id, curNode);
        else if(NODE_NAME(curNode, "ReqAmt")) xml::copyToNum(reqNum, curNode);
        else if(NODE_NAME(curNode, "CurAmt")) xml::copyToNum(curNum, curNode);

        curNode = curNode->next;
    }
}
xmlNodePtr QuestCatRef::save(xmlNodePtr rootNode, const std::string& saveName) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, saveName.c_str());

    xml::newStringChild(curNode, "Area", area);
    xml::newNumChild(curNode, "Id", id);
    xml::newNumChild(curNode, "ReqAmt", reqNum);
    xml::newNumChild(curNode, "CurAmt", curNum);
    return(curNode);
}

QuestInfo::QuestInfo(xmlNodePtr rootNode) {
    std::string faction = "";

    questId = xml::getIntProp(rootNode, "Num");
    repeatable = sharable = false;
    expReward = minLevel = minFaction = alignmentChange = level = 0;
    repeatFrequency = QuestRepeatFrequency::REPEAT_NEVER;

    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Name")) xml::copyToString(name, curNode);
        else if(NODE_NAME(curNode, "Revision")) xml::copyToString(revision, curNode);
        else if(NODE_NAME(curNode, "Description")) xml::copyToString(description, curNode);
        else if(NODE_NAME(curNode, "ReceiveString")) xml::copyToString(receiveString, curNode);
        else if(NODE_NAME(curNode, "CompletionString")) xml::copyToString(completionString, curNode);
        else if(NODE_NAME(curNode, "TimesRepeatable")) xml::copyToNum(timesRepeatable, curNode);
        else if(NODE_NAME(curNode, "RepeatFrequency")) xml::copyToNum<QuestRepeatFrequency, short>(repeatFrequency, curNode);
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
                if(NODE_NAME(childNode, "Object")) initialItems.emplace_back(childNode);

                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "Requirements")) {
            xmlNodePtr childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Object")) itemsToGet.emplace_back(childNode);
                else if(NODE_NAME(childNode, "Monster")) mobsToKill.emplace_back(childNode);
                else if(NODE_NAME(childNode, "Room")) roomsToVisit.emplace_back(childNode);

                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "Rewards")) {
            xmlNodePtr childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Coins")) cashReward.load(childNode);
                else if(NODE_NAME(childNode, "Experience")) xml::copyToNum(expReward, childNode);
                else if(NODE_NAME(childNode, "AlignmentChange")) xml::copyToNum(alignmentChange, childNode);
                else if(NODE_NAME(childNode, "Object")) itemRewards.emplace_back(childNode);
                else if(NODE_NAME(childNode, "Faction")) {
                    xml::copyPropToString(faction, childNode, "id");
                    if(!faction.empty())
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


TalkResponse::TalkResponse(xmlNodePtr rootNode) {
    quest = nullptr;

    // And then read in the XML file
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Keyword")) keywords.push_back(boost::to_lower_copy(xml::getString(curNode)));
        else if(NODE_NAME(curNode, "Response")) xml::copyToString(response, curNode);
        else if(NODE_NAME(curNode, "Action")) {
            xml::copyToString(action, curNode);
            parseQuest();
        }

        curNode = curNode->next;
    }
}

xmlNodePtr TalkResponse::saveToXml(xmlNodePtr rootNode) const {
    xmlNodePtr talkNode = xml::newStringChild(rootNode, "TalkResponse");

    for(const auto& keyword : keywords) {
        xml::newStringChild(talkNode, "Keyword", keyword);
    }
    xml::newStringChild(talkNode, "Response", response);
    xml::newStringChild(talkNode, "Action", action);
    return(talkNode);
}



