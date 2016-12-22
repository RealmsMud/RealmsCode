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
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <boost/filesystem.hpp>
#include <iostream>
#include "common.h"
#include "rooms.h"
#include "xml.h"

namespace fs = boost::filesystem;

#include "config.h"
#include "server.h"

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

    for(fs::path area : areas) {
        if (fs::is_directory(area)) {
            std::cout << "**********" << area.filename() << "**********" << "\n";

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
                    std::cout << lRoom->info.rstr() << " " << lRoom->getCName() << "\n";
                }
            }
        }
    }

    return 1;
}


int list_objects() {
    return 1;
}


int list_monsters() {
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
    list_rooms();
    return 1;
}
