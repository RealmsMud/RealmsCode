/*
 * raceData.cpp
 *	 Race data storage file
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
//						raceCount
//*********************************************************************

unsigned short Config::raceCount() {
	return(races.size());
}

//*********************************************************************
//						getPlayableRaceCount
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
//						RaceData
//*********************************************************************

RaceData::RaceData(xmlNodePtr rootNode) {
	Sex		sex;
	Dice	dice;
	char	sizeTxt[20];
	int		i=0;
	short	n=0;
	bstring str;

	id = 0;
	name = adjective = abbr = "";
	infra = bonus = 0;
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
	xml::copyPropToBString(name, rootNode, "name");

	xmlNodePtr subNode, childNode, curNode = rootNode->children;

	while(curNode) {
			 if(NODE_NAME(curNode, "Adjective")) xml::copyToBString(adjective, curNode);
		else if(NODE_NAME(curNode, "Abbr")) xml::copyToBString(abbr, curNode);
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
							xml::copyPropToBString(str, subNode, "id");
							xml::copyToNum(stats[gConfig->stattoNum(str)], subNode);
						}
						subNode = subNode->next;
					}
				}
				else if(NODE_NAME(childNode, "Classes")) {
					subNode = childNode->children;
					while(subNode) {
						if(NODE_NAME(subNode, "Class")) {
							xml::copyPropToBString(str, subNode, "id");
							classes[gConfig->classtoNum(str)] = true;
						}
						subNode = subNode->next;
					}
				}
				else if(NODE_NAME(childNode, "MultiClericDeities")) {
					subNode = childNode->children;
					while(subNode) {
						if(NODE_NAME(subNode, "Deity")) {
							xml::copyPropToBString(str, subNode, "id");
							multiClerDeities[gConfig->deitytoNum(str)] = true;
						}
						subNode = subNode->next;
					}
				}
				else if(NODE_NAME(childNode, "ClericDeities")) {
					subNode = childNode->children;
					while(subNode) {
						if(NODE_NAME(subNode, "Deity")) {
							xml::copyPropToBString(str, subNode, "id");
							clerDeities[gConfig->deitytoNum(str)] = true;
						}
						subNode = subNode->next;
					}
				}
				else if(NODE_NAME(childNode, "PaladinDeities")) {
					subNode = childNode->children;
					while(subNode) {
						if(NODE_NAME(subNode, "Deity")) {
							xml::copyPropToBString(str, subNode, "id");
							palDeities[gConfig->deitytoNum(str)] = true;
						}
						subNode = subNode->next;
					}
				}
				else if(NODE_NAME(childNode, "DeathknightDeities")) {
					subNode = childNode->children;
					while(subNode) {
						if(NODE_NAME(subNode, "Deity")) {
							xml::copyPropToBString(str, subNode, "id");
							dkDeities[gConfig->deitytoNum(str)] = true;
						}
						subNode = subNode->next;
					}
				}
				else if(NODE_NAME(childNode, "SavingThrows")) {
					subNode = childNode->children;
					while(subNode) {
						if(NODE_NAME(subNode, "SavingThrow")) {
							xml::copyPropToBString(str, subNode, "id");
							xml::copyToNum(saves[gConfig->savetoNum(str)], subNode);
						}
						subNode = subNode->next;
					}
				}
				else if(NODE_NAME(childNode, "Effects")) {
					subNode = childNode->children;
					while(subNode) {
						if(NODE_NAME(subNode, "Effect")) {
							xml::copyToBString(str, subNode);
							effects.push_back(str);
						}
						subNode = subNode->next;
					}
				} else if(NODE_NAME(childNode, "Skills")) {
					xmlNodePtr skillNode = childNode->children;
					while(skillNode) {
						if(NODE_NAME(skillNode, "Skill")) {
							SkillGain* skillGain = new SkillGain(skillNode);
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

//*********************************************************************
//						stattoNum
//*********************************************************************

int Config::stattoNum(bstring str) {
	if(str == "Str") return(STR);
	if(str == "Con") return(CON);
	if(str == "Dex") return(DEX);
	if(str == "Int") return(INT);
	if(str == "Pty") return(PTY);
	return(0);
}

//*********************************************************************
//						savetoNum
//*********************************************************************

int Config::savetoNum(bstring str) {
	if(str == "Poi") return(POI);
	if(str == "Dea") return(DEA);
	if(str == "Bre") return(BRE);
	if(str == "Men") return(MEN);
	if(str == "Spl") return(SPL);
	return(0);
}

//*********************************************************************
//						classtoNum
//*********************************************************************

int Config::classtoNum(bstring str) {
	if(str == "Assassin") return(ASSASSIN);
	if(str == "Berserker") return(BERSERKER);
	if(str == "Cleric") return(CLERIC);
	if(str == "Fighter") return(FIGHTER);
	if(str == "Mage") return(MAGE);
	if(str == "Paladin") return(PALADIN);
	if(str == "Ranger") return(RANGER);
	if(str == "Thief") return(THIEF);
	if(str == "Vampire") return(VAMPIRE);
	if(str == "Monk") return(MONK);
	if(str == "Deathknight") return(DEATHKNIGHT);
	if(str == "Druid") return(DRUID);
	if(str == "Lich") return(LICH);
	if(str == "Werewolf") return(WEREWOLF);
	if(str == "Bard") return(BARD);
	if(str == "Rogue") return(ROGUE);
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
//						racetoNum
//*********************************************************************

int Config::racetoNum(bstring str) {
	for(unsigned i=0; i<races.size(); i++) {
		if(!races[i])
			continue;
		if(str == races[i]->getName() || str == races[i]->getAdjective())
			return(i);
	}
	return(-1);
}

//*********************************************************************
//						deitytoNum
//*********************************************************************

int Config::deitytoNum(bstring str) {
	for(unsigned i=0; i<deities.size(); i++) {
		if(!deities[i])
			continue;
		if(str == deities[i]->getName())
			return(i);
	}
	return(-1);
}

//**********************************************************************
//						loadRaces
//**********************************************************************

bool Config::loadRaces() {
	xmlDocPtr xmlDoc;
	xmlNodePtr curNode;
	int		i=0;

	char filename[80];
	snprintf(filename, 80, "%s/races.xml", Path::Game);
	xmlDoc = xml::loadFile(filename, "Races");

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

	clearRaces();
	while(curNode != NULL) {
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
//						getRace
//**********************************************************************

const RaceData* Config::getRace(bstring race) const {
	std::map<int, RaceData*>::const_iterator it;
	RaceData* data=0;
	int len = race.getLength();
	race = race.toLower();

	for(it = races.begin() ; it != races.end() ; it++) {
		// exact match
		if((*it).second->getName().toLower() == race)
			return((*it).second);
		if((*it).second->getName().substr(0, len) == race) {
			// not unique
			if(data)
				return(0);
			data = (*it).second;
		}
	}

	return(data);
}

//**********************************************************************
//						getRace
//**********************************************************************

const RaceData* Config::getRace(int id) const {
	std::map<int, RaceData*>::const_iterator it = races.find(id);

	if(it == races.end())
		it = races.begin();

	return((*it).second);
}

//*********************************************************************
//						dmShowRaces
//*********************************************************************

int dmShowRaces(Player* player, cmd* cmnd) {
	std::map<int, RaceData*>::const_iterator it;
	RaceData* data=0;
	bool	all = player->isDm() && cmnd->num > 1 && !strcmp(cmnd->str[1], "all");
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
			if(data->baseHeight.size())
				player->print("\n");
			// variable weight for all genders
			for(dIt = data->varHeight.begin() ; dIt != data->varHeight.end() ; dIt++) {
				player->printColor("   VarHeight: %s: ^c%s", getSexName((*dIt).first).c_str(), (*dIt).second.str().c_str());
			}
			if(data->varHeight.size())
				player->print("\n");

			// base height for all genders
			for(bIt = data->baseWeight.begin() ; bIt != data->baseWeight.end() ; bIt++) {
				player->printColor("   BaseWeight: %s: ^c%d", getSexName((*bIt).first).c_str(), (*bIt).second);
			}
			if(data->baseWeight.size())
				player->print("\n");
			// variable height for all genders
			for(dIt = data->varWeight.begin() ; dIt != data->varWeight.end() ; dIt++) {
				player->printColor("   VarWeight: %s: ^c%s", getSexName((*dIt).first).c_str(), (*dIt).second.str().c_str());
			}
			if(data->varWeight.size())
				player->print("\n");
		}
	}
	player->print("\n");
	return(0);
}

//**********************************************************************
//						clearRaces
//**********************************************************************

void Config::clearRaces() {
	std::map<int, RaceData*>::iterator it;

	for(it = races.begin() ; it != races.end() ; it++) {
		RaceData* r = (*it).second;
		delete r;
	}
	races.clear();
}

//*********************************************************************
//						getAdjective
//*********************************************************************

bstring RaceData::getAdjective() const { return(adjective); }

//*********************************************************************
//						getAbbr
//*********************************************************************

bstring RaceData::getAbbr() const { return(abbr); }

//*********************************************************************
//						getParentRace
//*********************************************************************

int RaceData::getParentRace() const { return(parentRace); }

//*********************************************************************
//						getPorphyriaResistance
//*********************************************************************

short RaceData::getPorphyriaResistance() const { return(porphyriaResistance); }

//*********************************************************************
//						makeParent
//*********************************************************************

void RaceData::makeParent() { isParentRace = true; }

//*********************************************************************
//						isParent
//*********************************************************************

bool RaceData::isParent() const { return(isParentRace); }

//*********************************************************************
//						isPlayable
//*********************************************************************

bool RaceData::isPlayable() const { return(playable); }

//*********************************************************************
//						isGendered
//*********************************************************************

bool RaceData::isGendered() const { return(gendered); }

//*********************************************************************
//						hasInfravision
//*********************************************************************

bool RaceData::hasInfravision() const { return(infra); }

//*********************************************************************
//						bonusStat
//*********************************************************************

bool RaceData::bonusStat() const { return(bonus); }

//*********************************************************************
//						getSize
//*********************************************************************

Size RaceData::getSize() const { return(size); }

//*********************************************************************
//						getStartAge
//*********************************************************************

int RaceData::getStartAge() const { return(startAge); }

//*********************************************************************
//						getStatAdj
//*********************************************************************

int RaceData::getStatAdj(int stat) const { return(stats[stat]); }

//*********************************************************************
//						getSave
//*********************************************************************

int RaceData::getSave(int save) const { return(saves[save]); }

//*********************************************************************
//						allowedClass
//*********************************************************************

bool RaceData::allowedClass(int cls) const { return(classes[cls]); }

//*********************************************************************
//						allowedDeity
//*********************************************************************

bool RaceData::allowedDeity(int cls, int cls2, int dty) const {
	return(	(cls == CLERIC && !cls2 && allowedClericDeity(dty)) ||
			(cls == PALADIN && !cls2 && allowedPaladinDeity(dty)) ||
			(cls == DEATHKNIGHT && !cls2 && allowedDeathknightDeity(dty)) ||
			(cls == CLERIC && cls2 && allowedMultiClericDeity(dty))
	);
}

//*********************************************************************
//						allowedMultiClericDeity
//*********************************************************************

bool RaceData::allowedMultiClericDeity(int dty) const { return(multiClerDeities[dty]); }

//*********************************************************************
//						allowedClericDeity
//*********************************************************************

bool RaceData::allowedClericDeity(int dty) const { return(clerDeities[dty]); }

//*********************************************************************
//						allowedPaladinDeity
//*********************************************************************

bool RaceData::allowedPaladinDeity(int dty) const { return(palDeities[dty]); }

//*********************************************************************
//						allowedDeathknightDeity
//*********************************************************************

bool RaceData::allowedDeathknightDeity(int dty) const { return(dkDeities[dty]); }

//*********************************************************************
//						getId
//*********************************************************************

int RaceData::getId() const { return(id); }

//*********************************************************************
//						getName
//*********************************************************************

bstring RaceData::getName(int tryToShorten) const {
	if(!tryToShorten)
		return(name);
	bstring txt = name;
	if(txt.length() > (unsigned)tryToShorten)
		txt.Replace("Half-", "H-");
	return(txt);
}

//*********************************************************************
//						getSkillBegin
//*********************************************************************

std::list<SkillGain*>::const_iterator RaceData::getSkillBegin() const {
	return(baseSkills.begin());
}

//*********************************************************************
//						getSkillEnd
//*********************************************************************

std::list<SkillGain*>::const_iterator RaceData::getSkillEnd() const {
	return(baseSkills.end());
}
