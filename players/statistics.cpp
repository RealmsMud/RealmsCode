/*
 * statistics.cpp
 *   Player statistics
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

#include <iomanip>

#include "commands.hpp"
#include "creatures.hpp"
#include "mud.hpp"
#include "server.hpp"
#include "statistics.hpp"
#include "xml.hpp"

//*********************************************************************
//                      LevelInfo
//*********************************************************************

LevelInfo::LevelInfo(int pLevel, int pHp, int pMp, int pStat, int pSave, time_t pTime) {
    level = pLevel;
    hpGain = pHp;
    mpGain = pMp;
    statUp = pStat;
    saveGain = pSave;
    levelTime = pTime;
}

LevelInfo::LevelInfo(const LevelInfo* l) {
    level = l->level;;
    hpGain = l->hpGain;
    mpGain = l->mpGain;
    statUp = l->statUp;
    saveGain = l->saveGain;
    levelTime = l->levelTime;

}
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

void Statistics::setLevelInfo(int level, LevelInfo* levelInfo) {
    LevelInfoMap::iterator it = levelHistory.find(level);
    if(it != levelHistory.end()) {
        levelHistory.erase(it);
    }

    levelHistory.insert(LevelInfoMap::value_type(level, levelInfo));
}

LevelInfo* Statistics::getLevelInfo(int level) {
    LevelInfoMap::iterator it = levelHistory.find(level);
    if(it == levelHistory.end())
        return(nullptr);

    return(it->second);
}

int LevelInfo::getLevel() { return(level); }
int LevelInfo::getHpGain() { return(hpGain); }
int LevelInfo::getMpGain() { return(mpGain); }
int LevelInfo::getStatUp() { return(statUp); }
int LevelInfo::getSaveGain() { return(saveGain); }
time_t LevelInfo::getLevelTime() { return(levelTime); }

//*********************************************************************
//                      StringStatistic
//*********************************************************************

StringStatistic::StringStatistic() {
    reset();
}

//*********************************************************************
//                      save
//*********************************************************************

void StringStatistic::save(xmlNodePtr rootNode, bstring nodeName) const {
    if(!value && name == "")
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
//                      update
//*********************************************************************

void StringStatistic::update(unsigned long num, bstring with) {
    if(num > value) {
        value = num;
        name = stripColor(with);
    }
}

//*********************************************************************
//                      reset
//*********************************************************************

void StringStatistic::reset() {
    value = 0;
    name = "";
}

//*********************************************************************
//                      Statistics
//*********************************************************************

Statistics::Statistics() {
    track = true;
    numPkWon = numPkIn = 0;
    reset();
}
Statistics::Statistics(Statistics& cr) {
    doCopy(cr);
}

Statistics::Statistics(const Statistics& cr) {
    doCopy(cr);
}

Statistics& Statistics::operator=(const Statistics& cr) {
    doCopy(cr);
    return(*this);
}

Statistics::~Statistics() {
    reset();
}

void Statistics::doCopy(const Statistics& st) {
    start = st.start;

    // combat
    numSwings = st.numSwings;
    numHits = st.numHits;
    numMisses = st.numMisses;
    numFumbles = st.numFumbles;
    numDodges = st.numDodges;
    numCriticals = st.numCriticals;
    numTimesHit = st.numTimesHit;
    numTimesMissed = st.numTimesMissed;
    numTimesFled = st.numTimesFled;
    numPkIn = st.numPkIn;
    numPkWon = st.numPkWon;
    // magic
    numCasts = st.numCasts;
    numOffensiveCasts = st.numOffensiveCasts;
    numHealingCasts = st.numHealingCasts;
    numWandsUsed = st.numWandsUsed;
    numTransmutes = st.numTransmutes;
    numPotionsDrank = st.numPotionsDrank;
    // death
    numKills = st.numKills;
    numDeaths = st.numDeaths;
    expLost = st.expLost; // New
    lastExpLoss = st.lastExpLoss; // New

    // other
    numThefts = st.numThefts;
    numAttemptedThefts = st.numAttemptedThefts;
    numSaves = st.numSaves;
    numAttemptedSaves = st.numAttemptedSaves;
    numRecalls = st.numRecalls;
    numLagouts = st.numLagouts;
    numFishCaught = st.numFishCaught;
    numItemsCrafted = st.numItemsCrafted;
    numCombosOpened = st.numCombosOpened;
    levelHistoryStart = st.levelHistoryStart;

    // most
    mostGroup = st.mostGroup;
    mostExperience = st.mostExperience; // New
    mostMonster = st.mostMonster;
    mostAttackDamage = st.mostAttackDamage;
    mostMagicDamage = st.mostMagicDamage;

    // so we can reference
    parent = st.parent;
    LevelInfoMap::const_iterator it;
    LevelInfo* lInfo;
    for(it = st.levelHistory.begin() ; it != st.levelHistory.end() ; it++) {
        lInfo = new LevelInfo((*it).second);
        levelHistory[(*it).first] = lInfo;
    }
}
//*********************************************************************
//                      reset
//*********************************************************************

void Statistics::reset() {
    numSwings = numHits = numMisses = numFumbles = numDodges =
        numCriticals = numTimesHit = numTimesMissed = numTimesFled =
        numCasts = numOffensiveCasts = numHealingCasts = numKills =
        numDeaths = numThefts = numAttemptedThefts = numSaves =
        numAttemptedSaves = numRecalls = numLagouts = numWandsUsed =
        numPotionsDrank = numItemsCrafted = numFishCaught = numCombosOpened =
        mostGroup = numPkIn = numPkWon = numTransmutes = lastExpLoss = expLost = 0;
    levelHistoryStart = 0;
    mostMonster.reset();
    mostAttackDamage.reset();
    mostMagicDamage.reset();
    mostExperience.reset();
    long t = time(0);
    start = ctime(&t);
    start = start.trim();

    LevelInfoMap::iterator it;
    for(it = levelHistory.begin() ; it != levelHistory.end() ; ) {
        delete (*it++).second;
    }
    levelHistory.clear();
}

//*********************************************************************
//                      StartLevelHistoryTracking
//*********************************************************************
// Timing of level history after this point in time should be accurate

void Statistics::startLevelHistoryTracking() {
    levelHistoryStart = time(0);
}
//*********************************************************************
//                      getLevelHistoryStart
//*********************************************************************

time_t Statistics::getLevelHistoryStart() {
    return(levelHistoryStart);
}
//*********************************************************************
//                      displayLevelHistory
//*********************************************************************

void Statistics::displayLevelHistory(const Player* viewer) {
    bstring padding;
    std::ostringstream oStr;
    // set left aligned
    oStr.setf(std::ios::right, std::ios::adjustfield);
    oStr.imbue(std::locale(""));

    oStr << "Leveling history times valid since ^C" << ctime(&levelHistoryStart) << "^x\n";

    LevelInfo *lInfo;
    for(LevelInfoMap::value_type p : levelHistory) {
        lInfo = p.second;
        time_t levelTime = lInfo->getLevelTime();
        bstring levelTimeStr = ctime(&levelTime);
        levelTimeStr.Remove('\n');

        oStr << "Level ^C" << std::setfill('0') << std::setw(2) << lInfo->getLevel() << "^x Date: ^C" << levelTimeStr << "^x";
        oStr << " Gains - HP: ^C" << std::setfill('0') << std::setw(2) << lInfo->getHpGain() << "^x MP: ^C";
        oStr << std::setfill('0') << std::setw(2) << lInfo->getMpGain() << "^x Stat: ^C";
        oStr << getStatName(lInfo->getStatUp()) << "^x Save: ^C" << getSaveName(lInfo->getSaveGain()) << "^x\n";
    }

    viewer->printColor("%s\n", oStr.str().c_str());
}


//*********************************************************************
//                      display
//*********************************************************************

void Statistics::display(const Player* viewer, bool death) {
    bstring padding;
    std::ostringstream oStr;
    // set left aligned
    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr.imbue(std::locale(""));

    if(death) {
        // if death = true, player will always be the owner
        oStr << "^WGeneral player statistics:^x\n"
             << "  Level:            ^C" << parent->getLevel() << "^x\n"
             << "  Total Experience: ^C" << parent->getExperience() << "^x\n"
             << "  Time Played:      ^C" << parent->getTimePlayed() << "^x\n"
             << "\n";
    }

    oStr << "Tracking statistics since: ^C" << start << "\n";
    if( numSwings ||
        numDodges ||
        numTimesHit ||
        numTimesMissed ||
        numDeaths ||
        numKills ||
        numPkIn
    )
        oStr << "^WCombat^x\n";
    if(numSwings) {
        oStr << "  Swings:       ^C" << numSwings << "^x\n";
        if(numHits)
            oStr << "  Hits:         ^C" << numHits << " (" << (numHits * 100 / numSwings) << "%)^x\n";
        if(numMisses)
            oStr << "  Misses:       ^C" << numMisses << " (" << (numMisses * 100 / numSwings) << "%)^x\n";
        if(numFumbles)
            oStr << "  Fumbles:      ^C" << numFumbles << " (" << (numFumbles * 100 / numSwings) << "%)^x\n";
        if(numCriticals)
            oStr << "  Criticals:    ^C" << numCriticals << " (" << (numCriticals * 100 / numSwings) << "%)^x\n";
    }
    if(numDodges)
        oStr << "  Dodges:       ^C" << numDodges << "^x\n";
    if(numTimesHit)
        oStr << "  Times Hit:    ^C" << numTimesHit << "^x\n";
    if(numTimesMissed)
        oStr << "  Times Missed: ^C" << numTimesMissed << "^x\n";
    // Don't want to see this info after all
    //if(numTimesFled)
    //  oStr << "  Times Fled:   ^C" << numTimesFled << "^x\n";
    if(numDeaths)
        oStr << "  Deaths:       ^C" << numDeaths << "^x\n";
    if(numKills)
        oStr << "  Kills:        ^C" << numKills << "^x\n";
    if(numPkIn)
        oStr << "  Player kills: ^C" << numPkWon << "/" << numPkIn << " (" << (numPkWon * 100 / numPkIn) << "%)^x\n";


    padding = (numTransmutes > 0 ? "     " : "");

    if( numCasts ||
        numOffensiveCasts ||
        numHealingCasts ||
        numWandsUsed ||
        numPotionsDrank ||
        numTransmutes
    )
        oStr << "\n^WMagic^x\n";
    if(numHealingCasts)
        oStr << "  Spells cast: ^C" << padding << numCasts << "^x\n";
    if(numCasts) {
        if(numOffensiveCasts)
            oStr << "     Offensive Spells: ^C" << numOffensiveCasts << " (" << (numOffensiveCasts * 100 / numCasts) << "%)^x\n";
        if(numHealingCasts)
            oStr << "     Healing Spells:   ^C" << numHealingCasts << " (" << (numHealingCasts * 100 / numCasts) << "%)^x\n";
    }
    if(numWandsUsed)
        oStr << "  Wands used:       ^C" << padding << numWandsUsed << "^x\n";
    if(numTransmutes)
        oStr << "  Wands transmuted: ^C" << padding << numTransmutes << "^x\n";
    if(numPotionsDrank)
        oStr << "  Potions used:     ^C" << padding << numPotionsDrank << "^x\n";


    if( mostGroup ||
        mostMonster.value ||
        mostAttackDamage.value ||
        mostMagicDamage.value
    )
        oStr << "\n^WMost / Largest^x\n";
    if(mostGroup)
        oStr << "  Largest group:               ^C" << mostGroup << "^x\n";
    if(mostMonster.value)
        oStr << "  Toughest monster killed:     ^C" << mostMonster.name << "^x\n";
    if(mostExperience.value)
        oStr << "  Highest experience gained:   ^C" << mostExperience.value << " from " << mostExperience.name << "^x\n";
    if(mostAttackDamage.value)
        oStr << "  Most damage in one attack:   ^C" << mostAttackDamage.value << " with " << mostAttackDamage.name << "^x\n";
    if(mostMagicDamage.value)
        oStr << "  Most damage in one spell:    ^C" << mostMagicDamage.value << " with " << mostMagicDamage.name << "^x\n";
    if(expLost)
        oStr << "  Experience Lost:             ^C" << expLost << "^x\n";


    int rooms = parent->numDiscoveredRooms();
    int numRecipes = parent->recipes.size();
    if( (numThefts && numAttemptedThefts) ||
        (numSaves && numAttemptedSaves) ||
        numRecalls ||
        numLagouts ||
        numFishCaught ||
        numItemsCrafted ||
        numRecipes ||
        numCombosOpened ||
        rooms
    )
        oStr << "\n^WOther^x\n";
    if(numThefts && numAttemptedThefts)
        oStr << "  Items stolen:                ^C" << numThefts << "/" << numAttemptedThefts << " (" << (numThefts * 100 / numAttemptedThefts) << "%)^x\n";
    if(numSaves && numAttemptedSaves)
        oStr << "  Saving throws made:          ^C" << numSaves << "/" << numAttemptedSaves << " (" << (numSaves * 100 / numAttemptedSaves) << "%)^x\n";
    if(numRecalls)
        oStr << "  Hazies/word-of-recalls used: ^C" << numRecalls << "^x\n";
    if(numLagouts)
        oStr << "  Lagouts:                     ^C" << numLagouts << "^x\n";
    if(numFishCaught)
        oStr << "  Fish caught:                 ^C" << numFishCaught << "^x\n";
    if(numItemsCrafted)
        oStr << "  Items crafted:               ^C" << numItemsCrafted << "^x\n";
    if(numRecipes)
        oStr << "  Recipes known:               ^C" << numRecipes << "^x\n";
    if(numCombosOpened)
        oStr << "  Combinations opened:         ^C" << numCombosOpened << "^x\n";
    if(rooms)
        oStr << "  Special rooms discovered:    ^C" << rooms << "^x\n";

    viewer->printColor("%s\n", oStr.str().c_str());
}

//*********************************************************************
//                      calcToughness
//*********************************************************************

unsigned long Statistics::calcToughness(Creature* target) {
    unsigned long t = 0;

    if(target->isMonster()) {

        const Monster* monster = target->getAsConstMonster();
        t += target->hp.getMax();
        t += target->getAttackPower();
        t += target->getWeaponSkill();
        t += target->getDefenseSkill();
        // level is weighted
        t += target->getLevel() * 2;
        // only a percentage of their mana counts
        t += (target->mp.getMax() * monster->getCastChance() / 100);

    } else {

        t = target->getLevel() * 10;

    }

    return(t);
}

//*********************************************************************
//                      damageWith
//*********************************************************************

bstring Statistics::damageWith(const Player* player, const Object* weapon) {
    if(weapon)
        return(weapon->getObjStr(nullptr, INV | MAG, 1));
    return((bstring)"your " + player->getUnarmedWeaponSkill() + "s");
}

//*********************************************************************
//                      pkDemographics
//*********************************************************************

unsigned long Statistics::pkDemographics() const {
    if(numPkIn == numPkWon)
        return(0);
    return(pkRank());
}

//*********************************************************************
//                      getTime
//*********************************************************************

bstring Statistics::getTime() {
    return(start);
}

//*********************************************************************
//                      save
//*********************************************************************

void Statistics::save(xmlNodePtr rootNode, bstring nodeName) const {
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
                    LevelInfo *info = new LevelInfo(infoNode);
                    levelHistory.insert(LevelInfoMap::value_type(info->getLevel(), info));
                } catch (...) {

                }
                infoNode = infoNode->next;
            }
        }

        childNode = childNode->next;
    }
}

//*********************************************************************
//                      swing
//*********************************************************************

void Statistics::swing() { if(track) numSwings++; }

//*********************************************************************
//                      hit
//*********************************************************************

void Statistics::hit() { if(track) numHits++; }

//*********************************************************************
//                      miss
//*********************************************************************

void Statistics::miss() { if(track) numMisses++; }

//*********************************************************************
//                      fumble
//*********************************************************************

void Statistics::fumble() { if(track) numFumbles++; }

//*********************************************************************
//                      dodge
//*********************************************************************

void Statistics::dodge() { if(track) numDodges++; }

//*********************************************************************
//                      critical
//*********************************************************************

void Statistics::critical() { if(track) numCriticals++; }

//*********************************************************************
//                      wasHit
//*********************************************************************

void Statistics::wasHit() { if(track) numTimesHit++; }

//*********************************************************************
//                      wasMissed
//*********************************************************************

void Statistics::wasMissed() { if(track) numTimesMissed++; }

//*********************************************************************
//                      flee
//*********************************************************************

void Statistics::flee() { if(track) numTimesFled++; }

//*********************************************************************
//                      winPk
//*********************************************************************
// because these two stats are older, they're always tracked

void Statistics::winPk() { numPkIn++; numPkWon++; }

//*********************************************************************
//                      losePk
//*********************************************************************

void Statistics::losePk() { numPkIn++; }

//*********************************************************************
//                      cast
//*********************************************************************

void Statistics::cast() { if(track) numCasts++; }

//*********************************************************************
//                      offensiveCast
//*********************************************************************

void Statistics::offensiveCast() { if(track) numOffensiveCasts++; }

//*********************************************************************
//                      healingCast
//*********************************************************************

void Statistics::healingCast() { if(track) numHealingCasts++; }

//*********************************************************************
//                      kill
//*********************************************************************

void Statistics::kill() { if(track) numKills++; }

//*********************************************************************
//                      die
//*********************************************************************

void Statistics::die() { if(track) numDeaths++; }

//*********************************************************************
//                      experienceLost
//*********************************************************************

void Statistics::experienceLost(unsigned long amt) {
    // We only track the total if they're tracking statistics
    if (track)
        expLost += amt;
    // However, we always track the last exp loss (For resurrect)
    lastExpLoss = amt;
}
void Statistics::setExperienceLost(unsigned long amt) {
    expLost = amt;
}

//*********************************************************************
//                      steal
//*********************************************************************

void Statistics::steal() { if(track) numThefts++; }

//*********************************************************************
//                      attemptSteal
//*********************************************************************

void Statistics::attemptSteal() { if(track) numAttemptedThefts++; }

//*********************************************************************
//                      save
//*********************************************************************

void Statistics::save() { if(track) numSaves++; }

//*********************************************************************
//                      attemptSave
//*********************************************************************

void Statistics::attemptSave() { if(track) numAttemptedSaves++; }

//*********************************************************************
//                      recall
//*********************************************************************

void Statistics::recall() { if(track) numRecalls++; }

//*********************************************************************
//                      lagout
//*********************************************************************

void Statistics::lagout() { if(track) numLagouts++; }

//*********************************************************************
//                      wand
//*********************************************************************

void Statistics::wand() { if(track) numWandsUsed++; }

//*********************************************************************
//                      transmute
//*********************************************************************

void Statistics::transmute() { if(track) numTransmutes++; }

//*********************************************************************
//                      potion
//*********************************************************************

void Statistics::potion() { if(track) numPotionsDrank++; }

//*********************************************************************
//                      fish
//*********************************************************************

void Statistics::fish() { if(track) numFishCaught++; }

//*********************************************************************
//                      craft
//*********************************************************************

void Statistics::craft() { if(track) numItemsCrafted++; }

//*********************************************************************
//                      combo
//*********************************************************************

void Statistics::combo() { if(track) numCombosOpened++; }

//*********************************************************************
//                      group
//*********************************************************************

void Statistics::group(unsigned long num) { if(track) mostGroup = MAX(num, mostGroup); }

//*********************************************************************
//                      monster
//*********************************************************************

void Statistics::monster(Monster* monster) {
    if(!track || monster->isPet())
        return;
    mostMonster.update(calcToughness(monster), monster->getCName());
}

//*********************************************************************
//                      experience
//*********************************************************************

void Statistics::experience(unsigned long num, bstring with) {
    if(track) mostExperience.update(num, with);
}

//*********************************************************************
//                      attackDamage
//*********************************************************************

void Statistics::attackDamage(unsigned long num, bstring with) {
    if(track) mostAttackDamage.update(num, with);
}

//*********************************************************************
//                      magicDamage
//*********************************************************************

void Statistics::magicDamage(unsigned long num, bstring with) {
    if(track) mostMagicDamage.update(num, with);
}

//*********************************************************************
//                      setParent
//*********************************************************************

void Statistics::setParent(Player* player) { parent = player; }

//*********************************************************************
//                      pkRank
//*********************************************************************
// This function returns the rank of the player based on how many
// pkills they has been in, and how many they have won

unsigned long Statistics::pkRank() const {
    if(!numPkWon || !numPkIn)
        return(0);
    return(numPkWon * 100 / numPkIn);
}

//*********************************************************************
//                      getPkin
//*********************************************************************

unsigned long Statistics::getPkin() const { return(numPkIn); }

//*********************************************************************
//                      getPkwon
//*********************************************************************

unsigned long Statistics::getPkwon() const { return(numPkWon); }

//*********************************************************************
//                      getLostExperience
//*********************************************************************

unsigned long Statistics::getLostExperience() const { return(expLost); }

//*********************************************************************
//                      getLastExperienceLoss
//*********************************************************************

unsigned long Statistics::getLastExperienceLoss() const { return(lastExpLoss); }


//*********************************************************************
//                      setPkin
//*********************************************************************
// remove when all players are up to 2.42i

void Statistics::setPkin(unsigned long p) { numPkIn = p; }

//*********************************************************************
//                      setPkwon
//*********************************************************************

void Statistics::setPkwon(unsigned long p) { numPkWon = p; }


//*********************************************************************
//                      cmdLevelHistory
//*********************************************************************

int cmdLevelHistory(Player* player, cmd* cmnd) {
    Player* target = player;
    bool online=true;

    if(player->isDm() && cmnd->num > 1) {
        cmnd->str[1][0] = up(cmnd->str[1][0]);
        target = gServer->findPlayer(cmnd->str[1]);
        if(!target) {
            loadPlayer(cmnd->str[1], &target);
            online = false;
            // If the player is offline, init() won't be run and the statistics object won't
            // get its parent set. Do so now.
            if(target)
                target->statistics.setParent(target);
        }
        if(!target) {
            player->print("That player does not exist.\n");
            return(0);
        }
        player->printColor("^W%s's Level History\n", target->getCName());
    }

    target->statistics.displayLevelHistory(player);

    if(!online)
        free_crt(target);
    return(0);
}

//*********************************************************************
//                      cmdStatistics
//*********************************************************************

int cmdStatistics(Player* player, cmd* cmnd) {
    Player* target = player;
    bool online=true;

    if(!strcmp(cmnd->str[1], "reset")) {
        player->statistics.reset();
        player->print("Your statistics have been reset.\n");
        player->print("Note that player kill statistics are always tracked and cannot be reset.\n");
        return(0);
    }

    if(player->isDm() && cmnd->num > 1) {
        cmnd->str[1][0] = up(cmnd->str[1][0]);
        target = gServer->findPlayer(cmnd->str[1]);
        if(!target) {
            loadPlayer(cmnd->str[1], &target);
            online = false;
            // If the player is offline, init() won't be run and the statistics object won't
            // get its parent set. Do so now.
            if(target)
                target->statistics.setParent(target);
        }
        if(!target) {
            player->print("That player does not exist.\n");
            return(0);
        }
        player->printColor("^W%s's Statistics\n", target->getCName());
    }

    target->statistics.display(player);

    if(target == player) {
        *player << ColorOn << "You may type ^Wstats reset^x to reset your statistic counts to zero.\n" << ColorOff;
        *player << "You may use the set, clear, and toggle commands to control tracking of statistics.\n";
    }

    if(!online)
        free_crt(target);
    return(0);
}
