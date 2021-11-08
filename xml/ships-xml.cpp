/*
 * ships-xml.c
 *   Moving exits and such - XML
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

#include "config.hpp"     // for Config, gConfig
#include "paths.hpp"      // for Paths
#include "proto.hpp"      // for file_exists
#include "ships.hpp"      // for ShipStop, ShipRaid
#include "xml.hpp"        // for copyToString

void Ship::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    int     at_stop=0, stop=0;

    while(childNode) {
        if(NODE_NAME(childNode, "Name")) xml::copyToString(name, childNode);
        else if(NODE_NAME(childNode, "Clan")) xml::copyToNum(clan, childNode);
        else if(NODE_NAME(childNode, "Transit")) xml::copyToString(transit, childNode);
        else if(NODE_NAME(childNode, "Movement")) xml::copyToString(movement, childNode);
        else if(NODE_NAME(childNode, "Docked")) xml::copyToString(docked, childNode);

        else if(NODE_NAME(childNode, "CanQuery")) xml::copyToBool(canQuery, childNode);
        else if(NODE_NAME(childNode, "InPort")) xml::copyToBool(inPort, childNode);
        else if(NODE_NAME(childNode, "TimeLeft")) xml::copyToNum(timeLeft, childNode);
            // at_stop is only used when loading - the cycling of the stops below
            // will put the stop pointed to by at_stop at the front. when it's saved,
            // it will load first
        else if(NODE_NAME(childNode, "AtStop")) xml::copyToNum(at_stop, childNode);

        else if(NODE_NAME(childNode, "BoatRange")) {
            xmlNodePtr subNode = childNode->children;

            while(subNode) {
                if(NODE_NAME(subNode, "Range")) {
                    Range r;
                    r.load(subNode);
                    ranges.push_back(r);
                }
                subNode = subNode->next;
            }
        } else if(NODE_NAME(childNode, "Stops"))
            loadStops(childNode);
        childNode = childNode->next;
    }

    // now move what stop we're at to the front of the list
    while(stop < at_stop) {
        stop++;
        stops.push_back(stops.front());
        stops.pop_front();
    }
}


void Ship::loadStops(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    ShipStop    *stop=nullptr;

    while(childNode) {
        if(NODE_NAME(childNode, "Stop")) {
            stop = new ShipStop;
            stop->load(childNode);
            stops.push_back(stop);
        }
        childNode = childNode->next;
    }
}


void Ship::save(xmlNodePtr rootNode) const {
    xmlNodePtr  curNode, childNode;

    curNode = xml::newStringChild(rootNode, "Ship");
    xml::saveNonNullString(curNode, "Name", name);
    xml::saveNonZeroNum(curNode, "Clan", clan);
    xml::saveNonZeroNum(curNode, "AtStop", 0);
    xml::saveNonZeroNum(curNode, "InPort", inPort);
    xml::saveNonZeroNum(curNode, "TimeLeft", timeLeft);

    xml::saveNonZeroNum(curNode, "CanQuery", canQuery);
    xml::saveNonNullString(curNode, "Transit", transit);
    xml::saveNonNullString(curNode, "Movement", movement);
    xml::saveNonNullString(curNode, "Docked", docked);

    if(!ranges.empty()) {
        childNode = xml::newStringChild(curNode, "BoatRange");
        std::list<Range>::const_iterator rt;
        for(rt = ranges.begin() ; rt != ranges.end() ; rt++)
            (*rt).save(childNode, "Range");
    }

    childNode = xml::newStringChild(curNode, "Stops");
    std::list<ShipStop*>::const_iterator st;
    for(st = stops.begin() ; st != stops.end() ; st++)
        (*st)->save(childNode);
}



//*********************************************************************
//                      saveShips
//*********************************************************************

void Config::saveShips() const {
    std::list<Ship*>::const_iterator it;
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char            filename[80];

    // we only save if we're up on the main port
    if(getPortNum() != 3333)
        return;

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Ships", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    for(it = ships.begin() ; it != ships.end() ; it++)
        (*it)->save(rootNode);

    sprintf(filename, "%s/ships.xml", Path::Game);
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
}


//*********************************************************************
//                      loadShips
//*********************************************************************

bool Config::loadShips() {
    char filename[80];
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr  curNode;
    Ship    *lShip;

    sprintf(filename, "%s/ships.xml", Path::Game);

    if(!file_exists(filename))
        return(false);

    if((xmlDoc = xml::loadFile(filename, "Ships")) == nullptr)
        return(false);

    rootNode = xmlDocGetRootElement(xmlDoc);
    curNode = rootNode->children;

    clearShips();
    while(curNode) {
        if(NODE_NAME(curNode, "Ship")) {
            lShip = new Ship;
            lShip->load(curNode);
            ships.push_back(lShip);
        }
        curNode = curNode->next;
    }

    // exits have all been deleted - go and set the current ones
    for(const auto& aShip : ships) {
        // if at a stop, make the exits
        if(aShip->inPort)
            shipSetExits(aShip, aShip->stops.front());
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}
