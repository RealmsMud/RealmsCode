/*
 * specials-xml.cpp
 *   Special attacks and such - XML
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

#include "specials.hpp"                             // for SpecialAttack
#include "xml.hpp"                                  // for newStringChild

SpecialAttack::SpecialAttack(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    reset();

    while(curNode) {
        if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
        else if(NODE_NAME(curNode, "Verb")) xml::copyToBString(verb, curNode);
        else if(NODE_NAME(curNode, "TargetStr")) xml::copyToBString(targetStr, curNode);
        else if(NODE_NAME(curNode, "TargetFailStr")) xml::copyToBString(targetFailStr, curNode);
        else if(NODE_NAME(curNode, "RoomFailStr")) xml::copyToBString(roomFailStr, curNode);
        else if(NODE_NAME(curNode, "SelfStr")) xml::copyToBString(selfStr, curNode);
        else if(NODE_NAME(curNode, "RoomStr")) xml::copyToBString(roomStr, curNode);

        else if(NODE_NAME(curNode, "TargetSaveStr")) xml::copyToBString(targetSaveStr, curNode);
        else if(NODE_NAME(curNode, "SelfSaveStr")) xml::copyToBString(selfSaveStr, curNode);
        else if(NODE_NAME(curNode, "RoomSaveStr")) xml::copyToBString(roomSaveStr, curNode);

        else if(NODE_NAME(curNode, "Delay")) xml::copyToNum(delay, curNode);
        else if(NODE_NAME(curNode, "Chance")) xml::copyToNum(chance, curNode);
        else if(NODE_NAME(curNode, "StunLength")) xml::copyToNum(stunLength, curNode);
        else if(NODE_NAME(curNode, "Limit")) xml::copyToNum(limit, curNode);
        else if(NODE_NAME(curNode, "MaxBonus")) xml::copyToNum(maxBonus, curNode);
        else if(NODE_NAME(curNode, "SaveType")) saveType = (SpecialSaveType)xml::toNum<int>(xml::getCString(curNode));
        else if(NODE_NAME(curNode, "SaveBonus")) saveBonus = (SaveBonus)xml::toNum<int>(xml::getCString(curNode));
        else if(NODE_NAME(curNode, "Type")) type = (SpecialType)xml::toNum<int>(xml::getCString(curNode));
        else if(NODE_NAME(curNode, "Flags")) loadBits(curNode, flags);
        else if(NODE_NAME(curNode, "LastTime")) loadLastTime(curNode, &ltime);
        else if(NODE_NAME(curNode, "Dice")) damage.load(curNode);

        curNode = curNode->next;
    }
}

bool SpecialAttack::save(xmlNodePtr rootNode) const {
    xmlNodePtr attackNode = xml::newStringChild(rootNode, "SpecialAttack");
    xml::newStringChild(attackNode, "Name", name);
    xml::newStringChild(attackNode, "Verb", verb);

    xml::newStringChild(attackNode, "TargetStr", targetStr);
    xml::newStringChild(attackNode, "TargetFailStr", targetFailStr);
    xml::newStringChild(attackNode, "RoomFailStr", roomFailStr);
    xml::newStringChild(attackNode, "SelfStr", selfStr);
    xml::newStringChild(attackNode, "RoomStr", roomStr);

    xml::newStringChild(attackNode, "TargetSaveStr", targetSaveStr);
    xml::newStringChild(attackNode, "SelfSaveStr", selfSaveStr);
    xml::newStringChild(attackNode, "RoomSaveStr", roomSaveStr);

    xml::newNumChild(attackNode, "Limit", limit);
    xml::newNumChild(attackNode, "Delay", delay);
    xml::newNumChild(attackNode, "Chance", chance);
    xml::newNumChild(attackNode, "StunLength", stunLength);
    xml::newNumChild(attackNode, "Type", type);
    xml::newNumChild(attackNode, "SaveType", saveType);
    xml::newNumChild(attackNode, "SaveBonus", saveBonus);
    xml::newNumChild(attackNode, "MaxBonus", maxBonus);

    saveBits(attackNode, "Flags", SA_MAX_FLAG, flags);
    saveLastTime(attackNode, 0, ltime);
    damage.save(attackNode, "Dice");

    return(true);
}

