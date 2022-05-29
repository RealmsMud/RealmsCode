/* files-xml-read.cpp
 *   Used to read objects/rooms/creatures etc from xml files
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

#include <libxml/parser.h>                          // for xmlFreeDoc, xmlDo...
#include <stdio.h>                                  // for snprintf
#include <boost/dynamic_bitset.hpp>
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <cstring>                                  // for strcpy, strlen
#include <list>                                     // for list
#include <map>                                      // for allocator, map
#include <string>                                   // for string

#include "catRef.hpp"                               // for CatRef
#include "config.hpp"                               // for accountDouble
#include "lasttime.hpp"                             // for lasttime
#include "libxml/parser.h"                          // for xmlCleanupParser
#include "paths.hpp"                                // for Config
#include "xml.hpp"                                  // for getIntProp, NODE_...

//*********************************************************************
//                      loadCatRefArray
//*********************************************************************

void loadCatRefArray(xmlNodePtr curNode, std::map<int, CatRef>& array, const char* name, int maxProp) {
    xmlNodePtr childNode = curNode->children;
    int i=0;

    while(childNode) {
        if(NODE_NAME(childNode , name)) {
            i = xml::getIntProp(childNode, "Num");
            if(i >= 0 && i < maxProp)
                array[i].load(childNode);
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      loadCatRefArray
//*********************************************************************

void loadCatRefArray(xmlNodePtr curNode, CatRef array[], const char* name, int maxProp) {
    xmlNodePtr childNode = curNode->children;
    int i=0;

    while(childNode) {
        if(NODE_NAME(childNode , name)) {
            i = xml::getIntProp(childNode, "Num");
            if(i >= 0 && i < maxProp)
                array[i].load(childNode);
        }
        childNode = childNode->next;
    }
}



//*********************************************************************
//                      loadString Array
//*********************************************************************
// Since we can't easily pass in variable length arrays, we pass in a pointer to the array
// as well as the size of the array. Based on the size and current number being read in
// we do some pointer arithmetic and then memcpy to the appropriate location

// Note: Size MUST be accurate or you will get unpredictable results

void loadStringArray(xmlNodePtr curNode, void* array, int size, const char* name, int maxProp) {
    size_t j=0;
    int i=0;
    char temp[255];
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode , name)) {
            i = xml::getIntProp(childNode, "Num");
            if(i < maxProp) {
                xml::copyToCString(temp, childNode);

                j = strlen(temp);
                if(j > (size - 1))
                    j = (size - 1);
                temp[j] = '\0';

                strcpy((char*)array + ( i * sizeof(char) * size), temp);
            }
        }
        childNode = childNode->next;
    }
}


//*********************************************************************
//                      loadBits
//*********************************************************************
// Sets all bits it finds into the given bitset

void loadBitset(xmlNodePtr curNode, boost::dynamic_bitset<>& bits) {
    xmlNodePtr childNode = curNode->children;
    int bit=0;

    while(childNode) {
        if(NODE_NAME(childNode, "Bit")) {
            bit = xml::getIntProp(childNode, "Num");
            bits.set(bit);
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      loadLastTimes
//*********************************************************************
// This function will load all lasttimer into the given lasttime array

void loadLastTimes(xmlNodePtr curNode, struct lasttime* pLastTimes) {
    xmlNodePtr childNode = curNode->children;
    int i=0;

    while(childNode) {
        if(NODE_NAME(childNode, "LastTime")) {
            i = xml::getIntProp(childNode, "Num");
            // TODO: Sanity check i
            loadLastTime(childNode, &pLastTimes[i]);
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      loadLastTime
//*********************************************************************
// Loads a single lasttime into the given lasttime

void loadLastTime(xmlNodePtr curNode, struct lasttime* pLastTime) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Interval")) xml::copyToNum(pLastTime->interval, childNode);
        else if(NODE_NAME(childNode, "LastTime")) xml::copyToNum(pLastTime->ltime, childNode);
        else if(NODE_NAME(childNode, "Misc")) xml::copyToNum(pLastTime->misc, childNode);

        childNode = childNode->next;
    }
}
