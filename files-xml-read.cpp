/*
 * files-xml-read.cpp
 *	 Used to read objects/rooms/creatures etc from xml files
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
/*			Web Editor
 * 			 _   _  ____ _______ ______
 *			| \ | |/ __ \__   __|  ____|
 *			|  \| | |  | | | |  | |__
 *			| . ` | |  | | | |  |  __|
 *			| |\  | |__| | | |  | |____
 *			|_| \_|\____/  |_|  |______|
 *
 * 		If you change anything here, make sure the changes are reflected in the web
 * 		editor! Either edit the PHP yourself or tell Dominus to make the changes.
 */
#include "mud.h"
#include "effects.h"
#include "bans.h"
#include "guilds.h"
#include "factions.h"
#include "version.h"
#include "specials.h"
#include "calendar.h"
#include "quests.h"
#include "unique.h"
#include "alchemy.h"
#include "mxp.h"
#include "socials.h"

extern int objRefSaveFlags[];

// TODO: Make a function that will use an xml reader to only parse the player to find
// name and password

//*********************************************************************
//						loadPlayer
//*********************************************************************
// Attempt to load the player named 'name' into the address given
// return 0 on success, -1 on failure

bool loadPlayer(const bstring name, Player** player, LoadType loadType) {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	char		filename[256];
	bstring		pass = "", loadName = "";

	if(!checkWinFilename(NULL, name)) {
		printf("Failed lookup on player %s due to checkWinFilename.\n", name.c_str());
		return(false);
	}

	if(loadType == LS_BACKUP)
		sprintf(filename, "%s/%s.bak.xml", Path::PlayerBackup, name.c_str());
	else if(loadType == LS_CONVERT)
		sprintf(filename, "%s/convert/%s.xml", Path::Player, name.c_str());
	else // LS_NORMAL
		sprintf(filename, "%s/%s.xml", Path::Player, name.c_str());

	//printf("Attempting to load player %s from file %s.\n", name, filename);

	if((xmlDoc = xml::loadFile(filename, "Player")) == NULL)
		return(false);

	rootNode = xmlDocGetRootElement(xmlDoc);
	loadName = xml::getProp(rootNode, "Name");
	if(loadName != name) {
		printf("Error loading %s, found %s instead!\n", name.c_str(), loadName.c_str());
		xmlFreeDoc(xmlDoc);
		xmlCleanupParser();
		return(false);
	}

	*player = new Player;
	if(!*player)
		merror("loadPlayer", FATAL);

//	strcpy((*player)->name, tmpStr);
	xml::copyPropToCString((*player)->name, rootNode, "Name");
	xml::copyPropToBString(pass, rootNode, "Password");
	(*player)->setPassword(pass);
	//xml::copyPropToBString((*player)->version, rootNode, "Version");
	(*player)->setLastLogin(xml::getIntProp(rootNode, "LastLogin"));

	// If we get here, we should be loading the correct player, so start reading them in
	(*player)->readFromXml(rootNode);

	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	return(true);
}

//*********************************************************************
//						loadMonster
//*********************************************************************

bool loadMonster(int index, Monster ** pMonster) {
	CatRef cr;
	cr.id = index;
	return(loadMonster(cr, pMonster));
}

bool loadMonster(const CatRef cr, Monster ** pMonster) {
	if(!validMobId(cr))
		return(false);

	// Check if monster is already loaded, and if so return pointer
	if(gConfig->monsterInQueue(cr)) {
		gConfig->frontMonsterQueue(cr);
		*pMonster = new Monster;
		gConfig->getMonsterQueue(cr, pMonster);
	} else {
		// Otherwise load the monster and return a pointer to the newly loaded monster
		// Load the creature from it's file
		if(!loadMonsterFromFile(cr, pMonster))
			return(false);
		gConfig->addMonsterQueue(cr, pMonster);
	}

	(*pMonster)->fd = -1;
	(*pMonster)->lasttime[LT_TICK].ltime =
	(*pMonster)->lasttime[LT_TICK_SECONDARY].ltime =
	(*pMonster)->lasttime[LT_TICK_HARMFUL].ltime = time(0);

	(*pMonster)->lasttime[LT_TICK].interval  =
	(*pMonster)->lasttime[LT_TICK_SECONDARY].interval = 60;
	(*pMonster)->lasttime[LT_TICK_HARMFUL].interval = 30;
	(*pMonster)->getMobSave();

	return(true);
}

//*********************************************************************
//						loadObject
//*********************************************************************

bool loadObject(int index, Object** pObject) {
	CatRef cr;
	cr.id = index;
	return(loadObject(cr, pObject));
}

bool loadObject(const CatRef cr, Object** pObject) {
	if(!validObjId(cr))
		return(false);

	// Check if object is already loaded, and if so return pointer
	if(gConfig->objectInQueue(cr)) {
		gConfig->frontObjectQueue(cr);

		*pObject = new Object;
		gConfig->getObjectQueue(cr, pObject);
	}
	// Otherwise load the object and return a pointer to the newly loaded object
	else {
		// Load the object from it's file
		if(!loadObjectFromFile(cr, pObject))
			return(false);
		gConfig->addObjectQueue(cr, pObject);
	}


	if(pObject) {
		// Quest items are now auto NO-DROP
		if((*pObject)->getQuestnum()) {
			(*pObject)->setFlag(O_NO_DROP);
		}
		// cannot steal scrolls
		if((*pObject)->getType() == SCROLL) {
			(*pObject)->setFlag(O_NO_STEAL);
		}
	}
	return(true);
}

//*********************************************************************
//						loadRoom
//*********************************************************************

bool loadRoom(int index, UniqueRoom **pRoom) {
	CatRef	cr;
	cr.id = index;
	return(loadRoom(cr, pRoom));
}

bool loadRoom(const CatRef cr, UniqueRoom **pRoom) {
	if(!validRoomId(cr))
		return(false);

	// If the room is already loaded, return it
	if(gConfig->roomInQueue(cr)) {
		gConfig->frontRoomQueue(cr);
		gConfig->getRoomQueue(cr, pRoom);
	} else {
		// Otherwise load the room and return it
		if(!loadRoomFromFile(cr, pRoom))
			return(false);
		gConfig->addRoomQueue(cr, pRoom);
	}
	return(true);
}

//*********************************************************************
//						loadMonsterFromFile
//*********************************************************************

bool loadMonsterFromFile(const CatRef cr, Monster **pMonster, bstring filename) {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	int		num=0;

	if(filename == "")
		filename = monsterPath(cr);
	//printf("Attempting to load creature %d from %s\n", index, filename);

	if((xmlDoc = xml::loadFile(filename.c_str(), "Creature")) == NULL)
		return(false);

	if(xmlDoc == NULL) {
		printf("Error parsing file %s\n", filename.c_str());
		return(false);
	}
	rootNode = xmlDocGetRootElement(xmlDoc);
	num = xml::getIntProp(rootNode, "Num");

	if(cr.id == -1 || num == cr.id) {
		*pMonster = new Monster;
		if(!*pMonster)
			merror("loadMonsterFromFile", FATAL);
		(*pMonster)->setVersion(rootNode);

		(*pMonster)->readFromXml(rootNode);
		if((*pMonster)->flagIsSet(M_TALKS)) {
			loadCreature_tlk((*pMonster));
			(*pMonster)->convertOldTalks();
		}
	}

	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	return(true);
}

//*********************************************************************
//						loadObjectFromFile
//*********************************************************************

bool loadObjectFromFile(const CatRef cr, Object** pObject) {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	int			num;
	char		filename[256];

	sprintf(filename, "%s", objectPath(cr));

	//printf("Attempting to load object %d from %s\n", index, filename);

	if((xmlDoc = xml::loadFile(filename, "Object")) == NULL)
		return(false);

	if(xmlDoc == NULL) {
		printf("Error parsing file %s\n", filename);
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

		(*pObject)->readFromXml(rootNode);
	}

	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	return(true);
}

//*********************************************************************
//						loadRoomFromFile
//*********************************************************************
// if we're loading only from a filename, get CatRef from file

bool loadRoomFromFile(const CatRef cr, UniqueRoom **pRoom, bstring filename) {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	int			num;

	if(filename == "")
		filename = roomPath(cr);

	//printf("Attempting to load room %d from file %s.\n", index, filename);
	if((xmlDoc = xml::loadFile(filename.c_str(), "Room")) == NULL)
		return(false);

	rootNode = xmlDocGetRootElement(xmlDoc);
	num = xml::getIntProp(rootNode, "Num");

	if(cr.id == -1 || num == cr.id) {
		*pRoom = new UniqueRoom;
		if(!*pRoom)
			merror("loadRoomFromFile", FATAL);
		(*pRoom)->setVersion(xml::getProp(rootNode, "Version"));

		(*pRoom)->readFromXml(rootNode);
	}
	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	return(true);
}

//*********************************************************************
//						readFromXml
//*********************************************************************
// Reads a creature from the given xml document and root node

