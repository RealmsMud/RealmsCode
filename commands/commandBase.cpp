/*
 * command1.cpp
 *   Command handling/parsing routines.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <sys/stat.h>
#include <sstream>
#include <iomanip>
#include <locale>

#include "calendar.hpp"
#include "creatures.hpp"
#include "commands.hpp"
#include "config.hpp"
#include "factions.hpp"
#include "login.hpp"
#include "raceData.hpp"
#include "rooms.hpp"
#include "mud.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "xml.hpp"

//static int    newline;
//static int    color;

//#include <arpa/telnet.h>
//#define TELOPT_COMPRESS2       86


//*********************************************************************
//                      cmdNoExist
//*********************************************************************
// This function tells the player the command does not exist.

int cmdNoExist(Player* player, cmd* cmnd) {
    player->print("The command \"%s\" does not exist.\n", cmnd->str[0]);
    return(0);
}

//*********************************************************************
//                      cmdNoAuth
//*********************************************************************
// This function tells the player they're not allowed to use that command

int cmdNoAuth(Player* player) {
    player->print("You do not have the proper authorization to use that command.\n");
    return(0);
}

//*********************************************************************
//                      getFailFd
//*********************************************************************
// This function grabs the fd to print error messages to.
// A check of "fd != ffd" means messages are going to different places,
// hence fd is a pet and ffd is a player. "fd == ffd" means a
// player is running the command.

int getFailFd(Creature *user) {
    if(!user)
        return(-1);
    return(user->isPet() ? user->getMaster()->fd : user->fd);
}


//*********************************************************************
//                      command
//*********************************************************************
// This function handles the main prompt commands, and calls the
// appropriate function, depending on what service is requested by the
// player.

void command(Socket* sock, const bstring& inStr) {
    cmd cmnd;
    int n;
    Player* ply = sock->getPlayer();
    bstring str = inStr;

    ASSERTLOG( ply );

    /*
    this logn command will print out all the commands entered by players.
    It should be used in extreme cases when trying to isolate a players
    input which may be causing a crash.
    */
    //zero(&cmnd, sizeof(cmd));


    if(ply->getClass() == CreatureClass::CARETAKER && !dmIson() )
        log_immort(false, ply, "%s-%d (%s): %s\n", ply->getCName(), sock->getFd(),
            ply->getRoomParent()->fullName().c_str(), str.c_str());

    if(str == "!")
        str = ply->getLastCommand();

    if(str[0])
        ply->setLastCommand(str);


    cmnd.fullstr = str;

    stripBadChars(str); // removes '.' and '/'
    lowercize(str, 0);
    parse(str, &cmnd);


    n = 0;
    if(cmnd.num)
        n = cmdProcess(ply, &cmnd);
    else
        n = PROMPT;

    if(n == DISCONNECT) {
        sock->write("Goodbye!\n\r\n\r");
        sock->disconnect();
    } else if(n == PROMPT) {
        ply->sendPrompt();
    }

}

//#ifdef NEW_PARSER

//*********************************************************************
//                      parse
//*********************************************************************
// This function takes the string in the first parameter and breaks it
// up into its component words, stripping out useless words. The
// resulting words are stored in a command structure pointed to by the
// second argument.

void parse(const bstring str, cmd *cmnd) {
    int     i=0, j=0, l=0, n=0;
    //char  token[MAX_TOKEN_SIZE];
    bstring token;
    int     isquote=0;

    j = str.length();

    for(i=0; i<=j; i++) {

        // look for first non space or comment
        if(str[i] == ' ')
            continue;

        // ok we at first non space
        if(str[i] == '\"') {
            isquote = 1;
            // skip quote char
            i++;
        }
        // save this position as the begining of a token
        l = i;

        // now find the end of the token
        if(isquote) {
            while(str[i] != '\0' && str[i] != '\"')
                i++;

//          // terminate the token
//          if(str[i] == '\"')
//              str[i] = '\0';
        } else {
            while(str[i] != '\0' && str[i] != ' ')
                i++;

            // terminate the token
//          str[i] = '\0';
        }

        // don't overflow the buffers
        token = str.substr(l, i-l);

        /* was there any thing here? */
        if(token.empty()) {
            isquote = 0;
            continue;
        }

        if(isquote) {
            strncpy(cmnd->str[n], token.c_str(), MAX_TOKEN_SIZE);
            cmnd->str[n][MAX_TOKEN_SIZE - 1] = '\0';
            cmnd->val[n] = 1L;
            isquote = 0;
        } else {
            // Copy into command structure
            if(n == 0) {
                strncpy(cmnd->str[n], token.c_str(), MAX_TOKEN_SIZE);
                cmnd->str[n][MAX_TOKEN_SIZE - 1] = '\0';
                // set the value to 1 in case there is non following
                cmnd->val[n] = 1L;
                n++;
            }
            else if(isdigit((int)token[0]) || (token[0] == '-' &&
                    isdigit((int)token[1]))) {
                // this is a value for the previous command
                cmnd->val[MAX(0, n - 1)] = atol(token.c_str());
            } else {
                strncpy(cmnd->str[n], token.c_str(), MAX_TOKEN_SIZE);
                cmnd->str[n][MAX_TOKEN_SIZE - 1] = '\0';
                // set the value to 1 in case there is non following
                cmnd->val[n] = 1L;
                n++;
            }
        }

        if(n >= COMMANDMAX)
            break;
    }

    // set the number of tokens in the command struct
    cmnd->num = n;
}

