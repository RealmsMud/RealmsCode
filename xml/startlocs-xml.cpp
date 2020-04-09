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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "config.hpp"
#include "paths.hpp"
#include "startlocs.hpp"
#include "xml.hpp"

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
    bstring loc = "";
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Location")) {
            xml::copyPropToBString(loc, curNode, "Name");
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
    std::map<bstring, StartLoc*>::const_iterator it;
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
    xml::copyPropToBString(name, curNode, "Name");
    while(childNode) {
        if(NODE_NAME(childNode, "BindName")) xml::copyToBString(bindName, childNode);
        else if(NODE_NAME(childNode, "RequiredName")) xml::copyToBString(requiredName, childNode);
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


