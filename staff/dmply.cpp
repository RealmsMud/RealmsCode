/*
 * dmply.cpp
 *   Staff functions related to players
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

#include <fmt/format.h>              // for format
#include <strings.h>                 // for strcasecmp
#include <sys/stat.h>                // for stat
#include <unistd.h>                  // for unlink, link
#include <cctype>                    // for isspace, isalpha, isdigit
#include <cstdio>                    // for sprintf, snprintf, fclose, fopen
#include <cstdlib>                   // for atoi
#include <cstring>                   // for strcmp, strlen, strcpy, strstr
#include <ctime>                     // for time, ctime
#include <map>                       // for operator==, _Rb_tree_iterator, map
#include <ostream>                   // for operator<<, basic_ostream, ostri...
#include <string>                    // for string, allocator, char_traits
#include <string_view>               // for string_view
#include <utility>                   // for pair

#include "area.hpp"                  // for MapMarker, Area (ptr only)
#include "bank.hpp"                  // for Bank
#include "catRef.hpp"                // for CatRef
#include "cmd.hpp"                   // for cmd
#include "color.hpp"                 // for escapeColor
#include "commands.hpp"              // for getFullstrText, cmdNoAuth, cmdNo...
#include "config.hpp"                // for Config, gConfig
#include "enums/loadType.hpp"        // for LoadType, LoadType::LS_BACKUP
#include "flags.hpp"                 // for P_SPYING, P_BUGGED, P_CANT_BROAD...
#include "global.hpp"                // for PROMPT, MAX_STAT_NUM, CreatureClass
#include "group.hpp"                 // for operator<<, Group, GROUP_INVITED
#include "guilds.hpp"                // for Guild
#include "lasttime.hpp"              // for lasttime
#include "location.hpp"              // for Location
#include "login.hpp"                 // for PASSWORD_MAX_LENGTH, PASSWORD_MI...
#include "magic.hpp"                 // for MAXSPELL, S_ANIMIATE_DEAD, S_JUD...
#include "money.hpp"                 // for Money, GOLD
#include "mud.hpp"                   // for LT_NO_BROADCAST, LT_RP_AWARDED, LT
#include "mudObjects/container.hpp"  // for PlayerSet, Container
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/objects.hpp"    // for Object, ObjectType, ObjectType::...
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "paths.hpp"                 // for Config, Player, PlayerBackup, Post
#include "proto.hpp"                 // for broadcast, log_immort, up, lower...
#include "random.hpp"                // for Random
#include "realm.hpp"                 // for Realm, MAX_REALM, MIN_REALM
#include "server.hpp"                // for Server, gServer, PlayerMap
#include "socket.hpp"                // for Socket
#include "stats.hpp"                 // for Stat
#include "unique.hpp"                // for addOwner, deleteOwner
#include "web.hpp"                   // for callWebserver
#include "xml.hpp"                   // for loadPlayer, loadRoom
#include "toNum.hpp"

class UniqueRoom;



//*********************************************************************
//                      dmLastCommand
//*********************************************************************

std::string dmLastCommand(const std::shared_ptr<const Player> &player) {
    if(player->isPassword(player->getLastCommand()))
        return("**********");
    return(escapeColor(player->getLastCommand()));
}


//*********************************************************************
//                      dmForce
//*********************************************************************
// This function allows a DM to force another user to do a command.

int dmForce(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    int     index=0;
    std::string::size_type i=0;
    char    str[IBUFSIZE+1];

    if(cmnd->num < 2) {
        player->print("*force whom?\n");
        return(0);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);
    if(!target) {
        player->print("%s is not on.\n", cmnd->str[1]);
        return(0);
    }
    if(target == player) {
        player->print("You can't *force yourself!\n");
        return(0);
    }

    if(!target->getSock()->canForce()) {
        player->print("Can not force %s right now.\n", cmnd->str[1]);
        return(0);
    }

    for(i=0; i<cmnd->fullstr.length(); i++)
        if(cmnd->fullstr[i] == ' ') {
            index = i+1;
            break;
        }
    for(i=index; i<cmnd->fullstr.length(); i++)
        if(cmnd->fullstr[i] != ' ') {
            index = i+1;
            break;
        }
    for(i=index; i<cmnd->fullstr.length(); i++)
        if(cmnd->fullstr[i] == ' ') {
            index = i+1;
            break;
        }
    for(i=index; i<cmnd->fullstr.length(); i++)
        if(cmnd->fullstr[i] != ' ') {
            index = i;
            break;
        }
    strcpy(str, &cmnd->fullstr[index]);
    if(target->getClass() >= player->getClass())
        target->print("%s forces you to \"%s\".\n", player->getCName(), str);

    std::string txt = escapeColor(str);
    log_immort(true, player, "%s forced %s to \"%s\".\n", player->getCName(), target->getCName(), txt.c_str());
    command(target->getSock(), str);

    return(0);
}


//*********************************************************************
//                      dmSpy
//*********************************************************************

int dmSpy(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;


    if(cmnd->num < 2 && !player->flagIsSet(P_SPYING)) {
        player->print("Spy on whom?\n");
        return(0);
    }

    if(player->flagIsSet(P_SPYING)) {
        player->getSock()->clearSpying();
        player->clearFlag(P_SPYING);
        player->print("Spy mode off.\n");
        if(!player->isDm())
            log_immort(false,player, "%s turned spy mode off.\n", player->getCName());
        return(0);
    }

    cmnd->str[1][0] = up(cmnd->str[1][0]);
    target = gServer->findPlayer(cmnd->str[1]);
    if(!target || !player->canSee(target)) {
        player->print("Spy on whom?  Use full names.\n");
        return(0);
    }

    if(target->isDm() && !player->isDm()) {
        player->print("You cannot spy on DM's!\n");
        target->printColor("^r%M tried to spy on you.\n", player.get());
        return(0);
    }

    if(target == player) {
        player->print("You can't spy on yourself.\n");
        return(0);
    }

    if(player->getClass() <= target->getClass())
        target->printColor("^r%s is observing you.\n", player->getCName());

    player->getSock()->setSpying(target->getSock());

    player->setFlag(P_SPYING);
    player->setFlag(P_DM_INVIS);

    player->print("Spy on. Type *spy to turn it off.\n");
    if(!player->isDm())
        log_immort(false,player, "%s started spying on %s.\n", player->getCName(), target->getCName());
    return(0);
}


//*********************************************************************
//                      dmSilence
//*********************************************************************
// This function sets it so a player can't broadcast for 10 minutes or
// however long specified by [minutes]

int dmSilence(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> target=nullptr;


    if(cmnd->num < 2) {
        player->print("syntax: *silence <player> [minutes]/[r]\n");
        return(0);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target || !player->canSee(target)) {
        player->print("That player is not logged on.\n");
        return(0);
    }

    target->print("You have been silenced by the gods.\n");
    target->lasttime[LT_NO_BROADCAST].ltime = time(nullptr);


    if(cmnd->num > 2 && low(cmnd->str[2][0]) == 'r') {
        if(target->flagIsSet(P_CANT_BROADCAST)) {
            player->print("%s can now broadcast again.\n", target->getCName());
            target->clearFlag(P_CANT_BROADCAST);
        } else {
            player->print("But %s can already broadcast!\n", target->getCName());
        }
    } else {
        if(cmnd->val[1]) {
            target->lasttime[LT_NO_BROADCAST].interval = cmnd->val[1] * 60L;
            player->print("%s silenced for %d minute(s).\n", target->getCName(), cmnd->val[1]);
        } else {
            player->print("%s silenced for 10 minutes.\n", target->getCName());
            target->lasttime[LT_NO_BROADCAST].interval = 600L;
        }
        target->setFlag(P_CANT_BROADCAST);
        log_immort(true, player, "%s removed %s's ability to broadcast.\n", player->getCName(), target->getCName());
    }
    return(0);
}


//*********************************************************************
//                      dmTitle
//*********************************************************************

int dmTitle(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    std::string title = "";

    if(cmnd->num < 3) {
        player->print("\nSyntax: *title <player> [<title>|-d]\n");
        return(PROMPT);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target || !player->canSee(target)) {
        player->print("Player not found.\n");
        return(0);
    }

    if(!player->isDm() && target != player) {
        player->print("You can only set your own title.\n");
        return(0);
    }

    title = getFullstrText(cmnd->fullstr, 2);

    if(title == "-d") {
        target->setTitle("");
        player->print("\nTitle cleared.\n");
    } else {
        title = escapeColor(title);
        target->setTitle(title);
        player->print("\nTitle Set\n");
    }
    return(0);
}

//*********************************************************************
//                      dmSurname
//*********************************************************************

int dmSurname(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    int     i=0;
    char    which=0;

    if(cmnd->num < 3) {
        player->print("\nSyntax: *surname (player) [(surname)|-d|-l]\n");
        return(PROMPT);
    }

    // Parse the flag here
    while(isspace(cmnd->fullstr[i]))
        i++;
    while(!isspace(cmnd->fullstr[i]))
        i++;
    while(isspace(cmnd->fullstr[i]))
        i++;
    while(isalpha(cmnd->fullstr[i]))
        i++;
    while(isspace(cmnd->fullstr[i]))
        i++;

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target || !player->canSee(target)) {
        player->print("Player not found.\n");
        return(PROMPT);
    }

    if(!player->isDm() && target->isStaff()) {
        player->print("You can only set the surnames of players.\n");
        return(0);
    }



    while(isdigit(cmnd->fullstr[i]))
        i++;
    while(isspace(cmnd->fullstr[i]))
        i++;

    // parse flag
    if(cmnd->fullstr[i] == '-') {
        if(cmnd->fullstr[i + 1] == 'd') {
            which = 1;
            i += 2;
        } else if(cmnd->fullstr[i + 1] == 'l') {
            which = 2;
            i += 2;
        }
        while(isspace(cmnd->fullstr[i]))
            i++;
    }



    switch (which) {
    case 0:
        target->setSurname(&cmnd->fullstr[i]);
        player->print("\nSurname set\n");
        target->setFlag(P_CHOSEN_SURNAME);
        break;
    case 1:
        target->setSurname("");
        player->print("\nSurname cleared.\n");
        target->clearFlag(P_CHOSEN_SURNAME);
        break;
    case 2:
        target->setSurname("");
        player->print("\n%s's surname command is now locked.\n", target->getCName());
        target->setFlag(P_NO_SURNAME);
        break;
    default:
        player->print("\nNothing Done.\n");
        break;
    }
    return(0);
}


//*********************************************************************
//                      dmGroup
//*********************************************************************
// This function allows you to see who is in a group or party of people
// who are following you.

int dmGroup(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature>target=nullptr;

    if(cmnd->num < 2) {
        player->printColor("Active Groups: \n%s", gServer->getGroupList().c_str());
        return(PROMPT);
    }

    target = player->getParent()->findCreature(player, cmnd);

    if(!target) {
        lowercize(cmnd->str[1], 1);
        target = gServer->findPlayer(cmnd->str[1]);
    }

    if(!target || !player->canSee(target)) {
        player->print("That player is not logged on.\n");
        return(PROMPT);
    }

    std::ostringstream oStr;

    Group* group = target->getGroup(false);
    if(group) {
        if(target->getGroupStatus() == GROUP_INVITED) {
            oStr << "Invited to join " << group->getName() << " (" << group->getLeader()->getName() << ")." << std::endl;
        } else {
            switch(target->getGroupStatus()) {
                case GROUP_MEMBER:
                    oStr << "Member of ";
                    break;
                case GROUP_LEADER:
                    oStr << "Leader of ";
                    break;
                default:
                    oStr << "Unknown in ";
                    break;
            }
            oStr << group;
        }
    } else {
        player->print("%M is not a member of a group.\n", target.get());
    }
    return(0);
}

//*********************************************************************
//                      dmDust
//*********************************************************************
// This function allows staff to delete a player.

int dmDust(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    char    buf[120];

    if(cmnd->num < 2) {
        player->print("\nDust whom?\n");
        return(PROMPT);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target || !player->canSee(target)) {
        player->print("%s is not on.\n", cmnd->str[1]);
        return(0);
    }

    if(target->isCt() && !player->isDm()) {
        target->printColor("^r%s tried to dust you!\n", player->getCName());
        return(0);
    }

    if(player->getClass() == CreatureClass::CARETAKER && target->isStaff()) {
        target->printColor("^r%s tried to dust you!\n", player->getCName());
        return(0);
    }

    if(isdm(target->getCName())) {
        target->printColor("^r%s tried to dust you!\n", player->getCName());
        return(0);
    }

    if(player->getClass() == CreatureClass::CARETAKER && target->getLevel() > 10) {
        player->print("You are only able to dust people levels 10 and below.\n");
        return(0);
    }

    logn("log.dust", "%s was dusted by %s.\n", target->getCName(), player->getCName());

    // TODO: Handle guild creations
    sprintf(buf, "\n%c[35mLightning comes down from on high! You have angered the gods!%c[35m\n", 27, 27);
    target->getSock()->write(buf);

    if(!strcmp(cmnd->str[2], "-n")) {
        broadcast(isCt, "^y### %s has been turned to dust!", target->getCName());
    } else {
        broadcast("### %s has been turned to dust!", target->getCName());
        broadcast(target->getSock(), target->getRoomParent(), "A bolt of lightning strikes %s from on high.", target->getCName());
        last_dust_output = time(nullptr) + 15L;
    }

    target->deletePlayer();
    return(0);
}

//*********************************************************************
//                      dmFlash
//*********************************************************************
//  This function allows a DM to output a string to an individual
//  players screen.

int dmFlash(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::string text = "";
    std::shared_ptr<Player> target=nullptr;


    if(cmnd->num < 2) {
        player->print("DM flash to whom?\n");
        return(0);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target) {
        player->print("Send to whom?\n");
        return(0);
    }

    text = getFullstrText(cmnd->fullstr, 2);
    if(text.empty()) {
        player->print("Send what?\n");
        return(0);
    }

    player->printColor("^cYou flashed: \"%s^c\" to %N.\n", text.c_str(), target.get());

    target->printColor("%s\n", text.c_str());
    return(0);
}

//*********************************************************************
//                      dmAward
//*********************************************************************

int dmAward(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target;
    long    i=0, t=0, amount=0, gp=0;
    char    temp[80];


    if(!(player->isDm() || (player->flagIsSet(P_WATCHER) && !player->isStaff()) || (player->getClass() == CreatureClass::CARETAKER && player->flagIsSet(P_CAN_AWARD)) )) {
        player->print("The command \"%s\" does not exist.\n", cmnd->str[0]);
        return(0);
    }

    if(cmnd->num < 2) {
        player->printColor("^cSyntax: *award (player) %s\n", player->isCt() ? "[xp amount] [-g] [gold amount]":"");
        player->printColor("^c-Default award amount is 1 percent of player's current xp, or 500xp, whichever is higher.\n");
        if (player->isCt())
            player->printColor("^c-Default gold amount is 1 percent of player's current xp, or 500gp, whichever is higher.\n");
        return(PROMPT);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target || !player->canSee(target)) {
        player->print("%s is not on.\n", cmnd->str[1]);
        return(0);
    }

    if(target == player) {
        player->print("You can't *award yourself! Don't be such a narcissist.\n");
        return(0);
    }

    if(target->isStaff()) {
        player->print("%s's toil is %s reward!\n", target->getCName(), target->hisHer());
        return(0);
    }

    if(!player->isCt() && player->isWatcher() && target->getLevel() > 16) {
        player->print("Only DMs and CTs can *award players greater than level 16.\n");
        player->print("Please let them know so they can *award %s.\n", target->getCName());
        return(0);
    }

     if(!strcmp(cmnd->str[2], "-g") && cmnd->val[2] < 0 && player->isCt()) {
        player->printColor("^RDon't be a fool. You can't award negative gold.\n");
        return(0);
    }

    sprintf(temp, "%s", target->getCName());

    if(!target->getSock()->canForce()) {
        player->print("You can't award %s right now.\n", cmnd->str[1]);
        return(0);
    }

    if (player->isCt()) // Setting specific xp award is restricted to CT and DM only.
        amount = cmnd->val[1];


    if(amount < 2) {
        amount = target->getExperience() / 100;     // 1% of current xp is default award.
        amount = std::max(amount, 500L);                 // or 500XP if it's higher
    }

    i = LT(target, LT_RP_AWARDED);
    t = time(nullptr);

    if(i > t && !player->isDm()) { // DMs ignore timer
        if(i - t > 3600)
            player->print("%s cannot be awarded for %02d:%02d:%02d more hours.\n", target->getCName(),(i - t) / 3600L, ((i - t) % 3600L) / 60L, (i - t) % 60L);
        else if((i - t > 60) && (i - t < 3600))
            player->print("%s cannot be awarded for %d:%02d more minutes.\n", target->getCName(), (i - t) / 60L, (i - t) % 60L);
        else
            player->pleaseWait(i-t);
        return(0);
    }

    target->lasttime[LT_RP_AWARDED].ltime = t;
    if(player->isCt())
        target->lasttime[LT_RP_AWARDED].interval = 300L; // CTs/DMs always set the timer to 5 minutes
    // We have a watcher
    else if (target->getLevel() < 10)
        target->lasttime[LT_RP_AWARDED].interval = 900L; // 15 minutes between target awards if under level 10
    else 
        target->lasttime[LT_RP_AWARDED].interval = 3600L; // everybody else 1 hour between
    
    player->printColor("^c%ld xp awarded to %s for good roleplaying and/or being greatly helpful.\n", amount, target->getCName());
    target->printColor("^GYou have been awarded %ld xp for good roleplaying and/or being greatly helpful!\n", amount);
    if(!player->isCt())
        logn("log.watchers", "%s awarded %ld xp to %s[L%d].\n", player->getCName(), amount, target->getCName(), target->getLevel());
    target->addExperience(amount);

    if(!strcmp(cmnd->str[2], "-g") && player->isCt()) { // Watchers cannot award gold
        if(cmnd->val[2] > 0)
            gp = std::max(500L, std::min(cmnd->val[2], player->isDm() ? 5000000L:500000L));
        else
            gp = std::max(500L, std::min(player->isDm() ? 5000000L:500000L, amount));
        player->printColor("^cYou also awarded ^y%ld gold^c.\n", gp);
        target->printColor("^GYou have been awarded ^Y%ld gold^G as well! It was put in your bank account.\n", gp);
        target->bank.add(gp, GOLD);
        logn("log.bank", "%s was awarded %ld gold for good roleplaying and/or being greatly helpful. (Balance=%s)\n",target->getCName(), gp, target->bank.str().c_str());
        Bank::log(target->getCName(), "ROLEPLAY/HELPFUL AWARD: %ld [Balance: %s]\n", gp, target->bank.str().c_str());
    }

    if (!player->isCt())
        broadcast(isCt, "^y*** %s awarded %s[L%d] %ld xp for good roleplaying and/or being greatly helpful.\n", player->getCName(), temp, target->getLevel(), amount);
    log_immort(true, player, "%s awarded %s %ld xp for good roleplaying and/or being greatly helpful.\n", player->getCName(), temp, amount);

    if(!strcmp(cmnd->str[2], "-g"))
        log_immort(true, player, "%s was awarded %ld gold for good roleplaying and/or being greatly helpful. (Balance=%s)\n",target->getCName(), gp, target->bank.str().c_str());

    return(0);
}


//*********************************************************************
//                      dmBeep
//*********************************************************************
int dmBeep(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;

    if(cmnd->num < 2) {
        player->print("\n*Beep whom?\n");
        return(PROMPT);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target) {
        player->print("%s is not on.\n", cmnd->str[1]);
        return(0);
    }

    if(!target->getSock()->canForce()) {
        player->print("Can't beep %s right now.\n", cmnd->str[1]);
        return(0);
    }

    target->print("\a\a\a\a\a\a\a\a\a\a");
    return(0);
}


//*********************************************************************
//                      dmAdvance
//*********************************************************************

int dmAdvance(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    int     lev=0, i=0;

    if(cmnd->num < 2) {
        player->print("Advance who at what level?\n");
        return(0);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target) {
        player->print("Advance whom?\n");
        return(0);
    }

    if(target->getNegativeLevels()) {
        player->print("Clear %s's negative levels first.\n", target->getCName());
        return(0);
    }
    if(cmnd->val[1] <= 0 || cmnd->val[1] > MAXALVL) {
        player->print("Only levels between 1 and %d!\n", MAXALVL);
        return(0);
    }
    lev = cmnd->val[1] - target->getLevel();
    if(lev == 0) {
        player->print("But %N is already level %d!\n", target.get(), target->getLevel());
        return(0);
    }
    if(lev > 0) {
        for(i = 0; i < lev; i++)
            target->upLevel();
        player->print("%M has been raised to level %d.\n", target.get(), target->getLevel());
    }
    if(lev < 0) {
        for(i = 0; i > lev; i--)
            target->downLevel();
        player->print("%M has been lowered to level %d.\n", target.get(), target->getLevel());
    }
    if(target->getLevel() > 1)
        target->setExperience(Config::expNeeded(target->getLevel()-1)+1);
    else
        target->setExperience(0);
    return(0);
}

//*********************************************************************
//                      dmFinger
//*********************************************************************

int dmFinger(const std::shared_ptr<Player>& player, cmd* cmnd) {
    struct stat     f_stat{};
    std::shared_ptr<Player> target=nullptr;
    char    tmp[80];

    if(cmnd->num < 2) {
        player->print("Dmfinger who?\n");
        return(0);
    }

    cmnd->str[1][0] = up(cmnd->str[1][0]);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target) {

        if(!loadPlayer(cmnd->str[1], target)) {
            player->print("Player does not exist.\n");
            return(0);
        }

        player->print("\n");
        target->information(player, false);

    } else {
        player->print("\n");
        target->information(player, true);
    }

    sprintf(tmp, "%s/%s", Path::Post.c_str(), cmnd->str[1]);
    if(stat(tmp, &f_stat)) {
        player->print("No mail.\n");
        return(PROMPT);
    }

    if(f_stat.st_atime > f_stat.st_ctime)
        player->print("No unread mail since: %s", ctime(&f_stat.st_atime));
    else
        player->print("New mail since: %s", ctime(&f_stat.st_ctime));
    return(PROMPT);
}

//*********************************************************************
//                      dmDisconnect
//*********************************************************************

int dmDisconnect(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> creature=nullptr;

    if(cmnd->num < 2) {
        player->print("\nDisconnect whom?\n");
        return(PROMPT);
    }

    lowercize(cmnd->str[1], 1);
    creature = gServer->findPlayer(cmnd->str[1]);

    if(!creature || !player->canSee(creature)) {
        player->print("%s is not on.\n", cmnd->str[1]);
        return(0);
    }
    if(creature->isCt() && !player->isDm()) {
        creature->printColor("^r%s tried to disconnect you!\n", player->getCName());
        return(0);
    }

    log_immort(true, player, "%s disconnected %s.\n", player->getCName(), creature->getCName());

    creature->getSock()->disconnect();
    return(0);

}

//*********************************************************************
//                      dmTake
//*********************************************************************
// This function allows staff to steal items from a player remotely.

int dmTake(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    std::shared_ptr<Object> object=nullptr, container=nullptr;
    bool    online=false;

    if(cmnd->num < 2) {
        player->print("*take what?\n");
        return(0);
    }

    if(cmnd->num < 3) {
        player->print("*take what from whom?\n");
        return(0);
    }


    cmnd->str[2][0]=up(cmnd->str[2][0]);
    target = gServer->findPlayer(cmnd->str[2]);


    if(!target || !player->canSee(target)) {
        if(player->isDm()) {
            if(!loadPlayer(cmnd->str[2], target)) {
                player->print("Player does not exist.\n");
                return(0);
            }
        } else {
            player->print("That player is not logged on.\n");
            return(0);
        }
    } else
        online = true;



    if(cmnd->num > 3) {

        container = target->findObject(player, cmnd, 3);

        if(!container) {
            player->print("%s doesn't have that container.\n", target->upHeShe());
            return(0);
        }

        if(container->getType() != ObjectType::CONTAINER) {
            player->print("That isn't a container.\n");
            return(0);
        }

        object = container->findObject(player, cmnd, 1);
        if(!object) {
            player->print("That is not in that container.\n");
            return(0);
        }

        container->delObj(object);
        Limited::deleteOwner(target, object);
        // don't need to run addUnique on staff
        player->addObj(object);

        player->printColor("You remove a %P from %N's %s.\n", object.get(), target.get(), container->getCName());

        if(!player->isDm())
            log_immort(true, player, "%s removed %s from %s's %s.\n", player->getCName(), object->getCName(), target->getCName(), container->getCName());

        target->save(online);
        return(0);
    }


    object = target->findObject(player, cmnd, 1);

    if(!object) {
        player->print("%s doesn't have that.\n", target->upHeShe());
        return(0);
    }


    player->printColor("You remove %P from %N's inventory.\n", object.get(), target.get());

    if(!player->isDm())
        log_immort(true, player, "%s removed %s from %s's inventory.\n", player->getCName(), object->getCName(), target->getCName());

    target->delObj(object, false, true, true, true, true);
    // don't need to run addUnique on staff
    player->addObj(object);

    target->save(online);
    return(0);
}

//*********************************************************************
//                      dmRemove
//*********************************************************************
// This function allows staff to unwield/unwear items from a player remotely.

int dmRemove(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    std::shared_ptr<Object> object=nullptr;
    int     i=0, j=0, found=0;

    if(cmnd->num < 2) {
        player->print("*remove what?\n");
        return(0);
    }
    if(cmnd->num < 3) {
        player->print("*remove what from whom?\n");
        return(0);
    }

    cmnd->str[2][0] = up(cmnd->str[2][0]);
    target = gServer->findPlayer(cmnd->str[2]);


    if(!target || !player->canSee(target)) {
        player->print("That player is not logged on.\n");
        return(0);
    }


    for(i=0; i<MAXWEAR; i++)
        if(target->ready[i])
            found = 1;

    if(!found) {
        player->print("%s isn't wearing anything.\n", target->upHeShe());
        return(0);
    }



    for(i=0,j=0; i<MAXWEAR; i++) {
        if(target->ready[i] && keyTxtEqual(target->ready[i], cmnd->str[1])) {
            j++;
            if(j == cmnd->val[1]) {
                object = target->ready[i];
                if(target->inCombat() && object->getType() == ObjectType::WEAPON) {
                    player->print("Not while %s is in combat.\n", target->upHeShe());
                    return(0);
                }
                // i is wearloc-1, so add 1  -- We might be add it to someone else's
                // inventory, so do nothing afterwards
                target->unequip(i+1, UNEQUIP_NOTHING);
                Limited::deleteOwner(target, object);
                target->computeAttackPower();
                target->computeAC();
                break;
            }
        }
    }



    if(!object) {
        player->print("%s isn't wearing/wielding that.\n", target->upHeShe());
        return(0);
    }


    player->printColor("You remove %P from %N's worn equipment.\n", object.get(), target.get());

    if(!player->isDm())
        log_immort(true, player, "%s removed %s from %s's worn equipment.\n", player->getCName(), object->getCName(), target->getCName());

    player->addObj(object);

    target->save(true);
    return(0);
}

//*********************************************************************
//                     dmComputeLuck
//*********************************************************************
// This command allows a CT or DM to re-compute a player's luck value

int dmComputeLuck(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;

    if(cmnd->num < 2) {
        *player << "Re-compute which player's luck?\n";
        return(0);
    }

    cmnd->str[1][0] = up(cmnd->str[1][0]);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target || !player->canSee(target)) {
        *player << "That player is not logged on.\n";
        return(0);
    }
    else
    {
        *player << "Current luck for " << target << ": " << target->getLuck() << ".\n";
        target->computeLuck();
        *player << "Re-computed luck for " << target << ": " << target->getLuck() << ".\n";
    }

    target->save(true);
    return(0);

}

//*********************************************************************
//                      dmPut
//*********************************************************************
// This function allows a CT or DM to remotely add items to a player.

int dmPut(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    std::shared_ptr<Object> object=nullptr, container=nullptr;

    bool    online=false;

    if(cmnd->num < 2) {
        player->print("*put what?\n");
        return(0);
    }
    if(cmnd->num < 3) {
        player->print("*put what on whom?\n");
        return(0);
    }

    cmnd->str[2][0] = up(cmnd->str[2][0]);
    target = gServer->findPlayer(cmnd->str[2]);


    if(!target || !player->canSee(target)) {
        if(player->isDm()) {
            if(!loadPlayer(cmnd->str[2], target)) {
                player->print("Player does not exist.\n");
                return(0);
            }
        } else {
            player->print("That player is not logged on.\n");
            return(0);
        }
    } else
        online = true;



    if(cmnd->num > 3) {

        container = target->findObject(player, cmnd, 3);

        if(!container) {
            player->print("%s doesn't have that container.\n", target->upHeShe());
            return(0);
        }

        if(container->getType() != ObjectType::CONTAINER) {
            player->print("That isn't a container.\n");
            return(0);
        }

        object = player->findObject(player, cmnd, 1);
        if(!object) {
            player->print("You do not have that.\n");
            return(0);
        }


        if((container->getShotsCur() + 1) > container->getShotsMax()) {
            player->printColor("You will exceed the maximum allowed items for %P(%d).\n", container->getCName(), container->getShotsMax());
            player->print("Aborted.\n");
            return(0);
        }

        player->delObj(object, false, false, true, true, true);
        //container->incShotsCur();
        container->addObj(object);
        Limited::addOwner(target, object);

        player->printColor("You put %P into %N's %s.\n", object.get(), target.get(), container->getCName());

        target->save(online);
        return(0);
    }


    object = player->findObject(player, cmnd, 1);
    if(!object) {
        player->print("You don't have that.\n");
        return(0);
    }

    if(object->flagIsSet(O_DARKMETAL) && target->getRoomParent()->isSunlight()) {
        player->printColor("You cannot give %P to %N now!\nIt would surely be destroyed!\n", object.get(), target.get());
        return(0);
    }

    player->printColor("You add %P to %N's inventory.\n", object.get(), target.get());
    player->delObj(object, false, false, true, true, true);
    Limited::addOwner(target, object);
    target->addObj(object);

    target->save(online);
    return(0);
}


//*********************************************************************
//                      dmMove
//*********************************************************************
// this function allows a DM char to put a player into a totally new room
// when logged off. If logged on it will do nothing as teleport handles
// the online function.

int dmMove(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> creature=nullptr;

    MapMarker mapmarker;
    std::ostringstream log;
    CatRef  cr;

    // normal checks
    if(cmnd->num < 2) {
        player->print("Offline move whom?\n");
        return(0);
    }

    // already logged on?
    cmnd->str[1][0] = up(cmnd->str[1][0]);
    creature = gServer->findPlayer(cmnd->str[1]);
    if(creature) {
        if(player->canSee(creature))
            player->print("%s is already logged on, use *teleport!\n",  cmnd->str[1]);
        else
            player->print("You cannot move that player.\n");
        return(0);
    }

    // now load 'em in
    if(!loadPlayer(cmnd->str[1], creature)) {
        player->print("player %s does not exist\n", cmnd->str[1]);
        return(0);
    }
    if(creature->isCt() && !player->isDm()) {
        player->print("You cannot move that player.\n");
        return(0);
    }

    // put them in their new location
    log << player->getName() << " moved player " << creature->getName() << " from room ";
    // TODO: area_room isn't valid
    if(creature->currentLocation.mapmarker.getArea() != 0)
        log << creature->currentLocation.mapmarker.str(false);
    else
        log << creature->currentLocation.room.displayStr();

    log << " to room ";

    getDestination(getFullstrText(cmnd->fullstr, 2), mapmarker, cr, player);

    if(!cr.id) {
        std::shared_ptr<Area> area = gServer->getArea(mapmarker.getArea());
        if(!area) {
            player->print("That area does not exist.\n");
            return(0);
        }
        creature->currentLocation.room.clear();
        creature->currentLocation.mapmarker = mapmarker;

        log << creature->currentLocation.mapmarker.str(false);
        player->print("Player %s moved to location %s.\n", creature->getCName(), creature->currentLocation.mapmarker.str(false).c_str());
    } else {
        if(!validRoomId(cr)) {
            player->print("Can only put players in the range of 1-%d.\n", RMAX);
            return(0);
        }
        *&creature->currentLocation.room = *&cr;
        log << cr.displayStr();
        player->print("Player %s moved to location %s.\n", creature->getCName(), cr.displayStr().c_str());
    }

    log_immort(true, player, "%s.\n", log.str().c_str());

    creature->save();
    return(0);
}


//*********************************************************************
//                      dmWordAll
//*********************************************************************
// This function is to be used to quickly move ALL players online to
// one place for whatever reason.

int dmWordAll(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<BaseRoom> room=nullptr;

    std::shared_ptr<Player> target=nullptr;

    for(const auto& p : gServer->players) {
        target = p.second;

        if(!target->isConnected())
            continue;
        if(target->isStaff())
            continue;

        room = target->getRecallRoom().loadRoom(target);

        target->print("An astral vortex just arrived.\n");
        broadcast(target->getSock(), target->getRoomParent(), "An astral vortex just arrived.");

        target->print("The astral vortex sucks you in!\n");
        broadcast(target->getSock(), target->getRoomParent(), "The astral vortex sucks %N into it!", target.get());

        broadcast(target->getSock(), target->getRoomParent(), "The astral vortex disappears.");

        broadcast("^Y### Strangely, %N was sucked into an astral vortex.", target.get());

        // Optionally, can knock everyone out for one minute.
        if(!strcmp(cmnd->str[1], "-u"))
            target->knockUnconscious(60);


        if(!target->inJail()) {
            target->deleteFromRoom();
            target->addToRoom(room);
        }

        target->save(true);
    }
    return(0);
}

//*********************************************************************
//                      dmRename
//*********************************************************************

int dmRename(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    int     i=0;
    FILE    *fp;
    std::string oldName, newName;
    char    file[80];

    if(!player->isStaff() && !player->isWatcher())
        return(cmdNoExist(player, cmnd));
    if(!player->isWatcher())
        return(cmdNoAuth(player));

    if(cmnd->num < 2) {
        player->print("Rename who?\n");
        return(0);
    }
    if(cmnd->num < 3) {
        player->print("Rename to what?\n");
        return(0);
    }

    cmnd->str[1][0] = up(cmnd->str[1][0]);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target || !player->canSee(target)) {
        player->print("%s is not logged on!\n", cmnd->str[1]);
        return(0);
    }

    if(target->isCt() && !player->isDm()) {
        player->print("You cannot rename that player.\n");
        return(0);
    }

    if(target->getLevel() > 9 && player->isWatcher() && !player->isCt()) {
        player->print("You may only rename players under 10th level.\n");
        return(0);
    }

    oldName = target->getName();
    newName = cmnd->str[2];
    newName[0] = up(newName[0]);

    if(newName == oldName) {
        player->print("Names are the same: choose a different name.\n");
        return(0);
    }
    if(player->getName() == oldName) {
        player->print("You can't rename yourself.\n");
        return(0);
    }
    if(!parse_name(newName)) {
        player->print("The new name is not allowed, pick another.\n");
        return(0);
    }
    std::string::size_type len = newName.length();
    if(len >= 20) {
        player->print("The name must be less than 20 characters.\n");
        return(0);
    }
    if(len < 3) {
        player->print("The name must be more than 3 characters.\n");
        return(0);
    }
    for(i=0; i< (int)len; i++)
        if(!isalpha(newName[i]) && (newName[i] != '\'')) {
            player->print("Name must be alphabetic, pick another.\n");
            return(0);
        }

    // See if a player with the new name exists
    sprintf(file, "%s/%s.xml", Path::Player.c_str(), newName.c_str());
    fp = fopen(file, "r");
    if(fp) {
        player->print("A player with that name already exists.\n");
        fclose(fp);
        return(0);
    }
    gServer->clearPlayer(target);
    target->setName( newName);
    gServer->addPlayer(target);

    if(target->getGuild()) {
        Guild* guild = gConfig->getGuild(target->getGuild());
        guild->renameMember(oldName, newName);
    }
    gConfig->guildCreationsRenameSupporter(oldName, newName);
    gConfig->saveGuilds();


    gConfig->renamePropertyOwner(oldName, target);
    renamePlayerFiles(oldName.c_str(), newName.c_str());
    gServer->clearPlayer(oldName);
    gServer->addPlayer(target);

    player->print("%s has been renamed to %s.\n", oldName.c_str(), target->getCName());
    target->print("You have been renamed to %s.\n", target->getCName());
    broadcast(isDm, "^g### %s renamed %s to %s.", player->getCName(), oldName.c_str(), target->getCName());
    logn("log.rename", "%s renamed %s to %s.\n", player->getCName(), oldName.c_str(), target->getCName());

    // unassociate
    if(!target->getForum().empty()) {
        //target->printColor("Your forum account ^C%s^x is no longer unassociated with this character.\n", target->getForum().c_str());
        //target->print("Use the \"forum\" command to reassociate this character with your forum account.\n");
        //target->setForum("");
        callWebserver((std::string)"mud.php?type=forum&char=" + oldName + "&rename=" + newName);
    }

    target->save(true);
    return(0);
}


//*********************************************************************
//                      dmPassword
//*********************************************************************

int dmPassword(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;

    bool    online=false;
    std::string pass = "";

    if(cmnd->num < 2) {
        player->print("Change who's password?\n");
        return(0);
    }

    pass = getFullstrText(cmnd->fullstr, 2);
    if(pass.empty()) {
        player->print("Change the password to what?\n");
        return(0);
    }

    cmnd->str[1][0] = up(cmnd->str[1][0]);
    target = gServer->findPlayer(cmnd->str[1]);
    if(!target) {
        if(!loadPlayer(cmnd->str[1], target)) {
            player->print("Player does not exist.\n");
            return(0);
        }
    } else
        online = true;


    if(target->isPassword(pass)) {
        player->print("Passwords must be different.\n");
        return(0);
    }
    if(pass.length() > PASSWORD_MAX_LENGTH) {
        player->print("The password must be %d characters or less.\n", PASSWORD_MAX_LENGTH);
        return(0);
    }
    if(pass.length() < PASSWORD_MIN_LENGTH) {
        player->print("Password must be at least %d characters.\n", PASSWORD_MAX_LENGTH);
        return(0);
    }

    target->setPassword(pass);

    player->print("%s's password has been changed to %s.\n", target->getCName(), pass.c_str());
    logn("log.passwd", "### %s changed %s's password to %s.\n", player->getCName(), target->getCName(), pass.c_str());

    target->save(online);
    return(0);
}


//*********************************************************************
//                      dmRestorePlayer
//*********************************************************************

int dmRestorePlayer(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    int     n=0;
    long    old_xp=0;
    float   exp=0;
    bool    online=false;

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target) {
        if(!loadPlayer(cmnd->str[1], target)) {
            player->print("Player does not exist.\n");
            return(0);
        }
    } else
        online = true;

    if(target->getNegativeLevels()) {
        player->print("Clear %s's negative levels first.\n", target->getCName());
        return(0);
    }

    old_xp = target->getExperience();

    exp = (float)target->getExperience();
    exp /= 0.85;
    target->setExperience((long)exp);

    n = exp_to_lev(target->getExperience());
    while(target->getLevel() < n)
        target->upLevel();


    player->print("%M has been restored.\n", target.get());

    log_immort(true, player, "%s restored %s from %ld to %ld xp.\n", player->getCName(),
        target->getCName(), old_xp, target->getExperience());

    target->save(online);
    if(online)
        target->print("Your experience has been restored.\n");
    return(0);
}

//*********************************************************************
//              dmGeneric
//*********************************************************************
// This is a generic function that handles getting information or doing
// something minor to a player who may be online or offline.
// Authorization is not done here.

#define DM_GEN_BANK     1
#define DM_GEN_INVVAL   2
// OLD PROXY            3
#define DM_GEN_WARN     4
#define DM_GEN_BUG      5

int dmGeneric(const std::shared_ptr<Player>& player, cmd* cmnd, std::string_view action, int what) {
    std::shared_ptr<Player> target=nullptr;

    bool    online=false;

    if(cmnd->num < 2) {
        player->bPrint(fmt::format("{} whom?\n", action));
        return(0);
    }

    cmnd->str[1][0] = up(cmnd->str[1][0]);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target || !player->canSee(target)) {
        if(player->isCt()) {
            if(!loadPlayer(cmnd->str[1], target)) {
                player->print("Player does not exist.\n");
                return(DOPROMPT);
            }
        } else {
            player->print("That player is not logged on.\n");
            return(DOPROMPT);
        }
    } else
        online = true;


    // time to actually do what they want us to do
    if(what == DM_GEN_BANK)
        player->print("%s's bank balance is: %ldgp.\n", target->getCName(), target->bank[GOLD]);
    else if(what == DM_GEN_INVVAL)
        player->print("%s's total inventory assets: %ldgp.\n", target->getCName(), target->getInventoryValue());
    else if(what == DM_GEN_WARN) {

        if(!player->isDm() && target->isDm()) {
            player->print("Don't be silly.\n");
            return(0);
        }

        if(cmnd->num < 3) {
            player->print("%M has %d total warning%s.\n", target.get(), target->getWarnings(), (target->getWarnings()==1 ? "":"s") );
            return(0);
        }

        if(!strcmp(cmnd->str[2], "-r")) {
            if(!player->isCt()) {
                player->print("You are unable to remove warnings.\n");
                return(0);
            }
            if(target->getWarnings() < 1) {
                player->print("%M has no warnings.\n", target.get());
                return(0);
            } else {
                target->subWarnings(1);
                if(online)
                    target->print("%M removed one of your warnings. You now have %d.\n", player.get(), target->getWarnings());
                player->print("One warning removed. %M now has %d warning%s remaining.\n", target.get(), target->getWarnings(), (target->getWarnings()==1 ? "":"s"));
            }
            log_immort(false,player, "%s removed a warning from %s.\n", player->getCName(), target->getCName());
            logn("log.warn", "%s removed a warning from %s.\n", player->getCName(), target->getCName());

            broadcast(isCt, "^y%s removed a warning from %s. New total: %d", player->getCName(), target->getCName(), target->getWarnings());
        } else if(!strcmp(cmnd->str[2], "-a")) {
            if(target->getWarnings() > 2) {
                player->print("%M already has 3 warnings. %s cannot be warned further.\n", target.get(), target->upHeShe());
                return(0);
            } else {
                target->addWarnings(1);
                if(online)
                    target->print("%M has formally warned you. You now have %d warning%s.\n", player.get(), target->getWarnings(), (target->getWarnings()>1 ? "s":""));
                player->print("Warning added. %M now has %d total warning%s.\n", target.get(), target->getWarnings(), (target->getWarnings()>1 ? "s":""));

                logn("log.warn", "%s added a warning to %s.\n", player->getCName(), target->getCName());
                log_immort(false,player, "%s added a warning to %s.\n", player->getCName(), target->getCName());

                broadcast(isCt, "^y%s added a warning to %s. New total: %d", player->getCName(), target->getCName(), target->getWarnings());
            }

        } else {
            player->print("Syntax: *warn (player) [-r|-a]\n");
            return(0);
        }
        target->save(online);

    } else if(what == DM_GEN_BUG) {

        if(!target->flagIsSet(P_BUGGED)) {
            player->print("%s is now bugged for surveillance.\n", target->getCName());
            target->setFlag(P_BUGGED);
        } else {
            player->print("%s is no longer bugged for surveillance.\n", target->getCName());
            target->clearFlag(P_BUGGED);
        }
        target->save(online);

    } else
        player->print("Invalid option for dmGetInfo().\n");

    return(0);
}

//*********************************************************************
//                      dmBank
//*********************************************************************
int dmBank(const std::shared_ptr<Player>& player, cmd* cmnd) {
    return(dmGeneric(player, cmnd, "*bank", DM_GEN_BANK));
}
//*********************************************************************
//                      dmInventoryValue
//*********************************************************************
int dmInventoryValue(const std::shared_ptr<Player>& player, cmd* cmnd) {
    return(dmGeneric(player, cmnd, "*inv", DM_GEN_INVVAL));
}
//*********************************************************************
//                      dmWarn
//*********************************************************************
int dmWarn(const std::shared_ptr<Player>& player, cmd* cmnd) {

    // watchers can use *warn
    if(!player->isStaff() && !player->isWatcher())
        return(cmdNoExist(player, cmnd));
    if(!player->isWatcher())
        return(cmdNoAuth(player));

    return(dmGeneric(player, cmnd, "*warn", DM_GEN_WARN));
}
//*********************************************************************
//                      dmBugPlayer
//*********************************************************************
int dmBugPlayer(const std::shared_ptr<Player>& player, cmd* cmnd) {
    return(dmGeneric(player, cmnd, "*bug", DM_GEN_BUG));
}



// dmKill function decides which of these are player-kill,
// room-kill or all-kill
#define DM_KILL         0
#define DM_COMBUST      1
#define DM_SLIME        2
#define DM_GNATS        3
#define DM_TRIP         4
#define DM_BOMB         5
#define DM_NUCLEAR      6
#define DM_ARMAGEDDON   7
#define DM_IGMOO        8
#define DM_RAPE         9
#define DM_DRAIN        10
#define DM_CRUSH        11
#define DM_MISSILE      12
#define DM_SUPERNOVA    13
#define DM_NOMNOM       14
#define DM_GRENADE      15
#define DM_HEAD_BOOM    16
#define DM_STAB         17
#define DM_SUICIDE      18
#define DM_ALCOHOL      19
#define DM_DROWNED      20
#define DM_DRAGON       21
#define DM_CHOKE        22
#define DM_DEMON        23
#define DM_RATS         24
#define DM_COMET        25
#define DM_ABYSS        26
#define DM_SHARDS       27
#define DM_FLATULENCE   28
#define DM_DIARRHEA     29
#define DM_KINETIC      30

//*********************************************************************
//                      dmKillAll
//*********************************************************************
// not declared in proto.h, only used below
// this function handles and *kill commands that kill everyone

int dmKillAll(int type, int silent, int hpmp_loss, int unconscious, int uncon_length) {
    std::shared_ptr<BaseRoom> room=nullptr;

    std::shared_ptr<Player> target;
    for(const auto& p : gServer->players) {
        target = p.second;

        if(!target->isConnected())
            continue;
        if(target->isStaff())
            continue;

        room = target->getLimboRoom().loadRoom(target);

        if(!room)
            continue;

        switch(type) {
        case DM_ARMAGEDDON:
            target->print("Meteors begin falling from the sky!\n");
            broadcast(target->getSock(), target->getRoomParent(), "Meteors begin falling from the sky!\n");

            target->print("A freak meteor strikes you!\n");
            broadcast(target->getSock(), target->getRoomParent(), "A freak meteors strikes %N!", target.get());

            target->print("The freak meteor slams into you for %d damage!\n", Random::get(5000,10000));
            broadcast(target->getSock(), target->getRoomParent(), "The freak meteor blasts right through %N!", target.get());
            target->print("You die as your ashes fall to the ground.\n");
            broadcast(target->getSock(), target->getRoomParent(), "The freak meteor vaporized %N!", target.get());

            if(!silent)
                broadcast("### Sadly, %N was killed by a freak meteor shower.", target.get());
            break;
        case DM_IGMOO:
            target->print("Igmoo the Mad just arrived.\n");

            target->printColor("^GIgmoo the Mad casts a disintegration spell on you.\n");
            target->print("You are engulfed in an eerie green light.\n");
            target->printColor("You've been disintegrated!\n");

            target->print("Igmoo the Mad killed you.\n");

            if(!silent)
                broadcast("### Sadly, %N was disintegrated by Igmoo the Mad.", target.get());
            break;
        case DM_SUPERNOVA:
            target->printColor("\n^YThe searing rays of the sun grow uncomfortably hot.\n\n");

            target->printColor("^YA blinding flash of burning light engulfs the sky!\n");
            target->printColor("^YYour eyes are burned from their sockets and your skin begins to boil!\n");
            target->printColor("^YYou've been disintegrated!\n\n");

            if(!silent)
                broadcast("^Y### Sadly, %N was disintegrated by an exploding sun.", target.get());

        default:
            break;
        }

        if(hpmp_loss) {
            target->hp.setCur(1);
            target->mp.setCur(1);
        }

        if(unconscious)
            target->knockUnconscious(uncon_length);

        if(!target->inJail()) {
            target->deleteFromRoom();
            target->addToRoom(room);
            target->doPetFollow();
        }

    }
    return(0);
}

//*********************************************************************
//                      dmKill
//*********************************************************************
// not declared in proto.h, only used below
// handles player-targeted *kills

int dmKill(std::shared_ptr<Player> player, std::shared_ptr<Player>victim, int type, int silent, int hpmp_loss, int unconscious, int uncon_length) {
    int     kill_room=0, no_limbo=0;
    std::shared_ptr<BaseRoom> newRoom=nullptr;

    char filename[80];
    snprintf(filename, 80, "%s/crash.txt", Path::Config.c_str());

    // room kill or no limbo?
    switch(type) {
        case DM_BOMB:
        case DM_NUCLEAR:
            kill_room = 1;
            break;
        case DM_RAPE:
        case DM_DRAIN:
            no_limbo = 1;
            break;
        default:
            break;
    }

    if(victim->isDm()) {
        switch(type) {
        case DM_COMBUST:
            player->print("You tried to make %N spontaneously combust!\n", victim.get());
            victim->printColor("^r%M tried to make you spontaneously combust!\n", player.get());
            break;
        case DM_SLIME:
            player->print("You tried to make %N dissolve in green slime!\n", victim.get());
            victim->printColor("^g%M tried to dissolve you in green slime!\n", player.get());
            break;
        case DM_GNATS:
            player->print("You tried to make a swarm of gnats eat %N!\n", victim.get());
            victim->printColor("^m%M tried to send a swarm of ravenous gnats after you!\n", player.get());
            break;
        case DM_TRIP:
            player->print("You tried to make %N trip and break %s neck!\n", victim.get(), victim->hisHer());
            victim->printColor("^y%M tried to make you trip and break your neck!\n", player.get());
            break;
        case DM_BOMB:
            player->print("You tried to suicide bomb %N!\n", victim.get());
            victim->printColor("^r%M tried to suicide bomb you!\n", player.get());
            break;
        case DM_NUCLEAR:
            player->print("You tried to make %N meltdown!\n", victim.get());
            victim->printColor("^g%M tried to make you meltdown!\n", player.get());
            break;
        case DM_RAPE:
            player->print("You tried to rape %N!\n", victim.get());
            victim->printColor("^m%M tried to rape you!\n", player.get());
            break;
        case DM_DRAIN:
            player->print("You tried to drain %N!\n", victim.get());
            victim->printColor("^c%M tried to drain you!\n", player.get());
            break;
        case DM_CRUSH:
            player->print("You tried to crush %N!\n", victim.get());
            victim->printColor("^c%M tried to crush you!\n", player.get());
            break;
        case DM_MISSILE:
            player->print("You tried to hit %N with a cruise missile!\n", victim.get());
            victim->printColor("^r%M tried to hit you with a cruise missile!\n", player.get());
            break;
        case DM_NOMNOM:
            player->print("You tried to make a fearsome monster eat %N!\n", victim.get());
            victim->printColor("^r%M tried to make a fearsome monster!\n", player.get());
            break;
        case DM_GRENADE:
            player->print("You tried to make %N die from a grenade!\n", victim.get());
            victim->printColor("^r%M tried to kill you with a grenade!\n", player.get());
            break;
        case DM_HEAD_BOOM:
            player->print("You tried to make %N's head explode!\n", victim.get());
            victim->printColor("^r%M tried to make your head explode!\n", player.get());
            break;
        case DM_STAB:
            player->print("You tried to make %N stab %sself to death!\n", victim.get(), victim->himHer());
            victim->printColor("^r%M tried to make you stab yourself to death!\n", player.get());
            break;
        case DM_SUICIDE:
            player->print("You tried to make %N kill %sself!\n", victim.get(), victim->himHer());
            victim->printColor("^r%M tried to make you kill yourself!\n", player.get());
            break;
         case DM_ALCOHOL:
            player->print("You tried to make %N drink %sself to death!\n", victim.get(), victim->himHer());
            victim->printColor("^r%M tried to make you drink yourself to death!\n", player.get());
            break;
        case DM_DROWNED:
            player->print("You tried to drown %N!\n", victim.get());
            victim->printColor("^r%M tried to drown you!\n", player.get());
            break;
        case DM_DRAGON:
            player->print("You tried to kill %N with an ancient dragon!\n", victim.get());
            victim->printColor("^r%M tried to send an ancient dragon to kill you!\n", player.get());
            break;
         case DM_CHOKE:
            player->print("You tried to choke %N to death!\n", victim.get());
            victim->printColor("^r%M tried to choke you to death!\n", player.get());
            break;
        case DM_DEMON:
            player->print("You tried to make an ancient demon kill %N!\n", victim.get());
            victim->printColor("^r%M tried to kill you with an ancient demon!\n", player.get());
            break;
        case DM_RATS:
            player->print("You tried to kill %N with gutter rats!\n", victim.get());
            victim->printColor("^r%M tried to kill you with gutter rats!\n", player.get());
            break;
        case DM_COMET:
            player->print("You tried to kill %N with a firey comet!\n", victim.get());
            victim->printColor("^r%M tried to kill you with a firey comet!\n", player.get());
            break;
        case DM_ABYSS:
            player->print("You tried to send %N's soul to the Abyss!\n", victim.get());
            victim->printColor("^r%M tried to suck your soul into the Abyss!\n", player.get());
            break;
        case DM_SHARDS:
            player->print("You tried to break %N into millions of tiny crystalline shards!\n", victim.get());
            victim->printColor("^r%M tried break you into millions of tiny crystalline shards!\n", player.get());
            break;
        case DM_FLATULENCE:
            player->print("You tried to give %N death by an overabundance of flatulence!\n", victim.get());
            victim->printColor("^r%M tried to give you an overabundance of flatulence and kill you!\n", player.get());
            break;
        case DM_DIARRHEA:
            player->print("You tried to give %N excessive diarreah of the mouth and kill %s!\n", victim.get(), victim->himHer());
            victim->printColor("^r%M tried to give you excessive diarreah and kill you!\n", player.get());
            break;
        case DM_KINETIC:
            player->print("You tried to kill %N with a kinetic strike from space!\n", victim.get());
            victim->printColor("^r%M tried to kill you with a kinetic strike from space!\n", player.get());
            break;
        default:
            player->print("You tried to strike down %N!\n", victim.get());
            victim->printColor("^r%M tried to strike you down with lightning!\n", player.get());
            break;
        }
        return(0);
    }

    switch(type) {
    case DM_COMBUST:
        victim->printColor("^rYou just spontaneously combusted!\n");
        break;
    case DM_SLIME:
        victim->printColor("^gYou have been dissolved by green slime!\n");
        break;
    case DM_GNATS:
        victim->printColor("^mYou have been eaten by a ravenous demonic gnat swarm!\n");
        break;
    case DM_TRIP:
        victim->printColor("^yYou slip on an oversized banana and break your neck.\nYou're dead!\n");
        break;
    case DM_BOMB:
        victim->printColor("^rYou blow yourself to smithereens!\n");
        break;
    case DM_NUCLEAR:
        victim->printColor("^gYou begin to meltdown!\n");
        victim->getSock()->viewFile(Path::Config / "crash.txt");
        break;
    case DM_RAPE:
        victim->printColor("^mYou have been raped by the gods!\n");
        break;
    case DM_DRAIN:
        victim->printColor("^cYour life force drains away!\n");
        break;
    case DM_CRUSH:
        victim->printColor("^cA giant stone slab falls from the sky!\n");
        break;
    case DM_MISSILE:
        victim->printColor("^rYou are struck by a cruise missile!\nYou explode!\n");
        break;
    case DM_KILL:
        victim->printColor("^rA giant lightning bolt strikes you!\n");
        break;
    case DM_NOMNOM:
        victim->print("A fearsome monster leaps out of the shadows!\n");
        victim->print("It tears off your limbs and greedily devours them!\n");
        break;
    case DM_GRENADE:
        victim->print("You get out a hand grenade and pull the pin.\n");
        victim->print("You forget to throw it. It explodes.\n");
        break;
    case DM_HEAD_BOOM:
        victim->print("Your head expands inexplicably! It explodes.\n");
        break;
    case DM_STAB:
        victim->print("You pull out a hidden knife.\n");
        victim->print("You proceed to stab yourself to death.\n");
        break;
    case DM_SUICIDE:
        victim->print("You just can't take it anymore. You kill yourself.\n");
        break;
    case DM_ALCOHOL:
        victim->print("You drink yourself to death.\n");
        break;
    case DM_DROWNED:
        victim->print("You stick your head into a small puddle.\n");
        victim->print("You drown.\n");
        break;
    case DM_DRAGON:
        victim->print("An ancient great wyrm red dragon just arrived.\n");
        victim->print("It bites you in half and eats both pieces.\n");
        break;
    case DM_CHOKE:
        victim->print("You feel an overwhelming choking sensation.\n");
        victim->print("You desperately grab at your throat as you slowly choke to death.\n");
        break;
    case DM_DEMON:
        victim->print("An ancient demon just arrived.\n");
        victim->print("The ancient demon proceeds to rip you into tiny pieces.\n");
        break;
    case DM_RATS:
        victim->print("10000 gutter rats just arrived.\n");
        victim->print("They kill you horribly.\n");
        break;
    case DM_COMET:
        victim->print("A firey comet appears from the skies and annihilates you.\n");
        break;
    case DM_ABYSS:
        victim->print("An Abyssal void pocket appears out of nowhere.\n");
        victim->print("You are promptly sucked into it. You die.\n");
        break;
    case DM_SHARDS:
        victim->print("You spontaneously explode into millions of tiny crystalline shards.\n");
        break;
    case DM_FLATULENCE:
        victim->print("You suddenly have an overabundance of flatulence.\n");
        victim->print("It gets so bad that you explode. You die horribly.\n");
        break;
    case DM_DIARRHEA:
        victim->print("Excessive diarreah suddenly spurts uncontrollably from your mouth.\n");
        victim->print("You die in agony from dehydration.\n");
        break;
    case DM_KINETIC:
        victim->print("A small titanium rod hits you from space at nearly the speed of light!\n");
        victim->print("You were utterly obliterated!\n");
        break;
    default:
        break;
    }

    if(victim->getClass() < CreatureClass::BUILDER && !silent) {
        switch(type) {
        case DM_COMBUST:
            broadcast(victim->getSock(), victim->getRoomParent(), "^r%s bursts into flames!", victim->getCName());
            broadcast("^r### Sadly, %s spontaneously combusted.", victim->getCName());
            break;
        case DM_SLIME:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^gA massive green slime just arrived.\nThe massive green slime attacks %s!", victim->getCName());
            broadcast("^g### Sadly, %s was dissolved by a massive green slime.", victim->getCName());
            break;
        case DM_GNATS:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^mA demonic gnat swarm just arrived.\nThe demonic gnat swarm attacks %s!", victim->getCName());
            broadcast("^m### Sadly, %s was eaten by a ravenous demonic gnat swarm!", victim->getCName());
            break;
        case DM_TRIP:
            broadcast(victim->getSock(), victim->getRoomParent(), "^m%s slips on an oversized banana peel!", victim->getCName());
            broadcast("^m### Sadly, %s tripped and broke %s neck.", victim->getCName(), victim->hisHer());
            break;
        case DM_BOMB:
            broadcast(victim->getSock(), victim->getRoomParent(), "^r%s begins to tick!", victim->getCName());
            broadcast("^r### Sadly, %s blew %sself up!", victim->getCName(), victim->himHer());
            break;
        case DM_NUCLEAR:
            broadcast(victim->getSock(), victim->getRoomParent(), "^g%s begins to meltdown!", victim->getCName());
            broadcast("^g### Sadly, %s was killed in a nuclear explosion.", victim->getCName());
            break;
        case DM_KILL:
            broadcast(victim->getSock(), victim->getRoomParent(), "^rA giant lightning bolt strikes %s!", victim->getCName());
            broadcast("^r### Sadly, %s has been incinerated by lightning from the heavens!", victim->getCName());
            break;
        case DM_RAPE:
            broadcast(victim->getSock(), victim->getRoomParent(), "^m%s was raped by the gods!", victim->getCName());
            broadcast(isStaff, "^m*** %s raped %s.", player->getCName(), victim->getCName());
            break;
        case DM_DRAIN:
            broadcast(victim->getSock(), victim->getRoomParent(), "^c%s's life force drains away!", victim->getCName());
            broadcast(isStaff, "^g*** %s drained %s.", player->getCName(), victim->getCName());
            break;
        case DM_CRUSH:
            broadcast(victim->getSock(), victim->getRoomParent(), "^cA giant stone slab falls on %s!", victim->getCName());
            broadcast("^c### Sadly, %s was crushed under a giant stone slab.", victim->getCName());
            break;
        case DM_MISSILE:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^R%s was struck by a cruise missile!\n%s exploded!", victim->getCName(), victim->getCName());
            broadcast("^R### Sadly, %s was killed by a precision-strike cruise missile.", victim->getCName());
            break;
        case DM_NOMNOM:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^WA fearsome monster leaps out of the shadows!\nIt tears off %s's limbs and greedily devours them!", victim->getCName());
            broadcast("^W### Sadly, %s was devoured by a fearsome monster.", victim->getCName());
            break;
        case DM_GRENADE:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^g%s pulls the pin on a hand grenade. %s forgets to throw it.", victim->getCName(), victim->upHeShe());
            broadcast("^g### Sadly, %s was killed by an exploding hand grenade.", victim->getCName());
            break;
        case DM_HEAD_BOOM:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^y%s's head expands and violently explodes!", victim->getCName());
            broadcast("^y### Sadly, %s's head exploded. %s died.", victim->getCName(), victim->upHeShe());
            break;
        case DM_STAB:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^M%s pulls out a hidden knife and stabs %sself to death. Gory.", victim->getCName(), victim->himHer());
            broadcast("^M### Sadly, %s stabbed %sself to death.", victim->getCName(), victim->himHer());
            break;
        case DM_SUICIDE:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^D%s can't take it anymore. %s simply kills %sself.", victim->getCName(), victim->upHeShe(), victim->himHer());
            broadcast("^D### Sadly, %s killed %sself. %s won't be missed.", victim->getCName(), victim->himHer(), victim->upHeShe());
            break;
        case DM_ALCOHOL:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^C%s drinks %sself to death.", victim->getCName(), victim->himHer());
            broadcast("^C### Sadly, %s died from alcohol poisoning.", victim->getCName());
            break;
        case DM_DROWNED:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^b%s puts %s head in a small puddle of water. %s drowns.", victim->getCName(), victim->hisHer(), victim->upHeShe());
            broadcast("^b### Sadly, %s drowned in a small puddle of water.", victim->getCName());
            break;
        case DM_DRAGON:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^RAn ancient great wyrm red dragon suddenly appears. It bites %s in half and eats both pieces. It leaves to go take a dump.", victim->getCName());
            broadcast("^R### Sadly, %s was bitten in half by an ancient great wyrm red dragon and eaten. %s died horribly.", victim->getCName(), victim->upHeShe());
            break;
        case DM_CHOKE:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^c%s grabs at %s throat. %s inexplicably chokes to death.", victim->getCName(), victim->hisHer(), victim->upHeShe());
            broadcast("^c### Sadly, %s was choked to death by the gods.", victim->getCName());
            break;
        case DM_DEMON:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^DAn ancient demon just arrived.\nThe ancient demon gruesomely rips %s apart and vanishes.", victim->getCName());
            broadcast("^D### Sadly, %s was ripped into tiny pieces by an ancient demon.", victim->getCName());
            break;
        case DM_RATS:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^y10000 gutter rats just arrived.\nThe gutter rats horribly kill %s and then disappear.", victim->getCName());
            broadcast("^y### Sadly, %s was horribly killed by 10000 gutter rats.", victim->getCName());
            break;
        case DM_COMET:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^YA cataclysmic firey comet suddenly appears from the skies and annihilates %s.", victim->getCName());
            broadcast("^Y### Sadly, %s was annihilated by a cataclysmic firey comet.", victim->getCName());
            break;
        case DM_ABYSS:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^DAn Abyssal void pocket appears out of nowhere. %s is promptly sucked into it.\n%s is never heard from again.", victim->getCName(), victim->upHeShe());
            broadcast("^D### Sadly, %s was sucked into an Abyssal void pocket and has been lost forever.", victim->getCName());
            break;
        case DM_SHARDS:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^W%s spontaneously shatters into millions of tiny crystalline shards.", victim->getCName());
            broadcast("^W### Sadly, %s spontaneously exploded into millions of tiny crystalline shards.", victim->getCName());
            break;
        case DM_FLATULENCE:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^m%s is overcome by an overabundance of flatulence. It gets so bad it causes %s to explode.", victim->getCName(), victim->himHer());
            broadcast("^m### Sadly, %s exploded due to an overabundance of flatulence.", victim->getCName());
            break;
        case DM_DIARRHEA:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^yExcessive diarreah suddenly spurts uncontrollably from %s's mouth.\n%s dies in agony from dehydration.", victim->getCName(), victim->upHeShe());
            broadcast("^y### Sadly, %s died suddenly from excessive diarreah of the mouth.", victim->getCName());
            break;
        case DM_KINETIC:
            broadcast(victim->getSock(), victim->getRoomParent(),
                "^cA small titanium rod hits %s from space at nearly the speed of light!\n%s was utterly obliterated.", victim->getCName(), victim->upHeShe());
            broadcast("^c### Sadly, %s was killed by a kinetic strike from space.", victim->getCName());
            break;
        default:
            break;
        }
    } else {
        switch(type) {
        case DM_COMBUST:
            broadcast(isStaff, "^G### Sadly, %s spontaneously combusted.", victim->getCName());
            break;
        case DM_SLIME:
            broadcast(isStaff, "^G### Sadly, %s was dissolved by a massive green slime.", victim->getCName());
            break;
        case DM_GNATS:
            broadcast(isStaff, "^G### Sadly, %s was eaten by a ravenous demonic gnat swarm!", victim->getCName());
            break;
        case DM_TRIP:
            broadcast(isStaff, "^G### Sadly, %s tripped and broke %s neck.", victim->getCName(), victim->hisHer());
            break;
        case DM_BOMB:
            broadcast(isStaff, "^G### Sadly, %s blew %sself up!!", victim->getCName(), victim->himHer());
            break;
        case DM_NUCLEAR:
            broadcast(isStaff, "^G### Sadly, %s was killed in a nuclear explosion.", victim->getCName());
            break;
        case DM_KILL:
            broadcast(isStaff, "^G### Sadly, %s has been incinerated by lightning from the heavens!", victim->getCName());
            break;
        case DM_RAPE:
            broadcast(isStaff, "^g*** %s raped %s.", player->getCName(), victim->getCName());
            break;
        case DM_DRAIN:
            broadcast(isStaff, "^g*** %s drained %s.", player->getCName(), victim->getCName());
            break;
        case DM_CRUSH:
            broadcast(isStaff, "^G### Sadly, %s was crushed under a giant stone slab.", victim->getCName());
            break;
        case DM_MISSILE:
            broadcast(isStaff, "^G### Sadly, %s was killed by a precision-strike cruise missile.", victim->getCName());
            break;
        case DM_NOMNOM:
            broadcast(isStaff, "^G### Sadly, %s was devoured by a fearsome monster.", victim->getCName());
            break;
        case DM_GRENADE:
            broadcast(isStaff, "^g### Sadly, %s was killed by an exploding hand grenade.", victim->getCName());
            break;
        case DM_HEAD_BOOM:
            broadcast(isStaff, "^y### Sadly, %s's head exploded. %s died.", victim->getCName(), victim->upHeShe());
            break;
        case DM_STAB:
            broadcast(isStaff, "^M### Sadly, %s stabbed %sself to death.", victim->getCName(), victim->hisHer());
            break;
        case DM_SUICIDE:
            broadcast(isStaff, "^D### Sadly, %s killed %sself. %s won't be missed.", victim->getCName(), victim->himHer(), victim->upHeShe());
            break;
        case DM_ALCOHOL:
            broadcast(isStaff, "^C### Sadly, %s died from alcohol poisoning.", victim->getCName());
            break;
        case DM_DROWNED:
            broadcast(isStaff, "^b### Sadly, %s drowned in a small puddle of water.", victim->getCName());
            break;
        case DM_DRAGON:
            broadcast(isStaff, "^R### Sadly, %s was bitten in half by an ancient great wyrm red dragon and eaten. %s died horribly.", victim->getCName(), victim->upHeShe());
            break;
        case DM_CHOKE:
            broadcast(isStaff, "^c### Sadly, %s was choked to death by the gods.", victim->getCName());
            break;
        case DM_DEMON:
            broadcast(isStaff, "^D### Sadly, %s was ripped into tiny pieces by an ancient demon.", victim->getCName());
            break;
        case DM_RATS:
            broadcast(isStaff, "^y### Sadly, %s was horribly killed by 10000 gutter rats.", victim->getCName());
            break;
        case DM_COMET:
            broadcast(isStaff, "^Y### Sadly, %s was annihilated by a cataclysmic firey comet.", victim->getCName());
            break;
        case DM_ABYSS:
            broadcast(isStaff, "^D### Sadly, %s was sucked into an Abyssal void pocket and has been lost forever.", victim->getCName());
            break;
        case DM_SHARDS:
            broadcast(isStaff, "^W### Sadly, %s spontaneously exploded into millions of tiny crystalline shards.", victim->getCName());
            break;
        case DM_FLATULENCE:
            broadcast(isStaff, "^m### Sadly, %s exploded due to an overabundance of flatulence.", victim->getCName());
            break;
        case DM_DIARRHEA:
            broadcast(isStaff, "^y### Sadly, %s died suddenly from excessive diarreah of the mouth.", victim->getCName());
            break;
        case DM_KINETIC:
            broadcast(isStaff, "^r### Sadly, %s was killed by a kinetic strike from space.", victim->getCName());
            break;
        default:
            break;
        }
    }

    auto room = victim->getRoomParent();
    auto pIt = room->players.begin();
    auto pEnd = room->players.end();
    while(pIt != pEnd) {
        player = (*pIt++).lock();
        if(!player || (player->isStaff() && player!=victim))
            continue;


        if(player==victim || kill_room) {

            if(kill_room && player!=victim && !silent) {
                switch(type) {
                case DM_NUCLEAR:
                    player->getSock()->viewFile(filename);
                    broadcast("^g### Sadly, %s was killed by %s's radiation.", player->getCName(), victim->getCName());
                    break;
                default:
                    broadcast("^r### Sadly, %s died in %s's explosion!", player->getCName(), victim->getCName());
                    break;
                }
            }

            if (hpmp_loss || no_limbo) {
                player->hp.setCur(1);
                player->mp.setCur(1);
            }

            if(unconscious)
                player->knockUnconscious(uncon_length);

            // do we send them to limbo?
            if(!no_limbo && !player->isStaff()) {
                newRoom = player->getLimboRoom().loadRoom(player);
                if(newRoom) {
                    player->deleteFromRoom();
                    player->addToRoom(newRoom);
                    player->doPetFollow();
                }
            }
        }
    }
    return(0);
}

//*********************************************************************
//                      dmKillSwitch
//*********************************************************************
// all *kill commands are routed through this function; dmKillAll
// and dmKill are passed the appropriate arguments

int dmKillSwitch(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int     i, silent=0, unconscious=0, uncon_length=0, hpmp_loss=0, type=DM_KILL;
    std::shared_ptr<Player> victim;

    if(player->getClass() == CreatureClass::CARETAKER && !player->flagIsSet(P_CT_CAN_KILL)) {
        player->print("You don't have the authorization to use that command.\n");
        return(0);
    }

    // -s and -u and -l can be in any position, so we have to look for them
    for(i=1; i<cmnd->num; i++) {
        if(!strcmp(cmnd->str[i], "-s") && !(type==DM_ARMAGEDDON || type==DM_SUPERNOVA || type==DM_IGMOO))
            silent=1;

        if(!strcmp(cmnd->str[i], "-u")) {
            unconscious=1;
            if(i < cmnd->num)
                uncon_length = toNum<int>(cmnd->str[i+1]);

            uncon_length = std::min(120, std::max(15, uncon_length ) );
        }
        if(!strcmp(cmnd->str[i], "-l"))
            hpmp_loss=1;
    }


    // check for world-killers!
    if(!strcasecmp(cmnd->str[0], "*arma") || !strcasecmp(cmnd->str[0], "*armageddon"))
        type = DM_ARMAGEDDON;
    else if(!strcasecmp(cmnd->str[0], "*supernova"))
        type = DM_SUPERNOVA;
    else if(!strcasecmp(cmnd->str[0], "*igmoo"))
        type = DM_IGMOO;
    else if(cmnd->num < 2) {
        player->print("Kill whom?\n");
        return(0);
    } else {
        if(!strcasecmp(cmnd->str[1], "-arma") || !strcasecmp(cmnd->str[1], "-armageddon"))
            type = DM_ARMAGEDDON;
        else if(!strcasecmp(cmnd->str[1], "-supernova"))
            type = DM_SUPERNOVA;
        else if(!strcasecmp(cmnd->str[1], "-igmoo"))
            type = DM_IGMOO;
    }


    // if it's a world-killer
    if(type != DM_KILL) {
        return(dmKillAll(type, silent, hpmp_loss, unconscious, uncon_length));
    }


    // if it won't kill everyone, who will it kill?
    cmnd->str[1][0]=up(cmnd->str[1][0]);
    victim = gServer->findPlayer(cmnd->str[1]);
    if(!victim) {
        player->print("That person is not logged on.\n");
        return(0);
    }


    if(!strcasecmp(cmnd->str[0], "*kill")) {
        // default
    } else if(!strcasecmp(cmnd->str[0], "*combust"))
        type = DM_COMBUST;
    else if(!strcasecmp(cmnd->str[0], "*slime"))
        type = DM_SLIME;
    else if(!strcasecmp(cmnd->str[0], "*gnat"))
        type = DM_GNATS;
    else if(!strcasecmp(cmnd->str[0], "*trip"))
        type = DM_TRIP;
    else if(!strcasecmp(cmnd->str[0], "*bomb"))
        type = DM_BOMB;
    else if(!strcasecmp(cmnd->str[0], "*nuke"))
        type = DM_NUCLEAR;
    else if(!strcasecmp(cmnd->str[0], "*rape"))
        type = DM_RAPE;
    else if(!strcasecmp(cmnd->str[0], "*drain"))
        type = DM_DRAIN;
    else if(!strcasecmp(cmnd->str[0], "*crush"))
        type = DM_CRUSH;
    else if(!strcasecmp(cmnd->str[0], "*missile"))
        type = DM_MISSILE;
    else if(!strcasecmp(cmnd->str[0], "*nomnom"))
        type = DM_NOMNOM;
    else if(!strcasecmp(cmnd->str[0], "*grenade"))
        type = DM_GRENADE;
    else if(!strcasecmp(cmnd->str[0], "*head"))
        type = DM_HEAD_BOOM;
    else if(!strcasecmp(cmnd->str[0], "*stab"))
        type = DM_STAB;
    else if(!strcasecmp(cmnd->str[0], "*suicide"))
        type = DM_SUICIDE;
    else if(!strcasecmp(cmnd->str[0], "*alcohol"))
        type = DM_ALCOHOL;
    else if(!strcasecmp(cmnd->str[0], "*drown"))
        type = DM_DROWNED;
    else if(!strcasecmp(cmnd->str[0], "*dragon"))
        type = DM_DRAGON;
    else if(!strcasecmp(cmnd->str[0], "*choke"))
        type = DM_CHOKE;
    else if(!strcasecmp(cmnd->str[0], "*demon"))
        type = DM_DEMON;
    else if(!strcasecmp(cmnd->str[0], "*rats"))
        type = DM_RATS;
    else if(!strcasecmp(cmnd->str[0], "*comet"))
        type = DM_COMET;
    else if(!strcasecmp(cmnd->str[0], "*abyss"))
        type = DM_ABYSS;
    else if(!strcasecmp(cmnd->str[0], "*shard"))
        type = DM_SHARDS;
    else if(!strcasecmp(cmnd->str[0], "*flatulence"))
        type = DM_FLATULENCE;
    else if(!strcasecmp(cmnd->str[0], "*diarreah"))
        type = DM_DIARRHEA;
     else if(!strcasecmp(cmnd->str[0], "*kinetic"))
        type = DM_KINETIC;

    if(cmnd->num >  2) {
        if(!strcasecmp(cmnd->str[2], "-combust"))
            type = DM_COMBUST;
        else if(!strcasecmp(cmnd->str[2], "-slime"))
            type = DM_SLIME;
        else if(!strcasecmp(cmnd->str[2], "-gnats"))
            type = DM_GNATS;
        else if(!strcasecmp(cmnd->str[2], "-trip"))
            type = DM_TRIP;
        else if(!strcasecmp(cmnd->str[2], "-bomb"))
            type = DM_BOMB;
        else if(!strcasecmp(cmnd->str[2], "-nuclear") || !strcmp(cmnd->str[2], "-nuke"))
            type = DM_NUCLEAR;
        else if(!strcasecmp(cmnd->str[2], "-rape"))
            type = DM_RAPE;
        else if(!strcasecmp(cmnd->str[2], "-drain"))
            type = DM_DRAIN;
        else if(!strcasecmp(cmnd->str[2], "-crush"))
            type = DM_CRUSH;
        else if(!strcasecmp(cmnd->str[2], "-missile"))
            type = DM_MISSILE;
        else if(!strcasecmp(cmnd->str[2], "-nomnom"))
            type = DM_NOMNOM;
        else if(!strcasecmp(cmnd->str[2], "-grenade"))
            type = DM_GRENADE;
        else if(!strcasecmp(cmnd->str[2], "-head"))
            type = DM_HEAD_BOOM;
        else if(!strcasecmp(cmnd->str[2], "-stab"))
            type = DM_STAB;
        else if(!strcasecmp(cmnd->str[2], "-suicide"))
            type = DM_SUICIDE;
        else if(!strcasecmp(cmnd->str[2], "-alcohol"))
            type = DM_ALCOHOL;
        else if(!strcasecmp(cmnd->str[2], "-drown"))
            type = DM_DROWNED;
        else if(!strcasecmp(cmnd->str[2], "-dragon"))
            type = DM_DRAGON;
        else if(!strcasecmp(cmnd->str[2], "-choke"))
            type = DM_CHOKE;
        else if(!strcasecmp(cmnd->str[2], "-demon"))
            type = DM_DEMON;
        else if(!strcasecmp(cmnd->str[2], "-rats"))
            type = DM_RATS;
        else if(!strcasecmp(cmnd->str[2], "-comet"))
            type = DM_COMET;
        else if(!strcasecmp(cmnd->str[2], "-abyss"))
            type = DM_ABYSS;
        else if(!strcasecmp(cmnd->str[2], "-shard"))
            type = DM_SHARDS;
        else if(!strcasecmp(cmnd->str[2], "-flatulence"))
            type = DM_FLATULENCE;
        else if(!strcasecmp(cmnd->str[2], "-diarreah"))
            type = DM_DIARRHEA;
         else if(!strcasecmp(cmnd->str[2], "-kinetic"))
            type = DM_KINETIC;
    }

    dmKill(player, victim, type, silent, hpmp_loss, unconscious, uncon_length);
    return(0);
}

//*********************************************************************
//                      dmRepair
//*********************************************************************

int dmRepair(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> creature=nullptr;
    int     a=0, count=0, check=0;

    if(cmnd->num < 2) {
        player->print("Repair who's armor?\n");
        return(0);
    }

    if(!strcmp(cmnd->str[2], "-c"))
        check=1;

    cmnd->str[1][0] = up(cmnd->str[1][0]);
    creature = gServer->findPlayer(cmnd->str[1]);

    if(!creature) {
        player->print("Player not found online.\n");
        return(0);
    }

    if(check)
        player->print("%s's current armor status:\n", creature->getCName());

    for(a=0;a<MAXWEAR;a++) {
        if(!creature->ready[a])
            continue;

        if(check) {
            player->print("%s: [%d/%d](%2f)\n", creature->ready[a]->getCName(),
                  creature->ready[a]->getShotsCur(), creature->ready[a]->getShotsMax(),
                  ((float)creature->ready[a]->getShotsCur()/(float)creature->ready[a]->getShotsMax()));
            continue;
        }
        if(creature->ready[a]->getShotsCur() < creature->ready[a]->getShotsMax()) {
            creature->ready[a]->setShotsCur(creature->ready[a]->getShotsMax());

            player->print("%s's %s repaired to full shots (%d/%d).\n", creature->getCName(), creature->ready[a]->getCName(),
                  creature->ready[a]->getShotsCur(), creature->ready[a]->getShotsMax());
            count++;

        }

    }


    if(count) {
        broadcast(isDm, "^g### %s repaired all of %s's armor.", player->getCName(), creature->getCName());
        log_immort(true, player, "%s repaired all of %s's armor.", player->getCName(), creature->getCName());
    }

    if(!count)
        player->print("%s is wearing nothing that needs to be repaired.\n", creature->getCName());

    return(0);
}

//*********************************************************************
//                      dmMax
//*********************************************************************

int dmMax(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int i=0;

    if(cmnd)
        player->print("Maxing your stats out.\n");

    player->strength.setMax(MAX_STAT_NUM);
    player->dexterity.setMax(MAX_STAT_NUM);
    player->intelligence.setMax(MAX_STAT_NUM);
    player->constitution.setMax(MAX_STAT_NUM);
    player->piety.setMax(MAX_STAT_NUM);

    player->strength.setCur(MAX_STAT_NUM);
    player->dexterity.setCur(MAX_STAT_NUM);
    player->intelligence.setCur(MAX_STAT_NUM);
    player->constitution.setCur(MAX_STAT_NUM);
    player->piety.setCur(MAX_STAT_NUM);

    for(Realm r = MIN_REALM; r<MAX_REALM; r = (Realm)((int)r + 1))
        player->setRealm(10000000, r);
    player->coins.set(100000000, GOLD);

    player->hp.setMax(30000);
    player->hp.setCur(30000);
    player->mp.setMax(30000);
    player->mp.setCur(30000);

    if(player->getName() == "Bane") {
        player->setLevel(69);
        player->setExperience(2100000000);
    } else {
        player->setLevel(MAXALVL);
        player->setExperience(900000000);
    }

    for(i=0; i<MAXSPELL; i++)
        player->learnSpell(i);
    for(i=0; i<gConfig->getMaxSong(); i++)
        player->learnSong(i);

    if(player->getClass() == CreatureClass::BUILDER) {
        player->forgetSpell(S_PLANE_SHIFT);
        player->forgetSpell(S_JUDGEMENT);
        player->forgetSpell(S_ANIMIATE_DEAD);
    }

    player->setAlignment(0);
    return(0);
}


//*********************************************************************
//                      dmBackupPlayer
//*********************************************************************

int dmBackupPlayer(const std::shared_ptr<Player>& player, cmd* cmnd) {

    char    filename[80], restoredFile[80];
    std::shared_ptr<Player> target=nullptr;
    bool    online=false;

    if(cmnd->num < 2) {
        player->print("Backup who?\n");
        return(0);
    }


    if(cmnd->num > 2 && !strcmp(cmnd->str[2], "-r")) {
        cmnd->str[1][0] = up(cmnd->str[1][0]);
        sprintf(filename, "%s/%s.bak.xml", Path::PlayerBackup.c_str(), cmnd->str[1]);
        sprintf(restoredFile, "%s/%s.xml", Path::Player.c_str(), cmnd->str[1]);

        if(fs::exists(filename)) {

            target = gServer->findPlayer(cmnd->str[1]);
            if(target) {
                player->print("%s is currently online.\n%s must be offline for auto-backup restore.\n", target->getCName(), target->upHeShe());
                return(0);
            }

            if(fs::exists(restoredFile))
                unlink(restoredFile);

            link(filename, restoredFile);
            broadcast(isDm, "^g*** %s restored %s from auto-backup.", player->getCName(), cmnd->str[1]);
            player->print("Restoring %s from last auto-backup.\n", cmnd->str[1]);
            log_immort(false,player, "%s restored %s from auto-backup file.\n", player->getCName(), cmnd->str[1]);
        } else {
            player->print("Backup file for that player does not exist.\n");
        }
        return(0);
    }


    cmnd->str[1][0] = up(cmnd->str[1][0]);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target) {
        if(!loadPlayer(cmnd->str[1], target)) {
            player->print("Player does not exist.\n");
            return(0);
        }
    } else
        online = true;


    if(cmnd->num > 2 && !strcmp(cmnd->str[2], "-d")) {
        sprintf(filename, "%s/%s.bak.xml", Path::PlayerBackup.c_str(), target->getCName());
        if(fs::exists(filename)) {
            unlink(filename);
            broadcast(isDm, "^g*** %s deleted %s's backup file.", player->getCName(), target->getCName());
            player->print("Deleted %s.bak from disk.\n", target->getCName());

            log_immort(false,player, "%s deleted backup file for player %s.\n", player->getCName(), target->getCName());
        } else {
            player->print("Backup file for player %s does not exist.\n", target->getCName());

        }

        return(0);
    }


    if(target->save(online, LoadType::LS_BACKUP) > 0) {
        player->print("Backup failed.\n");
        return(0);
    }

    broadcast(isDm, "^g*** %s backed up %s to disk.", player->getCName(), target->getCName());
    player->print("%s has been backed up.\n", target->getCName());

    log_immort(false,player, "%s backed up player %s to disk.\n", player->getCName(), target->getCName());
    return(0);
}


//*********************************************************************
//                      dmChangeStats
//*********************************************************************

int dmChangeStats(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;


    if(cmnd->num < 2) {
        player->print("Allow whom to change their stats?\n");
        return(0);
    }

    cmnd->str[1][0]=up(cmnd->str[1][0]);
    target = gServer->findPlayer(cmnd->str[1]);
    if(!target) {
        player->print("Player not online.\n");
        return(0);
    }

    if(target->flagIsSet(P_DM_INVIS) && !isDm(player)) {
        player->print("Player not online.\n");
        return(0);
    }

    if(!target->flagIsSet(P_CAN_CHANGE_STATS)) {
        target->setFlag(P_CAN_CHANGE_STATS);
        player->print("%s can now choose new stats.\n", target->getCName());
    } else {
        target->clearFlag(P_CAN_CHANGE_STATS);
        player->print("%s can no longer choose new stats.\n", target->getCName());
    }

    log_immort(false, player, "%s set %s to choose new stats\n", player->getCName(), target->getCName());

    return(0);
}



//*********************************************************************
//                      dmJailPlayer
//*********************************************************************

int dmJailPlayer(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int         tm=0, i=0, iTmp=0, len=0, strLen=0;
    long        t=0;
    std::shared_ptr<UniqueRoom> newRoom=nullptr;
    std::shared_ptr<Player> target=nullptr;
    char        *reason=nullptr;
    bool        online=false;

    if(!player->isStaff() && !player->isWatcher())
        return(cmdNoExist(player, cmnd));
    if(!player->isWatcher())
        return(cmdNoAuth(player));

    if(cmnd->num < 2) {
        player->print("Jail whom?\n");
        player->print("Syntax: *jail (player) (minutes) [-b|-r]\n");
        return(0);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);
    if(!target) {
        if(!loadPlayer(cmnd->str[1], target)) {
            player->print("Player does not exist.\n");
            return(0);
        }
    } else
        online = true;

    if(target->isStaff()) {
        player->print("Don't use *jail on staff members.\n");
        return(0);
    }
    if(player == target) {
        player->print("Do not jail yourself.\n");
        return(0);
    }

    if(player->isWatcher() && !isCt(player) && target->isWatcher()) {
        player->print("Do not jail other watchers.\n");
        return(0);
    }

    strLen = cmnd->fullstr.length();

    // This kills all leading whitespace
    while(i<strLen && isspace(cmnd->fullstr[i]))
        i++;

    // This kills the command itself
    while(i<strLen && !isspace(cmnd->fullstr[i]))
        i++;

    // This kills all the white space before the guild
    while(i<strLen && isspace(cmnd->fullstr[i]))
        i++;

    len = strlen(&cmnd->fullstr[i]);

    reason = strstr(&cmnd->fullstr[i], "-r ");
    if(reason) { // There is a reason string then
        iTmp = i;
        while(iTmp < strLen && &cmnd->fullstr[iTmp] != reason)
            iTmp++;
        reason += 3;
        while(isspace(*reason))
            reason++;
        // Kill trailing whitespace
        len = strlen(reason);
        while(isspace(reason[len-1]))
            len--;
        reason[len] = '\0';
    }
    if(iTmp) { // We have a reason, everything else stops at the beginging of the -r
        strLen = iTmp;
        cmnd->fullstr[iTmp] = '\0';
    }
    if(!reason && player->isWatcher()) {
        player->print("You must enter a reason. Use the -r option.\n");
        return(0);
    }

    if(!reason && player->getName() != "Bane") {
        player->print("Use the -r option and enter a reason, assbandit!\n");
        return(0);
    }

    tm = std::max(1, (int)cmnd->val[1]);

    if(player->isWatcher())
        tm = std::min(60, (int)cmnd->val[1]);

    t = (long)tm*60;

    target->lasttime[LT_JAILED].ltime = time(nullptr);
    target->lasttime[LT_JAILED].interval = t;

    target->setFlag(P_JAILED);


    //log_immort(true, player, "%s jailed %s for %d minutes.\n", player->getCName(), target->getCName(), tm);
    if(!reason) {
        logn("log.jail", "%s jailed %s for %d minutes.%s\n", player->getCName(), target->getCName(), tm);
        broadcast(isCt, "^y*** %s jailed %s for %d minutes.%s", player->getCName(), target->getCName(), tm);
    } else {
        logn("log.jail", "%s jailed %s for %d minutes. Reason: %s\n", player->getCName(), target->getCName(), tm, reason);
        broadcast(isCt, "^y*** %s jailed %s for %d minutes. Reason: %s", player->getCName(), target->getCName(), tm, reason);
    }

    if(player->isWatcher())
        logn("log.wjail", "%s jailed %s(%s) for %d minutes. Reason: %s\n",
             player->getCName(), target->getCName(), target->currentLocation.room.displayStr().c_str(), tm, reason);

    CatRef  cr;
    cr.setArea("jail");
    cr.id = 1;

    if(!online) {

        if(!strcmp(cmnd->str[2], "-b"))
            broadcast("^R### Cackling demons drag %s to the Dungeon of Despair.", target->getCName());
        target->currentLocation.room = cr;
        target->currentLocation.mapmarker.reset();

        player->print("%s is now jailed.\n", target->getCName());

    } else {

        if(!loadRoom(cr, newRoom)) {
            player->print("Problem loading room %s.\nAborting.\n", cr.displayStr().c_str());
            return(0);
        } else {
            player->print("%s is now jailed.\n", target->getCName());
            broadcast((std::shared_ptr<Socket> )nullptr, target->getRoomParent(),
                "^RA demonic jailer just arrived.\nThe demonic jailer opens a portal to Hell.\nThe demonic jailer drags %s screaming to the Dungeon of Despair.", target->getCName());

            target->printColor("^RThe demonic jailer grips your soul and drags you to the Dungeon of Despair.\n");
            broadcast(target->getSock(), target->getRoomParent(), "^RThe portal closes with a puff of smoke.");

            target->deleteFromRoom();
            target->addToRoom(newRoom);
            target->printColor("^RThe demonic jailer says, \"Pathetic mortal!\"\nThe demonic jailer spits on you.\nThe demonic jailer vanishes.\n");
//          target->doPetFollow();
        }

        if(!strcmp(cmnd->str[2], "-b"))
            broadcast("^R### Cackling demons drag %s to the Dungeon of Despair.", target->getCName());

    }

    target->save();
    return(0);
}


//*********************************************************************
//                      dmLts
//*********************************************************************

int dmLts(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    int     i=0;
    long    t = time(nullptr);

    if(cmnd-> num < 2) {
        player->print("syntax: *lt <player>\n");
        return(0);
    }
    cmnd->str[1][0] = up(cmnd->str[1][0]);
    target = gServer->findPlayer(cmnd->str[1]);
    if(target == nullptr) {
        player->print("%s is not on.\n", cmnd->str[1]);
        return(0);
    }
    player->print("Now: %ld\n", t);
    for(i = 0 ; i < MAX_LT ; i++ ) {
        player->print("LT %d - %ld %ld %ld (%ld)\n", i, target->lasttime[i].interval,
              target->lasttime[i].ltime, target->lasttime[i].misc, LT(player, i) - t);
    }
    return(0);
}



//*********************************************************************
//                      dmLtClear
//*********************************************************************

int dmLtClear(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
//  int     i=0;

    if(cmnd->num < 2) {
        player->print("syntax: *ltclear <player>\n");
        return(0);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target) {
        player->print("%s is not on.\n", cmnd->str[1]);
        return(0);
    }
    target->lasttime[LT_INVOKE].interval = 0;
//  for(i=0; i<MAX_LT; i++) {
//      target->lasttime[i].interval = 0;
//  }
//
    player->print("Last times plyReset!\n");
    return(0);

}
