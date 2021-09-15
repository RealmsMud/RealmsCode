/*
 * playerClass.cpp
 *   Player Class
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

#include <libxml/parser.h>                          // for xmlNode, xmlNodePtr
#include <cstring>                                 // for strcmp

#include "bstring.hpp"                              // for bstring
#include "cmd.hpp"                                  // for cmd
#include "config.hpp"                               // for Config, gConfig
#include "creatures.hpp"                            // for Player
#include "levelGain.hpp"                            // for LevelGain
#include "playerClass.hpp"                          // for PlayerClass
#include "playerTitle.hpp"                          // for PlayerTitle
#include "server.hpp"                               // for Server, gServer
#include "skillGain.hpp"                            // for SkillGain


//*********************************************************************
//                      PlayerClass
//*********************************************************************

PlayerClass::PlayerClass(xmlNodePtr rootNode) {
    needDeity = false;
    numProf = 1;
    hasAutomaticStats = false;
    baseStrength=-1;
    baseDexterity=-1;
    baseConstitution=-1;
    baseIntelligence=-1;
    basePiety=-1;
    unarmedWeaponSkill = "bare-hand";
    load(rootNode);
}

PlayerClass::~PlayerClass() {
    std::list<SkillGain*>::iterator bsIt;
    std::map<int, LevelGain*>::iterator lgIt;
    std::map<int, PlayerTitle*>::iterator tt;
    SkillGain* sGain;
    LevelGain* lGain;

    for(bsIt = baseSkills.begin() ; bsIt != baseSkills.end() ; bsIt++) {
        sGain = (*bsIt);
        delete sGain;
    }
    baseSkills.clear();
    for(lgIt = levels.begin() ; lgIt != levels.end() ; lgIt++) {
        lGain = (*lgIt).second;
        delete lGain;
    }
    levels.clear();

    for(tt = titles.begin() ; tt != titles.end() ; tt++) {
        PlayerTitle* p = (*tt).second;
        delete p;
    }
    titles.clear();
}

int PlayerClass::getId() const { return(id); }
bstring PlayerClass::getName() const { return(name); }
bstring PlayerClass::getUnarmedWeaponSkill() const { return(unarmedWeaponSkill); }
std::list<SkillGain*>::const_iterator PlayerClass::getSkillBegin() { return(baseSkills.begin()); }
std::list<SkillGain*>::const_iterator PlayerClass::getSkillEnd() { return(baseSkills.end()); }
std::map<int, LevelGain*>::const_iterator PlayerClass::getLevelBegin() { return(levels.begin()); }
std::map<int, LevelGain*>::const_iterator PlayerClass::getLevelEnd() { return(levels.end()); }
short PlayerClass::getBaseHp() { return(baseHp); }
short PlayerClass::getBaseMp() { return(baseMp); }
bool PlayerClass::needsDeity() { return(needDeity); }
short PlayerClass::numProfs() { return(numProf); }
LevelGain* PlayerClass::getLevelGain(int lvl) { return(levels[lvl]); }
bool PlayerClass::hasDefaultStats() { return(hasAutomaticStats); }
bool PlayerClass::setDefaultStats(Player* player) {
    if(!hasDefaultStats() || !player)
        return(false);

    player->strength.setInitial(baseStrength * 10);
    player->dexterity.setInitial(baseDexterity * 10);
    player->constitution.setInitial(baseConstitution * 10);
    player->intelligence.setInitial(baseIntelligence * 10);
    player->piety.setInitial(basePiety * 10);

    return(true);
}
//*********************************************************************
//                      checkAutomaticStats
//*********************************************************************
void PlayerClass::checkAutomaticStats() {
    if(baseStrength != -1 && baseDexterity != -1 && baseConstitution != -1 && baseIntelligence != -1 && basePiety != -1) {
        std::clog << "Automatic stats registered for " << this->getName() << std::endl;
        hasAutomaticStats = true;
    }
}

//*********************************************************************
//                      clearClasses
//*********************************************************************

void Config::clearClasses() {
    std::map<bstring, PlayerClass*>::iterator pcIt;

    for(pcIt = classes.begin() ; pcIt != classes.end() ; pcIt++) {
        //printf("Erasing class %s\n", ((*pcIt).first).c_str());
        PlayerClass* pClass = (*pcIt).second;
        delete pClass;
    }
    classes.clear();
}

//*********************************************************************
//                      dmShowClasses
//*********************************************************************

int dmShowClasses(Player* admin, cmd* cmnd) {

    bool    more = admin->isDm() && cmnd->num > 1 && !strcmp(cmnd->str[1], "more");
    bool    all = admin->isDm() && cmnd->num > 1 && !strcmp(cmnd->str[1], "all");

    SkillGain* sGain;
    bstring tmp;

    admin->printColor("Displaying Classes:%s\n",
        admin->isDm() ? "  Type ^y*classlist more^x to view more, ^y*classlist all^x to view all information." : "");

    for(auto& [clsName, pClass] : gConfig->classes) {
        admin->printColor("Id: ^c%-2d^x   Name: ^c%s\n", pClass->getId(), clsName.c_str());
        if(more || all) {
            std::ostringstream cStr;
            std::map<int, PlayerTitle*>::iterator tt;

            //cStr << "Class: " << (*cIt).first << "\n";
            cStr << "    Base: " << pClass->getBaseHp() << "hp, " << pClass->getBaseMp() << "mp, "
                 << "Dice: " << pClass->damage.str() << "\n"
                 << "    NumProfs: " << pClass->numProfs() << "   NeedsDeity: " << (pClass->needsDeity() ? "^gYes" : "^rNo") << "^x\n";
            std::list<SkillGain*>::const_iterator sgIt;
            if(all) {
                cStr << "    Skills: ";
                for(sgIt = pClass->getSkillBegin() ; sgIt != pClass->getSkillEnd() ; sgIt++) {
                    sGain = *sgIt;
                    cStr << gConfig->getSkillDisplayName(sGain->getName()) << "(" << sGain->getGained() << ") ";
                }
                std::map<int, LevelGain*>::const_iterator lgIt;

                cStr << "\n    Levels: ";
                for(lgIt = pClass->getLevelBegin() ; lgIt != pClass->getLevelEnd() ; lgIt ++) {
                    LevelGain* lGain = (*lgIt).second;
                    int lvl = (*lgIt).first;
                    if(!lGain) {
                        cStr << "\n ERROR: No level gain information found for " << pClass->getName() << " : " << lvl << ".";
                        continue;
                    }

                    cStr << "\n        " << lvl << ": " << lGain->getHp() << "HP, " << lGain->getMp() << "MP, St:" << lGain->getStatStr() << ", Sa:" << lGain->getSaveStr();
                    if(lGain->hasSkills()) {
                        cStr << "\n        Skills:";
                        for(sgIt = lGain->getSkillBegin() ; sgIt != lGain->getSkillEnd() ; sgIt++) {
                            sGain = *sgIt;
                            cStr << "\n            " << gConfig->getSkillDisplayName(sGain->getName()) << "(" << sGain->getGained() << ") ";
                        }
                    }
                }
                cStr << "\n    Titles: ";
                for(tt = pClass->titles.begin() ; tt != pClass->titles.end(); tt++) {
                    PlayerTitle* title = (*tt).second;
                    cStr << "\n        Level: ^c" << (*tt).first << "^x"
                        << "   Male: ^c" << title->getTitle(true) << "^x"
                        << "   Female: ^c" << title->getTitle(false) << "^x";
                }
            }
            cStr << "\n";
            tmp = cStr.str();
            *admin << ColorOn << cStr.str() << ColorOff;
            gServer->processOutput();
        }
    }
    return(0);
}
