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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "area.hpp"
#include "creatures.hpp"
#include "config.hpp"
#include "objects.hpp"
#include "proto.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "mud.hpp"
#include "xml.hpp"

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
        else if(NODE_NAME(curNode, "Type")) xml::copyToNum<ObjectType>(type, curNode);
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
    std::list<bstring> *idList;

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



