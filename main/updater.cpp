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
#include <chrono>                              // for index-build pump budget
#include <filesystem>                          // for per-zone migration output
#include <fstream>
#include <deque>                               // for _Deque_iterator
#include <iomanip>                             // for setw
#include <iostream>                            // for operator<<, basic_ostream
#include <iterator>                            // for back_insert_iterator
#include <list>                                // for operator==
#include <map>                                 // for map, operator==
#include <set>                                 // for set
#include <string>                              // for string, operator<<
#include <vector>                              // for vector

#include "catRef.hpp"                          // for CatRef
#include "config.hpp"                          // for Config, gConfig
#include "csv.hpp"                             // for util::parseCsv
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
#include "quests.hpp"                          // for buildQuestRemap
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

// one-shot: write XML-loaded quests as per-zone JSON.
static void migrateZonesToJson(bool force) {
    namespace fs = std::filesystem;
    fs::path zoneRoot = Path::Zone;
    std::set<std::string> zoneAreas(zones.begin(), zones.end());

    int qWrote = 0, qSkipped = 0;
    for(const auto& [cr, quest] : gConfig->quests) {
        if(!quest)
            continue;
        zoneAreas.insert(cr.area);
        fs::path qdir = zoneRoot / cr.area / "quests";
        fs::create_directories(qdir);
        fs::path qfile = qdir / (std::to_string(cr.id) + ".json");
        if(fs::exists(qfile) && !force) {   // preserve hand-edited quest JSON
            qSkipped++;
            continue;
        }
        json j = *quest;
        std::ofstream out(qfile);
        out << std::setw(2) << j << std::endl;
        qWrote++;
    }

    int zWrote = 0, zSkipped = 0;
    for(const std::string& z : zoneAreas) {
        fs::path zfile = zoneRoot / z / "zone.json";
        fs::create_directories(zfile.parent_path());
        if(fs::exists(zfile) && !force) {   // don't clobber hand-edited metadata
            zSkipped++;
            continue;
        }
        json zj = {{"name", z}, {"display", z}};
        std::ofstream out(zfile);
        out << std::setw(2) << zj << std::endl;
        zWrote++;
    }

    std::cout << "Quests: wrote " << qWrote << ", skipped " << qSkipped
              << "; zones: wrote " << zWrote << ", skipped " << zSkipped
              << (force ? " (force)" : "") << " -> " << zoneRoot << std::endl;
}

// one-shot: (re)build all per-zone entity indices on disk.
static void buildAllIndices(bool force) {
    namespace fs = std::filesystem;
    fs::path zoneRoot = Path::Zone;
    std::error_code ec;
    if(!fs::is_directory(zoneRoot, ec))
        return;

    const ZoneIndex::Type types[] = { ZoneIndex::Type::Room, ZoneIndex::Type::Object,
                                      ZoneIndex::Type::Monster, ZoneIndex::Type::Quest };
    int zoneCount = 0;
    for(const auto& entry : fs::directory_iterator(zoneRoot)) {
        if(!entry.is_directory())
            continue;
        std::string zone = entry.path().filename().string();
        for(ZoneIndex::Type t : types) {
            if(force)
                fs::remove(entry.path() / (std::string(ZoneIndex::typeName(t)) + ".index.json"), ec);
            gServer->zoneIndexBuilder.request(t, zone);
        }
        zoneCount++;
    }
    while(gServer->zoneIndexBuilder.hasWork())
        gServer->zoneIndexBuilder.pump(std::chrono::microseconds(5'000'000), 100000);
    gServer->zoneIndex.flushDirty();

    std::cout << "Built indices for " << zoneCount << " zones" << (force ? " (force)" : "") << std::endl;
}

