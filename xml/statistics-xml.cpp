/*
 * statistics-xml.cpp
 *   Player statistics - XML
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <libxml/parser.h>          // for xmlNodePtr, xmlNode
#include "statistics.hpp"           // for Statistics, Level...
#include "xml.hpp"                  // for copyToNum, saveNo...

LevelInfo::LevelInfo(xmlNodePtr rootNode) {

    level = 0;
    hpGain = 0;
    mpGain = 0;
    statUp = 0;
    saveGain = 0;
    levelTime = 0;

    xmlNodePtr childNode = rootNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Level")) xml::copyToNum(level, childNode);
        else if(NODE_NAME(childNode, "HpGain")) xml::copyToNum(hpGain, childNode);
        else if(NODE_NAME(childNode, "MpGain")) xml::copyToNum(mpGain, childNode);
        else if(NODE_NAME(childNode, "StatUp")) xml::copyToNum(statUp, childNode);
        else if(NODE_NAME(childNode, "SaveGain")) xml::copyToNum(saveGain, childNode);
        else if(NODE_NAME(childNode, "LevelTime")) xml::copyToNum(levelTime, childNode);
        childNode = childNode->next;
    }

    if(level == 0)
        throw std::exception();

}
void LevelInfo::save(xmlNodePtr rootNode) {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "LevelInfo");

    xml::saveNonZeroNum(curNode, "Level", level);
    xml::saveNonZeroNum(curNode, "HpGain", hpGain);
    xml::saveNonZeroNum(curNode, "MpGain", mpGain);
    xml::saveNonZeroNum(curNode, "StatUp", statUp);
    xml::saveNonZeroNum(curNode, "SaveGain", saveGain);
    xml::saveNonZeroNum(curNode, "LevelTime", levelTime);


}


//*********************************************************************
//                      save
//*********************************************************************

void StringStatistic::save(xmlNodePtr rootNode, std::string_view nodeName) const {
    if(!value && name.empty())
        return;
    xmlNodePtr curNode = xml::newStringChild(rootNode, nodeName);

    xml::saveNonZeroNum(curNode, "Value", value);
    xml::saveNonNullString(curNode, "Name", name);
}

//*********************************************************************
//                      load
//*********************************************************************

void StringStatistic::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Value")) xml::copyToNum(value, childNode);
        else if(NODE_NAME(childNode, "Name")) xml::copyToBString(name, childNode);
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void Statistics::save(xmlNodePtr rootNode, std::string_view nodeName) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, nodeName);

    xml::newNumChild(curNode, "Track", track);
    xml::saveNonNullString(curNode, "Start", start);

    xml::saveNonZeroNum(curNode, "NumSwings", numSwings);
    xml::saveNonZeroNum(curNode, "NumHits", numHits);
    xml::saveNonZeroNum(curNode, "NumMisses", numMisses);
    xml::saveNonZeroNum(curNode, "NumFumbles", numFumbles);
    xml::saveNonZeroNum(curNode, "NumDodges", numDodges);
    xml::saveNonZeroNum(curNode, "NumCriticals", numCriticals);
    xml::saveNonZeroNum(curNode, "NumTimesHit", numTimesHit);
    xml::saveNonZeroNum(curNode, "NumTimesMissed", numTimesMissed);
    xml::saveNonZeroNum(curNode, "NumTimesFled", numTimesFled);
    xml::saveNonZeroNum(curNode, "NumPkIn", numPkIn);
    xml::saveNonZeroNum(curNode, "NumPkWon", numPkWon);
    xml::saveNonZeroNum(curNode, "NumCasts", numCasts);
    xml::saveNonZeroNum(curNode, "NumOffensiveCasts", numOffensiveCasts);
    xml::saveNonZeroNum(curNode, "NumHealingCasts", numHealingCasts);
    xml::saveNonZeroNum(curNode, "NumKills", numKills);
    xml::saveNonZeroNum(curNode, "NumDeaths", numDeaths);
    xml::saveNonZeroNum(curNode, "NumThefts", numThefts);
    xml::saveNonZeroNum(curNode, "NumAttemptedThefts", numAttemptedThefts);
    xml::saveNonZeroNum(curNode, "NumSaves", numSaves);
    xml::saveNonZeroNum(curNode, "NumAttemptedSaves", numAttemptedSaves);
    xml::saveNonZeroNum(curNode, "NumRecalls", numRecalls);
    xml::saveNonZeroNum(curNode, "NumLagouts", numLagouts);
    xml::saveNonZeroNum(curNode, "NumWandsUsed", numWandsUsed);
    xml::saveNonZeroNum(curNode, "NumTransmutes", numTransmutes);
    xml::saveNonZeroNum(curNode, "NumPotionsDrank", numPotionsDrank);
    xml::saveNonZeroNum(curNode, "NumFishCaught", numFishCaught);
    xml::saveNonZeroNum(curNode, "NumItemsCrafted", numItemsCrafted);
    xml::saveNonZeroNum(curNode, "NumCombosOpened", numCombosOpened);
    xml::saveNonZeroNum(curNode, "MostGroup", mostGroup);
    xml::saveNonZeroNum(curNode, "ExpLost", expLost);
    xml::saveNonZeroNum(curNode, "LastExpLoss", lastExpLoss);
    mostMonster.save(curNode, "MostMonster");
    mostAttackDamage.save(curNode, "MostAttackDamage");
    mostMagicDamage.save(curNode, "MostMagicDamage");
    mostExperience.save(curNode, "MostExperience");

    xml::saveNonZeroNum(curNode, "LevelHistoryStart", levelHistoryStart);
    xmlNodePtr historyNode = xml::newStringChild(curNode, "LevelHistory", "");
    LevelInfoMap::const_iterator it;
    for( it = levelHistory.begin() ; it != levelHistory.end() ; it++) {
        (*it).second->save(historyNode);
    }
//  for(LevelInfoMap::value_type p : levelHistory) {
//      p.second->save(historyNode);
//  }
}

//*********************************************************************
//                      load
//*********************************************************************

void Statistics::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Track")) xml::copyToBool(track, childNode);
        else if(NODE_NAME(childNode, "Start")) xml::copyToBString(start, childNode);
        else if(NODE_NAME(childNode, "NumSwings")) xml::copyToNum(numSwings, childNode);
        else if(NODE_NAME(childNode, "NumHits")) xml::copyToNum(numHits, childNode);
        else if(NODE_NAME(childNode, "NumMisses")) xml::copyToNum(numMisses, childNode);
        else if(NODE_NAME(childNode, "NumFumbles")) xml::copyToNum(numFumbles, childNode);
        else if(NODE_NAME(childNode, "NumDodges")) xml::copyToNum(numDodges, childNode);
        else if(NODE_NAME(childNode, "NumCriticals")) xml::copyToNum(numCriticals, childNode);
        else if(NODE_NAME(childNode, "NumTimesHit")) xml::copyToNum(numTimesHit, childNode);
        else if(NODE_NAME(childNode, "NumTimesMissed")) xml::copyToNum(numTimesMissed, childNode);
        else if(NODE_NAME(childNode, "NumTimesFled")) xml::copyToNum(numTimesFled, childNode);
        else if(NODE_NAME(childNode, "NumPkIn")) xml::copyToNum(numPkIn, childNode);
        else if(NODE_NAME(childNode, "NumPkWon")) xml::copyToNum(numPkWon, childNode);
        else if(NODE_NAME(childNode, "NumCasts")) xml::copyToNum(numCasts, childNode);
        else if(NODE_NAME(childNode, "NumOffensiveCasts")) xml::copyToNum(numOffensiveCasts, childNode);
        else if(NODE_NAME(childNode, "NumHealingCasts")) xml::copyToNum(numHealingCasts, childNode);
        else if(NODE_NAME(childNode, "NumKills")) xml::copyToNum(numKills, childNode);
        else if(NODE_NAME(childNode, "NumDeaths")) xml::copyToNum(numDeaths, childNode);
        else if(NODE_NAME(childNode, "NumThefts")) xml::copyToNum(numThefts, childNode);
        else if(NODE_NAME(childNode, "NumAttemptedThefts")) xml::copyToNum(numAttemptedThefts, childNode);
        else if(NODE_NAME(childNode, "NumSaves")) xml::copyToNum(numSaves, childNode);
        else if(NODE_NAME(childNode, "NumAttemptedSaves")) xml::copyToNum(numAttemptedSaves, childNode);
        else if(NODE_NAME(childNode, "NumRecalls")) xml::copyToNum(numRecalls, childNode);
        else if(NODE_NAME(childNode, "NumLagouts")) xml::copyToNum(numLagouts, childNode);
        else if(NODE_NAME(childNode, "NumWandsUsed")) xml::copyToNum(numWandsUsed, childNode);
        else if(NODE_NAME(childNode, "NumTransmutes")) xml::copyToNum(numTransmutes, childNode);
        else if(NODE_NAME(childNode, "NumPotionsDrank")) xml::copyToNum(numPotionsDrank, childNode);
        else if(NODE_NAME(childNode, "NumFishCaught")) xml::copyToNum(numFishCaught, childNode);
        else if(NODE_NAME(childNode, "NumItemsCrafted")) xml::copyToNum(numItemsCrafted, childNode);
        else if(NODE_NAME(childNode, "NumCombosOpened")) xml::copyToNum(numCombosOpened, childNode);
        else if(NODE_NAME(childNode, "MostGroup")) xml::copyToNum(mostGroup, childNode);
        else if(NODE_NAME(childNode, "MostMonster")) mostMonster.load(childNode);
        else if(NODE_NAME(childNode, "MostAttackDamage")) mostAttackDamage.load(childNode);
        else if(NODE_NAME(childNode, "MostMagicDamage")) mostMagicDamage.load(childNode);
        else if(NODE_NAME(childNode, "MostExperience")) mostExperience.load(childNode);
        else if(NODE_NAME(childNode, "ExpLost")) xml::copyToNum(expLost, childNode);
        else if(NODE_NAME(childNode, "LastExpLoss")) xml::copyToNum(lastExpLoss, childNode);
        else if(NODE_NAME(childNode, "LevelHistoryStart")) xml::copyToNum(levelHistoryStart, childNode);
        else if(NODE_NAME(childNode, "LevelHistory")) {
            xmlNodePtr infoNode = childNode->children;
            while(infoNode) {
                try {
                    auto *info = new LevelInfo(infoNode);
                    levelHistory.insert(LevelInfoMap::value_type(info->getLevel(), info));
                } catch (...) {

                }
                infoNode = infoNode->next;
            }
        }

        childNode = childNode->next;
    }
}



