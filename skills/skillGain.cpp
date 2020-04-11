/*
 * skillGain.h
 *   Skill gain
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
#include <map>                                      // for operator!=, opera...

#include "bstring.hpp"                              // for bstring
#include "config.hpp"                               // for Config, gConfig
#include "skillGain.hpp"                            // for SkillGain
#include "xml.hpp"                                  // for copyToBString

//*********************************************************************
//                      SkillGain
//*********************************************************************

SkillGain::SkillGain(xmlNodePtr rootNode) {
    load(rootNode);
}

SkillGain::~SkillGain() {
    deities.clear();
}

//*********************************************************************
//                      load
//*********************************************************************

void SkillGain::load(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "Name")) {
            xml::copyToBString(skillName, curNode);
        } else if(NODE_NAME(curNode, "Gained")) {
            xml::copyToNum(gainLevel, curNode);
        } else if(NODE_NAME(curNode, "Deity")) {
            bstring deityStr;
            xml::copyToBString(deityStr, curNode);
            int deity = gConfig->deitytoNum(deityStr);
            if(deity != -1) {
                deities[deity] = true;
            }
        }

        curNode = curNode->next;
    }
}

//*********************************************************************
//                      getGained
//*********************************************************************

int SkillGain::getGained() { return(gainLevel); }

//*********************************************************************
//                      getName
//*********************************************************************

bstring SkillGain::getName() const { return(skillName); }

//*********************************************************************
//                      deityIsAllowed
//*********************************************************************

bool SkillGain::deityIsAllowed(int deity) {
    if(deities.find(deity) != deities.end())
        return(true);
    else
        return(false);
}

//*********************************************************************
//                      hasDeities
//*********************************************************************

bool SkillGain::hasDeities() {
    return(!this->deities.empty());
}
