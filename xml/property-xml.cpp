/*
 * property-xml.cpp
 *   Code dealing with properties xml
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
#include "property.hpp"
#include "paths.hpp"
#include "rooms.hpp"
#include "xml.hpp"

void PartialOwner::load(xmlNodePtr rootNode) {
    xml::copyPropToBString(name, rootNode, "Name");
    loadBits(rootNode, flags);
}

void PartialOwner::save(xmlNodePtr rootNode) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "Owner") ;

    xml::newProp(curNode, "Name", name.c_str());
    for(int i=0; i<32; i++) {
        if(BIT_ISSET(flags, i))
            saveBit(curNode, i);
    }
}

//*********************************************************************
//                      load
//*********************************************************************

void Property::load(xmlNodePtr rootNode) {
    xmlNodePtr childNode, curNode = rootNode->children;
    bstring temp = "";
    CatRef low;

    while(curNode) {
        if(NODE_NAME(curNode, "Owner")) xml::copyToBString(owner, curNode);
        else if(NODE_NAME(curNode, "Area")) xml::copyToBString(area, curNode);
        else if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
        else if(NODE_NAME(curNode, "DateFounded")) xml::copyToBString(dateFounded, curNode);
        else if(NODE_NAME(curNode, "Location")) xml::copyToBString(location, curNode);
        else if(NODE_NAME(curNode, "Guild")) xml::copyToNum(guild, curNode);
        else if(NODE_NAME(curNode, "Type")) type = (PropType)xml::toNum<int>(curNode);
        else if(NODE_NAME(curNode, "Flags")) loadBits(curNode, flags);

        else if(NODE_NAME(curNode, "Ranges")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Range")) {
                    Range r;
                    r.load(childNode);

                    // find the first in the range
                    if(!low.id)
                        low = r.low;

                    ranges.push_back(r);
                }
                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "PartialOwners")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Owner")) {
                    PartialOwner po;
                    po.load(childNode);
                    if(!po.getName().empty())
                        partialOwners.push_back(po);
                }
                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "LogType")) logType = (PropLog)xml::toNum<int>(curNode);
        else if(NODE_NAME(curNode, "Log")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Entry")) {
                    xml::copyToBString(temp, childNode);
                    log.push_back(temp);
                }
                childNode = childNode->next;
            }
        }
        curNode = curNode->next;
    }

    // backwards compatability - removable when all are updated
    if(type != PROP_STORAGE && area.empty()) {
        // load intro room, find the out exit, look at the room info
        UniqueRoom* room=nullptr;
        if(loadRoom(low, &room)) {
            for(Exit* ext : room->exits) {
                if(ext->target.room.area != "shop" && low.area != ext->target.room.area) {
                    area = ext->target.room.area;
                    break;
                }
            }
        }
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void Property::save(xmlNodePtr rootNode) const {
    xmlNodePtr childNode, curNode = xml::newStringChild(rootNode, "Property");

    xml::saveNonNullString(curNode, "Owner", owner.c_str());
    xml::saveNonNullString(curNode, "Area", area.c_str());
    xml::saveNonNullString(curNode, "Name", name.c_str());
    xml::saveNonNullString(curNode, "DateFounded", dateFounded.c_str());
    xml::saveNonNullString(curNode, "Location", location.c_str());
    xml::saveNonZeroNum(curNode, "Guild", guild);
    xml::saveNonZeroNum(curNode, "Type", (int)type);
    saveBits(curNode, "Flags", 32, flags);

    if(!ranges.empty()) {
        std::list<Range>::const_iterator rt;
        childNode = xml::newStringChild(curNode, "Ranges");

        for(rt = ranges.begin() ; rt != ranges.end() ; rt++)
            (*rt).save(childNode, "Range");
    }

    if(!partialOwners.empty()) {
        std::list<PartialOwner>::const_iterator pt;
        childNode = xml::newStringChild(curNode, "PartialOwners");

        for(pt = partialOwners.begin() ; pt != partialOwners.end() ; pt++)
            (*pt).save(childNode);
    }
    xml::saveNonZeroNum(curNode, "LogType", logType);
    if(!log.empty()) {
        std::list<bstring>::const_iterator it;
        childNode = xml::newStringChild(curNode, "Log");

        for(it = log.begin() ; it != log.end() ; it++) {
            xml::saveNonNullString(childNode, "Entry", (*it).c_str());
        }
    }
}


//*********************************************************************
//                      loadProperties
//*********************************************************************

bool Config::loadProperties() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;
    Property *p=nullptr;

    char filename[80];
    snprintf(filename, 80, "%s/properties.xml", Path::PlayerData);
    xmlDoc = xml::loadFile(filename, "Properties");

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

    clearProperties();
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Property")) {
            p = new Property;
            p->load(curNode);
            properties.push_back(p);
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();

    return(true);
}

//*********************************************************************
//                      saveProperties
//*********************************************************************

bool Config::saveProperties() const {
    std::list<Property*>::const_iterator it;
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char            filename[80];

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Properties", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    for(it = properties.begin() ; it != properties.end() ; it++) {
        (*it)->save(rootNode);
    }

    sprintf(filename, "%s/properties.xml", Path::PlayerData);
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
    return(true);
}