enum HandleObject {
    Push,
    Pull,
    Press,
};

//*********************************************************************
//                      handleObject
//*********************************************************************

void handleObject(Player* player, cmd* cmnd, HandleObject type) {
    Object* object=0;
    long t = time(0);
    MudObject* target=0;

    bstring action;
    bstring action2;
    switch(type) {
        case Push:
            action = "push";
            action2 = "pushes";
            break;
        case Pull:
            action = "pull";
            action2 = "pulls";
            break;
        case Press:
            action = "press";
            action2 = "presses";
            break;
        default:
            return;
    }

    if(!player->ableToDoCommand())
        return;
    if(player->isBlind()) {
        player->printColor("^CYou're blind!\n");
        return;
    }

    if(cmnd->num > 1) {
        target = player->findTarget(FIND_OBJ_INVENTORY | FIND_OBJ_EQUIPMENT | FIND_OBJ_ROOM,
            player->displayFlags(), cmnd->str[1], cmnd->val[1]);

        if(target != nullptr)
            object = dynamic_cast<Object*>(target);
    }

    if(!object) {
        player->print("You don't see that here.\n");
        return;
    }

    // If it's press or push, and the object is a combo lock, do that instead
    if((type == Push || type == Press) && object->getSpecial() == SP_COMBO) {
        object->doSpecial(player);
        return;
    }

    // press can only be used for combo locks.
    if((type == Push || type == Pull) && object->flagIsSet(O_PUSH_PULL_SPRINGS_TRAP)) {

        if(player->lasttime[LT_PLAYER_STUNNED].ltime+(player->lasttime[LT_PLAYER_STUNNED].interval) > t) {
            player->pleaseWait((player->lasttime[LT_PLAYER_STUNNED].interval + 1) - t + player->lasttime[LT_PLAYER_STUNNED].ltime - 1);
            return;
        }

        player->printColor("You %s %P^x.\n", action.c_str(), object);
        broadcast(player->getSock(), player->getParent(), "%M %s %P^x.", player, action2.c_str(), object);

        if(player->checkTraps(player->getUniqueRoomParent(), true, false))
            player->stun(3);
        return;
    }

    // other actions failed
    player->print("You can't %s that.\n", action.c_str());
}

//*********************************************************************
//                      cmdPush
//*********************************************************************

int cmdPush(Player* player, cmd* cmnd) {
    handleObject(player, cmnd, Push);
    return(0);
}

//*********************************************************************
//                      cmdPull
//*********************************************************************

int cmdPull(Player* player, cmd* cmnd) {
    handleObject(player, cmnd, Pull);
    return(0);
}

//*********************************************************************
//                      cmdPress
//*********************************************************************

int cmdPress(Player* player, cmd* cmnd) {
    handleObject(player, cmnd, Press);
    return(0);
}


//*********************************************************************
//                      doFinger
//*********************************************************************
// sending 0 to cls means we're not a player and we want reduced padding

