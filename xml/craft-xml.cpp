/*
 * craft-xml.cpp
 *   Main file for craft-related functions - XML
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

#include <libxml/parser.h>                          // for xmlFreeDoc, xmlNode

#include "bstring.hpp"                              // for bstring, operator+
#include "craft.hpp"                                // for Recipe, operator<<
#include "xml.hpp"                                  // for NODE_NAME, loadOb...

//*********************************************************************
//                      save
//*********************************************************************

void Recipe::save(xmlNodePtr rootNode) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "Recipe");
    xml::newNumProp(curNode, "Id", id);

    result.save(curNode, "Result", false);
    xml::saveNonNullString(curNode, "Skill", skill);
    xml::saveNonNullString(curNode, "Creator", creator);
    xml::saveNonZeroNum(curNode, "Experience", experience);
    xml::saveNonZeroNum(curNode, "RequireRecipe", requireRecipe);
    xml::saveNonZeroNum(curNode, "Sizable", sizable);

    saveList(curNode, "Ingredients", &ingredients);
    saveList(curNode, "Reusables", &reusables);
    saveList(curNode, "Equipment", &equipment);
}


//*********************************************************************
//                      saveList
//*********************************************************************

void Recipe::saveList(xmlNodePtr curNode, const bstring& name, const std::list<CatRef>* list) const {
    if(list->empty())
        return;
    xmlNodePtr childNode = xml::newStringChild(curNode, name.c_str());
    CatRef  cr;

    std::list<CatRef>::const_iterator it;
    for(it = list->begin(); it != list->end() ; it++) {
        cr = (*it);
        cr.save(childNode, "Item", false);
    }
}


//*********************************************************************
//                      load
//*********************************************************************

void Recipe::load(xmlNodePtr curNode) {
    id = xml::getIntProp(curNode, "Id");
    curNode = curNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Result")) result.load(curNode);
        else if(NODE_NAME(curNode, "MinSkill")) xml::copyToNum(minSkill, curNode);
        else if(NODE_NAME(curNode, "Experience")) xml::copyToNum(experience, curNode);
        else if(NODE_NAME(curNode, "Skill")) { xml::copyToBString(skill, curNode); }
        else if(NODE_NAME(curNode, "Creator")) { xml::copyToBString(creator, curNode); }
        else if(NODE_NAME(curNode, "Sizable")) xml::copyToBool(sizable, curNode);
        else if(NODE_NAME(curNode, "RequireRecipe")) xml::copyToBool(requireRecipe, curNode);
        else if(NODE_NAME(curNode, "Ingredients")) loadList(curNode->children, &ingredients);
        else if(NODE_NAME(curNode, "Reusables")) loadList(curNode->children, &reusables);
        else if(NODE_NAME(curNode, "Equipment")) loadList(curNode->children, &equipment);
        curNode = curNode->next;
    }
}

//*********************************************************************
//                      loadList
//*********************************************************************

void Recipe::loadList(xmlNodePtr curNode, std::list<CatRef>* list) {
    while(curNode) {
        if(NODE_NAME(curNode, "Item")) {
            CatRef cr;
            cr.load(curNode);
            list->push_back(cr);
        }
        curNode = curNode->next;
    }
}






