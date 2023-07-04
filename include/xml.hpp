/*
 * xml.h
 *   Prototypes and anything global needed to read/write xml files
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

#pragma once

#include <map>
#include <filesystem>

#include <libxml/parser.h>           // for xmlNodePtr
#include <boost/lexical_cast.hpp>
#include <boost/dynamic_bitset.hpp>
#include "boost/stacktrace.hpp"

#include "carry.hpp"
#include "mudObjects/container.hpp"
#include "enums/loadType.hpp"

namespace fs = std::filesystem;

using boost::lexical_cast;
using boost::bad_lexical_cast;

class Anchor;
class Ban;
class BaseRoom;
class Creature;
class CRLastTime;
class Guild;
class GuildCreation;
class LastTime;
class Monster;
class Object;
class Player;
class Room;
class Stat;

//**********************
//  Defines Section
//**********************

#define OBJ     1
#define PLY     2
#define CRT     3
#define ROOM    4

#define NODE_NAME(pNode, pName)         (!strcmp((char *)(pNode)->name, (pName) ))

namespace xml {
    // copyToString - will make store the string into a temp cstr, set the string
    // and then free the temp cstr
    void copyToString(std::string &to, xmlNodePtr node);

    std::string getString(xmlNodePtr node);

    // getProp -- You MUST free the return value
    std::string getProp(xmlNodePtr node, const char *name);

    // getIntProp -- Properly frees the return value using toNum
    int getIntProp(xmlNodePtr node, const char *name);

    // getIntProp -- Properly frees the return value using toNum
    short getShortProp(xmlNodePtr node, const char *name);

    // getCString -- You MUST free the return value
    char* getCString(xmlNodePtr node);

    //***************************************************************************************
    // XML_COPY_* - These functions will copy the given type to the variable 'to'
    //***************************************************************************************

    // copyPropToCString -- This will properly free the return value using doStrCpy
    void copyPropToCString(char* to, xmlNodePtr node, const std::string &name);
    //#define copyPropToCString(to, node, name) doStrCpy( (to) , getProp( (node) , (name) ))

    // copyPropToString -- This will properly free the value
    void copyPropToString(std::string &to, xmlNodePtr node, const std::string &name);

    // copyToCString -- Properly frees the return value using doStrCpy
    void copyToCString(char* to, xmlNodePtr node);
    //#define copyToCString(to, node)   doStrCpy( (to), getCString( (node) ))

    // XML_DUP_STRING_CHILD -- Will return strdup of a string -- you MUST free it
    void dupeToCString(char* to, xmlNodePtr node);

        // copyToBool -- Properly frees the return value using toBoolean
    void copyToBool(bool& to, xmlNodePtr node);


    //*******************************************************************************
    // XML_NEW_* will always save (even if blank or 0) and will still let you
    // get the return value from xmlNewChild
    // ###  IMPORTANT  ### -- Name and Value must NOT include any bad xml characters
    // like &, <, >, etc.
    //********************************************************************************

    xmlAttrPtr newProp(xmlNodePtr node, const std::string &name, const std::string &value);

    template <class Type>
    std::string numToStr(const Type& value) {
        std::ostringstream str;
        str << value;
        return(str.str());
    }

    template <class Type>
    xmlAttrPtr newNumProp(xmlNodePtr node, std::string name, const Type& value) {
        return( xmlNewProp( node, BAD_CAST (name.c_str()), BAD_CAST(numToStr(value).c_str())));
    }

    template <class Type>
    xmlNodePtr newNumChild(xmlNodePtr node, std::string name, const Type& value) {
        return(xmlNewChild( node, nullptr, BAD_CAST (name.c_str()), BAD_CAST numToStr(value).c_str()));
    }

    template <class Type>
    xmlNodePtr saveNonZeroNum(xmlNodePtr node, std::string name, const Type& value) {
        xmlNodePtr toReturn = nullptr;
        // precision for floats
        if((int)(value*10000)) {
            toReturn = xmlNewChild( (node), nullptr, BAD_CAST (name.c_str()), BAD_CAST numToStr(value).c_str());
        }
        return(toReturn);
    }

    template <class Type>
    Type toNum(char *fromStr) {
        Type toReturn = static_cast<Type>(0);
        if(!fromStr)
            return(toReturn);

        try {
            toReturn = (lexical_cast<Type>(fromStr));
        } catch (bad_lexical_cast &) {
            // And do nothing
            std::clog << "Error from lexical_cast `" << fromStr << "'\n";
            std::clog << boost::stacktrace::stacktrace();
        }

        free(fromStr);
        return (toReturn);
    }

    template <class Type>
    Type toNum(xmlNodePtr node) {
        return(toNum<Type>(getCString(node)));
    }

    template <class Type>
    void copyToNum(Type& to, xmlNodePtr node) {
        to = toNum<Type>(getCString(node));
    }

    template <class Type, class NumType>
    void copyToNum(Type& to, xmlNodePtr node) {
        // Useful if you need to lexical_cast to an INT, and then to an ENUM
        to = static_cast<Type>(toNum<NumType>(getCString(node)));
    }

    template <class Type>
    void loadNumArray(xmlNodePtr curNode, Type array[], const char* nodeName, int maxProp) {
        xmlNodePtr childNode = curNode->children;
        int i;
        while(childNode) {
            if(NODE_NAME(childNode , nodeName)) {
                i = xml::getIntProp(childNode, "Num");
                if(i < maxProp)
                    array[i] = xml::toNum<Type>(childNode);
            }
            childNode = childNode->next;
        }
    }

    xmlNodePtr newBoolChild(xmlNodePtr node, const std::string &name, const bool value);
    xmlNodePtr newStringChild(xmlNodePtr node, const std::string &name, const std::string &value = "");

    //***************************************************************************************
    // saveNonZero/Null will only save if it is non 0 and also escape any bad XML characters
    //***************************************************************************************

    xmlNodePtr saveNonNullString(xmlNodePtr node, const std::string &name, const std::string &value);

    xmlChar* ConvertInput(const char *in, const char *encoding);
    char *doStrCpy(char *dest, char *src);
    char *doStrDup(char *src);
    xmlDocPtr loadFile(const fs::path&, const char *expectedRoot);
    int saveFile(const fs::path& filename, xmlDocPtr cur);

} // End xml namespace



//******************
//  Load Section
//******************

bool loadMonster(int index, std::shared_ptr<Monster>& pMonster, bool offline=false);
bool loadMonster(const CatRef& cr, std::shared_ptr<Monster>&  pMonster, bool offline=false);
bool loadMonsterFromFile(const CatRef& cr, std::shared_ptr<Monster>& pMonster, std::string filename="", bool offline=false);
bool loadObject(int index, std::shared_ptr<Object>&  pObject, bool offline=false);
bool loadObject(const CatRef& cr, std::shared_ptr<Object>&  pObject, bool offline=false);
bool loadObjectFromFile(const CatRef& cr, std::shared_ptr<Object>&  pObject, bool offline=false);
const Object* getCachedObject(const CatRef& cr);
bool loadRoom(int index, std::shared_ptr<UniqueRoom>& pRoom, bool offline=false);
bool loadRoom(const CatRef& cr, std::shared_ptr<UniqueRoom> &pRoom, bool offline=false);
bool loadRoomFromFile(const CatRef& cr, std::shared_ptr<UniqueRoom> &pRoom, std::string filename="", bool offline=false);

bool loadPlayer(std::string_view name, std::shared_ptr<Player>& player, enum LoadType loadType=LoadType::LS_NORMAL);

void loadCarryArray(xmlNodePtr curNode, Carry array[], const char* name, int maxProp);
void loadCatRefArray(xmlNodePtr curNode, std::map<int, CatRef>& array, const char* name, int maxProp);
void loadCatRefArray(xmlNodePtr curNode, CatRef array[], const char* name, int maxProp);
void loadStringArray(xmlNodePtr curNode, void* array, int size, const char* name, int maxProp);
void loadBitset(xmlNodePtr curNode, boost::dynamic_bitset<>& bits);
void loadDaily(xmlNodePtr curNode, struct daily* pDaily);
void loadDailys(xmlNodePtr curNode, struct daily* pDailys);
void loadCrLastTime(xmlNodePtr curNode, CRLastTime* pCrLastTime);
void loadCrLastTimes(xmlNodePtr curNode, std::map<int, CRLastTime>& pCrLastTimes);
void loadLastTime(xmlNodePtr curNode, LastTime* pLastTime);
void loadLastTimes(xmlNodePtr curNode, LastTime* pLastTimes);
void loadSavingThrow(xmlNodePtr curNode, struct saves* pSavingThrow);
void loadSavingThrows(xmlNodePtr curNode, struct saves* pSavingThrows);
void loadRanges(xmlNodePtr curNode, Player* player);


//**********************
//  Save Section
//**********************
int saveObjectsXml(xmlNodePtr parentNode, const ObjectSet& set, int permOnly);
int saveCreaturesXml(xmlNodePtr parentNode, const MonsterSet& set, int permOnly);

GuildCreation* parseGuildCreation(xmlNodePtr cur);

xmlNodePtr saveDaily(xmlNodePtr parentNode, int i, struct daily pDaily);
xmlNodePtr saveCrLastTime(xmlNodePtr parentNode, int i, const CRLastTime& pCrLastTime);
xmlNodePtr saveLastTime(xmlNodePtr parentNode, int i, LastTime pLastTime);
xmlNodePtr saveSavingThrow(xmlNodePtr parentNode, int i, struct saves pSavingThrow);
xmlNodePtr saveBitset(xmlNodePtr parentNode, const char* name, int maxBit, const boost::dynamic_bitset<>& bits);
xmlNodePtr saveBit(xmlNodePtr parentNode, int bit);
xmlNodePtr saveLongArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const long array[], int arraySize);
xmlNodePtr saveULongArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const unsigned long array[], int arraySize);
xmlNodePtr saveCatRefArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const std::map<int, CatRef>& array, int arraySize);
xmlNodePtr saveCarryArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const Carry array[], int arraySize);
xmlNodePtr saveCatRefArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const CatRef array[], int arraySize);
xmlNodePtr saveShortIntArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const short array[], int arraySize);


// xml.cpp
int toBoolean(char *fromStr);
char *iToYesNo(int fromInt);

