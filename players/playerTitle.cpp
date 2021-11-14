/*
 * playerTitle.cpp
 *   Player titles
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

#include <cctype>                  // for isalpha
#include <cstring>                 // for strlen, size_t
#include <map>                     // for map, operator==, map<>::const_iter...
#include <ostream>                 // for ostringstream, basic_ostream, oper...
#include <string>                  // for string, allocator, char_traits
#include <string_view>             // for string_view
#include <utility>                 // for pair

#include "cmd.hpp"                 // for cmd
#include "commands.hpp"            // for getFullstrText, cmdSurname, cmdTitle
#include "config.hpp"              // for Config, gConfig
#include "deityData.hpp"           // for DeityData
#include "flags.hpp"               // for P_CAN_CHOOSE_CUSTOM_TITLE, P_CHOSE...
#include "global.hpp"              // for CreatureClass, CreatureClass::CLERIC
#include "login.hpp"               // for CON_PLAYING, CON_CONFIRM_SURNAME
#include "mud.hpp"                 // for SURNAME_LEVEL
#include "mudObjects/players.hpp"  // for Player
#include "playerClass.hpp"         // for PlayerClass
#include "playerTitle.hpp"         // for PlayerTitle
#include "proto.hpp"               // for get_class_string, broadcast, low
#include "socket.hpp"              // for Socket
#include "structs.hpp"             // for SEX_MALE

//*********************************************************************
//                      PlayerTitle
//*********************************************************************

PlayerTitle::PlayerTitle() {
    male = "";
    female = "";
}


//*********************************************************************
//                      getTitle
//*********************************************************************

std::string PlayerTitle::getTitle(bool sexMale) const {
    return(sexMale ? male : female);
}

//*********************************************************************
//                      getTitle
//*********************************************************************

std::string getTitle(const std::map<int, PlayerTitle*>& titles, int lvl, bool male, bool ignoreCustom) {
    std::map<int, PlayerTitle*>::const_iterator it;
    if(lvl < 1)
        lvl = 1;
    if(lvl > MAXALVL)
        lvl = MAXALVL;
    while(  lvl > 1 && (
                (it = titles.find(lvl)) == titles.end() ||
                (   ignoreCustom &&
                    (*it).second->getTitle(male) == "[custom]"
                )
            )
    )
        lvl--;
    it = titles.find(lvl);
    return((*it).second->getTitle(male));
}

std::string DeityData::getTitle(int lvl, bool male, bool ignoreCustom) const {
    return(::getTitle(titles, lvl, male, ignoreCustom));
}

std::string PlayerClass::getTitle(int lvl, bool male, bool ignoreCustom) const {
    return(::getTitle(titles, lvl, male, ignoreCustom));
}

//********************************************************************
//              titlePunctuation
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
//              cmdTitle
//********************************************************************

int cmdTitle(Player* player, cmd* cmnd) {
    double  punctuation=0;
    std::string title = getFullstrText(cmnd->fullstr, 1);

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
//                      doTitle
//*********************************************************************

void doTitle(Socket* sock, const std::string& str) {
    Player* player = sock->getPlayer();

    if(low(str[0]) == 'y') {
        player->print("You are now known as %s the %s.\n", player->getCName(), player->getTempTitle().c_str());
        if(!player->isStaff()) {
            broadcast("^y%s the %s is now known as %s the %s.", player->getCName(),
                    player->getTitle().c_str(), player->getCName(), player->getTempTitle().c_str());

            sendMail(gConfig->getReviewer(), player->getName() + " has chosen the title " + player->getTempTitle() + ".\n");
        }

        player->setTitle(player->getTempTitle());
        player->clearFlag(P_CAN_CHOOSE_CUSTOM_TITLE);
    } else
        player->print("Aborted.\n");

    player->setTempTitle("");
    sock->setState(CON_PLAYING);
}

//*********************************************************************
//                      cmdSurname
//*********************************************************************

int cmdSurname(Player* player, cmd* cmnd) {
    int     nonalpha=0;
    unsigned int i=0;
    bool    illegalNonAlpha=false;

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

    if( cmnd->str[1][strlen(cmnd->str[1])-1] == '\'' ||
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
    player->print("Your full name will be %s %s.\n", player->getCName(), player->getSurname().c_str());
    player->print("Is this acceptable?(Y/N)?\n");

    player->getSock()->setState(CON_CONFIRM_SURNAME);
    return(0);
}

//*********************************************************************
//                      doSurname
//*********************************************************************

void doSurname(Socket* sock, const std::string& str) {
    if(low(str[0]) == 'y') {
        sock->print("You are now known as %s %s.\n", sock->getPlayer()->getCName(), sock->getPlayer()->getSurname().c_str());

        if(!sock->getPlayer()->isStaff()) {
            broadcast("### %s is now known as %s %s.", sock->getPlayer()->getCName(), sock->getPlayer()->getCName(),
                sock->getPlayer()->getSurname().c_str());

            sendMail(gConfig->getReviewer(), (std::string)sock->getPlayer()->getName() + " has chosen the surname " + sock->getPlayer()->getSurname() + ".\n");
        }

        sock->getPlayer()->setFlag(P_CHOSEN_SURNAME);

    } else
        sock->print("Aborted.\n");

    sock->setState(CON_PLAYING);
}


//*********************************************************************
//                      getTitle
//*********************************************************************
// This function returns a string with the player's character title in
// it.  The title is determined by looking at the player's class and
// level.

std::string Player::getTitle() const {
    std::ostringstream titleStr;

    int titlnum;

    if(title.empty() || title == " " || title == "[custom]") {
        titlnum = (level - 1) / 3;
        if(titlnum > 9)
            titlnum = 9;

        if(cClass == CreatureClass::CLERIC)
            titleStr << gConfig->getDeity(deity)->getTitle(level, getSex() == SEX_MALE);
        else
            titleStr << gConfig->classes[get_class_string(static_cast<int>(cClass))]->getTitle(level, getSex() == SEX_MALE);

        if(hasSecondClass()) {
            titleStr <<  "/";

            if(cClass2 == CreatureClass::CLERIC)
                titleStr << gConfig->getDeity(deity)->getTitle(level, getSex() == SEX_MALE);
            else
                titleStr << gConfig->classes[get_class_string(static_cast<int>(cClass2))]->getTitle(level, getSex() == SEX_MALE);
        }

        return(titleStr.str());

    } else {
        return(title);
    }
}

std::string Player::getTempTitle() const { return(tempTitle); }

bool Player::canChooseCustomTitle() const {
    const std::map<int, PlayerTitle*>* titles=nullptr;
    std::map<int, PlayerTitle*>::const_iterator it;
    const PlayerTitle* cTitle=nullptr;

    if(cClass == CreatureClass::CLERIC)
        titles = &(gConfig->getDeity(deity)->titles);
    else
        titles = &(gConfig->classes[get_class_string(static_cast<int>(cClass))]->titles);

    it = titles->find(level);
    if(it == titles->end())
        return(false);

    cTitle = (*it).second;
    return(cTitle && cTitle->getTitle(getSex() == SEX_MALE) == "[custom]");
}

void Player::setTitle(std::string_view newTitle) { title = newTitle; }
void Player::setTempTitle(std::string_view newTitle) { tempTitle = newTitle; }
