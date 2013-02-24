/*
 * files-xml-save.cpp
 *	 Used to serialize structures (object, creature, room, etc) to xml files
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
#include "version.h"
#include "effects.h"
#include "bans.h"
#include "guilds.h"
#include "factions.h"
#include "specials.h"
#include "calendar.h"
#include "quests.h"
#include "unique.h"
#include "alchemy.h"
#include "socials.h"

xmlNodePtr saveObjRefFlags(xmlNodePtr parentNode, const char* name, int maxBit, const char *bits);
// Object flags to be saved for object refs
int objRefSaveFlags[] =
{
	O_PERM_ITEM,
	O_HIDDEN,
	O_CURSED,
	O_WORN,
	O_TEMP_ENCHANT,
	O_WAS_SHOPLIFTED,
	O_ENVENOMED,
	O_JUST_BOUGHT,
	O_NO_DROP,
	O_BROKEN_BY_CMD,
	O_BEING_PREPARED,
	O_UNIQUE,
	O_KEEP,
	O_DARKNESS,
	O_RECLAIMED,
	-1
};

//*********************************************************************
//						saveToFile
//*********************************************************************
// This function will write the supplied player to Player.xml
// NOTE: For now, it will ignore equiped equipment, so be sure to
// remove the equiped equipment and put it in the inventory before
// calling this function otherwise it will be lost

int Player::saveToFile(LoadType saveType) {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	char		filename[256];

	ASSERTLOG( this != NULL );
	ASSERTLOG( !getName().empty() );
	ASSERTLOG( isPlayer() );

	if(getName()[0] == '\0') {
		printf("Invalid player passed to save\n");
		return(-1);
	}

	gServer->saveIds();

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Player", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	escapeText();
	saveToXml(rootNode, ALLITEMS, LS_FULL);

	if(saveType == LS_BACKUP) {
		sprintf(filename, "%s/%s.bak.xml", Path::PlayerBackup, getCName());
	} else {
		sprintf(filename, "%s/%s.xml", Path::Player, getCName());
	}

	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
	return(0);
}

//*********************************************************************
//						saveToFile
//*********************************************************************
// This function will write the supplied room to ROOM_PATH/r#####.xml
// It will most likely only be called from *save r

int UniqueRoom::saveToFile(int permOnly, LoadType saveType) {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	char		filename[256];

	ASSERTLOG( this != NULL );
	ASSERTLOG( info.id >= 0 );
	if(saveType == LS_BACKUP)
		Path::checkDirExists(info.area, roomBackupPath);
	else
		Path::checkDirExists(info.area, roomPath);

	gServer->saveIds();

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Room", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	escapeText();
	saveToXml(rootNode, permOnly);

	if(saveType == LS_BACKUP)
		strcpy(filename, roomBackupPath(info));
	else
		strcpy(filename, roomPath(info));
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
	return(0);
}

//*********************************************************************
//						saveToFile
//*********************************************************************
// This function will write the supplied creature to the proper
// m##.xml file - To accomplish this it parse the proper m##.xml file if
// available and walk down the tree to the proper place in numerical order
// if the file does not exist it will create it with only that monster in it.
// It will most likely only be called from *save c

int Monster::saveToFile() {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	char		filename[256];

	// If we can't clean the monster properly, don't save anything
	if(cleanMobForSaving() != 1)
		return(-1);

	// Invalid Number
	if(info.id < 0)
		return(-1);
	Path::checkDirExists(info.area, monsterPath);

	gServer->saveIds();

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Creature", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	escapeText();
	bstring idTemp = id;
	id = "-1";
	saveToXml(rootNode, ALLITEMS, LS_FULL);
	id = idTemp;

	strcpy(filename, monsterPath(info));
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
	return(0);
}

//*********************************************************************
//						saveObject
//*********************************************************************
// This function will write the supplied object to the proper
// o##.xml file - To accomplish this it parse the proper o##.xml file if
// available and walk down the tree to the proper place in numerical order
// if the file does not exist it will create it with only that object in it.
// It will most likely only be called from *save o

int Object::saveToFile() {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	char		filename[256];

	ASSERTLOG( this );

	// Invalid Number
	if(info.id < 0)
		return(-1);
	Path::checkDirExists(info.area, objectPath);

	gServer->saveIds();

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Object", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	// make sure uniqueness stays intact
	setFlag(O_UNIQUE);
	if(!gConfig->getUnique(this))
		clearFlag(O_UNIQUE);

	escapeText();
	saveToXml(rootNode, ALLITEMS, LS_PROTOTYPE, false);

	strcpy(filename, objectPath(info));
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
	return(0);
}

//*********************************************************************
//						saveGuilds
//*********************************************************************
// Causes the guilds structure to be generated into an xml tree and saved

bool Config::saveGuilds() const {
	GuildCreation * gcp;
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	//xmlNodePtr		curNode, bankNode, membersNode;
	char		filename[80];
	//int i;
	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Guilds", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);
	xml::newNumProp(rootNode, "NextGuildId", nextGuildId);

	std::map<int, Guild*>::const_iterator it;
	const Guild *guild;
	for(it = guilds.begin() ; it != guilds.end() ; it++) {
		guild = (*it).second;
		if(guild->getNum() == 0)
			continue;
		guild->saveToXml(rootNode);
	}

	std::list<GuildCreation*>::const_iterator gcIt;
	for(gcIt = guildCreations.begin() ; gcIt != guildCreations.end() ; gcIt++) {
		gcp = (*gcIt);
		gcp->saveToXml(rootNode);
	}

	sprintf(filename, "%s/guilds.xml", Path::PlayerData);
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
	return(true);
}

//*********************************************************************
//						saveBans
//*********************************************************************

bool Config::saveBans() const {
	int found=0;
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	xmlNodePtr	curNode;
	char		filename[80];

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL,BAD_CAST "Bans", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	std::list<Ban*>::const_iterator it;
	Ban* ban;

	for(it = bans.begin() ; it != bans.end() ; it++) {
		found++;
		ban = (*it);

		curNode = xmlNewChild(rootNode, NULL, BAD_CAST "Ban", NULL);
		// Site
		xmlNewChild(curNode, NULL, BAD_CAST "Site", BAD_CAST ban->site.c_str());
		// Duration
		//sprintf(buf, "%d", ban->duration);
		xml::newNumChild(curNode, "Duration", ban->duration);
		// Unban Time
		//sprintf(buf, "%ld", ban->unbanTime);
		xml::newNumChild(curNode, "UnbanTime", ban->unbanTime);
		// Banned By
		xmlNewChild(curNode, NULL, BAD_CAST "BannedBy", BAD_CAST ban->by.c_str());
		// Ban Time
		xmlNewChild(curNode, NULL, BAD_CAST "BanTime", BAD_CAST ban->time.c_str());
		// Reason
		xmlNewChild(curNode, NULL, BAD_CAST "Reason", BAD_CAST ban->reason.c_str());
		// Password
		xmlNewChild(curNode, NULL, BAD_CAST "Password", BAD_CAST ban->password.c_str());
		// Suffix
		xmlNewChild(curNode, NULL, BAD_CAST "Suffix", BAD_CAST iToYesNo(ban->isSuffix));
		// Prefix
		xmlNewChild(curNode, NULL, BAD_CAST "Prefix", BAD_CAST iToYesNo(ban->isPrefix));
	}
	sprintf(filename, "%s/bans.xml", Path::Config);
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
	return(true);
}

//*********************************************************************
//						saveToXml
//*********************************************************************
// This will save the entire creature with anything non zero to the given
// node which should be a creature or player node

int Creature::saveToXml(xmlNodePtr rootNode, int permOnly, LoadType saveType, bool saveID) const {
//	xmlNodePtr	rootNode;
	xmlNodePtr		curNode;
	xmlNodePtr		childNode;
	int i;

	if(getName()[0] == '\0' || rootNode == NULL)
		return(-1);

	const Player	*pPlayer = getAsConstPlayer();
	const Monster	*mMonster = getAsConstMonster();

	if(pPlayer) {
		xml::newProp(rootNode, "Name", pPlayer->getName());
		xml::newProp(rootNode, "Password", pPlayer->getPassword());
		xml::newNumProp(rootNode, "LastLogin", pPlayer->getLastLogin());
	} else if(mMonster) {
		// Saved for LS_REF and LS_FULL
		xml::newNumProp(rootNode, "Num", mMonster->info.id);
		xml::saveNonNullString(rootNode, "Name", mMonster->getName());
		xml::newProp(rootNode, "Area", mMonster->info.area);
	}

	if(saveID == true)
		xml::saveNonNullString(rootNode, "Id", getId());
	else
		xml::newProp(rootNode, "ID", "-1");

	// For the future, when we change things, can read them in based on the version they
	// were saved in before or do modifications based on the new version
	xml::newProp(rootNode, "Version", VERSION);

	if(pPlayer) {
		if(pPlayer->inAreaRoom()) {
			curNode = xml::newStringChild(rootNode, "AreaRoom");
			pPlayer->getConstAreaRoomParent()->mapmarker.save(curNode);
			//pPlayer->currentLocation.mapmarker.save(curNode);
		} else
			pPlayer->currentLocation.room.save(rootNode, "Room", true);
	}

	// Saved for LS_REF and LS_FULL
	xml::saveNonZeroNum(rootNode, "Race", race);
	xml::saveNonZeroNum(rootNode, "Class", cClass);
	if(pPlayer) {
		xml::saveNonZeroNum(rootNode, "Class2", pPlayer->getSecondClass());
	} else if(mMonster) {
		// TODO: Dom: for compatability, remove when possible
		xml::saveNonZeroNum(rootNode, "Class2", mMonster->getMobTrade());
		xml::saveNonZeroNum(rootNode, "MobTrade", mMonster->getMobTrade());
	}

	xml::saveNonZeroNum(rootNode, "Level", level);
	xml::saveNonZeroNum(rootNode, "Type", (int)type);
	xml::saveNonZeroNum(rootNode, "Experience", experience);
	coins.save("Coins", rootNode);
	xml::saveNonZeroNum(rootNode, "Alignment", alignment);

	curNode = xml::newStringChild(rootNode, "Stats");
	strength.save(curNode, "Strength");
	dexterity.save(curNode, "Dexterity");
	constitution.save(curNode, "Constitution");
	intelligence.save(curNode, "Intelligence");
	piety.save(curNode, "Piety");
	hp.save(curNode, "Hp");
	mp.save(curNode, "Mp");

	if(pPlayer)
		pPlayer->focus.save(curNode, "Focus");

	// Only saved for LS_FULL saves, or player saves
	if(saveType == LS_FULL || pPlayer) {
		xml::saveNonNullString(rootNode, "Description", description);

		// Keys
		curNode = xml::newStringChild(rootNode, "Keys");
		for(i=0; i<3; i++) {
			if(key[i][0] == 0)
				continue;
			childNode = xml::newStringChild(curNode, "Key", key[i]);
			xml::newNumProp(childNode, "Num", i);
		}

		// MoveTypes
		curNode = xml::newStringChild(rootNode, "MoveTypes");
		for(i=0; i<3; i++) {
			if(movetype[i][0] == 0)
				continue;
			childNode = xml::newStringChild(curNode, "MoveType", movetype[i]);
			xml::newNumProp(childNode, "Num", i);
		}


		xml::saveNonZeroNum(rootNode, "Armor", armor);
		xml::saveNonZeroNum(rootNode, "Deity", deity);


		saveULongArray(rootNode, "Realms", "Realm", realm, MAX_REALM-1);
		saveLongArray(rootNode, "Proficiencies", "Proficiency", proficiency, 6);

		damage.save(rootNode, "Dice");

		xml::saveNonZeroNum(rootNode, "Clan", clan);
		xml::saveNonZeroNum(rootNode, "PoisonDuration", poison_dur);
		xml::saveNonZeroNum(rootNode, "PoisonDamage", poison_dmg);
		xml::saveNonZeroNum(rootNode, "Size", size);

		if(pPlayer) pPlayer->saveXml(rootNode);
		else if(mMonster) mMonster->saveXml(rootNode);

		saveFactions(rootNode);
		saveSkills(rootNode);

		hooks.save(rootNode, "Hooks");

		effects.save(rootNode, "Effects");
		saveAttacks(rootNode);

		if(!minions.empty()) {
			curNode = xml::newStringChild(rootNode, "Minions");
			std::list<bstring>::const_iterator mIt;
			for(mIt = minions.begin() ; mIt != minions.end() ; mIt++) {
				childNode = xml::newStringChild(curNode, "Minion", (*mIt));
			}
		}

		//		saveShortIntArray(rootNode, "Factions", "Faction", faction, MAX_FACTION);

		// Save dailys
		curNode = xml::newStringChild(rootNode, "DailyTimers");
		for(i=0; i<DAILYLAST+1; i++)
			saveDaily(curNode, i, daily[i]);

		// Save saving throws
		curNode = xml::newStringChild(rootNode, "SavingThrows");
		for(i=0; i<MAX_SAVE; i++)
			saveSavingThrow(curNode, i, saves[i]);

		// Perhaps change this into saveInt/CharArray
		saveBits(rootNode, "Spells", MAXSPELL, spells);
		saveBits(rootNode, "Quests", MAXSPELL, quests);
		xml::saveNonZeroNum(rootNode, "CurrentLanguage", current_language);
		saveBits(rootNode, "Languages", LANGUAGE_COUNT, languages);
	}

	// Saved for LS_FULL and LS_REF
	saveBits(rootNode, "Flags", pPlayer ? MAX_PLAYER_FLAGS : MAX_MONSTER_FLAGS, flags);

	// Save lasttimes
	for(i=0; i<TOTAL_LTS; i++) {
		// this nested loop means we won't create an xml node if we don't have to
		if(lasttime[i].interval || lasttime[i].ltime || lasttime[i].misc) {
			curNode = xml::newStringChild(rootNode, "LastTimes");
			for(; i<TOTAL_LTS; i++)
				saveLastTime(curNode, i, lasttime[i]);
		}
	}

	curNode = xml::newStringChild(rootNode, "Inventory");
	saveObjectsXml(curNode, objects, permOnly);

	// We want quests saved after inventory so when they are loaded we can calculate
	// if the quest is complete or not and store it in the appropriate variables
	if(pPlayer)
		pPlayer->saveQuests(rootNode);

	return(0);
}

//*********************************************************************
//						saveXml
//*********************************************************************

void Monster::saveXml(xmlNodePtr curNode) const {
	xmlNodePtr childNode, subNode;

	// record monsters saved during swap
	if(gConfig->swapIsInteresting(this))
		gConfig->swapLog((bstring)"m" + info.rstr(), false);

	xml::saveNonNullString(curNode, "Plural", plural);
	xml::saveNonZeroNum(curNode, "SkillLevel", skillLevel);
	xml::saveNonZeroNum(curNode, "UpdateAggro", updateAggro);
	xml::saveNonZeroNum(curNode, "LoadAggro", loadAggro);
	// Attacks
	childNode = xml::newStringChild(curNode, "Attacks");
	for(int i=0; i<3; i++) {
		if(attack[i][0] == 0)
			continue;
		subNode = xml::newStringChild(childNode, "Attack", attack[i]);
		xml::newNumProp(subNode, "Num", i);
	}
	xml::saveNonNullString(curNode, "LastMod", last_mod);
	xml::saveNonNullString(curNode, "Talk", talk);
	xmlNodePtr talkNode = xml::newStringChild(curNode, "TalkResponses");
	for(TalkResponse * talkResponse : responses)
		talkResponse->saveToXml(talkNode);

	xml::saveNonNullString(curNode, "TradeTalk", ttalk);
	xml::saveNonZeroNum(curNode, "NumWander", numwander);
	xml::saveNonZeroNum(curNode, "MagicResistance", magicResistance);
	xml::saveNonZeroNum(curNode, "DefenseSkill", defenseSkill);
	xml::saveNonZeroNum(curNode, "AttackPower", attackPower);
	xml::saveNonZeroNum(curNode, "WeaponSkill", weaponSkill);
	saveCarryArray(curNode, "CarriedItems", "Carry", carry, 10);
	saveCatRefArray(curNode, "AssistMobs", "Mob", assist_mob, NUM_ASSIST_MOB);
	saveCatRefArray(curNode, "EnemyMobs", "Mob", enemy_mob, NUM_ENEMY_MOB);

	xml::saveNonNullString(curNode, "PrimeFaction", primeFaction);
	xml::saveNonNullString(curNode, "AggroString", aggroString);

	jail.save(curNode, "Jail", false);
	xml::saveNonZeroNum(curNode, "WeaponSkill", maxLevel);
	xml::saveNonZeroNum(curNode, "Cast", cast);
	saveCatRefArray(curNode, "Rescue", "Mob", rescue, NUM_RESCUE);

	saveBits(curNode, "ClassAggro", 32, cClassAggro);
	saveBits(curNode, "RaceAggro", 32, raceAggro);
	saveBits(curNode, "DeityAggro", 32, deityAggro);
}

//*********************************************************************
//						saveXml
//*********************************************************************

void Player::saveXml(xmlNodePtr curNode) const {
	xmlNodePtr childNode;
	int i;

	// record people logging off during swap
	if(gConfig->swapIsInteresting(this))
		gConfig->swapLog((bstring)"p" + getName(), false);

	bank.save("Bank", curNode);
	xml::saveNonZeroNum(curNode, "WeaponTrains", weaponTrains);
	xml::saveNonNullString(curNode, "Surname", surname);
	xml::saveNonNullString(curNode, "Forum", forum);
	xml::newNumChild(curNode, "Wrap", wrap);
	bound.save(curNode, "BoundRoom");
	previousRoom.save(curNode, "PreviousRoom"); // monster do not save PreviousRoom
	statistics.save(curNode, "Statistics");
	xml::saveNonZeroNum(curNode, "ActualLevel", actual_level);
	xml::saveNonZeroNum(curNode, "Created", created);
	xml::saveNonNullString(curNode, "OldCreated", oldCreated);
	xml::saveNonZeroNum(curNode, "Guild", guild);
	if(title != " ")
		xml::saveNonNullString(curNode, "Title", title);
	xml::saveNonZeroNum(curNode, "GuildRank", guildRank);
	if(isStaff()) {
		childNode = xml::newStringChild(curNode, "Ranges");
		for(i=0; i<MAX_BUILDER_RANGE; i++) {
			// empty range, skip it
			if(!bRange[i].low.id && !bRange[i].high)
				continue;
			bRange[i].save(childNode, "Range", i);
		}
	}

	xml::saveNonZeroNum(curNode, "NegativeLevels", negativeLevels);
	xml::saveNonZeroNum(curNode, "LastInterest", lastInterest);
	xml::saveNonZeroNum(curNode, "TickDmg", tickDmg);

	xml::saveNonNullString(curNode, "CustomColors", customColors);
	xml::saveNonZeroNum(curNode, "Thirst", thirst);
	saveBits(curNode, "Songs", gConfig->getMaxSong(), songs);

	std::list<CatRef>::const_iterator it;

	if(!storesRefunded.empty()) {
	    childNode = xml::newStringChild(curNode, "StoresRefunded");
	    for(it = storesRefunded.begin() ; it != storesRefunded.end() ; it++) {
	        (*it).save(childNode, "Store", true);
	    }
	}

	if(!roomExp.empty()) {
		childNode = xml::newStringChild(curNode, "RoomExp");
		for(it = roomExp.begin() ; it != roomExp.end() ; it++) {
			(*it).save(childNode, "Room", true);
		}
	}
	if(!objIncrease.empty()) {
		childNode = xml::newStringChild(curNode, "ObjIncrease");
		for(it = objIncrease.begin() ; it != objIncrease.end() ; it++) {
			(*it).save(childNode, "Object", true);
		}
	}
	if(!lore.empty()) {
		childNode = xml::newStringChild(curNode, "Lore");
		for(it = lore.begin() ; it != lore.end() ; it++) {
			(*it).save(childNode, "Info", true);
		}
	}
	std::list<int>::const_iterator rt;
	if(!recipes.empty()) {
		childNode = xml::newStringChild(curNode, "Recipes");
		for(rt = recipes.begin() ; rt != recipes.end() ; rt++) {
			xml::newNumChild(childNode, "Recipe", (*rt));
		}
	}

	// Save any Anchors
	for(i=0; i<MAX_DIMEN_ANCHORS; i++) {
		if(anchor[i]) {
			childNode = xml::newStringChild(curNode, "Anchors");
			for(; i<MAX_DIMEN_ANCHORS; i++)
				if(anchor[i]) {
					xmlNodePtr subNode = xml::newStringChild(childNode, "Anchor");
					xml::newNumProp(subNode, "Num", i);
					anchor[i]->save(subNode);
				}
			break;
		}
	}

	xml::saveNonZeroNum(curNode, "Wimpy", wimpy);

	childNode = xml::newStringChild(curNode, "Pets");
	savePets(childNode);

	if(birthday) {
		childNode = xml::newStringChild(curNode, "Birthday");
		birthday->save(childNode);
	}

//			childNode = xml::newStringChild(curNode, "Deaths", NULL);
//			for(i=0; i<LUCKY_DEATHS; i++) {
//				if(lastDeath[i]) {
//					subNode = xml::newStringChild(childNode, "Death", NULL);
//					xml::saveNonZeroInt(subNode, "Time", lastDeath[i]);
//					xml::saveNonNullString(subNode, "Text", killedBy[i]);
//				}
//			}
	xml::saveNonNullString(curNode, "LastPassword", lastPassword);
	xml::saveNonNullString(curNode, "PoisonedBy", poisonedBy);
	xml::saveNonNullString(curNode, "AfflictedBy", afflictedBy);
}

//*********************************************************************
//						saveQuests
//*********************************************************************

void Player::saveQuests(xmlNodePtr rootNode) const {
	xmlNodePtr questNode;

	questNode = xml::newStringChild(rootNode, "QuestsInProgress");
	for(std::pair<int, QuestCompletion*> p : questsInProgress) {
		p.second->save(questNode);
	}

	questNode = xml::newStringChild(rootNode, "QuestsCompleted");
	if(!questsCompleted.empty()) {
		xmlNodePtr completionNode;
		for(std::pair<int,int> qp : questsCompleted) {
			completionNode = xml::newStringChild(questNode, "Quest");
			xml::newNumChild(completionNode, "Id", qp.first);
			xml::newNumChild(completionNode, "Num", qp.second);
		}
	}
//	for(it = questsCompleted.begin() ; it != questsCompleted.end() ; it++) {
//		xml::newNumChild(questNode, "Quest", *it);
//	}
}

//*********************************************************************
//						saveFactions
//*********************************************************************

void Creature::saveFactions(xmlNodePtr rootNode) const {
	xmlNodePtr curNode = xml::newStringChild(rootNode, "Factions");
	xmlNodePtr factionNode;
	std::map<bstring, long>::const_iterator fIt;
	for(fIt = factions.begin() ; fIt != factions.end() ; fIt++) {
		factionNode = xml::newStringChild(curNode, "Faction");
		xml::newStringChild(factionNode, "Name", (*fIt).first);
		xml::newNumChild(factionNode, "Regard", (*fIt).second);
	}
}

//*********************************************************************
//						saveSkills
//*********************************************************************

void Creature::saveSkills(xmlNodePtr rootNode) const {
	xmlNodePtr curNode = xml::newStringChild(rootNode, "Skills");
	std::map<bstring, Skill*>::const_iterator sIt;
	for(sIt = skills.begin() ; sIt != skills.end() ; sIt++) {
		(*sIt).second->save(curNode);
	}
}

//*********************************************************************
//						save
//*********************************************************************

void Effects::save(xmlNodePtr rootNode, const char* name) const {
	xmlNodePtr curNode = xml::newStringChild(rootNode, name);
	EffectList::const_iterator eIt;
	for(eIt = effectList.begin() ; eIt != effectList.end() ; eIt++) {
		(*eIt)->save(curNode);
	}
}

//*********************************************************************
//						save
//*********************************************************************

void EffectInfo::save(xmlNodePtr rootNode) const {
	xmlNodePtr effectNode = xml::newStringChild(rootNode, "Effect");
	xml::newStringChild(effectNode, "Name", name);
	xml::newNumChild(effectNode, "Duration", duration);
	xml::newNumChild(effectNode, "Strength", strength);
	xml::newNumChild(effectNode, "Extra", extra);
	xml::newNumChild(effectNode, "PulseModifier", pulseModifier);
}

//*********************************************************************
//						saveAttacks
//*********************************************************************

void Creature::saveAttacks(xmlNodePtr rootNode) const {
	xmlNodePtr curNode = xml::newStringChild(rootNode, "SpecialAttacks");
	std::list<SpecialAttack*>::const_iterator eIt;
	for(eIt = specials.begin() ; eIt != specials.end() ; eIt++) {
		(*eIt)->save(curNode);
	}
}

//*********************************************************************
//						save
//*********************************************************************

void Skill::save(xmlNodePtr rootNode) const {
	xmlNodePtr skillNode = xml::newStringChild(rootNode, "Skill");
	xml::newStringChild(skillNode, "Name", getName());
	xml::newNumChild(skillNode, "Gained", getGained());
	xml::newNumChild(skillNode, "GainBonus", getGainBonus());
}


int AlchemyEffect::saveToXml(xmlNodePtr rootNode) {
	if(rootNode == NULL)
		return(-1);
	xml::newStringChild(rootNode, "Effect", effect);
	xml::newNumChild(rootNode, "Duration", duration);
	xml::newNumChild(rootNode, "Strength", strength);
	xml::newNumChild(rootNode, "Quality", quality);

	return(0);
}

//*********************************************************************
//						saveToXml
//*********************************************************************
// This function will save the entire object except 0 values to the
// given node which should be an object node unless saveType is REF
// in which case it will only save fields that are changed in the
// ordinary course of the game

int Object::saveToXml(xmlNodePtr rootNode, int permOnly, LoadType saveType, int quantity, bool saveId, std::list<bstring> *idList) const {
//	xmlNodePtr	rootNode;
	xmlNodePtr		curNode;
	xmlNodePtr		childNode;

	int i=0;

	if(rootNode == NULL)
		return(-1);

	ASSERTLOG( info.id >= 0 );
	ASSERTLOG( info.id < OMAX );

	// If the object's index is 0, then we have to do a full save
	if(!info.id && saveType != LS_FULL && saveType != LS_PROTOTYPE) {
		// We should never get here...if it's a 0 index, should have O_SAVE_FULL set
		printf("ERROR: Forcing full save.\n");
		saveType = LS_FULL;
	}

	// record objects saved during swap
	if(gConfig->swapIsInteresting(this))
		gConfig->swapLog((bstring)"o" + info.rstr(), false);

	xml::newNumProp(rootNode, "Num", info.id);
	xml::newProp(rootNode, "Area", info.area);
	xml::newProp(rootNode, "Version", VERSION);
	if(quantity > 1) {
		xml::newNumProp(rootNode, "Quantity", quantity);
		curNode = xml::newStringChild(rootNode, "IdList");
		if(idList != 0) {
			std::list<bstring>::iterator idIt = idList->begin();
			while(idIt != idList->end()) {
				xml::newStringChild(curNode, "Id", (*idIt++));
			}
		}
	} else {
		if(saveId == true)
			xml::saveNonNullString(rootNode, "Id", getId());
		else
			xml::newProp(rootNode, "ID", "-1");
	}

	// These are saved for full and reference
	xml::saveNonNullString(rootNode, "Name", getName());
	if(saveType != LS_PROTOTYPE) {
		droppedBy.save(rootNode);
		xml::saveNonZeroNum(rootNode, "ShopValue", shopValue);
	}


	xml::saveNonNullString(rootNode, "Plural", plural);
	xml::saveNonZeroNum(rootNode, "Adjustment", adjustment);
	xml::saveNonZeroNum(rootNode, "ShotsMax", shotsMax);
	xml::saveNonZeroNum(rootNode, "ChargesMax", chargesMax);
	// NOTE: This is saved even if zero so that it loads broken items
	// properly.
	xml::newNumChild(rootNode, "ShotsCur", shotsCur);
	xml::newNumChild(rootNode, "ChargesCur", chargesCur);
	xml::saveNonZeroNum(rootNode, "Armor", armor);
	xml::saveNonZeroNum(rootNode, "NumAttacks", numAttacks);
	xml::saveNonZeroNum(rootNode, "Delay", delay);
	xml::saveNonZeroNum(rootNode, "Extra", extra);
	xml::saveNonZeroNum(rootNode, "LotteryCycle", lotteryCycle);

	if(type == LOTTERYTICKET) {
		// Lottery tickets have a custom description so be sure to save it
		xml::saveNonNullString(rootNode, "Description", description);
		saveShortIntArray(rootNode, "LotteryNumbers", "LotteryNum", lotteryNumbers, 6);
	}

	damage.save(rootNode, "Dice");
	value.save("Value", rootNode);
	hooks.save(rootNode, "Hooks");

	// Save the keys in case they were modified
	// Keys
	curNode = xml::newStringChild(rootNode, "Keys");
	for(i=0; i<3; i++) {
		if(key[i][0] == 0)
			continue;
		childNode = xml::newStringChild(curNode, "Key", key[i]);
		xml::newNumProp(childNode, "Num", i);
	}

	for(i=0; i<4; i++) {
		// this nested loop means we won't create an xml node if we don't have to
		if(lasttime[i].interval || lasttime[i].ltime || lasttime[i].misc) {
			curNode = xml::newStringChild(rootNode, "LastTimes");
			for(; i<4; i++)
				saveLastTime(curNode, i, lasttime[i]);
		}
	}

	xml::saveNonNullString(rootNode, "Effect", effect);
	xml::saveNonZeroNum(rootNode, "EffectDuration", effectDuration);
	xml::saveNonZeroNum(rootNode, "EffectStrength", effectStrength);

	xml::saveNonZeroNum(rootNode, "Recipe", recipe);
	xml::saveNonZeroNum(rootNode, "Made", made);
	xml::saveNonNullString(rootNode, "Owner", questOwner);

	if(saveType != LS_FULL && saveType != LS_PROTOTYPE) {
		// Save only a subset of flags for obj references
		saveObjRefFlags(rootNode, "Flags", MAX_OBJECT_FLAGS, flags);
	}

	if(saveType == LS_FULL || saveType == LS_PROTOTYPE) {
		saveBits(rootNode, "Flags", MAX_OBJECT_FLAGS, flags);
		// These are only saved for full objects
		if(type != LOTTERYTICKET)
			xml::saveNonNullString(rootNode, "Description", description);


		if(!alchemyEffects.empty()) {
			curNode = xml::newStringChild(rootNode, "AlchemyEffects");

			for(std::pair<int, AlchemyEffect> p : alchemyEffects) {
				childNode = xml::newStringChild(curNode, "AlchemyEffect");
				p.second.saveToXml(childNode);
				xml::newNumProp(childNode, "Num", p.first);
			}
		}
		xml::saveNonNullString(rootNode, "UseOutput", use_output);
		xml::saveNonNullString(rootNode, "UseAttack", use_attack);
		xml::saveNonNullString(rootNode, "LastMod", lastMod);

		xml::saveNonZeroNum(rootNode, "Weight", weight);
		xml::saveNonZeroNum(rootNode, "Type", type);
		xml::saveNonNullString(rootNode, "SubType", subType);

		xml::saveNonZeroNum(rootNode, "WearFlag", wearflag);
		xml::saveNonZeroNum(rootNode, "MagicPower", magicpower);

		xml::saveNonZeroNum(rootNode, "Level", level);
		xml::saveNonZeroNum(rootNode, "Quality", quality);
		xml::saveNonZeroNum(rootNode, "RequiredSkill", requiredSkill);
		xml::saveNonZeroNum(rootNode, "Clan", clan);
		xml::saveNonZeroNum(rootNode, "Special", special);
		xml::saveNonZeroNum(rootNode, "QuestNum", questnum);

		xml::saveNonZeroNum(rootNode, "Bulk", bulk);
		xml::saveNonZeroNum(rootNode, "Size", size);
		xml::saveNonZeroNum(rootNode, "MaxBulk", maxbulk);

		xml::saveNonZeroNum(rootNode, "CoinCost", coinCost);

		deed.save(rootNode, "Deed", false);

		xml::saveNonZeroNum(rootNode, "KeyVal", keyVal);
		xml::saveNonZeroNum(rootNode, "Material", (int)material);
		xml::saveNonZeroNum(rootNode, "MinStrength", minStrength);

		saveCatRefArray(rootNode, "InBag", "Obj", in_bag, 3);

		std::list<CatRef>::const_iterator it;
		if(!randomObjects.empty()) {
			curNode = xml::newStringChild(rootNode, "RandomObjects");
			for(it = randomObjects.begin() ; it != randomObjects.end() ; it++) {
				(*it).save(curNode, "RandomObject", false);
			}
		}

	}

	if(compass) {
		curNode = xml::newStringChild(rootNode, "Compass");
		compass->save(curNode);
	}
	if(increase) {
		curNode = xml::newStringChild(rootNode, "ObjIncrease");
		increase->save(curNode);
	}

	// Save contained items for both full and reference saves
	// Also check for container flag
	if(saveType != LS_PROTOTYPE && !objects.empty()) {
		curNode = xml::newStringChild(rootNode, "SubItems");
		// If we're a permenant container, always save the items inside of it
		if(type == CONTAINER && flagIsSet(O_PERM_ITEM))
			permOnly = ALLITEMS;
		saveObjectsXml(curNode, objects, permOnly);
	}
	return(0);
}

//*********************************************************************
//						saveToXml
//*********************************************************************

int UniqueRoom::saveToXml(xmlNodePtr rootNode, int permOnly) const {
	std::map<int, crlasttime>::const_iterator it;
	xmlNodePtr		curNode;
	int i;

	if(!this || getName()[0] == '\0' || rootNode == NULL)
		return(-1);

	// record rooms saved during swap
	if(gConfig->swapIsInteresting(this))
		gConfig->swapLog((bstring)"r" + info.rstr(), false);

	xml::newNumProp(rootNode, "Num", info.id);
	xml::newProp(rootNode, "Version", VERSION);
	xml::newProp(rootNode, "Area", info.area);

	xml::saveNonNullString(rootNode, "Name", getName());
	xml::saveNonNullString(rootNode, "ShortDescription", short_desc);
	xml::saveNonNullString(rootNode, "LongDescription", long_desc);
	xml::saveNonNullString(rootNode, "Fishing", fishing);
	xml::saveNonNullString(rootNode, "Faction", faction);
	xml::saveNonNullString(rootNode, "LastModBy", last_mod);
	// TODO: Change this into a TimeStamp
	xml::saveNonNullString(rootNode, "LastModTime", lastModTime);
	xml::saveNonNullString(rootNode, "LastPlayer", lastPly);
	// TODO: Change this into a TimeStamp
	xml::saveNonNullString(rootNode, "LastPlayerTime", lastPlyTime);


	xml::saveNonZeroNum(rootNode, "LowLevel", lowLevel);
	xml::saveNonZeroNum(rootNode, "HighLevel", highLevel);
	xml::saveNonZeroNum(rootNode, "MaxMobs", maxmobs);

	xml::saveNonZeroNum(rootNode, "Trap", trap);
	trapexit.save(rootNode, "TrapExit", false);
	xml::saveNonZeroNum(rootNode, "TrapWeight", trapweight);
	xml::saveNonZeroNum(rootNode, "TrapStrength", trapstrength);

	xml::saveNonZeroNum(rootNode, "BeenHere", beenhere);
	xml::saveNonZeroNum(rootNode, "RoomExp", roomExp);

	saveBits(rootNode, "Flags", MAX_ROOM_FLAGS, flags);

	curNode = xml::newStringChild(rootNode, "Wander");
	wander.save(curNode);

	if(track.getDirection() != "") {
		curNode = xml::newStringChild(rootNode, "Track");
		track.save(curNode);
	}

	xml::saveNonZeroNum(rootNode, "Size", size);
	effects.save(rootNode, "Effects");
	hooks.save(rootNode, "Hooks");



	// Save Perm Mobs
	curNode = xml::newStringChild(rootNode, "PermMobs");
	for(it = permMonsters.begin(); it != permMonsters.end() ; it++) {
		saveCrLastTime(curNode, (*it).first, (*it).second);
	}

	// Save Perm Objs
	curNode = xml::newStringChild(rootNode, "PermObjs");
	for(it = permObjects.begin(); it != permObjects.end() ; it++) {
		saveCrLastTime(curNode, (*it).first, (*it).second);
	}

	// Save LastTimes -- Dunno if we need this?
	for(i=0; i<16; i++) {
		// this nested loop means we won't create an xml node if we don't have to
		if(lasttime[i].interval || lasttime[i].ltime || lasttime[i].misc) {
			curNode = xml::newStringChild(rootNode, "LastTimes");
			for(; i<16; i++)
				saveLastTime(curNode, i, lasttime[i]);
		}
	}

	// Save Objects
	curNode = xml::newStringChild(rootNode, "Objects");
	saveObjectsXml(curNode, objects, permOnly);

	// Save Creatures
	curNode = xml::newStringChild(rootNode, "Creatures");
	saveCreaturesXml(curNode, monsters, permOnly);

	// Save Exits
	curNode = xml::newStringChild(rootNode, "Exits");
	saveExitsXml(curNode);
	return(0);
}

//*********************************************************************
//						saveToXml
//*********************************************************************

int Exit::saveToXml(xmlNodePtr parentNode) const {
	xmlNodePtr	rootNode;
	xmlNodePtr		curNode;
	xmlNodePtr		childNode;

	int i;

	if(this == NULL || parentNode == NULL || flagIsSet(X_PORTAL))
		return(-1);

	rootNode = xml::newStringChild(parentNode, "Exit");
	xml::newProp(rootNode, "Name", getName());

	// Exit Keys
	curNode = xml::newStringChild(rootNode, "Keys");
	for(i=0; i<3; i++) {
		if(desc_key[i][0] == 0)
			continue;
		childNode = xml::newStringChild(curNode, "Key", desc_key[i]);
		xml::newNumProp(childNode, "Num",i);
	}

	target.save(rootNode, "Target");
	xml::saveNonZeroNum(rootNode, "Toll", toll);
	xml::saveNonZeroNum(rootNode, "Level", level);
	xml::saveNonZeroNum(rootNode, "Trap", trap);
	xml::saveNonZeroNum(rootNode, "Key", key);
	xml::saveNonNullString(rootNode, "KeyArea", keyArea);
	xml::saveNonZeroNum(rootNode, "Size", size);
	xml::saveNonZeroNum(rootNode, "Direction", (int)direction);
	xml::saveNonNullString(rootNode, "PassPhrase", passphrase);
	xml::saveNonZeroNum(rootNode, "PassLang", passlang);
	xml::saveNonNullString(rootNode, "Description", description);
	xml::saveNonNullString(rootNode, "Enter", enter);
	xml::saveNonNullString(rootNode, "Open", open);
	effects.save(rootNode, "Effects");

	saveBits(rootNode, "Flags", MAX_EXIT_FLAGS, flags);
	saveLastTime(curNode, 0, ltime);
	hooks.save(rootNode, "Hooks");

	return(0);
}

//*********************************************************************
//						saveExitsXml
//*********************************************************************

int BaseRoom::saveExitsXml(xmlNodePtr curNode) const {
	for(Exit* exit : exits) {
		exit->saveToXml(curNode);
	}
	return(0);
}

//*********************************************************************
//						saveObjectsXml
//*********************************************************************

int saveObjectsXml(xmlNodePtr parentNode, const ObjectSet& set, int permOnly) {
	xmlNodePtr curNode;
	int quantity=0;
	LoadType lt;
	ObjectSet::const_iterator it;
	const Object* obj;
	std::list<bstring> *idList = 0;
	for( it = set.begin() ; it != set.end() ; ) {
		obj = (*it++);
		if(	obj &&
			(	permOnly == ALLITEMS ||
				(permOnly == PERMONLY && obj->flagIsSet(O_PERM_ITEM))
			))
		{
			if(obj->flagIsSet(O_CUSTOM_OBJ) || obj->flagIsSet(O_SAVE_FULL) || !obj->info.id) {
				// If it's a custom or has the save flag set, save the entire object
				curNode = xml::newStringChild(parentNode, "Object");
				lt = LS_FULL;
			} else {
				// Just save a reference and any changed fields
				curNode = xml::newStringChild(parentNode, "ObjRef");
				lt = LS_REF;
			}

			// TODO: Modify this to work with object ids
			// quantity code reduces filesize for player shops, storage rooms, and
			// inventories (which tend of have a lot of identical items in them)
			quantity = 1;
			idList = new std::list<bstring>;
			idList->push_back(obj->getId());
			while(it != set.end() && *(*it) == *obj) {
				idList->push_back((*it)->getId());
				quantity++;
				it++;
			}

			obj->saveToXml(curNode, permOnly, lt, quantity, true, idList);
			delete idList;
			idList = 0;
		}
	}
	return(0);
}

//*********************************************************************
//						saveCreaturesXml
//*********************************************************************

int saveCreaturesXml(xmlNodePtr parentNode, const MonsterSet& set, int permOnly) {
	xmlNodePtr curNode;
	for(const Monster* mons : set) {
        if( mons && mons->isMonster() &&
            (   (permOnly == ALLITEMS && !mons->isPet()) ||
                (permOnly == PERMONLY && mons->flagIsSet(M_PERMENANT_MONSTER))
            ) )
        {
            if(mons->flagIsSet(M_SAVE_FULL)) {
                // Save a fully copy of the mob to the node
                curNode = xml::newStringChild(parentNode, "Creature");
                mons->saveToXml(curNode, permOnly, LS_FULL);
            } else {
                // Just save a reference
                curNode = xml::newStringChild(parentNode, "CrtRef");
                mons->saveToXml(curNode, permOnly, LS_REF);
            }
        }

	}
	return(0);
}

//*********************************************************************
//						savePets
//*********************************************************************

void Creature::savePets(xmlNodePtr parentNode) const {
	xmlNodePtr curNode;

	for(const Monster* pet : pets) {
		// Only save pets, not creatures just following
		if(!pet->isPet()) continue;
	    curNode = xml::newStringChild(parentNode, "Creature");
	    pet->saveToXml(curNode, ALLITEMS, LS_FULL);
	}
	return;
}

//*********************************************************************
//						saveToXml
//*********************************************************************

bool Guild::saveToXml(xmlNodePtr rootNode) const {
	xmlNodePtr curNode, membersNode;

	curNode = xml::newStringChild(rootNode, "Guild");
//		xml::newStringChild(curNode, "", );
//		xml::newIntChild(curNode, "", guild->);
	xml::newNumProp(curNode, "ID", num);
	xml::newStringChild(curNode, "Name", name);
	xml::newStringChild(curNode, "Leader", leader);

	xml::newNumChild(curNode, "Level", level);
	xml::newNumChild(curNode, "NumMembers", numMembers);

	membersNode = xml::newStringChild(curNode, "Members");
	std::list<bstring>::const_iterator mIt;
	for(mIt = members.begin() ; mIt != members.end() ; mIt++) {
		xml::newStringChild(membersNode, "Member", *mIt);
	}

	xml::newNumChild(curNode, "PkillsWon", pkillsWon);
	xml::newNumChild(curNode, "PkillsIn", pkillsIn);
	xml::newNumChild(curNode,"Points", points);
	bank.save("Bank", curNode);

	return(true);
}


bool Config::saveSocials() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Socials", NULL);
    xmlDocSetRootElement(xmlDoc, rootNode);

    for(SocialMap::value_type p : socials) {
        p.second->saveToXml(rootNode);
    }
    bstring filename = bstring(Path::Code) + "/" + "socials.xml";
    xml::saveFile(filename.c_str(), xmlDoc);
    xmlFreeDoc(xmlDoc);
    return(true);

}

bool SocialCommand::saveToXml(xmlNodePtr rootNode) const {
    xmlNodePtr curNode;

    curNode = xml::newStringChild(rootNode, "Social");
    xml::newStringChild(curNode, "Name", name);

    if(wakeTarget)
        xml::newBoolChild(curNode, "WakeTarget", wakeTarget);
    if(rudeWakeTarget)
        xml::newBoolChild(curNode, "RudeWakeTarget", rudeWakeTarget);
    if(wakeRoom)
        xml::newBoolChild(curNode, "WakeRoom", wakeRoom);

    xml::saveNonNullString(curNode, "Description", description);
    xml::saveNonZeroNum(curNode, "Priority", priority);

    xml::saveNonNullString(curNode, "SelfNoTarget", selfNoTarget);
    xml::saveNonNullString(curNode, "RoomNoTarget", roomNoTarget);
    xml::saveNonNullString(curNode, "SelfOnTarget", selfOnTarget);
    xml::saveNonNullString(curNode, "RoomOnTarget", roomOnTarget);
    xml::saveNonNullString(curNode, "VictimOnTarget", victimOnTarget);
    xml::saveNonNullString(curNode, "SelfOnSelf", selfOnSelf);
    xml::saveNonNullString(curNode, "RoomOnSelf", roomOnSelf);

    return(true);
}
//*********************************************************************
//						saveToXml
//*********************************************************************

bool GuildCreation::saveToXml(xmlNodePtr rootNode) const {
	xmlNodePtr childNode, curNode = xml::newStringChild(rootNode, "GuildCreation");

	xml::newStringChild(curNode, "Name", name);
	xml::newStringChild(curNode, "Leader", leader);
	xml::newStringChild(curNode, "LeaderIp", leaderIp);
	xml::newNumChild(curNode, "Status", status);

	// Supporters
	std::map<bstring, bstring>::const_iterator sIt;
	for(sIt = supporters.begin() ; sIt != supporters.end() ; sIt++) {
		childNode = xml::newStringChild(curNode, "Supporter", (*sIt).first);
		xml::newProp(childNode, "Ip", (*sIt).second);
	}
	return(true);
}


void DroppedBy::save(xmlNodePtr rootNode) const {
	xmlNodePtr curNode = xml::newStringChild(rootNode, "DroppedBy");
	xml::newStringChild(curNode, "Name", name);
	xml::saveNonNullString(curNode, "Index", index);
	xml::saveNonNullString(curNode, "ID", id);
	xml::newStringChild(curNode, "Type", type);
}


#define BIT_ISSET(p,f)	((p)[(f)/8] & 1<<((f)%8))
#define BIT_SET(p,f)	((p)[(f)/8] |= 1<<((f)%8))
#define BIT_CLEAR(p,f)	((p)[(f)/8] &= ~(1<<((f)%8)))

//*********************************************************************
//						save
//*********************************************************************

void Stat::save(xmlNodePtr parentNode, const char* statName) const {
	xmlNodePtr curNode = xml::newStringChild(parentNode, "Stat");
	xml::newProp(curNode, "Name", statName);

//	xml::newNumChild(curNode, "Current", cur);
//	xml::newNumChild(curNode, "Max", max);
	xml::newNumChild(curNode, "Initial", initial);
	xmlNodePtr modNode = xml::newStringChild(curNode, "Modifiers");
	for(ModifierMap::value_type p: modifiers) {
		p.second->save(modNode);
	}
}

void StatModifier::save(xmlNodePtr parentNode) {
	xmlNodePtr curNode = xml::newStringChild(parentNode, "StatModifier");
	xml::newStringChild(curNode, "Name", name);
	xml::newNumChild(curNode, "ModAmt", modAmt);
	xml::newNumChild(curNode, "ModType", modType);
}
//*********************************************************************
//						saveDaily
//*********************************************************************

xmlNodePtr saveDaily(xmlNodePtr parentNode, int i, struct daily pDaily) {
	// Avoid writing un-used daily timers
	if(pDaily.max == 0 && pDaily.cur == 0 && pDaily.ltime == 0)
		return(NULL);

	xmlNodePtr curNode = xml::newStringChild(parentNode, "Daily");
	xml::newNumProp(curNode, "Num", i);

	xml::newNumChild(curNode, "Max", pDaily.max);
	xml::newNumChild(curNode, "Current", pDaily.cur);
	xml::newNumChild(curNode, "LastTime", pDaily.ltime);
	return(curNode);
}

//*********************************************************************
//						saveCrLastTime
//*********************************************************************

xmlNodePtr saveCrLastTime(xmlNodePtr parentNode, int i, struct crlasttime pCrLastTime) {
	// Avoid writing un-used last times
	if(!pCrLastTime.interval && !pCrLastTime.ltime && !pCrLastTime.cr.id)
		return(NULL);

	if(i < 0 || i >= NUM_PERM_SLOTS)
		return(NULL);

	xmlNodePtr curNode = xml::newStringChild(parentNode, "LastTime");
	xml::newNumProp(curNode, "Num", i);

	xml::newNumChild(curNode, "Interval", pCrLastTime.interval);
	xml::newNumChild(curNode, "LastTime", pCrLastTime.ltime);
	pCrLastTime.cr.save(curNode, "Misc", false);
	return(curNode);
}

//*********************************************************************
//						saveLastTime
//*********************************************************************

xmlNodePtr saveLastTime(xmlNodePtr parentNode, int i, struct lasttime pLastTime) {
	// Avoid writing un-used last times
	if(!pLastTime.interval && !pLastTime.ltime && !pLastTime.misc)
		return(NULL);

	xmlNodePtr curNode = xml::newStringChild(parentNode, "LastTime");
	xml::newNumProp(curNode, "Num", i);

	xml::newNumChild(curNode, "Interval", pLastTime.interval);
	xml::newNumChild(curNode, "LastTime", pLastTime.ltime);
	xml::newNumChild(curNode, "Misc", pLastTime.misc);
	return(curNode);
}

//*********************************************************************
//						saveSavingThrow
//*********************************************************************

xmlNodePtr saveSavingThrow(xmlNodePtr parentNode, int i, struct saves pSavingThrow) {
	xmlNodePtr curNode = xml::newStringChild(parentNode, "SavingThrow");
	xml::newNumProp(curNode, "Num", i);

	xml::newNumChild(curNode, "Chance", pSavingThrow.chance);
	xml::newNumChild(curNode, "Gained", pSavingThrow.gained);
	xml::newNumChild(curNode, "Misc", pSavingThrow.misc);
	return(curNode);
}

//*********************************************************************
//						saveObjRefFlags
//*********************************************************************

xmlNodePtr saveObjRefFlags(xmlNodePtr parentNode, const char* name, int maxBit, const char *bits) {
	xmlNodePtr curNode=NULL;
	// this nested loop means we won't create an xml node if we don't have to
	for(int i=0; objRefSaveFlags[i] != -1; i++) {
		if(BIT_ISSET(bits, objRefSaveFlags[i])) {
			curNode = xml::newStringChild(parentNode, name);
			for(; objRefSaveFlags[i] != -1; i++) {
				if(BIT_ISSET(bits, objRefSaveFlags[i]))
					saveBit(curNode, objRefSaveFlags[i]);
			}
			return(curNode);
		}
	}
	return(curNode);
}

//*********************************************************************
//						saveBits
//*********************************************************************

xmlNodePtr saveBits(xmlNodePtr parentNode, const char* name, int maxBit, const char *bits) {
	xmlNodePtr curNode=NULL;
	// this nested loop means we won't create an xml node if we don't have to
	for(int i=0; i<maxBit; i++) {
		if(BIT_ISSET(bits, i)) {
			curNode = xml::newStringChild(parentNode, name);
			for(; i<maxBit; i++) {
				if(BIT_ISSET(bits, i))
					saveBit(curNode, i);
			}
			return(curNode);
		}
	}
	return(curNode);
}

//*********************************************************************
//						saveBit
//*********************************************************************

xmlNodePtr saveBit(xmlNodePtr parentNode, int bit) {
	xmlNodePtr curNode;

	curNode = xml::newStringChild(parentNode, "Bit");
	xml::newNumProp(curNode, "Num", bit);
	return(curNode);
}

//*********************************************************************
//						saveLongArray
//*********************************************************************

xmlNodePtr saveLongArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const long array[], int arraySize) {
	xmlNodePtr childNode, curNode=NULL;
	// this nested loop means we won't create an xml node if we don't have to
	for(int i=0; i<arraySize; i++) {
		if(!array[i]) {
			curNode = xml::newStringChild(parentNode, rootName);
			for(; i<arraySize; i++) {
				if(array[i]) {
					childNode = xml::newNumChild(curNode, childName, array[i]);
					xml::newNumProp(childNode, "Num", i);
				}
			}
			return(curNode);
		}
	}
	return(curNode);
}

//*********************************************************************
//						saveULongArray
//*********************************************************************

xmlNodePtr saveULongArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const unsigned long array[], int arraySize) {
	xmlNodePtr childNode, curNode=NULL;
	// this nested loop means we won't create an xml node if we don't have to
	for(int i=0; i<arraySize; i++) {
		if(array[i]) {
			curNode = xml::newStringChild(parentNode, rootName);
			for(; i<arraySize; i++) {
				if(array[i]) {
					childNode = xml::newNumChild(curNode, childName, array[i]);
					xml::newNumProp(childNode, "Num", i);
				}
			}
			return(curNode);
		}
	}
	return(curNode);
}

//*********************************************************************
//						saveCatRefArray
//*********************************************************************

xmlNodePtr saveCatRefArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const std::map<int, CatRef>& array, int arraySize) {
	xmlNodePtr curNode=NULL;
	std::map<int, CatRef>::const_iterator it;

	// this nested loop means we won't create an xml node if we don't have to
	for(it = array.begin(); it != array.end() ; it++) {
		if((*it).second.id) {
			curNode = xml::newStringChild(parentNode, rootName);
			for(; it != array.end() ; it++) {
				if((*it).second.id)
					(*it).second.save(curNode, childName, false, (*it).first);
			}
			return(curNode);
		}
	}
	return(curNode);
}

//*********************************************************************
//						saveCatRefArray
//*********************************************************************

xmlNodePtr saveCatRefArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const CatRef array[], int arraySize) {
	xmlNodePtr curNode=NULL;
	// this nested loop means we won't create an xml node if we don't have to
	for(int i=0; i<arraySize; i++) {
		if(array[i].id) {
			curNode = xml::newStringChild(parentNode, rootName);
			for(; i<arraySize; i++) {
				if(array[i].id)
					array[i].save(curNode, childName, false, i);
			}
			return(curNode);
		}
	}
	return(curNode);
}

//*********************************************************************
//						saveCarryArray
//*********************************************************************

xmlNodePtr saveCarryArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const Carry array[], int arraySize) {
	xmlNodePtr curNode=NULL;
	// this nested loop means we won't create an xml node if we don't have to
	for(int i=0; i<arraySize; i++) {
		if(array[i].info.id) {
			curNode = xml::newStringChild(parentNode, rootName);
			for(; i<arraySize; i++) {
				if(array[i].info.id)
					array[i].save(curNode, childName, false, i);
			}
			return(curNode);
		}
	}
	return(curNode);
}
//*********************************************************************
//						saveShortIntArray
//*********************************************************************

xmlNodePtr saveShortIntArray(xmlNodePtr parentNode, const char* rootName, const char* childName, const short array[], int arraySize) {
	xmlNodePtr childNode, curNode=NULL;
	// this nested loop means we won't create an xml node if we don't have to
	for(int i=0; i<arraySize; i++) {
		if(array[i]) {
			curNode = xml::newStringChild(parentNode, rootName);
			for(; i<arraySize; i++) {
				if(array[i]) {
					childNode = xml::newNumChild(curNode, childName, array[i]);
					xml::newNumProp(childNode, "Num", i);
				}
			}
			return(curNode);
		}
	}
	return(curNode);
}

#undef BIT_ISSET
#undef BIT_SET
#undef BIT_CLEAR