bstring doFinger(const Player* player, bstring name, CreatureClass cls) {
    struct stat f_stat;
    char    tmp[80];
    Player* target=0;
    std::ostringstream oStr;
    bool online=true;

    // set left aligned
    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr.imbue(std::locale(""));

    if(name == "")
        return("Finger who?\n");

    name = name.toLower();
    name.setAt(0, up(name.getAt(0)));
    target = gServer->findPlayer(name);

    if(!target) {
        if(!loadPlayer(name.c_str(), &target))
            return("Player does not exist.\n");

        online = false;
    }

    if(target->isStaff() && cls < target->getClass()) {
        if(!online)
            free_crt(target);
        return("You are currently unable to finger that player.\n");
    }

    // cls=CreatureClass::NONE means we don't want padding
    if(cls == CreatureClass::NONE) {
        oStr << std::setw(25) << target->getName() << " "
             << std::setw(15) << gConfig->getRace(target->getDisplayRace())->getName();
        // will they see through the illusion?
        if(player && player->willIgnoreIllusion() && target->getDisplayRace() != target->getRace())
            oStr << " (" << gConfig->getRace(target->getRace())->getName() << ")";
        oStr << " "
             << target->getTitle() << "\n";
    } else {
        oStr << target->getName() << " the "
             << gConfig->getRace(target->getDisplayRace())->getAdjective();
        // will they see through the illusion?
        if(player && player->willIgnoreIllusion() && target->getDisplayRace() != target->getRace())
            oStr << " (" << gConfig->getRace(target->getRace())->getAdjective() << ")";
        oStr << " "
             << target->getTitle() << "\n";
    }

    sprintf(tmp, "%s/%s.txt", Path::Post, name.c_str());
    if(stat(tmp, &f_stat))
        oStr << "No mail.\n";
    else if(f_stat.st_atime > f_stat.st_mtime)
        oStr << "No unread mail since: " << ctime(&f_stat.st_atime);
    else
        oStr << "New mail since: " << ctime(&f_stat.st_mtime);

    if(target->getForum() != "")
        oStr << "Forum account: " << target->getForum() << "\n";

    if(online) {
        oStr << "Currently logged on.\n";
    } else {
        long t = target->getLastLogin();
        free_crt(target, false);
        oStr << "Last login: " << ctime(&t);
    }

    return(oStr.str());
}

//*********************************************************************
//                      cmdFinger
//*********************************************************************

int cmdFinger(Player* player, cmd* cmnd) {
    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    player->print("%s", doFinger(player, cmnd->str[1], player->getClass()).c_str());
    return(0);
}


//*********************************************************************
//                      cmdPayToll
//*********************************************************************
// This function will allow players to pay to get through exits. Two
// flags are used for it: X_TOLL_TO_PASS, and X_LEVEL_BASED_TOLL. X_TOLL_TO_PASS sets the exit as
// a toll booth. A toll field will be required in the Exit structure
// defined as a long. X_LEVEL_BASED_TOLL will make the toll vary depending on the
// players' level, making the cost equal exit->toll*player->getLevel().

int cmdPayToll(Player* player, cmd* cmnd) {
    Monster* target=0;
    BaseRoom    *newRoom=0;
    Exit    *exit=0;
    unsigned long tc=0, amt=0;

    if(cmnd->num < 4) {
        player->print("Pay what to whom to get in where?\n");
        player->print("Syntax: pay $(amount) (target) (exit name)\n");
        return(0);
    }

    if(cmnd->num < 3) {
        player->print("Pay to get in where?\n");
        player->print("Syntax: pay $(amount) (target) (exit name)\n");
        return(0);
    }


    if(cmnd->str[1][0] != '$') {
        player->print("Syntax: pay $(amount) (target) (exit name)\n");
        return(0);
    }

    amt = atol(&cmnd->str[1][1]);
    if(!amt || amt < 1) {
        player->print("Please enter the amount to pay the tollkeeper.\n");
        return(0);
    }

    amt = MIN<unsigned long>(amt, 30000);

    target = player->getParent()->findMonster(player, cmnd, 2);
    if(!target) {
        player->print("That creature is not here.\n");
        return(0);
    }

    if(!Faction::willDoBusinessWith(player, target->getPrimeFaction())) {
        player->print("%M refuses to do business with you.\n", target);
        return(0);
    }

    if(!target->flagIsSet(M_TOLLKEEPER)) {
        player->print("%M doesn't take toll payments.\n", target);
        return(0);
    }

    exit = findExit(player, cmnd, 3);
    if(!exit) {
        player->print("That exit is not here.\n");
        return(0);
    }
    if(!exit->flagIsSet(X_TOLL_TO_PASS)) {
        player->print("That is not a tolled exit.\n");
        return(0);
    }


    tc = tollcost(player, exit, target);


    if(player->coins[GOLD] < amt) {
        player->print("You do not have that much gold on you.\n");
        return(0);
    }

    if(amt < tc) {
        player->printColor("You must pay at least %ld gold coins to pass through the %s^x.\n", tc, exit->getCName());
        return(0);
    }

    if(amt > tc) {
        player->printColor("That is too much. The cost to pass through %s^x is %ld gold coins.\n", exit->getCName(), tc);
        return(0);
    }

    newRoom = exit->target.loadRoom(player);
    if(!newRoom) {
        player->printColor("The %s^x appears to be jammed shut.\nPlease try again later.\n", exit->getCName());
        return(0);
    }

    UniqueRoom* uRoom = newRoom->getAsUniqueRoom();
    if(uRoom && !player->canEnter(uRoom, true))
        return(0);

    player->coins.sub(amt, GOLD);
    gServer->logGold(GOLD_OUT, player, Money(amt, GOLD), exit, "Toll");

    player->printColor("%M accepts your toll and ushers you through the %s^x.\n", target, exit->getCName());
    broadcast(player->getSock(), player->getParent(), "%M pays %N some coins and goes through the %s^x.", player, target, exit->getCName());

    player->deleteFromRoom();
    player->addToRoom(newRoom);
    player->doPetFollow();
    return(0);
}

