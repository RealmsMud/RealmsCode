/*
 * objects-xml.cpp
 *   Objects xml
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

#include <libxml/parser.h>                          // for xmlNodePtr, xmlNode
#include <cstdio>                                   // for sprintf
#include <cstring>                                  // for strcpy
#include <ostream>                                  // for basic_ostream::op...
#include <stdexcept>                                // for runtime_error

#include "alchemy.hpp"                              // for AlchemyEffect
#include "area.hpp"                                 // for MapMarker
#include "bstring.hpp"                              // for bstring, operator+
#include "catRef.hpp"                               // for CatRef
#include "config.hpp"                               // for Config, gConfig
#include "container.hpp"                            // for ObjectSet
#include "creatures.hpp"                            // for Creature
#include "enums/loadType.hpp"                       // for LoadType, LoadTyp...
#include "flags.hpp"                                // for O_UNIQUE, O_PERM_...
#include "global.hpp"                               // for ALLITEMS, FATAL
#include "hooks.hpp"                                // for Hooks
#include "mudObject.hpp"                            // for MudObject
#include "objIncrease.hpp"                          // for ObjIncrease
#include "objects.hpp"                              // for Object, DroppedBy
#include "os.hpp"                                   // for ASSERTLOG, merror
#include "paths.hpp"                                // for checkDirExists
#include "proto.hpp"                                // for objectPath, valid...
#include "rooms.hpp"                                // for UniqueRoom
#include "server.hpp"                               // for Server, gServer
#include "xml.hpp"                                  // for saveNonZeroNum


// Object flags to be saved for object refs
int objRefSaveFlags[] =
        {
                O_PERM_ITEM,
                O_HIDDEN,
                O_CURSED,
                O_WORN,
                O_TEMP_ENCHANT,
                O_WAS_SHOPLIFTED,
                O_ENVENOMED,
                O_JUST_BOUGHT,
                O_NO_DROP,
                O_BROKEN_BY_CMD,
                O_BEING_PREPARED,
                O_UNIQUE,
                O_KEEP,
                O_DARKNESS,
                O_RECLAIMED,
                -1
        };


//*********************************************************************
//                      loadObject
//*********************************************************************

bool loadObject(int index, Object** pObject, bool offline) {
    CatRef cr;
    cr.id = index;
    return(loadObject(cr, pObject, offline));
}

bool loadObject(const CatRef& cr, Object** pObject, bool offline) {
    if(!validObjId(cr))
        return(false);

    // Check if object is already loaded, and if so return pointer
    if(gServer->objectCache.contains(cr)) {
        *pObject = new Object;
        gServer->objectCache.fetch(cr, pObject, false);
    } else {
        // Otherwise load the object and return a pointer to the newly loaded object
        // Load the object from it's file
        if(!loadObjectFromFile(cr, pObject, offline))
            return(false);
        gServer->objectCache.insert(cr, pObject);
    }

    if(pObject) {
        // Quest items are now auto NO-DROP
        if((*pObject)->getQuestnum()) {
            (*pObject)->setFlag(O_NO_DROP);
        }
        // cannot steal scrolls
        if((*pObject)->getType() == ObjectType::SCROLL) {
            (*pObject)->setFlag(O_NO_STEAL);
        }
    }
    return(true);
}

//*********************************************************************
//                      loadObjectFromFile
//*********************************************************************

bool loadObjectFromFile(const CatRef& cr, Object** pObject, bool offline) {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    int         num;
    char        filename[256];

    sprintf(filename, "%s", objectPath(cr));

    if((xmlDoc = xml::loadFile(filename, "Object")) == nullptr)
        return(false);

    if(xmlDoc == nullptr) {
        std::clog << "Error parsing file " << filename << std::endl;
        return(false);
    }
    rootNode = xmlDocGetRootElement(xmlDoc);
    num = xml::getIntProp(rootNode, "Num");

    if(num == cr.id) {
        // BINGO: This is the object we want, read it in
        *pObject = new Object;
        if(!*pObject)
            merror("loadObjectFile", FATAL);
        xml::copyPropToBString((*pObject)->version, rootNode, "Version");

        (*pObject)->readFromXml(rootNode, nullptr, offline);
        (*pObject)->setId("-1");
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

//*********************************************************************
//                      readFromXml
//*********************************************************************
// Reads an object from the given xml document and root node

int Object::readFromXml(xmlNodePtr rootNode, std::list<bstring> *idList, bool offline) {
    xmlNodePtr curNode, childNode;
    info.load(rootNode);
    info.id = xml::getIntProp(rootNode, "Num");
    xml::copyPropToBString(version, rootNode, "Version");
    curNode = rootNode->children;
    // Start reading stuff in!

    while(curNode) {
        if(NODE_NAME(curNode, "Name")) setName(xml::getBString(curNode));
        else if(NODE_NAME(curNode, "Id")) setId(xml::getBString(curNode));
        else if(NODE_NAME(curNode, "IdList") && idList != nullptr) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Id")) {
                    idList->push_back(xml::getBString(childNode));
                }
                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "Plural")) xml::copyToBString(plural, curNode);
        else if(NODE_NAME(curNode, "DroppedBy")) droppedBy.load(curNode);
        else if(NODE_NAME(curNode, "Description")) xml::copyToBString(description, curNode);
        else if(NODE_NAME(curNode, "LotteryNumbers")) {
            xml::loadNumArray<short>(curNode, lotteryNumbers, "LotteryNum", 6);
        }
        else if(NODE_NAME(curNode, "UseOutput")) xml::copyToCString(use_output, curNode);
        else if(NODE_NAME(curNode, "UseAttack")) xml::copyToCString(use_attack, curNode);
        else if(NODE_NAME(curNode, "LastMod")) xml::copyToBString(lastMod, curNode);
        else if(NODE_NAME(curNode, "Keys")) {
            loadStringArray(curNode, key, OBJ_KEY_LENGTH, "Key", 3);
        }
        else if(NODE_NAME(curNode, "Weight")) xml::copyToNum(weight, curNode);
        else if(NODE_NAME(curNode, "Type")) xml::copyToNum<ObjectType, short>(type, curNode);
        else if(NODE_NAME(curNode, "SubType")) xml::copyToBString(subType, curNode);
        else if(NODE_NAME(curNode, "Adjustment")) setAdjustment(xml::toNum<short int>(curNode));
        else if(NODE_NAME(curNode, "ShotsMax")) xml::copyToNum(shotsMax, curNode);
        else if(NODE_NAME(curNode, "ShotsCur")) xml::copyToNum(shotsCur, curNode);
        else if(NODE_NAME(curNode, "ChargesMax")) xml::copyToNum(chargesMax, curNode);
        else if(NODE_NAME(curNode, "ChargesCur")) xml::copyToNum(chargesCur, curNode);
        else if(NODE_NAME(curNode, "Armor")) xml::copyToNum(armor, curNode);
        else if(NODE_NAME(curNode, "WearFlag")) xml::copyToNum(wearflag, curNode);
        else if(NODE_NAME(curNode, "MagicPower")) xml::copyToNum(magicpower, curNode);
        else if(NODE_NAME(curNode, "Effect")) xml::copyToBString(effect, curNode);
        else if(NODE_NAME(curNode, "EffectDuration")) xml::copyToNum(effectDuration, curNode);
        else if(NODE_NAME(curNode, "EffectStrength")) xml::copyToNum(effectStrength, curNode);
        else if(NODE_NAME(curNode, "Level")) xml::copyToNum(level, curNode);
        else if(NODE_NAME(curNode, "Quality")) xml::copyToNum(quality, curNode);
        else if(NODE_NAME(curNode, "RequiredSkill")) xml::copyToNum(requiredSkill, curNode);
        else if(NODE_NAME(curNode, "Clan")) xml::copyToNum(clan, curNode);
        else if(NODE_NAME(curNode, "Special")) xml::copyToNum(special, curNode);
        else if(NODE_NAME(curNode, "QuestNum")) xml::copyToNum(questnum, curNode);
        else if(NODE_NAME(curNode, "Bulk")) xml::copyToNum(bulk, curNode);
        else if(NODE_NAME(curNode, "Size")) size = whatSize(xml::toNum<int>(curNode));
        else if(NODE_NAME(curNode, "MaxBulk")) xml::copyToNum(maxbulk, curNode);
        else if(NODE_NAME(curNode, "LotteryCycle")) xml::copyToNum(lotteryCycle, curNode);
        else if(NODE_NAME(curNode, "CoinCost")) coinCost = xml::toNum<unsigned long>(curNode);

        else if(NODE_NAME(curNode, "Deed")) deed.load(curNode);

        else if(NODE_NAME(curNode, "ShopValue")) setShopValue(xml::toNum<unsigned long>(curNode));
        else if(NODE_NAME(curNode, "Made")) xml::copyToNum(made, curNode);
        else if(NODE_NAME(curNode, "KeyVal")) xml::copyToNum(keyVal, curNode);
        else if(NODE_NAME(curNode, "Material")) material = (Material)xml::toNum<int>(curNode);
        else if(NODE_NAME(curNode, "MinStrength")) xml::copyToNum(minStrength, curNode);
        else if(NODE_NAME(curNode, "NumAttacks")) xml::copyToNum(numAttacks, curNode);
        else if(NODE_NAME(curNode, "Delay")) xml::copyToNum(delay, curNode);
        else if(NODE_NAME(curNode, "Extra")) xml::copyToNum(extra, curNode);
        else if(NODE_NAME(curNode, "Recipe")) xml::copyToNum(recipe, curNode);
        else if(NODE_NAME(curNode, "Value")) value.load(curNode);
        else if(NODE_NAME(curNode, "InBag")) {
            loadCatRefArray(curNode, in_bag, "Obj", 3);
        }
        else if(NODE_NAME(curNode, "Dice")) damage.load(curNode);
        else if(NODE_NAME(curNode, "Flags")) {
            loadBits(curNode, flags);
        }
        else if(NODE_NAME(curNode, "LastTimes")) {
            loadLastTimes(curNode, lasttime);
        }
        else if(NODE_NAME(curNode, "SubItems")) {
            readObjects(curNode, offline);
        }
        else if(NODE_NAME(curNode, "Compass")) {
            if(!compass)
                compass = new MapMarker;
            compass->load(curNode);
        }
        else if(NODE_NAME(curNode, "ObjIncrease")) {
            if(!increase)
                increase = new ObjIncrease;
            increase->load(curNode);
            // it wasnt good after all
            if(!increase->isValid()) {
                delete increase;
                increase = nullptr;
            }
        }
        else if(NODE_NAME(curNode, "AlchemyEffects")) {
            loadAlchemyEffects(curNode);
        }
        else if(NODE_NAME(curNode, "Hooks")) hooks.load(curNode);
        else if(NODE_NAME(curNode, "RandomObjects")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "RandomObject")) {
                    CatRef cr;
                    cr.load(childNode);
                    randomObjects.push_back(cr);
                }
                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "Owner")) xml::copyToBString(questOwner, curNode);

            // Now handle version changes

        else if(getVersion() < "2.41" ) {

            if(NODE_NAME(curNode, "DeedLow")) deed.low.load(curNode);
            else if(NODE_NAME(curNode, "DeedHigh")) xml::copyToNum(deed.high, curNode);

        } else if(getVersion() < "2.42b") {
            if(NODE_NAME(curNode, "SpecialThree")) xml::copyToNum(effectDuration, curNode);
        }

        curNode = curNode->next;
    }

    if(version < "2.47c" && flagIsSet(O_WEAPON_CASTS)) {
        // Version 2.46j added charges for casting weapons, versions before that
        // used shots, items until 2.47c were bugged due to charges not being
        // copied in doCopy()  Initialize charges as 1/3rd of shots
        chargesMax = shotsMax / 3;
        chargesCur = shotsCur /3;
    }

    if(version < "2.47b" && flagIsSet(O_OLD_INVISIBLE)) {
        addEffect("invisibility", -1);
        clearFlag(O_OLD_INVISIBLE);
    }
    // make sure uniqueness stays intact
    setFlag(O_UNIQUE);
    if(!gConfig->getUnique(this))
        clearFlag(O_UNIQUE);

    escapeText();
    return(0);
}


//*********************************************************************
//                      readObjects
//*********************************************************************
// Reads in objects and objrefs and adds them to the parent

void MudObject::readObjects(xmlNodePtr curNode, bool offline) {
    xmlNodePtr childNode = curNode->children;
    Object* object=nullptr, *object2=nullptr;
    CatRef  cr;
    int     i=0,quantity=0;

    Creature* cParent = getAsCreature();
//  Monster* mParent = parent->getMonster();
//  Player* pParent = parent->getPlayer();
    UniqueRoom* rParent = getAsUniqueRoom();
    Object* oParent = getAsObject();
    std::list<bstring> *idList = nullptr;

    while(childNode) {
        object = nullptr;
        object2 = nullptr;
        quantity = xml::getIntProp(childNode, "Quantity");
        if(quantity < 1)
            quantity = 1;
        if(quantity > 1)
            idList = new std::list<bstring>;
        else
            idList = nullptr;
        try {
            if(NODE_NAME(childNode, "Object")) {
                // If it's a full object, read it in
                object = new Object;
                if(!object)
                    merror("loadObjectsXml", FATAL);
                object->readFromXml(childNode, idList);
            } else if(NODE_NAME(childNode, "ObjRef")) {
                // If it's an object reference, we want to first load the parent object
                // and then use readObjectXml to update any fields that may have changed
                cr.load(childNode);
                cr.id = xml::getIntProp(childNode, "Num");
                if(!validObjId(cr)) {
                    std::clog <<  "Invalid object " << cr.str() << std::endl;
                } else {
                    if(loadObject(cr, &object)) {
                        // These two flags might be cleared on the reference object, so let that object set them if it wants to
                        object->clearFlag(O_HIDDEN);
                        object->clearFlag(O_CURSED);
                        object->readFromXml(childNode, idList, offline);
                    }
                }
            }
            if(object) {
                std::list<bstring>::iterator idIt;
                if(quantity > 1)
                    idIt = idList->begin();

                for(i=0; i<quantity; i++) {
                    if(!i) {
                        object2 = object;  // just a reference
                    } else {
                        object2 = new Object;
                        *object2 = *object;
                    }
                    try {
                        if(quantity > 1 && idIt != idList->end())
                            object2->setId(*idIt++);
                        // Add it to the appropriate parent
                        if(oParent) {
                            oParent->addObj(object2, false);
                        } else if(cParent) {
                            cParent->addObj(object2);
                        } else if(rParent) {
                            object2->addToRoom(rParent);
                        }
                    } catch(std::runtime_error &e) {
                        std::clog << "Error setting ID: " << e.what() << std::endl;
                        if(object2) {
                            delete object2;
                            object2 = nullptr;
                        }
                    }
                }
            }
        } catch(std::runtime_error &e) {
            std::clog << "Error loading object: " << e.what() << std::endl;
            if(object != nullptr) {
                delete object;
                object = nullptr;
            }
        }
        if(idList) {
            delete idList;
            idList = nullptr;
        }
        childNode = childNode->next;
    }
}

void DroppedBy::load(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
        else if(NODE_NAME(curNode, "Index")) xml::copyToBString(index, curNode);
        else if(NODE_NAME(curNode, "ID")) xml::copyToBString(id, curNode);
        else if(NODE_NAME(curNode, "Type")) xml::copyToBString(type, curNode);

        curNode = curNode->next;
    }
}

xmlNodePtr saveObjRefFlags(xmlNodePtr parentNode, const char* name, int maxBit, const char *bits);

//*********************************************************************
//                      saveObject
//*********************************************************************
// This function will write the supplied object to the proper
// o##.xml file - To accomplish this it parse the proper o##.xml file if
// available and walk down the tree to the proper place in numerical order
// if the file does not exist it will create it with only that object in it.
// It will most likely only be called from *save o

int Object::saveToFile() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char        filename[256];

    ASSERTLOG( this );

    // Invalid Number
    if(info.id < 0)
        return(-1);
    Path::checkDirExists(info.area, objectPath);

    gServer->saveIds();

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Object", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    // make sure uniqueness stays intact
    setFlag(O_UNIQUE);
    if(!gConfig->getUnique(this))
        clearFlag(O_UNIQUE);

    escapeText();
    bstring idTemp = id;
    id = "-1";
    saveToXml(rootNode, ALLITEMS, LoadType::LS_PROTOTYPE, false);
    id = idTemp;

    strcpy(filename, objectPath(info));
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
    return(0);
}


//*********************************************************************
//                      saveToXml
//*********************************************************************
// This function will save the entire object except 0 values to the
// given node which should be an object node unless saveType is REF
// in which case it will only save fields that are changed in the
// ordinary course of the game

int Object::saveToXml(xmlNodePtr rootNode, int permOnly, LoadType saveType, int quantity, bool saveId, std::list<bstring> *idList) const {
//  xmlNodePtr  rootNode;
    xmlNodePtr      curNode;
    xmlNodePtr      childNode;

    int i=0;

    if(rootNode == nullptr)
        return(-1);

    ASSERTLOG( info.id >= 0 );
    ASSERTLOG( info.id < OMAX );

    // If the object's index is 0, then we have to do a full save
    if(!info.id && saveType != LoadType::LS_FULL && saveType != LoadType::LS_PROTOTYPE) {
        // We should never get here...if it's a 0 index, should have O_SAVE_FULL set
        std::clog << "ERROR: Forcing full save.\n";
        saveType = LoadType::LS_FULL;
    }

    // record objects saved during swap
    if(gConfig->swapIsInteresting(this))
        gConfig->swapLog((bstring)"o" + info.rstr(), false);

    xml::newNumProp(rootNode, "Num", info.id);
    xml::newProp(rootNode, "Area", info.area);
    xml::newProp(rootNode, "Version", gConfig->getVersion());
    if(quantity > 1) {
        xml::newNumProp(rootNode, "Quantity", quantity);
        curNode = xml::newStringChild(rootNode, "IdList");
        if(idList != nullptr) {
            auto idIt = idList->begin();
            while(idIt != idList->end()) {
                xml::newStringChild(curNode, "Id", (*idIt++));
            }
        }
    } else {
        if(saveId)
            xml::saveNonNullString(rootNode, "Id", getId());
        else
            xml::newProp(rootNode, "Id", "-1");
    }

    // These are saved for full and reference
    xml::saveNonNullString(rootNode, "Name", getName());
    if(saveType != LoadType::LS_PROTOTYPE) {
        droppedBy.save(rootNode);
        xml::saveNonZeroNum(rootNode, "ShopValue", shopValue);
    }


    xml::saveNonNullString(rootNode, "Plural", plural);
    xml::saveNonZeroNum(rootNode, "Adjustment", adjustment);
    xml::saveNonZeroNum(rootNode, "ShotsMax", shotsMax);
    xml::saveNonZeroNum(rootNode, "ChargesMax", chargesMax);
    // NOTE: This is saved even if zero so that it loads broken items
    // properly.
    xml::newNumChild(rootNode, "ShotsCur", shotsCur);
    xml::newNumChild(rootNode, "ChargesCur", chargesCur);
    xml::saveNonZeroNum(rootNode, "Armor", armor);
    xml::saveNonZeroNum(rootNode, "NumAttacks", numAttacks);
    xml::saveNonZeroNum(rootNode, "Delay", delay);
    xml::saveNonZeroNum(rootNode, "Extra", extra);
    xml::saveNonZeroNum(rootNode, "LotteryCycle", lotteryCycle);

    if(type == ObjectType::LOTTERYTICKET) {
        // Lottery tickets have a custom description so be sure to save it
        xml::saveNonNullString(rootNode, "Description", description);
        saveShortIntArray(rootNode, "LotteryNumbers", "LotteryNum", lotteryNumbers, 6);
    }

    damage.save(rootNode, "Dice");
    value.save("Value", rootNode);
    hooks.save(rootNode, "Hooks");

    // Save the keys in case they were modified
    // Keys
    curNode = xml::newStringChild(rootNode, "Keys");
    for(i=0; i<3; i++) {
        if(key[i][0] == 0)
            continue;
        childNode = xml::newStringChild(curNode, "Key", key[i]);
        xml::newNumProp(childNode, "Num", i);
    }

    for(i=0; i<4; i++) {
        // this nested loop means we won't create an xml node if we don't have to
        if(lasttime[i].interval || lasttime[i].ltime || lasttime[i].misc) {
            curNode = xml::newStringChild(rootNode, "LastTimes");
            for(; i<4; i++)
                saveLastTime(curNode, i, lasttime[i]);
        }
    }

    xml::saveNonNullString(rootNode, "Effect", effect);
    xml::saveNonZeroNum(rootNode, "EffectDuration", effectDuration);
    xml::saveNonZeroNum(rootNode, "EffectStrength", effectStrength);

    xml::saveNonZeroNum(rootNode, "Recipe", recipe);
    xml::saveNonZeroNum(rootNode, "Made", made);
    xml::saveNonNullString(rootNode, "Owner", questOwner);

    if(saveType != LoadType::LS_FULL && saveType != LoadType::LS_PROTOTYPE) {
        // Save only a subset of flags for obj references
        saveObjRefFlags(rootNode, "Flags", MAX_OBJECT_FLAGS, flags);
    }

    if(saveType == LoadType::LS_FULL || saveType == LoadType::LS_PROTOTYPE) {
        saveBits(rootNode, "Flags", MAX_OBJECT_FLAGS, flags);
        // These are only saved for full objects
        if(type != ObjectType::LOTTERYTICKET)
            xml::saveNonNullString(rootNode, "Description", description);


        if(!alchemyEffects.empty()) {
            curNode = xml::newStringChild(rootNode, "AlchemyEffects");

            for(std::pair<int, AlchemyEffect> p : alchemyEffects) {
                childNode = xml::newStringChild(curNode, "AlchemyEffect");
                p.second.saveToXml(childNode);
                xml::newNumProp(childNode, "Num", p.first);
            }
        }
        xml::saveNonNullString(rootNode, "UseOutput", use_output);
        xml::saveNonNullString(rootNode, "UseAttack", use_attack);
        xml::saveNonNullString(rootNode, "LastMod", lastMod);

        xml::saveNonZeroNum(rootNode, "Weight", weight);
        xml::saveNonZeroNum(rootNode, "Type", static_cast<int>(type));
        xml::saveNonNullString(rootNode, "SubType", subType);

        xml::saveNonZeroNum(rootNode, "WearFlag", wearflag);
        xml::saveNonZeroNum(rootNode, "MagicPower", magicpower);

        xml::saveNonZeroNum(rootNode, "Level", level);
        xml::saveNonZeroNum(rootNode, "Quality", quality);
        xml::saveNonZeroNum(rootNode, "RequiredSkill", requiredSkill);
        xml::saveNonZeroNum(rootNode, "Clan", clan);
        xml::saveNonZeroNum(rootNode, "Special", special);
        xml::saveNonZeroNum(rootNode, "QuestNum", questnum);

        xml::saveNonZeroNum(rootNode, "Bulk", bulk);
        xml::saveNonZeroNum(rootNode, "Size", size);
        xml::saveNonZeroNum(rootNode, "MaxBulk", maxbulk);

        xml::saveNonZeroNum(rootNode, "CoinCost", coinCost);

        deed.save(rootNode, "Deed", false);

        xml::saveNonZeroNum(rootNode, "KeyVal", keyVal);
        xml::saveNonZeroNum(rootNode, "Material", (int)material);
        xml::saveNonZeroNum(rootNode, "MinStrength", minStrength);

        saveCatRefArray(rootNode, "InBag", "Obj", in_bag, 3);

        std::list<CatRef>::const_iterator it;
        if(!randomObjects.empty()) {
            curNode = xml::newStringChild(rootNode, "RandomObjects");
            for(it = randomObjects.begin() ; it != randomObjects.end() ; it++) {
                (*it).save(curNode, "RandomObject", false);
            }
        }

    }

    if(compass) {
        curNode = xml::newStringChild(rootNode, "Compass");
        compass->save(curNode);
    }
    if(increase) {
        curNode = xml::newStringChild(rootNode, "ObjIncrease");
        increase->save(curNode);
    }

    // Save contained items for both full and reference saves
    // Also check for container flag
    if(saveType != LoadType::LS_PROTOTYPE && !objects.empty()) {
        curNode = xml::newStringChild(rootNode, "SubItems");
        // If we're a permenant container, always save the items inside of it
        if(type == ObjectType::CONTAINER && flagIsSet(O_PERM_ITEM))
            permOnly = ALLITEMS;
        saveObjectsXml(curNode, objects, permOnly);
    }
    return(0);
}


//*********************************************************************
//                      saveObjectsXml
//*********************************************************************

int saveObjectsXml(xmlNodePtr parentNode, const ObjectSet &set, int permOnly) {
    xmlNodePtr curNode;
    int quantity = 0;
    LoadType lt;
    ObjectSet::const_iterator it;
    const Object *obj;
    std::list<bstring> *idList = nullptr;
    for (it = set.begin(); it != set.end();) {
        obj = (*it++);
        if (obj &&
            (permOnly == ALLITEMS ||
             (permOnly == PERMONLY && obj->flagIsSet(O_PERM_ITEM))
            )) {
            if (obj->flagIsSet(O_CUSTOM_OBJ) || obj->flagIsSet(O_SAVE_FULL) || !obj->info.id) {
                // If it's a custom or has the save flag set, save the entire object
                curNode = xml::newStringChild(parentNode, "Object");
                lt = LoadType::LS_FULL;
            } else {
                // Just save a reference and any changed fields
                curNode = xml::newStringChild(parentNode, "ObjRef");
                lt = LoadType::LS_REF;
            }

            // TODO: Modify this to work with object ids
            // quantity code reduces filesize for player shops, storage rooms, and
            // inventories (which tend of have a lot of identical items in them)
            quantity = 1;
            idList = new std::list<bstring>;
            idList->push_back(obj->getId());
            while (it != set.end() && *(*it) == *obj) {
                idList->push_back((*it)->getId());
                quantity++;
                it++;
            }

            obj->saveToXml(curNode, permOnly, lt, quantity, true, idList);
            delete idList;
        }
    }
    return (0);
}


void DroppedBy::save(xmlNodePtr rootNode) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "DroppedBy");
    xml::newStringChild(curNode, "Name", name);
    xml::saveNonNullString(curNode, "Index", index);
    xml::saveNonNullString(curNode, "ID", id);
    xml::newStringChild(curNode, "Type", type);
}

//*********************************************************************
//                      saveObjRefFlags
//*********************************************************************

xmlNodePtr saveObjRefFlags(xmlNodePtr parentNode, const char* name, int maxBit, const char *bits) {
    xmlNodePtr curNode=nullptr;
    // this nested loop means we won't create an xml node if we don't have to
    for(int i=0; objRefSaveFlags[i] != -1; i++) {
        if(BIT_ISSET(bits, objRefSaveFlags[i])) {
            curNode = xml::newStringChild(parentNode, name);
            for(; objRefSaveFlags[i] != -1; i++) {
                if(BIT_ISSET(bits, objRefSaveFlags[i]))
                    saveBit(curNode, objRefSaveFlags[i]);
            }
            return(curNode);
        }
    }
    return(curNode);
}