int convertProf(Creature* player, Realm realm) {
	int skill = player->getLevel()*7 + player->getLevel()*3 * mprofic(player, realm) / 100 - 5;
	skill = MAX(0, skill);
	return(skill);
}

int Creature::readFromXml(xmlNodePtr rootNode) {
	xmlNodePtr curNode;
	int i;
	unsigned short c = 0;

	Player *pPlayer = getAsPlayer();
	Monster *mMonster = getAsMonster();

	if(mMonster) {
		mMonster->info.load(rootNode);
		mMonster->info.id = xml::getIntProp(rootNode, "Num");
	}
	xml::copyPropToBString(version, rootNode, "Version");

	curNode = rootNode->children;
	// Start reading stuff in!

	while(curNode) {
		// Name will only be loaded for Monsters
			 if(NODE_NAME(curNode, "Name")) xml::copyToCString(name, curNode);
		else if(NODE_NAME(curNode, "Id")) setId(xml::getBString(curNode));
		else if(NODE_NAME(curNode, "Description")) xml::copyToBString(description, curNode);
		else if(NODE_NAME(curNode, "Keys")) {
			loadStringArray(curNode, key, CRT_KEY_LENGTH, "Key", 3);
		}
		else if(NODE_NAME(curNode, "MoveTypes")) {
			loadStringArray(curNode, movetype, CRT_MOVETYPE_LENGTH, "MoveType", 3);
		}
		else if(NODE_NAME(curNode, "Level")) setLevel(xml::toNum<unsigned short>(curNode), true);
		else if(NODE_NAME(curNode, "Type")) setType(xml::toNum<unsigned short>(curNode));
		else if(NODE_NAME(curNode, "RoomNum")&& getVersion() < "2.34" ) xml::copyToNum(currentLocation.room.id, curNode);
		else if(NODE_NAME(curNode, "Room")) currentLocation.room.load(curNode);

		else if(NODE_NAME(curNode, "Race")) setRace(xml::toNum<unsigned short>(curNode));
		else if(NODE_NAME(curNode, "Class")) c = xml::toNum<unsigned short>(curNode);
		else if(NODE_NAME(curNode, "AttackPower")) setAttackPower(xml::toNum<unsigned int>(curNode));
		else if(NODE_NAME(curNode, "DefenseSkill")) {
			if(mMonster) {
				mMonster->setDefenseSkill(xml::toNum<int>(curNode));
			}
		}
		else if(NODE_NAME(curNode, "WeaponSkill")) {
			if(mMonster) {
				mMonster->setWeaponSkill(xml::toNum<int>(curNode));
			}
		}
		else if(NODE_NAME(curNode, "Class2")) {
			if(pPlayer) {
				pPlayer->setSecondClass(xml::toNum<unsigned short>(curNode));
			} else if(mMonster) {
				// TODO: Dom: for compatability, remove when possible
				mMonster->setMobTrade(xml::toNum<unsigned short>(curNode));
			}
		}
		else if(NODE_NAME(curNode, "Alignment")) setAlignment(xml::toNum<short>(curNode));
		else if(NODE_NAME(curNode, "Armor")) setArmor(xml::toNum<unsigned int>(curNode));
		else if(NODE_NAME(curNode, "Experience")) setExperience(xml::toNum<unsigned long>(curNode));

		else if(NODE_NAME(curNode, "Deity")) setDeity(xml::toNum<unsigned short>(curNode));

		else if(NODE_NAME(curNode, "Clan")) setClan(xml::toNum<unsigned short>(curNode));
		else if(NODE_NAME(curNode, "PoisonDuration")) setPoisonDuration(xml::toNum<unsigned short>(curNode));
		else if(NODE_NAME(curNode, "PoisonDamage")) setPoisonDamage(xml::toNum<unsigned short>(curNode));
		else if(NODE_NAME(curNode, "CurrentLanguage")) xml::copyToNum(current_language, curNode);
		else if(NODE_NAME(curNode, "Coins")) coins.load(curNode);
		else if(NODE_NAME(curNode, "Realms")) {
			xml::loadNumArray<unsigned long>(curNode, realm, "Realm", MAX_REALM-1);
		}
		else if(NODE_NAME(curNode, "Proficiencies")) {
			long proficiency[6];
			zero(proficiency, sizeof(proficiency));
			xml::loadNumArray<long>(curNode, proficiency, "Proficiency", 6);

			// monster jail rooms shouldn't be stored here
			if(mMonster && mMonster->getVersion() < "2.42b") {
				mMonster->setCastChance(proficiency[0]);
				mMonster->rescue[0].setArea("misc");
				mMonster->rescue[0].id = proficiency[1];
				mMonster->rescue[1].setArea("misc");
				mMonster->rescue[1].id = proficiency[2];
				mMonster->setMaxLevel(proficiency[3]);
				mMonster->jail.setArea("misc");
				mMonster->jail.id = proficiency[4];
			}
		}
		else if(NODE_NAME(curNode, "Dice")) damage.load(curNode);
		else if(NODE_NAME(curNode, "Skills")) {
			loadSkills(curNode);
		}
		else if(NODE_NAME(curNode, "Factions")) {
			loadFactions(curNode);
		}
		else if(NODE_NAME(curNode, "Effects")) {
			effects.load(curNode, this);
		}
		else if(NODE_NAME(curNode, "SpecialAttacks")) {
			loadAttacks(curNode);
		}
		else if(NODE_NAME(curNode, "Stats")) {
			loadStats(curNode);
		}
		else if(NODE_NAME(curNode, "Flags")) {
			// Clear flags before loading incase we're loading a reference creature
			for(i=0; i<CRT_FLAG_ARRAY_SIZE; i++)
				flags[i] = 0;
			loadBits(curNode, flags);
		}
		else if(NODE_NAME(curNode, "Spells")) {
			loadBits(curNode, spells);
		}
		else if(NODE_NAME(curNode, "Quests")) {
			loadBits(curNode, quests);
		}
		else if(NODE_NAME(curNode, "Languages")) {
			loadBits(curNode, languages);
		}
		else if(NODE_NAME(curNode, "DailyTimers")) {
			loadDailys(curNode, daily);
		}
		else if(NODE_NAME(curNode, "LastTimes")) {
			loadLastTimes(curNode, lasttime);
		}
		else if(NODE_NAME(curNode, "SavingThrows")) {
			loadSavingThrows(curNode, saves);
		}
		else if(NODE_NAME(curNode, "Inventory")) {
			readObjects(curNode);
		}
		else if(NODE_NAME(curNode, "Pets")) {
			readCreatures(curNode);
		}
		else if(NODE_NAME(curNode, "AreaRoom")) gConfig->areaInit(this, curNode);
		else if(NODE_NAME(curNode, "Size")) setSize(whatSize(xml::toNum<int>(curNode)));
		else if(NODE_NAME(curNode, "Hooks")) hooks.load(curNode);


		// code for only players
		else if(pPlayer) pPlayer->readXml(curNode);

		// code for only monsters
		else if(mMonster) mMonster->readXml(curNode);

		curNode = curNode->next;
	}

	// run this here so effects are added properly
	setClass(c);

	convertOldEffects();


	if(isPlayer()) {
		if(getVersion() < "2.46l") {
			pPlayer->upgradeStats();
		}
		if(getVersion() < "2.46k" && knowsSkill("endurance")) {
			remSkill("endurance");
			#define P_RUNNING_OLD 56
			clearFlag(P_RUNNING_OLD);
		}

		if(getVersion() < "2.43" && getClass() != BERSERKER) {
			int skill = level;
			if(isPureCaster() || isHybridCaster() || isStaff()) {
				skill *= 10;
			} else {
				skill *= 7;
			}
			skill -= 5;
			skill = MAX(0, skill);

			addSkill("abjuration", skill);
			addSkill("conjuration", skill);
			addSkill("divination", skill);
			addSkill("enchantment", skill);
			addSkill("evocation", skill);
			addSkill("illusion", skill);
			addSkill("necromancy", skill);
			addSkill("translocation", skill);
			addSkill("transmutation", skill);

			addSkill("fire", convertProf(this, FIRE));
			addSkill("water", convertProf(this, WATER));
			addSkill("earth", convertProf(this, EARTH));
			addSkill("air", convertProf(this, WIND));
			addSkill("cold", convertProf(this, COLD));
			addSkill("electric", convertProf(this, ELEC));
		}
		if(getVersion() < "2.45c" && getCastingType() == Divine) {
			int skill = level;
			if(isPureCaster() || isHybridCaster() || isStaff()) {
				skill *= 10;
			} else {
				skill *= 7;
			}
			skill -= 5;
			skill = MAX(0, skill);

			addSkill("healing", skill);
			addSkill("destruction", getSkillGained("evocation"));
			addSkill("evil", getSkillGained("necromancy"));
			addSkill("knowledge", getSkillGained("divination"));
			addSkill("protection", getSkillGained("abjuration"));
			addSkill("augmentation", getSkillGained("transmutation"));
			addSkill("travel", getSkillGained("translocation"));
			addSkill("creation", getSkillGained("conjuration"));
			addSkill("trickery", getSkillGained("illusion"));
			addSkill("good", skill);
			addSkill("nature", skill);

			remSkill("abjuration");
			remSkill("conjuration");
			remSkill("divination");
			remSkill("enchantment");
			remSkill("evocation");
			remSkill("illusion");
			remSkill("necromancy");
			remSkill("translocation");
			remSkill("transmutation");
		}
		if(getClass() == LICH && getVersion() < "2.43b" && !spellIsKnown(S_SAP_LIFE))
			learnSpell(S_SAP_LIFE);
	}

	setVersion();


	if(size == NO_SIZE && race)
		size = gConfig->getRace(race)->getSize();

	escapeText();
	return(0);
}

