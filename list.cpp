//
// Created by jason on 12/10/16.
//

#include <boost/filesystem.hpp>
#include <iostream>
#include "common.h"
#include "rooms.h"
#include "xml.h"

namespace fs = boost::filesystem;

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

                    if((xmlDoc = xml::loadFile(filename.c_str(), "Room")) == nullptr) {
                        std::cout << "Error loading: " << room << "\n";
                        continue;
                    }
                    rootNode = xmlDocGetRootElement(xmlDoc);
                    lRoom->readFromXml(rootNode);

                    std::cout << "Processed " << room << "\n";
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
    list_rooms();
    return 1;
}
