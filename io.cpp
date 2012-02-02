/*
 * io.cpp
 *	 Socket input/output/establishment functions.
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
#include <arpa/telnet.h>
#include "mud.h"

#include <stdio.h>
#include <sys/types.h>

#include <netinet/in.h> // Needs: htons, htonl, INADDR_ANY, sockaddr_in

#include <sys/ioctl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>



#include "login.h"
#include "socket.h"
#include "timer.h"
#include "clans.h"

#include <sstream>

#define TELOPT_COMPRESS2       86
#define buflen(a,b,c)	(a-b + (a<b ? c:0))
#define saddr		addr.sin_addr.s_addr
#define MAXPLAYERS	100

int	Numplayers;
int	controlSock;
extern unsigned short Port;

//********************************************************************
//						broadcast
//********************************************************************

bool hearBroadcast(Creature* target, Socket* ignore1, Socket* ignore2, bool showTo(Socket*)) {
	if(!target)
		return(false);
	if(target < 0)
		return(false);
	if(target->getSock() == ignore1 || target->getSock() == ignore2)
		return(false);

	if(showTo && !showTo(target->getSock()))
		return(false);

	Player* pTarget = target->getPlayer();
	if(pTarget) {
		if(	ignore1 != NULL &&
			ignore1->getPlayer() &&
			pTarget->isGagging(ignore1->getPlayer()->getName())
		)
			return(false);
		if(	ignore2 != NULL &&
			ignore2->getPlayer() &&
			pTarget->isGagging(ignore2->getPlayer()->getName())
		)
			return(false);
	}

	return(true);
}


// global broadcast
void doBroadCast(bool showTo(Socket*), bool showAlso(Socket*), const char *fmt, va_list ap, Creature* player) {
	for(Socket* sock : gServer->sockets) {
		const Player* ply = sock->getPlayer();

		if(!ply)
			continue;
		if(ply->fd < 0)
			continue;
		if(!showTo(sock))
			continue;
		if(showAlso && !showAlso(sock))
			continue;
		// No gagging staff!
		if(player && ply->isGagging(player->name) && !player->isCt())
			continue;

		ply->vprint(ply->customColorize(fmt).c_str(), ap);
		ply->printColor("^x\n");
	}
}


// room broadcast
void doBroadcast(bool showTo(Socket*), Socket* ignore1, Socket* ignore2, BaseRoom* room, const char *fmt, va_list ap) {
	Player* target=0;
	ctag	*cp=0;

	if(!room)
		return;

	cp = room->first_ply;
	while(cp) {
		target = cp->crt->getPlayer();
		cp = cp->next_tag;

		if(!hearBroadcast(target, ignore1, ignore2, showTo))
			continue;
		if(target->flagIsSet(P_UNCONSCIOUS))
			continue;

		target->vprint(target->customColorize(fmt).c_str(), ap);
		target->printColor("^x\n");

	}
}


// room broadcast, 1 ignore
void broadcast(Socket* ignore, BaseRoom* room, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	doBroadcast(0, ignore, NULL, room, fmt, ap);
	va_end(ap);
}

// room broadcast, 2 ignores
void broadcast(Socket* ignore1, Socket* ignore2, BaseRoom* room, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	doBroadcast(0, ignore1, ignore2, room, fmt, ap);
	va_end(ap);
}

// room broadcast, 1 ignore, showTo function
void broadcast(bool showTo(Socket*), Socket* ignore, BaseRoom* room, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	doBroadcast(showTo, ignore, NULL, room, fmt, ap);
	va_end(ap);
}

// simple broadcast
void broadcast(const char *fmt,...) {
	va_list ap;
	va_start(ap, fmt);
	doBroadCast(yes, 0, fmt, ap);
	va_end(ap);
}

// broadcast with showTo function
void broadcast(bool showTo(Socket*), bool showAlso(Socket*), const char *fmt,...) {
	ASSERTLOG(showTo);
	va_list ap;
	va_start(ap, fmt);
	doBroadCast(showTo, showAlso, fmt, ap);
	va_end(ap);
}

// broadcast with showTo function
void broadcast(bool showTo(Socket*), const char *fmt,...) {
	ASSERTLOG(showTo);
	va_list ap;
	va_start(ap, fmt);
	doBroadCast(showTo, 0, fmt, ap);
	va_end(ap);
}

void broadcast(Creature* player, bool showTo(Socket*), int color, const char *fmt,...) {
	ASSERTLOG(showTo);
	va_list ap;
	va_start(ap, fmt);
	doBroadCast(showTo, (bool(*)(Socket*))NULL, fmt, ap, player);
	va_end(ap);
}

bool yes(Creature* player) {
	return(true);
}
bool yes(Socket* sock) {
	return(true);
}
bool wantsPermDeaths(Socket* sock) {
	Player* ply = sock->getPlayer();
	return(ply && ply && !ply->flagIsSet(P_NO_BROADCASTS) && ply->flagIsSet(P_PERM_DEATH));
}
void announcePermDeath(Creature* player, const char *fmt,...) {
	va_list ap;

	if(player->isStaff())
		return;

	va_start(ap, fmt);
	doBroadCast(wantsPermDeaths, 0, fmt, ap);
	va_end(ap);
}

//********************************************************************
//						broadcast_login
//********************************************************************
// This function broadcasts a message to all the players that are in the
// game. If they have the NO-BROADCAST flag set, then they will not see it.

void broadcast_login(Player* player, int login) {
	std::ostringstream preText, postText, extra, room;
	bstring text = "", illusion = "";
	int    logoff=0;

	ASSERTLOG( player );
	ASSERTLOG( player->name );

	preText << "### " << player->fullName() << " ";

	if(login) {
		preText << "the " << gConfig->getRace(player->getDisplayRace())->getAdjective();
		// the illusion will be checked here
		postText << " " << player->getTitle().c_str();

		if(player->getDeity()) {
			const DeityData* deity = gConfig->getDeity(player->getDeity());
			postText << " (" << deity->getName() << ")";
		} else if(player->getClan()) {
			const Clan* clan = gConfig->getClan(player->getClan());
			postText << " (" << clan->getName() << ")";
		}

		postText << " just logged in.";

		extra << "### (Lvl: " << (int)player->getLevel() << ") (Host: " << player->getSock()->getHostname().c_str() << ")";
	} else {
		logoff = 1;
		preText << "just logged off.";
	}

	// format text for illusions
	text = preText.str() + postText.str();
	illusion = preText.str();
	if(login && player->getDisplayRace() != player->getRace())
		illusion += " (" + gConfig->getRace(player->getRace())->getAdjective() + ")";
	illusion += postText.str();

	if(player->parent_rom)
		room << " (" << player->parent_rom->info.str() << ")";
	else if(player->area_room)
		room << " " << player->area_room->mapmarker.str();


	// TODO: these are set elsewhere, too... check that out
	if(!player->isStaff()) {
		player->clearFlag(P_DM_INVIS);
		player->clearFlag(P_INCOGNITO);
	} else if(player->getClass()==BUILDER) {
		player->setFlag(P_INCOGNITO);
	}

	if(player->flagIsSet(P_DM_INVIS) || player->flagIsSet(P_INCOGNITO)) {
		if(player->isDm())
			broadcast(isDm, "^g%s", illusion.c_str());
		else if(player->getClass() == CARETAKER)
			broadcast(isCt, "^Y%s", illusion.c_str());
		else if(player->getClass() == BUILDER) {
			broadcast(isStaff, "^G%s", illusion.c_str());
		}
	} else {


		Player* target=0;
		for(std::pair<bstring, Player*> p : gServer->players) {
			target = p.second;

			if(!target->isConnected())
				continue;
			if(target->isGagging(player->name))
				continue;
			if(target->flagIsSet(P_NO_LOGIN_MESSAGES))
				continue;

			if(target->getClass() <= BUILDER && !target->isWatcher()) {
				if(	!player->isStaff() &&
					!(logoff && target->getClass() == BUILDER)
				)
					target->print("%s\n", player->willIgnoreIllusion() ? illusion.c_str() : text.c_str());
			} else if((target->isCt() || target->isWatcher()) && player->getClass() != BUILDER) {
				target->print("%s%s\n", player->willIgnoreIllusion() ? illusion.c_str() : text.c_str(), room.str().c_str());
				if(login && target->isCt())
					target->print("%s\n", extra.str().c_str());
			}
		}
	}
}


//********************************************************************
//						broadcast_rom
//********************************************************************
// This function outputs a message to everyone in the room specified
// by the integer in the second parameter.  If the first parameter
// is greater than -1, then if the player specified by that file
// descriptor is present in the room, they are not given the message

// TODO: Dom: remove
void broadcast_rom_LangWc(int face, int color, int lang, Socket* ignore, AreaRoom* aRoom, CatRef cr, const char *fmt,...) {
	char	fmt2[1024];
	va_list ap;

	va_start(ap, fmt);
	strcpy(fmt2, fmt);
	strcat(fmt2, "\n");

	face = MAX(0,face);
	color = MAX(0,color);

	Player* ply;
	for(std::pair<bstring, Player*> p : gServer->players) {
		ply = p.second;
		Socket* sock = ply->getSock();

		if(!sock->isConnected())
			continue;
		if(	ignore != NULL &&
			ignore->getPlayer() &&
			ignore->getPlayer()->inSameRoom(ply) &&
			ply->isGagging(ignore->getPlayer()->name)
		)
			continue;
		if(	(	(cr.id && *&ply->room == *&cr) ||
				(aRoom && ply->area_room && *&aRoom->mapmarker == *&ply->area_room->mapmarker)
			) &&
			sock->getFd() > -1 &&
			sock != ignore &&
			!ply->flagIsSet(P_UNCONSCIOUS) &&
			(ply->languageIsKnown(lang) || ply->isEffected("comprehend-languages"))
		) {
			if(face)
				ANSI(sock, face);
			if(color)
				ANSI(sock, color);
			ply->vprint(fmt2, ap);
		}
	}
	va_end(ap);
}

//********************************************************************
//						broadcastGroup
//********************************************************************
// this function broadcasts a message to all players in a group except
// the source player

void broadcastGroupMember(bool dropLoot, Creature* player, const Player* listen, const char *fmt, va_list ap) {

	if(!listen)
		return;
	if(listen == player)
		return;

	// dropLoot broadcasts to all of this
	if(!dropLoot) {
		if(listen->isGagging(player->name))
			return;
		if(listen->flagIsSet(P_IGNORE_ALL))
			return;
		if(listen->flagIsSet(P_IGNORE_GROUP_BROADCAST))
			return;
	} else {
		if(!player->inSameRoom(listen))
			return;
	}

	// staff don't broadcast to mortals when invis
	if(player->isPlayer() && player->isStaff() && player->getClass() > listen->getClass()) {
		if(player->flagIsSet(P_DM_INVIS))
			return;
		if(player->flagIsSet(P_INCOGNITO) && !player->inSameRoom(listen))
			return;
	}

	listen->vprint(listen->customColorize(fmt).c_str(), ap);
}

void broadcastGroup(bool dropLoot, Creature* player, const char *fmt,...) {
	Group* group = player->getGroup();
	if(!group)
		return;
	va_list ap;
	va_start(ap, fmt);

//	broadcastGroupMember(dropLoot, player, leader, fmt, ap);
	for(Creature* crt : group->members) {
		broadcastGroupMember(dropLoot, player, crt->getConstPlayer(), fmt, ap);
	}
}

//**********************************************************************
//						inetname
//**********************************************************************
// This function returns the internet address of the address structure
// passed in as the first parameter.

char *inetname(struct in_addr in) {
	register char *cp=0;
	static char line[50];

	if(in.s_addr == INADDR_ANY)
		strcpy(line, "*");
	else if(cp)
		strcpy(line, cp);
	else {
		in.s_addr = ntohl(in.s_addr);

		sprintf(line, "%u.%u.%u.%u",
			(int)(in.s_addr >> 24) & 0xff,
			(int)(in.s_addr >> 16) & 0xff,
			(int)(in.s_addr >> 8) & 0xff,
			(int)(in.s_addr) & 0xff
		);
	}
	return (line);
}

//**********************************************************************
//						child_died
//**********************************************************************
// This function gets called when a SIGCHLD signal is sent to the
// program.

void child_died(int sig) {
	gServer->childDied();
	std::cout << "Child died: " << gServer->getDeadChildren() << " children dead now.\n";
	signal(SIGCHLD, child_died);
}
//**********************************************************************

//	This causes the game to shutdown in one minute.  It is used
//  by signal to force a  shutdown in response to a HUP signal
//  (i.e. kill -HUP pid) from the system.

void quick_shutdown(int sig) {
	Shutdown.ltime = time(0);
	Shutdown.interval = 120;
}

//**********************************************************************
//						broadcastGuild
//**********************************************************************
// Broadcasts to everyone in the same guild as player

void broadcastGuild(int guildNum, int showName, const char *fmt,...) {
	char	fmt2[1024];
	va_list ap;

	va_start(ap, fmt);
	if(showName) {
		strcpy(fmt2, "*CC:GUILD*[");
		strcat(fmt2, getGuildName(guildNum));
		strcat(fmt2, "] ");
	} else
		strcpy(fmt2, "");
	strcat(fmt2, fmt);
	strcat(fmt2, "\n");

	Player* target=0;
	for(std::pair<bstring, Player*> p : gServer->players) {
		target = p.second;

		if(!target->isConnected())
			continue;
		if(target->getGuild() == guildNum && target->getGuildRank() >= GUILD_PEON && !target->flagIsSet(P_NO_BROADCASTS)) {
			target->vprint(target->customColorize(fmt2).c_str(), ap);
		}
	}
	va_end(ap);

}

//********************************************************************
//						disconnect_all_ply
//********************************************************************
// This function disconnects all players.

void disconnect_all_ply() {
	for(Socket* sock : gServer->sockets) {
		sock->disconnect();
	}
	gServer->cleanUp();
}

//*********************************************************************
//						shutdown_now
//*********************************************************************

void shutdown_now(int sig) {
	broadcast("### Quick shutdown now!");
	gServer->processOutput();
	loge("--- Game shutdown with SIGINT\n");
	gConfig->resaveAllRooms(1);
	save_all_ply();

	printf("Goodbye.\n");
	kill(getpid(), 9);
}

//*********************************************************************
//						check_flood
//*********************************************************************

int check_flood(int fd) {
//	int i,j=0;
//
//	for(i=0; i<Tablesize ;i++) {
//		if(fd!=i) {
//			if(Ply[i].sock) {
//				if(!strcmp(Ply[i].sock->getHostname().c_str(),sock->getHostname().c_str()) && i != fd) {
//					j++;
//					if(j >= 3) {
//						disconnect(i);
//						return(1);
//					}
//				}
//			}
//		}
//	}
//	gServer->cleanUp();
	return(0);
}

//*********************************************************************
//						checkWinFilename
//*********************************************************************

int checkWinFilename(Socket* sock, const bstring str) {
	// only do this if we're on windows
	#ifdef __CYGWIN__
	if(	str.equals("aux") ||
	        str.equals("prn") ||
	        str.equals("nul") ||
	        str.equals("con") ||
	        str.equals("com1") ||
	        str.equals("com2"))
	{
		if(sock) sock->print("\"%s\" could not be read.\n", str);
		return(0);
	}
	#endif
	return(1);
}

//*********************************************************************
//						pueblo functions
//*********************************************************************

bool Pueblo::is(bstring txt) {
	return(txt.left(activation.getLength()).toLower() == activation);
}

bstring Pueblo::multiline(bstring str) {
	int i=1, plen = activation.getLength()-1;
	if(Pueblo::is(str)) {
		i = plen;
		str.Insert(0, ' ');
	}
	int len = str.getLength();
	for(; i<len; i++) {
		if(	str.getAt(i-1) == '\n' &&
			Pueblo::is(str.right(len-i))
		) {
			str.Insert(i, ' ');
			len++;
			i += plen;
		}
	}
	return(str);
}

//*********************************************************************
//						escapeText
//*********************************************************************

void Monster::escapeText() {
	if(Pueblo::is(name))
		strcpy(name, "clay form");
	description = Pueblo::multiline(description);
}

void Player::escapeText() {
	if(Pueblo::is(description))
		description = "";
}

void Object::escapeText() {
	if(Pueblo::is(name))
		strcpy(name, "clay ball");
	if(Pueblo::is(use_output))
		zero(use_output, sizeof(use_output));
	description = Pueblo::multiline(description);
}

void UniqueRoom::escapeText() {
	if(Pueblo::is(name))
		strcpy(name, "New Room");

	short_desc = Pueblo::multiline(short_desc);
	long_desc = Pueblo::multiline(long_desc);

	for(Exit* exit : exits) {
		exit->escapeText();
	}
}

void Exit::escapeText() {
	if(Pueblo::is(name))
		strcpy(name, "exit");
	if(Pueblo::is(enter))
		enter = "";
	if(Pueblo::is(open))
		open = "";
	description = Pueblo::multiline(description);
}

//*********************************************************************
//						xsc
//*********************************************************************
// Stands for xmlspecialchars (based on htmlspecialchars for PHP) -
// escapes special characters to make the text safe for XML.
// If given Br�gal (which the xml parser cannot save), it will turn it
// into Br&#252;gal (which the xml parser can save). Display will be affected
// on old clients, so this should only be run when saving the string.

bstring xsc(const bstring& txt) {
	bstring ret = "";
	unsigned char c=0;
	int t = txt.getLength();
	for(int i=0; i<t; i++) {
		c = txt.getAt(i);
		// beyond 127 we get into the unsupported character range
		if(c > 127)
			ret += (bstring)"&#"+c+";";
		else
			ret += (char)c;
	}
	return(ret);
}

//*********************************************************************
//						unxsc
//*********************************************************************
// Reverse of xsc - attempts to turn &#252; into �. We do thhis when we load from file.

bstring unxsc(const bstring& txt) {
	return(unxsc(txt.c_str()));
}
bstring unxsc(const char* txt) {
	bstring ret = "";
	int c=0, len = strlen(txt);
	for(int i=0; i<len; i++) {
		c = txt[i];

		if(c == '&' && txt[i+1] == '#') {
			// get the number from the string
			c = atoi(&txt[i+2]);
			// advance i appropriately
			i += 2;
			while(txt[i] != ';') {
				i++;
				if(i >= len)
					return("");
			}
		}

		ret += (char)c;
	}
	return(ret);
}

//*********************************************************************
//						keyTxtConvert
//*********************************************************************
// Treat all accented characters like their unaccented version. This makes
// it easier for players without the accents on their keyboards. Make
// sure everything is treated as lower case.

char keyTxtConvert(unsigned char c) {
	// early exit for most characters
	if(isalpha(c))
		return(tolower(c));
	switch(c) {
	case 192:
	case 193:
	case 194:
	case 195:
	case 196:
	case 197:
	case 224:
	case 225:
	case 226:
	case 227:
	case 228:
	case 229:
		return('a');
	case 199:
	case 231:
		return('c');
	case 200:
	case 201:
	case 202:
	case 203:
	case 232:
	case 233:
	case 234:
	case 235:
		return('e');
	case 204:
	case 205:
	case 206:
	case 207:
	case 236:
	case 237:
	case 238:
	case 239:
		return('i');
	case 209:
	case 241:
		return('n');
	case 210:
	case 211:
	case 212:
	case 213:
	case 214:
	case 216:
	case 242:
	case 243:
	case 244:
	case 245:
	case 246:
	case 248:
		return('o');
	case 217:
	case 218:
	case 219:
	case 220:
	case 249:
	case 250:
	case 251:
	case 252:
		return('u');
	case 221:
	case 253:
	case 255:
		return('y');
	default:

		return(tolower(c));
	}
}

//*********************************************************************
//						keyTxtConvert
//*********************************************************************

bstring keyTxtConvert(const bstring& txt) {
	bstring ret = "";
	for(int i=0; i<txt.getLength(); i++) {
		ret += keyTxtConvert(txt.getAt(i));
	}
	return(ret);
}

//*********************************************************************
//						keyTxtCompare
//*********************************************************************
// This does the hard work of comparing two different strings:
// Br�gAl, bRU&#252;gaL, Br�g�l, and brugal must all match. We can do
// this by using keyTxtConvert to take any supported character and make it
// lower case and by manually extracting the ASCII code.

bool keyTxtCompare(const char* key, const char* txt, int tLen) {
	int kI=0, tI=0, convert=0, kLen = strlen(key);
	char kC, tC;
	if(!kLen || !tLen)
		return(false);
	for(;;) {
		// at the end of the input string and no errors
		if(tI >= tLen)
			return(true);
		// at the end of the main string but not at the end of the input string
		if(kI >= kLen)
			return(false);

		// remove color
		if(key[kI] == '^') {
			kI += 2;
			continue;
		}
		if(txt[tI] == '^') {
			tI += 2;
			continue;
		}

		if(key[kI] == '&' && key[kI+1] == '#') {
			// get the number from the string
			convert = atoi(&key[kI+2]);
			// advance kI appropriately
			kI += 2;
			while(key[kI] != ';') {
				kI++;
				if(kI >= kLen)
					return(false);
			}
			// turn it into a character we can compare
			kC = keyTxtConvert(convert);
		} else
			kC = keyTxtConvert(key[kI]);

		tC = keyTxtConvert(txt[tI]);

		if(tC != kC)
			return(false);
		tI++;
		kI++;
	}
}

//*********************************************************************
//						keyTxtEqual
//*********************************************************************

bool keyTxtEqual(const Creature* target, const char* txt) {
	int len = strlen(txt);
	if(	keyTxtCompare(target->key[0], txt, len) ||
		keyTxtCompare(target->key[1], txt, len) ||
		keyTxtCompare(target->key[2], txt, len) ||
		keyTxtCompare(target->name, txt, len)
	)
		return(true);
	return(false);
}

bool keyTxtEqual(const Object* target, const char* txt) {
	int len = strlen(txt);
	if(	keyTxtCompare(target->key[0], txt, len) ||
		keyTxtCompare(target->key[1], txt, len) ||
		keyTxtCompare(target->key[2], txt, len) ||
		keyTxtCompare(target->name, txt, len)
	)
		return(true);
	return(false);
}

//*********************************************************************
//						isPrintable
//*********************************************************************
// wrapper for isprint to handle supported characters

bool isPrintable(char c) {
	return(isprint(keyTxtConvert(c)));
}