//*********************************************************************
//						readXml
//*********************************************************************

void Monster::readXml(xmlNodePtr curNode) {
	xmlNodePtr childNode;

	if(NODE_NAME(curNode, "Plural")) xml::copyToBString(plural, curNode);
	else if(NODE_NAME(curNode, "CarriedItems")) {
		loadCarryArray(curNode, carry, "Carry", 10);
	}
	else if(NODE_NAME(curNode, "LoadAggro")) setLoadAggro(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "LastMod")) xml::copyToCString(last_mod, curNode);

	// get rid of these after conversion
	else if(NODE_NAME(curNode, "StorageRoom") && getVersion() < "2.34")
		setLoadAggro(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "BoundRoom") && getVersion() < "2.34")
		setSkillLevel(xml::toNum<int>(curNode));


	else if(NODE_NAME(curNode, "SkillLevel")) setSkillLevel(xml::toNum<int>(curNode));
	else if(NODE_NAME(curNode, "ClassAggro")) {
		loadBits(curNode, cClassAggro);
	}
	else if(NODE_NAME(curNode, "RaceAggro")) {
		loadBits(curNode, raceAggro);
	}
	else if(NODE_NAME(curNode, "DeityAggro")) {
		loadBits(curNode, deityAggro);
	}
	else if(NODE_NAME(curNode, "Attacks")) {
		loadStringArray(curNode, attack, CRT_ATTACK_LENGTH, "Attack", 3);
	}
	else if(NODE_NAME(curNode, "Talk")) xml::copyToBString(talk, curNode);
	else if(NODE_NAME(curNode, "TalkResponses")) {
		childNode = curNode->children;
		TalkResponse* newTalk;
		while(childNode) {
			if(NODE_NAME(childNode, "TalkResponse")) {
				if((newTalk = new TalkResponse(childNode)) != NULL) {
					responses.push_back(newTalk);
				}
			}
			childNode = childNode->next;
		}

	}
	else if(NODE_NAME(curNode, "TradeTalk")) xml::copyToCString(ttalk, curNode);
	else if(NODE_NAME(curNode, "NumWander")) setNumWander(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "MagicResistance")) setMagicResistance(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "MobTrade")) setMobTrade(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "AssistMobs")) {
		loadCatRefArray(curNode, assist_mob, "Mob", NUM_ASSIST_MOB);
	}
	else if(NODE_NAME(curNode, "EnemyMobs")) {
		loadCatRefArray(curNode, enemy_mob, "Mob", NUM_ENEMY_MOB);
	}
	else if(NODE_NAME(curNode, "PrimeFaction")) {
		xml::copyToBString(primeFaction, curNode);
	}
	else if(NODE_NAME(curNode, "AggroString")) xml::copyToCString(aggroString, curNode);
	else if(NODE_NAME(curNode, "MaxLevel")) setMaxLevel(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "Cast")) setCastChance(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "Jail")) jail.load(curNode);
	else if(NODE_NAME(curNode, "Rescue")) loadCatRefArray(curNode, rescue, "Mob", NUM_RESCUE);

	else if(NODE_NAME(curNode, "UpdateAggro")) setUpdateAggro(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "PkIn") && getVersion() < "2.42b") setUpdateAggro(xml::toNum<unsigned short>(curNode));

	// Now handle version changes

	else if(getVersion() < "2.21") {
		//std::cout << "Loading mob pre version 2.21" << std::endl;
		// Title was changed to AggroString as of 2.21
		if(NODE_NAME(curNode, "Title")) xml::copyToCString(aggroString, curNode);
	}

}

//*********************************************************************
//						readXml
//*********************************************************************

void Player::readXml(xmlNodePtr curNode) {
	xmlNodePtr childNode;

	if(NODE_NAME(curNode, "Birthday")) {
		birthday = new cDay;
		birthday->load(curNode);
	}
	else if(NODE_NAME(curNode, "BoundRoom")) {
		if(getVersion() < "2.42h")
			bound.room.load(curNode);
		else
			bound.load(curNode);
	}
	else if(NODE_NAME(curNode, "PreviousRoom")) previousRoom.load(curNode); // monster do not save PreviousRoom
	else if(NODE_NAME(curNode, "Statistics")) statistics.load(curNode);

	else if(NODE_NAME(curNode, "Bank")) bank.load(curNode);
	else if(NODE_NAME(curNode, "Surname")) xml::copyToBString(surname, curNode);
	else if(NODE_NAME(curNode, "Wrap")) xml::copyToNum(wrap, curNode);
	else if(NODE_NAME(curNode, "Forum")) xml::copyToBString(forum, curNode);
	else if(NODE_NAME(curNode, "Ranges"))
		loadRanges(curNode, this);
	else if(NODE_NAME(curNode, "Wimpy")) setWimpy(xml::toNum<unsigned short>(curNode));

	else if(NODE_NAME(curNode, "Songs")) {
		loadBits(curNode, songs);
	}
	else if(NODE_NAME(curNode, "Anchors")) {
		loadAnchors(curNode);
	}

	else if(NODE_NAME(curNode, "LastPassword")) xml::copyToBString(lastPassword, curNode);
	else if(NODE_NAME(curNode, "PoisonedBy")) xml::copyToBString(poisonedBy, curNode);
	else if(NODE_NAME(curNode, "AfflictedBy")) xml::copyToBString(afflictedBy, curNode);
	else if(NODE_NAME(curNode, "ActualLevel")) setActualLevel(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "WeaponTrains")) setWeaponTrains(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "LastMod")) xml::copyToBString(oldCreated, curNode);
	else if(NODE_NAME(curNode, "Created")) {
		if(getVersion() < "2.43c")
			xml::copyToBString(oldCreated, curNode);
		else
			xml::copyToNum(created, curNode);
	}
	else if(NODE_NAME(curNode, "Guild")) setGuild(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "GuildRank")) setGuildRank(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "TickDmg")) setTickDamage(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "NegativeLevels")) setNegativeLevels(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "LastInterest")) setLastInterest(xml::toNum<long>(curNode));
	else if(NODE_NAME(curNode, "Title")) setTitle(xml::getBString(curNode));
	else if(NODE_NAME(curNode, "CustomColors")) xml::copyToCString(customColors, curNode);
	else if(NODE_NAME(curNode, "Thirst")) setThirst(xml::toNum<unsigned short>(curNode));
	else if(NODE_NAME(curNode, "RoomExp")) {
		childNode = curNode->children;
		while(childNode) {
			if(NODE_NAME(childNode, "Room")) {
				CatRef cr;
				cr.load(childNode);
				roomExp.push_back(cr);
			}
			childNode = childNode->next;
		}
	}
	else if(NODE_NAME(curNode, "ObjIncrease")) {
		childNode = curNode->children;
		while(childNode) {
			if(NODE_NAME(childNode, "Object")) {
				CatRef cr;
				cr.load(childNode);
				objIncrease.push_back(cr);
			}
			childNode = childNode->next;
		}
	}
	else if(NODE_NAME(curNode, "StoresRefunded")) {\
        childNode = curNode->children;
        while(childNode) {
            if(NODE_NAME(childNode, "Store")) {
                CatRef cr;
                cr.load(childNode);
                storesRefunded.push_back(cr);
            }
            childNode = childNode->next;
        }
	}
	else if(NODE_NAME(curNode, "Lore")) {
		childNode = curNode->children;
		while(childNode) {
			if(NODE_NAME(childNode, "Info")) {
				CatRef cr;
				cr.load(childNode);
				lore.push_back(cr);
			}
			childNode = childNode->next;
		}
	}
	else if(NODE_NAME(curNode, "Recipes")) {
		childNode = curNode->children;
		while(childNode) {
			if(NODE_NAME(childNode, "Recipe")) {
				int i=0;
				xml::copyToNum(i, childNode);
				recipes.push_back(i);
			}
			childNode = childNode->next;
		}
	}
	else if(NODE_NAME(curNode, "QuestsInProgress")) {
		childNode = curNode->children;
		QuestCompletion* qc;
		while(childNode) {
			if(NODE_NAME(childNode, "QuestCompletion")) {
				qc = new QuestCompletion(childNode, this);
				questsInProgress[qc->getParentQuest()->getId()] = qc;
			}
			childNode = childNode->next;
		}
	}
	else if(NODE_NAME(curNode, "QuestsCompleted")) {
		childNode = curNode->children;
		if(getVersion() < "2.45") {
			// Old format
			while(childNode) {
				if(NODE_NAME(childNode, "Quest")) {
					int questNum = xml::toNum<int>(childNode);
					questsCompleted.insert(std::pair<int,int>(questNum,1));
				}
				childNode = childNode->next;
			}
		} else {
			// New Format
			xmlNodePtr subNode;
			 while(childNode) {
				if(NODE_NAME(childNode, "Quest")) {
					int questNum = -1;
					int numCompletions = 1;
					subNode = childNode->children;
					while(subNode) {
						if(NODE_NAME(subNode, "Id")) {
							questNum = xml::toNum<int>(subNode);
						} else if(NODE_NAME(subNode, "Num")) {
							numCompletions = xml::toNum<int>(subNode);
						}
						if(questNum != -1) {
							questsCompleted.insert(std::pair<int,int>(questNum,numCompletions));
						}
						subNode = subNode->next;
					}
				}
				childNode = childNode->next;
			}
		}
	}
	else if(NODE_NAME(curNode, "Minions")) {
		childNode = curNode->children;
		while(childNode) {
			if(NODE_NAME(childNode, "Minion")) {
				minions.push_back(xml::getBString(childNode));
			}
			childNode = childNode->next;
		}
	}
	else if(getVersion() < "2.42i") {
			 if(NODE_NAME(curNode, "PkWon")) statistics.setPkwon(xml::toNum<unsigned long>(curNode));
		else if(NODE_NAME(curNode, "PkIn")) statistics.setPkin(xml::toNum<unsigned long>(curNode));
	} else if(getVersion() < "2.46l") {
	    if(NODE_NAME(curNode, "LostExperience")) statistics.setExperienceLost(xml::toNum<unsigned long>(curNode));
	}
}

