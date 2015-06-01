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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef XML_H_
#define XML_H_

#include <map>

#include "boost/lexical_cast.hpp"
using boost::lexical_cast;
using boost::bad_lexical_cast;

class Anchor;
class Guild;
class Ban;
class GuildCreation;
class Monster;
class Player;
class BaseRoom;
class Stat;
class Object;
class Room;
class Creature;

//**********************
//  Defines Section
//**********************

#define OBJ     1
#define PLY     2
#define CRT     3
#define ROOM    4

#define BIT_ISSET(p,f)    ((p)[(f)/8] & 1<<((f)%8))
#define BIT_SET(p,f)      ((p)[(f)/8] |= 1<<((f)%8))
#define BIT_CLEAR(p,f)      ((p)[(f)/8] &= ~(1<<((f)%8)))

#define NODE_NAME(pNode, pName)         (!strcmp((char *)(pNode)->name, (pName) ))

namespace xml {
    // copyToBString - will make store the string into a temp cstr, set the string
    // and then free the temp cstr
    void copyToBString(bstring &to, xmlNodePtr node);

    bstring getBString(xmlNodePtr node);

    // getProp -- You MUST free the return value
    bstring getProp(xmlNodePtr node, const char *name);

    // getIntProp -- Properly frees the return valueusing toInt
    int getIntProp(xmlNodePtr node, const char *name);

    // getCString -- You MUST free the return value
    char* getCString(xmlNodePtr node);

    //***************************************************************************************
    // XML_COPY_* - These functions will copy the given type to the variable 'to'
    //***************************************************************************************

    // copyPropToCString -- This will properly free the return value using doStrCpy
    void copyPropToCString(char* to, xmlNodePtr node, bstring name);
    //#define copyPropToCString(to, node, name) doStrCpy( (to) , getProp( (node) , (name) ))

    // copyPropToBString -- This will properly free the value
    void copyPropToBString(bstring& to, xmlNodePtr node, bstring name);

    // copyToCString -- Properly frees the return value using doStrCpy
    void copyToCString(char* to, xmlNodePtr node);
    //#define copyToCString(to, node)   doStrCpy( (to), getCString( (node) ))

    // XML_DUP_STRING_CHILD -- Will return strdup of a string -- you MUST free it
    void dupeToCString(char* to, xmlNodePtr node);

        // copyToBool -- Properly frees the return value using toBoolean
    void copyToBool(bool& to, xmlNodePtr node);

    // copyToColor -- Properly frees the return value using toColor
    void copyToColor(Color& to, xmlNodePtr node);


    //*******************************************************************************
    // XML_NEW_* will always save (even if blank or 0) and will still let you
    // get the return value from xmlNewChild
    // ###  IMPORTANT  ### -- Name and Value must NOT include any bad xml characters
    // like &, <, >, etc.
    //********************************************************************************

    xmlAttrPtr newProp(xmlNodePtr node, bstring name, const bstring value);

    template <class Type>
    bstring numToStr(const Type& value) {
        std::ostringstream str;
        str << value;
        return(str.str());
    }

    template <class Type>
    xmlAttrPtr newNumProp(xmlNodePtr node, bstring name, const Type& value) {
        return( xmlNewProp( node, BAD_CAST (name.c_str()), BAD_CAST(numToStr(value).c_str())));
    }

    template <class Type>
    xmlNodePtr newNumChild(xmlNodePtr node, bstring name, const Type& value) {
        return(xmlNewChild( node, NULL, BAD_CAST (name.c_str()), BAD_CAST numToStr(value).c_str()));
    }

    template <class Type>
    xmlNodePtr saveNonZeroNum(xmlNodePtr node, bstring name, const Type& value) {
        xmlNodePtr toReturn = NULL;
        // precision for floats
        if((int)(value*10000)) {
            toReturn = xmlNewChild( (node), NULL, BAD_CAST (name.c_str()), BAD_CAST numToStr(value).c_str());
        }
        return(toReturn);
    }

