/*
 * list.cpp
 *   Class for listing thing
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
#include "proto.hpp"



int list_rooms() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;

    std::vector<fs::path> areas;
    fs::directory_iterator areas_end, areas_start(Path::UniqueRoom);
    std::copy(areas_start, areas_end, std::back_inserter(areas));
    std::sort(areas.begin(), areas.end());

    std::cout << "Room" << ","
              << "Name" << ","
              << "CntPermCrt" << ","
              << "PermCrt" << ","
              << "CntPermObj" << ","
              << "PermObj" << ","
              << "Traffic" << ","
              << "RandomCrt" << std::endl;


    for(const fs::path& area : areas) {
        if (fs::is_directory(area)) {
            std::vector<fs::path> rooms;
            fs::directory_iterator rooms_end, rooms_start(area);
            std::copy(rooms_start, rooms_end, std::back_inserter(rooms));
            std::sort(rooms.begin(), rooms.end());
            for (const fs::path& room : rooms) {
                if (fs::is_regular_file(room)) {
                    auto lRoom = std::make_shared<UniqueRoom>();
                    if((xmlDoc = xml::loadFile(room.c_str(), "Room")) == nullptr) {
                        std::cout << "Error loading: " << room.string() << "\n";
                        continue;
                    }
                    rootNode = xmlDocGetRootElement(xmlDoc);
                    lRoom->readFromXml(rootNode, true);
                    xmlFreeDoc(xmlDoc);

                    std::cout << lRoom->info.str() << ","
                              << "\"" << lRoom->getName() << "\"" << ","
                              << lRoom->permMonsters.size() << ","
                              << mjoin(lRoom->permMonsters, "|") << ","
                              << lRoom->permObjects.size() << ","
                              << mjoin(lRoom->permObjects, "|") << ","
                              << lRoom->wander.getTraffic() << ","
                              << lRoom->wander.getRandomCount() << std::endl;
                }
            }
        }
    }

    return 1;
}


int list_objects() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;

    std::vector<fs::path> areas;
    fs::directory_iterator areas_end, areas_start(Path::Object);
    std::copy(areas_start, areas_end, std::back_inserter(areas));
    std::sort(areas.begin(), areas.end());

    std::cout << "Object" << ","
              << "LastMod" << ","
              << "Name" << ","
              << "Description" << ","
              << "Type" << ","
              << "SubType" << ","
              << "Key" << ","
              << "WearName" << ","

              << "Adj" << ","
              << "Quality" << ","
              << "Lvl" << ","
              << "ReqSkill" << ","
              << "NumAttks" << ","
              << "Delay" << ","
              << "Armor" << ","
              << "ShotsCur" << ","
              << "ShotsMax" << ","

              << "DiceNum" << ","
              << "DiceSides" << ","
              << "DicePlus" << ","
              << "DLow" << ","
              << "DHigh" << ","
              << "DAvg" << ","

              << "Recipe" << ","
              << "MagicPower" << ","

              << "Size" << ","
              << "weight" << ","
              << "bulk" << ","
              << "maxbulk" << ","
              << "material" << ","

              << "increase" << ","
              << "Bestows" << ","
              << "Effects" << ","
              << "Flags" << ","
              << "" << std::endl;


    for(const fs::path& area : areas) {
        if (fs::is_directory(area)) {
            std::vector<fs::path> objects;
            fs::directory_iterator rooms_end, rooms_start(area);
            std::copy(rooms_start, rooms_end, std::back_inserter(objects));
            std::sort(objects.begin(), objects.end());
            for (const fs::path& object : objects) {
                if (fs::is_regular_file(object)) {
                    auto lObject = std::make_shared<Object>();
                    if((xmlDoc = xml::loadFile(object.c_str(), "Object")) == nullptr) {
                        std::cout << "Error loading: " << object.string() << "\n";
                        continue;
                    }
                    rootNode = xmlDocGetRootElement(xmlDoc);
                    lObject->readFromXml(rootNode, nullptr, true);
                    xmlFreeDoc(xmlDoc);

                    std::string description = lObject->description;
                    boost::replace_all(description, "\n", "\\n");
                    boost::replace_all(description, "\"", "\"\"");


                    std::cout << lObject->info.str() << ","
                              << lObject->lastMod << ","
                              << "\"" << boost::replace_all_copy(lObject->getName(), "\n", "\\n") << "\"" << ","
                              << "\"" << description << "\"" << ","
                              << lObject->getTypeName() << ","
                              << lObject->getSubType() << ","
                              << lObject->getKey() << ","
                              << lObject->getWearName() << ","

                              << lObject->getAdjustment() << ","
                              << lObject->getQuality() << ","
                              << lObject->getLevel() << ","
                              << lObject->getRequiredSkill() << ","
                              << lObject->getNumAttacks() << ","
                              << (lObject->getDelay()/10.0) << ","
                              << lObject->getArmor() << ","
                              << lObject->getShotsCur() << ","
                              << lObject->getShotsMax() << ","

                              << lObject->damage.getNumber() << ","
                              << lObject->damage.getSides() << ","
                              << lObject->damage.getPlus() << ","
                              << lObject->damage.low() << ","
                              << lObject->damage.high() << ","
                              << lObject->damage.average() << ","

                              << lObject->getRecipe() << ","
                              << lObject->getMagicpower() << ","

                              << lObject->getSizeStr() << ","
                              << lObject->getWeight() << ","
                              << lObject->getBulk() << ","
                              << lObject->getMaxbulk() << ","
                              << lObject->getMaterial() << ","

                              << (lObject->increase ? lObject->increase->increase : "") << ","
                              << lObject->getEffect() << ","
                              << join(lObject->effects.effectList, "|") << ","
                              << lObject->getFlagList("|") << ","
                              << std::endl;
                }
            }
        }
    }

    return 1;
}


int list_monsters() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;

    std::vector<fs::path> areas;
    fs::directory_iterator areas_end, areas_start(Path::Monster);
    std::copy(areas_start, areas_end, std::back_inserter(areas));
    std::sort(areas.begin(), areas.end());

    std::cout << "Monster" << ","
              << "Name" << ","
              << "Level" << ","
              << "Class" << ","
              << "Toughness" << ","
              << "Experience" << ","
              << "Gold" << ","

              << "HpMax" << ","
              << "MpMax" << ","
              << "Str" << ","
              << "Dex" << ","
              << "Con" << ","
              << "Int" << ","
              << "Pie" << ","

              << "Armor" << ","
              << "Defense" << ","
              << "WeaponSkill" << ","
              << "AttackPower" << ","
              << "DiceNum" << ","
              << "DiceSides" << ","
              << "DicePlus" << ","
              << "DLow" << ","
              << "DHigh" << ","
              << "DAvg" << ","
              << "Effects" << ","
              << "Flags" << ","
              << "" << std::endl;


    for(const fs::path& area : areas) {
        if (fs::is_directory(area)) {
            std::vector<fs::path> monsters;
            fs::directory_iterator rooms_end, rooms_start(area);
            std::copy(rooms_start, rooms_end, std::back_inserter(monsters));
            std::sort(monsters.begin(), monsters.end());
            for (const fs::path& monster : monsters) {
                if (fs::is_regular_file(monster)) {
                    auto lMonster = std::make_shared<Monster>();
                    if((xmlDoc = xml::loadFile(monster.c_str(), "Creature")) == nullptr) {
                        std::cout << "Error loading: " << monster.string() << "\n";
                        continue;
                    }
                    rootNode = xmlDocGetRootElement(xmlDoc);
                    lMonster->readFromXml(rootNode, true);
                    xmlFreeDoc(xmlDoc);

                    std::cout << lMonster->info.str() << ","
                              << "\"" << lMonster->getName() << "\"" << ","
                              << lMonster->getLevel() << ","
                              << lMonster->getClassString() << ","
                              << Statistics::calcToughness(lMonster) << ","

                              << lMonster->getExperience() << ","
                              << lMonster->coins[GOLD] << ","

                              << lMonster->hp.getMax() << ","
                              << lMonster->mp.getMax() << ","
                              << lMonster->strength.getMax() << ","
                              << lMonster->dexterity.getMax() << ","
                              << lMonster->constitution.getMax() << ","
                              << lMonster->intelligence.getMax() << ","
                              << lMonster->piety.getMax() << ","

                              << lMonster->getArmor() << ","
                              << lMonster->getDefenseSkill() << ","
                              << lMonster->getWeaponSkill() << ","
                              << lMonster->getAttackPower() << ","
                              << lMonster->damage.getNumber() << ","
                              << lMonster->damage.getSides() << ","
                              << lMonster->damage.getPlus() << ","
                              << lMonster->damage.low() << ","
                              << lMonster->damage.high() << ","
                              << lMonster->damage.average() << ","
                              << join(lMonster->effects.effectList, "|") << ","
                              << lMonster->getFlagList("|") << ","
                              << std::endl;
                }
            }
        }
    }

    return 1;
}


int list_players() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;

    std::vector<fs::path> player;
    fs::directory_iterator player_end, player_start(Path::Player);
    std::copy(player_start, player_end, std::back_inserter(player));
    std::sort(player.begin(), player.end());

    std::cout << "Name" << ","
              << "Level" << ","
              << "Race" << ","
              << "Class" << ","
              << "Deity" << ","
              << "Experience" << ","
              << "Gold" << ","
              << "Bank" << ","
              << "HpMax" << ","
              << "MpMax" << ","
              << "Str" << ","
              << "Dex" << ","
              << "Con" << ","
              << "Int" << ","
              << "Pie" << ","
             // << "Password" << ","
              << "Last Login" << ","
              << "" << std::endl;


    std::string pass, lastLoginString;
    long lastLogin=0;
    
    std::copy(player_start, player_end, std::back_inserter(player));
    std::sort(player.begin(), player.end());
    for (const fs::path& ply : player) {
        if (fs::is_regular_file(ply)) {
            auto lPlayer = std::make_shared<Player>();
            
            if((xmlDoc = xml::loadFile(ply.c_str(), "Player")) == nullptr) {
                std::cout << "Error loading: " << ply.string() << "\n";
                continue;
            }

            //ignore any .bak files in player path
            std::size_t found_bak = ply.string().find(".bak");
            if (found_bak!=std::string::npos) {
                continue;
            }

            rootNode = xmlDocGetRootElement(xmlDoc);
            lPlayer->readFromXml(rootNode, true);
            (lPlayer)->setName(xml::getProp(rootNode, "Name"));
            
          //  xml::copyPropToString(pass, rootNode, "Password");
          //  (lPlayer)->setPassword(pass);
            (lPlayer)->setLastLogin(xml::getIntProp(rootNode, "LastLogin"));
            lastLogin = lPlayer->getLastLogin();
            lastLoginString = ctime(&lastLogin);

            //ctime() adds an annoying \n, so we're going to strip it
            lastLoginString.erase(std::remove(lastLoginString.begin(), lastLoginString.end(), '\n'), lastLoginString.cend());

            xmlFreeDoc(xmlDoc);
           

            
            std::cout << "\"" << lPlayer->getName() << "\"" << ","
                      << lPlayer->getLevel() << ","
                      << lPlayer->getRace() << ","
                      << lPlayer->getClassString() << ","
                      << lPlayer->getDeity() << ","
                      << lPlayer->getExperience() << ","
                      << lPlayer->coins[GOLD] << ","
                      << lPlayer->bank[GOLD] << ","
                      << lPlayer->hp.getMax() << ","
                      << lPlayer->mp.getMax() << ","
                      << lPlayer->strength.getMax() << ","
                      << lPlayer->dexterity.getMax() << ","
                      << lPlayer->constitution.getMax() << ","
                      << lPlayer->intelligence.getMax() << ","
                      << lPlayer->piety.getMax() << ","
                    //  << "\"" << lPlayer->getPassword() << "\"" << ","
                      << "\"" << lastLoginString << "\"" << ","
                      << std::endl;
        }
    }

    return 1;
}
   


int main(int argc, char *argv[]) {
    gConfig = Config::getInstance();
    gServer = Server::getInstance();

    gConfig->setListing(true);
    gServer->init();

    switch(argv[1][0]) {
        case 'm':
            list_monsters();
            break;
        case 'o':
            list_objects();
            break;
        case 'r':
            list_rooms();
            break;
        case 'p':
            list_players();
            break;
        case 'a':
            list_objects();
            list_monsters();
            list_players();
            list_rooms();
            break;
        default:
            break;
    }
    return 1;
}
