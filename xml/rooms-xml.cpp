/*
 * rooms-xml.cpp
 *   Rooms xml
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

#include <rooms.hpp>
#include <proto.hpp>
#include <server.hpp>
#include <mud.hpp>
#include <xml.hpp>

//*********************************************************************
//                      loadRoom
//*********************************************************************

bool loadRoom(int index, UniqueRoom **pRoom, bool offline) {
    CatRef  cr;
    cr.id = index;
    return(loadRoom(cr, pRoom, offline));
}

bool loadRoom(const CatRef& cr, UniqueRoom **pRoom, bool offline) {
    if(!validRoomId(cr))
        return(false);

    if(!gServer->roomCache.fetch(cr, pRoom)) {
        if(!loadRoomFromFile(cr, pRoom, "", offline))
            return(false);
        gServer->roomCache.insert(cr, pRoom);
        if(!offline) {
            (*pRoom)->registerMo();
        }
    }

    return(true);
}

//*********************************************************************
//                      loadRoomFromFile
//*********************************************************************
// if we're loading only from a filename, get CatRef from file

bool loadRoomFromFile(const CatRef& cr, UniqueRoom **pRoom, bstring filename, bool offline) {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    int         num;

    if(filename.empty())
        filename = roomPath(cr);

    if((xmlDoc = xml::loadFile(filename.c_str(), "Room")) == nullptr)
        return(false);

    rootNode = xmlDocGetRootElement(xmlDoc);
    num = xml::getIntProp(rootNode, "Num");
    bool toReturn = false;

    if(cr.id == -1 || num == cr.id) {
        *pRoom = new UniqueRoom;
        if(!*pRoom)
            merror("loadRoomFromFile", FATAL);
        (*pRoom)->setVersion(xml::getProp(rootNode, "Version"));

        (*pRoom)->readFromXml(rootNode, offline);
        toReturn = true;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(toReturn);
}


//*********************************************************************
//                      readFromXml
//*********************************************************************
// Reads a room from the given xml document and root node

int UniqueRoom::readFromXml(xmlNodePtr rootNode, bool offline) {
    xmlNodePtr curNode;
//  xmlNodePtr childNode;

    info.load(rootNode);
    info.id = xml::getIntProp(rootNode, "Num");

    setId(bstring("R") + info.rstr());

    xml::copyPropToBString(version, rootNode, "Version");
    curNode = rootNode->children;
    // Start reading stuff in!
    while(curNode) {
        if(NODE_NAME(curNode, "Name")) setName(xml::getBString(curNode));
        else if(NODE_NAME(curNode, "ShortDescription")) xml::copyToBString(short_desc, curNode);
        else if(NODE_NAME(curNode, "LongDescription")) xml::copyToBString(long_desc, curNode);
        else if(NODE_NAME(curNode, "Fishing")) xml::copyToBString(fishing, curNode);
        else if(NODE_NAME(curNode, "Faction")) xml::copyToBString(faction, curNode);
        else if(NODE_NAME(curNode, "LastModBy")) xml::copyToCString(last_mod, curNode);
        else if(NODE_NAME(curNode, "LastModTime")) xml::copyToCString(lastModTime, curNode);
        else if(NODE_NAME(curNode, "LastPlayer")) xml::copyToCString(lastPly, curNode);
        else if(NODE_NAME(curNode, "LastPlayerTime")) xml::copyToCString(lastPlyTime, curNode);
        else if(NODE_NAME(curNode, "LowLevel")) xml::copyToNum(lowLevel, curNode);
        else if(NODE_NAME(curNode, "HighLevel")) xml::copyToNum(highLevel, curNode);
        else if(NODE_NAME(curNode, "MaxMobs")) xml::copyToNum(maxmobs, curNode);
        else if(NODE_NAME(curNode, "Trap")) xml::copyToNum(trap, curNode);
        else if(NODE_NAME(curNode, "TrapExit")) trapexit.load(curNode);
        else if(NODE_NAME(curNode, "TrapWeight")) xml::copyToNum(trapweight, curNode);
        else if(NODE_NAME(curNode, "TrapStrength")) xml::copyToNum(trapstrength, curNode);

        else if(NODE_NAME(curNode, "BeenHere")) xml::copyToNum(beenhere, curNode);
        else if(NODE_NAME(curNode, "Track")) track.load(curNode);
        else if(NODE_NAME(curNode, "Wander")) wander.load(curNode);
        else if(NODE_NAME(curNode, "RoomExp")) xml::copyToNum(roomExp, curNode);
        else if(NODE_NAME(curNode, "Flags")) {
            // No need to clear flags, no room refs
            loadBits(curNode, flags);
        }
        else if(NODE_NAME(curNode, "PermMobs")) {
            loadCrLastTimes(curNode, permMonsters);
        }
        else if(NODE_NAME(curNode, "PermObjs")) {
            loadCrLastTimes(curNode, permObjects);
        }
        else if(NODE_NAME(curNode, "LastTimes")) {
            loadLastTimes(curNode, lasttime);
        }
        else if(NODE_NAME(curNode, "Objects")) {
            readObjects(curNode, offline);
        }
        else if(NODE_NAME(curNode, "Creatures")) {
            readCreatures(curNode, offline);
        }
        else if(NODE_NAME(curNode, "Exits")) {
            readExitsXml(curNode, offline);
        }
        else if(NODE_NAME(curNode, "Effects")) effects.load(curNode, this);
        else if(NODE_NAME(curNode, "Size")) size = whatSize(xml::toNum<int>(curNode));
        else if(NODE_NAME(curNode, "Hooks")) hooks.load(curNode);


        // load old tracks
        if(getVersion() < "2.32d") {
            if(NODE_NAME(curNode, "TrackExit")) track.setDirection(xml::getBString(curNode));
            else if(NODE_NAME(curNode, "TrackSize")) track.setSize(whatSize(xml::toNum<int>(curNode)));
            else if(NODE_NAME(curNode, "TrackNum")) track.setNum(xml::toNum<int>(curNode));
        }
        // load old wander info
        if(getVersion() < "2.33b") {
            if(NODE_NAME(curNode, "Traffic")) wander.setTraffic(xml::toNum<int>(curNode));
            else if(NODE_NAME(curNode, "RandomMonsters")) {
                loadCatRefArray(curNode, wander.random, "Mob", NUM_RANDOM_SLOTS);
            }
        }
        if(getVersion() < "2.42e") {
            if(NODE_NAME(curNode, "Special")) xml::copyToNum(maxmobs, curNode);
        }
        curNode = curNode->next;
    }

    escapeText();

    if(!offline)
        addEffectsIndex();

    return(0);
}

//*********************************************************************
//                      loadCrLastTimes
//*********************************************************************
// This function will load all crlasttimer into the given crlasttime array

void loadCrLastTimes(xmlNodePtr curNode, std::map<int, crlasttime>& pCrLastTimes) {
    xmlNodePtr childNode = curNode->children;
    int i=0;

    while(childNode) {
        if(NODE_NAME(childNode, "LastTime")) {
            i = xml::getIntProp(childNode, "Num");
            if(i >= 0 && i < NUM_PERM_SLOTS) {
                struct crlasttime cr;
                loadCrLastTime(childNode, &cr);
                if(cr.cr.id)
                    pCrLastTimes[i] = cr;
            }
        }
        childNode = childNode->next;
    }
}


