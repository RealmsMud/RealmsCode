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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>

#include "creatures.hpp"
#include "common.hpp"
#include "join.hpp"
#include "proto.hpp"
#include "objects.hpp"
#include "rooms.hpp"
#include "xml.hpp"


namespace fs = boost::filesystem;

#include "config.hpp"
#include "server.hpp"

extern Config *gConfig;
extern Server *gServer;


int list_rooms() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;

    const char* room_path = "/home/realms/realms/rooms/";
    std::vector<fs::path> areas;
    fs::directory_iterator areas_end, areas_start(room_path);
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


    for(fs::path area : areas) {
        if (fs::is_directory(area)) {
            std::vector<fs::path> rooms;
            fs::directory_iterator rooms_end, rooms_start(area);
            std::copy(rooms_start, rooms_end, std::back_inserter(rooms));
            std::sort(rooms.begin(), rooms.end());
            for (fs::path room : rooms) {
                if (fs::is_regular_file(room)) {
                    UniqueRoom *lRoom = new UniqueRoom();
                    const char *filename = room.string().c_str();
                    if((xmlDoc = xml::loadFile(filename, "Room")) == nullptr) {
                        std::cout << "Error loading: " << filename << "\n";
                        continue;
                    }
                    rootNode = xmlDocGetRootElement(xmlDoc);
                    lRoom->readFromXml(rootNode, true);

                    std::cout << lRoom->info.rstr() << ","
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

    const char* objects_path = "/home/realms/realms/objects/";
    std::vector<fs::path> areas;
    fs::directory_iterator areas_end, areas_start(objects_path);
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


    for(fs::path area : areas) {
        if (fs::is_directory(area)) {
            std::vector<fs::path> objects;
            fs::directory_iterator rooms_end, rooms_start(area);
            std::copy(rooms_start, rooms_end, std::back_inserter(objects));
            std::sort(objects.begin(), objects.end());
            for (fs::path object : objects) {
                if (fs::is_regular_file(object)) {
                    Object *lObject = new Object();
                    const char *filename = object.string().c_str();
                    if((xmlDoc = xml::loadFile(filename, "Object")) == nullptr) {
                        std::cout << "Error loading: " << filename << "\n";
                        continue;
                    }
                    rootNode = xmlDocGetRootElement(xmlDoc);
                    lObject->readFromXml(rootNode, 0, true);
                    bstring description = lObject->description;
                    description.Replace("\n", "\\n");
                    description.Replace("\"", "\"\"");


                    std::cout << lObject->info.rstr() << ","
                              << lObject->lastMod << ","
                              << "\"" << boost::replace_all_copy(lObject->getName(), "\n", "\\n") << "\"" << ","
                              << "\"" << description << "\"" << ","
                              << lObject->getTypeStr() << ","
                              << lObject->getSubType() << ","
                              << lObject->getKey() << ","
                              << lObject->getWearName() << ","

                              << lObject->getAdjustment() << ","
                              << lObject->getQuality() << ","
                              << lObject->getLevel() << ","
                              << lObject->getRequiredSkill() << ","
                              << lObject->getNumAttacks() << ","
                              << (lObject->getDelay()/10.0) << ","

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

    const char* monster_path = "/home/realms/realms/monsters/";
    std::vector<fs::path> areas;
    fs::directory_iterator areas_end, areas_start(monster_path);
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


    for(fs::path area : areas) {
        if (fs::is_directory(area)) {
            std::vector<fs::path> monsters;
            fs::directory_iterator rooms_end, rooms_start(area);
            std::copy(rooms_start, rooms_end, std::back_inserter(monsters));
            std::sort(monsters.begin(), monsters.end());
            for (fs::path monster : monsters) {
                if (fs::is_regular_file(monster)) {
                    Monster *lMonster = new Monster();
                    const char *filename = monster.string().c_str();
                    if((xmlDoc = xml::loadFile(filename, "Creature")) == nullptr) {
                        std::cout << "Error loading: " << filename << "\n";
                        continue;
                    }
                    rootNode = xmlDocGetRootElement(xmlDoc);
                    lMonster->readFromXml(rootNode, true);

                    std::cout << lMonster->info.rstr() << ","
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
        case 'a':
            list_objects();
            list_monsters();
            list_rooms();
            break;
        default:
            break;
    }
    return 1;
}
