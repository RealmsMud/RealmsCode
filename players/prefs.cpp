/*
 * prefs.cpp
 *   Player preferences
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

#include <bits/exception.h>                        // for exception
#include <fmt/format.h>                            // for format
#include <strings.h>                               // for strcasecmp
#include <boost/algorithm/string/predicate.hpp>    // for istarts_with
#include <boost/format.hpp>                        // for boost::format
#include <boost/optional/optional.hpp>             // for get_pointer
#include <cstring>                                 // for strcmp, strcpy
#include <ostream>                                 // for operator<<, basic_...
#include <string>                                  // for string, allocator

#include "cmd.hpp"                                 // for cmd
#include "commands.hpp"                            // for cmdPrefs, cmdTelOpts
#include "flags.hpp"                               // for P_NO_SHOW_MAIL
#include "login.hpp"                               // for ANSI_COLOR, NO_COLOR
#include "mudObjects/players.hpp"                  // for Player
#include "mudObjects/rooms.hpp"                    // for BaseRoom
#include "proto.hpp"                               // for isCt, isDm, isStaff
#include "server.hpp"                              // for Server, gServer
#include "socket.hpp"                              // for Socket


// having a pref that starts with a hyphen (-) is instead a category
typedef struct prefInfo {
    std::string name;
    int     flag;
    bool    (*canUse)(const std::shared_ptr<Creature> &);
    std::string desc;
    bool    invert;                 // reverses the language
} prefInfo, *prefPtr;

prefInfo prefList[] =
{
    // name         flag                    canUse  desc
    { "-Staff Preferences", 0, isStaff, "", false },
    { "eaves",      P_EAVESDROPPER,         isCt,   "eavesdropper mode",        false },
    { "supereaves", P_SUPER_EAVESDROPPER,   isCt,   "super eavesdropper mode",  false },
    { "msg",        P_NO_MSG,               isCt,   "*msg channel",             true },
    { "mobtick",    P_NO_TICK_MSG,          isCt,   "See mob tick broadcasts",  true },
    { "mobaggro",   P_NO_AGGRO_MSG,         isCt,   "See mob aggro broadcasts", true },
    { "mobdeath",   P_NO_DEATH_MSG,         isCt,   "See mob death broadcasts", true },
    { "ttobound",   P_T_TO_BOUND,           isStaff,"*t to bound room",         false },
    { "wts",        P_NO_WTS,               isCt,   "*wts channel",             true },
    { "zones",      P_VIEW_ZONES,           isCt,   "view overland zones",      false },
    { "autoinvis",  P_AUTO_INVIS,           isCt,   "auto-invis if idle for 100+ mins",     false },
    { "allhooks",   P_SEE_ALL_HOOKS,        isDm,   "see all hooks",            false },
    { "hooks",      P_SEE_HOOKS,            isDm,   "see triggered hooks",      false },

    { "-Color",     0, nullptr, "", false },
    { "ansi",       P_ANSI_COLOR,           nullptr,      "ansi colors",              false },
    { "langcolor",  P_LANGUAGE_COLORS,      nullptr,      "language colors",          false },
    { "extracolor", P_NO_EXTRA_COLOR,       nullptr,      "extra color options",      true },
    { "areacolor",  P_INVERT_AREA_COLOR,    nullptr,      "invert area colors",       false },
    { "mxpcolors",  P_MXP_ENABLED,          nullptr,      "use MXP for more than 16 colors",      false },

    { "-Channels",  0, nullptr, "", false },
    { "cls",        P_IGNORE_CLASS_SEND,    nullptr,      "class channel",            true },
    { "race",       P_IGNORE_RACE_SEND,     nullptr,      "race channel",             true },
    { "broad",      P_NO_BROADCASTS,        nullptr,      "broadcast channel",        true },
    { "newbie",     P_IGNORE_NEWBIE_SEND,   nullptr,      "newbie channel",           true },
    { "gossip",     P_IGNORE_GOSSIP,        nullptr,      "gossip channel",           true },
    { "clan",       P_IGNORE_CLAN,          nullptr,      "clan channel",             true },
    { "tells",      P_NO_TELLS,             nullptr,      "send/tell/whisper/sign",   true },
    { "sms",        P_IGNORE_SMS,           nullptr,      "receive text messages",    true },
    { "ignore",     0,                      nullptr,      "ignore all channels",      false },
    // do something with P_IGNORE_ALL

    { "-Notifications", 0, nullptr, "", false },
    { "duel",       P_NO_DUEL_MESSAGES,     nullptr,      "duel messages",            true },
    { "login",      P_NO_LOGIN_MESSAGES,    nullptr,      "login messages",           true },
    { "shopprofit", P_DONT_SHOW_SHOP_PROFITS,nullptr,     "shop profit notifications",true },
    { "mail",       P_NO_SHOW_MAIL,         nullptr,      "mudmail notifications",    true },
    { "permdeath",  P_PERM_DEATH,           nullptr,      "perm death broadcasts",    false },
    { "auction",    P_NO_AUCTIONS,          nullptr,      "player auctions",          true },
    { "showforum",  P_HIDE_FORUM_POSTS,     nullptr,      "notification of forum posts", true },
    { "forumbrevity",P_FULL_FORUM_POSTS,    nullptr,      "forum posts notification length", true },
    { "notifications",0,                    nullptr,      "show all notifications",   false },

    { "-Combat",    0, nullptr, "", false },
    { "autoattack", P_NO_AUTO_ATTACK,       nullptr,      "auto attack",              true },
    { "lagprotect", P_LAG_PROTECTION_SET,   nullptr,      "lag protection",           false },
    { "lagrecall",  P_NO_LAG_HAZY,          nullptr,      "recall potion if below half hp",true },
    { "wimpy",      P_WIMPY,                nullptr,      "flee when HP below this number",false },
    { "killaggros", P_KILL_AGGROS,          nullptr,      "attack aggros first",      false },
    { "mobnums",    P_NO_NUMBERS,           nullptr,      "monster ordinal numbers",  true },
    { "autotarget", P_NO_AUTO_TARGET,       nullptr,      "automatic targeting",      true },
    { "fleetarget", P_CLEAR_TARGET_ON_FLEE, nullptr,      "clear target on flee",     false },

    { "-Group",     0, nullptr, "", false },
    { "group",      P_IGNORE_GROUP_BROADCAST,nullptr,     "group combat messages",    true },
    { "xpsplit",    P_XP_DIVIDE,            nullptr,      "group experience split",   false },
    { "split",      P_GOLD_SPLIT,           nullptr,      "split gold among group",   false },
    { "stats",      P_NO_SHOW_STATS,        nullptr,      "show group your stats",    true },
    { "follow",     P_NO_FOLLOW,            nullptr,      "can be followed",          true },

    { "-Display",   0, nullptr, "", false },
    { "showall",    P_SHOW_ALL_PREFS,       nullptr,      "show all preferences",     false },
    { "nlprompt",   P_NEWLINE_AFTER_PROMPT, nullptr,      "newline after prompt",     false },
    { "xpprompt",   P_SHOW_XP_IN_PROMPT,    nullptr,      "show exp in prompt",       false },
    { "prompt",     P_PROMPT,               nullptr,      "descriptive prompt",       false },
    { "compact",    P_COMPACT,              nullptr,      "compacts most output",     false },
    { "tick",       P_SHOW_TICK,            nullptr,      "shows ticks",              false },
    { "skillprogress", P_SHOW_SKILL_PROGRESS,nullptr,     "show skill progress bar",  false },
    { "durability", P_SHOW_DURABILITY_INDICATOR,nullptr,  "show durability indicator",  false },

    { "-Miscellaneous", 0, nullptr, "", false },
    { "summon",     P_NO_SUMMON,            nullptr,      "can be summoned",          true },
    //{ "pkill",        P_NO_PKILL_PERCENT,     0,      "show pkill percentage",    true },
    { "afk",        P_AFK,                  nullptr,      "away from keyboard",       false },
    { "statistics", P_NO_TRACK_STATS,       nullptr,      "track statistics",         true },

    // handle wimpy below
    // handle afk below

    { "", 0, nullptr, "" }
};

//*********************************************************************
//                      cmdTelOpts
//*********************************************************************

int cmdTelOpts(const std::shared_ptr<Player>& player, cmd* cmnd) {
    Socket* sock = player->getSock();
    std::shared_ptr<Player> target = player;


    if(!sock)
        return(0);
    if(cmnd->num > 1) {
        if(!strcasecmp(cmnd->str[1], "wrap")) {
            player->print("Setting wrap to '%d'\n", player->setWrap(cmnd->val[1]));
            return(0);
        } else {
            if(player->isCt()) {
                cmnd->str[1][0] = up(cmnd->str[1][0]);
                target = gServer->findPlayer(cmnd->str[1]);
                if(target && (!target->isDm() || player->isDm())) {
                    sock = target->getSock();
                    if(!sock) {
                        player->print("No socket.\n");
                        return(0);
                    }
                } else {
                    player->print("Unknown option.\n");
                    return(0);
                }
            } else {
                player->print("Unknown option.\n");
                return(0);
            }
        }
    }

    std::ostringstream oStr;
    boost::format formatWithDesc("%1% (%2%) %|40t|%3%\n");
    boost::format formatNoDesc("%1% ^c%|42t|%2%^x\n");
    formatNoDesc.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit )  );
    formatWithDesc.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit )  );
    try {
        if(target != player)
            oStr << "^cNegotiated Telnet Options for " << target->getName() << "^x\n";
        else
            oStr << "^cNegotiated Telnet Options^x\n";

        oStr << "-------------------------------------------------------\n";
        oStr << formatNoDesc % "Term" % sock->getTermType();
        oStr << formatNoDesc % "Term Size" % std::string(std::to_string(sock->getTermCols()) + " x " + std::to_string(sock->getTermRows()));
        if(player->getWrap() == 0)
            oStr << formatNoDesc % "Server Linewrap" % "None";
        else if(player->getWrap() == -1)
            oStr << formatNoDesc % "Server Linewrap" % (fmt::format("Using Cols ({})", sock->getTermCols()));
        else
            oStr << formatNoDesc % "Server Linewrap" % target->getWrap();
        oStr << formatWithDesc % "MCCP" % "Mud Client Compression Protocol" % ((sock->mccpEnabled() != 0) ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "MXP" % "MUD eXtension Protocol" % (sock->mxpEnabled() ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "MSDP" % "MUD Server Data Protocol" % (sock->msdpEnabled() ? "^gon^x" : "^roff^x");
        if (sock->msdpEnabled()) {
            oStr << "\t MSDP Reporting: " << sock->getMsdpReporting() << "\n";

        }
        oStr << formatWithDesc % "Charset" % "Charset Negotiation" % (sock->charsetEnabled() ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "UTF-8" % "UTF-8 Support" % (sock->utf8Enabled() ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "MSP" % "Mud Sound Protocol" % (sock->mspEnabled() ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "EOR" % "End of Record" % (sock->eorEnabled() ? "^gon^x" : "^roff^x");
    } catch (std::exception &e) {
        std::clog << e.what() << std::endl;
    }

    player->printColor("%s", oStr.str().c_str());
    //player->print("  %-13s ::  %-31s ::  ", prefList[i].name.c_str(), prefList[i].desc.c_str());
    return(0);
}

//*********************************************************************
//                      cmdPrefs
//*********************************************************************

int cmdPrefs(const std::shared_ptr<Player>& player, cmd* cmnd) {
    prefPtr pref = nullptr;
    int     i=0, match=0, len=strlen(cmnd->str[1]);
    bool    set = cmnd->str[0][0]=='s';
    bool    toggle = cmnd->str[0][0]=='t' || cmnd->str[0][0]=='p';
    bool    show=false;
    bool all = !strcmp(cmnd->str[1], "-all") || (len == 0 && player->flagIsSet(P_SHOW_ALL_PREFS));
    std::string prefName;

    if(!player->ableToDoCommand())
        return(0);

    // display a list of preferences
    if(cmnd->num == 1 || cmnd->str[1][0] == '-') {

        if(!len || !all) {
            player->print("Below is a list of preference categories. To view a cateogry, type\n");
            player->print("\"pref -[category]\". To view all preferences, type \"pref -all\".\n\n");
        }

        while(!prefList[i].name.empty()) {
            if(!prefList[i].canUse || prefList[i].canUse(player)) {

                if(prefList[i].name.at(0) == '-') {
                    // previous category was shown? newline
                    if(show)
                        player->print("\n");
                    // no parameters? indent
                    if(!len)
                        player->print("  ");

                    prefName = prefList[i].name;
                    show = all || (len && boost::istarts_with(prefName, cmnd->str[1]));

                    player->printColor("^%s%s^w\n", show ? "C" : "c", prefList[i].name.substr(1, prefList[i].name.length() - 1).c_str());
                } else {
                    if(!show) {
                        i++;
                        continue;
                    }
                    player->print("  %-13s ::  %-31s ::  ", prefList[i].name.c_str(), prefList[i].desc.c_str());
                    // wimpy gets its own special format
                    if(prefList[i].name == "wimpy" && player->flagIsSet(prefList[i].flag))
                        player->print("%d", player->getWimpy());
                    else if(prefList[i].name == "forumbrevity")
                        player->print("%s", player->flagIsSet(prefList[i].flag) ? "full" : "brief");
                    // print everything exp ignore
                    else if(prefList[i].flag)
                        player->printColor("%s^w", player->flagIsSet(prefList[i].flag) ^ !prefList[i].invert ? "^roff" : "^gon");
                    player->print("\n");
                }

            }
            i++;
        }
        player->print("\n");
        return(0);
    }


    // everyone is used to nosummon - let them keep it that way
    if(!strcmp(cmnd->str[1], "nosummon")) {
        if(!toggle)
            set = !set;
        strcpy(cmnd->str[1], "summon");
    }


    // we're setting or clearing a preference
    // find out which one
    while(!prefList[i].name.empty()) {
        if( prefList[i].name.at(0) != '-' &&
            (!prefList[i].canUse || prefList[i].canUse(player))
        ) {

            if(!strcmp(cmnd->str[1], prefList[i].name.c_str())) {
                match = 1;
                pref = &prefList[i];
                break;
            } else if(!strncmp(cmnd->str[1], prefList[i].name.c_str(), len)) {
                match++;
                pref = &prefList[i];
            }

        }
        i++;
    }

    if(match > 1) {
        player->print("Preference not unique.\n");
        return(0);
    }
    if(!pref) {
        player->print("Unknown preference.\n");
        return(0);
    }


    // if any channels are being ignored, toggle = stop ignorning
    if(toggle && pref->name == "ignore") {
        toggle = false;
        set = (!player->flagIsSet(P_IGNORE_CLASS_SEND) &&
            !player->flagIsSet(P_IGNORE_RACE_SEND) &&
            !player->flagIsSet(P_NO_BROADCASTS) &&
            !player->flagIsSet(P_IGNORE_NEWBIE_SEND) &&
            !player->flagIsSet(P_IGNORE_GOSSIP) &&
            !player->flagIsSet(P_IGNORE_CLAN) &&
            !player->flagIsSet(P_NO_TELLS)
        );
    }

    // if any notifications are off, toggle = turn them all on
    if(toggle && pref->name == "notifications") {
        toggle = false;
        set = !(player->flagIsSet(P_NO_DUEL_MESSAGES) ||
            !player->flagIsSet(P_NO_LOGIN_MESSAGES) ||
            !player->flagIsSet(P_DONT_SHOW_SHOP_PROFITS) ||
            !player->flagIsSet(P_NO_SHOW_MAIL) ||
            !player->flagIsSet(P_NO_SHOW_MAIL) ||
            !player->flagIsSet(P_IGNORE_GROUP_BROADCAST) ||
            player->flagIsSet(P_PERM_DEATH) ||
            !player->flagIsSet(P_NO_AUCTIONS) ||
            !player->flagIsSet(P_HIDE_FORUM_POSTS)
        );
    }

    // set
    //---------------

    //NORMAL
    // toggle
    // !flag

    // toggle
    // invert && flag

    // are we setting or clearing?
    if( ((set ^ pref->invert) && !toggle) ||
        (toggle && !player->flagIsSet(pref->flag))
    ) {

        if(pref->flag == P_MXP_ENABLED) {
            if(!player->getSock()->mxpEnabled()) {
                player->print("Your client does not support MXP!\n");
                return(0);
            }
            player->getSock()->defineMxp();
        }

        if(pref->name == "ansi") {
            player->getSock()->setColorOpt(ANSI_COLOR);
        }

        if(pref->name == "ignore") {
            player->setFlag(P_IGNORE_CLASS_SEND);
            player->setFlag(P_IGNORE_RACE_SEND);
            player->setFlag(P_NO_BROADCASTS);
            player->setFlag(P_IGNORE_NEWBIE_SEND);
            player->setFlag(P_IGNORE_GOSSIP);
            player->setFlag(P_IGNORE_CLAN);
            player->setFlag(P_NO_TELLS);
        } else if(pref->name == "notifications") {
            player->clearFlag(P_NO_DUEL_MESSAGES);
            player->clearFlag(P_NO_LOGIN_MESSAGES);
            player->clearFlag(P_DONT_SHOW_SHOP_PROFITS);
            player->clearFlag(P_NO_SHOW_MAIL);
            player->clearFlag(P_IGNORE_GROUP_BROADCAST);
            player->setFlag(P_PERM_DEATH);
            player->clearFlag(P_NO_AUCTIONS);
            player->clearFlag(P_HIDE_FORUM_POSTS);
        } else
            player->setFlag(pref->flag);

        if(pref->name == "afk") {
            // broadcast for going afk
            if(!player->flagIsSet(P_DM_INVIS))
                broadcast(player->getSock(), player->getParent(), "%M has gone afk.", player.get());
            else
                broadcast(isCt, player->getSock(), player->getRoomParent(), "*DM* %M has gone afk.", player.get());
        }
        if(pref->name == "wimpy") {
            player->setWimpy(cmnd->val[1] == 1L ? 10 : cmnd->val[1]);
            player->setWimpy(std::max<unsigned short>(player->getWimpy(), 2));
        }
        set = true;

    } else {
        if(pref->name == "ansi") {
            player->getSock()->setColorOpt(NO_COLOR);
        }

        if(pref->name == "ignore") {
            player->clearFlag(P_IGNORE_CLASS_SEND);
            player->clearFlag(P_IGNORE_RACE_SEND);
            player->clearFlag(P_NO_BROADCASTS);
            player->clearFlag(P_IGNORE_NEWBIE_SEND);
            player->clearFlag(P_IGNORE_GOSSIP);
            player->clearFlag(P_IGNORE_CLAN);
            player->clearFlag(P_NO_TELLS);
        } else if(pref->name == "notifications") {
            player->setFlag(P_NO_DUEL_MESSAGES);
            player->setFlag(P_NO_LOGIN_MESSAGES);
            player->setFlag(P_DONT_SHOW_SHOP_PROFITS);
            player->setFlag(P_NO_SHOW_MAIL);
            player->setFlag(P_IGNORE_GROUP_BROADCAST);
            player->clearFlag(P_PERM_DEATH);
            player->setFlag(P_NO_AUCTIONS);
            player->setFlag(P_HIDE_FORUM_POSTS);
        } else
            player->clearFlag(pref->flag);

        if(pref->name == "wimpy")
            player->setWimpy(0);

        set = false;

    }


    // give them the message
    player->print("Preference : %s : ", pref->desc.c_str());
    if(pref->name == "wimpy" && player->flagIsSet(pref->flag))
        player->print("%d", player->getWimpy());
    else if(pref->name == "forumbrevity")
        player->print("%s", player->flagIsSet(pref->flag) ? "full" : "brief");
    else
        player->printColor("%s^w", set ^ !pref->invert ? "^roff" : "^gon");
    player->print(".\n");

    if(pref->flag == P_NO_TRACK_STATS)
        player->print("Note that player kill statistics are always tracked and cannot be reset.\n");

    return(0);
}
