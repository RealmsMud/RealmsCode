/*
 * demographics.cpp
 *   Handles the generation of demographics.
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

#include <dirent.h>                                 // for closedir, dirent
#include <fcntl.h>                                  // for open, O_CREAT
#include <libxml/parser.h>                          // for xmlCleanupParser
#include <unistd.h>                                 // for write, close, unlink
#include <boost/dynamic_bitset.hpp>
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <cctype>                                   // for isupper
#include <cstdio>                                   // for sprintf
#include <cstdlib>                                  // for exit
#include <cstring>                                  // for strlen, strcat
#include <ctime>                                    // for ctime, time
#include <ostream>                                  // for operator<<, ostream
#include <fstream>                                  // for fstream
#include <iostream>                                 // for ofstream
#include <string>                                   // for allocator, string
#include <boost/algorithm/string/trim.hpp>

#include "calendar.hpp"                             // for cDay, Calendar
#include "config.hpp"                               // for Config, gConfig
#include "deityData.hpp"                            // for DeityData
#include "global.hpp"                               // for STAFF, MAX_PLAYAB...
#include "lasttime.hpp"                             // for lasttime
#include "mud.hpp"                                  // for ACC, LT_AGE
#include "mudObjects/players.hpp"                   // for Player
#include "paths.hpp"                                // for Sign, Player
#include "proto.hpp"                                // for zero, get_class_s...
#include "raceData.hpp"                             // for RaceData
#include "server.hpp"                               // for Server
#include "statistics.hpp"                           // for Statistics
#include "xml.hpp"                                  // for copyToNum, bad_le...

#define MINIMUM_LEVEL   3

std::string getFullClassName(CreatureClass cClass, CreatureClass secondClass);

void printEnding(std::ofstream& out) {
    out << "    |                                      |\n";
    out << "    |    __________________________________|_\n";
    out << "    \\   /                                   /\n";
    out << "     \\_/___________________________________/\n\n";
}

class Highest {
public:
    std::string playerName;
    long amount{};
    long count{};
};

void doDemographics() {

#ifndef NODEMOGRAPHICS
    DIR *dir;
    Statistics statistics;
    boost::dynamic_bitset<> spells{256};
    std::shared_ptr<cDay> birthday;
    const Calendar *calendar = gConfig->getCalendar();
    int cClass = 0, cClass2 = 0, race = 0, deity = 0, totalCount = 0, deityCount = 0;
    int i, age;
    int level = 0, numRooms=0;
    long exp, numSpells;
    long money[5], cgold, t = 0;
    std::map<std::string, Highest> highest;
    struct dirent *dirp;
    LastTime lasttime[128];
    xmlDocPtr xmlDoc;
    xmlNodePtr rootNode, curNode, childNode;

    std::set<std::string> classList;
    std::set<std::string> multiClassList;
    std::set<std::string> raceList;
    std::set<std::string> deityList;

    t = time(nullptr);
    std::clog << "Begining demographics routine.\n";


    // load the directory all the players are stored in
    std::clog << "Opening " << Path::Player << "...";
    if((dir = opendir(Path::Player.c_str())) == nullptr) {
        std::clog << "Directory could not be opened.\n";
        return;
    }
    std::clog << "done.\n";

    std::clog << "Reading player directory...";
    while((dirp = readdir(dir)) != nullptr) {
        // is this a player file?
        if(dirp->d_name[0] == '.')
            continue;
        if(!isupper(dirp->d_name[0]))
            continue;

        // is this a readable player file?
        auto filename = Path::Player / dirp->d_name;
        if((xmlDoc = xml::loadFile(filename.c_str(), "Player")) == nullptr)
            continue;

        // get the information we need
        rootNode = xmlDocGetRootElement(xmlDoc);
        curNode = rootNode->children;


        // reset some variables
        spells.reset();
        cgold = 0;
        deity = 0;
        birthday = std::make_shared<cDay>();
        statistics.reset();
        cClass = cClass2 = static_cast<int>(CreatureClass::NONE);

        auto name = xml::getProp(rootNode, "Name");
        
        // not for these characters
        if(name == "Johny" || name == "Min" || name == "Bob") {
            xmlFreeDoc(xmlDoc);
            xmlCleanupParser();
            continue;
        }

        // load the information we need
        while(curNode) {
            if(NODE_NAME(curNode, "Experience")) xml::copyToNum(exp, curNode);
            else if(NODE_NAME(curNode, "Level")) {
                xml::copyToNum(level, curNode);
                if(level < MINIMUM_LEVEL)
                    break;
            }
            else if(NODE_NAME(curNode, "Class")) xml::copyToNum(cClass, curNode);
            else if(NODE_NAME(curNode, "Class2")) xml::copyToNum(cClass2, curNode);
            else if(NODE_NAME(curNode, "Race")) xml::copyToNum(race, curNode);

            // ok, deity is spelled wrong in the xml files, so try both
            else if(NODE_NAME(curNode, "Deity")) xml::copyToNum(deity, curNode);

            else if(NODE_NAME(curNode, "LastTimes")) {
                loadLastTimes(curNode, lasttime);
            } else if(NODE_NAME(curNode, "Birthday")) {
                birthday->load(curNode);
            } else if(NODE_NAME(curNode, "Bank") || NODE_NAME(curNode, "Coins")) {
                xml::loadNumArray<long>(curNode, money, "Coin", 5);
                cgold += money[2];
                for(i=0; i<5; i++)
                    money[i] = 0;
            } 
            else if(NODE_NAME(curNode, "Statistics")) statistics.load(curNode);
            else if(NODE_NAME(curNode, "Spells")) loadBitset(curNode, spells);
            else if(NODE_NAME(curNode, "RoomExp")) {
                childNode = curNode->children;
                while(childNode) {
                    if(NODE_NAME(childNode, "Room")) numRooms++;
                    childNode = childNode->next;
                }
            }

            curNode = curNode->next;
        }

        // don't do staff!
        if(cClass >= static_cast<int>(STAFF) || level < MINIMUM_LEVEL) {
            xmlFreeDoc(xmlDoc);
            xmlCleanupParser();
            continue;
        }

        auto classStr = getFullClassName(static_cast<CreatureClass>(cClass), static_cast<CreatureClass>(cClass2));
        if (static_cast<CreatureClass>(cClass2) == CreatureClass::NONE) {
            classList.insert(classStr);
        } else {
            multiClassList.insert(classStr);
        }
        highest[classStr].count++;
        if(exp > highest[classStr].amount) {
            highest[classStr].amount = exp;
            highest[classStr].playerName = name;
        }
        if(exp > highest["player"].amount) {
            highest["player"].amount = exp;
            highest["player"].playerName = name;
        }
        // highest in their race?
        auto raceStr = gConfig->getRace(race)->getName();
        raceList.insert(raceStr);
        highest[raceStr].count++;
        if(exp > highest[raceStr].amount) {
            highest[raceStr].amount = exp;
            highest[raceStr].playerName = name;
        }
        // highest in their deity?
        if(deity) {
            auto deityStr = gConfig->getDeity(deity)->getName();
            deityList.insert(deityStr);
            if(exp > highest[deityStr].amount) {
                highest[deityStr].amount = exp;
                highest[deityStr].playerName = name;
            }
            highest[deityStr].count++;
            deityCount++;
        }

        numSpells = static_cast<long>(spells.count());
        if(numSpells > highest["spells"].amount) {
            highest["spells"].amount = numSpells;
            highest["spells"].playerName = name;
        }


        if(numRooms > highest["rooms"].amount) {
            highest["rooms"].amount = numRooms;
            highest["rooms"].playerName = name;
        }

        totalCount++;


        // longest played?
        if(lasttime[LT_AGE].interval > highest["longest"].amount) {
            highest["longest"].amount = lasttime[LT_AGE].interval;
            highest["longest"].playerName = name;
        }

        // character age?
        if(birthday) {
            age = calendar->getCurYear() - birthday->getYear();
            if(calendar->getCurMonth() < birthday->getMonth())
                age--;
            if(calendar->getCurMonth() == birthday->getMonth() && calendar->getCurDay() < birthday->getDay())
                age--;
            if(age > highest["age"].amount) {
                highest["age"].amount = age;
                highest["age"].playerName = name;
            }
        }

        // richest character?
        if(cgold > highest["richest"].amount) {
            highest["richest"].amount = cgold;
            highest["richest"].playerName = name;
        }

        // best non-100% pk
        if(statistics.pkDemographics() > highest["pkill"].amount) {
            highest["pkill"].amount = static_cast<long>(statistics.pkDemographics());
            highest["pkill"].playerName = name;
        }
        birthday.reset();
        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();
    }
    std::clog << "done.\n";
    closedir(dir);

    xmlCleanupParser();

    std::clog << "Formatting stone scrolls...";

    // load the file we will be printing to
    fs::path scroll = Path::Sign / "stone_scroll.txt";
    fs::remove(scroll);
    std::ofstream scrollOut(scroll.string());


    scrollOut << "   _______________________________________\n";
    scrollOut << "  /\\                                      \\\n";
    scrollOut << " /  | Only characters who have reached     |\n";
    scrollOut << " \\  | at least level 3 are counted for the |\n";
    scrollOut << "  \\_| stone scroll.                        |\n";
    scrollOut << "    |                                      |\n";
    scrollOut << fmt::format("    |   Highest Player : {:<17s} |\n", highest["player"].playerName);
    scrollOut << fmt::format("    |   Richest Player : {:<17s} |\n", highest["richest"].playerName);
    //out << fmt::format("    |   Oldest Player  : {:-17s} |\n", highest["age].playerName);
    scrollOut << fmt::format("    |   Longest Played : {:<17s} |\n", highest["longest"].playerName);
    scrollOut << fmt::format("    |   Best Pkiller   : {:<17s} |\n", highest["pkill"].playerName);
    scrollOut << fmt::format("    |   Most Spells    : {:<17s} |\n", highest["spells"].playerName);
    scrollOut << fmt::format("    |   Explorer       : {:<17s} |\n", highest["rooms"].playerName);
    scrollOut << "    |                                      |\n";
    scrollOut << "    | To view a specific category, type    |\n";
    scrollOut << "    | \"look stone <category>\". The         |\n";
    scrollOut << "    | categories are:                      |\n";
    scrollOut << "    |         class                        |\n";
    scrollOut << "    |         race                         |\n";
    scrollOut << "    |         deity                        |\n";
    scrollOut << "    |         breakdown                    |\n";
    scrollOut << "    |                                      |\n";
    scrollOut << "    | Last Updated:                        |\n";

    // let them know when we ran this program
    std::string txt;
    txt += ctime(&t);
    boost::trim(txt);
    txt += " (" + Server::getTimeZone() + ").";
    scrollOut << fmt::format("    | {:36s} |\n", txt);


    printEnding(scrollOut);


    fs::path classScroll = Path::Sign / "stone_scroll_class.txt";
    fs::remove(classScroll);
    std::ofstream classScrollOut(classScroll.string());

    classScrollOut << "   _______________________________________\n";
    classScrollOut << "  /\\                                      \\\n";
    classScrollOut << " /  | Highest Player Based on Class        |\n";
    classScrollOut << " \\  |                                      |\n";
    classScrollOut << "  \\_|                                      |\n";

    for(const auto& cStr : classList)
        classScrollOut << fmt::format("    | {:15s} : {:<18s} |\n", cStr, highest[cStr].playerName);

    classScrollOut << "    |                                      |\n";

    for(const auto& cStr : multiClassList)
        classScrollOut << fmt::format("    | {:15s} : {:<18s} |\n", cStr, highest[cStr].playerName);

    printEnding(classScrollOut);


    fs::path raceScroll = Path::Sign / "stone_scroll_race.txt";
    fs::remove(raceScroll);
    std::ofstream raceScrollOut(raceScroll.string());

    raceScrollOut << "   _______________________________________\n";
    raceScrollOut << "  /\\                                      \\\n";
    raceScrollOut << " /  | Highest Player Based on Race         |\n";
    raceScrollOut << " \\  |                                      |\n";
    raceScrollOut << "  \\_|                                      |\n";

    for(const auto& rStr : raceList)
        raceScrollOut << fmt::format("    | {:13s} : {:<20s} |\n", rStr, highest[rStr].playerName);

    printEnding(raceScrollOut);

    fs::path deityScroll = Path::Sign / "stone_scroll_deity.txt";
    fs::remove(deityScroll);
    std::ofstream deityScrollOut(deityScroll.string());


    deityScrollOut << "   _______________________________________\n";
    deityScrollOut << "  /\\                                      \\\n";
    deityScrollOut << " /  | Highest Player Based on Deity        |\n";
    deityScrollOut << " \\  |                                      |\n";
    deityScrollOut << "  \\_|                                      |\n";

    for(const auto& dStr : deityList)
        deityScrollOut << fmt::format("    | {:13s} : {:<20s} |\n", dStr, highest[dStr].playerName);

    printEnding(deityScrollOut);

    fs::path breakdownScroll = Path::Sign / "stone_scroll_breakdown.txt";
    fs::remove(breakdownScroll);
    std::ofstream breakdownScrollOut(breakdownScroll.string());


    breakdownScrollOut << "   ________________________________________________\n";
    breakdownScrollOut << "  /\\                                               \\\n";
    breakdownScrollOut << " /  | Player Breakdown                              |\n";
    breakdownScrollOut << " \\  |                                               |\n";
    breakdownScrollOut << "  \\_|                                               |\n";

    std::vector<std::string> leftCol;
    std::vector<std::string> rightCol;

    for(const auto& cStr : classList) {
        leftCol.emplace_back(
            fmt::format("{:15s} : {:>4.1f}%", cStr, ((highest[cStr].count * 100.0) / totalCount))
        );
    }
    leftCol.emplace_back("");

    for(const auto& cStr : multiClassList) {
        leftCol.emplace_back(
            fmt::format("{:15s} : {:>4.1f}%", cStr, ((highest[cStr].count * 100.0 )/ totalCount))
        );
    }
    leftCol.emplace_back("");

    for(const auto& dStr : deityList) {
        leftCol.emplace_back(
            fmt::format("{:15s} : {:>4.1f}%", dStr, ((highest[dStr].count * 100.0) / deityCount))
        );
    }
    // make a space for multiclass to come

    for(const auto& rStr : raceList) {
        rightCol.emplace_back(
            fmt::format("{:>14s} : {:>4.1f}%", rStr, ((highest[rStr].count * 100.0) / totalCount))
        );
    }

    auto numRows = std::max(leftCol.size(), rightCol.size());
    for(auto row = 0 ; row < numRows ; row++) {
        breakdownScrollOut << fmt::format("    | {:23s}{:22s} |", (row < leftCol.size() ? leftCol[row] : ""), (row < rightCol.size() ? rightCol[row] : "")) << std::endl;
    }

    breakdownScrollOut << "    |                                               |\n";
    breakdownScrollOut << "    | deity breakdown is limited to classes         |\n";
    breakdownScrollOut << "    | pledged to a god.                             |\n";
    breakdownScrollOut << "    |                                               |\n";
    breakdownScrollOut << "    |    ___________________________________________|_\n";
    breakdownScrollOut << "    \\   /                                            /\n";
    breakdownScrollOut << "     \\_/____________________________________________/\n\n";

    breakdownScrollOut.close();

    std::clog << "done.\nDone.\n";
#endif
}

void runDemographics() {
#ifndef NODEMOGRAPHICS
    if(!fork()) {
        doDemographics();
        exit(0);
    }
#endif
}

int cmdDemographics(const std::shared_ptr<Player>& player, cmd* cmnd) {
    player->print("Beginning demographics routine.\nCheck for results in a few seconds.\n");
    runDemographics();
    return(0);
}
