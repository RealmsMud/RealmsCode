/*
 * io.cpp
 *   Socket input/output/establishment functions.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <cctype>         // for tolower, isalpha, isprint
#include <netinet/in.h>   // for in_addr, INADDR_ANY, ntohl
#include <cstdarg>        // for va_list, va_start, va_end
#include <cstdlib>        // for atoi, exit
#include <cstring>        // for strcat, strcpy, strlen
#include <csignal>        // for signal, SIGCHLD
#include <cstdio>         // for size_t, sprintf, NULL
#include <sstream>        // for operator<<, basic_ostream, ostringstream

#include "bstring.hpp"    // for bstring, operator+
#include "clans.hpp"      // for Clan
#include "config.hpp"     // for Config, gConfig
#include "container.hpp"  // for Container, PlayerSet
#include "creatures.hpp"  // for Player, Creature, Monster
#include "deityData.hpp"  // for DeityData
#include "exits.hpp"      // for Exit
#include "flags.hpp"      // for P_DM_INVIS, P_NO_BROADCASTS, P_UNCONSCIOUS
#include "global.hpp"     // for CreatureClass, CreatureClass::BUILDER, Crea...
#include "group.hpp"      // for CreatureList, Group
#include "location.hpp"   // for Location
#include "mud.hpp"        // for GUILD_PEON
#include "objects.hpp"    // for Object
#include "os.hpp"         // for ASSERTLOG
#include "proto.hpp"      // for broadcast, activation, getGuildName, get_la...
#include "raceData.hpp"   // for RaceData
#include "rooms.hpp"      // for UniqueRoom, BaseRoom, AreaRoom, ExitList
#include "server.hpp"     // for Server, gServer, PlayerMap, SocketList
#include "socket.hpp"     // for Socket


// Communication.cpp
extern const long IN_GAME_WEBHOOK;

#define TELOPT_COMPRESS2       86

int Numplayers;


//********************************************************************
//                      broadcast
//********************************************************************

bool hearBroadcast(Creature* target, Socket* ignore1, Socket* ignore2, bool showTo(Socket*)) {
    if(!target)
        return(false);
    if(target == NULL)
        return(false);
    if(target->getSock() == ignore1 || target->getSock() == ignore2)
        return(false);

    if(showTo && !showTo(target->getSock()))
        return(false);

    Player* pTarget = target->getAsPlayer();
    if(pTarget) {
        if( ignore1 != nullptr &&
            ignore1->getPlayer() &&
            pTarget->isGagging(ignore1->getPlayer()->getName())
        )
            return(false);
        if( ignore2 != nullptr &&
            ignore2->getPlayer() &&
            pTarget->isGagging(ignore2->getPlayer()->getName())
        )
            return(false);
    }

    return(true);
}


// global broadcast
void doBroadCast(bool showTo(Socket*), bool showAlso(Socket*), const char *fmt, va_list ap, Creature* player) {
    for(Socket &sock : gServer->sockets) {
        const Player* ply = sock.getPlayer();

        if(!ply)
            continue;
        if(ply->fd < 0)
            continue;
        if(!showTo(&sock))
            continue;
        if(showAlso && !showAlso(&sock))
            continue;
        // No gagging staff!
        if(player && ply->isGagging(player->getName()) && !player->isCt())
            continue;

        ply->vprint(ply->customColorize(fmt).c_str(), ap);
        ply->printColor("^x\n");
    }
}


// room broadcast
void doBroadcast(bool showTo(Socket*), Socket* ignore1, Socket* ignore2, const Container* container, const char *fmt, va_list ap) {
    if(!container)
        return;

    for(Player* ply : container->players) {
        if(!hearBroadcast(ply, ignore1, ignore2, showTo))
            continue;
        if(ply->flagIsSet(P_UNCONSCIOUS))
            continue;

        ply->vprint(ply->customColorize(fmt).c_str(), ap);
        ply->printColor("^x\n");

    }
}


// room broadcast, 1 ignore
void broadcast(Socket* ignore, const Container* container, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    doBroadcast(0, ignore, nullptr, container, fmt, ap);
    va_end(ap);
}

// room broadcast, 2 ignores
void broadcast(Socket* ignore1, Socket* ignore2, const Container* container, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    doBroadcast(0, ignore1, ignore2, container, fmt, ap);
    va_end(ap);
}

// room broadcast, 1 ignore, showTo function
void broadcast(bool showTo(Socket*), Socket* ignore, const Container* container, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    doBroadcast(showTo, ignore, nullptr, container, fmt, ap);
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
    doBroadCast(showTo, (bool(*)(Socket*))nullptr, fmt, ap, player);
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
    return(ply != nullptr && !ply->flagIsSet(P_NO_BROADCASTS) && ply->flagIsSet(P_PERM_DEATH));
}

//********************************************************************
//                      broadcast_login
//********************************************************************
// This function broadcasts a message to all the players that are in the
// game. If they have the NO-BROADCAST flag set, then they will not see it.

void broadcastLogin(Player* player, BaseRoom* inRoom, int login) {
    std::ostringstream preText, postText, extra, room;
    bstring text = "", illusion = "";
    int    logoff=0;

    ASSERTLOG( player );
    ASSERTLOG( !player->getName().empty() );

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

        extra << "### (Lvl: " << (int)player->getLevel() << ") (Host: " << player->getSock()->getHostname() << ")";
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

    if(inRoom) {
        if(inRoom->isUniqueRoom())
            room << " (" << inRoom->getAsUniqueRoom()->info.str() << ")";
        else if(inRoom->isAreaRoom())
            room << " " << inRoom->getAsAreaRoom()->mapmarker.str();
    }

    // TODO: these are set elsewhere, too... check that out
    if(!player->isStaff()) {
        player->clearFlag(P_DM_INVIS);
        player->removeEffect("incognito");
    } else if(player->getClass() == CreatureClass::BUILDER) {
        player->addEffect("incognito", -1);
    }

    if(player->flagIsSet(P_DM_INVIS) || player->isEffected("incognito")) {
        if(player->isDm())
            broadcast(isDm, "^g%s", illusion.c_str());
        else if(player->getClass() == CreatureClass::CARETAKER)
            broadcast(isCt, "^Y%s", illusion.c_str());
        else if(player->getClass() == CreatureClass::BUILDER) {
            broadcast(isStaff, "^G%s", illusion.c_str());
        }
    } else {
        gServer->sendDiscordWebhook(IN_GAME_WEBHOOK, -1, "", illusion);

        for(const auto& [tId, target] : gServer->players) {
            if(!target->isConnected())
                continue;
            if(target->isGagging(player->getName()))
                continue;
            if(target->flagIsSet(P_NO_LOGIN_MESSAGES))
                continue;

            if(target->getClass() <= CreatureClass::BUILDER && !target->isWatcher()) {
                if( !player->isStaff() && !(logoff && target->getClass() == CreatureClass::BUILDER))
                    *target << (player->willIgnoreIllusion() ? illusion : text) << "\n";
            } else if((target->isCt() || target->isWatcher()) && player->getClass() !=  CreatureClass::BUILDER) {
                *target << (player->willIgnoreIllusion() ? illusion : text) << room.str() << "\n";
                if(login && target->isCt())
                    *target << extra.str() << "\n";
            }
        }
    }
}


//********************************************************************
//                      broadcast_rom
//********************************************************************
// This function outputs a message to everyone in the room specified
// by the integer in the second parameter.  If the first parameter
// is greater than -1, then if the player specified by that file
// descriptor is present in the room, they are not given the message

// TODO: Dom: remove
void broadcast_rom_LangWc(int lang, Socket* ignore, const Location& currentLocation, const char *fmt,...) {
    char    fmt2[1024];
    va_list ap;

    va_start(ap, fmt);
    strcpy(fmt2, fmt);
    strcat(fmt2, "\n");

    Player* ply;
    for(const auto& p : gServer->players) {
        ply = p.second;
        Socket* sock = ply->getSock();

        if(!sock->isConnected())
            continue;
        if( ignore != nullptr &&
            ignore->getPlayer() &&
            ignore->getPlayer()->inSameRoom(ply) &&
            ply->isGagging(ignore->getPlayer()->getName())
        )
            continue;
        if( (   (currentLocation.room.id && ply->currentLocation.room == currentLocation.room) ||
                (currentLocation.mapmarker.getArea() != 0 && currentLocation.mapmarker == ply->currentLocation.mapmarker)
            ) &&
            sock->getFd() > -1 &&
            sock != ignore &&
            !ply->flagIsSet(P_UNCONSCIOUS) &&
            (ply->languageIsKnown(lang) || ply->isEffected("comprehend-languages")))
        {
            ply->printColor("%s", get_lang_color(lang));
            ply->vprint(fmt2, ap);
        }
    }
    va_end(ap);
}

//********************************************************************
//                      broadcastGroup
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
        if(listen->isGagging(player->getName()))
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
        if(player->isEffected("incognito") && !player->inSameRoom(listen))
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

//  broadcastGroupMember(dropLoot, player, leader, fmt, ap);
    for(Creature* crt : group->members) {
        broadcastGroupMember(dropLoot, player, crt->getAsConstPlayer(), fmt, ap);
    }
}

//**********************************************************************
//                      inetname
//**********************************************************************
// This function returns the internet address of the address structure
// passed in as the first parameter.

char *inetname(struct in_addr in) {
    char *cp=nullptr;
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

//  This causes the game to shutdown in one minute.  It is used
//  by signal to force a  shutdown in response to a HUP signal
//  (i.e. kill -HUP pid) from the system.

//**********************************************************************
//                      broadcastGuild
//**********************************************************************
// Broadcasts to everyone in the same guild as player

void broadcastGuild(int guildNum, int showName, const char *fmt,...) {
    char    fmt2[1024];
    va_list ap;

    va_start(ap, fmt);
    if(showName) {
        strcpy(fmt2, "*CC:GUILD*[");
        bstring guild = getGuildName(guildNum);
        strcat(fmt2, guild.c_str());
        strcat(fmt2, "] ");
    } else
        strcpy(fmt2, "");
    strcat(fmt2, fmt);
    strcat(fmt2, "\n");

    Player* target=nullptr;
    for(const auto& p : gServer->players) {
        target = p.second;

        if(!target->isConnected())
            continue;
        if(target->getGuild() == guildNum && target->getGuildRank() >= GUILD_PEON && !target->flagIsSet(P_NO_BROADCASTS)) {
            target->vprint(target->customColorize(fmt2).c_str(), ap);
        }
    }
    va_end(ap);

}

//*********************************************************************
//                      shutdown_now
//*********************************************************************

void shutdown_now(int sig) {
    broadcast("### Quick shutdown now!");
    gServer->processOutput();
    loge("--- Game shutdown via signal\n");
    gServer->resaveAllRooms(1);
    gServer->saveAllPly();

    std::clog << "Goodbye.\n";
    exit(0);
}

//*********************************************************************
//                      pueblo functions
//*********************************************************************

bool Pueblo::is(const bstring& txt) {
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
        if( str.getAt(i-1) == '\n' &&
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
//                      escapeText
//*********************************************************************

void Monster::escapeText() {
    if(Pueblo::is(getName()))
        setName("clay form");
    description = Pueblo::multiline(description);
}

void Player::escapeText() {
    if(Pueblo::is(description))
        description = "";
}

void Object::escapeText() {
    if(Pueblo::is(getName()))
        setName("clay ball");
    if(Pueblo::is(use_output))
        zero(use_output, sizeof(use_output));
    description = Pueblo::multiline(description);
}

void UniqueRoom::escapeText() {
    if(Pueblo::is(getName()))
        setName("New Room");

    short_desc = Pueblo::multiline(short_desc);
    long_desc = Pueblo::multiline(long_desc);

    for(Exit* exit : exits) {
        exit->escapeText();
    }
}

void Exit::escapeText() {
    if(Pueblo::is(getName()))
        setName("exit");
    if(Pueblo::is(enter))
        enter = "";
    if(Pueblo::is(open))
        open = "";
    description = Pueblo::multiline(description);
}

//*********************************************************************
//                      xsc
//*********************************************************************
// Stands for xmlspecialchars (based on htmlspecialchars for PHP) -
// escapes special characters to make the text safe for XML.
// If given Br�gal (which the xml parser cannot save), it will turn it
// into Br&#252;gal (which the xml parser can save). Display will be affected
// on old clients, so this should only be run when saving the string.

bstring xsc(std::string_view txt) {
    std::ostringstream ret;
    unsigned char c=0;
    int t = txt.length();
    for(int i=0; i<t; i++) {
        c = txt.at(i);
        // beyond 127 we get into the unsupported character range
        if(c > 127)
            ret << "&#" << c << ";";
        else
            ret << (char)c;
    }
    return(ret.str());
}

//*********************************************************************
//                      unxsc
//*********************************************************************
// Reverse of xsc - attempts to turn &#252; into �. We do this when we load from file.

bstring unxsc(std::string_view txt) {
    return(unxsc(txt));
}
bstring unxsc(const char* txt) {
    std::ostringstream ret;
    size_t c=0, len = strlen(txt);
    for(size_t i=0; i<len; i++) {
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

        ret << (char)c;
    }
    return(ret.str());
}

//*********************************************************************
//                      keyTxtConvert
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
//                      keyTxtConvert
//*********************************************************************

bstring keyTxtConvert(std::string_view txt) {
    std::ostringstream ret;
    for(int i=0; i<txt.length(); i++) {
        ret << keyTxtConvert(txt.at(i));
    }
    return(ret.str());
}

//*********************************************************************
//                      keyTxtCompare
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
//                      keyTxtEqual
//*********************************************************************

bool keyTxtEqual(const Creature* target, const char* txt) {
    int len = strlen(txt);
    return keyTxtCompare(target->key[0], txt, len) ||
           keyTxtCompare(target->key[1], txt, len) ||
           keyTxtCompare(target->key[2], txt, len) ||
           keyTxtCompare(target->getCName(), txt, len) ||
           target->getId() == txt;
}

bool keyTxtEqual(const Object* target, const char* txt) {
    int len = strlen(txt);
    return keyTxtCompare(target->key[0], txt, len) ||
           keyTxtCompare(target->key[1], txt, len) ||
           keyTxtCompare(target->key[2], txt, len) ||
           keyTxtCompare(target->getCName(), txt, len) ||
           target->getId() == txt;
}

//*********************************************************************
//                      isPrintable
//*********************************************************************
// wrapper for isprint to handle supported characters

bool isPrintable(char c) {
    return(isprint(keyTxtConvert(c)));
}
