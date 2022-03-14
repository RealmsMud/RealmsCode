/*
 * exits-xml.cpp
 *   Exits xml
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

#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <list>                                     // for operator==, _List...
#include <ostream>                                  // for basic_ostream::op...
#include <string>                                   // for allocator, string

#include "area.hpp"                                 // for MapMarker
#include "catRef.hpp"                               // for CatRef
#include "flags.hpp"                                // for X_PORTAL, MAX_EXI...
#include "hooks.hpp"                                // for Hooks
#include "location.hpp"                             // for Location
#include "mudObjects/areaRooms.hpp"                 // for AreaRoom
#include "mudObjects/exits.hpp"                     // for Exit, Direction
#include "mudObjects/rooms.hpp"                     // for BaseRoom, ExitList
#include "size.hpp"                                 // for whatSize
#include "xml.hpp"                                  // for NODE_NAME, saveNo...


//*********************************************************************
//                      readFromXml
//*********************************************************************
// Reads an exit from the given xml document and root node

int Exit::readFromXml(xmlNodePtr rootNode, BaseRoom* room, bool offline) {
    xmlNodePtr curNode, childNode;

    setName(xml::getProp(rootNode, "Name"));
    //xml::copyPropToCString(name, rootNode, "Name");

    curNode = rootNode->children;
    // Start reading stuff in!
    while(curNode) {
        if(NODE_NAME(curNode, "Name")) setName(xml::getString(curNode));
        else if(NODE_NAME(curNode, "Keys")) {
            loadStringArray(curNode, desc_key, EXIT_KEY_LENGTH, "Key", 3);
        }
        else if(NODE_NAME(curNode, "Level")) xml::copyToNum(level, curNode);
        else if(NODE_NAME(curNode, "Trap")) xml::copyToNum(trap, curNode);
        else if(NODE_NAME(curNode, "Key")) xml::copyToNum(key, curNode);
        else if(NODE_NAME(curNode, "KeyArea")) xml::copyToString(keyArea, curNode);
        else if(NODE_NAME(curNode, "Size")) size = whatSize(xml::toNum<int>(curNode));
        else if(NODE_NAME(curNode, "Direction")) direction = (Direction)xml::toNum<int>(curNode);
        else if(NODE_NAME(curNode, "Toll")) xml::copyToNum(toll, curNode);
        else if(NODE_NAME(curNode, "PassPhrase")) xml::copyToString(passphrase, curNode);
        else if(NODE_NAME(curNode, "PassLang")) xml::copyToNum(passlang, curNode);
        else if(NODE_NAME(curNode, "Description")) xml::copyToString(description, curNode);
        else if(NODE_NAME(curNode, "Open")) xml::copyToString(open, curNode);
        else if(NODE_NAME(curNode, "Enter")) { xml::copyToString(enter, curNode); }
        else if(NODE_NAME(curNode, "Flags")) {
            // No need to clear flags, no exit refs
            loadBits(curNode, flags);
        }
        else if(NODE_NAME(curNode, "Target")) target.load(curNode);
        else if(NODE_NAME(curNode, "Effects")) effects.load(curNode, this);
        else if(NODE_NAME(curNode, "Hooks")) hooks.load(curNode);

            // depreciated, but there's no version function
            // TODO: Use room->getVersion()
        else if(NODE_NAME(curNode, "Room")) target.room.load(curNode);
        else if(NODE_NAME(curNode, "AreaRoom")) target.mapmarker.load(curNode);
        else if(NODE_NAME(curNode, "UsedBy")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Player")) {
                    std::string s;
                    xml::copyToString(s, childNode);
                    usedBy.push_back(s);
                }
                childNode = childNode->next;
            }
        }

        curNode = curNode->next;
    }
    if(room) {
        room->addExit(this);

#define X_OLD_INVISIBLE             1         // Invisible
        if(room->getVersion() < "2.47b" && flagIsSet(X_OLD_INVISIBLE)) {
            addEffect("invisibility", -1);
            clearFlag(X_OLD_INVISIBLE);
        }
    }
    escapeText();
    return(0);
}


//*********************************************************************
//                      readExitsXml
//*********************************************************************

void BaseRoom::readExitsXml(xmlNodePtr curNode, bool offline) {
    xmlNodePtr childNode = curNode->children;
    AreaRoom* aRoom = getAsAreaRoom();

    while(childNode) {
        if(NODE_NAME(childNode , "Exit")) {
            auto ext = new Exit;
            ext->readFromXml(childNode, this, offline);

            if(!ext->flagIsSet(X_PORTAL)) {
                // moving cardinal exit on the overland?
                if(ext->flagIsSet(X_MOVING) && aRoom && aRoom->updateExit(ext->getName()))
                    delExit(ext);
            } else
                delExit(ext);
        }
        childNode = childNode->next;
    }
}


//*********************************************************************
//                      saveToXml
//*********************************************************************

int Exit::saveToXml(xmlNodePtr parentNode) const {
    xmlNodePtr  rootNode;
    xmlNodePtr      curNode;
    xmlNodePtr      childNode;

    int i;

    if(parentNode == nullptr || flagIsSet(X_PORTAL))
        return(-1);

    rootNode = xml::newStringChild(parentNode, "Exit");
    xml::newProp(rootNode, "Name", getName());

    // Exit Keys
    curNode = xml::newStringChild(rootNode, "Keys");
    for(i=0; i<3; i++) {
        if(desc_key[i][0] == 0)
            continue;
        childNode = xml::newStringChild(curNode, "Key", desc_key[i]);
        xml::newNumProp(childNode, "Num",i);
    }

    target.save(rootNode, "Target");
    xml::saveNonZeroNum(rootNode, "Toll", toll);
    xml::saveNonZeroNum(rootNode, "Level", level);
    xml::saveNonZeroNum(rootNode, "Trap", trap);
    xml::saveNonZeroNum(rootNode, "Key", key);
    xml::saveNonNullString(rootNode, "KeyArea", keyArea);
    xml::saveNonZeroNum(rootNode, "Size", size);
    xml::saveNonZeroNum(rootNode, "Direction", (int)direction);
    xml::saveNonNullString(rootNode, "PassPhrase", passphrase);
    xml::saveNonZeroNum(rootNode, "PassLang", passlang);
    xml::saveNonNullString(rootNode, "Description", description);
    xml::saveNonNullString(rootNode, "Enter", enter);
    xml::saveNonNullString(rootNode, "Open", open);

    std::list<std::string>::const_iterator ub;
    if(!usedBy.empty()) {
        childNode = xml::newStringChild(rootNode, "UsedBy");
        for(ub = usedBy.begin() ; ub != usedBy.end() ; ub++) {
            xml::newStringChild(childNode, "Player", (*ub));
        }
    }

    effects.save(rootNode, "Effects");
    saveBits(rootNode, "Flags", MAX_EXIT_FLAGS, flags);
    saveLastTime(curNode, 0, ltime);
    hooks.save(rootNode, "Hooks");

    return(0);
}




//*********************************************************************
//                      saveExitsXml
//*********************************************************************

int BaseRoom::saveExitsXml(xmlNodePtr curNode) const {
    for(Exit* exit : exits) {
        exit->saveToXml(curNode);
    }
    return(0);
}

