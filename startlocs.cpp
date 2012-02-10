/*
 * startlocs.cpp
 *	 Player starting location code.
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
#include "login.h"
#include "commands.h"

//*********************************************************************
//						initialBind
//*********************************************************************
// setup the player for the first time they are bound

void initialBind(Player* player, bstring str) {
	const StartLoc* location = gConfig->getStartLoc(str);
	if(!location) {
		broadcast(isCt, "Invalid start location: %s", str.c_str());
		return;
	}
	CatRef cr = location->getStartingGuide();

	if(str == "oceancrest") {
		player->setFlag(P_HARDCORE);
		player->setFlag(P_CHAOTIC);
		player->setFlag(P_CHOSEN_ALIGNMENT);
	}

	player->bind(location);
	player->room = player->bound.room;
	player->room = player->getRecallRoom().room;

	if(cr.id)
		Create::addStartingItem(player, cr.area, cr.id, false, true);
}

//*********************************************************************
//						startingChoices
//*********************************************************************
// This function will determine the player's starting location.
// choose = false	return true if we can easily pick their location
//					return false if they must choose
// choose = true	return true if they made a valid selection
//					return false if they made an invalid selection

bool startingChoices(Player* player, bstring str, char* location, bool choose) {

	// if only one start location is defined, our choices are easy!
	if(gConfig->start.size() == 1) {
		std::map<bstring, StartLoc*>::iterator s = gConfig->start.begin();
		sprintf(location, "%s", (*s).first.c_str());
		location[0] = up(location[0]);
		player->bind((*s).second);
		return(true);
	}

	std::list<bstring> options;
	int		race = player->getRace();

	// set race equal to their parent race, use player->getRace() if checking
	// for subraces
	const RaceData* r = gConfig->getRace(race);
	if(r && r->getParentRace())
		race = r->getParentRace();

	if(player->getClass() == DRUID) {

		// druidic order overrides all other starting locations
		options.push_back("druidwood");

	} else if(player->getDeity() == ENOCH || player->getRace() == SERAPH) {

		// religious states
		options.push_back("sigil");

	} else if(player->getDeity() == ARAMON || player->getRace() == CAMBION) {

		// religious states
		options.push_back("caladon");

	} else if(race == HUMAN && player->getClass() == CLERIC) {

		// all other human clerics have to start in HP because
		// Sigil and Caladon are very religious
		options.push_back("highport");

	} else if(race == GNOME || race == HALFLING) {

		options.push_back("gnomebarrow");

	} else if(race == HALFGIANT) {

		options.push_back("schnai");
		options.push_back("highport");

	} else if(race == BARBARIAN) {

		options.push_back("schnai");

	} else if(race == DWARF) {

		options.push_back("ironguard");

	} else if(race == DARKELF) {

		options.push_back("oakspire");

	} else if(race == TIEFLING) {

		options.push_back("highport");
		options.push_back("caladon");

	} else if(race == MINOTAUR) {

		options.push_back("ruhrdan");

	} else if(race == KATARAN) {

		options.push_back("kataran");

	} else if(race == HALFORC) {

		// half-breeds can start in more places
		options.push_back("highport");
		options.push_back("caladon");
		options.push_back("orc");

	} else if(race == ORC) {

		options.push_back("orc");

	} else if(race == ELF || player->getDeity() == LINOTHAN) {

		options.push_back("meadhil");

	} else if(race == HALFELF || race == HUMAN) {

		// humans and half-breeds can start in more places
		options.push_back("highport");
		options.push_back("sigil");

		if(race == HUMAN)
			options.push_back("caladon");
		else if(race == HALFELF)
			options.push_back("meadhil");


	}

	switch(player->getClass()) {
	case RANGER:
		options.push_back("druidwood");
		break;
	// even seraphs of these classes cannot start in Sigil
	case ASSASSIN:
	case LICH:
	case THIEF:
	case DEATHKNIGHT:
	case VAMPIRE:
	case ROGUE:
	case WEREWOLF:
		options.remove("sigil");
		break;
	default:
		break;
	}

	// needed:
	// troll, goblin, kobold, ogre

	// these areas arent finished yet
	options.remove("caladon");
	options.remove("meadhil");
	options.remove("orc");
	options.remove("schnai");
	options.remove("ironguard");
	options.remove("kataran");

	// if the areas aren't open, give them the default starting location,
	// which is the first one on the list
	if(!options.size())
		options.push_back(gConfig->getDefaultStartLoc()->getName());

	// OCEANCREST: Dom: HC
	//options.push_back("oceancrest");

	// if they have no choice, we assign them a location and are done with it
	if(options.size() == 1) {
		sprintf(location, "%s", options.front().c_str());
		location[0] = up(location[0]);

		initialBind(player, options.front());
		return(true);
	}


	std::list<bstring>::iterator it;
	int		i=0;


	// we don't need to make any choices - we need to show them what they can choose
	if(!choose) {
		std::ostringstream oStr;
		char opt = 'A';
		for(it = options.begin(); it != options.end(); it++) {
			if(i)
				oStr << "     ";
			i++;

			sprintf(location, "%s", (*it).c_str());
			location[0] = up(location[0]);

			oStr << "[^W" << (opt++) << "^x] " << location;
		}
		sprintf(location, "%s", oStr.str().c_str());
		return(false);
	}

	// determine where they want to start
	int choice = low(str[0]) - 'a' + 1;

	for(it = options.begin(); it != options.end(); it++) {
		if(++i == choice) {
			sprintf(location, "%s", (*it).c_str());
			location[0] = up(location[0]);

			initialBind(player, *it);
			return(true);
		}
	}

	return(false);
}


//*********************************************************************
//						dmStartLocs
//*********************************************************************

int dmStartLocs(Player* player, cmd* cmnd) {
	player->print("Starting Locations:\n");

	std::map<bstring, StartLoc*>::iterator it;
	for(it = gConfig->start.begin(); it != gConfig->start.end() ; it++) {

		player->print("%s\n", (*it).first.c_str());

	}
	return(0);
}

//*********************************************************************
//						splBind
//*********************************************************************
// This function will change the player's bound room. If cast as a spell,
// it must belong to a list of rooms. If used from the floor, it can point
// to any room

int splBind(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* creature=0;
	Player*	target=0, *pPlayer = player->getPlayer();
	const StartLoc* location=0;

	if(!pPlayer)
		return(0);

	if(spellData->how == CAST) {

		if(!pPlayer->isCt()) {
			if(	pPlayer->getClass() != MAGE &&
				pPlayer->getClass() != LICH &&
				pPlayer->getClass() != CLERIC &&
				pPlayer->getClass() != DRUID
			) {
				pPlayer->print("Your class prevents you from casting that spell.\n");
				return(0);
			}
			if(pPlayer->getLevel() < 13) {
				pPlayer->print("You are not yet powerful enough to cast that spell.\n");
				return(0);
			}
		}

		if(cmnd->num < 2) {
			pPlayer->print("Syntax: cast bind [target]%s.\n", pPlayer->isCt() ? " <location>" : "");
			return(0);
		}


		if(cmnd->num == 2)
			target = pPlayer;
		else {

			cmnd->str[2][0] = up(cmnd->str[2][0]);

			creature = pPlayer->getParent()->findCreature(pPlayer, cmnd->str[2], cmnd->val[2], false);
			if(creature)
				target = creature->getPlayer();

			if(!target) {
				pPlayer->print("You don't see that person here.\n");
				return(0);
			}
			if(checkRefusingMagic(player, target))
				return(0);
		}


		if(pPlayer->isCt() && cmnd->num == 4)
			location = gConfig->getStartLoc(cmnd->str[3]);
		else if(pPlayer->parent_rom && pPlayer->parent_rom->info.id)
			location = gConfig->getStartLocByReq(pPlayer->parent_rom->info);

		if(	!location ||
			!location->getRequired().getId() ||
			!location->getBind().getId()
		) {
			pPlayer->print("%s is not a valid location to bind someone!\n",
				pPlayer->isCt() && cmnd->num == 4 ? "That" : "This");
			return(0);
		}

		if(!pPlayer->isCt()) {
			if(	(pPlayer->area_room && pPlayer->area_room->mapmarker != location->getRequired().mapmarker) ||
				(pPlayer->parent_rom && pPlayer->parent_rom->info != location->getRequired().room)
			) {
				pPlayer->print("To bind to this location, you must be at %s.\n", location->getRequiredName().c_str());
				return(0);
			}
			if(!dec_daily(&pPlayer->daily[DL_TELEP])) {
				pPlayer->print("You are too weak to bind again today.\n");
				return(0);
			}
		}

		if(pPlayer == target) {

			pPlayer->print("Bind spell cast.\nYou are now bound to %s.\n", location->getBindName().c_str());
			broadcast(pPlayer->getSock(), pPlayer->getRoom(), "%M casts a bind spell on %sself.", pPlayer, pPlayer->himHer());

		} else {

			// nosummon flag
			if(	target->flagIsSet(P_NO_SUMMON) &&
				!pPlayer->checkStaff("The spell fizzles.\n%M's summon flag is not set.\n", target)
			) {
				target->print("%s tried to bind you to this room!\nIf you wish to be bound, type \"set summon\".\n", pPlayer->name);
				return(0);
			}

			pPlayer->print("Bind cast on %s.\n%s is now bound to %s.\n",
				target->name, target->name, location->getBindName().c_str());
			target->print("%M casts a bind spell on you.\nYou are now bound to %s.\n",
				pPlayer, location->getBindName().c_str());
			broadcast(player->getSock(), target->getSock(), pPlayer->getRoom(),
				"%M casts a bind spell on %N.", pPlayer, target);

		}

		target->bind(location);

	} else {

		if(spellData->how != WAND) {
			pPlayer->print("Nothing happens.\n");
			return(0);
		} else {

			// we need more info from the item!
			pPlayer->print("Nothing happens (for now).\n");
			return(0);

		}
	}

	return(1);
}


//*********************************************************************
//						StartLoc
//*********************************************************************

StartLoc::StartLoc() {
	name = requiredName = bindName = "";
	primary = false;
}

//*********************************************************************
//						load
//*********************************************************************

void StartLoc::load(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;
	xml::copyPropToBString(name, curNode, "Name");
	while(childNode) {
			 if(NODE_NAME(childNode, "BindName")) xml::copyToBString(bindName, childNode);
		else if(NODE_NAME(childNode, "RequiredName")) xml::copyToBString(requiredName, childNode);
		else if(NODE_NAME(childNode, "Bind")) bind.load(childNode);
		else if(NODE_NAME(childNode, "Required")) required.load(childNode);
		else if(NODE_NAME(childNode, "StartingGuide")) startingGuide.load(childNode);
		childNode = childNode->next;
	}
}

//*********************************************************************
//						save
//*********************************************************************

void StartLoc::save(xmlNodePtr curNode) const {
	xmlNodePtr childNode = xml::newStringChild(curNode, "Location");
	xml::newProp(childNode, "Name", name);

	xml::saveNonNullString(childNode, "BindName", bindName);
	xml::saveNonNullString(childNode, "RequiredName", requiredName);
	bind.save(childNode, "Bind");
	required.save(childNode, "Required");
	startingGuide.save(childNode, "StartingGuide", false);
}

bstring StartLoc::getName() const { return(name); }
bstring StartLoc::getBindName() const { return(bindName); }
bstring StartLoc::getRequiredName() const { return(requiredName); }
Location StartLoc::getBind() const { return(bind); }
Location StartLoc::getRequired() const { return(required); }
CatRef StartLoc::getStartingGuide() const { return(startingGuide); }
bool StartLoc::isDefault() const { return(primary); }
void StartLoc::setDefault() { primary = true; }

//*********************************************************************
//						bind
//*********************************************************************

void Player::bind(const StartLoc* location) {
	bound = location->getBind();
}


//*********************************************************************
//						getStartLoc
//*********************************************************************

const StartLoc *Config::getStartLoc(bstring id) const {
	std::map<bstring, StartLoc*>::const_iterator it = start.find(id);

	if(it == start.end())
		return(0);

	return((*it).second);
}


//*********************************************************************
//						getDefaultStartLoc
//*********************************************************************

const StartLoc *Config::getDefaultStartLoc() const {
	std::map<bstring, StartLoc*>::const_iterator it;
	for(it = start.begin(); it != start.end() ; it++) {
		if((*it).second->isDefault())
			return((*it).second);
	}
	return(0);
}


//*********************************************************************
//						getStartLocByReq
//*********************************************************************

const StartLoc *Config::getStartLocByReq(CatRef cr) const {
	std::map<bstring, StartLoc*>::const_iterator it;
	StartLoc* s=0;

	for(it = start.begin() ; it != start.end() ; it++) {
		s = (*it).second;
		if(cr == s->getRequired().room)
			return(s);
	}
	return(0);
}


//*********************************************************************
//						clearStartLoc
//*********************************************************************

void Config::clearStartLoc() {
	std::map<bstring, StartLoc*>::iterator it;

	for(it = start.begin() ; it != start.end() ; it++) {
		StartLoc* s = (*it).second;
		delete s;
	}
	start.clear();
}

//*********************************************************************
//						loadStartLoc
//*********************************************************************

bool Config::loadStartLoc() {
	xmlDocPtr xmlDoc;
	xmlNodePtr curNode;
	// first one loaded is the primary one
	bool primary=true;

	char filename[80];
	snprintf(filename, 80, "%s/start.xml", Path::Game);
	xmlDoc = xml::loadFile(filename, "Locations");

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

	clearStartLoc();
	bstring loc = "";
	while(curNode != NULL) {
		if(NODE_NAME(curNode, "Location")) {
			xml::copyPropToBString(loc, curNode, "Name");
			if(loc != "" && start.find(loc) == start.end()) {
				start[loc] = new StartLoc;
				start[loc]->load(curNode);
				if(primary) {
					start[loc]->setDefault();
					primary = false;
				}
			}
		}
		curNode = curNode->next;
	}
	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();

	return(true);
}

//*********************************************************************
//						saveStartLocs
//*********************************************************************

void Config::saveStartLocs() const {
	std::map<bstring, StartLoc*>::const_iterator it;
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	char			filename[80];

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Locations", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	for(it = start.begin() ; it != start.end() ; it++)
		(*it).second->save(rootNode);

	sprintf(filename, "%s/start.xml", Path::Game);
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
}
