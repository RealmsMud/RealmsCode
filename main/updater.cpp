/*
 * Updater.cpp
 *   Updates stuff
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

#include <libxml/parser.h>                     // for xmlDocGetRootElement
#include <algorithm>                           // for copy, sort
#include <boost/algorithm/string/replace.hpp>  // for replace_all, replace_a...
#include <boost/iterator/iterator_facade.hpp>  // for operator!=
#include <boost/iterator/iterator_traits.hpp>  // for iterator_value<>::type
#include <fstream>
#include <deque>                               // for _Deque_iterator
#include <iostream>                            // for operator<<, basic_ostream
#include <iterator>                            // for back_insert_iterator
#include <list>                                // for operator==
#include <map>                                 // for map, operator==
#include <string>                              // for string, operator<<
#include <vector>                              // for vector

#include "catRef.hpp"                          // for CatRef
#include "config.hpp"                          // for Config, gConfig
#include "dice.hpp"                            // for Dice
#include "effects.hpp"                         // for EffectList, operator<<
#include "join.hpp"                            // for join, mjoin
#include "json.hpp"                            // for json, operator<<, opera...
#include "lasttime.hpp"                        // for operator<<, CRLastTime
#include "money.hpp"                           // for GOLD, Money
#include "mudObjects/monsters.hpp"             // for Monster
#include "mudObjects/objects.hpp"              // for Object
#include "mudObjects/uniqueRooms.hpp"          // for UniqueRoom
#include "objIncrease.hpp"                     // for ObjIncrease
#include "paths.hpp"
#include "server.hpp"                          // for Server, gServer
#include "statistics.hpp"                      // for Statistics
#include "stats.hpp"                           // for Stat
#include "wanderInfo.hpp"                      // for WanderInfo
#include "xml.hpp"                             // for loadFile
#include "zone.hpp"


using json = nlohmann::json;

int update_rooms() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;

    const fs::path room_path = Path::UniqueRoom;
    std::vector<fs::path> areas;
    fs::directory_iterator areas_end, areas_start(room_path);
    std::copy(areas_start, areas_end, std::back_inserter(areas));
    std::sort(areas.begin(), areas.end());

    for(const fs::path& area : areas) {
        if (fs::is_directory(area)) {
            std::cout << "Updating rooms in area: " << area << std::endl;

            std::vector<fs::path> rooms;
            fs::directory_iterator rooms_end, rooms_start(area);
            std::copy(rooms_start, rooms_end, std::back_inserter(rooms));
            std::sort(rooms.begin(), rooms.end());
            for (const fs::path& room : rooms) {
                if (fs::is_regular_file(room)) {
                    auto *lRoom = new UniqueRoom();
                    if((xmlDoc = xml::loadFile(room.c_str(), "Room")) == nullptr) {
                        std::cout << "Error loading: " << room.string() << "\n";
                        continue;
                    }
                    rootNode = xmlDocGetRootElement(xmlDoc);
                    lRoom->readFromXml(rootNode, true);
                    std::cout << "Updating room: " << lRoom->info.str() << std::endl;
                    if(lRoom->saveToFile(1))
                        std::cout << "***** Update Failed " << lRoom->info.str() << std::endl;

                }
            }
        }
    }

    return 1;
}


int update_objects() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;

    const fs::path objects_path = Path::Object;
    std::vector<fs::path> areas;
    fs::directory_iterator areas_end, areas_start(objects_path);
    std::copy(areas_start, areas_end, std::back_inserter(areas));
    std::sort(areas.begin(), areas.end());


    for(const fs::path& area : areas) {
        if (fs::is_directory(area)) {
            std::cout << "Updating objects in area: " << area << std::endl;

            std::vector<fs::path> objects;
            fs::directory_iterator rooms_end, rooms_start(area);
            std::copy(rooms_start, rooms_end, std::back_inserter(objects));
            std::sort(objects.begin(), objects.end());
            for (const fs::path& object : objects) {
                if (fs::is_regular_file(object)) {
                    auto *lObject = new Object();
                    if((xmlDoc = xml::loadFile(object.c_str(), "Object")) == nullptr) {
                        std::cout << "Error loading: " << object.string() << "\n";
                        continue;
                    }
                    rootNode = xmlDocGetRootElement(xmlDoc);
                    lObject->readFromXml(rootNode, nullptr, true);
                    std::cout << "Updating object: " << lObject->info.str() << std::endl;
                    if(lObject->saveToFile())
                        std::cout << "***** Update Failed " << lObject->info.str() << std::endl;

                }
            }
        }
    }

    return 1;
}


int update_monsters() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;

    const fs::path monster_path = Path::Monster;
    std::vector<fs::path> areas;
    fs::directory_iterator areas_end, areas_start(monster_path);
    std::copy(areas_start, areas_end, std::back_inserter(areas));
    std::sort(areas.begin(), areas.end());

    for(const fs::path& area : areas) {
        if (fs::is_directory(area)) {
            std::cout << "Updating monsters in area: " << area << std::endl;

            std::vector<fs::path> monsters;
            fs::directory_iterator rooms_end, rooms_start(area);
            std::copy(rooms_start, rooms_end, std::back_inserter(monsters));
            std::sort(monsters.begin(), monsters.end());
            for (const fs::path& monster : monsters) {
                if (fs::is_regular_file(monster)) {
                    auto *lMonster = new Monster();
                    if((xmlDoc = xml::loadFile(monster.c_str(), "Creature")) == nullptr) {
                        std::cout << "Error loading: " << monster.string() << "\n";
                        continue;
                    }
                    rootNode = xmlDocGetRootElement(xmlDoc);
                    lMonster->readFromXml(rootNode, true);
                    std::cout << "Updating monster: " << lMonster->info.str() << std::endl;

                    // TODO: updateMonsterQuests()

                    if(lMonster->saveToFile())
                        std::cout << "***** Update Failed " << lMonster->info.str() << std::endl;

                }
            }
        }
    }

    return 1;
}

int update_quests() {
    json allQuests = json();
    json questOverview = json();
    for(const auto& [questId, quest] : gConfig->quests) {
        allQuests.push_back(*quest);
        json qSummary = json();
        qSummary["id"] = questId.id;
        qSummary["name"] = quest->getName();
        qSummary["turnInArea"] = quest->getTurnInMob().area;
        questOverview.push_back(qSummary);
    }

    std::ofstream questFile("quests.json");
    questFile << std::setw(2) << allQuests << std::endl;

    std::ofstream summaryFile("questSummary.json");
    summaryFile << std::setw(2) << questOverview << std::endl;


    return 1;
}

std::list<std::string> zones = {"airship", "alc", "anhoni", "avenger", "azure", "baladus", "bane", "bergen", "bw", "caladon", "cgiant", "craft", "crescent",
    "dcarnival", "dhold", "dloch", "drakken", "druid", "durgas", "eldinwood", "elec", "et", "events", "fdrake", "gb", "gedge", "gemstone", "ghost", "gren",
    "guild", "hampton", "hellbog", "hp", "iclad", "ironguard", "jail", "joy", "kael", "kat", "kbtung", "kenku", "kenner", "kesh", "mino", "misc", "morgtala",
    "nexus", "niamei", "nikola", "oce", "ocean", "orym", "pirate", "plane", "quest", "seawolf", "shadow", "shop", "sigil", "srunner", "stor", "trade", "tut",
    "voodan", "wave", "wizard", "wolf", "wville", "yuanti"};

int main(int argc, char *argv[]) {
    gConfig = Config::getInstance();
    gServer = Server::getInstance();

    gConfig->setListing(true);
    gServer->init();

    // Update quests first, so we can update the quests on the monsters
    std::cout << "Updating Quests" << std::endl;
    update_quests();

    std::string str(R"(["test", "test2"])");
    json js = json::parse(str);
    std::cout << js << std::endl;
    std::list<std::string> testList = js;

    for (const auto& s : testList)
        std::cout << s << std::endl;

    // MapQuests()
    // SaveQuests()
//
//    std::cout << "Updating Monsters" << std::endl;
//    update_monsters();
//
//    std::cout << "Updating Objects" << std::endl;
//    update_objects();
//
//    std::cout << "Updating Rooms" << std::endl;
//    update_rooms();


    return 1;
}
