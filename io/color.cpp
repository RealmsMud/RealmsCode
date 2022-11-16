/*
 * color.cpp
 *   Functions to handle color.
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

#include <boost/algorithm/string/case_conv.hpp>  // for to_lower
#include <boost/algorithm/string/replace.hpp>    // for replace_all
#include <boost/iterator/iterator_traits.hpp>    // for iterator_value<>::type
#include <cstdio>                                // for sprintf, size_t
#include <cstring>                               // for memset, strcmp
#include <deque>                                 // for _Deque_iterator
#include <map>                                   // for map, map<>::mapped_type
#include <sstream>                               // for operator<<, ostrings...
#include <string>                                // for string, operator==
#include <string_view>                           // for string_view, basic_s...
#include <utility>                               // for pair

#include "cmd.hpp"                               // for cmd
#include "commands.hpp"                          // for isPtester, getFullst...
#include "config.hpp"                            // for Config, gConfig
#include "flags.hpp"                             // for P_ANSI_COLOR, P_MXP_...
#include "global.hpp"                            // for CUSTOM_COLOR_DEFAULT
#include "login.hpp"                             // for ANSI_COLOR, NO_COLOR
#include "mudObjects/creatures.hpp"              // for Creature
#include "mudObjects/monsters.hpp"               // for Monster
#include "mudObjects/players.hpp"                // for Player
#include "socket.hpp"                            // for Socket, Socket::Sock...

#define CLEAR       "\033[0m"       // Resets color
#define C_BLACK     "\033[0;30m"    // Normal colors
#define C_RED       "\033[0;31m"
#define C_GREEN     "\033[0;32m"
#define C_YELLOW    "\033[0;33m"
#define C_BLUE      "\033[0;34m"
#define C_MAGENTA   "\033[0;35m"
#define C_CYAN      "\033[0;36m"
#define C_WHITE     "\033[0;37m"
#define C_D_GREY    "\033[1;30m"    // Light Colors
#define C_B_RED     "\033[1;31m"
#define C_B_GREEN   "\033[1;32m"
#define C_B_YELLOW  "\033[1;33m"
#define C_B_BLUE    "\033[1;34m"
#define C_B_MAGENTA "\033[1;35m"
#define C_B_CYAN    "\033[1;36m"
#define C_B_WHITE   "\033[1;37m"
#define C_BOLD      "\033[1m"
#define C_BLINK     "\033[5m"

int Ansi[12] = { 30, 31, 32, 33, 34, 35, 36, 37, 1, 0, 5 };
int Mirc[9] = {  1,  4,  9,  8, 12, 13, 11, 15, 15 };

//***********************************************************************
//                      nameToColorCode
//***********************************************************************

char nameToColorCode(std::string name) {
    boost::algorithm::to_lower(name);

    if(name == "blink") return('!');
    if(name == "blue") return('b');
    if(name == "bold blue") return('B');
    if(name == "red") return('r');
    if(name == "bold red") return('R');
    if(name == "cyan") return('c');
    if(name == "bold cyan") return('C');
    if(name == "green") return('g');
    if(name == "bold green") return('G');
    if(name == "magenta") return('m');
    if(name == "bold magenta") return('M');
    if(name == "yellow") return('y');
    if(name == "bold yellow") return('Y');
    if(name == "white") return('w');
    if(name == "bold white") return('W');
    if(name == "black") return('d');
    if(name == "grey") return('D');
    if(name == "gold") return('l');
    if(name == "cerulean") return('e');
    if(name == "pink") return('p');
    if(name == "sky blue") return('s');
    if(name == "brown") return('o');
    return('x');
}

//***********************************************************************
//                      getColor
//***********************************************************************

const char* getAnsiColorCode(const unsigned char ch) {
    switch(ch) {
    case 'd':   return(C_BLACK);
    case 'b':   return(C_BLUE);
    case 'E':
    case 'c':   return(C_CYAN);
    case 'g':   return(C_GREEN);
    case 'm':   return(C_MAGENTA);
    case 'r':   return(C_RED);
    case 'w':   return(C_WHITE);
    case 'o':
    case 'y':   return(C_YELLOW);
    case 'B':   return(C_B_BLUE);
    case 'e':
    case 'C':   return(C_B_CYAN);
    case 'G':   return(C_B_GREEN);
    case 'M':   return(C_B_MAGENTA);
    case 'R':   return(C_B_RED);
    case 'W':   return(C_B_WHITE);
    case 'Y':   return(C_B_YELLOW);
    case 'D':   return(C_D_GREY);
    case '@':   return(C_BOLD);
    case '#':   return(C_BLINK);
    case '^':   return("^");
    case 'x':
    default:    return(CLEAR C_WHITE);
    }
}

//*********************************************************************
//                      getCustomColor
//**********************************************************************

std::string Player::getCustomColor(CustomColor i, bool caret) const {
    char color;
    if(customColors[(int)i] != CUSTOM_COLOR_DEFAULT) {
        color = customColors[(int)i];
        if(color == '!')
            color = '#';
        return((std::string)(caret?"^":"") + color);
    }
    return(gConfig->getCustomColor(i, caret));
}

std::string Config::getCustomColor(CustomColor i, bool caret) const {
    if(customColors[(int)i] == CUSTOM_COLOR_DEFAULT)
        return((std::string)(caret?"^":"") + "x");
    return((std::string)(caret?"^":"") + customColors[(int)i]);
}

//*********************************************************************
//                      customColorize
//**********************************************************************

std::string Monster::customColorize(const std::string& pText, bool caret) const {
    std::string text = pText;
    if(getMaster() && getMaster()->isPlayer())
        text = getMaster()->getAsConstPlayer()->customColorize(text, caret);
    return(text);
}
std::string Player::customColorize(const std::string&  pText, bool caret) const {
    std::string text = pText;
    boost::replace_all(text, "*CC:BROADCAST*", getCustomColor(CUSTOM_COLOR_BROADCAST, caret).c_str());
    boost::replace_all(text, "*CC:GOSSIP*", getCustomColor(CUSTOM_COLOR_GOSSIP, caret).c_str());
    boost::replace_all(text, "*CC:PTEST*", getCustomColor(CUSTOM_COLOR_PTEST, caret).c_str());
    boost::replace_all(text, "*CC:NEWBIE*", getCustomColor(CUSTOM_COLOR_NEWBIE, caret).c_str());
    boost::replace_all(text, "*CC:DM*", getCustomColor(CUSTOM_COLOR_DM, caret).c_str());
    boost::replace_all(text, "*CC:ADMIN*", getCustomColor(CUSTOM_COLOR_ADMIN, caret).c_str());
    boost::replace_all(text, "*CC:SEND*", getCustomColor(CUSTOM_COLOR_SEND, caret).c_str());
    boost::replace_all(text, "*CC:MESSAGE*", getCustomColor(CUSTOM_COLOR_MESSAGE, caret).c_str());
    boost::replace_all(text, "*CC:WATCHER*", getCustomColor(CUSTOM_COLOR_WATCHER, caret).c_str());
    boost::replace_all(text, "*CC:CLASS*", getCustomColor(CUSTOM_COLOR_CLASS, caret).c_str());
    boost::replace_all(text, "*CC:RACE*", getCustomColor(CUSTOM_COLOR_RACE, caret).c_str());
    boost::replace_all(text, "*CC:CLAN*", getCustomColor(CUSTOM_COLOR_CLAN, caret).c_str());
    boost::replace_all(text, "*CC:TELL*", getCustomColor(CUSTOM_COLOR_TELL, caret).c_str());
    boost::replace_all(text, "*CC:GROUP*", getCustomColor(CUSTOM_COLOR_GROUP, caret).c_str());
    boost::replace_all(text, "*CC:DAMAGE*", getCustomColor(CUSTOM_COLOR_DAMAGE, caret).c_str());
    boost::replace_all(text, "*CC:SELF*", getCustomColor(CUSTOM_COLOR_SELF, caret).c_str());
    boost::replace_all(text, "*CC:GUILD*", getCustomColor(CUSTOM_COLOR_GUILD, caret).c_str());
    boost::replace_all(text, "*CC:SPORTS*", getCustomColor(CUSTOM_COLOR_SPORTS, caret).c_str());
    return(text);
}

//*********************************************************************
//                      resetCustomColors
//**********************************************************************

void Player::resetCustomColors() {
    memset(customColors, CUSTOM_COLOR_DEFAULT, sizeof(customColors));
    customColors[CUSTOM_COLOR_SIZE-1] = 0;
}

//***********************************************************************
//                      cmdColors
//***********************************************************************
// this function demonstrates all the color options available
// Letters Remaining:
// :: a    f hijk  no q  tuv   z
// :: A    F HIJKL NOPQ STUV X Z

const char* colorSection(bool staff, const char* color, char colorChar = 0) {
    static char str[80];
    char code[10];
    code[0] = 0;
    if(colorChar == 0)
        colorChar = nameToColorCode(color);

    // staff get to see the color code
    if(staff)
        sprintf(code, "^x^^%c: ", colorChar);

    sprintf(str, "%s^%c%-10s", code, colorChar, color);
    return(str);
}

int cmdColors(const std::shared_ptr<Player>& player, cmd* cmnd) {
    bool staff = player->isStaff();

    if(!strcmp(cmnd->str[1], "plyReset")) {
        player->print("Custom colors have been plyReset to defaults.\n");
        player->resetCustomColors();
        return(0);
    } else if(cmnd->num > 2) {
        CustomColor i;
        std::string type = cmnd->str[1];
        boost::algorithm::to_lower(type);

        if(type == "broadcast")
            i = CUSTOM_COLOR_BROADCAST;
        else if((type == "ptest" || type == "p-test") && isPtester(player))
            i = CUSTOM_COLOR_PTEST;
        else if(type == "gossip")
            i = CUSTOM_COLOR_GOSSIP;
        else if(type == "newbie")
            i = CUSTOM_COLOR_NEWBIE;
        else if(type == "dm" && player->isDm())
            i = CUSTOM_COLOR_DM;
        else if(type == "admin" && player->isAdm())
            i = CUSTOM_COLOR_ADMIN;
        else if(type == "send" && player->isCt())
            i = CUSTOM_COLOR_SEND;
        else if(type == "message" && player->isStaff())
            i = CUSTOM_COLOR_MESSAGE;
        else if(type == "watcher" && player->isWatcher())
            i = CUSTOM_COLOR_WATCHER;
        else if(type == "class")
            i = CUSTOM_COLOR_CLASS;
        else if(type == "race")
            i = CUSTOM_COLOR_RACE;
        else if(type == "clan")
            i = CUSTOM_COLOR_CLAN;
        else if(type == "tell")
            i = CUSTOM_COLOR_TELL;
        else if(type == "group")
            i = CUSTOM_COLOR_GROUP;
        else if(type == "damage")
            i = CUSTOM_COLOR_DAMAGE;
        else if(type == "self")
            i = CUSTOM_COLOR_SELF;
        else if(type == "guild")
            i = CUSTOM_COLOR_GUILD;
        else if(type == "sports")
            i = CUSTOM_COLOR_SPORTS;
        else {
            player->print("Custom color type choice not understood.\n");
            return(0);
        }

        char color = nameToColorCode(getFullstrText(cmnd->fullstr, 2));
        player->setCustomColor(i, color);
        if(color == '!')
            color = '#';
        player->printColor("The ^%ccustom color^x has been set.\n", color);
        return(0);
    }

    // must be on separate lines because colorSection uses a static char
    player->print("Colors:\n");
    player->printColor("   %s", colorSection(staff, "Blue"));
    player->printColor("   %s\n", colorSection(staff, "Bold Blue"));
    player->printColor("   %s", colorSection(staff, "Red"));
    player->printColor("   %s\n", colorSection(staff, "Bold Red"));
    player->printColor("   %s", colorSection(staff, "Cyan"));
    player->printColor("   %s\n", colorSection(staff, "Bold Cyan"));
    player->printColor("   %s", colorSection(staff, "Green"));
    player->printColor("   %s\n", colorSection(staff, "Bold Green"));
    player->printColor("   %s", colorSection(staff, "Magenta"));
    player->printColor("   %s\n", colorSection(staff, "Bold Magenta"));
    player->printColor("   %s", colorSection(staff, "Yellow"));
    player->printColor("   %s\n", colorSection(staff, "Bold Yellow"));
    player->printColor("   %s", colorSection(staff, "White"));
    player->printColor("   %s\n", colorSection(staff, "Bold White"));
    player->printColor("   %s", colorSection(staff, "Black"));
    player->printColor("   %s\n", colorSection(staff, "Grey"));
    player->printColor("   %s\n", colorSection(staff, "Blink", '#'));
    player->print("\n");

    player->print("New MXP Colors:\n");
    if(!player->flagIsSet(P_MXP_ENABLED))
        player->printColor("   ^yYou will need to type \"set mxpcolors\" to enable the following colors.\n");
    player->printColor("   %s", colorSection(staff, "Gold"));
    player->printColor("   %s\n", colorSection(staff, "Cerulean"));
    player->printColor("   %s", colorSection(staff, "Pink"));
    player->printColor("   %s\n", colorSection(staff, "Sky Blue"));
    player->printColor("   %s\n", colorSection(staff, "Brown"));
    player->print("\n");
    player->printColor("Custom Colors:  type [color ^W<type> <color>^x]\n");
    player->printColor("                Choose ^Wtype^x from below, ^Wcolor^x from above.\n");
    player->printColor("                Type ^Wcolor plyReset^x to return to default colors.\n\n");

    std::map<std::string,std::string> options;
    std::map<std::string,std::string>::iterator it;
    options["Self: @"] = player->customColorize("*CC:SELF*");
    options["Broadcast"] = player->customColorize("*CC:BROADCAST*");
    options["Tell"] = player->customColorize("*CC:TELL*");
    options["Gossip"] = player->customColorize("*CC:GOSSIP*");
    options["Newbie"] = player->customColorize("*CC:NEWBIE*");
    options["Class"] = player->customColorize("*CC:CLASS*");
    options["Race"] = player->customColorize("*CC:RACE*");
    options["Clan"] = player->customColorize("*CC:CLAN*");
    options["Group"] = player->customColorize("*CC:GROUP*");
    options["Damage"] = player->customColorize("*CC:DAMAGE*");
    if(player->isDm())
        options["DM"] = player->customColorize("*CC:DM*");
    if(player->isAdm())
        options["Admin"] = player->customColorize("*CC:ADMIN*");
    if(player->isCt())
        options["Send"] = player->customColorize("*CC:SEND*");
    if(player->isStaff())
        options["Message"] = player->customColorize("*CC:MESSAGE*");
    if(player->isWatcher())
        options["Watcher"] = player->customColorize("*CC:WATCHER*");
    if(isPtester(player))
        options["P-Test"] = player->customColorize("*CC:PTEST*");
    if(player->getGuild())
        options["Guild"] = player->customColorize("*CC:GUILD*");
    options["Sports"] = player->customColorize("*CC:SPORTS*");

    for(it = options.begin() ; it != options.end() ; ) {
        for(int n=0; n<3; n++) {
            if(it != options.end()) {
                player->printColor("   ^x* %s%-10s", (*it).second.c_str(), (*it).first.c_str());
                it++;
            }
        }
        player->print("\n");
    }
    player->print("\n");

    return(0);
}

//***********************************************************************
//                      defineColors
//***********************************************************************
// Set color flags according to negotiated telnet options on character
// creation.  Character flags override these settings after creation

void Player::defineColors() {
    if(auto sock = mySock.lock()) {
        if (sock->getColorOpt() == ANSI_COLOR) {
            setFlag(P_ANSI_COLOR);
            clearFlag(P_MXP_ENABLED);
            clearFlag(P_NEWLINE_AFTER_PROMPT);
        } else if (sock->getColorOpt() == NO_COLOR) {
            clearFlag(P_ANSI_COLOR);
            clearFlag(P_MXP_ENABLED);
            clearFlag(P_NEWLINE_AFTER_PROMPT);
        }
    }
}
//***********************************************************************
//                      setSocketColors
//***********************************************************************
// Set color options on the socket according to player flags (which can
// overide what we've negotiated)
void Player::setSockColors() {
    if(auto sock = mySock.lock()) {
        if (flagIsSet(P_ANSI_COLOR)) {
            sock->setColorOpt(ANSI_COLOR);
        } else {
            sock->setColorOpt(NO_COLOR);
        }
    }
}

//***********************************************************************
//                      getColorCode
//***********************************************************************
// Get the color code we need to print
// Currently handles MXP & ANSI Color
// TODO: Handle xterm256 color

std::string Socket::getColorCode(const unsigned char ch) {
    std::ostringstream oStr;
    if(!opts.color) {
        // Color is not active, only replace a caret
        if(ch == '^')
            oStr << "^";
        return(oStr.str());
    } else {
        // Color is active, do replacement

        // Only return a color if the last color is not equal to the current color
        if(opts.lastColor != ch || opts.lastColor == '^') {
            opts.lastColor = ch;
            oStr << getAnsiColorCode(ch);
        }
        return(oStr.str());
    }
}

//***********************************************************************
//                      stripColor
//***********************************************************************

std::string stripColor(std::string_view colored) {
    std::ostringstream str;
    unsigned int i=0, max = colored.length();
    for(; i < max ; i++) {
        if(colored.at(i) == '^') i++;
        else str << colored.at(i);
    }
    return(str.str());
}

size_t lengthNoColor(std::string_view colored) {
    size_t len = 0;
    unsigned int i=0, max = colored.length();
    for(; i < max ; i++) {
        if(colored.at(i) == '^')i++;
        else len++;
    }
    return len;
}

//***********************************************************************
//                      escapeColor
//***********************************************************************

std::string escapeColor(std::string_view colored) {
    std::ostringstream str;
    for (const auto c : colored) {
        if(c == '^')  // Double it
            str << c;
        str << c;
    }
    return(str.str());
}

//***********************************************************************
//                      padColor
//***********************************************************************


std::string padColor(const std::string &toPad, size_t pad) {
    pad -= std::min(lengthNoColor(toPad), pad);
    if(pad <= 0)
        return {toPad};
    else
        return toPad + std::string(pad, ' ');
}