// one-shot: relocate + renumber the JSON quest store
static void remapQuestsToZones(bool force) {
    namespace fs = std::filesystem;
    fs::path zoneRoot = Path::Zone;
    fs::path mapFile  = zoneRoot / "quest-remap.json";
    fs::path csvFile  = zoneRoot / "quest-mapping.csv";
    std::error_code ec;

    if(fs::exists(mapFile) && !force) {
        std::cout << "Refusing: " << mapFile << " exists (already remapped); use --force.\n";
        return;
    }
    if(!fs::exists(csvFile)) {
        std::cout << "Missing mapping CSV: " << csvFile << "\n";
        return;
    }

    // quest id -> target zone from the CSV (id,name,turnInArea); the header row's
    // non-numeric id is skipped by the stoi failure
    std::ifstream csv(csvFile);
    std::map<int, std::string> idToZone;
    for(const auto& row : util::parseCsv(csv)) {
        if(row.size() < 3) continue;
        try { idToZone[std::stoi(row[0])] = row[2]; } catch(...) {}
    }

    // load every quest doc from the misc store (content held in memory so in-zone renumber
    // collisions are safe)
    fs::path miscQuests = zoneRoot / "misc" / "quests";
    std::vector<std::pair<CatRef, std::string>> questZones;
    std::map<CatRef, json> docs;
    if(fs::is_directory(miscQuests, ec)) {
        for(const auto& e : fs::directory_iterator(miscQuests)) {
            if(e.path().extension() != ".json") continue;
            std::ifstream ifs(e.path());
            json j;
            try { j = json::parse(ifs); } catch(...) { continue; }
            CatRef cur = j.at("id").get<CatRef>();
            auto it = idToZone.find(cur.id);
            if(it == idToZone.end()) {
                std::cout << "No mapping for quest " << cur.id << "; leaving in misc\n";
                continue;
            }
            questZones.emplace_back(cur, it->second);
            docs[cur] = std::move(j);
        }
    }

    std::map<CatRef, CatRef> remap = buildQuestRemap(questZones);

    // write all new files first, then delete old files not reused as a new path
    std::set<fs::path> newPaths;
    json mapping = json::object();
    int moved = 0, prereqFixed = 0, prereqMissed = 0;
    for(const auto& [oldCr, zone] : questZones) {
        const CatRef& nw = remap.at(oldCr);
        json j = docs.at(oldCr);
        j["id"] = nw;
        if(j.contains("preRequisites"))
            for(auto& pre : j["preRequisites"]) {
                auto pit = remap.find(pre.get<CatRef>());
                if(pit != remap.end()) { pre = pit->second; prereqFixed++; }
                else { prereqMissed++; }
            }
        fs::path outDir  = zoneRoot / nw.area / "quests";
        fs::create_directories(outDir);
        fs::path newPath = outDir / (std::to_string(nw.id) + ".json");
        std::ofstream out(newPath);
        out << std::setw(2) << j << std::endl;
        newPaths.insert(fs::weakly_canonical(newPath, ec));
        mapping[oldCr.area + "." + std::to_string(oldCr.id)] =
            json{{"area", nw.area}, {"id", nw.id}, {"name", j.value("name", "")}};
        moved++;
    }

    for(const auto& [oldCr, zone] : questZones) {
        fs::path oldPath = miscQuests / (std::to_string(oldCr.id) + ".json");
        if(!newPaths.count(fs::weakly_canonical(oldPath, ec)))
            fs::remove(oldPath, ec);
    }

    std::ofstream mout(mapFile);
    mout << std::setw(2) << mapping << std::endl;

    std::cout << "Remapped " << moved << " quests; prereqs rewritten " << prereqFixed
              << ", unmapped " << prereqMissed << "; mapping -> " << mapFile << "\n";
}

int main(int argc, char *argv[]) {
    bool force = false, remapQuests = false, buildIndices = false;
    for(int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if(a == "--force") force = true;
        else if(a == "--remap-quests") remapQuests = true;
        else if(a == "--build-indices") buildIndices = true;
    }

    gConfig = Config::getInstance();
    gServer = Server::getInstance();

    gConfig->setListing(true);
    gServer->init();

    if(remapQuests) {
        std::cout << "Remapping quests into per-zone dirs" << (force ? " (force)" : "") << std::endl;
        remapQuestsToZones(force);
        return 0;
    }

    // rebuild indices from the current on-disk JSON layout, skipping the xml->json migration
    // (which would reload quests.xml and re-introduce the flat misc ids)
    if(buildIndices) {
        std::cout << "Rebuilding per-zone indices" << (force ? " (force)" : "") << std::endl;
        buildAllIndices(force);
        return 0;
    }

    // Update quests first, so we can update the quests on the monsters
    std::cout << "Updating Quests" << std::endl;
    update_quests();

    std::cout << "Migrating quests/zones to per-zone JSON" << (force ? " (force)" : "") << std::endl;
    migrateZonesToJson(force);

    std::cout << "Building per-zone indices" << std::endl;
    buildAllIndices(force);

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
