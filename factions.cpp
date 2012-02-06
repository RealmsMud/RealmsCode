/*
 * factions.cpp
 *	 Faction code.
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
#include "factions.h"
#include "guilds.h"
#include "clans.h"
#include "commands.h"
#include <iomanip>

//*********************************************************************
//						FactionRegard
//*********************************************************************

FactionRegard::FactionRegard() {
	memset(classRegard, 0, sizeof(classRegard));
	memset(raceRegard, 0, sizeof(raceRegard));
	memset(deityRegard, 0, sizeof(deityRegard));
	vampirismRegard = 0;
	lycanthropyRegard = 0;
	overallRegard = 0;
}

void FactionRegard::load(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	bstring temp = "";
	int id=0;

	while(curNode) {
		if(NODE_NAME(curNode, "Class")) {
			xml::copyPropToBString(temp, curNode, "Name");
			xml::copyToNum(classRegard[gConfig->classtoNum(temp)], curNode);
		}
		else if(NODE_NAME(curNode, "Race")) {
			xml::copyPropToBString(temp, curNode, "Name");
			xml::copyToNum(raceRegard[gConfig->racetoNum(temp)], curNode);
		}
		else if(NODE_NAME(curNode, "Deity")) {
			xml::copyPropToBString(temp, curNode, "Name");
			xml::copyToNum(deityRegard[gConfig->deitytoNum(temp)], curNode);
		}
		else if(NODE_NAME(curNode, "Vampirism")) xml::copyToNum(vampirismRegard, curNode);
		else if(NODE_NAME(curNode, "Lycanthropy")) xml::copyToNum(lycanthropyRegard, curNode);
		else if(NODE_NAME(curNode, "Guild")) {
			id = xml::getIntProp(curNode, "Id");
			guildRegard[id] = xml::toNum<long>(curNode);
		}
		else if(NODE_NAME(curNode, "Clan")) {
			id = xml::getIntProp(curNode, "Id");
			clanRegard[id] = xml::toNum<long>(curNode);
		}
		else if(NODE_NAME(curNode, "Overall")) xml::copyToNum(overallRegard, curNode);

		curNode = curNode->next;
	}
}

long FactionRegard::getClassRegard(int i) const {
	return(classRegard[i]);
}
long FactionRegard::getRaceRegard(int i) const {
	return(raceRegard[i]);
}
long FactionRegard::getDeityRegard(int i) const {
	return(deityRegard[i]);
}

long FactionRegard::getVampirismRegard() const {
	return(vampirismRegard);
}
long FactionRegard::getLycanthropyRegard() const {
	return(lycanthropyRegard);
}
long FactionRegard::getGuildRegard(int i) const {
	if(!i)
		return(0);
	std::map<int,long>::const_iterator it = guildRegard.find(i);
	if(it != guildRegard.end())
		return((*it).second);
	return(0);
}
long FactionRegard::getClanRegard(int i) const {
	if(!i)
		return(0);
	std::map<int,long>::const_iterator it = clanRegard.find(i);
	if(it != clanRegard.end())
		return((*it).second);
	return(0);
}

long FactionRegard::getOverallRegard() const {
	return(overallRegard);
}
//*********************************************************************
//						guildDisplay
//*********************************************************************

bstring FactionRegard::guildDisplay() const {
	std::map<int,long>::const_iterator it;
	std::ostringstream oStr;
	const Guild* guild=0;
	for(it = guildRegard.begin() ; it != guildRegard.end() ; it++) {
		guild = gConfig->getGuild((*it).first);

		oStr << "Guild(" << (*it).first << "-" << (guild ? guild->getName() : "invalid guild") << "):"
			<< Faction::getColor((*it).second)
			<< (*it).second << "^X ";
	}
	return(oStr.str());
}

//*********************************************************************
//						clanDisplay
//*********************************************************************

bstring FactionRegard::clanDisplay() const {
	std::map<int,long>::const_iterator it;
	std::ostringstream oStr;
	const Clan* clan=0;
	for(it = clanRegard.begin() ; it != clanRegard.end() ; it++) {
		clan = gConfig->getClan((*it).first);

		oStr << "Clan(" << (*it).first << "-" << (clan && clan->getId() == (unsigned int)(*it).first ? clan->getName() : "invalid clan") << "):"
			<< Faction::getColor((*it).second)
			<< (*it).second << "^X ";
	}
	return(oStr.str());
}

//*********************************************************************
//						Faction
//*********************************************************************

Faction::Faction() {
	name = displayName = social = parent = group = "";
	baseRegard = 0;
	is_parent = false;
}


void Faction::load(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;

	while(curNode) {
			 if(NODE_NAME(curNode, "Name")) { xml::copyToBString(name, curNode); }
		else if(NODE_NAME(curNode, "DisplayName")) { xml::copyToBString(displayName, curNode); }
		else if(NODE_NAME(curNode, "Parent")) { xml::copyToBString(parent, curNode); }
		else if(NODE_NAME(curNode, "Group")) { xml::copyToBString(group, curNode); }
		else if(NODE_NAME(curNode, "Social")) { xml::copyToBString(social, curNode); }
		else if(NODE_NAME(curNode, "BaseRegard")) { xml::copyToNum( baseRegard, curNode); }
		else if(NODE_NAME(curNode, "InitialRegard")) initial.load(curNode);
		else if(NODE_NAME(curNode, "MaxRegard")) max.load(curNode);
		else if(NODE_NAME(curNode, "MinRegard")) min.load(curNode);
		curNode = curNode->next;
	}
}

bstring Faction::getName() const {
	return(name);
}
bstring Faction::getDisplayName() const {
	return(displayName);
}
bstring Faction::getParent() const {
	return(parent);
}
bstring Faction::getGroup() const {
	return(group != "" ? group : "Other");
}
bstring Faction::getSocial() const {
	return(social);
}
long Faction::getBaseRegard() const {
	return(baseRegard);
}
long Faction::getClassRegard(int i) const {
	return(initial.getClassRegard(i));
}
long Faction::getRaceRegard(int i) const {
	return(initial.getRaceRegard(i));
}
long Faction::getDeityRegard(int i) const {
	return(initial.getDeityRegard(i));
}
long Faction::getVampirismRegard() const {
	return(initial.getVampirismRegard());
}
long Faction::getLycanthropyRegard() const {
	return(initial.getLycanthropyRegard());
}
long Faction::getGuildRegard(int i) const {
	return(initial.getGuildRegard(i));
}
long Faction::getClanRegard(int i) const {
	return(initial.getClanRegard(i));
}
const FactionRegard* Faction::getInitial() {
	return(&initial);
}
const FactionRegard* Faction::getMax() {
	return(&max);
}
const FactionRegard* Faction::getMin() {
	return(&min);
}
//*********************************************************************
//						getInitialRegard
//*********************************************************************

long Faction::getInitialRegard(const Player* player) const {
	long regard = baseRegard;
	regard += initial.getClassRegard(player->getClass());
	// illusioned race affects faction
	regard += initial.getRaceRegard(player->getDisplayRace());
	if(player->getDeity())
		regard += initial.getDeityRegard(player->getDeity());
	if(player->isNewVampire())
		regard += initial.getVampirismRegard();
	if(player->isNewWerewolf())
		regard += initial.getLycanthropyRegard();
	if(player->getGuild())
		regard += initial.getGuildRegard(player->getGuild());
	if(player->getClan())
		regard += initial.getClanRegard(player->getClan());
	return(regard);
}

//*********************************************************************
//						alwaysHates
//*********************************************************************

bool Faction::alwaysHates(const Player* player) const {
	if(initial.getClassRegard(player->getClass()) == ALWAYS_HATE)
		return(true);
	if(initial.getRaceRegard(player->getRace()) == ALWAYS_HATE)
		return(true);
	if(player->getDeity() && initial.getDeityRegard(player->getDeity()) == ALWAYS_HATE)
		return(true);
	if(player->isNewVampire() && initial.getVampirismRegard() == ALWAYS_HATE)
		return(true);
	if(player->isNewWerewolf() && initial.getLycanthropyRegard() == ALWAYS_HATE)
		return(true);
	if(player->getGuild() && initial.getGuildRegard(player->getGuild()) == ALWAYS_HATE)
		return(true);
	if(player->getClan() && initial.getClanRegard(player->getClan()) == ALWAYS_HATE)
		return(true);
	return(false);
}

//*********************************************************************
//						getUpperLimit
//*********************************************************************

long Faction::getUpperLimit(const Player* player) const {
	long 	limit = MAX, x;
	x = max.getClassRegard(player->getClass());
	if(x) limit = MIN(x, limit);

	x = max.getRaceRegard(player->getRace());
	if(x) limit = MIN(x, limit);

	if(player->getDeity()) {
		x = max.getDeityRegard(player->getDeity());
		if(x) limit = MIN(x, limit);
	}
	if(player->isNewVampire()) {
		x = max.getVampirismRegard();
		if(x) limit = MIN(x, limit);
	}
	if(player->isNewWerewolf()) {
		x = max.getLycanthropyRegard();
		if(x) limit = MIN(x, limit);
	}
	if(player->getGuild()) {
		x = max.getGuildRegard(player->getGuild());
		if(x) limit = MIN(x, limit);
	}
	if(player->getClan()) {
		x = max.getClanRegard(player->getClan());
		if(x) limit = MIN(x, limit);
	}

	x = max.getOverallRegard();
	if(x) limit = MIN(x, limit);

	return(limit - getInitialRegard(player));
}

//*********************************************************************
//						getLowerLimit
//*********************************************************************

long Faction::getLowerLimit(const Player* player) const {
	long 	limit = MIN, x;
	x = min.getClassRegard(player->getClass());
	if(x) limit = MAX(x, limit);

	x = min.getRaceRegard(player->getRace());
	if(x) limit = MAX(x, limit);

	if(player->getDeity()) {
		x = min.getDeityRegard(player->getDeity());
		if(x) limit = MAX(x, limit);
	}
	if(player->isNewVampire()) {
		x = min.getVampirismRegard();
		if(x) limit = MAX(x, limit);
	}
	if(player->isNewWerewolf()) {
		x = min.getLycanthropyRegard();
		if(x) limit = MAX(x, limit);
	}
	if(player->getGuild()) {
		x = min.getGuildRegard(player->getGuild());
		if(x) limit = MAX(x, limit);
	}
	if(player->getClan()) {
		x = min.getClanRegard(player->getClan());
		if(x) limit = MAX(x, limit);
	}

	x = min.getOverallRegard();
	if(x) limit = MAX(x, limit);

	return(limit - getInitialRegard(player));
}

//*********************************************************************
//						getFaction
//*********************************************************************

const Faction *Config::getFaction(bstring factionStr) const {
	if(factionStr == "")
		return(0);
	std::map<bstring, Faction*>::const_iterator it = factions.find(factionStr);

	if(it != factions.end())
		return((*it).second);
	return(0);
}

//*********************************************************************
//						findAggro
//*********************************************************************

Player* Faction::findAggro(BaseRoom *room) {
	return(0);
}

//*********************************************************************
//						isParent
//*********************************************************************

bool Faction::isParent() const {
	return(is_parent);
}

//*********************************************************************
//						setIsParent
//*********************************************************************

void Faction::setParent(bstring value) {
	parent = value;
}

//*********************************************************************
//						setIsParent
//*********************************************************************

void Faction::setIsParent(bool value) {
	is_parent = value;
}

//*********************************************************************
//						showRegard
//*********************************************************************

void showRegard(const Player* viewer, bstring type, const FactionRegard* regard) {
	int a;
	std::ostringstream oStr;

	for(a=0; a < CLASS_COUNT; a++) {
		if(regard->getClassRegard(a)) {
			oStr << getClassAbbrev(a) << ":"
				<< Faction::getColor(regard->getClassRegard(a))
				<< regard->getClassRegard(a) << "^X ";
		}
	}
	for(a=0; a < RACE_COUNT; a++) {
		if(regard->getRaceRegard(a)) {
			oStr << gConfig->getRace(a)->getAbbr() << ":"
				<< Faction::getColor(regard->getRaceRegard(a))
				<< regard->getRaceRegard(a) << "^X ";
		}
	}
	for(a=0; a < DEITY_COUNT; a++) {
		if(regard->getDeityRegard(a)) {
			oStr << gConfig->getDeity(a)->getName() << ":"
				<< Faction::getColor(regard->getDeityRegard(a))
				<< regard->getDeityRegard(a) << "^X ";
		}
	}
	if(regard->getVampirismRegard()) {
		oStr << "Vampirism:"
			<< Faction::getColor(regard->getVampirismRegard())
			<< regard->getVampirismRegard() << "^X ";
	}
	oStr << regard->guildDisplay();
	oStr << regard->clanDisplay();
	if(regard->getOverallRegard())
		oStr << "Overall: " << regard->getOverallRegard();
	
	if(oStr.str() != "")
		viewer->printColor("    ^y%s:^x %s\n", type.c_str(), oStr.str().c_str());
}

//*********************************************************************
//						listFactions
//*********************************************************************

int listFactions(const Player* viewer, bool all) {
	if(!all)
		viewer->printColor("Type ^y*faction all^x to view all details.\n");
	viewer->printColor("^xFactions\n%-20s - %40s\n^b---------------------------------------------------------------\n", "Name", "DisplayName");

	std::map<bstring, Faction*>::iterator fIt;
	Faction* faction=0;

	for(fIt = gConfig->factions.begin() ; fIt != gConfig->factions.end() ; fIt++) {
		bstring tmp;
		faction = (*fIt).second;

		viewer->print("%-20s - %40s\n", faction->getName().c_str(), faction->getDisplayName().c_str());

		if(!all)
			continue;

		viewer->printColor(" Base Regard: %s%6d\n", (faction->getBaseRegard() >= 0 ? "^g" : "^R"), faction->getBaseRegard());

		showRegard(viewer, "Initial Regard", faction->getInitial());
		showRegard(viewer, "Max Regard", faction->getMax());
		showRegard(viewer, "Min Regard", faction->getMin());

		viewer->print("\n");
	}
	return(0);
}

//*********************************************************************
//						listFactions
//*********************************************************************

int listFactions(const Player* viewer, const Creature* target) {
	std::map<bstring, bstring> str;
	std::map<bstring, bstring>::const_iterator it;
	std::map<bstring, long>::const_iterator fIt;
	const Faction* faction=0;
	const Player* player = target->getConstPlayer();
	long	regard=0;
	long	i=0;

	if(viewer == target)
		viewer->print("Faction Standing");
	else {
		viewer->print("%s's Faction", target->name);
		const Monster* monster = target->getConstMonster();
		if(monster)
			viewer->printColor(", primeFaction: ^y%s", monster->getPrimeFaction().c_str());
	}
	viewer->printColor("\n^b----------------------------------------------------------------------\n");

	for(fIt = target->factions.begin() ; fIt != target->factions.end() ; fIt++) {
		faction = gConfig->getFaction((*fIt).first);
		regard = (*fIt).second;

		if(!faction)
			continue;
		// if showing a mob, show straight faction
		// if showing a player, show adjusted faction
		if(player) {
			regard += faction->getInitialRegard(player);
			if(faction->getParent() != "")
				regard += player->getFactionStanding(faction->getParent());
		}

		// Set left aligned
		std::ostringstream oStr;
		oStr.setf(std::ios::left, std::ios::adjustfield);
		oStr.imbue(std::locale(""));

		oStr << "   " << std::setw(35) << faction->getDisplayName() << Faction::getColor(regard)
			 << std::setw(13) << Faction::getNoun(regard) << "^x";

		// this doesnt make sense for mobs
		if(player)
			oStr << Faction::getBar(regard, true);

		if(viewer != target || target->isStaff()) {
			oStr << " ";
			if(player && i)
				oStr << "(Adj:" << regard << " Unadj:" << (regard - i) << ")";
			else
				oStr << "(" << regard << ")";
		}
		oStr << "\n";
		str[faction->getGroup()] += oStr.str();
	}

	for(it = str.begin() ; it != str.end() ; it++) {
		if((*it).first == "Other")
			continue;
		viewer->printColor("%s\n%s", (*it).first.c_str(), (*it).second.c_str());
	}
	if(str["Other"] != "")
		viewer->printColor("%s\n%s\n", "Other", str["Other"].c_str());
	return(0);
}

//*********************************************************************
//						dmShowFactions
//*********************************************************************

int dmShowFactions(Player *player, cmd* cmnd) {
	bool all = !strcmp(cmnd->str[1], "all");
	if(cmnd->num < 2 || all)
		listFactions(player, all);
	else {
		if(player->getClass() == BUILDER) {
			if(!player->canBuildMonsters())
				return(cmdNoAuth(player));
			if(!player->checkBuilder(player->parent_rom)) {
				player->print("Error: room number not in any of your alotted ranges.\n");
				return(0);
			}
		}
		Creature* target=0;
		Monster *mTarget=0;

		target = player->getRoom()->findCreature(player, cmnd);
		if(player->isCt()) {
			cmnd->str[1][0] = up(cmnd->str[1][0]);
			if(!target)
				target = gServer->findPlayer(cmnd->str[1]);
		}
		if(target) {
			if(player->getClass() == BUILDER) {
				mTarget = target->getMonster();
				if(mTarget && !player->checkBuilder(mTarget->info)) {
					player->print("Error: monster index not in any of your alotted ranges.\n");
					return(0);
				} else if(!mTarget) {
					player->print("Can't find '%s'\n", cmnd->str[1]);
					return(0);
				}
			}
			listFactions(player, target);
		} else
			player->print("Can't find '%s'\n", cmnd->str[1]);
	}
	return(0);
}

//*********************************************************************
//						cmdFactions
//*********************************************************************

int cmdFactions(Player* player, cmd* cmnd) {
	if(cmnd->num >= 2 && player->isStaff())
		return(dmShowFactions(player, cmnd));
	return(listFactions(player, player));
}

//*********************************************************************
//						getCutoff
//*********************************************************************

int Faction::getCutoff(int attitude) {
	switch(attitude) {
	case WORSHIP:
		return(WORSHIP_CUTOFF);
	case REGARD:
		return(REGARD_CUTOFF);
	case ADMIRATION:
		return(ADMIRATION_CUTOFF);
	case FAVORABLE:
		return(FAVORABLE_CUTOFF);
	case INDIFFERENT:
		return(INDIFFFERENT_CUTOFF);
	case DISAPPROVE:
		return(DISAPPROVE_CUTOFF);
	case DISFAVOR:
		return(DISFAVOR_CUTTOFF);
	case CONTEMPT:
		return(CONTEMPT_CUTOFF);
	default:
		return(0);
	}
}

//*********************************************************************
//						getAttitude
//*********************************************************************

int Faction::getAttitude(int regard) {
	if(regard >= WORSHIP_CUTOFF)
		return(WORSHIP);
	else if(regard >= REGARD_CUTOFF)
		return(REGARD);
	else if(regard >= ADMIRATION_CUTOFF)
		return(ADMIRATION);
	else if(regard >= FAVORABLE_CUTOFF)
		return(FAVORABLE);
	else if(regard >= INDIFFFERENT_CUTOFF)
		return(INDIFFERENT);
	else if(regard >= DISAPPROVE_CUTOFF)
		return(DISAPPROVE);
	else if(regard >= DISFAVOR_CUTTOFF)
		return(DISFAVOR);
	else if(regard >= CONTEMPT_CUTOFF)
		return(CONTEMPT);
	else
		return(MALICE);
}

//*********************************************************************
//						getBar
//*********************************************************************

bstring Faction::getBar(int regard, bool alwaysPad) {
	int attitude = getAttitude(regard);
	if(attitude == WORSHIP || attitude == MALICE) {
		if(alwaysPad)
			return("            ");
		return("");
	}

	int low = getCutoff(attitude);
	int num = (regard - low) * 10 / (getCutoff(attitude+1) - low);

	std::ostringstream oStr;
	oStr << "[^m";
	for(low = 0; low<num; low++)
		oStr << "x";
	for(; low<10; low++)
		oStr << " ";
	oStr << "^x]";
	return(oStr.str());
}

//*********************************************************************
//						getColor
//*********************************************************************

bstring Faction::getColor(int regard) {
	if(regard >= FAVORABLE_CUTOFF)
		return("^g");
	else if(regard <= DISAPPROVE_CUTOFF)
		return("^R");
	return("");
}

//*********************************************************************
//						getNoun
//*********************************************************************

bstring Faction::getNoun(int regard) {
	switch(getAttitude(regard)) {
	case WORSHIP:
		return("worshipped");
	case REGARD:
		return("high regard");
	case ADMIRATION:
		return("admiration");
	case FAVORABLE:
		return("favorable");
	case INDIFFERENT:
		return("indifferent");
	case DISAPPROVE:
		return("disapproving");
	case DISFAVOR:
		return("unfavorable");
	case CONTEMPT:
		return("contempt");
	case MALICE:
		return("malice");
	default:
		return("confusion");
	}
}

//*********************************************************************
//						getFactionMessage
//*********************************************************************
// Returns the faction standing of the current creature with regards to 'faction'

bstring Player::getFactionMessage(bstring factionStr) const {
	int regard = getFactionStanding(factionStr);
	std::ostringstream oStr;

	switch(Faction::getAttitude(regard)) {
	case Faction::WORSHIP:
		oStr << "worships you";
		break;
	case Faction::REGARD:
		oStr << "holds you in the highest regard";
		break;
	case Faction::ADMIRATION:
		oStr << "looks upon with you admiration";
		break;
	case Faction::FAVORABLE:
		oStr << "looks upon you favorably";
		break;
	case Faction::INDIFFERENT:
		oStr << "regards you indifferently";
		break;
	case Faction::DISAPPROVE:
		oStr << "disapproves of your presence";
		break;
	case Faction::DISFAVOR:
		oStr << "looks upon you unfavorably";
		break;
	case Faction::CONTEMPT:
		oStr << "looks upon you with contempt";
		break;
	case Faction::MALICE:
		oStr << "regards you with malice";
		break;
	default:
		oStr << "stares at you with confusion";
		break;
	}
	if(isDm())
		oStr << " (" << factionStr << ":" << Faction::getColor(regard) << regard << "^x)";

	return(oStr.str());
}

//*********************************************************************
//						getFactionStanding
//*********************************************************************
// Gets the faction standing of Creature with regards to 'faction'

int Player::getFactionStanding(bstring factionStr) const {
	int regard = 0;
	std::map<bstring, long>::const_iterator it = factions.find(factionStr);

	if(it != factions.end())
		regard += (*it).second;

	const Faction* faction = gConfig->getFaction(factionStr);
	if(faction) {
		regard += faction->getInitialRegard(this);
		if(faction->getParent() != "")
			regard += getFactionStanding(faction->getParent());
	}

	return(regard);
}

//*********************************************************************
//						adjustFactionStanding
//*********************************************************************
// Adjust the killer's faction standing for killing the victim

void Player::adjustFactionStanding(const std::map<bstring, long>& factionList) {
	std::map<bstring, long>::const_iterator fIt;
	std::ostringstream oStr;
	const Faction *faction=0;
	long	regard=0, current=0, limit=0;

	for(fIt = factionList.begin() ; fIt != factionList.end() ; fIt++) {
		faction = gConfig->getFaction((*fIt).first);
		regard = (*fIt).second;

		if(!regard || !faction)
			continue;

		current = factions[faction->getName()];

		// this is where adjusted faction comes into play;
		// prevent them from going out-of-bounds
		limit = faction->getUpperLimit(this);
		if(current - regard > limit)
			regard = current - limit;
		limit = faction->getLowerLimit(this);
		if(current - regard < limit)
			regard = current - limit;

		// it was adjusted to 0, meaning we can't move any more
		if(!regard)
			continue;
		if(faction->alwaysHates(this) && regard <= 0)
			continue;

		oStr << (regard > 0 ? "^R" : "^c")
			<< "Your faction standing with " << faction->getDisplayName()
			<< (faction->isParent() ? " and its subfactions" : "")
			<< (regard > 0 ? " has gotten worse." : " has improved.");

		if(isDm())
			oStr << " (" << abs(regard) << ")";

		oStr << "\n";
		factions[faction->getName()] -= regard;
	}

	printColor("%s", oStr.str().c_str());
}

//*********************************************************************
//						worshipSocial
//*********************************************************************

void Faction::worshipSocial(Monster *monster) {
	if(monster->getPrimeFaction() == "")
		return;
	// only certain monster types perform this action
	if(!monType::isIntelligent(monster->getType()) && !monster->getRace())
		return;
	if(monster->inCombat())
		return;
	const Faction *faction = gConfig->getFaction(monster->getPrimeFaction());
	if(!faction)
		return;
	bstring social = faction->getSocial();
	if(social == "")
		return;

	// we've made sure everything is ok; let's continue
	// with the actual work
	Player* player;
	cmd		cmnd;

	strcpy(cmnd.str[0], social.c_str());
	cmnd.num = 2;
	cmnd.val[1] = 1;
	for(Player* player : monster->getRoom()->players) {
		if(	mrand(1,100)>2 ||
			!player ||
			player->flagIsSet(P_UNCONSCIOUS) ||
			!monster->canSee(player) ||
			monster->isEnemy(player) ||
			player->isEffected("petrification") ||
			player->inCombat() ||
			player->isStaff()
		)
			continue;

		if(getAttitude(player->getFactionStanding(monster->getPrimeFaction())) < WORSHIP)
			continue;

		strcpy(cmnd.str[1], player->name);
		cmdProcess(monster, &cmnd);
	}
}

//*********************************************************************
//						willAggro
//*********************************************************************

bool Faction::willAggro(const Player* player, bstring faction) {
	if(faction == "")
		return(false);
	int attitude = getAttitude(player->getFactionStanding(faction));
	if(attitude <= MALICE)
		return(true);
	if(attitude == CONTEMPT && mrand(1,100) <= 10)
		return(true);
	return(false);
}

//*********************************************************************
//						willSpeakWith
//*********************************************************************

bool Faction::willSpeakWith(const Player* player, bstring faction) {
	if(faction == "")
		return(true);
	return(getAttitude(player->getFactionStanding(faction)) > CONTEMPT);
}

//*********************************************************************
//						willDoBusinessWith
//*********************************************************************

bool Faction::willDoBusinessWith(const Player* player, bstring faction) {
	if(faction == "")
		return(true);
	return(getAttitude(player->getFactionStanding(faction)) > DISFAVOR);
}

//*********************************************************************
//						willBeneCast
//*********************************************************************

bool Faction::willBeneCast(const Player* player, bstring faction) {
	if(faction == "")
		return(true);
	return(getAttitude(player->getFactionStanding(faction)) >= INDIFFERENT);
}

//*********************************************************************
//						willLetThrough
//*********************************************************************

bool Faction::willLetThrough(const Player* player, bstring faction) {
	if(faction == "")
		return(false);
	return(getAttitude(player->getFactionStanding(faction)) >= FAVORABLE);
}

//*********************************************************************
//						adjustPrice
//*********************************************************************

Money Faction::adjustPrice(const Player* player, bstring faction, Money money, bool sell) {
	if(faction == "")
		return(money);
	int attitude = getAttitude(player->getFactionStanding(faction));

	if(attitude <= DISAPPROVE) {
		if(sell)
			money.set(money[GOLD] * 2, GOLD);
		else
			money.set(money[GOLD] / 2, GOLD);
	} else if(attitude >= FAVORABLE && attitude < REGARD) {
		if(sell)
			money.sub(money[GOLD] / 20, GOLD);
		else
			money.add(money[GOLD] / 20, GOLD);
	} else if(attitude >= REGARD) {
		if(sell)
			money.sub(money[GOLD] / 10, GOLD);
		else
			money.add(money[GOLD] / 10, GOLD);
	}

	return(money);
}


//*********************************************************************
//						canPledgeTo
//*********************************************************************

bool Faction::canPledgeTo(const Player* player, bstring faction) {
	if(faction == "")
		return(true);
	return(getAttitude(player->getFactionStanding(faction)) >= INDIFFERENT);
}


// Clears factions
void Config::clearFactionList() {
	std::map<bstring, Faction*>::iterator fIt;
	Faction* faction;

	for(fIt = factions.begin() ; fIt != factions.end() ; fIt++) {
		faction = (*fIt).second;
		delete faction;
		//factions.erase(fIt);
	}
	factions.clear();
}


bool Config::loadFactions() {
	xmlDocPtr	xmlDoc;
	//GuildCreation * curCreation;
	xmlNodePtr	cur;
	char		filename[80];

	// build an XML tree from the file
	sprintf(filename, "%s/factions.xml", Path::Game);

	xmlDoc = xml::loadFile(filename, "Factions");
	if(xmlDoc == NULL)
		return(false);

	cur = xmlDocGetRootElement(xmlDoc);

	// First level we expect a faction
	cur = cur->children;
	while(cur && xmlIsBlankNode(cur))
		cur = cur->next;

	if(cur == 0) {
		xmlFreeDoc(xmlDoc);
		xmlCleanupParser();
		return(false);
	}

	clearFactionList();
	Faction* faction;
	while(cur != NULL) {
		if(NODE_NAME(cur, "Faction")) {
			faction = new Faction;
			faction->load(cur);
			factions[faction->getName()] = faction;
		}
		cur = cur->next;
	}
	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();

	std::map<bstring, Faction*>::const_iterator it, fIt;

	for(it = factions.begin(); it != factions.end(); it++) {
		faction = (*it).second;

		if(faction->getParent() != "") {
			fIt = factions.find(faction->getParent());

			if(fIt == factions.end()) {
				faction->setParent("");
			} else {
				(*fIt).second->setIsParent(true);
			}
		}
	}
	return(true);
}
