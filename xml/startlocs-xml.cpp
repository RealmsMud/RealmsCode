/*
 * startlocs-xml.cpp
 *   Player starting location code - XML
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

#include <libxml/parser.h>        // for xmlFreeDoc, xmlNodePtr, xmlNode
#include <cstdio>                 // for snprintf, sprintf
#include <map>                    // for operator==, map, operator!=, allocator

#include "config.hpp"             // for Config
#include "location.hpp"           // for Location
#include "paths.hpp"              // for Game
#include "startlocs.hpp"          // for StartLoc
#include "xml.hpp"                // for NODE_NAME, copyPropToString, copyT...

//*********************************************************************
//                      loadStartLoc
//*********************************************************************

bool Config::loadStartLoc() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;
    // first one loaded is the primary one
    bool primary=true;

    char filename[80];
    snprintf(filename, 80, "%s/start.xml", Path::Game);
    xmlDoc = xml::loadFile(filename, "Locations");

    if(xmlDoc == nullptr)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode))
        curNode = curNode->next;

    if(curNode == nullptr) {
        xmlFreeDoc(xmlDoc);
        return(false);
    }

    clearStartLoc();
    std::string loc = "";
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Location")) {
            xml::copyPropToString(loc, curNode, "Name");
            if(!loc.empty() && start.find(loc) == start.end()) {
                start[loc] = new StartLoc;
                start[loc]->load(curNode);
                if(primary) {
                    start[loc]->setDefault();
                    primary = false;
                }
            }
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();

    return(true);
}

//*********************************************************************
//                      saveStartLocs
//*********************************************************************

void Config::saveStartLocs() const {
    std::map<std::string, StartLoc*>::const_iterator it;
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char            filename[80];

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Locations", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    for(it = start.begin() ; it != start.end() ; it++)
        (*it).second->save(rootNode);

    sprintf(filename, "%s/start.xml", Path::Game);
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
}

//*********************************************************************
//                      load
//*********************************************************************

void StartLoc::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    xml::copyPropToString(name, curNode, "Name");
    while(childNode) {
        if(NODE_NAME(childNode, "BindName")) xml::copyToString(bindName, childNode);
        else if(NODE_NAME(childNode, "RequiredName")) xml::copyToString(requiredName, childNode);
        else if(NODE_NAME(childNode, "Bind")) bind.load(childNode);
        else if(NODE_NAME(childNode, "Required")) required.load(childNode);
        else if(NODE_NAME(childNode, "StartingGuide")) startingGuide.load(childNode);
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void StartLoc::save(xmlNodePtr curNode) const {
    xmlNodePtr childNode = xml::newStringChild(curNode, "Location");
    xml::newProp(childNode, "Name", name);

    xml::saveNonNullString(childNode, "BindName", bindName);
    xml::saveNonNullString(childNode, "RequiredName", requiredName);
    bind.save(childNode, "Bind");
    required.save(childNode, "Required");
    startingGuide.save(childNode, "StartingGuide", false);
}


