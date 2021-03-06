/*
 * rooms-xml.cpp
 *   Rooms xml
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

#include <bits/exception.h>                         // for exception
#include <libxml/parser.h>                          // for xmlNodePtr, xmlNode
#include <cstdio>                                   // for sprintf
#include <cstdlib>                                  // for abort
#include <map>                                      // for map, map<>::mappe...
#include <ostream>                                  // for operator<<, basic...
#include <stdexcept>                                // for runtime_error
#include <string>                                   // for operator==, opera...

#include "bstring.hpp"                              // for bstring
#include "config.hpp"                               // for Config, SkillComm...
#include "paths.hpp"                                // for Code
#include "skills.hpp"                               // for SkillCommand, Ski...
#include "xml.hpp"                                  // for NODE_NAME, copyTo...

class Creature;
class cmd;

//*********************************************************************
//                      loadSkills
//*********************************************************************

bool Config::loadSkills() {
    xmlDocPtr   xmlDoc;
    //GuildCreation * curCreation;
    xmlNodePtr  cur;
    char        filename[80];

    // build an XML tree from a the file
    sprintf(filename, "%s/skills.xml", Path::Code);

    xmlDoc = xml::loadFile(filename, "Skills");
    if(xmlDoc == nullptr)
        return(false);

    cur = xmlDocGetRootElement(xmlDoc);

    cur = cur->children;
    while(cur && xmlIsBlankNode(cur)) {
        cur = cur->next;
    }
    if(cur == nullptr) {
        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();
        return(false);
    }

    clearSkills();
    while(cur != nullptr) {
        if(NODE_NAME(cur, "SkillGroups")) {
            loadSkillGroups(cur);
        } else if(NODE_NAME(cur, "Skills")) {
            loadSkills(cur);
        }
        cur = cur->next;
    }
    updateSkillPointers();
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);

}

//*********************************************************************
//                      loadSkillGroups
//*********************************************************************

void Config::loadSkillGroups(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "SkillGroup")) {
            loadSkillGroup(curNode);
        }

        curNode = curNode->next;
    }
}

//*********************************************************************
//                      loadSkillGroup
//*********************************************************************

void Config::loadSkillGroup(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    bstring name;
    bstring displayName;
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Name")) {
            xml::copyToBString(name, curNode);
        } else if(NODE_NAME(curNode, "DisplayName")) {
            xml::copyToBString(displayName, curNode);
        }
        curNode = curNode->next;
    }
    if(!name.empty() && !displayName.empty()) {
        skillGroups[name] = displayName;
    }
}

//*********************************************************************
//                      loadSkills
//*********************************************************************

void Config::loadSkills(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    SkillInfo* skill=nullptr;
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Skill")) {
            try {
                skill = new SkillInfo(curNode);
                // A SkillInfo is not a SkillCommand, it only goes in the SkillInfo table
                skills.insert(SkillInfoMap::value_type(skill->getName(), skill));
            } catch(std::exception &e) {

            }
        } else if(NODE_NAME(curNode, "SkillCommand")) {
            try {
                skill = new SkillCommand(curNode);
                auto* skillCmd = dynamic_cast<SkillCommand*>(skill);
                // All SkillCommands are also SkillInfos, put them in both tables
                skills.insert(SkillInfoMap::value_type(skill->getName(), skill));
                skillCommands.insert(SkillCommandMap::value_type(skill->getName(), skillCmd));
                // Insert any aliases as well
                for(const bstring& alias : skillCmd->aliases) {
                    skillCommands.insert(SkillCommandMap::value_type(alias, skillCmd));
                }
            } catch(std::exception &e) {

            }
        }

        curNode = curNode->next;
    }
}

//*********************************************************************
//                      load
//*********************************************************************

Skill::Skill(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    reset();

    while(curNode) {
        if(NODE_NAME(curNode, "Name")) {
            setName(xml::getBString(curNode));
        } else if(NODE_NAME(curNode, "Gained")) {
            xml::copyToNum(gained, curNode);
        } else if(NODE_NAME(curNode, "GainBonus")) {
            xml::copyToNum(gainBonus, curNode);
        }
        curNode = curNode->next;
    }
    if(name.empty() || gained == 0) {
        throw(std::runtime_error("Invalid Skill Xml"));
    }
}



//*********************************************************************
//                      load
//*********************************************************************

SkillInfo::SkillInfo(xmlNodePtr rootNode) {
    gainType = SKILL_NORMAL;
    knownOnly = false;

    xmlNodePtr curNode = rootNode->children;
    bstring group;
    bstring description;

    while(curNode != nullptr) {
        readNode(curNode);
        curNode = curNode->next;
    }
    if(name.empty() || displayName.empty()) {
        std::clog << "Invalid skill (Name:" << name << ", DisplayName: " << displayName <<  ")" << std::endl;
        throw(std::runtime_error("Invalid SkillInfo XML"));
    }
}

int cmdSkill(Creature* creature, cmd* cmnd);

SkillCommand::SkillCommand(xmlNodePtr rootNode) {
    gainType = SKILL_NORMAL;
    knownOnly = false;
    usesAttackTimer = true;

    xmlNodePtr curNode = rootNode->children;
    priority = 100;
    auth = nullptr;
    description = "";
    targetType = TARGET_NONE;

    fn = cmdSkill;
    while(curNode != nullptr) {
        readNode(curNode);
        curNode = curNode->next;
    }
    if(SkillInfo::name.empty() || displayName.empty()) {
        std::clog << "Invalid skillCommand (Name:" << SkillInfo::name << ", DisplayName: " << displayName <<  ")" << std::endl;
        throw(std::runtime_error("Invalid Skill Command Xml"));
    } else {
        std::clog << "Found SkillCommand: " << SkillInfo::name << std::endl;
    }

}

bool SkillInfo::readNode(xmlNodePtr curNode) {
    bool retVal = true;
    bstring tGroup;
    if(NODE_NAME(curNode, "Name")) {
        setName(xml::getBString(curNode));
    } else if(NODE_NAME(curNode, "DisplayName")) {
        xml::copyToBString(displayName, curNode);
    } else if(NODE_NAME(curNode, "Base")) {
        xml::copyToBString(tGroup, curNode);
        if(!setBase(tGroup)) {
            std::clog << "Error setting skill '" << name << "' base skill to '" << tGroup << "'" << std::endl;
            abort();
        }
    } else if(NODE_NAME(curNode, "Group")) {
        xml::copyToBString(tGroup, curNode);
        if(!setGroup(tGroup)) {
            std::clog << "Error setting skill '" << name << "' to group '" << tGroup << "'" << std::endl;
            abort();
        }
    } else if(NODE_NAME(curNode, "Description")) {
        xml::copyToBString(description, curNode);
    } else if(NODE_NAME(curNode, "GainType")) {
        bstring strGainType;
        xml::copyToBString(strGainType, curNode);
        if(strGainType == "Medium") {
            gainType = SKILL_MEDIUM;
        } else if(strGainType == "Hard") {
            gainType = SKILL_HARD;
        } else if(strGainType == "Normal") {
            gainType = SKILL_NORMAL;
        } else if(strGainType == "Easy") {
            gainType = SKILL_EASY;
        }
    } else if(NODE_NAME(curNode, "KnownOnly")) {
        xml::copyToBool(knownOnly, curNode);
    } else {
        // Nothing was read
        retVal = false;
    }

    return(retVal);
}
bool SkillCommand::readNode(xmlNodePtr curNode) {
    bool retVal = true;

    // Try the base class method first, if they return true, it found something so continue on
    if(SkillInfo::readNode(curNode)) {
        retVal = true;
    } else if(NODE_NAME(curNode, "Cooldown")) {
        xml::copyToNum(cooldown, curNode);
    } else if(NODE_NAME(curNode, "UsesAttackTimer")) {
        xml::copyToBool(usesAttackTimer, curNode);
    } else if(NODE_NAME(curNode, "FailCooldown")) {
        xml::copyToNum(failCooldown, curNode);
    } else if(NODE_NAME(curNode, "Resources")) {
        loadResources(curNode);
    } else if(NODE_NAME(curNode, "Offensive")) {
        xml::copyToBool(offensive, curNode);
    } else if(NODE_NAME(curNode, "Alias")) {
        aliases.push_back(xml::getBString(curNode));
    } else if(NODE_NAME(curNode, "TargetType")) {
        bstring tType = xml::getBString(curNode);
        if(tType == "Creature")
            targetType = TARGET_CREATURE;
        else if(tType == "Monster")
            targetType = TARGET_MONSTER;
        else if(tType == "Player")
            targetType = TARGET_PLAYER;
        else if(tType == "Object")
            targetType = TARGET_OBJECT;
        else if(tType == "Exit")
            targetType = TARGET_EXIT;
        else if(tType == "Any" || tType == "All" || tType == "MudObject")
            targetType = TARGET_MUDOBJECT;
        else if(tType == "Self")
            targetType = TARGET_NONE;
    } else if(NODE_NAME(curNode, "Script")) {
        xml::copyToBString(pyScript, curNode);
    } else {
        retVal = false;
    }
    return(retVal);
}
void SkillCommand::loadResources(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Resource")) {
            resources.emplace_back(curNode);
        }
        curNode = curNode->next;
    }
}

SkillCost::SkillCost(xmlNodePtr rootNode) {
    resource = RES_NONE;
    cost = 0;

    xmlNodePtr curNode = rootNode->children;
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Type")) {
            bstring resourceStr;
            xml::copyToBString(resourceStr, curNode);
            if(resourceStr.equals("Gold", false)) {
                resource = RES_GOLD;
            } else if(resourceStr.equals("Mana", false) || resourceStr.equals("Mp")) {
                resource = RES_MANA;
            } else if(resourceStr.equals("HitPoints", false) || resourceStr.equals("Hp")) {
                resource = RES_HIT_POINTS;
            } else if(resourceStr.equals("Focus", false)) {
                resource = RES_FOCUS;
            } else if(resourceStr.equals("Energy", false)) {
                resource = RES_ENERGY;
            }
        } else if(NODE_NAME(curNode, "Cost")) {
            xml::copyToNum(cost, curNode);
        }
        curNode = curNode->next;
    }

}

//*********************************************************************
//                      save
//*********************************************************************

void Skill::save(xmlNodePtr rootNode) const {
    xmlNodePtr skillNode = xml::newStringChild(rootNode, "Skill");
    xml::newStringChild(skillNode, "Name", getName());
    xml::newNumChild(skillNode, "Gained", getGained());
    xml::newNumChild(skillNode, "GainBonus", getGainBonus());
}

