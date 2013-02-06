/*
 * oldQuest.cpp
 *	 Xml parsing and various quest related functions (Including monster interaction)
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
#include "commands.h"
#include "factions.h"
#include "quests.h"
#include "tokenizer.h"

// Function prototypes
static questPtr parseQuest(xmlDocPtr doc, xmlNodePtr cur);

// Loads the quests file
bool Config::loadQuestTable() {
	xmlDocPtr       doc;
	questPtr	    curquest;
	xmlNodePtr      cur;
	char	filename[80];

	// build an XML tree from a the file
	sprintf(filename, "%s/questTable.xml", Path::Game);
	doc = xml::loadFile(filename, "Quests");
	if(doc == NULL)
		return(false);

	cur = xmlDocGetRootElement(doc);
	numQuests = 0;
	memset(questTable, 0, sizeof(questTable));

	// First level we expect a Quest
	cur = cur->children;
	while(cur && xmlIsBlankNode(cur))
		cur = cur->next;

	if(cur == 0) {
		xmlFreeDoc(doc);
		return(false);
	}

	clearQuestTable();
	// Go through the nodes and grab off all Quests
	while(cur != NULL) {
		if((!strcmp((char *)cur->name, "Quest"))) {
			// Parse the quest
			curquest = parseQuest(doc, cur);
			// If we parsed a valid quest, add it to the table
			if(curquest != NULL)
				questTable[numQuests++] = curquest;
			// No more than 128 quests currently allowed
			if(numQuests >= 128)
				break;
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();
	return(true);
}

static questPtr parseQuest(xmlDocPtr doc, xmlNodePtr cur) {
	questPtr ret = NULL;
	ret = new quest;
	if(ret == NULL) {
		loge("Quest_Load: out of memory\n");
		return(NULL);
	}

	cur = cur->children;
	// Note: (char *)xmlNodeListGetString returns a string which MUST be freed
	//       thus we can't use atoi...use toInt or toLong

	while(cur != NULL) {
		if((!strcmp((char *)cur->name, "ID")))
			ret->num = xml::toNum<int>((char *)xmlNodeListGetString(doc, cur->children, 1));
		if((!strcmp((char *)cur->name, "name")))
			ret->name = (char *)xmlNodeListGetString(doc, cur->children, 1);
		if((!strcmp((char *)cur->name, "exp")))
			ret->exp = xml::toNum<int>((char *)xmlNodeListGetString(doc, cur->children, 1));
		cur = cur->next;
	}

	return (ret);
}


// Free the memory being used by the quest table
void Config::clearQuestTable() {
	int i;
	for(i=0;i<numQuests;i++)
		freeQuest(questTable[i]);
}

// Free the memory being used by a quest
void freeQuest(questPtr toFree) {
	if(toFree->name)
		free(toFree->name);
	delete toFree;
}

void fulfillQuest(Player* player, Object* object) {
	if(object->getQuestnum()) {
		player->print("Quest fulfilled!");
		if(object->getType() != MONEY) {
			player->printColor(" Don't drop %P.\n", object);
			player->print("You won't be able to pick it up again.");
		}
		player->print("\n");
		player->setQuest(object->getQuestnum()-1);
		player->addExperience(get_quest_exp(object->getQuestnum()));
		if(!player->halftolevel())
			player->print("%ld experience granted.\n", get_quest_exp(object->getQuestnum()));
	}
	for(Object *obj : object->objects) {
		fulfillQuest(player, obj);
	}
}




//
//
////*********************************************************************
////						cmdTalk
////*********************************************************************
//// This function allows players to speak with monsters if the monster
//// has a talk string set.
//
//int cmdTalk(Player* player, cmd* cmnd) {
//	Monster *target=0;
//	ttag	*tp=0;
//
//	ttag*	t=0;
//	bstring talk;
//
//	player->clearFlag(P_AFK);
//
//	if(!player->ableToDoCommand())
//		return(0);
//
//	if(cmnd->num < 2) {
//		player->print("Talk to whom?\n");
//		return(0);
//	}
//
//	target = player->getParent()->findMonster(player, cmnd);
//	if(!target) {
//		player->print("You don't see that here.\n");
//		return(0);
//	}
//
//	player->unhide();
//	if(player->checkForSpam())
//		return(0);
//
//	if(target->flagIsSet(M_AGGRESSIVE_AFTER_TALK))
//		target->addEnemy(player);
//
//	if(	target->current_language &&
//		!target->languageIsKnown(player->current_language) &&
//		!player->checkStaff("%M doesn't seem to understand anything in %s.\n", target, get_language_adj(player->current_language))
//	)
//		return(0);
//
//	if(	!target->canSpeak() &&
//		!player->checkStaff("%M is unable to speak right now.\n", target)
//	)
//		return(0);
//
//	if(	target->talk != "" &&
//		!Faction::willSpeakWith(player, target->getPrimeFaction()) &&
//		!player->checkStaff("%M refuses to speak with you.\n", target)
//	)
//		return(0);
//
//	if(cmnd->num == 2 || !target->flagIsSet(M_TALKS)) {
//		talk = target->talk;
//		if(talk != "")
//			broadcast(player->getSock(), player->getParent(), "%M speaks to %N in %s.",
//				player, target, get_language_adj(player->current_language));
//	} else {
//		if(!target->first_tlk && !loadCreature_tlk(target)) {
//			broadcast(NULL, player->getRoom(), "%M shrugs.", target);
//			return(PROMPT);
//		}
//
//		broadcast_rom_LangWc(NORMAL, get_lang_color(target->current_language),
//			target->current_language, fd, player->area_room, player->currentLocation.room, "%M asks %N about \"%s\".",
//			player, target, cmnd->str[2]);
//
//		tp = target->first_tlk;
//		while(tp && !t) {
//			if(!strcmp(cmnd->str[2], tp->key)) {
//				t = tp;
//				talk = tp->response;
//			}
//			tp = tp->next_tag;
//		}
//		if(!t) {
//			broadcast(NULL, player->getRoom(), "%M shrugs.", target);
//			return(0);
//		}
//	}
//
//
//	if(talk != "") {
//		target->sayTo(player, talk);
//
//		if(t)
//			talk_action(player, target, t);
//	} else
//		broadcast(NULL, player->getRoom(), "%M doesn't say anything.", target);
//
//	return(0);
//}
//
////*********************************************************************
////						talk_action
////*********************************************************************
//// The talk_action function handles a monster's actin when a
//// player asks the monster about a key word. The format for the
//// is defined in the monster's talk file. Currently a monster
//// can attack, or cast spells on the player who mentions the key
//// word or preform any of the defined social commands
//
//void talk_action(Player* player, Monster *target, ttag *tt ) {
//	Object	*object=0;
//	cmd 	cm;
//	int		i=0, n=0, splno=0, reqMp=0, fd = player->fd;
//	int		(*fn)(Creature *, cmd *, int);
//	//unsigned int pos=0;
//	//bstring text = "";
//
//	for(i=0; i<COMMANDMAX; i++) {
//		cm.str[i][0] = 0;
//		cm.str[i][24] = 0;
//		cm.val[i] = 0;
//	}
//	cm.fullstr[0] = 0;
//	cm.num = 0;
//
//	switch(tt->type) {
//	// attack
//	case 1:
//		target->addEnemy(player);
//		broadcast(player->getSock(), player->getParent(), "%M attacks %N\n", target, player);
//		oldPrint(fd, "%M attacks you.\n", target);
//		break;
//
//	// action command
//	case 2:
//		//if(tt->action) {
//			// comes in the format of "keyword ACTION -------"
//
//			strncpy(cm.fullstr, tt->action, 100);
//
//			if(tt->target && !strcmp(tt->target, "PLAYER")) {
//				strcat(cm.fullstr, " ");
//				strcat(cm.fullstr, player->getCName());
//			}
//
//			stripBadChars(cm.fullstr); // removes '.' and '/'
//			lowercize(cm.fullstr, 0);
//			parse(cm.fullstr, &cm);
//
//			cmdProcess(target, &cm);
//		//}
//		break;
//	// cast
//	case 3:
//		if(tt->action) {
//			strcpy(cm.str[0], "cast");
//			strncpy(cm.str[1], tt->action,25);
//			strcpy(cm.str[2], player->getCName());
//			cm.val[0] = cm.val[1] = cm.val[2] = 1;
//			cm.num = 3;
//			sprintf(cm.fullstr, "cast %s %s", tt->action, player->getCName());
//
//			i = 0;
//			do {
//				if(!strcmp(tt->action, get_spell_name(i))) {
//					splno = i;
//					break;
//				}
//				i++;
//			} while(get_spell_num(i) != -1);
//
//			if(splno == -1)
//				return;
//			reqMp = doMpCheck(target, splno);
//			if(!reqMp)
//				return;
//			fn = get_spell_function(splno);
//
//			if((int(*)(Creature *, cmd*, int, char*, osp_t*))fn == splOffensive) {
//				for(i=0; ospell[i].splno != get_spell_num(splno); i++)
//					if(ospell[i].splno == -1)
//						return;
//				n = ((int(*)(Creature *, cmd*, int, const char*, osp_t*))*fn)(target, &cm, CAST, get_spell_name(splno),
//				      &ospell[i]);
//			} else if(target->isEnemy(player)) {
//				player->print("%M refuses to cast any spells on you.\n", target);
//				return;
//			} else
//				n = ((int(*)(Creature *, cmd*, int))*fn)(target, &cm, CAST);
//
//			if(!n)
//				player->print("%M apologizes that %s cannot currently cast that spell on you.\n",
//					target, target->heShe());
//			// If reqMp is valid, subtract the mp here
//			else if(reqMp != -1)
//				target->subMp(reqMp);
//
//		}
//		break;
//	case 4:
//		i = atoi(tt->action);
//		if(i > 0) {
//			if(loadObject(i, &object)) {
//				if(object->flagIsSet(O_RANDOM_ENCHANT))
//					object->randomEnchant();
//
//				if(	(player->getWeight() + object->getActualWeight() > player->maxWeight()) &&
//					!player->checkStaff("You can't hold anymore.\n") )
//					break;
//
//				if(	object->questnum &&
//					player->questIsSet(object->questnum) &&
//					!player->checkStaff("You may not get that. You have already fulfilled that quest.\n") )
//					break;
//
//				fulfillQuest(player, object);
//
//				player->addObj(object);
//				player->printColor("%M gives you %P\n", target, object);
//				broadcast(player->getSock(), player->getParent(), "%M gives %N %P\n", target, player, object);
//			} else
//				player->print("%M has nothing to give you.\n", target);
//		}
//		break;
//	default:
//		break;
//	}
//}