//*********************************************************************
//						load
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
	if(name == "" || gained == 0) {
		throw(new std::exception());
	}
}


//*********************************************************************
//						loadFaction
//*********************************************************************

bool Creature::loadFaction(xmlNodePtr rootNode) {
	bstring name = "";
	int			regard=0;
	xmlNodePtr curNode = rootNode->children;
	while(curNode) {
		if(NODE_NAME(curNode, "Name")) {
			xml::copyToBString(name, curNode);
		} else if(NODE_NAME(curNode, "Regard")) {
			xml::copyToNum(regard, curNode);
		}
		curNode = curNode->next;
	}
	// players; load faction even if 0
	if(name != "" && (regard != 0 || isPlayer())) {
		factions[name] = regard;
		return(true);
	}
	return(false);
}

//*********************************************************************
//						loadSkills
//*********************************************************************

void Creature::loadSkills(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	while(curNode) {
		if(NODE_NAME(curNode, "Skill")) {
			try {
				Skill *skill = new Skill(curNode);
				skills.insert(SkillMap::value_type(skill->getName(), skill));
			} catch(...) {
				std::cout << "Error loading skill for " << getName() << std::endl;
			}
		}
		curNode = curNode->next;
	}
}

//*********************************************************************
//						loadFactions
//*********************************************************************

void Creature::loadFactions(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	while(curNode) {
		if(NODE_NAME(curNode, "Faction")) {
			loadFaction(curNode);
		}
		curNode = curNode->next;
	}
}

//*********************************************************************
//						load
//*********************************************************************

void Effects::load(xmlNodePtr rootNode, MudObject* pParent) {
	xmlNodePtr curNode = rootNode->children;
	while(curNode) {
		if(NODE_NAME(curNode, "Effect")) {
			try {
				EffectInfo* newEffect = new EffectInfo(curNode);
				if(newEffect) {
					newEffect->setParent(pParent);
					effectList.push_back(newEffect);
				}
			} catch(bstring err) {
				std::cout << "Error adding effect: " << err << std::endl;
			}
		}
		curNode = curNode->next;
	}
}

//*********************************************************************
//						readFromXml
//*********************************************************************
// Reads an object from the given xml document and root node

