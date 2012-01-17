/*
 * playerTitle.cpp
 *	 Player titles
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
#include "login.h"

//*********************************************************************
//						PlayerTitle
//*********************************************************************

PlayerTitle::PlayerTitle() {
	male = "";
	female = "";
}

//*********************************************************************
//						load
//*********************************************************************

void PlayerTitle::load(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	while(curNode) {
			 if(NODE_NAME(curNode, "Female")) xml::copyToBString(female, curNode);
		else if(NODE_NAME(curNode, "Male")) xml::copyToBString(male, curNode);
		curNode = curNode->next;
	}
}

//*********************************************************************
//						getTitle
//*********************************************************************

bstring PlayerTitle::getTitle(bool sexMale) const {
	return(sexMale ? male : female);
}

//*********************************************************************
//						getTitle
//*********************************************************************

bstring getTitle(const std::map<int, PlayerTitle*>& titles, int lvl, bool male, bool ignoreCustom) {
	std::map<int, PlayerTitle*>::const_iterator it;
	if(lvl < 1)
		lvl = 1;
	if(lvl > MAXALVL)
		lvl = MAXALVL;
	while(	lvl > 1 && (
				(it = titles.find(lvl)) == titles.end() ||
				(	ignoreCustom &&
					(*it).second->getTitle(male) == "[custom]"
				)
			)
	)
		lvl--;
	it = titles.find(lvl);
	return((*it).second->getTitle(male));
}

bstring DeityData::getTitle(int lvl, bool male, bool ignoreCustom) const {
	return(::getTitle(titles, lvl, male, ignoreCustom));
}

bstring PlayerClass::getTitle(int lvl, bool male, bool ignoreCustom) const {
	return(::getTitle(titles, lvl, male, ignoreCustom));
}

//********************************************************************
//				titlePunctuation
//********************************************************************

bool titlePunctuation(char c) {
	switch(c) {
	case ' ':
	case '/':
	case '\'':
	case '-':
		return(true);
	default:
		return(false);
	}
}

//********************************************************************
//				cmdTitle
//********************************************************************

int cmdTitle(Player* player, cmd* cmnd) {
	double	punctuation=0;
	bstring title = getFullstrText(cmnd->fullstr, 1);

	if(player->flagIsSet(P_CANNOT_CHOOSE_CUSTOM_TITLE)) {
		player->print("You have lost the privilege of choosing a custom title.\n");
		return(0);
	}
	if(!player->flagIsSet(P_CAN_CHOOSE_CUSTOM_TITLE)) {
		player->print("You cannot currently choose a custom title.\n");
		return(0);
	}
	if(cmnd->num < 2) {
		player->print("Please enter a title.\n");
		return(0);
	}

	for(size_t i = 0; i<title.length(); i++) {
		if(!titlePunctuation(title[i]) && !isalpha(title[i])) {
			player->print("Sorry, ""%c"" cannot be used in your title.\n", title[i]);
			return(0);
		}
		if(titlePunctuation(title[i]))
			punctuation++;
	}

	if(title.length() < 4) {
		player->print("Title must be atleast 4 characters.\n");
		return(0);
	}

	punctuation = punctuation / (float)title.length();
	if(punctuation > 0.25) {
		player->print("This title has too much punctuation.\n");
		return(0);
	}

	player->print("Your new title: %s\n", title.c_str());
	player->print("Is this acceptable? (Y/N)\n");

	player->setTempTitle(title);
	player->getSock()->setState(CON_CONFIRM_TITLE);
	return(0);
}

//*********************************************************************
//						doTitle
//*********************************************************************

void doTitle(Socket* sock, char *str) {
	Player* player = sock->getPlayer();

	if(low(str[0]) == 'y') {
		player->print("You are now known as %s the %s.\n", player->name, player->getTempTitle().c_str());
		if(!player->isStaff()) {
			broadcast("^y%s the %s is now known as %s the %s.", player->name,
					player->getTitle().c_str(), player->name, player->getTempTitle().c_str());

			sendMail(gConfig->getReviewer(), (bstring)player->name + " has chosen the title " + player->getTempTitle() + ".\n");
		}

		player->setTitle(player->getTempTitle());
		player->clearFlag(P_CAN_CHOOSE_CUSTOM_TITLE);
	} else
		player->print("Aborted.\n");

	player->setTempTitle("");
	sock->setState(CON_PLAYING);
}

//*********************************************************************
//						cmdSurname
//*********************************************************************

int cmdSurname(Player* player, cmd* cmnd) {
	int		nonalpha=0;
	unsigned int i=0;
	bool	illegalNonAlpha=false;

	if(!player->ableToDoCommand())
		return(0);

	if(player->flagIsSet(P_NO_SURNAME)) {
		player->print("You have lost the privilege of choosing a surname.\n");
		return(0);
	}

	if(player->flagIsSet(P_CHOSEN_SURNAME) && !player->isCt()) {
		player->print("You've already chosen your surname.\n");
		return(0);
	}

	if(player->getLevel() < SURNAME_LEVEL && !player->isStaff()) {
		player->print("You must be level %d to choose a surname.\n", SURNAME_LEVEL);
		return(0);
	}

	if(cmnd->num < 2) {
		player->print("Syntax: surname <surname>\n");
		return(0);
	}


	if(strlen(cmnd->str[1]) > 14) {
		player->print("Your surname may only be a max of 14 characters long.\n");
		return(0);
	}

	if(strlen(cmnd->str[1]) < 3) {
		player->print("Your surname must be at least 3 characters in length.\n");
		return(0);
	}

	for(i=0; i< strlen(cmnd->str[1]); i++) {
		if(!isalpha(cmnd->str[1][i])) {
			nonalpha++;
			if(cmnd->str[1][i] != '\'' && cmnd->str[1][i] != '-')
				illegalNonAlpha = true;
		}

	}

	if(illegalNonAlpha) {
		player->print("The only non-alpha characters allowed in surnames are ' and -.\n");
		return(0);
	}


	if(nonalpha && strlen(cmnd->str[1]) < 6) {
		player->print("Your surname must be at least 6 characters in order to contain a - or '.\n");
		return(0);
	}


	if(nonalpha > 1) {
		player->print("Your surname may not have more than one non-alpha character.\n");
		return(0);
	}

	for(i=0; i < strlen(cmnd->str[1]); i++) {
		if(!isalpha(cmnd->str[1][i]) && cmnd->str[1][i] != '\'' && cmnd->str[1][i] != '-') {
			player->print("Your surname must be alphabetic.\n");
			player->print("It may only contain the non-alpha characters ' and -.\n");
			return(0);
		}
	}

	if(cmnd->str[1][0] == '\'' || cmnd->str[1][0] == '-') {
		player->print("The first character of your surname must be a letter.\n");
		return(0);
	}

	if(	cmnd->str[1][strlen(cmnd->str[1])-1] == '\'' ||
		cmnd->str[1][strlen(cmnd->str[1])-1] == '-' ||
		cmnd->str[1][strlen(cmnd->str[1])-2] == '\'' ||
		cmnd->str[1][strlen(cmnd->str[1])-2] == '-'
	) {
		player->print("The last two characters of your surname must be letters.\n");
		return(0);
	}

	lowercize(cmnd->str[1], 1);

	player->setSurname(cmnd->str[1]);

	player->printColor("^WNote that profane or otherwise idiotic surnames, as well as\n");
	player->printColor("idiotic combinations of name and surname, will not be tolerated.^x\n\n");
	player->print("Your full name will be %s %s.\n", player->name, player->getSurname().c_str());
	player->print("Is this acceptable?(Y/N)?\n");

	player->getSock()->setState(CON_CONFIRM_SURNAME);
	return(0);
}

//*********************************************************************
//						doSurname
//*********************************************************************

void doSurname(Socket* sock, char *str) {
	if(low(str[0]) == 'y') {
		sock->print("You are now known as %s %s.\n", sock->getPlayer()->getName(), sock->getPlayer()->getSurname().c_str());

		if(!sock->getPlayer()->isStaff()) {
			broadcast("### %s is now known as %s %s.", sock->getPlayer()->getName(), sock->getPlayer()->getName(),
				sock->getPlayer()->getSurname().c_str());

			sendMail(gConfig->getReviewer(), (bstring)sock->getPlayer()->getName() + " has chosen the surname " + sock->getPlayer()->getSurname() + ".\n");
		}

		sock->getPlayer()->setFlag(P_CHOSEN_SURNAME);

	} else
		sock->print("Aborted.\n");

	sock->setState(CON_PLAYING);
}
