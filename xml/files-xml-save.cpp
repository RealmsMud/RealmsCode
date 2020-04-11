/*
 * files-xml-save.cpp
 *   Used to serialize structures (object, creature, room, etc) to xml files
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
/*          Web Editor
 *           _   _  ____ _______ ______
 *          | \ | |/ __ \__   __|  ____|
 *          |  \| | |  | | | |  | |__
 *          | . ` | |  | | | |  |  __|
 *          | |\  | |__| | | |  | |____
 *          |_| \_|\____/  |_|  |______|
 *
 *      If you change anything here, make sure the changes are reflected in the web
 *      editor! Either edit the PHP yourself or tell Dominus to make the changes.
 */
#include <libxml/parser.h>        // for xmlNodePtr
#include <map>                    // for operator==, operator!=, allocator
#include <ostream>                // for basic_ostream::operator<<
#include <utility>                // for pair

#include "catRef.hpp"             // for CatRef
#include "config.hpp"             // for Config
#include "lasttime.hpp"           // for lastti
#include "paths.hpp"              // for Path
#include "xml.hpp"                // for newStringChild, newNumChild, newNum...

//#define BIT_ISSET(p,f)  ((p)[(f)/8] & 1<<((f)%8))
//#define BIT_SET(p,f)    ((p)[(f)/8] |= 1<<((f)%8))
//#define BIT_CLEAR(p,f)  ((p)[(f)/8] &= ~(1<<((f)%8)))


//*********************************************************************
//                      saveLastTime
//*********************************************************************

xmlNodePtr saveLastTime(xmlNodePtr parentNode, int i, struct lasttime pLastTime) {
    // Avoid writing un-used last times
    if(!pLastTime.interval && !pLastTime.ltime && !pLastTime.misc)
        return(nullptr);

    xmlNodePtr curNode = xml::newStringChild(parentNode, "LastTime");
    xml::newNumProp(curNode, "Num", i);

    xml::newNumChild(curNode, "Interval", pLastTime.interval);
    xml::newNumChild(curNode, "LastTime", pLastTime.ltime);
    xml::newNumChild(curNode, "Misc", pLastTime.misc);
    return(curNode);
}


//*********************************************************************
//                      saveBits
//*********************************************************************

xmlNodePtr saveBits(xmlNodePtr parentNode, const char* name, int maxBit, const char *bits) {
    xmlNodePtr curNode=nullptr;
    // this nested loop means we won't create an xml node if we don't have to
    for(int i=0; i<maxBit; i++) {
        if(BIT_ISSET(bits, i)) {
            curNode = xml::newStringChild(parentNode, name);
            for(; i<maxBit; i++) {
                if(BIT_ISSET(bits, i))
                    saveBit(curNode, i);
            }
            return(curNode);
        }
    }
    return(curNode);
}

//*********************************************************************
//                      saveBit
//*********************************************************************

xmlNodePtr saveBit(xmlNodePtr parentNode, int bit) {
    xmlNodePtr curNode;

    curNode = xml::newStringChild(parentNode, "Bit");
    xml::newNumProp(curNode, "Num", bit);
    return(curNode);
}

//*********************************************************************
//                      saveLongArray
//*********************************************************************

xmlNodePtr saveLongArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const long array[], int arraySize) {
    xmlNodePtr childNode, curNode=nullptr;
    // this nested loop means we won't create an xml node if we don't have to
    for(int i=0; i<arraySize; i++) {
        if(!array[i]) {
            curNode = xml::newStringChild(parentNode, rootName);
            for(; i<arraySize; i++) {
                if(array[i]) {
                    childNode = xml::newNumChild(curNode, childName, array[i]);
                    xml::newNumProp(childNode, "Num", i);
                }
            }
            return(curNode);
        }
    }
    return(curNode);
}

//*********************************************************************
//                      saveULongArray
//*********************************************************************

xmlNodePtr saveULongArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const unsigned long array[], int arraySize) {
    xmlNodePtr childNode, curNode=nullptr;
    // this nested loop means we won't create an xml node if we don't have to
    for(int i=0; i<arraySize; i++) {
        if(array[i]) {
            curNode = xml::newStringChild(parentNode, rootName);
            for(; i<arraySize; i++) {
                if(array[i]) {
                    childNode = xml::newNumChild(curNode, childName, array[i]);
                    xml::newNumProp(childNode, "Num", i);
                }
            }
            return(curNode);
        }
    }
    return(curNode);
}

//*********************************************************************
//                      saveCatRefArray
//*********************************************************************

xmlNodePtr saveCatRefArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const std::map<int, CatRef>& array, int arraySize) {
    xmlNodePtr curNode=nullptr;
    std::map<int, CatRef>::const_iterator it;

    // this nested loop means we won't create an xml node if we don't have to
    for(it = array.begin(); it != array.end() ; it++) {
        if((*it).second.id) {
            curNode = xml::newStringChild(parentNode, rootName);
            for(; it != array.end() ; it++) {
                if((*it).second.id)
                    (*it).second.save(curNode, childName, false, (*it).first);
            }
            return(curNode);
        }
    }
    return(curNode);
}

//*********************************************************************
//                      saveCatRefArray
//*********************************************************************

xmlNodePtr saveCatRefArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const CatRef array[], int arraySize) {
    xmlNodePtr curNode=nullptr;
    // this nested loop means we won't create an xml node if we don't have to
    for(int i=0; i<arraySize; i++) {
        if(array[i].id) {
            curNode = xml::newStringChild(parentNode, rootName);
            for(; i<arraySize; i++) {
                if(array[i].id)
                    array[i].save(curNode, childName, false, i);
            }
            return(curNode);
        }
    }
    return(curNode);
}


//*********************************************************************
//                      saveShortIntArray
//*********************************************************************

xmlNodePtr saveShortIntArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const short array[], int arraySize) {
    xmlNodePtr childNode, curNode=nullptr;
    // this nested loop means we won't create an xml node if we don't have to
    for(int i=0; i<arraySize; i++) {
        if(array[i]) {
            curNode = xml::newStringChild(parentNode, rootName);
            for(; i<arraySize; i++) {
                if(array[i]) {
                    childNode = xml::newNumChild(curNode, childName, array[i]);
                    xml::newNumProp(childNode, "Num", i);
                }
            }
            return(curNode);
        }
    }
    return(curNode);
}
//
//#undef BIT_ISSET
//#undef BIT_SET
//#undef BIT_CLEAR


//*********************************************************************
//                      saveDoubleLog
//*********************************************************************

void Config::saveDoubleLog() const {
    std::list<accountDouble>::const_iterator it;
    xmlDocPtr   xmlDoc;
    xmlNodePtr      rootNode, curNode;
    char            filename[80];

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "DoubleLog", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    for(it = accountDoubleLog.begin(); it != accountDoubleLog.end() ; it++) {
        curNode = xml::newStringChild(rootNode, "Accounts");
        xml::saveNonNullString(curNode, "Forum1", (*it).first);
        xml::saveNonNullString(curNode, "Forum2", (*it).second);
    }

    sprintf(filename, "%s/doubleLog.xml", Path::Config);
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
}
