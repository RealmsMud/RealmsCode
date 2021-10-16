/*
 * guilds.cpp
 *   Player run guilds.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute ics granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <cctype>                 // for isspace, isalpha
#include <fcntl.h>                // for open, O_APPEND, O_CREAT, O_RDWR
#include <cstdio>                 // for sprintf
#include <cstring>                // for strncmp, strcpy, strlen, strstr
#include <ctime>                  // for ctime, time
#include <unistd.h>               // for close, write

#include "bank.hpp"               // for balance, deleteStatement, deposit
#include "bstring.hpp"            // for bstring, operator+
#include "catRef.hpp"             // for CatRef
#include "cmd.hpp"                // for cmd
#include "commands.hpp"           // for getFullstrText, cmdGuild, cmdGuildHall
#include "config.hpp"             // for Config, gConfig
#include "color.hpp"              // for stripColor
#include "creatures.hpp"          // for Player
#include "flags.hpp"              // for P_CREATING_GUILD, P_AFK, P_PTESTER
#include "global.hpp"             // for PROP_GUILDHALL, PROP_SHOP
#include "guilds.hpp"             // for Guild, GuildCreation, shopStaysWith...
#include "move.hpp"               // for tooFarAway
#include "mud.hpp"                // for SUPPORT_REQUIRED, ACC
#include "paths.hpp"              // for Paths
#include "property.hpp"           // for Property
#include "proto.hpp"              // for broadcast, free_crt, broadcastGuild
#include "rooms.hpp"              // for UniqueRoom, ExitList
#include "server.hpp"             // for Server, gServer, PlayerMap
#include "socket.hpp"             // for Socket
#include "utils.hpp"              // for MAX, MIN
#include "web.hpp"                // for callWebserver
#include "xml.hpp"                // for loadPlayer, loadRoom

//*********************************************************************
//                      Guild
//*********************************************************************

Guild::Guild() {
    name = "";
    leader = "";
    num = 0;
    level = numMembers = pkillsIn = pkillsWon = 0;
    points = 0;
}

bstring Guild::getName() const { return(name); }
unsigned short Guild::getNum() const { return(num); }
bstring Guild::getLeader() const { return(leader); }
long Guild::getLevel() const { return(level); }
int Guild::getNumMembers() const { return(numMembers); }
long Guild::getPkillsIn() const { return(pkillsIn); }
long Guild::getPkillsWon() const { return(pkillsWon); }
long Guild::getPoints() const { return(points); }
void Guild::setName(std::string_view n) { name = n; }
void Guild::setNum(unsigned short n) { num = n; }
void Guild::setLeader(std::string_view l) { leader = l; }
void Guild::setLevel(long l) { level = l; }
void Guild::setNumMembers(int n) { numMembers = n; }
void Guild::setPkillsIn(long pk) { pkillsIn = pk; }
void Guild::setPkillsWon(long pk) { pkillsWon = pk; }
void Guild::setPoints(long p) { points = p; }
void Guild::incLevel(int l) { level += l; }
void Guild::incNumMembers(int n) { numMembers += n; }
void Guild::incPkillsIn(long pk) { pkillsIn += pk; }
void Guild::incPkillsWon(long pk) { pkillsWon += pk; }

//*********************************************************************
//                      guildhallLocations
//*********************************************************************
// Calling function must provide fmt with 2 %s's. Example:
// "Your guildhall is located at %s in the city of %s.\n"

void Guild::guildhallLocations(const Player* player, const char* fmt) const {
    std::list<Property*>::const_iterator it;

    for(it = gConfig->properties.begin() ; it !=  gConfig->properties.end() ; it++) {
        if((*it)->getType() == PROP_GUILDHALL && (*it)->getGuild() == num) {
            player->print(fmt, (*it)->getLocation().c_str(),
                gConfig->catRefName((*it)->getArea()).c_str());
        }
    }
}

//*********************************************************************
//                      GuildCreation
//*********************************************************************

GuildCreation::GuildCreation() {
    name = "";
    leader = leaderIp = "";
    status = numSupporters = 0;
}


// Prototypes

void printGuildSyntax(Player* player) {
    player->printColor("Syntax: guild ^e<^xlist^e>\n");
    player->printColor("              ^e<^xfound^e>^x ^e<^cguild name^e>\n");
    if(!gConfig->getWebserver().empty()) {
        player->printColor("              ^e<^xforum^e>^x :: set your account to guildmaster on the forum\n");
        player->printColor("                      -create :: create a guild and private board on the website\n");
    }
    player->printColor("              ^e<^xcancel^e>\n");
    player->printColor("              ^e<^xsupport^e>^x ^e<^cguild name^e>\n");
    player->printColor("              ^e<^xinvite^e>^x ^e<^cplayer name^e>\n");
    player->printColor("              ^e<^xremove^e>^x ^e<^cplayer name^e>\n");
    player->printColor("              ^e<^xabdicate^e>^x ^e<^cplayer name^e>\n");
    player->printColor("              ^e<^xaccept^e>\n");
    player->printColor("              ^e<^xreject^e>\n");
    player->printColor("              ^e<^xpromote^e>^x ^e<^cplayer name^e>^x ^e[^crank^e]\n");
    player->printColor("              ^e<^xdemote^e>^x ^e<^cplayer name^e>^x ^e[^crank^e]\n");
    player->printColor("              ^e<^xmembers^e>\n");
    player->printColor("              ^e<^xhall^e>\n");
}


//"              <disband>\n";
// Guild Ranks
//          Non Officer     Officer
//          4,  5,  6,      7,  8,  9     Invite flags
//          12, 13, 14,     17, 18, 19    Actual status

const int   GUILD_NONE = 0,
            GUILD_INVITED = 1,
            GUILD_INVITED_OFFICER = 5,
            GUILD_INVITED_BANKER = 6,

            GUILD_PEON = 11,
            GUILD_OFFICER = 15,
            GUILD_BANKER = 16,
            GUILD_MASTER = 20;

// What happened -- Used in adjusting guild level
const int   GUILD_JOIN = 0,
            GUILD_REMOVE = 1,
            GUILD_LEVEL = 2,
            GUILD_DIE = 3;

// Guild Creation Status variables
const int   GUILD_NEEDS_SUPPORT = 1,
            GUILD_AWAITING_APPROVAL = 2;

// level requirements
const int   GUILD_FOUND_LEVEL = 13,
            GUILD_SUPPORT_LEVEL = 10,
            GUILD_JOIN_LEVEL = 7;


//********************************************************************
//                      isGuildKill
//********************************************************************

bool Player::isGuildKill(const Player *killer) const {
    // Dueling guild members don't count
    if(induel(killer, this))
        return(false);

    // Staff members don't count
    if(killer->isStaff() || isStaff())
        return(false);

    // Validate the guild: are they both in one?
    if(!killer->getGuild() || !getGuild())
        return(false);
    // They've reached the lowest level? (ie, skip invited but unconfirmed)
    if(killer->getGuildRank() < GUILD_PEON || getGuildRank() < GUILD_PEON)
        return(false);

    // If they've de-leveled past the join level, they don't count for guild pkilling
    if(killer->getLevel() < GUILD_JOIN_LEVEL || getLevel() < GUILD_JOIN_LEVEL)
        return(false);

    return(true);
}

//*********************************************************************
//                      cmdGuild
//*********************************************************************
// Player command used to do various guild functions

int cmdGuild(Player* player, cmd* cmnd) {
    int     len=0;

    if(cmnd->num < 2) {
        printGuildSyntax(player);
        return(0);
    }

    len = strlen(cmnd->str[1]);

    // Found/Create a new guild
    if( !strncmp(cmnd->str[1], "found", len) ||
        !strncmp(cmnd->str[1], "create", len)
    ) {
        Guild::create(player, cmnd);
    }
    // Cancel the creation of a guild
    else if(!strncmp(cmnd->str[1], "cancel", len)) Guild::cancel(player, cmnd);
    // Create a forum for the guild
    else if(!strncmp(cmnd->str[1], "forum", len)) Guild::forum(player, cmnd);
    // Support the creation of a guild
    else if(!strncmp(cmnd->str[1], "support", len)) Guild::support(player, cmnd);
    // Invite someone into your guild
    else if(!strncmp(cmnd->str[1], "invite", len)) Guild::invite(player, cmnd);
    // Transfer leadership of the guild to someone else
    else if(!strncmp(cmnd->str[1], "abdicate", len)) Guild::abdicate(player, cmnd);
    // Remove someone from your guild
    else if(!strncmp(cmnd->str[1], "remove", len) ||
            !strncmp(cmnd->str[1], "dismiss", len)
    ) {
        Guild::remove(player, cmnd);
    }
    // Accept an offer to join a guild
    else if(!strncmp(cmnd->str[1], "join", len) ||
            !strncmp(cmnd->str[1], "accept", len)
    ) {
        Guild::join(player, cmnd);
    }
    // Reject someone's offer to join a guild
    else if(!strncmp(cmnd->str[1], "reject", len))  Guild::reject(player, cmnd);
    else if(!strcmp(cmnd->str[1], "disband")) Guild::disband(player, cmnd);
    // Promote someone
    else if(!strncmp(cmnd->str[1], "promote", len)) Guild::promote(player, cmnd);
    // Demote someone
    else if(!strncmp(cmnd->str[1], "demote", len)) Guild::demote(player, cmnd);
    // List current guilds
    else if(!strncmp(cmnd->str[1], "list", len)) Guild::list(player, cmnd);
    else if(!strncmp(cmnd->str[1], "members", len)) Guild::viewMembers(player, cmnd);

    // Guild Bank Stuff
    else if(!strncmp(cmnd->str[1], "balance", len)) Bank::balance(player, true);
    else if(!strncmp(cmnd->str[1], "deposit", len)) Bank::deposit(player, cmnd, true);
    else if(!strncmp(cmnd->str[1], "withdraw", len)) Bank::withdraw(player, cmnd, true);
    else if(!strncmp(cmnd->str[1], "transfer", len)) Bank::transfer(player, cmnd, true);
    else if(!strncmp(cmnd->str[1], "statement", len)) Bank::statement(player, true);
    else if(!strncmp(cmnd->str[1], "deletestatement", len)) Bank::deleteStatement(player, true);

    else if(!strncmp(cmnd->str[1], "hall", len)) Property::manage(player, cmnd, PROP_GUILDHALL, 2);
    else
        printGuildSyntax(player);

    return(0);
}


//*********************************************************************
//                      create
//*********************************************************************

void Guild::create(Player* player, cmd* cmnd) {
    int len,i=0,j=0, a=0;
    GuildCreation* newGuildCreation;
    char guildName[41];

    if(player->getLevel() < GUILD_FOUND_LEVEL && !player->isDm()) {
        player->print("You must be level %d to found a guild.\n", GUILD_FOUND_LEVEL);
        return;
    }

    if(player->getGuild() > 0) {
        player->print("You may not create a guild because you already belong to one.\n");
        return;
    }

    // TODO: Check the awaiting approval list to see if they are supporting, or have a guild
    // on the approval list
    if(player->flagIsSet(P_CREATING_GUILD)) {
        player->print("You may only create one guild at a time.\n");
        return;
    }

    if(cmnd->num < 3) {
        printGuildSyntax(player);
        return;
    }

    len = cmnd->fullstr.length();

    // Kill the first 2 spaces
    while(i < len) {
        if(cmnd->fullstr[i] == ' ' && cmnd->fullstr[i+1] != ' ')
            j++;
        if(j==2)
            break;
        i++;
    }

    len = strlen(&cmnd->fullstr[i+1]);
    if(!len) {
        player->print("What would you like to call your guild?\n");
        return;
    }
    if(len > 40) {
        player->print("The guild name is too long.\n");
        return;
    }

    for(a=0;a<len;a++) {
        if(!isalpha(cmnd->fullstr[i+1+a]) && cmnd->fullstr[i+1+a] != ' ') {
            player->print("Name error: Characters other than letters are illegal.\n");
            player->print("Please attempt creation with a different name.\n");
            return;
        }
    }

    strcpy(guildName, &cmnd->fullstr[i+1]);
    if(gConfig->guildExists(guildName)) {
        player->print("A guild with that name already exists or is being considered for creation.\nPlease choose a different guild name.\n");
        return;
    }

    //  newGuild = gConfig->numGuilds + 1;
    //  if(newGuild >= gConfig->maxGuilds)
    //      expandGuildArray();

    newGuildCreation = new GuildCreation;
    newGuildCreation->name = guildName;
    newGuildCreation->leader = player->getName();
    newGuildCreation->leaderIp = player->getSock()->getIp();
    newGuildCreation->status = GUILD_NEEDS_SUPPORT;
    newGuildCreation->numSupporters = 0;

    gConfig->addGuildCreation(newGuildCreation);
    player->setFlag(P_CREATING_GUILD);
    player->printColor("%sYou attempting to create a guild with the name of '%s'.\n", player->customColorize("*CC:GUILD*").c_str(), guildName);
    player->print("Go find %d people to support your guild now.\n", SUPPORT_REQUIRED);
    broadcast(isCt, "^y%s is attempting to create the guild '%s'.", player->getCName(), guildName);
    gConfig->saveGuilds();
}

//*********************************************************************
//                      cancel
//*********************************************************************

void Guild::cancel(Player* player, cmd* cmnd) {
    if(!player->flagIsSet(P_CREATING_GUILD)) {
        player->print("You aren't currently creating any guilds!\n");
        return;
    }
    player->clearFlag(P_CREATING_GUILD);
    bstring guildName = gConfig->removeGuildCreation(player->getName());

    if(!guildName.empty()) {
        player->print("You remove your bid to create '%s'.\n", guildName.c_str());
        return;
    }

    player->print("You aren't creating any guilds!\n");
}

//*********************************************************************
//                      forum
//*********************************************************************
// calls the webserver - no work is done here (besides parsing)

void Guild::forum(Player* player, cmd* cmnd) {
    if(!player->getGuild() || player->getGuildRank() != GUILD_MASTER) {
        player->print("You are not a guildmaster.\n");
        return;
    }

    Guild* guild = gConfig->getGuild(player->getGuild());
    if(!guild || strstr(guild->name.c_str(), "&")) {
        player->print("Invalid guild!\n");
        return;
    }
    if(gConfig->getWebserver().empty()) {
        player->print("The mud currently does not have a webserver configured.\n");
        return;
    }

    if(player->getForum().empty()) {
        player->print("Your player must be associated with a forum account to update or create a guild forum.\n");
        return;
    }

    bstring action = cmnd->str[2];
    std::ostringstream url;
    url << "mud.php?mud_id=" << guild->getNum() << "&char=" << player->getName() << "&type=";

    if(action == "-create") {
        player->printColor("Attempting to create guild forum with you as guildmaster.\n");
        url << "guildCreate&guildName=" << guild->name;
    } else {
        player->printColor("Attempting to update guild forum with you as guildmaster.\n");
        url << "guildAbdicate";
    }

    callWebserver(url.str());
}

//*********************************************************************
//                      support
//*********************************************************************

void Guild::support(Player* player, cmd* cmnd) {
    GuildCreation *toSupport=nullptr;
    char    guildName[41];
    int     len=0, i=0, j=0;

    if(player->getLevel() < GUILD_SUPPORT_LEVEL && !player->isDm()) {
        player->print("You must be level %d to support a guild!\n", GUILD_SUPPORT_LEVEL);
        return;
    }

    if(player->getGuild() > 0) {
        player->print("You may not support a guild because you already belong to one.\n");
        return;
    }
    if(player->flagIsSet(P_CREATING_GUILD)) {
        player->print("You may only support one guild at a time.\n");
        return;
    }

    if(cmnd->num < 3) {
        printGuildSyntax(player);
        return;
    }

    len = cmnd->fullstr.length();

    // Kill the first 2 spaces
    while(i < len) {
        if(cmnd->fullstr[i] == ' ' && cmnd->fullstr[i+1] != ' ')
            j++;
        if(j==2)
            break;
        i++;
    }

    len = strlen(&cmnd->fullstr[i+1]);
    if(!len) {
        player->print("Which guild would you like to support?\n");
        return;
    }
    if(len>40)
        cmnd->fullstr[40] = '\0';

    strcpy(guildName, &cmnd->fullstr[i+1]);
    toSupport = gConfig->findGuildCreation(guildName);
    if(toSupport == nullptr) {
        player->print("'%s' is not currently up for creation.\n", guildName);
        return;
    }
    if(toSupport->status != GUILD_NEEDS_SUPPORT) {
        player->print("'%s' is not currently in need of support.\n", guildName);
        return;
    }


    if(!player->flagIsSet(P_PTESTER)) {
        // check the address
        bool sameIp = (player->getSock()->getIp() == toSupport->leaderIp);

        if(!sameIp) {
            std::map<bstring, bstring>::iterator sIt;
            for(sIt = toSupport->supporters.begin() ; sIt != toSupport->supporters.end() ; sIt++) {
                if((*sIt).second == player->getSock()->getIp()) {
                    sameIp = true;
                    break;
                }
            }
        }
        if(sameIp) {
            player->print("Someone from your IP address is already supporting this guild.\n");
            player->print("Remember, do not use alternate characters to support a guild.\n");
            return;
        }
    }

    player->setFlag(P_CREATING_GUILD);

    player->print("You now supporting the creation of the guild '%s'.\n", guildName);
    broadcast(isCt, "^y%s is supporting the guild '%s'.", player->getCName(), guildName);
    toSupport->addSupporter(player);

    if(toSupport->numSupporters >= SUPPORT_REQUIRED) {
        Player* leader=nullptr;

        toSupport->status = GUILD_AWAITING_APPROVAL;
        // We now have enough players to seek staff approval
        player->print("There are now enough people supporting the guild, it has been submitted for staff approval.\n");
        leader = gServer->findPlayer(toSupport->leader.c_str());
        if(leader)
            leader->printColor("%sThere are now enough people supporting your guild, it has been submitted for staff approval.\n", leader->customColorize("*CC:GUILD*").c_str());
        broadcast(isCt, "^yThe guild '%s' is in need of staff approval.", guildName);
    }

    gConfig->saveGuilds();
}

//*********************************************************************
//                      invite
//*********************************************************************

void Guild::invite(Player* player, cmd* cmnd) {
    Player  *target=nullptr;

    if(!player->getGuild() || player->getGuildRank() < GUILD_OFFICER) {
        player->print("You are not an officer in a guild.\n");
        return;
    }

    if(cmnd->num < 3) {
        player->print("Invite who into your guild?\n");
        return;
    }

    lowercize(cmnd->str[2], 1);
    target = gServer->findPlayer(cmnd->str[2]);

    if(!target || !player->canSee(target)) {
        player->print("%s is not on.\n", cmnd->str[2]);
        return;
    }

    if(Move::tooFarAway(player, target, "invite into your guild"))
        return;

    if(target->getLevel() < GUILD_JOIN_LEVEL) {
        player->print("%s does not meet the minimum required level of %d to join a guild!\n",
            target->getCName(), GUILD_JOIN_LEVEL);
        return;
    }

    if(target->getGuildRank() >= GUILD_INVITED && target->getGuildRank() < GUILD_PEON) {
        player->print("%s is currently considering joining a guild already.\n", target->getCName());
        return;
    }

    if(target->getGuild()) {
        player->print("%s is already in a guild.\n", target->getCName());
        return;
    }

    if(target->isStaff()) {
        player->print("You cannot invite staff members to join a guild.\n");
        return;
    }

    // Set this initially, change it if needed
    target->setGuildRank(GUILD_INVITED);
    target->setGuild(player->getGuild());
    bstring joinType;

    if(player->getGuildRank() == GUILD_MASTER) {
        if(!strncmp(cmnd->str[3], "officer", strlen(cmnd->str[3])) && cmnd->num == 4) {
            joinType = "an officer";
            target->setGuildRank(GUILD_INVITED_OFFICER);
        } else if(!strncmp(cmnd->str[3], "banker", strlen(cmnd->str[3])) && cmnd->num == 4) {
            joinType = "a banker";
            target->setGuildRank(GUILD_INVITED_BANKER);
        } else {
            joinType = "a normal member";
            target->setGuildRank(GUILD_INVITED);
        }

    } else {
        joinType = "a normal member";
        target->setGuildRank(GUILD_INVITED);
    }

    *player << "You invite " << target->getName() << " to join your guild as " << joinType << ".\n";
    *target << "You have been invited to join '" << getGuildName(player->getGuild()) << "' as " << joinType << " by " << player->getName() << ".\n";

    target->printColor("To join type ^yguild accept^x.\n");
}

//*********************************************************************
//                      shopStaysWithGuild
//*********************************************************************

bool shopStaysWithGuild(const UniqueRoom* shop) {
    return(!shop->exits.empty() && shop->exits.front()->target.room.isArea("guild"));
}

//*********************************************************************
//                      remove
//*********************************************************************

void shopRemoveGuild(Property *p, Player* player, UniqueRoom* shop, UniqueRoom* storage);

void Guild::remove(Player* player, cmd* cmnd) {
    Player  *target=nullptr;
    Property* p=nullptr;
    int guildId = player->getGuild();

    if((!player->getGuild() || player->getGuildRank() < GUILD_OFFICER) && cmnd->num >= 3) {
        player->print("You are not an officer. You can't do that.\n");
        return;
    }
    if(!player->getGuild()) {
        player->print("You aren't in a guild!\n");
        return;
    }

    if(cmnd->num < 3) { // Remove yourself
        if(player->getGuildRank() != GUILD_MASTER) {

            if(!canRemovePlyFromGuild(player)) {
                player->print("Guilds cannot drop below 3 players, the guild must be disbanded.\n");
                return;
            }
            updateGuild(player, GUILD_REMOVE);
            *player << "You remove yourself from your guild (" <<  getGuildName(player->getGuild()) << ").\n";
            
            player->setGuild(0);
            broadcastGuild(guildId, 1, "%s has removed %sself from your guild.", player->getCName(), player->himHer());
            player->setGuildRank(GUILD_NONE);

            // Kick them out if they don't belong
            p = gConfig->getProperty(player->currentLocation.room);
            if(p && p->getGuild() == guildId)
                p->expelToExit(player, false);
        } else {
            player->print("As a leader of the guild, you may not leave it.  Transfer ownership to someone else first or Disband.\n");
        }
        return;
    }
    lowercize(cmnd->str[2], 1);
    target = gServer->findPlayer(cmnd->str[2]);
    bool online = true;

    if(!target) {
        online = false;
        if(!loadPlayer(cmnd->str[2], &target)) {
            player->print("That player does not exist.\n");
            return;
        }
    }

    if(target->getGuild() != guildId) {
        player->print("%s is not in your guild!\n", target->getCName());
        return;
    }
    if(target->getGuildRank() >= player->getGuildRank()) {
        player->print("You may only remove members of lower rank than you.\n");
        return;
    }
    if(target->getGuildRank() < GUILD_PEON) {
        player->print("%s is not in your guild!\n", target->getCName());
        return;
    }
    if(!canRemovePlyFromGuild(target)) {
        player->print("Guilds cannot drop below 3 players; the guild must be disbanded.\n");
        return;
    }

    updateGuild(target, GUILD_REMOVE);
    target->setGuild(0);
    target->setGuildRank(GUILD_NONE);
    target->print("You have been removed from your guild by %s.\n", player->getCName());
    player->print("You remove %s from your guild.\n", target->getCName());
    broadcastGuild(guildId, 1, "%s has been removed from your guild by %s.", target->getCName(), player->getCName());

    if(!target->getForum().empty()) {
        player->printColor("%s is associated with forum account ^C%s^x.\n", target->getCName(), target->getForum().c_str());
        player->printColor("You may wish to consider removing this account from the guild forum.\n");
    }

    std::list<Property*>::iterator pt;
    for(pt = gConfig->properties.begin(); pt != gConfig->properties.end(); pt++) {
        if((*pt)->isOwner(target->getName()) && (*pt)->getGuild() == guildId) {
            if((*pt)->getType() == PROP_SHOP) {
                shopRemoveGuild(*pt, target, nullptr, nullptr);
            } else {
                bstring output = (*pt)->getName();
                broadcast(isCt, "^r%s was removed from guild %d.\nProperty \"%s\" belongs to the guild, but is not a shop.", target, guildId, output.c_str());
            }
        }
    }

    // Kick them out if they don't belong
    p = gConfig->getProperty(target->currentLocation.room);
    if(p && p->getGuild() == guildId)
        p->expelToExit(target, !online);

    gConfig->saveProperties();
    if(!online)
        free_crt(target);
}

//*********************************************************************
//                      promote
//*********************************************************************

void Guild::promote(Player* player, cmd* cmnd) {
    Player  *target=nullptr;
    int newRank=0;
    char rank[80];

    if(!player->getGuild() || player->getGuildRank() != GUILD_MASTER) {
        player->print("You are not the guild leader.\n");
        return;
    }
    if(cmnd->num < 3) {
        printGuildSyntax(player);
        return;
    }
    lowercize(cmnd->str[2], 1);
    target = gServer->findPlayer(cmnd->str[2]);
    if(!target || !player->canSee(target)) {
        player->print("%s is not on.\n", cmnd->str[2]);
        return;
    }
    if(target->getGuild() != player->getGuild() || target->getGuildRank() < GUILD_PEON) {
        player->print("%s is not in your guild!\n", target->getCName());
        return;
    }
    if(cmnd->num == 4) {
        if(!strncmp(cmnd->str[3], "banker", strlen(cmnd->str[3])) && cmnd->num == 4) {
            strcpy(rank, "banker");
            newRank = GUILD_BANKER;
        } else { // if(!strncmp(cmnd->str[3], "officer", strlen(cmnd->str[3])) && cmnd->num == 4) {
            strcpy(rank, "officer");
            newRank = GUILD_OFFICER;
        }
    } else {
        if(target->getGuildRank() == GUILD_PEON) {
            newRank = GUILD_OFFICER;
            strcpy(rank, "officer");
        } else if(target->getGuildRank() == GUILD_OFFICER) {
            strcpy(rank, "banker");
            newRank = GUILD_BANKER;
        } else {
            strcpy(rank, "officer");
            newRank = GUILD_OFFICER;
        }
    }

    if(newRank <= target->getGuildRank()) {
        player->print("You cannot promote someone to an equal or lesser rank.\n");
        return;
    }

    target->setGuildRank(newRank);
    player->print("You promote %s to %s.\n", target, rank);
    target->print("You have been promoted to %s by %s.\n", rank, player->getCName());
    broadcastGuild(player->getGuild(), 1, "%s has been promoted to the rank of %s by %s.", target, rank, player->getCName());
}

//*********************************************************************
//                      demote
//*********************************************************************

void Guild::demote(Player* player, cmd* cmnd) {
    Player  *target=nullptr;
    int newRank=0;
    char rank[80];

    if(!player->getGuild() || player->getGuildRank() != GUILD_MASTER) {
        player->print("You are not the guild leader.\n");
        return;
    }
    if(cmnd->num < 3) {
        printGuildSyntax(player);
        return;
    }
    lowercize(cmnd->str[2], 1);
    target = gServer->findPlayer(cmnd->str[2]);
    if(!target || !player->canSee(target)) {
        player->print("%s is not on.\n", cmnd->str[2]);
        return;
    }
    if(target->getGuild() != player->getGuild() || target->getGuildRank() < GUILD_PEON) {
        player->print("%s is not in your guild!\n", target->getCName());
        return;
    }
    if(cmnd->num == 4) {
        if(!strncmp(cmnd->str[3], "officer", strlen(cmnd->str[3])) && cmnd->num == 4) {
            strcpy(rank, "officer");
            newRank = GUILD_OFFICER;
        } else { // if(!strncmp(cmnd->str[3], "member", strlen(cmnd->str[3])) && cmnd->num == 4) {
            strcpy(rank, "member");
            newRank = GUILD_PEON;
        }
    } else {
        if(target->getGuildRank() == GUILD_BANKER) {
            newRank = GUILD_OFFICER;
            strcpy(rank, "officer");
        } else { //if(target->getGuildRank() == GUILD_OFFICER) {
            strcpy(rank, "member");
            newRank = GUILD_PEON;
        }

    }

    if(newRank >= target->getGuildRank()) {
        player->print("You cannot demote someone to an equal or higher rank.\n");
        return;
    }

    target->setGuildRank(newRank);
    player->print("You demote %s to %s.\n", target, rank);
    target->print("You have been demoted to %s rank by %s.\n", rank, player->getCName());
    broadcastGuild(player->getGuild(), 1, "%s has been demoted to %s rank by %s.", target, rank, player->getCName());
}

//*********************************************************************
//                      abdicate
//*********************************************************************

void Guild::abdicate(Player* player, Player* target, bool online) {
    int guildId = player->getGuild();

    if(!guildId || player->getGuildRank() != GUILD_MASTER) {
        player->print("You are not the leader of the guild.\n");
        return;
    }

    if(target->getGuild() != player->getGuild() || target->getGuildRank() < GUILD_PEON) {
        player->print("%s is not in your guild!\n", target->getCName());
        return;
    }


    target->setGuildRank(GUILD_MASTER);
    player->setGuildRank(GUILD_BANKER);

    target->print("You have been promoted to guild leader by %s.\n", player->getCName());
    target->print("Use the \"guild\" command to set your forum account as guildmaster of the guild board.\n");
    player->print("You promote %s to guild leader.\n", target->getCName());

    std::list<Property*>::iterator pt;
    bstring pName;
    for(pt = gConfig->properties.begin(); pt != gConfig->properties.end(); pt++) {
        if( (*pt)->isOwner(player->getName()) &&
            (*pt)->getGuild() == guildId
        ) {
            pName = gConfig->catRefName((*pt)->getArea());
            if((*pt)->getType() == PROP_GUILDHALL) {
                // guildhalls are easy - ownership transfers to the new guildmaster
                (*pt)->setOwner(target->getName());
                player->print("%s assumes ownership of the guildhall in %s.\n", target->getCName(), pName.c_str());
                target->print("You assume ownership of the guildhall in %s.\n", pName.c_str());
            } else if((*pt)->getType() == PROP_SHOP) {
                // shops located inside the guild transfer ownership to the new guildmaster
                CatRef cr = (*pt)->ranges.front().low;
                UniqueRoom* room=nullptr;
                bool transferOwnership = false;

                if(loadRoom(cr, &room)) {
                    // Look for the first exit linking to a guild
                    if(shopStaysWithGuild(room))
                        transferOwnership = true;
                }

                if(transferOwnership) {
                    (*pt)->setOwner(target->getName());
                    player->print("%s assumes ownership of the guild shop in %s.\n", target->getCName(), pName.c_str());
                    target->print("You assume ownership of the guild shop in %s.\n", pName.c_str());
                } else {
                    player->print("You retain ownership of the guild shop in %s.\n", pName.c_str());
                }
            }
        }
    }

    broadcastGuild(player->getGuild(), 1, "%s has been promoted to guild leader by %s.", target->getCName(), player->getCName());
    Guild* guild = gConfig->getGuild(guildId);

    guild->setLeader(target->getName());
    gConfig->saveGuilds();
    gConfig->saveProperties();

    player->save();
    target->save(online);

    std::ostringstream url;
    url << "mud.php?type=guildAbdicate&mud_id=" << guild->getNum() << "&char=" << target->getCName();
    callWebserver(url.str());
}

void Guild::abdicate(Player* player, cmd* cmnd) {
    Player  *target=nullptr;
    int guildId = player->getGuild();

    if((!guildId || player->getGuildRank() != GUILD_MASTER) && cmnd->num >= 3) {
        player->print("You are not the leader of the guild.\n");
        return;
    }

    if(cmnd->num < 3) {
        player->print("Abdicate to whom?\n");
        return;
    }
    lowercize(cmnd->str[2], 1);
    target = gServer->findPlayer(cmnd->str[2]);

    if(!target || !player->canSee(target)) {
        player->print("%s is not on.\n", cmnd->str[2]);
        return;
    }

    Guild::abdicate(player, target, true);
}

//*********************************************************************
//                      join
//*********************************************************************

void Guild::join(Player* player, cmd *cmnd) {
    bstring name = "";
    if(!player->getGuildRank()) {
        player->print("You have not been invited to join any guilds!\n");
        return;
    }
    if(player->getGuildRank() >= GUILD_PEON) {
        player->print("You are already in a guild.\n");
        return;
    }

    if(player->getLevel() < GUILD_JOIN_LEVEL) {
        player->print("You must be at least level %d to join a guild.\n", GUILD_JOIN_LEVEL);
        return;
    }

    if(player->getGuildRank() == GUILD_INVITED_OFFICER) {
        broadcastGuild(player->getGuild(), 1, "%s has joined your guild as an officer.", player->getCName());
        player->setGuildRank(GUILD_OFFICER);
    } else if(player->getGuildRank() == GUILD_INVITED_BANKER) {
        player->setGuildRank(GUILD_BANKER);
        broadcastGuild(player->getGuild(), 1, "%s has joined your guild as a banker.", player->getCName());
    } else { // Default to normal member
        broadcastGuild(player->getGuild(), 1, "%s has joined your guild as a normal member.", player->getCName());
        player->setGuildRank(GUILD_PEON);
    }

    updateGuild(player, GUILD_JOIN);
    name = getGuildName(player->getGuild());
    player->print("You have joined %s.\n", name.c_str());

    if(!player->getForum().empty())
        callWebserver((bstring)"mud.php?type=autoguild&guild=" + name + "&user=" + player->getForum() + "&char=" + player->getCName());

}

//*********************************************************************
//                      reject
//*********************************************************************

void Guild::reject(Player* player, cmd* cmnd) {
    if(!player->getGuild() || player->getGuildRank() >= GUILD_PEON) {
        player->print("You have no offer to reject.\n");
        return;
    }

    *player << "You reject the offer to join " << getGuildName(player->getGuild()) << "\n";
    player->setGuild(0);
    player->setGuildRank(0);
}

//*********************************************************************
//                      disband
//*********************************************************************

void Guild::disband(Player* player, cmd* cmnd) {
    if(!player->getGuild() || player->getGuildRank() < GUILD_MASTER) {
        player->print("You are not the guildmaster.\n");
        return;
    }
    int guildId = player->getGuild();
    Guild* guild = gConfig->getGuild(guildId);


    if(!guild->bank.isZero()) {
        player->print("You quickly swipe the guild's funds of %s!\n", guild->bank.str().c_str());
        player->coins.add(guild->bank);
    }

    broadcastGuild(guildId, 1, "Your guild has been disbanded by %s.", player->getCName());
    Player* ply;
    for(const auto& p : gServer->players) {
        ply = p.second;

        if(!ply->getGuild() || ply->getGuild() != guildId)
            continue;
        ply->setGuild(0);
        ply->setGuildRank(0);
    }

    gConfig->deleteGuild(guildId);
}

//*********************************************************************
//                      cmdGuildHall
//*********************************************************************

int cmdGuildHall(Player* player, cmd* cmnd) {
    Property::manage(player, cmnd, PROP_GUILDHALL, 1);
    return(0);
}

//*********************************************************************
//                      doGuildSend
//*********************************************************************
// aka guildchat

void doGuildSend(const Guild *guild, Player* player, bstring txt) {
    if(!guild) {
        player->print("Invalid guild!\n");
        return;
    }

    broadcastGuild(guild->getNum(), 1, "### %s sent, \"%s\".",
        player->getCName(), escapeColor(txt).c_str());
    txt = escapeColor(txt);
    broadcast(watchingEaves, "^E--- [%s] %s guild sent, \"%s\".",
        guild->getName().c_str(), player->getCName(), txt.c_str());
}

//*********************************************************************
//                      cmdGuildSend
//*********************************************************************

int cmdGuildSend(Player* player, cmd* cmnd) {
    bstring text = "";

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->inJail()) {
        player->print("People in jail do not have that privilage.\n");
        return(0);
    }

    if(!player->getGuild() || player->getGuildRank() < GUILD_PEON) {
        player->print("You do not belong to a guild.\n");
        return(0);
    }

    text = getFullstrText(cmnd->fullstr, 1);
    if(text.empty()) {
        player->print("Send what?\n");
        return(0);
    }

    if(!player->canSpeak()) {
        player->printColor("^yYou are unable to do that right now.\n");
        return(0);
    }

    doGuildSend(gConfig->getGuild(player->getGuild()), player, text);
    return(0);
}

//*********************************************************************
//                      dmApproveGuild
//*********************************************************************

int dmApproveGuild(Player* player, cmd* cmnd) {
    char guildName[41];
    GuildCreation * toApprove;
    int len,i=0,j=0;

    if(cmnd->num < 2) {
        player->print("Approve what guild?\n");
        return(0);
    }

    len = cmnd->fullstr.length();

    // Kill the first space
    while(i < len) {
        if(cmnd->fullstr[i] == ' ' && cmnd->fullstr[i+1] != ' ')
            j++;
        if(j==1)
            break;
        i++;
    }

    len = strlen(&cmnd->fullstr[i+1]);
    if(!len) {
        player->print("Approve what guild?\n");
        return(0);
    }

    if(len>40)
        cmnd->fullstr[40] = '\0';

    strcpy(guildName, &cmnd->fullstr[i+1]);
    toApprove = gConfig->findGuildCreation(guildName);
    if(toApprove == nullptr) {
        player->print("'%s' is not currently up for creation.\n", guildName);
        return(0);
    }
    if(toApprove->status == GUILD_NEEDS_SUPPORT) {
        player->print("'%s' still needs support.\n", guildName);
        return(0);
    }
    player->print("You approve the guild '%s'.\n", guildName);
    broadcast(isCt, "^y%s just approved the guild %s", player->getCName(), guildName);

    gConfig->creationToGuild(toApprove);
    // saved by above function
    //gConfig->saveGuilds();
    return(0);
}

//*********************************************************************
//                      dmRejectGuild
//*********************************************************************

int dmRejectGuild(Player* player, cmd* cmnd) {
    char guildName[41];
    char *reason=nullptr;
    GuildCreation * toReject;
    int len,i=0,j=0, iTmp=0, strLen;

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
    if(!len) {
        player->print("Reject what guild?\n");
        return(0);
    }
    // This finds out how long the guild is
    // Keep going untill we get to the end of the string or a -
    while(i+j < strLen && cmnd->fullstr[i+j] != '\0' && cmnd->fullstr[i+j] != '-')
        j++;
    j = MIN(j, 40);
    memcpy(guildName, &cmnd->fullstr[i], j);
    guildName[j] = '\0';
    // Kill trailling whitespace
    len = strlen(guildName);
    if(!len) {
        player->print("Reject what guild?\n");
        return(0);
    }
    while(isspace(guildName[len-1]))
        len--;
    guildName[len] = '\0';
    player->print("Trying to reject '%s'\n", guildName);
    i = i+j;
    j=0;
    len = strlen(&cmnd->fullstr[i+1]);

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
    if(reason != nullptr) {
        player->print("Reason '%s'\n", reason);
    }
    toReject = gConfig->findGuildCreation(guildName);
    if(toReject == nullptr) {
        player->print("'%s' is not currently up for creation.\n", guildName);
        return(0);
    }
    player->print("You reject the guild '%s'.\n", guildName);
    broadcast(isCt, "^y%s just rejected the guild %s", player->getCName(), guildName);
    rejectGuild(toReject, reason);
    gConfig->saveGuilds();
    return(0);
}

//*********************************************************************
//                      showGuildsNeedingApproval
//*********************************************************************

void showGuildsNeedingApproval(Player* viewer) {
    GuildCreation *gcp;
    int num = 0;
    std::list<GuildCreation*>::iterator it;
    std::ostringstream oStr;

    for(it = gConfig->guildCreations.begin() ; it != gConfig->guildCreations.end() ; it++) {
        gcp = (*it);
        if(gcp->status != GUILD_AWAITING_APPROVAL)
            continue;

        num++;
        if(num == 1) {
            oStr << "\nGuilds needing approval:\n";
            oStr << "--------------------------------------------------------------------------------\n";
        }

        oStr << "^c" << num << ") " << gcp->name << " by " << gcp->leader << ".\n"
             << "   " << gcp->numSupporters << " Supporter"
             << (gcp->numSupporters != 1 ? "s" : "") << ":";

        std::map<bstring, bstring>::iterator sIt;
        for(sIt = gcp->supporters.begin() ; sIt != gcp->supporters.end() ; sIt++)
            oStr << " " << (*sIt).first;

        oStr << "\n";
    }

    viewer->printColor("%s", oStr.str().c_str());
}

//*********************************************************************
//                      viewMembers
//*********************************************************************

void Guild::viewMembers(Player* player, cmd* cmnd) {
    if(cmnd->num < 3) {
        player->print("What guild were you looking for?\n\n");
        Guild::list(player, cmnd);
        return;
    }

    Guild *guild = gConfig->getGuild(player, getFullstrText(cmnd->fullstr, 2));
    if(!guild) {
        player->print("The guild you were looking for could not be found.\n");
        return;
    }

    player->print("Guild:   %s\nMembers: ", guild->name.c_str());

    std::list<bstring>::iterator it;
    bstring toPrint;
    for(it = guild->members.begin() ; it != guild->members.end() ; it++ ) {
        toPrint += (*it);
        toPrint += ", ";
    }
    toPrint[toPrint.length() - 2] = '.';
    player->printColor("^g%s^x\n", toPrint.c_str());

    if(player->isCt() || guild->getNum() == player->getGuild())
        guild->guildhallLocations(player, "Your guildhall is located at %s in %s.\n");
}

//*********************************************************************
//                      list
//*********************************************************************

void Guild::list(Player* player, cmd* cmnd) {
    int num = 1;

    player->printColor("^b    %-25s %-15s  Info\n", "Guild", "Leader");
    player->printColor("^b--------------------------------------------------------------------------------\n");

    for(auto const& [guildId, guild] : gConfig->guilds) {
        player->printColor("%2d> ^c%-25s ^g%-15s ^x%2d Member(s)",
            num++, guild->name.c_str(), guild->leader.c_str(), guild->numMembers);
        if(guild->pkillsIn)
            player->printColor("^r (Pk:%3d%%)", pkillPercent(guild->pkillsWon, guild->pkillsIn));
        else
            player->printColor("^r (Pk: N/A)");
        player->printColor("^y Avg Lvl: %2d\n", guild->averageLevel());
    }

    for(auto const gcp : gConfig->guildCreations) {
        if(gcp->status != GUILD_NEEDS_SUPPORT)
            continue;
        num++;
        if(num == 1) {
            player->print("\nGuilds needing support\n");
            player->print("--------------------------------------------------------------------------------\n");
        }
        player->printColor("^c%d) %s by %s.\n", num, gcp->name.c_str(), gcp->leader.c_str());
        player->print("   %d Supporter%s: ", gcp->numSupporters, gcp->numSupporters != 1 ? "s" : "");

        std::map<bstring, bstring>::iterator sIt;
        for(sIt = gcp->supporters.begin() ; sIt != gcp->supporters.end() ; sIt++) {
            player->print("%s", (*sIt).first.c_str());
        }

        player->print("\n");
    }
}

//*********************************************************************
//                      dmListGuilds
//*********************************************************************
// List currently created guilds, and guilds being opted for creation

int dmListGuilds(Player* player, cmd* cmnd) {
    GuildCreation * gcp;
    int     found = 0;

    //player->print("Max Guild Id:  %d\n", gConfig->maxGuilds);
    player->print("Next Guild Id: %d\n\n", gConfig->getNextGuildId());
    //  gp = firstGuild;

    //  while(gp) {
    GuildMap::iterator it;
    Guild *guild;
    for(it = gConfig->guilds.begin() ; it != gConfig->guilds.end() ; it++) {
        guild = (*it).second;

        player->printColor("^CGuild: ");
        player->printColor("^c%3d %-40s", guild->getNum(), guild->getName().c_str());
        player->print("Leader: %s\n", guild->getLeader().c_str());
        player->print("  NumMembers: %3d ", guild->getNumMembers());
        player->print(" Avg Level: %3d ", guild->averageLevel());
        player->print(" Total Levels %3d\n", guild->getLevel());
        player->printColor("  Members: ^g");
        std::list<bstring>::iterator mIt;
        bstring toPrint;
        for(mIt = guild->members.begin() ; mIt != guild->members.end() ; mIt++ ) {
            toPrint += (*mIt);
            toPrint += ", ";
        }
        toPrint[toPrint.length() - 2] = '.';
        player->printColor("%s\n", toPrint.c_str());

        player->print("  Pkills: %d/%d (%d%%) ", guild->getPkillsWon(), guild->getPkillsIn(), pkillPercent(guild->getPkillsWon(), guild->getPkillsIn()));
        player->print(" Points: %d\n", guild->getPoints());
        player->print("  GuildBank:   %s\n", guild->bank.str().c_str());
        guild->guildhallLocations(player, "  Guildhall: %s in %s\n");
    }
    player->print("\nGuilds being supported by players for creation:\n");
    std::list<GuildCreation*>::iterator gcIt;
    for(gcIt = gConfig->guildCreations.begin() ; gcIt != gConfig->guildCreations.end() ; gcIt++) {
        gcp = (*gcIt);
        found++;
        player->printColor("%s%d) %s by %s.\n", gcp->status == GUILD_NEEDS_SUPPORT ? "^c" : "^g",
                found, gcp->name.c_str(), gcp->leader.c_str());
        player->print("   %d Supporter%s:", gcp->numSupporters, gcp->numSupporters != 1 ? "s" : "");
        std::map<bstring, bstring>::iterator sIt;
        for(sIt = gcp->supporters.begin() ; sIt != gcp->supporters.end() ; sIt++) {
            player->print(" %s", (*sIt).first.c_str());
        }
        player->print("\n");
    }
    if(found == 0)
        player->printColor("^x  None.\n");
    return(0);
}

//*********************************************************************
//                      guild membership functions
//*********************************************************************

bool Guild::addMember(std::string_view memberName) {
    if(!memberName.empty()) {
        members.push_back(memberName);
        return(true);
    }
    return(false);
}

bool Guild::delMember(std::string_view memberName) {
    std::list<bstring>::iterator mIt;

    for(mIt = members.begin() ; mIt != members.end() ; mIt++) {
        if(memberName == (*mIt)) {
            members.erase(mIt);
            return(true);
        }
    }
    return(false);
}

bool Guild::isMember(std::string_view memberName) {
    std::list<bstring>::iterator mIt;

    for(mIt = members.begin() ; mIt != members.end() ; mIt++) {
        if(memberName == (*mIt))
            return(true);
    }
    return(false);
}

void Guild::renameMember(std::string_view oldName, std::string_view newName) {
    std::list<bstring>::iterator mIt;

    if(leader == oldName)
        leader = newName;

    for(mIt = members.begin() ; mIt != members.end() ; mIt++) {
        if(oldName == (*mIt)) {
            (*mIt) = newName;
            break;
        }
    }
}


//*********************************************************************
//                      addGuildCreation
//*********************************************************************

bool Config::addGuildCreation(GuildCreation* toAdd) {
    guildCreations.push_back(toAdd);
    return(true);
}

//*********************************************************************
//                      supporter functions
//*********************************************************************

bool GuildCreation::addSupporter(Player* supporter) {
    if(supporter && supporters.find(supporter->getName()) == supporters.end()) {
        supporters[supporter->getName()] = supporter->getSock()->getIp();
        numSupporters++;
        return(true);
    }
    return(false);
}

bool GuildCreation::removeSupporter(std::string_view supporterName) {
    if(supporters.find(supporterName) != supporters.end()) {
        supporters.erase(supporterName);
        numSupporters--;
        return(true);
    }
    return(false);
}

void GuildCreation::renameSupporter(std::string_view oldName, std::string_view newName) {
    if(leader == oldName)
        leader = newName;

    if(supporters.find(oldName) != supporters.end()) {
        supporters[newName] = supporters[oldName];
        supporters.erase(oldName);
    }
}

void Config::guildCreationsRenameSupporter(std::string_view oldName, std::string_view newName) {
    std::list<GuildCreation*>::iterator gcIt;
    for(gcIt = guildCreations.begin(); gcIt != guildCreations.end(); gcIt++) {
        (*gcIt)->renameSupporter(oldName, newName);
    }
}



//*********************************************************************
//                      findGuildCreation
//*********************************************************************

GuildCreation* Config::findGuildCreation(std::string_view creationName) {
    std::list<GuildCreation*>::iterator gcIt;
    GuildCreation* gcp;
    for(gcIt = guildCreations.begin(); gcIt != guildCreations.end(); gcIt++) {
        gcp = (*gcIt);
        if(gcp->name == creationName)
            return(gcp);
    }
    return(nullptr);
}

//*********************************************************************
//                      recalcLevel
//*********************************************************************
// this function is slow, so we don't want this to happen.

void Guild::recalcLevel() {
    std::list<bstring>::iterator mIt;
    Player* member=nullptr;
    bool    online;

    broadcast(isCt, "\n^yError: Guild level for \"%s\" is being recalculated.\n", name.c_str());

    level = 0;
    for(mIt = members.begin() ; mIt != members.end() ; mIt++) {
        online = false;
        member = gServer->findPlayer((*mIt).c_str());
        if(!member) {
            if(!loadPlayer((*mIt).c_str(), &member))
                continue;
        } else
            online = true;

        level += member->getLevel();

        if(!online)
            free_crt(member);
    }

    gConfig->saveGuilds();
}

//*********************************************************************
//                      averageLevel
//*********************************************************************

int Guild::averageLevel() {
    int cLevel, cMembers;

    if(!level)
        recalcLevel();

    cLevel = MAX(1L, level);
    cMembers = MAX(1, numMembers);

    return(cLevel/cMembers);
}

//*********************************************************************
//                      guildExists
//*********************************************************************

bool Config::guildExists(std::string_view guildName) {
    Guild* guild;
    guild = getGuild(guildName);
    if(guild)
        return(true);

    GuildCreation* gcp;
    gcp = findGuildCreation(guildName);
    if(gcp)
        return(true);

    return(false);
}

//*********************************************************************
//                      getGuildName
//*********************************************************************

bstring getGuildName(int guildNum) {
    Guild* guild = gConfig->getGuild(guildNum);
    if(guild)
        return(guild->getName());
    else
        return("Unknown Guild");
}


//*********************************************************************
//                      canRemovePlyFromGuild
//*********************************************************************
// True only if the guild will still have 3 or more players in it
// otherwise it must be disbanded

bool canRemovePlyFromGuild(Player* player) {
    const Guild* guild  = gConfig->getGuild(player->getGuild());
    if(!guild)
        return(false);
    if(guild->getNumMembers() > 3)
        return(true);
    return(false);
}

//*********************************************************************
//                      updateGuild
//*********************************************************************
// Updates level statistics based upon the player, and 'what' is happening

void updateGuild(Player* player, int what) {
    if(!player || !player->getGuild() || player->getGuildRank() < GUILD_PEON)
        return;
    std::ostringstream url;

    Guild *guild = gConfig->getGuild(player->getGuild());
    if(!guild)
        return;
    if(what == GUILD_JOIN) {
        guild->incLevel(player->getLevel());
        guild->incNumMembers();
        guild->addMember(player->getName());
        url << "mud.php?type=guildAddMember&mud_id=" << guild->getNum() << "&char=" << player->getCName();
    } else if(what == GUILD_REMOVE) {
        bool online=false;

        guild->incNumMembers(-1);

        // There will be nobody is left in the guild!
        if(!guild->getNumMembers()) {
            // This invalidates the "guild" variable. It will also take care of saving
            // the guild and destroying guild  property, so we can stop what we're doing.
            gConfig->deleteGuild(player->getGuild());
            return;
        }

        guild->incLevel(player->getLevel() * -1);
        guild->delMember(player->getName());

        // the guildmaster is leaving and did not abdicate their position!
        // find someone to take their place
        if(player->getGuildRank() == GUILD_MASTER) {
            Player* leader=nullptr;

            do {
                guild->setLeader(guild->members.front());

                leader = gServer->findPlayer(guild->getLeader().c_str());
                if(leader)
                    online = true;
                else {
                    online = false;
                    if(!loadPlayer(guild->getLeader().c_str(), &leader))
                        leader = nullptr;
                }


                if(leader) {
                    Guild::abdicate(player, leader, online);

                    if(!online)
                        free_crt(leader);
                    break;
                }

                // If we couldn't load a member, we've got data integrity problems
                guild->setLevel(0);
                guild->setLeader("");
                guild->incNumMembers(-1);
                guild->members.pop_front();
            } while(!guild->members.empty());
        }

        // If we're removing, find the forum account. If any remaining members are associated
        // with this forum account, don't remove the forum account from the guild
        std::list<bstring>::const_iterator it;

        if(!player->getForum().empty()) {
            Player* target=nullptr;
            bool removeForum = true;

            for(it = guild->members.begin(); it != guild->members.end(); it++) {
                target = gServer->findPlayer((*it).c_str());
                if(target)
                    online = true;
                else {
                    online = false;
                    if(!loadPlayer((*it).c_str(), &target))
                        target = nullptr;
                }


                if(target) {
                    // if another member of this guild has the same forum account, don't remove the account
                    if(player->getForum() == target->getForum())
                        removeForum = false;
                    if(!online)
                        free_crt(target);
                }

                if(!removeForum)
                    break;
            }

            if(removeForum)
                url << "mud.php?type=guildRemoveMember&mud_id=" << guild->getNum() << "&char=" << player->getName();
        }

    } else if(what == GUILD_LEVEL) {
        guild->incLevel();
    } else if(what == GUILD_DIE) {
        guild->incLevel(-1);
    }
    gConfig->saveGuilds();
    callWebserver(url.str());
}

//*********************************************************************
//                      creationToGuild
//*********************************************************************

void Config::creationToGuild(GuildCreation* toApprove) {
    std::map<bstring, bstring>::iterator sIt;
    Player *leader=nullptr, *officer=nullptr;
    int     guildId=0;
    bool    online=false;
    std::ostringstream url;
    url << "mud.php?type=guildCreate&char=" << toApprove->leader;

    // The suicide/*dust commands will take care of leaders / supporters
    // suiciding. These are extra checks in case anyone somehow sneaks past.

    // If the leader doesn't exist, we delete the guild.
    leader = gServer->findPlayer(toApprove->leader.c_str());
    if(!leader) {
        if(!loadPlayer(toApprove->leader.c_str(), &leader)) {
            broadcast(isCt, "^yError creating guild '%s': Can't find the leader %s.\nThis guild is being erased.",
                toApprove->name.c_str(), toApprove->leader.c_str());
            removeGuildCreation(toApprove->leader);
            return;
        }
    } else
        online = true;


    // If they suicide and remake a character with the same name, don't
    // let a level 1 character become the leader of the guild.
    if(!leader->flagIsSet(P_CREATING_GUILD)) {
        broadcast(isCt, "^yError creating guild '%s': %s is not flagged as creating a guild.\nThis guild is being erased.^",
            toApprove->name.c_str(), toApprove->leader.c_str());
        removeGuildCreation(leader->getName());
        if(!online)
            free_crt(leader);
        return;
    }


    // If we can't load the supporters
    for(sIt = toApprove->supporters.begin() ; sIt != toApprove->supporters.end() ; sIt++) {
        if(!gServer->findPlayer((*sIt).first.c_str())) {
            if(!loadPlayer((*sIt).first.c_str(), &officer)) {
                broadcast(isCt, "^yError loading guild supporter %s in guild %s.\nRemoving this person from the support list.", (*sIt).first.c_str(), toApprove->name.c_str());
                toApprove->removeSupporter((*sIt).first);
                return;
            }
            free_crt(officer);
        }
    }


    // Everything has checked out - create the guild.
    guildId = nextGuildId;
    url << "&mud_id=" << guildId;

    nextGuildId++;
    numGuilds++;
    auto* guild = new Guild;

    guild->setName(toApprove->name);
    url << "&guildName=" << toApprove->name;

    guild->setLeader(toApprove->leader);
    guild->setNum(guildId);
    guild->setPkillsWon(0);
    guild->setPkillsIn(0);
    guild->setPoints(0);

    guild->setLevel(leader->getLevel());
    guild->setNumMembers(1);

    // Set the leader
    leader->setGuild(guildId);
    leader->setGuildRank(GUILD_MASTER);
    leader->clearFlag(P_CREATING_GUILD);
    leader->save(online);

    guild->addMember(leader->getName());

    addGuild(guild);

    if(online)
        leader->print("You are now the leader of %s.\n", guild->getName().c_str());

    for(sIt = toApprove->supporters.begin() ; sIt != toApprove->supporters.end() ; sIt++) {
        bool offOnline = false;
        url << "&supporter[]=" << (*sIt).first;

        officer = gServer->findPlayer((*sIt).first.c_str());
        if(!officer) {
            if(!loadPlayer((*sIt).first.c_str(), &officer)) {
                broadcast(isCt, "^yError making %s an officer in %s.", (*sIt).first.c_str(), guild->getName().c_str());
                continue;
            }
        } else
            offOnline = true;
        officer->setGuild(guildId);
        officer->setGuildRank(GUILD_OFFICER);
        officer->clearFlag(P_CREATING_GUILD);
        officer->save(offOnline);

        guild->incNumMembers();
        guild->incLevel( officer->getLevel());
        guild->addMember(officer->getName());
        if(!offOnline)
            free_crt(officer);
        else
            officer->print("You are now an officer of %s.\n", guild->getName().c_str());
    }

    removeGuildCreation(leader->getName());
    if(!online)
        free_crt(leader);

    callWebserver(url.str());
}


//*********************************************************************
//                      rejectGuild
//*********************************************************************

void rejectGuild(GuildCreation * toReject, char *reason) {
    Player  *leader=nullptr, *officer=nullptr;
    bstring leaderName = "";
    int     error = 0, ff;
    char    Reason[250], file[80], outStr[1024], datestr[40];
    long    t;
    bool    online=false;


    if(reason == nullptr)
        strcpy(Reason, "No reason given.\n");
    else
        strcpy(Reason, reason);

    leader = gServer->findPlayer(toReject->leader.c_str());
    if(!leader) {
        if(!loadPlayer(toReject->leader.c_str(), &leader)) {
            broadcast(isCt, "^yError rejecting guild '%s': Can't find the leader %s.", toReject->name.c_str(), toReject->leader.c_str());
            error = 1;
        }
    } else
        online = true;

    if(error == 0) {
        leaderName = leader->getName();
        leader->clearFlag(P_CREATING_GUILD);
        leader->save(online);

        if(!online) {
            // Send them a mudmail
            sprintf(file, "%s/%s.txt", Path::Post, leader->getCName());
            ff = open(file, O_CREAT | O_APPEND | O_RDWR, ACC);
            if(ff > 0) {
                time(&t);
                strcpy(datestr, (char *) ctime(&t));
                datestr[strlen(datestr) - 1] = 0;
                sprintf(outStr, "\n--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--\n\nMail from System (%s):\n\nYour guild '%s' has been rejected.\nReason: %s\n", datestr, toReject->name.c_str(), Reason);
                write(ff, outStr, strlen(outStr));
                close(ff);
                leader->setFlag(P_UNREAD_MAIL);
            }
            free_crt(leader);
        } else
            leader->print("Your guild %s has been rejected.\nReason: %s\n", toReject->name.c_str(), Reason);

    } else
        error = 0;

    std::map<bstring, bstring>::iterator sIt;
    for(sIt = toReject->supporters.begin() ; sIt != toReject->supporters.end() ; sIt++) {
        online = false;
        officer = gServer->findPlayer((*sIt).first.c_str());
        if(!officer) {
            if(!loadPlayer((*sIt).first.c_str(), &officer))
                continue;
        } else
            online = true;
        officer->clearFlag(P_CREATING_GUILD);
        officer->save(online);
        if(!online)
            free_crt(officer);
    }

    gConfig->removeGuildCreation(leaderName);
}


//*********************************************************************
//                      removeGuildCreation
//*********************************************************************

bstring Config::removeGuildCreation(std::string_view leaderName) {
    std::list<GuildCreation*>::iterator it;
    GuildCreation *gc;
    bstring toReturn = "";
    for(it = gConfig->guildCreations.begin() ; it != gConfig->guildCreations.end() ; it++) {
        gc = (*it);
        if(gc && gc->leader == leaderName) {
            // We found the one we want to erase
            guildCreations.erase(it);
            toReturn = gc->name;
            delete gc;
            saveGuilds();
            return(toReturn);
        }
    }
    return("");
}

//*********************************************************************
//                      getGuild
//*********************************************************************

Guild* Config::getGuild(std::string_view name) {
    Guild* guild=nullptr;
    GuildMap::iterator it;
    for(it = guilds.begin(); it != guilds.end(); it++) {
        guild = (*it).second;
        if(guild->getName() == name)
            return(guild);
    }
    return (nullptr);
}

Guild* Config::getGuild(int guildId) {
    if(guilds.find(guildId) != guilds.end())
        return(guilds[guildId]);
    return(nullptr);
}

// Will find a guild based on "txt". If a guild could not be found, it
// will print a message to the player informing them of such.

Guild* Config::getGuild(const Player* player, bstring txt) {
    Guild* guild=nullptr;
    // 0 = not found, -1 = not unique
    int check = 0, len = txt.getLength();
    txt = txt.toLower();

    GuildMap::iterator it;
    for(it = gConfig->guilds.begin(); it != gConfig->guilds.end(); it++) {

        if((*it).second->getName().left(len).toLower() == txt) {
            if(!check) {
                check = 1;
                guild = (*it).second;
            } else
                check = -1;
        }
    }

    if(!check) {
        player->print("Guild not found.\n");
        return(nullptr);
    } else if(check == -1) {
        player->print("Guild name was not unique.\n");
        return(nullptr);
    }

    return(guild);
}

//*********************************************************************
//                      deleteGuild
//*********************************************************************
// When calling this function, it is your responsibility to save the guild
// leader (they get the guild's money).

bool Config::deleteGuild(int guildId) {
    if(guilds.find(guildId) == guilds.end())
        return(false);

    Guild* guild = guilds[guildId];
    guilds.erase(guildId);

    Player *player = gServer->findPlayer(guild->getLeader().c_str());
    bool online = true;
    if(!player) {
        online = false;
        if(!loadPlayer(guild->getLeader().c_str(), &player))
            player = nullptr;
    }

    if(player) {
        // Destroy guild property
        std::list<Property*>::iterator it;
        for(it = gConfig->properties.begin() ; it != gConfig->properties.end() ;) {
            if(guildId != (*it)->getGuild()) {
                it++;
                continue;
            }

            if((*it)->getType() == PROP_SHOP) {
                // If the shop is located inside the guildhall, then destroy it.
                // If it isn't, then unlink it.
                CatRef cr = (*it)->ranges.front().low;
                UniqueRoom* shop=nullptr;
                if(loadRoom(cr, &shop)) {
                    // If the shop belongs to the guild, remove and continue
                    if(!shop->exits.empty() && !shopStaysWithGuild(shop)) {
                        shopRemoveGuild(*it, player, shop, nullptr);
                        it++;
                        continue;
                    }
                }
            }

            (*it)->destroy();
            it = properties.erase(it);
        }

        if(!online)
            free_crt(player);
    }

    std::ostringstream url;
    url << "mud.php?type=guildDisband&mud_id=" << guild->getNum();
    callWebserver(url.str());

    delete guild;
    saveGuilds();
    return(true);
}

//*********************************************************************
//                      addGuild
//*********************************************************************

bool Config::addGuild(Guild* toAdd) {
    if(!toAdd || !toAdd->getNum())
        return(false);
    guilds[toAdd->getNum()] = toAdd;
    return(true);
}


// Clears the global list of guilds
void Config::clearGuildList() {
    GuildMap::iterator it;
    Guild* guild;

    for(it = guilds.begin(); it != guilds.end(); it++) {
        guild = (*it).second;
        delete guild;
        //guilds.erase(it);
    }
    guilds.clear();

    std::list<GuildCreation*>::iterator gcIt;
    GuildCreation* gcp;
    while(!guildCreations.empty()) {
        gcp = guildCreations.front();
        delete gcp;
        guildCreations.pop_front();
    }
    guildCreations.clear();
}
