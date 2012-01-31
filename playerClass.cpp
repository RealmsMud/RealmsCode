/*
 * playerClass.cpp
 *	 Player Class
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"


//*********************************************************************
//						PlayerClass
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

//*********************************************************************
//						load
//*********************************************************************

void PlayerClass::load(xmlNodePtr rootNode) {
//	// Stuff gained on each additional level
//	std::map<int, LevelGain*> levels;

	id = xml::getIntProp(rootNode, "id");
	xml::copyPropToBString(name, rootNode, "Name");

	xmlNodePtr curNode = rootNode->children;

	while(curNode) {
		if(NODE_NAME(curNode, "Title")) {
			titles[1] = new PlayerTitle;
			titles[1]->load(curNode);
		}
		else if(NODE_NAME(curNode, "UnarmedWeaponSkill")) xml::copyToBString(unarmedWeaponSkill, curNode);
		else if(NODE_NAME(curNode, "NumProfs")) xml::copyToNum(numProf, curNode);
		else if(NODE_NAME(curNode, "NeedsDeity")) {
			int i=0;
			xml::copyToNum(i, curNode);
			if(i)
				needDeity = true;
		} else if(NODE_NAME(curNode, "Base")) {
			xmlNodePtr baseNode = curNode->children;
			while(baseNode) {
				if(NODE_NAME(baseNode, "Stats")) {
					xmlNodePtr statNode = baseNode->children;
					while(statNode) {
						if(NODE_NAME(statNode, "Hp")) {
							xml::copyToNum(baseHp, statNode);
						} else if(NODE_NAME(statNode, "Mp")) {
							xml::copyToNum(baseMp, statNode);
						} else if(NODE_NAME(statNode, "Strength")) {
							xml::copyToNum(baseStrength, statNode);
							checkAutomaticStats();
						} else if(NODE_NAME(statNode, "Dexterity")) {
							xml::copyToNum(baseDexterity, statNode);
							checkAutomaticStats();
						} else if(NODE_NAME(statNode, "Constitution")) {
							xml::copyToNum(baseConstitution, statNode);
							checkAutomaticStats();
						} else if(NODE_NAME(statNode, "Intelligence")) {
							xml::copyToNum(baseIntelligence, statNode);
							checkAutomaticStats();
						} else if(NODE_NAME(statNode, "Piety")) {
							xml::copyToNum(basePiety, statNode);
							checkAutomaticStats();
						}
						statNode = statNode->next;
					}
				} else if(NODE_NAME(baseNode, "Dice")) damage.load(baseNode);
				else if(NODE_NAME(baseNode, "Skills")) {
					xmlNodePtr skillNode = baseNode->children;
					while(skillNode) {
						if(NODE_NAME(skillNode, "Skill")) {
							SkillGain* skillGain = new SkillGain(skillNode);
							baseSkills.push_back(skillGain);
						}
						skillNode = skillNode->next;
					}
				}
				baseNode = baseNode->next;
			}
		} else if(NODE_NAME(curNode, "Levels")) {
			xmlNodePtr levelNode = curNode->children;
			int lvl;
			while(levelNode) {
				if(NODE_NAME(levelNode, "Level")) {
					lvl = xml::getIntProp(levelNode, "Num");
					if(lvl > 0) {
						levels[lvl] = new LevelGain(levelNode);
					}

					xmlNodePtr childNode = levelNode->children;
					while(childNode) {
						if(NODE_NAME(childNode, "Title")) {
							titles[lvl] = new PlayerTitle;
							titles[lvl]->load(childNode);
						}
						childNode = childNode->next;
					}
				}
				levelNode = levelNode->next;
			}
		}

		curNode = curNode->next;

	}
}

//*********************************************************************
//						getId
//*********************************************************************

int PlayerClass::getId() const { return(id); }

//*********************************************************************
//						getName
//*********************************************************************

bstring PlayerClass::getName() const { return(name); }

//*********************************************************************
//						getUnarmedWeaponSkill
//*********************************************************************

bstring PlayerClass::getUnarmedWeaponSkill() const { return(unarmedWeaponSkill); }

//*********************************************************************
//						getSkillBegin
//*********************************************************************

std::list<SkillGain*>::const_iterator PlayerClass::getSkillBegin() { return(baseSkills.begin()); }

//*********************************************************************
//						getSkillEnd
//*********************************************************************

std::list<SkillGain*>::const_iterator PlayerClass::getSkillEnd() { return(baseSkills.end()); }

//*********************************************************************
//						getLevelBegin
//*********************************************************************

std::map<int, LevelGain*>::const_iterator PlayerClass::getLevelBegin() { return(levels.begin()); }

//*********************************************************************
//						getLevelEnd
//*********************************************************************

std::map<int, LevelGain*>::const_iterator PlayerClass::getLevelEnd() { return(levels.end()); }

//*********************************************************************
//						getBaseHp
//*********************************************************************

short PlayerClass::getBaseHp() { return(baseHp); }

//*********************************************************************
//						getBaseMp
//*********************************************************************

short PlayerClass::getBaseMp() { return(baseMp); }

//*********************************************************************
//						needsDeity
//*********************************************************************

bool PlayerClass::needsDeity() { return(needDeity); }

//*********************************************************************
//						numProfs
//*********************************************************************

short PlayerClass::numProfs() { return(numProf); }

//*********************************************************************
//						getLevelGain
//*********************************************************************

LevelGain* PlayerClass::getLevelGain(int lvl) { return(levels[lvl]); }

//*********************************************************************
//						hasDefaultStats
//*********************************************************************
bool PlayerClass::hasDefaultStats() { return(hasAutomaticStats); }

//*********************************************************************
//						setDefaultStats
//*********************************************************************
bool PlayerClass::setDefaultStats(Player* player) {
	if(!hasDefaultStats() || !player)
		return(false);

	player->strength.setMax(baseStrength * 10);
	player->dexterity.setMax(baseDexterity * 10);
	player->constitution.setMax(baseConstitution * 10);
	player->intelligence.setMax(baseIntelligence * 10);
	player->piety.setMax(basePiety * 10);

}
//*********************************************************************
//						checkAutomaticStats
//*********************************************************************
void PlayerClass::checkAutomaticStats() {
	if(baseStrength != -1 && baseDexterity != -1 && baseConstitution != -1 && baseIntelligence != -1 && basePiety != -1) {
		std::cout << "Automatic stats registered for " << this->getName() << std::endl;
		hasAutomaticStats = true;
	}
}
//*********************************************************************
//						loadClasses
//*********************************************************************

bool Config::loadClasses() {
	xmlDocPtr xmlDoc;
	xmlNodePtr curNode;

	char filename[80];
	snprintf(filename, 80, "%s/classes.xml", Path::Game);
	xmlDoc = xml::loadFile(filename, "Classes");

	if(xmlDoc == NULL)
		return(false);

	curNode = xmlDocGetRootElement(xmlDoc);

	curNode = curNode->children;
	while(curNode && xmlIsBlankNode(curNode))
		curNode = curNode->next;

	if(curNode == 0) {
		xmlFreeDoc(xmlDoc);
		return(false);
	}

	clearClasses();
	bstring className = "";
	while(curNode != NULL) {
		if(NODE_NAME(curNode, "Class")) {
			xml::copyPropToBString( className, curNode, "Name");
			if(className != "") {
				if(classes.find(className) == classes.end()) {
					//printf("\n\tLoaded %s", className.c_str());
					classes[className] = new PlayerClass(curNode);
				} else {
					printf("Error: Duplicate class: %s\n", className.c_str());
				}
			}
		}
		curNode = curNode->next;
	}
	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	//printf("\n");
	return(true);
}

//*********************************************************************
//						clearClasses
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
//						dmShowClasses
//*********************************************************************

int dmShowClasses(Player* admin, cmd* cmnd) {

	bool	more = admin->isDm() && cmnd->num > 1 && !strcmp(cmnd->str[1], "more");
	bool	all = admin->isDm() && cmnd->num > 1 && !strcmp(cmnd->str[1], "all");

	std::map<bstring, PlayerClass*>::iterator cIt;
	PlayerClass* pClass;
	SkillGain* sGain;
	bstring tmp;

	admin->printColor("Displaying Classes:%s\n",
		admin->isDm() ? "  Type ^y*classlist more^x to view more, ^y*classlist all^x to view all information." : "");
	for(cIt = gConfig->classes.begin() ; cIt != gConfig->classes.end() ; cIt++) {
		pClass = (*cIt).second;
		admin->printColor("Id: ^c%-2d^x   Name: ^c%s\n", pClass->getId(), (*cIt).first.c_str());
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
			admin->printColor("%s", tmp.c_str());
			gServer->processOutput();
		}
	}
	return(0);
}