//*********************************************************************
//                      tollcost
//*********************************************************************

unsigned long tollcost(const Player* player, const Exit* exit, Monster* keeper) {
    unsigned long cost = exit->getToll() ? exit->getToll() : DEFAULT_TOLL;
    if(!player)
        return(cost);
    if(exit->flagIsSet(X_LEVEL_BASED_TOLL))
        cost = cost * player->getLevel() * 2;

    if(!keeper)
        keeper = player->getConstRoomParent()->getTollkeeper();
    if(keeper) {
        Money money;
        money.set(cost, GOLD);
        money = Faction::adjustPrice(player, keeper->getPrimeFaction(), money, true);
        cost = money[GOLD];
    }
    return(cost);
}

//*********************************************************************
//                      infoGamestat
//*********************************************************************

int infoGamestat(Player* player, cmd* cmnd) {
    int     players=0, immorts=0;
    int     daytime=0;
    long    t=0, days=0, hours=0, minutes=0;
    char    *str;

    Player* target=0;
    for(std::pair<bstring, Player*> p : gServer->players) {
        target = p.second;

        if(!target->isConnected())
            continue;

        if(target->isStaff())
            immorts++;
        if(!target->isStaff())
            players++;
    }

    player->print("\nCurrent Realms Gamestat Information\n\n");

    t = time(0);
    daytime = gConfig->currentHour();

    days = (t - StartTime) / (60*60*24);
    hours = (t - StartTime) / (60*60);
    hours %= 24;
    minutes = (t - StartTime) / 60L;
    minutes %= 60;

    player->printColor("^cGame-Time: %d:%02d %s.  %s.\n",
        gConfig->currentHour(true),
        gConfig->currentMinutes(),
        daytime > 11 ? "PM" : "AM",
        isDay() ? "It is day" : "It is night"
    );

    if(player->isCt())
        player->printColor("^gThe mud has been running for %d game days.\n", gConfig->calendar->getTotalDays());


    player->printColor("^MReal-Time: %s.\n", gServer->getServerTime().c_str());

    if(!days)
        player->printColor("^RRealms Uptime: %02ld:%02ld:%02ld\n", hours, minutes, (t - StartTime) % 60L);
    else if(days == 1)
        player->printColor("^RRealms Uptime: %ld day %02ld:%02ld:%02ld\n", days, hours, minutes, (t - StartTime) % 60L);
    else
        player->printColor("^RRealms Uptime: %ld days %02ld:%02ld:%02ld\n", days, hours, minutes, (t - StartTime) % 60L);

    player->printColor("^yTotal players currently online: %d\n", players);
    if(player->isDm())
        player->printColor("^yTotal staffs online: %d\n", immorts);

    player->print("\n");

    return(0);
}

//*********************************************************************
//                      cmdDescription
//*********************************************************************
// this allows a player to set his/her description that is seen when you
// look at them.

int cmdDescription(Player* player, cmd* cmnd) {
    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num < 2) {
        player->print("Syntax: description [text|-d]\n.");
        return(0);
    }
    if(!strcmp(cmnd->str[1], "-d")) {
        player->print("Description cleared.\n");
        player->setDescription("");
        return(0);
    }

    player->setDescription(getFullstrText(cmnd->fullstr, 1));
    player->escapeText();
    player->print("Description set.\n");
    return(0);
}
