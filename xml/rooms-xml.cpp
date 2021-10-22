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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <config.hpp>                               // for Config, gConfig
#include <libxml/parser.h>                          // for xmlNodePtr, xmlFr...
#include <proto.hpp>                                // for roomPath, whatSize
#include <rooms.hpp>                                // for UniqueRoom, NUM_P...
#include <server.hpp>                               // for Server, gServer
#include <cstring>                                  // for strcpy
#include <xml.hpp>                                  // for NODE_NAME, copyToNum

#include "catRef.hpp"                               // for CatRef
#include "enums/loadType.hpp"                       // for LoadType, LoadTyp...
#include "flags.hpp"                                // for MAX_ROOM_FLAGS
#include "global.hpp"                               // for FATAL
#include "lasttime.hpp"                             // for lasttime, crlasttime
#include "os.hpp"                                   // for ASSERTLOG, merror
#include "paths.hpp"                                // for checkDirExists
#include "track.hpp"                                // for Track

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

bool loadRoomFromFile(const CatRef& cr, UniqueRoom **pRoom, std::string filename, bool offline) {
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

    setId(std::string("R") + info.rstr());

    xml::copyPropToString(version, rootNode, "Version");
    curNode = rootNode->children;
    // Start reading stuff in!
    while(curNode) {
        if(NODE_NAME(curNode, "Name")) setName(xml::getString(curNode));
        else if(NODE_NAME(curNode, "ShortDescription")) xml::copyToString(short_desc, curNode);
        else if(NODE_NAME(curNode, "LongDescription")) xml::copyToString(long_desc, curNode);
        else if(NODE_NAME(curNode, "Fishing")) xml::copyToString(fishing, curNode);
        else if(NODE_NAME(curNode, "Faction")) xml::copyToString(faction, curNode);
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
            if(NODE_NAME(curNode, "TrackExit")) track.setDirection(xml::getString(curNode));
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

//*********************************************************************
//                      saveToFile
//*********************************************************************
// This function will write the supplied room to ROOM_PATH/r#####.xml
// It will most likely only be called from *save r

int UniqueRoom::saveToFile(int permOnly, LoadType saveType) {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char        filename[256];

    ASSERTLOG( info.id >= 0 );
    if(saveType == LoadType::LS_BACKUP)
        Path::checkDirExists(info.area, roomBackupPath);
    else
        Path::checkDirExists(info.area, roomPath);

    gServer->saveIds();

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Room", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    escapeText();
    saveToXml(rootNode, permOnly);

    if(saveType == LoadType::LS_BACKUP)
        strcpy(filename, roomBackupPath(info));
    else
        strcpy(filename, roomPath(info));
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
    return(0);
}


//*********************************************************************
//                      saveToXml
//*********************************************************************

int UniqueRoom::saveToXml(xmlNodePtr rootNode, int permOnly) const {
    std::map<int, crlasttime>::const_iterator it;
    xmlNodePtr      curNode;
    int i;

    if(getName()[0] == '\0' || rootNode == nullptr)
        return(-1);

    // record rooms saved during swap
    if(gConfig->swapIsInteresting(this))
        gConfig->swapLog((std::string)"r" + info.rstr(), false);

    xml::newNumProp(rootNode, "Num", info.id);
    xml::newProp(rootNode, "Version", gConfig->getVersion());
    xml::newProp(rootNode, "Area", info.area);

    xml::saveNonNullString(rootNode, "Name", getName());
    xml::saveNonNullString(rootNode, "ShortDescription", short_desc);
    xml::saveNonNullString(rootNode, "LongDescription", long_desc);
    xml::saveNonNullString(rootNode, "Fishing", fishing);
    xml::saveNonNullString(rootNode, "Faction", faction);
    xml::saveNonNullString(rootNode, "LastModBy", last_mod);
    // TODO: Change this into a TimeStamp
    xml::saveNonNullString(rootNode, "LastModTime", lastModTime);
    xml::saveNonNullString(rootNode, "LastPlayer", lastPly);
    // TODO: Change this into a TimeStamp
    xml::saveNonNullString(rootNode, "LastPlayerTime", lastPlyTime);


    xml::saveNonZeroNum(rootNode, "LowLevel", lowLevel);
    xml::saveNonZeroNum(rootNode, "HighLevel", highLevel);
    xml::saveNonZeroNum(rootNode, "MaxMobs", maxmobs);

    xml::saveNonZeroNum(rootNode, "Trap", trap);
    trapexit.save(rootNode, "TrapExit", false);
    xml::saveNonZeroNum(rootNode, "TrapWeight", trapweight);
    xml::saveNonZeroNum(rootNode, "TrapStrength", trapstrength);

    xml::saveNonZeroNum(rootNode, "BeenHere", beenhere);
    xml::saveNonZeroNum(rootNode, "RoomExp", roomExp);

    saveBits(rootNode, "Flags", MAX_ROOM_FLAGS, flags);

    curNode = xml::newStringChild(rootNode, "Wander");
    wander.save(curNode);

    if(!track.getDirection().empty()) {
        curNode = xml::newStringChild(rootNode, "Track");
        track.save(curNode);
    }

    xml::saveNonZeroNum(rootNode, "Size", size);
    effects.save(rootNode, "Effects");
    hooks.save(rootNode, "Hooks");



    // Save Perm Mobs
    curNode = xml::newStringChild(rootNode, "PermMobs");
    for(it = permMonsters.begin(); it != permMonsters.end() ; it++) {
        saveCrLastTime(curNode, (*it).first, (*it).second);
    }

    // Save Perm Objs
    curNode = xml::newStringChild(rootNode, "PermObjs");
    for(it = permObjects.begin(); it != permObjects.end() ; it++) {
        saveCrLastTime(curNode, (*it).first, (*it).second);
    }

    // Save LastTimes -- Dunno if we need this?
    for(i=0; i<16; i++) {
        // this nested loop means we won't create an xml node if we don't have to
        if(lasttime[i].interval || lasttime[i].ltime || lasttime[i].misc) {
            curNode = xml::newStringChild(rootNode, "LastTimes");
            for(; i<16; i++)
                saveLastTime(curNode, i, lasttime[i]);
        }
    }

    // Save Objects
    curNode = xml::newStringChild(rootNode, "Objects");
    saveObjectsXml(curNode, objects, permOnly);

    // Save Creatures
    curNode = xml::newStringChild(rootNode, "Creatures");
    saveCreaturesXml(curNode, monsters, permOnly);

    // Save Exits
    curNode = xml::newStringChild(rootNode, "Exits");
    saveExitsXml(curNode);
    return(0);
}

//*********************************************************************
//                      load
//*********************************************************************

void AreaRoom::load(xmlNodePtr rootNode) {
    xmlNodePtr childNode = rootNode->children;

    clearExits();

    while(childNode) {
        if(NODE_NAME(childNode, "Exits"))
            readExitsXml(childNode);
        else if(NODE_NAME(childNode, "MapMarker")) {
            mapmarker.load(childNode);
            area->rooms[mapmarker.str()] = this;
        }
        else if(NODE_NAME(childNode, "Unique")) unique.load(childNode);
        else if(NODE_NAME(childNode, "NeedsCompass")) xml::copyToBool(needsCompass, childNode);
        else if(NODE_NAME(childNode, "DecCompass")) xml::copyToBool(decCompass, childNode);
        else if(NODE_NAME(childNode, "Effects")) effects.load(childNode);
        else if(NODE_NAME(childNode, "Hooks")) hooks.load(childNode);

        childNode = childNode->next;
    }
    addEffectsIndex();
}