int Object::readFromXml(xmlNodePtr rootNode) {
	xmlNodePtr curNode, childNode;
	info.load(rootNode);
	info.id = xml::getIntProp(rootNode, "Num");
	xml::copyPropToBString(version, rootNode, "Version");
	curNode = rootNode->children;
	// Start reading stuff in!

	while(curNode) {
			 if(NODE_NAME(curNode, "Name")) xml::copyToCString(name, curNode);
//		else if(NODE_NAME(curNode, "Id")) setId(xml::getBString(curNode));
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
		else if(NODE_NAME(curNode, "Type")) xml::copyToNum(type, curNode);
		else if(NODE_NAME(curNode, "SubType")) xml::copyToBString(subType, curNode);
		else if(NODE_NAME(curNode, "Adjustment")) xml::copyToNum(adjustment, curNode);
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

		else if(NODE_NAME(curNode, "ShopValue")) shopValue = xml::toNum<unsigned long>(curNode);
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
			readObjects(curNode);
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
				increase = 0;
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

	if(version < "2.46j" && flagIsSet(O_WEAPON_CASTS)) {
	    // Version 2.46j added charges for casting weapons, versions before that
	    // used shots.  Initialize charges as 1/3rd of shots
	    chargesMax = shotsMax / 3;
	    chargesCur = shotsCur /3;
	}
	// make sure uniqueness stays intact
	setFlag(O_UNIQUE);
	if(!gConfig->getUnique(this))
		clearFlag(O_UNIQUE);

	escapeText();
	return(0);
}

//*********************************************************************
//						readFromXml
//*********************************************************************
// Reads a room from the given xml document and root node

int UniqueRoom::readFromXml(xmlNodePtr rootNode) {
	xmlNodePtr curNode;
//	xmlNodePtr childNode;

	info.load(rootNode);
	info.id = xml::getIntProp(rootNode, "Num");
	xml::copyPropToBString(version, rootNode, "Version");
	curNode = rootNode->children;
	// Start reading stuff in!
	while(curNode) {
			 if(NODE_NAME(curNode, "Name")) xml::copyToCString(name, curNode);
		else if(NODE_NAME(curNode, "ShortDescription")) xml::copyToBString(short_desc, curNode);
		else if(NODE_NAME(curNode, "LongDescription")) xml::copyToBString(long_desc, curNode);
		else if(NODE_NAME(curNode, "Fishing")) xml::copyToBString(fishing, curNode);
		else if(NODE_NAME(curNode, "Faction")) xml::copyToBString(faction, curNode);
		else if(NODE_NAME(curNode, "LastModBy")) xml::copyToCString(last_mod, curNode);
		else if(NODE_NAME(curNode, "LastModTime")) xml::copyToCString(lastModTime, curNode);
		else if(NODE_NAME(curNode, "LastPlayer")) xml::copyToCString(lastPly, curNode);
		else if(NODE_NAME(curNode, "LastPlayerTime")) xml::copyToCString(lastPlyTime, curNode);
		else if(NODE_NAME(curNode, "LowLevel")) xml::copyToNum(lowLevel, curNode);
		else if(NODE_NAME(curNode, "HighLevel")) xml::copyToNum(highLevel, curNode);
		else if(NODE_NAME(curNode, "MaxMobs")) xml::copyToNum(maxmobs, curNode);
		else if(NODE_NAME(curNode, "Trap")) xml::copyToNum(trap, curNode);
		else if(NODE_NAME(curNode, "TrapExit")) trapexit.load(curNode);
		else if(NODE_NAME(curNode, "TrapWeight")) xml::copyToNum(trapweight, curNode);
		else if(NODE_NAME(curNode, "TrapStrength")) xml::copyToNum(trapstrength, curNode);

		else if(NODE_NAME(curNode, "BeenHere")) xml::copyToNum(beenhere, curNode);
		else if(NODE_NAME(curNode, "Track")) track.load(curNode);
		else if(NODE_NAME(curNode, "Wander")) wander.load(curNode);
		else if(NODE_NAME(curNode, "RoomExp")) xml::copyToNum(roomExp, curNode);
		else if(NODE_NAME(curNode, "Flags")) {
			// No need to clear flags, no room refs
			loadBits(curNode, flags);
		}
		else if(NODE_NAME(curNode, "PermMobs")) {
			loadCrLastTimes(curNode, permMonsters);
		}
		else if(NODE_NAME(curNode, "PermObjs")) {
			loadCrLastTimes(curNode, permObjects);
		}
		else if(NODE_NAME(curNode, "LastTimes")) {
			loadLastTimes(curNode, lasttime);
		}
		else if(NODE_NAME(curNode, "Objects")) {
			readObjects(curNode);
		}
		else if(NODE_NAME(curNode, "Creatures")) {
			readCreatures(curNode);
		}
		else if(NODE_NAME(curNode, "Exits")) {
			readExitsXml(curNode);
		}
		else if(NODE_NAME(curNode, "Effects")) effects.load(curNode, this);
		else if(NODE_NAME(curNode, "Size")) size = whatSize(xml::toNum<int>(curNode));
		else if(NODE_NAME(curNode, "Hooks")) hooks.load(curNode);


		// load old tracks
		if(getVersion() < "2.32d") {
				 if(NODE_NAME(curNode, "TrackExit")) track.setDirection(xml::getBString(curNode));
			else if(NODE_NAME(curNode, "TrackSize")) track.setSize(whatSize(xml::toNum<int>(curNode)));
			else if(NODE_NAME(curNode, "TrackNum")) track.setNum(xml::toNum<int>(curNode));
		}
		// load old wander info
		if(getVersion() < "2.33b") {
				 if(NODE_NAME(curNode, "Traffic")) wander.setTraffic(xml::toNum<int>(curNode));
			else if(NODE_NAME(curNode, "RandomMonsters")) {
				loadCatRefArray(curNode, wander.random, "Mob", NUM_RANDOM_SLOTS);
			}
		}
		if(getVersion() < "2.42e") {
			if(NODE_NAME(curNode, "Special")) xml::copyToNum(maxmobs, curNode);
		}
		curNode = curNode->next;
	}

	escapeText();
	addEffectsIndex();
	return(0);
}

//*********************************************************************
//						readFromXml
//*********************************************************************
// Reads a exit from the given xml document and root node

int Exit::readFromXml(xmlNodePtr rootNode, BaseRoom* room) {
	xmlNodePtr curNode;

	xml::copyPropToCString(name, rootNode, "Name");

	curNode = rootNode->children;
	// Start reading stuff in!
	while(curNode) {
			 if(NODE_NAME(curNode, "Name")) xml::copyToCString(name, curNode);
		else if(NODE_NAME(curNode, "Keys")) {
			loadStringArray(curNode, desc_key, EXIT_KEY_LENGTH, "Key", 3);
		}
		else if(NODE_NAME(curNode, "Level")) xml::copyToNum(level, curNode);
		else if(NODE_NAME(curNode, "Trap")) xml::copyToNum(trap, curNode);
		else if(NODE_NAME(curNode, "Key")) xml::copyToNum(key, curNode);
		else if(NODE_NAME(curNode, "KeyArea")) xml::copyToBString(keyArea, curNode);
		else if(NODE_NAME(curNode, "Size")) size = whatSize(xml::toNum<int>(curNode));
		else if(NODE_NAME(curNode, "Direction")) direction = (Direction)xml::toNum<int>(curNode);
		else if(NODE_NAME(curNode, "Toll")) xml::copyToNum(toll, curNode);
		else if(NODE_NAME(curNode, "PassPhrase")) xml::copyToBString(passphrase, curNode);
		else if(NODE_NAME(curNode, "PassLang")) xml::copyToNum(passlang, curNode);
		else if(NODE_NAME(curNode, "Description")) xml::copyToBString(description, curNode);
		else if(NODE_NAME(curNode, "Open")) xml::copyToBString(open, curNode);
		else if(NODE_NAME(curNode, "Enter")) { xml::copyToBString(enter, curNode); }
		else if(NODE_NAME(curNode, "Flags")) {
			// No need to clear flags, no exit refs
			loadBits(curNode, flags);
		}
		else if(NODE_NAME(curNode, "Target")) target.load(curNode);
		else if(NODE_NAME(curNode, "Effects")) effects.load(curNode, this);
		else if(NODE_NAME(curNode, "Hooks")) hooks.load(curNode);

		// depreciated, but there's no version function
		// TODO: Use room->getVersion()
		else if(NODE_NAME(curNode, "Room")) target.room.load(curNode);
		else if(NODE_NAME(curNode, "AreaRoom")) target.mapmarker.load(curNode);

		curNode = curNode->next;
	}

	escapeText();
	return(0);
}

//*********************************************************************
//						readObjects
//*********************************************************************
// Reads in objects and objrefs and adds them to the parent

void MudObject::readObjects(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	Object* object=0, *object2=0;
	CatRef	cr;
	int		i=0,quantity=0;

	Creature* cParent = getAsCreature();
//  Monster* mParent = parent->getMonster();
//  Player* pParent = parent->getPlayer();
    UniqueRoom* rParent = getAsUniqueRoom();
    Object* oParent = getAsObject();


	while(childNode) {
		object = 0;
		quantity = xml::getIntProp(childNode, "Quantity");
		if(quantity < 1)
			quantity = 1;
		if(NODE_NAME(childNode, "Object")) {
			// If it's a full object, read it in
			object = new Object;
			if(!object)
				merror("loadObjectsXml", FATAL);
			object->readFromXml(childNode);
		} else if(NODE_NAME(childNode, "ObjRef")) {
			// If it's an object reference, we want to first load the parent object
			// and then use readObjectXml to update any fields that may have changed
			cr.load(childNode);
			cr.id = xml::getIntProp(childNode, "Num");
			if(!validObjId(cr)) {
				printf("Invalid object %s\n", cr.str().c_str());
			} else {
				if(loadObject(cr, &object)) {
					// These two flags might be cleared on the reference object, so let
					// that object set them if it wants to
					object->clearFlag(O_HIDDEN);
					object->clearFlag(O_CURSED);
					object->readFromXml(childNode);
				} else {
					//printf("Error loading object %d.\n", num);
				}
			}
		}

		// BUG: This messes with object unique ids
		if(object) {
			for(i=0; i<quantity; i++) {
				if(!i) {
					object2 = object;  // just a reference
				} else {
					object2 = new Object;
					*object2 = *object;
				}
				// Add it to the appropriate parent
				if(oParent) {
					oParent->addObj(object2);
				} else if(cParent) {
					cParent->addObj(object2);
				} else if(rParent) {
					object2->addToRoom(rParent);
				}
			}
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						readCreatures
//*********************************************************************
// Reads in creatures and crts refs and adds them to the parent

void MudObject::readCreatures(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	Monster *mob=0;
	CatRef	cr;

	Creature* cParent = getAsCreature();
//	Monster* mParent = parent->getMonster();
//	Player* pParent = parent->getPlayer();
	UniqueRoom* rParent = getAsUniqueRoom();
	Object* oParent = getAsObject();

	while(childNode) {
		mob = NULL;
		if(NODE_NAME( childNode , "Creature")) {
			// If it's a full creature, read it in
			mob = new Monster;
			if(!mob)
				merror("loadCreaturesXml", FATAL);
			mob->readFromXml(childNode);
		} else if(NODE_NAME( childNode, "CrtRef")) {
			// If it's an creature reference, we want to first load the parent creature
			// and then use readCreatureXml to update any fields that may have changed
			cr.load(childNode);
			cr.id = xml::getIntProp( childNode, "Num" );

			if(loadMonster(cr, &mob)) {
				mob->readFromXml(childNode);
			} else {
				//printf("Unable to load creature %d\n", num);
			}
		}
		if(mob != NULL) {
			// Add it to the appropriate parent
			if(cParent) {
			    // If we have a creature parent, we must be a pet
			    cParent->addPet(mob);
			} else if(rParent) {
				mob->addToRoom(rParent, 0);
			} else if(oParent) {
				// TODO: Not currently implemented
			}
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						readExitsXml
//*********************************************************************

void BaseRoom::readExitsXml(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	Exit* ext=0;
	AreaRoom* aRoom = getAsAreaRoom();

	while(childNode) {
		if(NODE_NAME(childNode , "Exit")) {
			ext = new Exit;
			if(!ext)
				merror("loadExitsXml", FATAL);
			ext->readFromXml(childNode, this);

			if(!ext->flagIsSet(X_PORTAL)) {
				// moving cardinal exit on the overland?
				if(ext->flagIsSet(X_MOVING) && aRoom && aRoom->updateExit(ext->name))
					delete ext;
				else
					addExit(ext);
			} else
				delete ext;
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//                      loadSocials
//*********************************************************************

bool Config::loadSocials() {
    xmlDocPtr   xmlDoc;

    xmlNodePtr  curNode;
    char        filename[80];

    // build an XML tree from a the file
    sprintf(filename, "%s/socials.xml", Path::Code);

    xmlDoc = xml::loadFile(filename, "Socials");
    if(xmlDoc == NULL)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == 0) {
        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();
        return(false);
    }

    clearAlchemy();
    while(curNode != NULL) {
        if(NODE_NAME(curNode, "Social")) {
            SocialCommand* social = new SocialCommand(curNode);
            if(social)
                socials[social->getName()]  = social;
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

int cmdSocial(Creature* creature, cmd* cmnd);
SocialCommand::SocialCommand(xmlNodePtr rootNode) {
    rootNode = rootNode->children;
    priority = 100;
    auth = NULL;
    desc = "";
    fn = cmdSocial;

    wakeTarget = false;
    rudeWakeTarget = false;
    wakeRoom = false;

    while(rootNode != NULL)
    {
        if(NODE_NAME(rootNode, "Name")) xml::copyToBString(name, rootNode);
        else if(NODE_NAME(rootNode, "Description")) xml::copyToBString(desc, rootNode);
        else if(NODE_NAME(rootNode, "Priority")) xml::copyToNum(priority, rootNode);
        else if(NODE_NAME(rootNode, "Script")) xml::copyToBString(script, rootNode);
        else if(NODE_NAME(rootNode, "SelfNoTarget")) xml::copyToBString(selfNoTarget, rootNode);
        else if(NODE_NAME(rootNode, "RoomNoTarget")) xml::copyToBString(roomNoTarget, rootNode);
        else if(NODE_NAME(rootNode, "SelfOnTarget")) xml::copyToBString(selfOnTarget, rootNode);
        else if(NODE_NAME(rootNode, "RoomOnTarget")) xml::copyToBString(roomOnTarget, rootNode);
        else if(NODE_NAME(rootNode, "VictimOnTarget")) xml::copyToBString(victimOnTarget, rootNode);
        else if(NODE_NAME(rootNode, "SelfOnSelf")) xml::copyToBString(selfOnSelf, rootNode);
        else if(NODE_NAME(rootNode, "RoomOnSelf")) xml::copyToBString(roomOnSelf, rootNode);
        else if(NODE_NAME(rootNode, "WakeTarget")) xml::copyToBool(wakeTarget, rootNode);
        else if(NODE_NAME(rootNode, "RudeWakeTarget")) xml::copyToBool(rudeWakeTarget, rootNode);
        else if(NODE_NAME(rootNode, "WakeRoom")) xml::copyToBool(wakeRoom, rootNode);

        rootNode = rootNode->next;
    }
}
//*********************************************************************
//						loadAlchemy
//*********************************************************************

bool Config::loadAlchemy() {
	xmlDocPtr	xmlDoc;

	xmlNodePtr	curNode;
	char		filename[80];

	// build an XML tree from a the file
	sprintf(filename, "%s/alchemy.xml", Path::Code);

	xmlDoc = xml::loadFile(filename, "Alchemy");
	if(xmlDoc == NULL)
		return(false);

	curNode = xmlDocGetRootElement(xmlDoc);

	curNode = curNode->children;
	while(curNode && xmlIsBlankNode(curNode)) {
		curNode = curNode->next;
	}
	if(curNode == 0) {
		xmlFreeDoc(xmlDoc);
		xmlCleanupParser();
		return(false);
	}

	clearAlchemy();
	while(curNode != NULL) {
		if(NODE_NAME(curNode, "AlchemyInfo")) {
			AlchemyInfo* alcInfo = new AlchemyInfo(curNode);
			if(alcInfo)
				alchemy.push_back(alcInfo);
		}
		curNode = curNode->next;
	}
	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	return(true);
}

AlchemyInfo::AlchemyInfo(xmlNodePtr rootNode) {
	rootNode = rootNode->children;
	while(rootNode != NULL)
	{
		if(NODE_NAME(rootNode, "Name")) xml::copyToBString(name, rootNode);
		else if(NODE_NAME(rootNode, "Action")) xml::copyToBString(action, rootNode);
		else if(NODE_NAME(rootNode, "PythonScript")) xml::copyToBString(pythonScript, rootNode);
		else if(NODE_NAME(rootNode, "Positive")) xml::copyToBool(positive, rootNode);
		else if(NODE_NAME(rootNode, "Throwable")) xml::copyToBool(throwable, rootNode);

		rootNode = rootNode->next;
	}
}
//*********************************************************************
//                      loadMxpElements
//*********************************************************************

bool Config::loadMxpElements() {
    xmlDocPtr   xmlDoc;

    xmlNodePtr  curNode;
    char        filename[80];

    // build an XML tree from a the file
    sprintf(filename, "%s/mxp.xml", Path::Code);

    xmlDoc = xml::loadFile(filename, "Mxp");
    if(xmlDoc == NULL)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == 0) {
        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();
        return(false);
    }

    clearAlchemy();
    while(curNode != NULL) {
        if(NODE_NAME(curNode, "MxpElement")) {
            MxpElement* mxpElement = new MxpElement(curNode);
            if(mxpElement) {
                mxpElements.insert(std::pair<bstring, MxpElement*>(mxpElement->getName(), mxpElement));
                if(mxpElement->isColor()) {
                    mxpColors.insert(std::pair<bstring, bstring>(mxpElement->getColor(), mxpElement->getName()));
                }
            }
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}
//bstring name;
//MxpType mxpType;
//bstring command;
//bstring hint;
//bool prompt;
//bstring attributes;
//bstring expire;
MxpElement::MxpElement(xmlNodePtr rootNode) {
    rootNode = rootNode->children;
    prompt = false;
    while(rootNode != NULL)
    {
        if(NODE_NAME(rootNode, "Name")) xml::copyToBString(name, rootNode);
        else if(NODE_NAME(rootNode, "Command")) xml::copyToBString(command, rootNode);
        else if(NODE_NAME(rootNode, "Hint")) xml::copyToBString(hint, rootNode);
        else if(NODE_NAME(rootNode, "Type")) xml::copyToBString(mxpType, rootNode);
        else if(NODE_NAME(rootNode, "Prompt")) xml::copyToBool(prompt, rootNode);
        else if(NODE_NAME(rootNode, "Attributes")) xml::copyToBString(attributes, rootNode);
        else if(NODE_NAME(rootNode, "Expire")) xml::copyToBString(expire, rootNode);
        else if(NODE_NAME(rootNode, "Color")) xml::copyToBString(color, rootNode);

        rootNode = rootNode->next;
    }
}

//*********************************************************************
//						loadBans
//*********************************************************************

bool Config::loadBans() {
	xmlDocPtr xmlDoc;
	Ban *curBan=0;
	xmlNodePtr cur;

	char filename[80];
	snprintf(filename, 80, "%s/bans.xml", Path::Config);
	xmlDoc = xml::loadFile(filename, "Bans");

	if(xmlDoc == NULL)
		return(false);

	cur = xmlDocGetRootElement(xmlDoc);


	//	numBans = 0;
	//	memset(banTable, 0, sizeof(banTable));

	// First level we expect a Ban
	cur = cur->children;
	while(cur && xmlIsBlankNode(cur)) {
		cur = cur->next;
	}
	if(cur == 0) {
		xmlFreeDoc(xmlDoc);
		return(false);
	}

	clearBanList();
	while(cur != NULL) {
		if((!strcmp((char*) cur->name, "Ban"))) {
			curBan = new Ban(cur);
			if(curBan != NULL)
				bans.push_back(curBan);
		}
		cur = cur->next;
	}
	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	return(true);
}

//*********************************************************************
//						Ban
//*********************************************************************

Ban::Ban(xmlNodePtr curNode) {

	curNode = curNode->children;
	while(curNode != NULL)
	{
		if(NODE_NAME(curNode, "Site")) xml::copyToBString(site, curNode);
		else if(NODE_NAME(curNode, "Duration")) xml::copyToNum(duration, curNode);
		else if(NODE_NAME(curNode, "UnbanTime")) xml::copyToNum(unbanTime, curNode);
		else if(NODE_NAME(curNode, "BannedBy")) xml::copyToBString(by, curNode);
		else if(NODE_NAME(curNode, "BanTime")) xml::copyToBString(time, curNode);
		else if(NODE_NAME(curNode, "Reason")) xml::copyToBString(reason, curNode);
		else if(NODE_NAME(curNode, "Password")) xml::copyToBString(password, curNode);
		else if(NODE_NAME(curNode, "Suffix")) xml::copyToBool(isSuffix, curNode);
		else if(NODE_NAME(curNode, "Prefix")) xml::copyToBool(isPrefix, curNode);

		curNode = curNode->next;
	}
}


//*********************************************************************
//						loadGuilds
//*********************************************************************
// Causes the xml guild file to be parsed

bool Config::loadGuilds() {
	xmlDocPtr	xmlDoc;
	GuildCreation *curCreation=0;
	xmlNodePtr	cur;
	char		filename[80];

	// build an XML tree from a the file
	sprintf(filename, "%s/guilds.xml", Path::PlayerData);

	xmlDoc = xml::loadFile(filename, "Guilds");
	if(xmlDoc == NULL)
		return(false);

	cur = xmlDocGetRootElement(xmlDoc);
	gConfig->nextGuildId = xml::getIntProp(cur, "NextGuildId");
//	xml::copyToNum(gConfig->nextGuildId, = toNum<int>((char *)xmlGetProp(cur, BAD_CAST "NextGuildId"));

	//	numGuilds = 0;
	//	memset(guildTable, 0, sizeof(guildTable));

	// First level we expect a Guild
	cur = cur->children;
	while(cur && xmlIsBlankNode(cur))
		cur = cur->next;

	if(cur == 0) {
		// DOH! Forgot to clean up stuff here...not that it happened, but would have been leaky
		xmlFreeDoc(xmlDoc);
		xmlCleanupParser();
		return(false);
	}

	clearGuildList();
	Guild* guild;
	while(cur != NULL) {
		if(!strcmp((char *)cur->name, "Guild")) {
			guild = new Guild(cur);
			if(guild)
				addGuild(guild);
		}
		if(!strcmp((char *)cur->name, "GuildCreation")) {
			curCreation = parseGuildCreation(cur);
			if(curCreation != NULL)
				addGuildCreation(curCreation);
		}
		cur = cur->next;
	}
	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	return(true);
}

//*********************************************************************
//						parseGuild
//*********************************************************************
// Parse an individual guild from a valid xml file

Guild::Guild(xmlNodePtr curNode) {
	int guildId = xml::getIntProp(curNode, "ID");
	if(guildId == 0)
		throw new bstring("Invalid GuildID");

	num = guildId;
	gConfig->numGuilds = MAX(gConfig->numGuilds, guildId);
	curNode = curNode->children;
	while(curNode != NULL) {
			 if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
		else if(NODE_NAME(curNode, "Leader")) xml::copyToBString(leader, curNode);
		else if(NODE_NAME(curNode, "Level")) xml::copyToNum(level, curNode);
		else if(NODE_NAME(curNode, "NumMembers")) xml::copyToNum(numMembers, curNode);
		else if(NODE_NAME(curNode, "Members")) parseGuildMembers(curNode);
		else if(NODE_NAME(curNode, "PkillsWon")) xml::copyToNum(pkillsWon, curNode);
		else if(NODE_NAME(curNode, "PkillsIn")) xml::copyToNum(pkillsIn, curNode);
		else if(NODE_NAME(curNode, "Points")) xml::copyToNum(points, curNode);
		else if(NODE_NAME(curNode, "Bank")) bank.load(curNode);

		curNode = curNode->next;
	}
}

//*********************************************************************
//						parseGuildCreation
//*********************************************************************
// Parse an individual guild from a valid xml file

GuildCreation * parseGuildCreation(xmlNodePtr cur) {
	GuildCreation* gc = new GuildCreation;
	if(gc == NULL) {
		loge("Guild_Load: out of memory\n");
		return(NULL);
	}

	bstring temp = "";
	cur = cur->children;
	while(cur != NULL) {
			 if(NODE_NAME(cur, "Name")) xml::copyToBString(gc->name, cur);
		else if(NODE_NAME(cur, "Leader")) xml::copyToBString(gc->leader, cur);
		else if(NODE_NAME(cur, "LeaderIp")) xml::copyToBString(gc->leaderIp, cur);
		else if(NODE_NAME(cur, "Status")) xml::copyToNum(gc->status, cur);
		else if(NODE_NAME(cur, "Supporter")) {
			xml::copyToBString(temp, cur);
			gc->supporters[temp] = xml::getProp(cur, "Ip");
			gc->numSupporters++;
		}
		cur = cur->next;
	}
	return(gc);
}

//*********************************************************************
//						parseGuildMembers
//*********************************************************************

void Guild::parseGuildMembers(xmlNodePtr cur) {
	cur = cur->children;
	while(cur != NULL) {
		if(NODE_NAME(cur, "Member")) {
			char *tmp = xml::getCString(cur);
			addMember(tmp);
			free(tmp);
		}
		cur = cur->next;
	}
}

//*********************************************************************
//						loadSkills
//*********************************************************************

bool Config::loadSkills() {
	xmlDocPtr	xmlDoc;
	//GuildCreation * curCreation;
	xmlNodePtr	cur;
	char		filename[80];

	// build an XML tree from a the file
	sprintf(filename, "%s/skills.xml", Path::Code);

	xmlDoc = xml::loadFile(filename, "Skills");
	if(xmlDoc == NULL)
		return(false);

	cur = xmlDocGetRootElement(xmlDoc);

	cur = cur->children;
	while(cur && xmlIsBlankNode(cur)) {
		cur = cur->next;
	}
	if(cur == 0) {
		xmlFreeDoc(xmlDoc);
		xmlCleanupParser();
		return(false);
	}

	clearSkills();
	while(cur != NULL) {
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
//						loadSkillGroups
//*********************************************************************

void Config::loadSkillGroups(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	while(curNode != NULL) {
		if(NODE_NAME(curNode, "SkillGroup")) {
			loadSkillGroup(curNode);
		}

		curNode = curNode->next;
	}
}

//*********************************************************************
//						loadSkillGroup
//*********************************************************************

void Config::loadSkillGroup(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	bstring name;
	bstring displayName;
	while(curNode != NULL) {
		if(NODE_NAME(curNode, "Name")) {
			xml::copyToBString(name, curNode);
		} else if(NODE_NAME(curNode, "DisplayName")) {
			xml::copyToBString(displayName, curNode);
		}
		curNode = curNode->next;
	}
	if(name != "" && displayName != "") {
		skillGroups[name] = displayName;
	}
	return;
}

//*********************************************************************
//						loadSkills
//*********************************************************************

void Config::loadSkills(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	SkillInfo* skill=0;
	while(curNode != NULL) {
		if(NODE_NAME(curNode, "Skill")) {
			try {
				skill = new SkillInfo(curNode);
				skills.insert(SkillInfoMap::value_type(skill->getName(), skill));
			} catch(...) {

			}
		}

		curNode = curNode->next;
	}
}

//*********************************************************************
//						load
//*********************************************************************

SkillInfo::SkillInfo(xmlNodePtr rootNode) {
	gainType = SKILL_NORMAL;
	knownOnly = false;
	usesAttackTimer = true;

	xmlNodePtr curNode = rootNode->children;
	bstring group;
	bstring description;

	while(curNode != NULL) {
		if(NODE_NAME(curNode, "Name")) {
			xml::copyToBString(name, curNode);
		} else if(NODE_NAME(curNode, "DisplayName")) {
			xml::copyToBString(displayName, curNode);
		} else if(NODE_NAME(curNode, "Group")) {
			xml::copyToBString(group, curNode);
			if(!setGroup(group)) {
				printf("Error setting skill '%s' to group '%s'", name.c_str(), group.c_str());
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
		} else if(NODE_NAME(curNode, "Cooldown")) {
			xml::copyToNum(cooldown, curNode);
		} else if(NODE_NAME(curNode, "UsesAttackTimer")) {
			xml::copyToBool(usesAttackTimer, curNode);
		} else if(NODE_NAME(curNode, "Cooldown")) {
			xml::copyToNum(cooldown, curNode);
		} else if(NODE_NAME(curNode, "FailCooldown")) {
			xml::copyToNum(failCooldown, curNode);
		} else if(NODE_NAME(curNode, "Resources")) {
			loadResources(curNode);
		}
		curNode = curNode->next;
	}
	if(name == "" || displayName == "") {
		std::cout << "Invalid skill (Name:" << name << ", DisplayName: " << displayName <<  ")" << std::endl;
		throw(new std::exception());
	}
}

void SkillInfo::loadResources(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	while(curNode != NULL) {
		if(NODE_NAME(curNode, "Resource")) {
			resources.push_back(SkillCost(curNode));
		}
		curNode = curNode->next;
	}
}

SkillCost::SkillCost(xmlNodePtr rootNode) {
	resource = RES_NONE;
	cost = 0;

	xmlNodePtr curNode = rootNode->children;
	while(curNode != NULL) {
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
//						loadCatRefArray
//*********************************************************************

void loadCatRefArray(xmlNodePtr curNode, std::map<int, CatRef>& array, const char* name, int maxProp) {
	xmlNodePtr childNode = curNode->children;
	int i=0;

	while(childNode) {
		if(NODE_NAME(childNode , name)) {
			i = xml::getIntProp(childNode, "Num");
			if(i >= 0 && i < maxProp)
				array[i].load(childNode);
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadCatRefArray
//*********************************************************************

void loadCatRefArray(xmlNodePtr curNode, CatRef array[], const char* name, int maxProp) {
	xmlNodePtr childNode = curNode->children;
	int i=0;

	while(childNode) {
		if(NODE_NAME(childNode , name)) {
			i = xml::getIntProp(childNode, "Num");
			if(i >= 0 && i < maxProp)
				array[i].load(childNode);
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadCarryArray
//*********************************************************************

void loadCarryArray(xmlNodePtr curNode, Carry array[], const char* name, int maxProp) {
	xmlNodePtr childNode = curNode->children;
	int i=0;

	while(childNode) {
		if(NODE_NAME(childNode , name)) {
			i = xml::getIntProp(childNode, "Num");
			if(i >= 0 && i < maxProp)
				array[i].load(childNode);
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadString Array
//*********************************************************************
// Since we can't easily pass in variable length arrays, we pass in a pointer to the array
// as well as the size of the array. Based on the size and current number being read in
// we do some pointer arithmetic and then memcpy to the appropriate location

// Note: Size MUST be accurate or you will get unpredictable results

void loadStringArray(xmlNodePtr curNode, void* array, int size, const char* name, int maxProp) {
	int i=0, j=0;
	char temp[255];
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode , name)) {
			i = xml::getIntProp(childNode, "Num");
			if(i < maxProp) {
				xml::copyToCString(temp, childNode);

				j = strlen(temp);
				if(j > (size - 1))
					j = (size - 1);
				temp[j] = '\0';

				strcpy((char*)array + ( i * sizeof(char) * size), temp);
			}
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadAlchemyEffects
//*********************************************************************
AlchemyEffect::AlchemyEffect(xmlNodePtr curNode) {
	quality=duration = strength = 1;
	xmlNodePtr childNode = curNode->children;
	while(childNode) {
		if(NODE_NAME(childNode, "Effect")) {
			effect = xml::getBString(childNode);
		} else if(NODE_NAME(childNode, "Duration")) {
			xml::copyToNum(duration, childNode);
		} else if(NODE_NAME(childNode, "Strength")) {
			xml::copyToNum(strength, childNode);
		} else if(NODE_NAME(childNode, "Quality")) {
			xml::copyToNum(quality, childNode);
		}
		childNode = childNode->next;
	}
}
void Object::loadAlchemyEffects(xmlNodePtr curNode) {
	int i=0;
	xmlNodePtr childNode = curNode->children;
	while(childNode) {
		if(NODE_NAME(childNode, "AlchemyEffect")) {
			i = xml::getIntProp(childNode, "Num");
			if(i>0) {
				alchemyEffects[i] = AlchemyEffect(childNode);
			}
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadBits
//*********************************************************************
// Sets all bits it finds into the given bit array

void loadBits(xmlNodePtr curNode, char *bits) {
	xmlNodePtr childNode = curNode->children;
	int bit=0;

	while(childNode) {
		if(NODE_NAME(childNode, "Bit")) {
			bit = xml::getIntProp(childNode, "Num");
			// TODO: Sanity check bit
			BIT_SET(bits, bit);
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadDailys
//*********************************************************************
// This function will load all daily timers into the given dailys array

void loadDailys(xmlNodePtr curNode, struct daily* pDailys) {
	xmlNodePtr childNode = curNode->children;
	int i=0;

	while(childNode) {
		if(NODE_NAME(childNode, "Daily")) {
			i = xml::getIntProp(childNode, "Num");
			// TODO: Sanity check i
			loadDaily(childNode, &pDailys[i]);
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadDaily
//*********************************************************************
// This loads an individual daily into the provided pointer

void loadDaily(xmlNodePtr curNode, struct daily* pDaily) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Max")) xml::copyToNum(pDaily->max, childNode);
		else if(NODE_NAME(childNode, "Current")) xml::copyToNum(pDaily->cur, childNode);
		else if(NODE_NAME(childNode, "LastTime")) xml::copyToNum(pDaily->ltime, childNode);

		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadCrLastTimes
//*********************************************************************
// This function will load all crlasttimer into the given crlasttime array

void loadCrLastTimes(xmlNodePtr curNode, std::map<int, crlasttime>& pCrLastTimes) {
	xmlNodePtr childNode = curNode->children;
	int i=0;

	while(childNode) {
		if(NODE_NAME(childNode, "LastTime")) {
			i = xml::getIntProp(childNode, "Num");
			if(i >= 0 && i < NUM_PERM_SLOTS) {
				struct crlasttime cr;
				loadCrLastTime(childNode, &cr);
				if(cr.cr.id)
					pCrLastTimes[i] = cr;
			}
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadCrLastTime
//*********************************************************************
// Loads a single crlasttime into the given lasttime

void loadCrLastTime(xmlNodePtr curNode, struct crlasttime* pCrLastTime) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Interval")) xml::copyToNum(pCrLastTime->interval, childNode);
		else if(NODE_NAME(childNode, "LastTime")) xml::copyToNum(pCrLastTime->ltime, childNode);
		else if(NODE_NAME(childNode, "Misc")) pCrLastTime->cr.load(childNode);

		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadLastTimes
//*********************************************************************
// This function will load all lasttimer into the given lasttime array

void loadLastTimes(xmlNodePtr curNode, struct lasttime* pLastTimes) {
	xmlNodePtr childNode = curNode->children;
	int i=0;

	while(childNode) {
		if(NODE_NAME(childNode, "LastTime")) {
			i = xml::getIntProp(childNode, "Num");
			// TODO: Sanity check i
			loadLastTime(childNode, &pLastTimes[i]);
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadLastTime
//*********************************************************************
// Loads a single lasttime into the given lasttime

void loadLastTime(xmlNodePtr curNode, struct lasttime* pLastTime) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Interval")) xml::copyToNum(pLastTime->interval, childNode);
		else if(NODE_NAME(childNode, "LastTime")) xml::copyToNum(pLastTime->ltime, childNode);
		else if(NODE_NAME(childNode, "Misc")) xml::copyToNum(pLastTime->misc, childNode);

		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadAnchors
//*********************************************************************
// Loads all anchors into the given anchors array

void Player::loadAnchors(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	int i=0;

	while(childNode) {
		if(NODE_NAME(childNode, "Anchor")) {
			i = xml::getIntProp(childNode, "Num");
			if(i >= 0 && i < MAX_DIMEN_ANCHORS && !anchor[i]) {
				anchor[i] = new Anchor;
				anchor[i]->load(childNode);
			}
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadSavingThrows
//*********************************************************************
// Loads all saving throws into the given array of saving throws

void loadSavingThrows(xmlNodePtr curNode, struct saves* pSavingThrows) {
	xmlNodePtr childNode = curNode->children;
	int i=0;

	while(childNode) {
		if(NODE_NAME(childNode, "SavingThrow")) {
			i = xml::getIntProp(childNode, "Num");
			// TODO: Sanity check i
			loadSavingThrow(childNode, &pSavingThrows[i]);
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadSavingThrows
//*********************************************************************
// Loads an individual saving throw

void loadSavingThrow(xmlNodePtr curNode, struct saves* pSavingThrow) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Chance")) xml::copyToNum(pSavingThrow->chance, childNode);
		else if(NODE_NAME(childNode, "Gained")) xml::copyToNum(pSavingThrow->gained, childNode);
		else if(NODE_NAME(childNode, "Misc")) xml::copyToNum(pSavingThrow->misc, childNode);

		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadRanges
//*********************************************************************
// Loads builder ranges into the provided creature

void loadRanges(xmlNodePtr curNode, Player *pPlayer) {
	xmlNodePtr childNode = curNode->children;
	int i=0;

	while(childNode) {
		if(NODE_NAME(childNode, "Range")) {
			i = xml::getIntProp(childNode, "Num");
			if(i >= 0 && i < MAX_BUILDER_RANGE)
				pPlayer->bRange[i].load(childNode);
		}
		childNode = childNode->next;
	}
}

//*********************************************************************
//						loadStats
//*********************************************************************
// Loads all stats (strength, intelligence, hp, etc) into the given creature

void Creature::loadStats(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	bstring statName = "";

	while(childNode) {
		if(NODE_NAME(childNode, "Stat")) {
			statName = xml::getProp(childNode, "Name");

			if(statName == "")
				continue;

				 if(statName == "Strength") strength.load(childNode, statName);
			else if(statName == "Dexterity") dexterity.load(childNode, statName);
			else if(statName == "Constitution") constitution.load(childNode, statName);
			else if(statName == "Intelligence") intelligence.load(childNode, statName);
			else if(statName == "Piety") piety.load(childNode, statName);
			else if(statName == "Hp") hp.load(childNode, statName);
			else if(statName == "Mp") mp.load(childNode, statName);
			else if(statName == "Focus") {
				Player* player = getAsPlayer();
				if(player)
					player->focus.load(childNode, statName);
			}
		}

		childNode = childNode->next;
	}
}

//*********************************************************************
//						load
//*********************************************************************
// Loads a single stat into the given stat pointer

bool Stat::load(xmlNodePtr curNode, bstring statName) {
	xmlNodePtr childNode = curNode->children;
	name = statName;
	while(childNode) {
		if(NODE_NAME(childNode, "Current")) xml::copyToNum(cur, childNode);
		else if(NODE_NAME(childNode, "Max")) xml::copyToNum(max, childNode);
		else if(NODE_NAME(childNode, "Initial")) xml::copyToNum(initial, childNode);
		else if(NODE_NAME(childNode, "Modifiers")) loadModifiers(childNode);
		childNode = childNode->next;
	}
	return(true);
}

bool Stat::loadModifiers(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	while(childNode) {
		if(NODE_NAME(childNode, "StatModifier")) {
			StatModifier* mod = new StatModifier(childNode);
			if(mod->getName().equals("")) {
				delete mod;
			} else {
				modifiers.insert(ModifierMap::value_type(mod->getName(), mod));
			}
			childNode = childNode->next;
		}
	}
	return(true);
}
StatModifier::StatModifier(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Name")) xml::copyToBString(name, childNode);
		else if(NODE_NAME(childNode, "ModAmt")) xml::copyToNum(modAmt, childNode);
		else if(NODE_NAME(childNode, "ModType")) modType = (ModifierType)xml::toNum<unsigned short>(childNode);

		childNode = childNode->next;
	}
}
//*********************************************************************
//						loadAttacks
//*********************************************************************

void Creature::loadAttacks(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	while(curNode) {
		if(NODE_NAME(curNode, "SpecialAttack")) {
			SpecialAttack* newAttack = new SpecialAttack(curNode);
			if(newAttack) {
				specials.push_back(newAttack);
			}
		}
		curNode = curNode->next;
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
