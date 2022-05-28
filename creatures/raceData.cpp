/*
 * raceData.cpp
 *   Race data storage file
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

#include <libxml/parser.h>                          // for xmlFreeDoc, xmlCl...
#include <boost/algorithm/string/case_conv.hpp>     // for to_lower
#include <boost/algorithm/string/predicate.hpp>     // for iequals
#include <boost/algorithm/string/replace.hpp>       // for replace_all
#include <boost/iterator/iterator_traits.hpp>       // for iterator_value<>:...
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <cstdio>                                   // for snprintf
#include <cstring>                                  // for strcmp
#include <deque>                                    // for _Deque_iterator
#include <list>                                     // for list, operator==
#include <map>                                      // for operator==, map
#include <string>                                   // for string, char_traits
#include <string_view>                              // for operator==, basic...
#include <utility>                                  // for pair

#include "cmd.hpp"                                  // for cmd
#include "config.hpp"                               // for Config, RaceDataMap
#include "deityData.hpp"                            // for DeityData
#include "dice.hpp"                                 // for Dice
#include "global.hpp"                               // for CreatureClass
#include "mudObjects/players.hpp"                   // for Player
#include "paths.hpp"                                // for Game
#include "proto.hpp"                                // for zero, getSexName
#include "raceData.hpp"                             // for RaceData
#include "size.hpp"                                 // for getSize, getSizeName
#include "skillGain.hpp"                            // for SkillGain
#include "structs.hpp"                              // for Sex
#include "xml.hpp"                                  // for NODE_NAME, copyPr...


//*********************************************************************
//                      raceCount
//*********************************************************************

unsigned short Config::raceCount() const {
    return(races.size());
}

//*********************************************************************
//                      getPlayableRaceCount
//*********************************************************************

unsigned short Config::getPlayableRaceCount() {
    RaceData* curRace;
    int numPlayableRaces = 0;
    for(std::pair<int, RaceData*> rp : races) {
        curRace = rp.second;
        if(!curRace) continue;

        if(curRace->isPlayable())
            numPlayableRaces++;
    }
    return(numPlayableRaces);
}

//*********************************************************************
//                      RaceData
//*********************************************************************

RaceData::RaceData(xmlNodePtr rootNode) {
    Sex     sex;
    Dice    dice;
    char    sizeTxt[20];
    int     i=0;
    short   n=0;
    std::string str;

    id = 0;
    name = adjective = abbr = "";
    infra = bonus = false;
    startAge = parentRace = porphyriaResistance = 0;
    size = NO_SIZE;
    isParentRace = false;
    playable = gendered = true;
    zero(stats, sizeof(stats));
    zero(saves, sizeof(saves));
    zero(classes, sizeof(classes));

    zero(multiClerDeities, sizeof(multiClerDeities));
    zero(clerDeities, sizeof(clerDeities));
    zero(palDeities, sizeof(palDeities));
    zero(dkDeities, sizeof(dkDeities));

    id = xml::getIntProp(rootNode, "id");
    xml::copyPropToString(name, rootNode, "name");

    xmlNodePtr subNode, childNode, curNode = rootNode->children;

    while(curNode) {
             if(NODE_NAME(curNode, "Adjective")) xml::copyToString(adjective, curNode);
        else if(NODE_NAME(curNode, "Abbr")) xml::copyToString(abbr, curNode);
        else if(NODE_NAME(curNode, "NotPlayable")) playable = false;
        else if(NODE_NAME(curNode, "ParentRace")) {
            xml::copyToNum(parentRace, curNode);
            if(gConfig->races.find(parentRace) != gConfig->races.end())
                gConfig->races[parentRace]->makeParent();
        }
        else if(NODE_NAME(curNode, "Data")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Size")) {
                    xml::copyToCString(sizeTxt, childNode);
                    size = ::getSize(sizeTxt);
                }
                else if(NODE_NAME(childNode, "BaseHeight")) {
                    sex = (Sex)xml::getIntProp(childNode, "Sex");
                    xml::copyToNum(n, childNode);
                    baseHeight[sex] = n;
                }
                else if(NODE_NAME(childNode, "BaseWeight")) {
                    sex = (Sex)xml::getIntProp(childNode, "Sex");
                    xml::copyToNum(n, childNode);
                    baseWeight[sex] = n;
                }
                else if(NODE_NAME(childNode, "VarHeight")) {
                    sex = (Sex)xml::getIntProp(childNode, "Sex");
                    dice.load(childNode);
                    varHeight[sex] = dice;
                }
                else if(NODE_NAME(childNode, "VarWeight")) {
                    sex = (Sex)xml::getIntProp(childNode, "Sex");
                    dice.load(childNode);
                    varWeight[sex] = dice;
                }
                else if(NODE_NAME(childNode, "Gendered")) xml::copyToBool(gendered, childNode);
                else if(NODE_NAME(childNode, "BonusStat")) {
                    xml::copyToNum(i, childNode);
                    if(i)
                        bonus = true;
                }
                else if(NODE_NAME(childNode, "StartAge")) xml::copyToNum(startAge, childNode);
                else if(NODE_NAME(childNode, "PorphyriaResistance")) xml::copyToNum(porphyriaResistance, childNode);
                else if(NODE_NAME(childNode, "Infravision")) xml::copyToBool(infra, childNode);
                else if(NODE_NAME(childNode, "Stats")) {
                    subNode = childNode->children;
                    while(subNode) {
                        if(NODE_NAME(subNode, "Stat")) {
                            xml::copyPropToString(str, subNode, "id");
                            xml::copyToNum(stats[Config::stattoNum(str)], subNode);
                        }
                        subNode = subNode->next;
                    }
                }
                else if(NODE_NAME(childNode, "Classes")) {
                    subNode = childNode->children;
                    while(subNode) {
                        if(NODE_NAME(subNode, "Class")) {
                            xml::copyPropToString(str, subNode, "id");
                            classes[gConfig->classtoNum(str)] = true;
                        }
                        subNode = subNode->next;
                    }
                }
                else if(NODE_NAME(childNode, "MultiClericDeities")) {
                    subNode = childNode->children;
                    while(subNode) {
                        if(NODE_NAME(subNode, "Deity")) {
                            xml::copyPropToString(str, subNode, "id");
                            multiClerDeities[gConfig->deitytoNum(str)] = true;
                        }
                        subNode = subNode->next;
                    }
                }
                else if(NODE_NAME(childNode, "ClericDeities")) {
                    subNode = childNode->children;
                    while(subNode) {
                        if(NODE_NAME(subNode, "Deity")) {
                            xml::copyPropToString(str, subNode, "id");
                            clerDeities[gConfig->deitytoNum(str)] = true;
                        }
                        subNode = subNode->next;
                    }
                }
                else if(NODE_NAME(childNode, "PaladinDeities")) {
                    subNode = childNode->children;
                    while(subNode) {
                        if(NODE_NAME(subNode, "Deity")) {
                            xml::copyPropToString(str, subNode, "id");
                            palDeities[gConfig->deitytoNum(str)] = true;
                        }
                        subNode = subNode->next;
                    }
                }
                else if(NODE_NAME(childNode, "DeathknightDeities")) {
                    subNode = childNode->children;
                    while(subNode) {
                        if(NODE_NAME(subNode, "Deity")) {
                            xml::copyPropToString(str, subNode, "id");
                            dkDeities[gConfig->deitytoNum(str)] = true;
                        }
                        subNode = subNode->next;
                    }
                }
                else if(NODE_NAME(childNode, "SavingThrows")) {
                    subNode = childNode->children;
                    while(subNode) {
                        if(NODE_NAME(subNode, "SavingThrow")) {
                            xml::copyPropToString(str, subNode, "id");
                            xml::copyToNum(saves[Config::savetoNum(str)], subNode);
                        }
                        subNode = subNode->next;
                    }
                }
                else if(NODE_NAME(childNode, "Effects")) {
                    subNode = childNode->children;
                    while(subNode) {
                        if(NODE_NAME(subNode, "Effect")) {
                            xml::copyToString(str, subNode);
                            effects.push_back(str);
                        }
                        subNode = subNode->next;
                    }
                } else if(NODE_NAME(childNode, "Skills")) {
                    xmlNodePtr skillNode = childNode->children;
                    while(skillNode) {
                        if(NODE_NAME(skillNode, "Skill")) {
                            auto* skillGain = new SkillGain(skillNode);
                            baseSkills.push_back(skillGain);
                        }
                        skillNode = skillNode->next;
                    }
                }
                childNode = childNode->next;
            }
        }
        curNode = curNode->next;
    }
}
RaceData::~RaceData() {
    for(SkillGain* skGain : baseSkills) {
        delete skGain;
    }
    baseSkills.clear();
}
//*********************************************************************
//                      stattoNum
//*********************************************************************

int Config::stattoNum(std::string_view str) {
    if(str == "Str") return(STR);
    if(str == "Con") return(CON);
    if(str == "Dex") return(DEX);
    if(str == "Int") return(INT);
    if(str == "Pty") return(PTY);
    return(0);
}

//*********************************************************************
//                      savetoNum
//*********************************************************************

int Config::savetoNum(std::string_view str) {
    if(str == "Poi") return(POI);
    if(str == "Dea") return(DEA);
    if(str == "Bre") return(BRE);
    if(str == "Men") return(MEN);
    if(str == "Spl") return(SPL);
    return(0);
}

//*********************************************************************
//                      classtoNum
//*********************************************************************

int Config::classtoNum(std::string_view str) {
    if(str == "Assassin") return(static_cast<int>(CreatureClass::ASSASSIN));
    if(str == "Berserker") return(static_cast<int>(CreatureClass::BERSERKER));
    if(str == "Cleric") return(static_cast<int>(CreatureClass::CLERIC));
    if(str == "Fighter") return(static_cast<int>(CreatureClass::FIGHTER));
    if(str == "Mage") return(static_cast<int>(CreatureClass::MAGE));
    if(str == "Paladin") return(static_cast<int>(CreatureClass::PALADIN));
    if(str == "Ranger") return(static_cast<int>(CreatureClass::RANGER));
    if(str == "Thief") return(static_cast<int>(CreatureClass::THIEF));
    if(str == "Pureblood") return(static_cast<int>(CreatureClass::PUREBLOOD));
    if(str == "Monk") return(static_cast<int>(CreatureClass::MONK));
    if(str == "Deathknight") return(static_cast<int>(CreatureClass::DEATHKNIGHT));
    if(str == "Druid") return(static_cast<int>(CreatureClass::DRUID));
    if(str == "Lich") return(static_cast<int>(CreatureClass::LICH));
    if(str == "Werewolf") return(static_cast<int>(CreatureClass::WEREWOLF));
    if(str == "Bard") return(static_cast<int>(CreatureClass::BARD));
    if(str == "Rogue") return(static_cast<int>(CreatureClass::ROGUE));
    if(str == "Fighter/Mage") return(MULTI_BASE+0);
    if(str == "Fighter/Thief") return(MULTI_BASE+1);
    if(str == "Cleric/Assassin") return(MULTI_BASE+2);
    if(str == "Mage/Thief") return(MULTI_BASE+3);
    if(str == "Thief/Mage") return(MULTI_BASE+4);
    if(str == "Cleric/Fighter") return(MULTI_BASE+5);
    if(str == "Mage/Assassin") return(MULTI_BASE+6);
    return(0);
}

//*********************************************************************
//                      racetoNum
//*********************************************************************

int Config::racetoNum(std::string_view str) {
    for(const auto& [rId, race]: races) {
        if(boost::iequals(str, race->getName()) || boost::iequals(str, race->getAdjective()))
            return race->getId();
    }
    return(-1);
}

//*********************************************************************
//                      deitytoNum
//*********************************************************************

int Config::deitytoNum(std::string_view str) {
    for(unsigned i=0; i<deities.size(); i++) {
        if(!deities[i])
            continue;
        if(str == deities[i]->getName())
            return((int)i);
    }
    return(-1);
}

//**********************************************************************
//                      loadRaces
//**********************************************************************

bool Config::loadRaces() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;
    int     i=0;

    char filename[80];
    snprintf(filename, 80, "%s/races.xml", Path::Game.c_str());
    xmlDoc = xml::loadFile(filename, "Races");

    if(xmlDoc == nullptr)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode))
        curNode = curNode->next;

    if(curNode == nullptr) {
        xmlFreeDoc(xmlDoc);
        return(false);
    }

    clearRaces();
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Race")) {
            i = xml::getIntProp(curNode, "id");

            if(races.find(i) == races.end())
                races[i] = new RaceData(curNode);
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}


//**********************************************************************
//                      getRace
//**********************************************************************

const RaceData* Config::getRace(std::string race) const {
    RaceDataMap::const_iterator it;
    RaceData* data=nullptr;
    size_t len = race.length();
    boost::algorithm::to_lower(race);

    for(it = races.begin() ; it != races.end() ; it++) {
        // exact match
        if(boost::iequals((*it).second->getName(), race))
            return((*it).second);
        if((*it).second->getName().substr(0, len) == race) {
            // not unique
            if(data)
                return(nullptr);
            data = (*it).second;
        }
    }

    return(data);
}

//**********************************************************************
//                      getRace
//**********************************************************************

const RaceData* Config::getRace(int id) const {
    auto it = races.find(id);

    if(it == races.end())
        it = races.begin();

    return((*it).second);
}

//*********************************************************************
//                      dmShowRaces
//*********************************************************************

int dmShowRaces(const std::shared_ptr<Player>& player, cmd* cmnd) {
    RaceDataMap::const_iterator it;
    RaceData* data;
    bool    all = player->isDm() && cmnd->num > 1 && !strcmp(cmnd->str[1], "all");
    std::map<Sex, short>::const_iterator bIt;
    std::map<Sex, Dice>::const_iterator dIt;

    player->printColor("Displaying Races:%s\n",
        player->isDm() && !all ? "  Type ^y*racelist all^x to view all information." : "");
    for(it = gConfig->races.begin() ; it != gConfig->races.end() ; it++) {
        data = (*it).second;
        player->printColor("Id: ^c%-2d^x   Name: ^c%s\n", data->getId(), data->getName().c_str());
        if(all) {
            if(data->bonusStat())
                player->printColor("   ^mThis race picks a bonus and a penalty stat.\n");
            else
                player->printColor("   Str: ^m%-2d^x  Dex: ^m%-2d^x  Con: ^m%-2d^x  Int: ^m%-2d^x  Pty: ^m%-2d^x\n",
                    data->getStatAdj(STR), data->getStatAdj(DEX), data->getStatAdj(CON),
                    data->getStatAdj(INT), data->getStatAdj(PTY));
            player->printColor("   Infravision: %-3s^x  Size: %s  isParent: %-3s\n",
                data->hasInfravision() ? "^gYes" : "^rNo",
                getSizeName(data->getSize()).c_str(),
                data->isParent() ? "^gYes" : "^rNo");
            if(data->getParentRace())
                player->print("   Parent Race: %s", gConfig->getRace(data->getParentRace())->getName().c_str());
            if(data->getPorphyriaResistance())
                player->print("   Porphyria Resistance: %d", data->getPorphyriaResistance());
            player->printColor("   Playable: %s\n", data->isPlayable() ? "^gYes" : "^rNo");

            // base weight for all genders
            for(bIt = data->baseHeight.begin() ; bIt != data->baseHeight.end() ; bIt++) {
                player->printColor("   BaseHeight: %s: ^c%d", getSexName((*bIt).first).c_str(), (*bIt).second);
            }
            if(!data->baseHeight.empty())
                player->print("\n");
            // variable weight for all genders
            for(dIt = data->varHeight.begin() ; dIt != data->varHeight.end() ; dIt++) {
                player->printColor("   VarHeight: %s: ^c%s", getSexName((*dIt).first).c_str(), (*dIt).second.str().c_str());
            }
            if(!data->varHeight.empty())
                player->print("\n");

            // base height for all genders
            for(bIt = data->baseWeight.begin() ; bIt != data->baseWeight.end() ; bIt++) {
                player->printColor("   BaseWeight: %s: ^c%d", getSexName((*bIt).first).c_str(), (*bIt).second);
            }
            if(!data->baseWeight.empty())
                player->print("\n");
            // variable height for all genders
            for(dIt = data->varWeight.begin() ; dIt != data->varWeight.end() ; dIt++) {
                player->printColor("   VarWeight: %s: ^c%s", getSexName((*dIt).first).c_str(), (*dIt).second.str().c_str());
            }
            if(!data->varWeight.empty())
                player->print("\n");
        }
    }
    player->print("\n");
    return(0);
}

//**********************************************************************
//                      clearRaces
//**********************************************************************

void Config::clearRaces() {
    RaceDataMap::iterator it;

    for(it = races.begin() ; it != races.end() ; it++) {
        RaceData* r = (*it).second;
        delete r;
    }
    races.clear();
}

//*********************************************************************
//                      getAdjective
//*********************************************************************

std::string RaceData::getAdjective() const { return(adjective); }

//*********************************************************************
//                      getAbbr
//*********************************************************************

std::string RaceData::getAbbr() const { return(abbr); }

//*********************************************************************
//                      getParentRace
//*********************************************************************

int RaceData::getParentRace() const { return(parentRace); }

//*********************************************************************
//                      getPorphyriaResistance
//*********************************************************************

short RaceData::getPorphyriaResistance() const { return(porphyriaResistance); }

//*********************************************************************
//                      makeParent
//*********************************************************************

void RaceData::makeParent() { isParentRace = true; }

//*********************************************************************
//                      isParent
//*********************************************************************

bool RaceData::isParent() const { return(isParentRace); }

//*********************************************************************
//                      isPlayable
//*********************************************************************

bool RaceData::isPlayable() const { return(playable); }

//*********************************************************************
//                      isGendered
//*********************************************************************

bool RaceData::isGendered() const { return(gendered); }

//*********************************************************************
//                      hasInfravision
//*********************************************************************

bool RaceData::hasInfravision() const { return(infra); }

//*********************************************************************
//                      bonusStat
//*********************************************************************

bool RaceData::bonusStat() const { return(bonus); }

//*********************************************************************
//                      getSize
//*********************************************************************

Size RaceData::getSize() const { return(size); }

//*********************************************************************
//                      getStartAge
//*********************************************************************

int RaceData::getStartAge() const { return(startAge); }

//*********************************************************************
//                      getStatAdj
//*********************************************************************

int RaceData::getStatAdj(int stat) const { return(stats[stat]); }

//*********************************************************************
//                      getSave
//*********************************************************************

int RaceData::getSave(int save) const { return(saves[save]); }

//*********************************************************************
//                      allowedClass
//*********************************************************************

bool RaceData::allowedClass(int cls) const { return(classes[cls]); }

//*********************************************************************
//                      allowedDeity
//*********************************************************************

bool RaceData::allowedDeity(CreatureClass cls, CreatureClass cls2, int dty) const {
    return( (cls == CreatureClass::CLERIC && cls2 == CreatureClass::NONE && allowedClericDeity(dty)) ||
            (cls == CreatureClass::PALADIN && cls2 == CreatureClass::NONE && allowedPaladinDeity(dty)) ||
            (cls == CreatureClass::DEATHKNIGHT && cls2 == CreatureClass::NONE && allowedDeathknightDeity(dty)) ||
            (cls == CreatureClass::CLERIC && cls2 != CreatureClass::NONE && allowedMultiClericDeity(dty))
    );
}

//*********************************************************************
//                      allowedMultiClericDeity
//*********************************************************************

bool RaceData::allowedMultiClericDeity(int dty) const { return(multiClerDeities[dty]); }

//*********************************************************************
//                      allowedClericDeity
//*********************************************************************

bool RaceData::allowedClericDeity(int dty) const { return(clerDeities[dty]); }

//*********************************************************************
//                      allowedPaladinDeity
//*********************************************************************

bool RaceData::allowedPaladinDeity(int dty) const { return(palDeities[dty]); }

//*********************************************************************
//                      allowedDeathknightDeity
//*********************************************************************

bool RaceData::allowedDeathknightDeity(int dty) const { return(dkDeities[dty]); }

//*********************************************************************
//                      getId
//*********************************************************************

int RaceData::getId() const { return(id); }

//*********************************************************************
//                      getName
//*********************************************************************

std::string RaceData::getName(int tryToShorten) const {
    if(!tryToShorten)
        return(name);
    std::string txt = name;
    if(txt.length() > (unsigned)tryToShorten)
        boost::replace_all(txt, "Half-", "H-");
    return(txt);
}

//*********************************************************************
//                      getSkillBegin
//*********************************************************************

std::list<SkillGain*>::const_iterator RaceData::getSkillBegin() const {
    return(baseSkills.begin());
}

//*********************************************************************
//                      getSkillEnd
//*********************************************************************

std::list<SkillGain*>::const_iterator RaceData::getSkillEnd() const {
    return(baseSkills.end());
}
