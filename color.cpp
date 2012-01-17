/*
 * color.cpp
 *	 Functions to handle color.
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
#include <sstream>

#define CLEAR		"\033[0m"		// Resets color
#define C_BLACK		"\033[0;30m"	// Normal colors
#define C_RED		"\033[0;31m"
#define C_GREEN		"\033[0;32m"
#define C_YELLOW	"\033[0;33m"
#define C_BLUE		"\033[0;34m"
#define C_MAGENTA	"\033[0;35m"
#define C_CYAN		"\033[0;36m"
#define C_WHITE		"\033[0;37m"
#define C_D_GREY	"\033[1;30m"  	// Light Colors
#define C_B_RED		"\033[1;31m"
#define C_B_GREEN	"\033[1;32m"
#define C_B_YELLOW	"\033[1;33m"
#define C_B_BLUE	"\033[1;34m"
#define C_B_MAGENTA	"\033[1;35m"
#define C_B_CYAN	"\033[1;36m"
#define C_B_WHITE	"\033[1;37m"
#define C_BOLD		"\033[1m"
#define C_BLINK		"\033[5m"

int Ansi[12] = { 30, 31, 32, 33, 34, 35, 36, 37, 1, 0, 5 };
int Mirc[9] = {  1,  4,  9,  8, 12, 13, 11, 15, 15 };

//***********************************************************************
//						nameToColorCode
//***********************************************************************

char nameToColorCode(bstring name) {
	name = name.toLower();
	if(name == "blink")
		return('!');
	if(name == "blue")
		return('b');
	if(name == "bold blue")
		return('B');
	if(name == "red")
		return('r');
	if(name == "bold red")
		return('R');
	if(name == "cyan")
		return('c');
	if(name == "bold cyan")
		return('C');
	if(name == "green")
		return('g');
	if(name == "bold green")
		return('G');
	if(name == "magenta")
		return('m');
	if(name == "bold magenta")
		return('M');
	if(name == "yellow")
		return('y');
	if(name == "bold yellow")
		return('Y');
	if(name == "white")
		return('w');
	if(name == "bold white")
		return('W');
	if(name == "black")
		return('d');
	if(name == "grey")
		return('D');
	if(name == "gold")
		return('l');
	if(name == "cerulean")
		return('e');
	if(name == "pink")
		return('p');
	if(name == "sky blue")
		return('s');
	if(name == "brown")
		return('o');
	return('x');
}

//***********************************************************************
//						getColor
//***********************************************************************

const char* colorCodeToColor(const char type) {
	switch(type) {
	case 'd':	return(C_BLACK);
	case 'b':	return(C_BLUE);
	case 'E':
	case 'c':	return(C_CYAN);
	case 'g':	return(C_GREEN);
	case 'm':	return(C_MAGENTA);
	case 'r':	return(C_RED);
	case 'w':	return(C_WHITE);
	case 'o':
	case 'y':	return(C_YELLOW);
	case 'B':	return(C_B_BLUE);
	case 'e':
	case 'C':	return(C_B_CYAN);
	case 'G':	return(C_B_GREEN);
	case 'M':	return(C_B_MAGENTA);
	case 'R':	return(C_B_RED);
	case 'W':	return(C_B_WHITE);
	case 'Y':	return(C_B_YELLOW);
	case 'D':	return(C_D_GREY);
	case '@':	return(C_BOLD);
	case '#':	return(C_BLINK);
	case '^':	return("^");
	case 'x':
	default:	return(CLEAR C_WHITE);
	}
}

//*********************************************************************
//						getCustomColor
//**********************************************************************

bstring Player::getCustomColor(CustomColor i, bool caret) const {
	char color;
	if(customColors[(int)i] != CUSTOM_COLOR_DEFAULT) {
		color = customColors[(int)i];
		if(color == '!')
			color = '#';
		return((bstring)(caret?"^":"") + color);
	}
	return(gConfig->getCustomColor(i, caret));
}

bstring Config::getCustomColor(CustomColor i, bool caret) const {
	if(customColors[(int)i] == CUSTOM_COLOR_DEFAULT)
		return((bstring)(caret?"^":"") + "x");
	return((bstring)(caret?"^":"") + customColors[(int)i]);
}

//*********************************************************************
//						customColorize
//**********************************************************************

const bstring Monster::customColorize(bstring text, bool caret) const {
	if(following && following->isPlayer())
		text = following->getConstPlayer()->customColorize(text, caret);
	return(text);
}
const bstring Player::customColorize(bstring text, bool caret) const {
	text.Replace("*CC:BROADCAST*", getCustomColor(CUSTOM_COLOR_BROADCAST, caret).c_str());
	text.Replace("*CC:GOSSIP*", getCustomColor(CUSTOM_COLOR_GOSSIP, caret).c_str());
	text.Replace("*CC:PTEST*", getCustomColor(CUSTOM_COLOR_PTEST, caret).c_str());
	text.Replace("*CC:NEWBIE*", getCustomColor(CUSTOM_COLOR_NEWBIE, caret).c_str());
	text.Replace("*CC:DM*", getCustomColor(CUSTOM_COLOR_DM, caret).c_str());
	text.Replace("*CC:ADMIN*", getCustomColor(CUSTOM_COLOR_ADMIN, caret).c_str());
	text.Replace("*CC:SEND*", getCustomColor(CUSTOM_COLOR_SEND, caret).c_str());
	text.Replace("*CC:MESSAGE*", getCustomColor(CUSTOM_COLOR_MESSAGE, caret).c_str());
	text.Replace("*CC:WATCHER*", getCustomColor(CUSTOM_COLOR_WATCHER, caret).c_str());
	text.Replace("*CC:CLASS*", getCustomColor(CUSTOM_COLOR_CLASS, caret).c_str());
	text.Replace("*CC:RACE*", getCustomColor(CUSTOM_COLOR_RACE, caret).c_str());
	text.Replace("*CC:CLAN*", getCustomColor(CUSTOM_COLOR_CLAN, caret).c_str());
	text.Replace("*CC:TELL*", getCustomColor(CUSTOM_COLOR_TELL, caret).c_str());
	text.Replace("*CC:GROUP*", getCustomColor(CUSTOM_COLOR_GROUP, caret).c_str());
	text.Replace("*CC:DAMAGE*", getCustomColor(CUSTOM_COLOR_DAMAGE, caret).c_str());
	text.Replace("*CC:SELF*", getCustomColor(CUSTOM_COLOR_SELF, caret).c_str());
	text.Replace("*CC:GUILD*", getCustomColor(CUSTOM_COLOR_GUILD, caret).c_str());
	return(text);
}

//*********************************************************************
//						resetCustomColors
//**********************************************************************

void Player::resetCustomColors() {
	memset(customColors, CUSTOM_COLOR_DEFAULT, sizeof(customColors));
	customColors[CUSTOM_COLOR_SIZE-1] = 0;
}

//***********************************************************************
//						cmdColors
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

int cmdColors(Player* player, cmd* cmnd) {
	bool staff = player->isStaff();
	player->defineMXP();

	if(!strcmp(cmnd->str[1], "reset")) {
		player->print("Custom colors have been reset to defaults.\n");
		player->resetCustomColors();
		return(0);
	} else if(cmnd->num > 2) {
		CustomColor i;
		bstring type = cmnd->str[1];
		type = type.toLower();

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
	player->printColor("                Type ^Wcolor reset^x to return to default colors.\n\n");

	std::map<bstring,bstring> options;
	std::map<bstring,bstring>::iterator it;
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
//						defineColors
//***********************************************************************
// setup what color they want - called on new character and login of
// existing character

void Player::defineColors() {
	if(mySock->color == ANSI_COLOR) {
		setFlag(P_ANSI_COLOR);
		clearFlag(P_MXP_ENABLED);
		clearFlag(P_MIRC);
		clearFlag(P_NEWLINE_AFTER_PROMPT);
	}
	else if(mySock->color == MXP_COLOR) {
		setFlag(P_ANSI_COLOR);
		setFlag(P_MXP_ENABLED);
		defineMXP();
	} else if(mySock->color == NO_COLOR) {
		clearFlag(P_ANSI_COLOR);
		clearFlag(P_MXP_ENABLED);
		clearFlag(P_MIRC);
		clearFlag(P_NEWLINE_AFTER_PROMPT);
	}
}

//***********************************************************************
//						defineMXP
//***********************************************************************

void Player::defineMXP() {
	if(!flagIsSet(P_MXP_ENABLED) || !mySock->getMxp())
		return;
	print("%c[1z", 27);
	print("<!ELEMENT c1 '<COLOR #FFD700>'>");	// gold
	print("<!ELEMENT c2 '<COLOR #009CFF>'>");	// cerulean
	print("<!ELEMENT c3 '<COLOR #FF5ADE>'>");	// pink
	print("<!ELEMENT c4 '<COLOR #82E6FF>'>");	// sky blue
	print("<!ELEMENT c5 '<COLOR #484848>'>");	// dark grey
	print("<!ELEMENT c6 '<COLOR #95601A>'>");	// brown
	printColor("%c[3z^x", 27);
}

//***********************************************************************
//						ANSI
//***********************************************************************

void ANSI(Socket* sock, int color) {
	if(sock && sock->getPlayer()) {
		if(sock->getPlayer()->flagIsSet(P_ANSI_COLOR)) {
			if(color & BOLD)			sock->print("%c[%dm", 27, Ansi[COLOR_BOLD]);
			if(color & BLINK)			sock->print("%c[%dm", 27, Ansi[COLOR_BLINK]);
			if(color & NORMAL)			sock->print("%c[%dm", 27, Ansi[COLOR_NORMAL]);

				 if(color & RED)		sock->print("%c[%dm", 27, Ansi[COLOR_RED]);
			else if(color & GREEN)		sock->print("%c[%dm", 27, Ansi[COLOR_GREEN]);
			else if(color & YELLOW)		sock->print("%c[%dm", 27, Ansi[COLOR_YELLOW]);
			else if(color & BLUE)		sock->print("%c[%dm", 27, Ansi[COLOR_BLUE]);
			else if(color & MAGENTA)	sock->print("%c[%dm", 27, Ansi[COLOR_MAGENTA]);
			else if(color & CYAN)		sock->print("%c[%dm", 27, Ansi[COLOR_CYAN]);
			else if(color & BLACK)		sock->print("%c[%dm", 27, Ansi[COLOR_BLACK]);
			else if(color & WHITE)		sock->print("%c[%dm", 27, Ansi[COLOR_WHITE]);

		} else if(sock->getPlayer()->flagIsSet(P_MIRC) && color != BLINK) {
			if(color & BOLD)			sock->print("%c", 2);
			if(color & UNDERLINE)		sock->print("%c", 31);
			if(color & NORMAL)			sock->print("%c%c", 3, 3);

				 if(color & RED)		sock->print("%c%d", 3, Mirc[COLOR_RED]);
			else if(color & GREEN)		sock->print("%c%d", 3, Mirc[COLOR_GREEN]);
			else if(color & YELLOW)		sock->print("%c%d", 3, Mirc[COLOR_YELLOW]);
			else if(color & BLUE)		sock->print("%c%d", 3, Mirc[COLOR_BLUE]);
			else if(color & MAGENTA)	sock->print("%c%d", 3, Mirc[COLOR_MAGENTA]);
			else if(color & CYAN)		sock->print("%c%d", 3, Mirc[COLOR_CYAN]);
			else if(color & BLACK)		sock->print("%c%d", 3, Mirc[COLOR_BLACK]);
			else if(color & WHITE)		sock->print("%c%d", 3, Mirc[COLOR_WHITE]);
		}
	}
}

//***********************************************************************
//						getMXPColor
//***********************************************************************

const char* getMXPColor(const char type, Player* player) {
	// max length from getColor is 9, ending active MXP is 4,
	// so 15 storage will be enough
	static char r[15];
	r[0] = 0;

	// mxp specific color tags
	switch(type) {
	case 'l':
	case 'e':
	case 'p':
	case 's':
	case 'E':
	case 'o':
		// start mxp
		sprintf(r, "%c[4z", 27);
		player->setFlag(P_MXP_ACTIVE);

		// grab the color
		switch(type) {
		case 'l':	strcat(r, "<c1>");	break;
		case 'e':	strcat(r, "<c2>");	break;
		case 'p':	strcat(r, "<c3>");	break;
		case 's':	strcat(r, "<c4>");	break;
		case 'E':	strcat(r, "<c5>");	break;
		case 'o':	strcat(r, "<c6>");	break;
		default:	break;
		}
		return(r);
	default:
		break;
	}

	// don't deactivate color for carets!
	if(type == '^')
		return("^");

	if(player->flagIsSet(P_MXP_ACTIVE)) {
		// end mxp
		sprintf(r, "%c[3z", 27);
		player->clearFlag(P_MXP_ACTIVE);
	}

	strcat(r, colorCodeToColor(type));
	return(r);
}

//***********************************************************************
//						stripColor
//***********************************************************************

bstring stripColor(bstring color) {
	std::ostringstream str;
	unsigned int i=0, max = color.getLength();
	for(; i < max ; i++) {
		if(color.getAt(i) == '^')
			i++;
		else
			str << color.getAt(i);
	}
	return(str.str());
}

//***********************************************************************
//						escapeColor
//***********************************************************************

bstring escapeColor(bstring color) {
	color.Replace("^","^^");
	return(color);
}

//***********************************************************************
//						colorize
//***********************************************************************

bstring colorize(const char* txt, int option, Player* player) {
	const char* point;
	char last = '0';
	std::ostringstream coloredStr;

	if(option) {
		for(point = txt; *point; point++) {
			if(*point == '^') {
				point++;
				// If we're trying to send out the same color as before
				// no point wasting the extra space
				if(*point != last || last == '^') {
					if(player && player->flagIsSet(P_MXP_ENABLED))
						coloredStr << getMXPColor(*point, player);
					else
						coloredStr << colorCodeToColor(*point);
				}
				last = *point;
				continue;
			}
			coloredStr << *point;
		}
		// Now set the color back to normal
		if(player && player->flagIsSet(P_MXP_ENABLED))
			coloredStr << getMXPColor('x', player);
		else
			coloredStr << colorCodeToColor('x');
	} else {
		for(point=txt; *point; point++ ) {
			if(*point == '^') {
				point++;
				if(*point == '^')
					coloredStr << "^";
				continue;
			}
			coloredStr << *point;
		}
	}
	return(coloredStr.str());
}

