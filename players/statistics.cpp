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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <boost/algorithm/string/erase.hpp>    // for erase_all
#include <boost/algorithm/string/trim.hpp>     // for trim
#include <boost/iterator/iterator_traits.hpp>  // for iterator_value<>::type
#include <cstring>                             // for strcmp
#include <ctime>                               // for ctime, time, time_t
#include <deque>                               // for _Deque_iterator
#include <iomanip>                             // for operator<<, setfill, setw
#include <list>                                // for list
#include <locale>                              // for locale
#include <map>                                 // for operator==, _Rb_tree_i...
#include <ostream>                             // for operator<<, basic_ostream
#include <string>                              // for string, char_traits
#include <string_view>                         // for string_view
#include <utility>                             // for pair

#include "cmd.hpp"                             // for cmd
#include "color.hpp"                           // for stripColor
#include "commands.hpp"                        // for cmdLevelHistory, cmdSt...
#include "creatureStreams.hpp"                 // for Streamable, ColorOff
#include "global.hpp"                          // for INV, MAG, MAX_SAVE
#include "mudObjects/creatures.hpp"            // for Creature
#include "mudObjects/monsters.hpp"             // for Monster
#include "mudObjects/objects.hpp"              // for Object
#include "mudObjects/players.hpp"              // for Player
#include "proto.hpp"                           // for up, getSaveName
#include "server.hpp"                          // for Server, gServer
#include "statistics.hpp"                      // for Statistics, LevelInfo
#include "stats.hpp"                           // for Stat
#include "xml.hpp"                             // for loadPlayer

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


void Statistics::setLevelInfo(int level, LevelInfo* levelInfo) {
    auto it = levelHistory.find(level);
    if(it != levelHistory.end()) {
        levelHistory.erase(it);
    }

    levelHistory.insert(LevelInfoMap::value_type(level, levelInfo));
}

LevelInfo* Statistics::getLevelInfo(int level) {
    auto it = levelHistory.find(level);
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
//                      update
//*********************************************************************

void StringStatistic::update(unsigned long num, std::string_view with) {
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
    if (this == &cr)
        return (*this);

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
    long t = time(nullptr);
    start = ctime(&t);
    boost::trim(start);

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
    levelHistoryStart = time(nullptr);
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

char stat_names[][4] = { "STR", "DEX", "CON", "INT", "PTY", "CHA" };

char* getStatName(int stat) {
    stat = std::min<int>(std::max<int>(stat - 1, 0), MAX_STAT);
    return(stat_names[stat]);
}


char save_names[][4] = { "LCK", "POI", "DEA", "BRE", "MEN", "SPL" };

char* getSaveName(int save) {
    save = std::min<int>(std::max<int>(save, 0), MAX_SAVE-1);
    return(save_names[save]);
}

void Statistics::displayLevelHistory(const std::shared_ptr<Player> viewer) {
    std::string padding;
    std::ostringstream oStr;
    // set left aligned
    oStr.setf(std::ios::right, std::ios::adjustfield);
    oStr.imbue(std::locale(""));

    oStr << "Leveling history times valid since ^C" << ctime(&levelHistoryStart) << "^x\n";

    LevelInfo *lInfo;
    for(LevelInfoMap::value_type p : levelHistory) {
        lInfo = p.second;
        time_t levelTime = lInfo->getLevelTime();
        std::string levelTimeStr = ctime(&levelTime);
        boost::erase_all(levelTimeStr, "\n");

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

void Statistics::display(const std::shared_ptr<Player> viewer, bool death) {
    std::string padding;
    std::ostringstream oStr;
    // set left aligned
    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr.imbue(std::locale(""));

    auto statParent = parent.lock();
    if(!statParent) return;

    if(death) {
        // if death = true, player will always be the owner
        oStr << "^WGeneral player statistics:^x\n"
             << "  Level:            ^C" << statParent->getLevel() << "^x\n"
             << "  Total Experience: ^C" << statParent->getExperience() << "^x\n"
             << "  Time Played:      ^C" << statParent->getTimePlayed() << "^x\n"
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


    int rooms = statParent->numDiscoveredRooms();
    int numRecipes = statParent->recipes.size();
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

unsigned long Statistics::calcToughness(std::shared_ptr<Creature> target) {
    unsigned long t = 0;

    if(target->isMonster()) {

        const std::shared_ptr<const Monster>  monster = target->getAsConstMonster();
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

std::string Statistics::damageWith(const std::shared_ptr<Player> player, const std::shared_ptr<Object>  weapon) {
    if(weapon)
        return(weapon->getObjStr(nullptr, INV | MAG, 1));
    return((std::string)"your " + player->getUnarmedWeaponSkill() + "s");
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

std::string Statistics::getTime() {
    return(start);
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

void Statistics::group(unsigned long num) { if(track) mostGroup = std::max(num, mostGroup); }

//*********************************************************************
//                      monster
//*********************************************************************

void Statistics::monster(std::shared_ptr<Monster>  monster) {
    if(!track || monster->isPet())
        return;
    mostMonster.update(calcToughness(monster), monster->getCName());
}

//*********************************************************************
//                      experience
//*********************************************************************

void Statistics::experience(unsigned long num, std::string_view with) {
    if(track) mostExperience.update(num, with);
}

//*********************************************************************
//                      attackDamage
//*********************************************************************

void Statistics::attackDamage(unsigned long num, std::string_view with) {
    if(track) mostAttackDamage.update(num, with);
}

//*********************************************************************
//                      magicDamage
//*********************************************************************

void Statistics::magicDamage(unsigned long num, std::string_view with) {
    if(track) mostMagicDamage.update(num, with);
}

//*********************************************************************
//                      setParent
//*********************************************************************

void Statistics::setParent(std::shared_ptr<Player> player) { parent = player; }

//*********************************************************************
//                      pkRank
//*********************************************************************
// This function returns the rank of the player based on how many
// pkills they have been in, and how many they have won

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

int cmdLevelHistory(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target = player;

    if(player->isDm() && cmnd->num > 1) {
        cmnd->str[1][0] = up(cmnd->str[1][0]);
        target = gServer->findPlayer(cmnd->str[1]);
        if(!target) {
            loadPlayer(cmnd->str[1], target);
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

    return(0);
}

//*********************************************************************
//                      cmdStatistics
//*********************************************************************

int cmdStatistics(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target = player;

    if(!strcmp(cmnd->str[1], "plyReset")) {
        player->statistics.reset();
        player->print("Your statistics have been plyReset.\n");
        player->print("Note that player kill statistics are always tracked and cannot be plyReset.\n");
        return(0);
    }

    if(player->isDm() && cmnd->num > 1) {
        cmnd->str[1][0] = up(cmnd->str[1][0]);
        target = gServer->findPlayer(cmnd->str[1]);
        if(!target) {
            loadPlayer(cmnd->str[1], target);
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
        *player << ColorOn << "You may type ^Wstats monReset^x to plyReset your statistic counts to zero.\n" << ColorOff;
        *player << "You may use the set, clear, and toggle commands to control tracking of statistics.\n";
    }

    return(0);
}