    template <class Type>
    Type toNum(char *fromStr) {
        Type toReturn = 0;
        if(!fromStr)
            return(toReturn);

        try {
            toReturn = lexical_cast<Type>(fromStr);
        } catch (bad_lexical_cast &) {
            toReturn = 0;

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

    template <class Type>
    void loadNumArray(xmlNodePtr curNode, Type array[], const char* nodeName, int maxProp) {
        xmlNodePtr childNode = curNode->children;
        int i;
#ifndef PYTHON_CODE_GEN
        while(childNode) {
            if(NODE_NAME(childNode , nodeName)) {
                i = xml::getIntProp(childNode, "Num");
                if(i < maxProp)
                    array[i] = xml::toNum<Type>(childNode);
            }
            childNode = childNode->next;
        }
#endif
    }

    xmlNodePtr newBoolChild(xmlNodePtr node, const bstring name, const bool value);
    xmlNodePtr newStringChild(xmlNodePtr node, const bstring name, const bstring value = "");

    //***************************************************************************************
    // saveNonZero/Null will only save if it is non 0 and also escape any bad XML characters
    //***************************************************************************************

    xmlNodePtr saveNonNullString(xmlNodePtr node, bstring name, const bstring value);

    xmlChar* ConvertInput(const char *in, const char *encoding);
    char *doStrCpy(char *dest, char *src);
    char *doStrDup(char *src);
    xmlDocPtr loadFile(const char *filename, const char *expectedRoot);
    int saveFile(const char * filename, xmlDocPtr cur);

} // End xml namespace



//******************
//  Load Section
//******************

bool loadMonster(int index, Monster** pMonster);
bool loadMonster(const CatRef cr, Monster** pMonster);
bool loadMonsterFromFile(const CatRef cr, Monster **pMonster, bstring filename="");
bool loadObject(int index, Object** pObject);
bool loadObject(const CatRef cr, Object** pObject);
bool loadObjectFromFile(const CatRef cr, Object** pObject);
bool loadRoom(int index, UniqueRoom **pRoom);
bool loadRoom(const CatRef cr, UniqueRoom **pRoom);
bool loadRoomFromFile(const CatRef cr, UniqueRoom **pRoom, bstring filename="");

bool loadPlayer(const bstring name, Player** player, LoadType loadType=LS_NORMAL);

void loadCarryArray(xmlNodePtr curNode, Carry array[], const char* name, int maxProp);
void loadCatRefArray(xmlNodePtr curNode, std::map<int, CatRef>& array, const char* name, int maxProp);
void loadCatRefArray(xmlNodePtr curNode, CatRef array[], const char* name, int maxProp);
void loadStringArray(xmlNodePtr curNode, void* array, int size, const char* name, int maxProp);
void loadBits(xmlNodePtr curNode, char *bits);
void loadDaily(xmlNodePtr curNode, struct daily* pDaily);
void loadDailys(xmlNodePtr curNode, struct daily* pDailys);
void loadCrLastTime(xmlNodePtr curNode, struct crlasttime* pCrLastTime);
void loadCrLastTimes(xmlNodePtr curNode, std::map<int, crlasttime>& pCrLastTimes);
void loadLastTime(xmlNodePtr curNode, struct lasttime* pLastTime);
void loadLastTimes(xmlNodePtr curNode, struct lasttime* pLastTimes);
void loadSavingThrow(xmlNodePtr curNode, struct saves* pSavingThrow);
void loadSavingThrows(xmlNodePtr curNode, struct saves* pSavingThrows);
void loadRanges(xmlNodePtr curNode, Player *pPlayer);


//**********************
//  Save Section
//**********************
//int saveRoomToFile(Room* pRoom, int permOnly);
//int saveCreature(Creature * pCreature);
//int savePlayerToFile(Creature * player);
//int saveObject(Object* pObject);

//int saveObjectsXml(xmlNodePtr parentNode, const std::set<Object*, ObjectPtrLess>& set, int permOnly);
int saveObjectsXml(xmlNodePtr parentNode, const ObjectSet& set, int permOnly);
int saveCreaturesXml(xmlNodePtr parentNode, const MonsterSet& set, int permOnly);
//int saveCreatureXml(xmlNodePtr rootNode, Creature * pCreature, int permOnly, LoadType saveType);

Ban *parseBan(xmlNodePtr cur);

Guild* parseGuild(xmlNodePtr cur);
GuildCreation* parseGuildCreation(xmlNodePtr cur);
void parseGuildMembers(xmlNodePtr cur, int guildId);

xmlNodePtr saveDaily(xmlNodePtr parentNode, int i, struct daily pDaily);
xmlNodePtr saveCrLastTime(xmlNodePtr parentNode, int i, struct crlasttime pCrLastTime);
xmlNodePtr saveLastTime(xmlNodePtr parentNode, int i, struct lasttime pLastTime);
xmlNodePtr saveSavingThrow(xmlNodePtr parentNode, int i, struct saves pSavingThrow);
xmlNodePtr saveAnchor(xmlNodePtr parentNode, int i, Anchor* pAnchor);
xmlNodePtr saveBits(xmlNodePtr parentNode, const char* name, int maxBit, const char *bits);
xmlNodePtr saveBit(xmlNodePtr parentNode, int bit);
xmlNodePtr saveLongArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const long array[], int arraySize);
xmlNodePtr saveULongArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const unsigned long array[], int arraySize);
xmlNodePtr saveCatRefArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const std::map<int, CatRef>& array, int arraySize);
xmlNodePtr saveCarryArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const Carry array[], int arraySize);
xmlNodePtr saveCatRefArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const CatRef array[], int arraySize);
xmlNodePtr saveShortIntArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const short array[], int arraySize);



int saveFactions(xmlNodePtr rootNode, Creature* pCreature);
int saveSkills(xmlNodePtr rootNode, Creature* pCreature);
#endif /*XML_H_*/



